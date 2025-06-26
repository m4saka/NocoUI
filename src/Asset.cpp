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
		if (filePath.isEmpty() || !FileSystem::IsFile(filePath))
		{
			return Texture();
		}
		if (!TextureAsset::IsRegistered(filePath))
		{
			TextureAsset::Register(AssetNamePrefix + filePath, FileSystem::PathAppend(s_baseDirectoryPath, filePath));
		}
		return TextureAsset(AssetNamePrefix + filePath);
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
		if (filePath.isEmpty() || !FileSystem::IsFile(filePath))
		{
			return Audio();
		}
		if (!AudioAsset::IsRegistered(filePath))
		{
			AudioAsset::Register(AssetNamePrefix + filePath, FileSystem::PathAppend(s_baseDirectoryPath, filePath));
		}
		return AudioAsset(AssetNamePrefix + filePath);
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
