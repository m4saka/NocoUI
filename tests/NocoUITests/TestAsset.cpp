#include <catch2/catch.hpp>
#include <Siv3D.hpp>
#include <NocoUI.hpp>

// ========================================
// Assetのテスト
// ========================================

TEST_CASE("Asset GetOrLoadDownscaledTexture", "[Asset]")
{
	const FilePath squarePath = FileSystem::PathAppend(FileSystem::TemporaryDirectoryPath(), U"noco_test_downscale_square.png");
	const FilePath widePath = FileSystem::PathAppend(FileSystem::TemporaryDirectoryPath(), U"noco_test_downscale_wide.png");
	REQUIRE(Image{ 64, 64, Palette::White }.save(squarePath));
	REQUIRE(Image{ 64, 32, Palette::White }.save(widePath));
	noco::Asset::UnloadAllTextures();

	SECTION("Downscale to more than sqrt(2) times and at most 2 times the draw size")
	{
		// 64pxの画像を16pxで描画する場合は32pxに縮小
		const Texture& texture = noco::Asset::GetOrLoadDownscaledTexture(squarePath, SizeF{ 16, 16 }, noco::PreserveAspectYN::No);
		REQUIRE(&texture == &noco::detail::DownscaledTextureAssetTable().get(squarePath).textures.at(2));
		REQUIRE_FALSE(noco::detail::TextureAssetTable().isRegistered(squarePath));
	}

	SECTION("No downscale when the image is at most 2 times the draw size")
	{
		const Texture& texture = noco::Asset::GetOrLoadDownscaledTexture(squarePath, SizeF{ 32, 32 }, noco::PreserveAspectYN::No);
		REQUIRE(&texture == &noco::detail::TextureAssetTable().get(squarePath));
		REQUIRE(noco::detail::DownscaledTextureAssetTable().get(squarePath).textures.empty());
	}

	SECTION("preserveAspect uses the larger ratio")
	{
		// 64x32の画像を16x16で描画する場合、アスペクト比を保持すると横の比(4倍)に合わせて縮小
		const Texture& preserved = noco::Asset::GetOrLoadDownscaledTexture(widePath, SizeF{ 16, 16 }, noco::PreserveAspectYN::Yes);
		REQUIRE(&preserved == &noco::detail::DownscaledTextureAssetTable().get(widePath).textures.at(2));

		// 保持しない場合は縦の比(2倍)に合わせるため縮小しない
		const Texture& stretched = noco::Asset::GetOrLoadDownscaledTexture(widePath, SizeF{ 16, 16 }, noco::PreserveAspectYN::No);
		REQUIRE(&stretched == &noco::detail::TextureAssetTable().get(widePath));
	}

	noco::Asset::UnloadAllTextures();
	FileSystem::Remove(squarePath);
	FileSystem::Remove(widePath);
}
