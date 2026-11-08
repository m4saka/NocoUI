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
		if (!table.isRegistered(filePath))
		{
			const FilePath fullPath = GetFullPath(filePath);
			if (!FileSystem::IsFile(fullPath))
			{
				return EmptyTexture;
			}
			if (const auto& loader = TextureLoader())
			{
				table.registerAsset(filePath, loader(filePath, mipmapEnabled));
			}
			else
			{
				table.registerAsset(filePath, Texture{ fullPath, mipmapEnabled ? TextureDesc::Mipped : TextureDesc::Unmipped });
			}
		}

		return table.get(filePath);
	}

	void Asset::PrewarmTexture(FilePathView filePath, MipmapEnabledYN mipmapEnabled)
	{
		static_cast<void>(GetOrLoadTexture(filePath, mipmapEnabled));
	}

	Optional<Size> Asset::GetImageSize(FilePathView filePath)
	{
		if (filePath.isEmpty())
		{
			return none;
		}

		auto& table = detail::DownscaledTextureAssetTable();
		if (!table.isRegistered(filePath))
		{
			const FilePath fullPath = GetFullPath(filePath);
			if (!FileSystem::IsFile(fullPath))
			{
				return none;
			}

			// 毎フレーム読み直さないよう、取得できなかった場合もnoneとして登録する
			Optional<Size> imageSize;
			if (const Optional<ImageInfo> imageInfo = ImageDecoder::GetImageInfo(fullPath))
			{
				imageSize = imageInfo->size;
			}
			table.registerAsset(filePath, detail::DownscaledTextureSet{ .imageSize = imageSize });
		}

		return table.get(filePath).imageSize;
	}

	const Texture& Asset::GetOrLoadDownscaledTexture(FilePathView filePath, const SizeF& drawSize, PreserveAspectYN preserveAspect)
	{
		int32 downscaleLevel = 0;
		if (!TextureLoader())
		{
			if (const Optional<Size> imageSize = GetImageSize(filePath))
			{
				downscaleLevel = CalculateDownscaleLevel(*imageSize, drawSize, preserveAspect);
			}
		}

		if (downscaleLevel <= 0)
		{
			return GetOrLoadTexture(filePath, MipmapEnabledYN::No);
		}

		auto& textures = detail::DownscaledTextureAssetTable().get(filePath).textures;
		if (!textures.contains(downscaleLevel))
		{
			Texture texture;
			if (const Image image{ GetFullPath(filePath) })
			{
				const double scale = Math::Pow(2.0, -downscaleLevel / 2.0);
				const Size size{ Max(static_cast<int32>(Math::Round(image.width() * scale)), 1), Max(static_cast<int32>(Math::Round(image.height() * scale)), 1) };
				texture = Texture{ image.scaled(size, InterpolationAlgorithm::Area), TextureDesc::Unmipped };
			}
			textures.emplace(downscaleLevel, std::move(texture));
		}

		return textures.at(downscaleLevel);
	}

	void Asset::PrewarmDownscaledTexture(FilePathView filePath, const SizeF& drawSize, PreserveAspectYN preserveAspect)
	{
		static_cast<void>(GetOrLoadDownscaledTexture(filePath, drawSize, preserveAspect));
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
		if (!table.isRegistered(filePath))
		{
			const FilePath fullPath = GetFullPath(filePath);
			if (!FileSystem::IsFile(fullPath))
			{
				return EmptyAudio;
			}
			table.registerAsset(filePath, Audio{ fullPath });
		}
		return table.get(filePath);
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
}
