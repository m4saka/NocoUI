#pragma once
#include <Siv3D.hpp>

namespace noco
{
	namespace Asset
	{
		constexpr StringView AssetNamePrefix = U"noco::";

		[[nodiscard]]
		const FilePath& GetBaseDirectoryPath();

		void SetBaseDirectoryPath(FilePathView path);

		/// @brief 絶対パス/相対パスをアセットのフルパスに変換
		/// @param filePath 入力パス(絶対または相対)
		/// @return アセットのフルパス(絶対パスならそのまま、相対パスならベースディレクトリと結合)
		[[nodiscard]]
		FilePath GetFullPath(FilePathView filePath);

		[[nodiscard]]
		const Texture& GetOrLoadTexture(FilePathView filePath);

		const Texture& ReloadTexture(FilePathView filePath);

		bool UnloadTexture(FilePathView filePath);

		void UnloadAllTextures();

		[[nodiscard]]
		const Audio& GetOrLoadAudio(FilePathView filePath);

		const Audio& ReloadAudio(FilePathView filePath);

		bool UnloadAudio(FilePathView filePath);

		void UnloadAllAudios();

		[[nodiscard]]
		const JSON& GetOrLoadJSON(FilePathView filePath);

		const JSON& ReloadJSON(FilePathView filePath);

		bool UnloadJSON(FilePathView filePath);

		void UnloadAllJSONs();
	}
}
