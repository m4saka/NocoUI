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
	}
}
