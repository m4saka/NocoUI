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
}
