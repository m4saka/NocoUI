#include "NocoUI/Asset.hpp"

namespace noco
{
	namespace
	{
		FilePath s_baseDirectoryPath = U"";

		// Siv3DのAssetシステムを直接使うとユーザー側のアセットとの衝突回避用にPrefixを付与する必要が生じてコストがかかるため、別途管理する
		template <class T>
		class AssetTable
		{
		private:
			HashTable<AssetName, T> m_table;

		public:
			bool isRegistered(FilePathView filePath) const
			{
				return m_table.contains(filePath);
			}

			void registerAsset(FilePathView filePath, const T& asset)
			{
				m_table.emplace(filePath, asset);
			}

			void unregister(FilePathView filePath)
			{
				m_table.erase(filePath);
			}

			const T& get(FilePathView filePath) const
			{
				return m_table.at(filePath);
			}

			void clear()
			{
				m_table.clear();
			}
		};

		AssetTable<Texture> s_textureAssetTable;
		AssetTable<Audio> s_audioAssetTable;
	}

	const FilePath& Asset::GetBaseDirectoryPath()
	{
		return s_baseDirectoryPath;
	}

	void Asset::SetBaseDirectoryPath(FilePathView baseDirectoryPath)
	{
		UnloadAllTextures();
		UnloadAllAudios();
		s_baseDirectoryPath = baseDirectoryPath;
	}

	const Texture& Asset::GetOrLoadTexture(FilePathView filePath)
	{
		static const Texture EmptyTexture{};

		if (filePath.isEmpty())
		{
			return EmptyTexture;
		}

		if (!s_textureAssetTable.isRegistered(filePath))
		{
			const String combinedPath = FileSystem::PathAppend(s_baseDirectoryPath, filePath);
			if (!FileSystem::IsFile(combinedPath))
			{
				return EmptyTexture;
			}
			s_textureAssetTable.registerAsset(filePath, Texture{ combinedPath });
		}

		return s_textureAssetTable.get(filePath);
	}

	const Texture& Asset::ReloadTexture(FilePathView filePath)
	{
		if (s_textureAssetTable.isRegistered(filePath))
		{
			s_textureAssetTable.unregister(filePath);
			s_textureAssetTable.registerAsset(filePath, Texture{ FileSystem::PathAppend(s_baseDirectoryPath, filePath) });
			return s_textureAssetTable.get(filePath);
		}
		return Asset::GetOrLoadTexture(filePath);
	}

	bool Asset::UnloadTexture(FilePathView filePath)
	{
		if (s_textureAssetTable.isRegistered(filePath))
		{
			s_textureAssetTable.unregister(filePath);
			return true;
		}
		return false;
	}

	void Asset::UnloadAllTextures()
	{
		s_textureAssetTable.clear();
	}

	const Audio& Asset::GetOrLoadAudio(FilePathView filePath)
	{
		static const Audio EmptyAudio{};

		if (filePath.isEmpty())
		{
			return EmptyAudio;
		}
		if (!s_audioAssetTable.isRegistered(filePath))
		{
			const String combinedPath = FileSystem::PathAppend(s_baseDirectoryPath, filePath);
			if (!FileSystem::IsFile(combinedPath))
			{
				return EmptyAudio;
			}
			s_audioAssetTable.registerAsset(filePath, Audio{ combinedPath });
		}
		return s_audioAssetTable.get(filePath);
	}

	const Audio& Asset::ReloadAudio(FilePathView filePath)
	{
		if (s_audioAssetTable.isRegistered(filePath))
		{
			s_audioAssetTable.unregister(filePath);
			s_audioAssetTable.registerAsset(filePath, Audio{ FileSystem::PathAppend(s_baseDirectoryPath, filePath) });
			return s_audioAssetTable.get(filePath);
		}
		return Asset::GetOrLoadAudio(filePath);
	}

	bool Asset::UnloadAudio(FilePathView filePath)
	{
		if (s_audioAssetTable.isRegistered(filePath))
		{
			s_audioAssetTable.unregister(filePath);
			return true;
		}
		return false;
	}

	void Asset::UnloadAllAudios()
	{
		s_audioAssetTable.clear();
	}
}
