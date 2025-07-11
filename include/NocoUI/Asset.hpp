﻿#pragma once
#include <Siv3D.hpp>

namespace noco
{
	namespace Asset
	{
		constexpr StringView AssetNamePrefix = U"noco::";

		FilePathView GetBaseDirectoryPath();

		void SetBaseDirectoryPath(FilePathView path);

		Texture GetOrLoadTexture(FilePathView filePath);

		Texture ReloadTexture(FilePathView filePath);

		bool UnloadTexture(FilePathView filePath);

		void UnloadAllTextures();

		Audio GetOrLoadAudio(FilePathView filePath);

		Audio ReloadAudio(FilePathView filePath);

		bool UnloadAudio(FilePathView filePath);

		void UnloadAllAudios();
	}
}
