#pragma once
#include <Siv3D.hpp>

namespace noco
{
	namespace detail
	{
		// Siv3DのAssetシステムを直接使うとユーザー側のアセットとの衝突回避用にPrefixを付与する必要が生じてコストがかかるため、別途管理する
		template <class T>
		class AssetTable
		{
		private:
			// get関数が返すTの参照がregisterAsset関数の呼び出しによるHashTableのメモリ再配置で無効にならないよう、Tは直接持たずshared_ptr<T>で持つ
			HashTable<AssetName, std::shared_ptr<T>> m_table;

		public:
			bool isRegistered(FilePathView filePath) const
			{
				return m_table.contains(filePath);
			}

			void registerAsset(FilePathView filePath, const T& asset)
			{
				m_table.emplace(filePath, std::make_shared<T>(asset));
			}

			void registerAsset(FilePathView filePath, T&& asset)
			{
				m_table.emplace(filePath, std::make_shared<T>(std::move(asset)));
			}

			void unregister(FilePathView filePath)
			{
				m_table.erase(filePath);
			}

			const T& get(FilePathView filePath) const
			{
				return *m_table.at(filePath);
			}

			void clear()
			{
				m_table.clear();
			}

			template <class Pred>
			void eraseIf(Pred&& predicate)
			{
				for (auto it = m_table.begin(); it != m_table.end();)
				{
					if (predicate(*it))
					{
						it = m_table.erase(it);
					}
					else
					{
						++it;
					}
				}
			}
		};

		inline AssetTable<Texture>& TextureAssetTable()
		{
			static AssetTable<Texture> table;
			return table;
		}

		inline AssetTable<Audio>& AudioAssetTable()
		{
			static AssetTable<Audio> table;
			return table;
		}

		inline AssetTable<JSON>& JSONAssetTable()
		{
			static AssetTable<JSON> table;
			return table;
		}
	}

	namespace Asset
	{
		/// @brief アセットのベースディレクトリパスを取得
		/// @return ベースディレクトリパス
		[[nodiscard]]
		const FilePath& GetBaseDirectoryPath();

		/// @brief アセットのベースディレクトリパスを設定
		/// @param path ベースディレクトリパス
		void SetBaseDirectoryPath(FilePathView path);

		/// @brief 絶対パス/相対パスをアセットのフルパスに変換
		/// @param filePath 入力パス(絶対または相対)
		/// @return アセットのフルパス(絶対パスならそのまま、相対パスならベースディレクトリと結合)
		[[nodiscard]]
		FilePath GetFullPath(FilePathView filePath);

		/// @brief テクスチャを取得(未読み込みの場合はロードする)
		/// @param filePath テクスチャファイルのパス
		/// @return テクスチャ
		[[nodiscard]]
		const Texture& GetOrLoadTexture(FilePathView filePath);

		/// @brief テクスチャを再読み込み
		/// @param filePath テクスチャファイルのパス
		/// @return 再読み込みしたテクスチャ
		const Texture& ReloadTexture(FilePathView filePath);

		/// @brief テクスチャをアンロード
		/// @param filePath テクスチャファイルのパス
		/// @return アンロードに成功した場合はtrue、該当するテクスチャが存在しなかった場合はfalseを返す
		bool UnloadTexture(FilePathView filePath);

		/// @brief すべてのテクスチャをアンロード
		void UnloadAllTextures();

		/// @brief 条件を満たすテクスチャをアンロード
		/// @param predicate ファイルパスとアセットのpairを受け取り、アンロードする場合にtrueを返す関数
		template <class Pred>
		void UnloadTexturesIf(Pred&& predicate)
		{
			detail::TextureAssetTable().eraseIf(std::forward<Pred>(predicate));
		}

		/// @brief オーディオを取得(未読み込みの場合はロードする)
		/// @param filePath オーディオファイルのパス
		/// @return オーディオ
		[[nodiscard]]
		const Audio& GetOrLoadAudio(FilePathView filePath);

		/// @brief オーディオを再読み込み
		/// @param filePath オーディオファイルのパス
		/// @return 再読み込みしたオーディオ
		const Audio& ReloadAudio(FilePathView filePath);

		/// @brief オーディオをアンロード
		/// @param filePath オーディオファイルのパス
		/// @return アンロードに成功した場合はtrue、該当するオーディオが存在しなかった場合はfalseを返す
		bool UnloadAudio(FilePathView filePath);

		/// @brief すべてのオーディオをアンロード
		void UnloadAllAudios();

		/// @brief 条件を満たすオーディオをアンロード
		/// @param predicate ファイルパスとアセットのpairを受け取り、アンロードする場合にtrueを返す関数
		template <class Pred>
		void UnloadAudiosIf(Pred&& predicate)
		{
			detail::AudioAssetTable().eraseIf(std::forward<Pred>(predicate));
		}

		/// @brief JSONを取得(未読み込みの場合はロードする)
		/// @param filePath JSONファイルのパス
		/// @return JSON
		[[nodiscard]]
		const JSON& GetOrLoadJSON(FilePathView filePath);

		/// @brief JSONを再読み込み
		/// @param filePath JSONファイルのパス
		/// @return 再読み込みしたJSON
		const JSON& ReloadJSON(FilePathView filePath);

		/// @brief JSONをアンロード
		/// @param filePath JSONファイルのパス
		/// @return アンロードに成功した場合はtrue、該当するJSONが存在しなかった場合はfalseを返す
		bool UnloadJSON(FilePathView filePath);

		/// @brief すべてのJSONをアンロード
		void UnloadAllJSONs();

		/// @brief 条件を満たすJSONをアンロード
		/// @param predicate ファイルパスとアセットのpairを受け取り、アンロードする場合にtrueを返す関数
		template <class Pred>
		void UnloadJSONsIf(Pred&& predicate)
		{
			detail::JSONAssetTable().eraseIf(std::forward<Pred>(predicate));
		}
	}
}
