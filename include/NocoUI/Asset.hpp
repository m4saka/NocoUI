#pragma once
#include <Siv3D.hpp>
#include "YN.hpp"

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

			const T* find(FilePathView filePath) const
			{
				const auto it = m_table.find(filePath);
				return it != m_table.end() ? it->second.get() : nullptr;
			}

			T* find(FilePathView filePath)
			{
				const auto it = m_table.find(filePath);
				return it != m_table.end() ? it->second.get() : nullptr;
			}

			T& registerAsset(FilePathView filePath, const T& asset)
			{
				return *m_table.emplace(filePath, std::make_shared<T>(asset)).first->second;
			}

			T& registerAsset(FilePathView filePath, T&& asset)
			{
				return *m_table.emplace(filePath, std::make_shared<T>(std::move(asset))).first->second;
			}

			void unregister(FilePathView filePath)
			{
				m_table.erase(filePath);
			}

			const T& get(FilePathView filePath) const
			{
				return *m_table.at(filePath);
			}

			T& get(FilePathView filePath)
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

		// ミップマップありで読み込んだテクスチャは、同一パスでもUnmipped版とは別のテクスチャなので専用テーブルで管理する
		inline AssetTable<Texture>& MipmappedTextureAssetTable()
		{
			static AssetTable<Texture> table;
			return table;
		}

		// 描画サイズに合わせて縮小したテクスチャを、元画像のパス毎にまとめたもの
		struct DownscaledTextureSet
		{
			// 元画像のサイズ(ヘッダから取得できなかった場合はnone)
			Optional<Size> imageSize;

			// 縦横を1/√2に縮小した回数毎のテクスチャ
			HashTable<int32, Texture> textures;
		};

		// 縮小したテクスチャは、同一パスでも元のテクスチャとは別のテクスチャなので専用テーブルで管理する
		inline AssetTable<DownscaledTextureSet>& DownscaledTextureAssetTable()
		{
			static AssetTable<DownscaledTextureSet> table;
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

		/// @brief 別スレッドでのテクスチャ読み込みを進める(完了したものをキャッシュに登録し、待機中のものを開始する)
		/// @note noco::Initで登録されるアドオンから毎フレーム呼ばれる
		void UpdateAsyncTextureLoads();

		/// @brief 別スレッドでのテクスチャ読み込みが待機中または実行中か
		[[nodiscard]]
		bool HasAsyncTextureLoads();

		/// @brief 別スレッドでのテクスチャ読み込みを終了する(待機中のものは破棄し、実行中のものは完了を待つ)
		void ShutdownAsyncTextureLoads();
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
		/// @param mipmapEnabled ミップマップを生成するか(同一パスでもUnmipped版とMipped版は別々にキャッシュされる)
		/// @return テクスチャ
		[[nodiscard]]
		const Texture& GetOrLoadTexture(FilePathView filePath, MipmapEnabledYN mipmapEnabled);

		/// @brief テクスチャを事前に読み込む
		/// @param filePath テクスチャファイルのパス
		/// @param mipmapEnabled ミップマップを生成するか
		void PrewarmTexture(FilePathView filePath, MipmapEnabledYN mipmapEnabled);

		/// @brief テクスチャを別スレッドで事前に読み込む
		/// @param filePath テクスチャファイルのパス
		/// @param mipmapEnabled ミップマップを生成するか
		/// @note 読み込みが完了するまで、GetOrLoadTextureは空のテクスチャを返す
		/// @note SetTextureLoaderで読み込み処理を差し替えている場合は、別スレッドを使わずに読み込む
		void PrewarmTextureAsync(FilePathView filePath, MipmapEnabledYN mipmapEnabled);

		/// @brief 画像ファイルの縦横のピクセル数を取得
		/// @param filePath 画像ファイルのパス
		/// @return 縦横のピクセル数(ファイルが存在しない場合や取得できない場合はnone)
		[[nodiscard]]
		Optional<Size> GetImageSize(FilePathView filePath);

		/// @brief 描画サイズに合わせて縮小したテクスチャを取得(未読み込みの場合はロードする)
		/// @param filePath テクスチャファイルのパス
		/// @param drawSize 画面上の描画サイズ
		/// @param preserveAspect アスペクト比を保持するか
		/// @return テクスチャ(縮小が不要な場合は元のテクスチャ)
		/// @note SetTextureLoaderで読み込み処理を差し替えている場合は縮小しない
		[[nodiscard]]
		const Texture& GetOrLoadDownscaledTexture(FilePathView filePath, const SizeF& drawSize, PreserveAspectYN preserveAspect);

		/// @brief 描画サイズに合わせて縮小したテクスチャを事前に読み込む
		/// @param filePath テクスチャファイルのパス
		/// @param drawSize 画面上の描画サイズ
		/// @param preserveAspect アスペクト比を保持するか
		void PrewarmDownscaledTexture(FilePathView filePath, const SizeF& drawSize, PreserveAspectYN preserveAspect);

		/// @brief 描画サイズに合わせて縮小したテクスチャを別スレッドで事前に読み込む
		/// @param filePath テクスチャファイルのパス
		/// @param drawSize 画面上の描画サイズ
		/// @param preserveAspect アスペクト比を保持するか
		/// @note 読み込みが完了するまで、GetOrLoadDownscaledTextureは空のテクスチャを返す
		/// @note SetTextureLoaderで読み込み処理を差し替えている場合は、別スレッドを使わずに読み込む
		void PrewarmDownscaledTextureAsync(FilePathView filePath, const SizeF& drawSize, PreserveAspectYN preserveAspect);

		/// @brief 待機中の別スレッドでのテクスチャ読み込みを取り消す(実行中のものは継続する)
		/// @note 取り消した読み込みの空のテクスチャはキャッシュから削除される
		void CancelPendingAsyncTextureLoads();

		/// @brief テクスチャの読み込み処理を差し替える(nullptrで既定の読み込みに戻す)
		void SetTextureLoader(std::function<Texture(FilePathView, MipmapEnabledYN)> loader);

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
			detail::TextureAssetTable().eraseIf(predicate);
			detail::MipmappedTextureAssetTable().eraseIf(predicate);
			detail::DownscaledTextureAssetTable().eraseIf(predicate);
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
