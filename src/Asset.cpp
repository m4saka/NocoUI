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

			void registerAsset(FilePathView filePath, T&& asset)
			{
				m_table.emplace(filePath, std::move(asset));
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

		if (!s_textureAssetTable.isRegistered(filePath))
		{
			const FilePath fullPath = GetFullPath(filePath);
			if (!FileSystem::IsFile(fullPath))
			{
				return EmptyTexture;
			}
			s_textureAssetTable.registerAsset(filePath, Texture{ fullPath });
		}

		return s_textureAssetTable.get(filePath);
	}

	const Texture& Asset::ReloadTexture(FilePathView filePath)
	{
		if (s_textureAssetTable.isRegistered(filePath))
		{
			s_textureAssetTable.unregister(filePath);
			const FilePath fullPath = GetFullPath(filePath);
			s_textureAssetTable.registerAsset(filePath, Texture{ fullPath });
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
			const FilePath fullPath = GetFullPath(filePath);
			if (!FileSystem::IsFile(fullPath))
			{
				return EmptyAudio;
			}
			s_audioAssetTable.registerAsset(filePath, Audio{ fullPath });
		}
		return s_audioAssetTable.get(filePath);
	}

	const Audio& Asset::ReloadAudio(FilePathView filePath)
	{
		if (s_audioAssetTable.isRegistered(filePath))
		{
			s_audioAssetTable.unregister(filePath);
			const FilePath fullPath = GetFullPath(filePath);
			s_audioAssetTable.registerAsset(filePath, Audio{ fullPath });
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
