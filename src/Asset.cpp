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

	void Asset::SetTextureLoader(std::function<Texture(FilePathView, MipmapEnabledYN)> loader)
	{
		TextureLoader() = std::move(loader);
	}

	const Texture& Asset::ReloadTexture(FilePathView filePath)
	{
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
		return unloaded;
	}

	void Asset::UnloadAllTextures()
	{
		detail::TextureAssetTable().clear();
		detail::MipmappedTextureAssetTable().clear();
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
