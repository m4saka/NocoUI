#pragma once
#include <Siv3D.hpp>

namespace noco
{
	enum class HorizontalAlign : uint8
	{
		Left,
		Center,
		Right,
	};

	enum class VerticalAlign : uint8
	{
		Top,
		Middle,
		Bottom,
	};

	enum class HorizontalOverflow : uint8
	{
		Wrap,
		Overflow,
	};

	enum class VerticalOverflow : uint8
	{
		Clip,
		Overflow,
	};

	enum class FitTarget : uint8
	{
		None,
		WidthOnly,
		HeightOnly,
		Both,
	};

	enum class BlendMode : uint8
	{
		Normal,
		Additive,
		Subtractive,
		Multiply,
	};

	enum class TextureRegionMode : uint8
	{
		Full,
		OffsetSize,
		Grid,
	};

	enum class SpriteGridAnimationType : uint8
	{
		None,
		OneShot,
		Loop,
	};

	enum class SpriteOffsetAnimationType : uint8
	{
		None,
		Scroll,
	};

	enum class SpriteTextureFilter : uint8
	{
		Default,
		Nearest,
		Linear,
		Aniso,
	};

	enum class SpriteTextureAddressMode : uint8
	{
		Default,
		Repeat,
		Mirror,
		Clamp,
		BorderColor,
	};
}
