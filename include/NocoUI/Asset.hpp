#pragma once
// #include <Siv3D.hpp> // pch.hppに移動

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
	}
}
