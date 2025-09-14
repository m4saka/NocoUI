#pragma once
#include <Siv3D.hpp>

namespace noco
{
	namespace Asset
	{
		constexpr StringView AssetNamePrefix = U"noco::";

		[[nodiscard]]
		FilePathView GetBaseDirectoryPath();

		void SetBaseDirectoryPath(FilePathView path);

		[[nodiscard]]
		Texture GetOrLoadTexture(FilePathView filePath);

		Texture ReloadTexture(FilePathView filePath);

		bool UnloadTexture(FilePathView filePath);

		void UnloadAllTextures();

		[[nodiscard]]
		Audio GetOrLoadAudio(FilePathView filePath);

		Audio ReloadAudio(FilePathView filePath);

		bool UnloadAudio(FilePathView filePath);

		void UnloadAllAudios();
	}
}
