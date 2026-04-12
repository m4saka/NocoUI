#include "NocoUI/Asset.hpp"
#include <filesystem>

namespace noco
{
	namespace
	{
		FilePath s_baseDirectoryPath = U"";

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

			return std::filesystem::path(path.toUTF8()).is_absolute();
		}
	}

	const FilePath& Asset::GetBaseDirectoryPath()
	{
		return s_baseDirectoryPath;
	}

	void Asset::SetBaseDirectoryPath(FilePathView baseDirectoryPath)
	{
		UnloadAllTextures();
		UnloadAllAudios();
		UnloadAllJSONs();
		s_baseDirectoryPath = baseDirectoryPath;
	}

	FilePath Asset::GetFullPath(FilePathView filePath)
	{
		return IsAbsolutePath(filePath)
			? FilePath{ filePath }
			: FileSystem::PathAppend(s_baseDirectoryPath, filePath);
	}

	const Texture& Asset::GetOrLoadTexture(FilePathView filePath)
	{
		static const Texture EmptyTexture{};

		if (filePath.isEmpty())
		{
			return EmptyTexture;
		}

		auto& table = detail::TextureAssetTable();
		if (!table.isRegistered(filePath))
		{
			const FilePath fullPath = GetFullPath(filePath);
			if (!FileSystem::IsFile(fullPath))
			{
				return EmptyTexture;
			}
			table.registerAsset(filePath, Texture{ fullPath });
		}

		return table.get(filePath);
	}

	const Texture& Asset::ReloadTexture(FilePathView filePath)
	{
		auto& table = detail::TextureAssetTable();
		if (table.isRegistered(filePath))
		{
			table.unregister(filePath);
			const FilePath fullPath = GetFullPath(filePath);
			table.registerAsset(filePath, Texture{ fullPath });
			return table.get(filePath);
		}
		return Asset::GetOrLoadTexture(filePath);
	}

	bool Asset::UnloadTexture(FilePathView filePath)
	{
		auto& table = detail::TextureAssetTable();
		if (table.isRegistered(filePath))
		{
			table.unregister(filePath);
			return true;
		}
		return false;
	}

	void Asset::UnloadAllTextures()
	{
		detail::TextureAssetTable().clear();
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
