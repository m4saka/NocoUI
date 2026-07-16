#pragma once
#include <Siv3D.hpp>

namespace noco::editor
{
	/// @brief カスタム定義(コンポーネントスキーマ・フォントアセット・ピクセルシェーダーアセット)の探索と読み込みを行うローダー
	class CustomAssetLoader
	{
	private:
		Array<AssetName> m_registeredFontAssetNames;
		Array<AssetName> m_registeredPixelShaderAssetNames;

		void loadFontAssetsFromDirectory(const FilePath& directory);

		void loadFontAssetFromFile(const FilePath& path);

		void loadPixelShaderAssetsFromDirectory(const FilePath& directory);

		void loadPixelShaderAssetFromFile(const FilePath& path);

	public:
		/// @brief カスタム定義を再読み込み
		/// @remark 前回登録したフォント・シェーダーアセットは解除される
		void reload(const Optional<FilePath>& nocoFilePath);

		/// @brief 適用するカスタム定義ディレクトリの一覧を優先度の高い順に取得
		/// @remark nocoファイルのディレクトリから上位に向かって各.nocouiフォルダ、最後に実行ファイル側のCustomフォルダが入る
		[[nodiscard]]
		static Array<FilePath> ResolveCustomDirectories(const Optional<FilePath>& nocoFilePath);

		/// @brief .nocoui/config.jsonのassetBaseDirectoryPathで指定されたアセットのルートディレクトリを取得
		/// @remark nocoファイルのディレクトリから上位に向かって探索し、最も近いものが採用される。指定がない場合はnoneになる
		[[nodiscard]]
		static Optional<FilePath> ResolveAssetBaseDirectoryPath(const FilePath& nocoFilePath);
	};
}
