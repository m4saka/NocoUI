#include "NocoUI/Asset.hpp"
#include <exception>
#include <filesystem>

namespace noco
{
	namespace
	{
		FilePath& BaseDirectoryPath()
		{
			static FilePath baseDirectoryPath;
			return baseDirectoryPath;
		}

		const FilePath& BaseDirectoryPathConst()
		{
			return BaseDirectoryPath();
		}

		std::function<Texture(FilePathView, MipmapEnabledYN)>& TextureLoader()
		{
			static std::function<Texture(FilePathView, MipmapEnabledYN)> loader;
			return loader;
		}

		// 描画サイズを下回らない範囲で、縦横を1/√2に縮小する回数を計算
		int32 CalculateDownscaleLevel(const Size& imageSize, const SizeF& drawSize, PreserveAspectYN preserveAspect)
		{
			if (drawSize.x <= 0.0 || drawSize.y <= 0.0)
			{
				return 0;
			}

			// 元画像が描画サイズの何倍あるかを縦横それぞれで計算
			const double ratioX = imageSize.x / drawSize.x;
			const double ratioY = imageSize.y / drawSize.y;

			// アスペクト比を保持する場合は縦横同じ倍率で描画されるため大きい方に合わせる
			// 保持しない場合は縦横とも描画サイズを下回らないよう小さい方に合わせる
			const double ratio = preserveAspect ? Max(ratioX, ratioY) : Min(ratioX, ratioY);

			// 描画サイズに近すぎるとバイリニア補間でぼやけるため、描画サイズの√2倍より大きく2倍以下に縮小する
			const double level = Math::Ceil(2.0 * Math::Log2(ratio) - 2.0);
			if (level < 1.0)
			{
				return 0;
			}

			// 描画サイズが極端に小さい場合でもint32に変換できるよう制限
			return static_cast<int32>(Min(level, 60.0));
		}

		// 元画像のパスに対応するDownscaledTextureSetを取得(未登録の場合は画像サイズを取得して登録する。ファイルが存在しない場合はnullptr)
		detail::DownscaledTextureSet* FindOrRegisterDownscaledTextureSet(FilePathView filePath)
		{
			if (filePath.isEmpty())
			{
				return nullptr;
			}

			auto& table = detail::DownscaledTextureAssetTable();
			if (const auto pTextureSet = table.find(filePath))
			{
				return pTextureSet;
			}

			const FilePath fullPath = Asset::GetFullPath(filePath);
			if (!FileSystem::IsFile(fullPath))
			{
				return nullptr;
			}

			// 毎フレーム読み直さないよう、取得できなかった場合もnoneとして登録する
			Optional<Size> imageSize;
			if (const Optional<ImageInfo> imageInfo = ImageDecoder::GetImageInfo(fullPath))
			{
				imageSize = imageInfo->size;
			}
			return &table.registerAsset(filePath, detail::DownscaledTextureSet{ .imageSize = imageSize });
		}

		struct DownscaleInfo
		{
			// 元画像のパスに対応するDownscaledTextureSet
			detail::DownscaledTextureSet* pTextureSet;

			// 縦横を1/√2に縮小する回数
			int32 downscaleLevel;
		};

		// 描画サイズに合わせて縮小する場合の情報を取得(縮小しない場合はnone)
		Optional<DownscaleInfo> GetDownscaleInfo(FilePathView filePath, const SizeF& drawSize, PreserveAspectYN preserveAspect)
		{
			// SetTextureLoaderで読み込み処理を差し替えている場合は縮小しない
			if (TextureLoader())
			{
				return none;
			}

			const auto pTextureSet = FindOrRegisterDownscaledTextureSet(filePath);
			if (pTextureSet == nullptr || !pTextureSet->imageSize)
			{
				return none;
			}

			const int32 downscaleLevel = CalculateDownscaleLevel(*pTextureSet->imageSize, drawSize, preserveAspect);
			if (downscaleLevel <= 0)
			{
				return none;
			}
			return DownscaleInfo{ .pTextureSet = pTextureSet, .downscaleLevel = downscaleLevel };
		}

		// 画像を読み込み、downscaleLevelが指定されている場合は縦横を1/√2に縮小する
		Image DecodeImage(FilePathView fullPath, const Optional<int32>& downscaleLevel)
		{
			Image image{ fullPath };
			if (image && downscaleLevel)
			{
				const double scale = Math::Pow(2.0, -*downscaleLevel / 2.0);
				const Size size{ Max(static_cast<int32>(Math::Round(image.width() * scale)), 1), Max(static_cast<int32>(Math::Round(image.height() * scale)), 1) };
				image = image.scaled(size, InterpolationAlgorithm::Area);
			}
			return image;
		}

		// 同時に別スレッドで読み込むテクスチャの数の上限
		constexpr size_t MaxAsyncTextureLoads = 2;

		// 別スレッドでのテクスチャ読み込みの要求
		struct AsyncTextureLoadRequest
		{
			FilePath filePath;

			MipmapEnabledYN mipmapEnabled = MipmapEnabledYN::No;

			// 縮小したテクスチャの場合は縦横を1/√2に縮小する回数
			Optional<int32> downscaleLevel;
		};

		struct AsyncTextureLoad
		{
			AsyncTextureLoadRequest request;

			AsyncTask<Image> task;
		};

		Array<AsyncTextureLoadRequest>& PendingAsyncTextureLoads()
		{
			static Array<AsyncTextureLoadRequest> requests;
			return requests;
		}

		Array<AsyncTextureLoad>& RunningAsyncTextureLoads()
		{
			static Array<AsyncTextureLoad> loads;
			return loads;
		}

		// 読み込みが完了するまでキャッシュに登録しておく空のテクスチャを取得(キャッシュから削除された場合はnullptr)
		Texture* FindAsyncTextureLoadPlaceholder(const AsyncTextureLoadRequest& request)
		{
			if (request.downscaleLevel)
			{
				const auto pTextureSet = detail::DownscaledTextureAssetTable().find(request.filePath);
				if (pTextureSet == nullptr)
				{
					return nullptr;
				}
				auto& textures = pTextureSet->textures;
				const auto it = textures.find(*request.downscaleLevel);
				return (it != textures.end() && it->second.isEmpty()) ? &it->second : nullptr;
			}

			auto& table = request.mipmapEnabled ? detail::MipmappedTextureAssetTable() : detail::TextureAssetTable();
			const auto pTexture = table.find(request.filePath);
			return (pTexture != nullptr && pTexture->isEmpty()) ? pTexture : nullptr;
		}

		// 読み込みが完了するまでキャッシュに登録しておく空のテクスチャを削除(読み込み済みのテクスチャに置き換わっている場合は何もしない)
		void RemoveAsyncTextureLoadPlaceholder(const AsyncTextureLoadRequest& request)
		{
			if (!FindAsyncTextureLoadPlaceholder(request))
			{
				return;
			}

			if (request.downscaleLevel)
			{
				detail::DownscaledTextureAssetTable().get(request.filePath).textures.erase(*request.downscaleLevel);
				return;
			}

			auto& table = request.mipmapEnabled ? detail::MipmappedTextureAssetTable() : detail::TextureAssetTable();
			table.unregister(request.filePath);
		}

		Texture CreateTexture(const Image& image, const AsyncTextureLoadRequest& request)
		{
			return Texture{ image, request.mipmapEnabled ? TextureDesc::Mipped : TextureDesc::Unmipped };
		}

		void StartPendingAsyncTextureLoads()
		{
			auto& pending = PendingAsyncTextureLoads();
			auto& running = RunningAsyncTextureLoads();
			while (running.size() < MaxAsyncTextureLoads && !pending.isEmpty())
			{
				AsyncTextureLoadRequest request = std::move(pending.front());
				pending.pop_front();

				// 開始前にキャッシュから削除された場合は読み込まない
				Texture* placeholder = FindAsyncTextureLoadPlaceholder(request);
				if (!placeholder)
				{
					continue;
				}

				const FilePath fullPath = Asset::GetFullPath(request.filePath);
				AsyncTask<Image> task = Async(DecodeImage, fullPath, request.downscaleLevel);
				if (!task.isValid())
				{
					// 別スレッドを使用できない環境(pthread無効のWeb版)では、このスレッドで読み込む
					*placeholder = CreateTexture(DecodeImage(fullPath, request.downscaleLevel), request);
					continue;
				}
				running.push_back(AsyncTextureLoad{ .request = std::move(request), .task = std::move(task) });
			}
		}

		void RequestAsyncTextureLoad(AsyncTextureLoadRequest&& request)
		{
			PendingAsyncTextureLoads().push_back(std::move(request));
			StartPendingAsyncTextureLoads();
		}

		/// @brief パスが絶対パスかどうかを判定
		/// @param path 判定するパス
		/// @return 絶対パスの場合true
		[[nodiscard]]
		bool IsAbsolutePath(FilePathView path) noexcept
		{
			if (path.isEmpty())
			{
				return false;
			}

			try
			{
#ifdef _WIN32
				return std::filesystem::path(path.toWstr()).is_absolute();
#else
				return std::filesystem::path(path.toUTF8()).is_absolute();
#endif
			}
			catch (const std::exception& e)
			{
				Logger << U"[NocoUI warning] Failed to check whether path is absolute. Treating as relative path: '{}' ({})"_fmt(path, Unicode::Widen(e.what()));
				return false;
			}
			catch (...)
			{
				Logger << U"[NocoUI warning] Failed to check whether path is absolute. Treating as relative path: '{}'"_fmt(path);
				return false;
			}
		}
	}

	const FilePath& Asset::GetBaseDirectoryPath()
	{
		return BaseDirectoryPathConst();
	}

	void Asset::SetBaseDirectoryPath(FilePathView baseDirectoryPath)
	{
		UnloadAllTextures();
		UnloadAllAudios();
		UnloadAllJSONs();
		BaseDirectoryPath() = baseDirectoryPath;
	}

	FilePath Asset::GetFullPath(FilePathView filePath)
	{
		return IsAbsolutePath(filePath)
			? FilePath{ filePath }
			: FileSystem::PathAppend(BaseDirectoryPathConst(), filePath);
	}

	const Texture& Asset::GetOrLoadTexture(FilePathView filePath, MipmapEnabledYN mipmapEnabled)
	{
		static const Texture EmptyTexture{};

		if (filePath.isEmpty())
		{
			return EmptyTexture;
		}

		auto& table = mipmapEnabled ? detail::MipmappedTextureAssetTable() : detail::TextureAssetTable();
		if (const auto pTexture = table.find(filePath))
		{
			return *pTexture;
		}

		const FilePath fullPath = GetFullPath(filePath);
		if (!FileSystem::IsFile(fullPath))
		{
			return EmptyTexture;
		}
		if (const auto& loader = TextureLoader())
		{
			return table.registerAsset(filePath, loader(filePath, mipmapEnabled));
		}
		return table.registerAsset(filePath, Texture{ fullPath, mipmapEnabled ? TextureDesc::Mipped : TextureDesc::Unmipped });
	}

	void Asset::PrewarmTexture(FilePathView filePath, MipmapEnabledYN mipmapEnabled)
	{
		static_cast<void>(GetOrLoadTexture(filePath, mipmapEnabled));
	}

	void Asset::PrewarmTextureAsync(FilePathView filePath, MipmapEnabledYN mipmapEnabled)
	{
		if (TextureLoader())
		{
			PrewarmTexture(filePath, mipmapEnabled);
			return;
		}

		if (filePath.isEmpty())
		{
			return;
		}

		// 読み込み済みまたは読み込み中の場合は何もしない
		auto& table = mipmapEnabled ? detail::MipmappedTextureAssetTable() : detail::TextureAssetTable();
		if (table.isRegistered(filePath))
		{
			return;
		}

		if (!FileSystem::IsFile(GetFullPath(filePath)))
		{
			return;
		}

		// 読み込みが完了するまでは空のテクスチャを登録しておく
		table.registerAsset(filePath, Texture{});
		RequestAsyncTextureLoad(AsyncTextureLoadRequest{ .filePath = FilePath{ filePath }, .mipmapEnabled = mipmapEnabled });
	}

	Optional<Size> Asset::GetImageSize(FilePathView filePath)
	{
		if (const auto pTextureSet = FindOrRegisterDownscaledTextureSet(filePath))
		{
			return pTextureSet->imageSize;
		}
		return none;
	}

	const Texture& Asset::GetOrLoadDownscaledTexture(FilePathView filePath, const SizeF& drawSize, PreserveAspectYN preserveAspect)
	{
		const Optional<DownscaleInfo> downscaleInfo = GetDownscaleInfo(filePath, drawSize, preserveAspect);
		if (!downscaleInfo)
		{
			return GetOrLoadTexture(filePath, MipmapEnabledYN::No);
		}

		const int32 downscaleLevel = downscaleInfo->downscaleLevel;
		auto& textures = downscaleInfo->pTextureSet->textures;
		if (!textures.contains(downscaleLevel))
		{
			textures.emplace(downscaleLevel, Texture{ DecodeImage(GetFullPath(filePath), downscaleLevel), TextureDesc::Unmipped });
		}

		return textures.at(downscaleLevel);
	}

	void Asset::PrewarmDownscaledTexture(FilePathView filePath, const SizeF& drawSize, PreserveAspectYN preserveAspect)
	{
		static_cast<void>(GetOrLoadDownscaledTexture(filePath, drawSize, preserveAspect));
	}

	void Asset::PrewarmDownscaledTextureAsync(FilePathView filePath, const SizeF& drawSize, PreserveAspectYN preserveAspect)
	{
		if (TextureLoader())
		{
			PrewarmDownscaledTexture(filePath, drawSize, preserveAspect);
			return;
		}

		const Optional<DownscaleInfo> downscaleInfo = GetDownscaleInfo(filePath, drawSize, preserveAspect);
		if (!downscaleInfo)
		{
			PrewarmTextureAsync(filePath, MipmapEnabledYN::No);
			return;
		}

		// 読み込み済みまたは読み込み中の場合は何もしない
		const int32 downscaleLevel = downscaleInfo->downscaleLevel;
		auto& textures = downscaleInfo->pTextureSet->textures;
		if (textures.contains(downscaleLevel))
		{
			return;
		}

		// 読み込みが完了するまでは空のテクスチャを登録しておく
		textures.emplace(downscaleLevel, Texture{});
		RequestAsyncTextureLoad(AsyncTextureLoadRequest{ .filePath = FilePath{ filePath }, .downscaleLevel = downscaleLevel });
	}

	void Asset::CancelPendingAsyncTextureLoads()
	{
		auto& pending = PendingAsyncTextureLoads();
		for (const auto& request : pending)
		{
			RemoveAsyncTextureLoadPlaceholder(request);
		}
		pending.clear();
	}

	void Asset::SetTextureLoader(std::function<Texture(FilePathView, MipmapEnabledYN)> loader)
	{
		TextureLoader() = std::move(loader);
	}

	const Texture& Asset::ReloadTexture(FilePathView filePath)
	{
		// 縮小したテクスチャは次に使われた時に作り直す
		detail::DownscaledTextureAssetTable().unregister(filePath);

		// Unmipped版とMipped版が両方キャッシュされている可能性があるため、登録済みのものをそれぞれ再読み込みする
		const FilePath fullPath = GetFullPath(filePath);
		auto& table = detail::TextureAssetTable();
		auto& mippedTable = detail::MipmappedTextureAssetTable();
		const bool hasUnmipped = table.isRegistered(filePath);
		const bool hasMipped = mippedTable.isRegistered(filePath);
		if (hasUnmipped)
		{
			table.unregister(filePath);
			table.registerAsset(filePath, Texture{ fullPath, TextureDesc::Unmipped });
		}
		if (hasMipped)
		{
			mippedTable.unregister(filePath);
			mippedTable.registerAsset(filePath, Texture{ fullPath, TextureDesc::Mipped });
		}
		if (hasUnmipped)
		{
			return table.get(filePath);
		}
		if (hasMipped)
		{
			return mippedTable.get(filePath);
		}
		return Asset::GetOrLoadTexture(filePath, MipmapEnabledYN::No);
	}

	bool Asset::UnloadTexture(FilePathView filePath)
	{
		bool unloaded = false;
		auto& table = detail::TextureAssetTable();
		if (table.isRegistered(filePath))
		{
			table.unregister(filePath);
			unloaded = true;
		}
		auto& mippedTable = detail::MipmappedTextureAssetTable();
		if (mippedTable.isRegistered(filePath))
		{
			mippedTable.unregister(filePath);
			unloaded = true;
		}
		auto& downscaledTable = detail::DownscaledTextureAssetTable();
		if (downscaledTable.isRegistered(filePath))
		{
			downscaledTable.unregister(filePath);
			unloaded = true;
		}
		return unloaded;
	}

	void Asset::UnloadAllTextures()
	{
		detail::TextureAssetTable().clear();
		detail::MipmappedTextureAssetTable().clear();
		detail::DownscaledTextureAssetTable().clear();
	}

	const Audio& Asset::GetOrLoadAudio(FilePathView filePath)
	{
		static const Audio EmptyAudio{};

		if (filePath.isEmpty())
		{
			return EmptyAudio;
		}

		auto& table = detail::AudioAssetTable();
		if (const auto pAudio = table.find(filePath))
		{
			return *pAudio;
		}

		const FilePath fullPath = GetFullPath(filePath);
		if (!FileSystem::IsFile(fullPath))
		{
			return EmptyAudio;
		}
		return table.registerAsset(filePath, Audio{ fullPath });
	}

	const Audio& Asset::ReloadAudio(FilePathView filePath)
	{
		auto& table = detail::AudioAssetTable();
		if (table.isRegistered(filePath))
		{
			table.unregister(filePath);
			const FilePath fullPath = GetFullPath(filePath);
			table.registerAsset(filePath, Audio{ fullPath });
			return table.get(filePath);
		}
		return Asset::GetOrLoadAudio(filePath);
	}

	bool Asset::UnloadAudio(FilePathView filePath)
	{
		auto& table = detail::AudioAssetTable();
		if (table.isRegistered(filePath))
		{
			table.unregister(filePath);
			return true;
		}
		return false;
	}

	void Asset::UnloadAllAudios()
	{
		detail::AudioAssetTable().clear();
	}

	const JSON& Asset::GetOrLoadJSON(FilePathView filePath)
	{
		static const JSON EmptyJSON{};

		if (filePath.isEmpty())
		{
			return EmptyJSON;
		}

		auto& table = detail::JSONAssetTable();
		if (!table.isRegistered(filePath))
		{
			const FilePath fullPath = GetFullPath(filePath);
			if (!FileSystem::IsFile(fullPath))
			{
				return EmptyJSON;
			}
			const JSON json = JSON::Load(fullPath);
			if (!json)
			{
				return EmptyJSON;
			}
			table.registerAsset(filePath, json);
		}

		return table.get(filePath);
	}

	const JSON& Asset::ReloadJSON(FilePathView filePath)
	{
		auto& table = detail::JSONAssetTable();
		if (table.isRegistered(filePath))
		{
			table.unregister(filePath);
			const FilePath fullPath = GetFullPath(filePath);
			const JSON json = JSON::Load(fullPath);
			table.registerAsset(filePath, json);
			return table.get(filePath);
		}
		return Asset::GetOrLoadJSON(filePath);
	}

	bool Asset::UnloadJSON(FilePathView filePath)
	{
		auto& table = detail::JSONAssetTable();
		if (table.isRegistered(filePath))
		{
			table.unregister(filePath);
			return true;
		}
		return false;
	}

	void Asset::UnloadAllJSONs()
	{
		detail::JSONAssetTable().clear();
	}

	void detail::UpdateAsyncTextureLoads()
	{
		auto& running = RunningAsyncTextureLoads();
		for (auto it = running.begin(); it != running.end();)
		{
			if (!it->task.isReady())
			{
				++it;
				continue;
			}

			const Image image = it->task.get();

			// 読み込み中にキャッシュから削除されていなければ、空のテクスチャを読み込んだものに置き換える
			if (Texture* placeholder = FindAsyncTextureLoadPlaceholder(it->request))
			{
				*placeholder = CreateTexture(image, it->request);
			}
			it = running.erase(it);
		}

		StartPendingAsyncTextureLoads();
	}

	bool detail::HasAsyncTextureLoads()
	{
		return !PendingAsyncTextureLoads().isEmpty() || !RunningAsyncTextureLoads().isEmpty();
	}

	void detail::ShutdownAsyncTextureLoads()
	{
		PendingAsyncTextureLoads().clear();
		for (auto& load : RunningAsyncTextureLoads())
		{
			load.task.wait();
		}
		RunningAsyncTextureLoads().clear();
	}
}
