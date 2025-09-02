#include "NocoUI/Asset.hpp"

namespace noco
{
	namespace
	{
		FilePath s_baseDirectoryPath = U"";
	}

	FilePathView Asset::GetBaseDirectoryPath()
	{
		return s_baseDirectoryPath;
	}

	void Asset::SetBaseDirectoryPath(FilePathView baseDirectoryPath)
	{
		UnloadAllTextures();
		UnloadAllAudios();
		s_baseDirectoryPath = baseDirectoryPath;
	}

	Texture Asset::GetOrLoadTexture(FilePathView filePath)
	{
		if (filePath.isEmpty())
		{
			return Texture{};
		}
		const String key = AssetNamePrefix + filePath;
		if (!TextureAsset::IsRegistered(key))
		{
			const String combinedPath = FileSystem::PathAppend(s_baseDirectoryPath, filePath);
			if (!FileSystem::IsFile(combinedPath))
			{
				return Texture{};
			}
			TextureAsset::Register(key, combinedPath);
		}
		return TextureAsset(key);
	}

	Texture Asset::ReloadTexture(FilePathView filePath)
	{
		if (TextureAsset::IsRegistered(AssetNamePrefix + filePath))
		{
			TextureAsset::Release(AssetNamePrefix + filePath);
			TextureAsset::Load(AssetNamePrefix + filePath);
			return TextureAsset(AssetNamePrefix + filePath);
		}
		return Asset::GetOrLoadTexture(filePath);
	}

	bool Asset::UnloadTexture(FilePathView filePath)
	{
		if (TextureAsset::IsRegistered(AssetNamePrefix + filePath))
		{
			TextureAsset::Unregister(AssetNamePrefix + filePath);
			return true;
		}
		return false;
	}

	void Asset::UnloadAllTextures()
	{
		const HashTable<AssetName, AssetInfo> allAssetTable = TextureAsset::Enumerate();
		for (const auto& [name, assetInfo] : allAssetTable)
		{
			if (name.starts_with(AssetNamePrefix))
			{
				TextureAsset::Unregister(name);
			}
		}
	}

	Audio Asset::GetOrLoadAudio(FilePathView filePath)
	{
		if (filePath.isEmpty())
		{
			return Audio{};
		}
		const String key = AssetNamePrefix + filePath;
		if (!AudioAsset::IsRegistered(key))
		{
			const String combinedPath = FileSystem::PathAppend(s_baseDirectoryPath, filePath);
			if (!FileSystem::IsFile(combinedPath))
			{
				return Audio{};
			}
			AudioAsset::Register(key, combinedPath);
		}
		return AudioAsset(key);
	}

	Audio Asset::ReloadAudio(FilePathView filePath)
	{
		if (AudioAsset::IsRegistered(AssetNamePrefix + filePath))
		{
			AudioAsset::Release(AssetNamePrefix + filePath);
			AudioAsset::Load(AssetNamePrefix + filePath);
			return AudioAsset(AssetNamePrefix + filePath);
		}
		return Asset::GetOrLoadAudio(filePath);
	}

	bool Asset::UnloadAudio(FilePathView filePath)
	{
		if (AudioAsset::IsRegistered(AssetNamePrefix + filePath))
		{
			AudioAsset::Unregister(AssetNamePrefix + filePath);
			return true;
		}
		return false;
	}

	void Asset::UnloadAllAudios()
	{
		const HashTable<AssetName, AssetInfo> allAssetTable = AudioAsset::Enumerate();
		for (const auto& [name, assetInfo] : allAssetTable)
		{
			if (name.starts_with(AssetNamePrefix))
			{
				AudioAsset::Unregister(name);
			}
		}
	}
}
