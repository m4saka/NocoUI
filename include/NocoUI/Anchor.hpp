#pragma once
#include <Siv3D.hpp>
#include "Enums.hpp"

namespace noco
{
	namespace Anchor
	{
		constexpr Vec2 TopLeft{ 0.0, 0.0 };
		constexpr Vec2 TopCenter{ 0.5, 0.0 };
		constexpr Vec2 TopRight{ 1.0, 0.0 };
		constexpr Vec2 MiddleLeft{ 0.0, 0.5 };
		constexpr Vec2 MiddleCenter{ 0.5, 0.5 };
		constexpr Vec2 MiddleRight{ 1.0, 0.5 };
		constexpr Vec2 BottomLeft{ 0.0, 1.0 };
		constexpr Vec2 BottomCenter{ 0.5, 1.0 };
		constexpr Vec2 BottomRight{ 1.0, 1.0 };

		[[nodiscard]]
		inline Vec2 FromAlign(HorizontalAlign horizontalAlign, VerticalAlign verticalAlign)
		{
			double x;
			switch (horizontalAlign)
			{
			case HorizontalAlign::Left:
				x = 0.0;
				break;
			case HorizontalAlign::Center:
				x = 0.5;
				break;
			case HorizontalAlign::Right:
				x = 1.0;
				break;
			default:
				throw Error{ U"Anchor::FromAlign: Invalid horizontalAlign" };
			}
			double y = 0.0;
			switch (verticalAlign)
			{
			case VerticalAlign::Top:
				y = 0.0;
				break;
			case VerticalAlign::Middle:
				y = 0.5;
				break;
			case VerticalAlign::Bottom:
				y = 1.0;
				break;
			default:
				throw Error{ U"Anchor::FromAlign: Invalid verticalAlign" };
			}
			return Vec2{ x, y };
		}
	}

	enum class AnchorPreset
	{
		TopLeft,
		TopCenter,
		TopRight,
		MiddleLeft,
		MiddleCenter,
		MiddleRight,
		BottomLeft,
		BottomCenter,
		BottomRight,

		StretchTop,
		StretchMiddle,
		StretchBottom,
		StretchLeft,
		StretchCenter,
		StretchRight,
		StretchFull,

		Custom,
	};

	[[nodiscard]]
	inline constexpr AnchorPreset ToAnchorPreset(const Vec2& anchorMin, const Vec2& anchorMax, const Vec2& pivot)
	{
		if (anchorMin == Anchor::TopLeft && anchorMax == Anchor::TopLeft && pivot == Anchor::TopLeft)
		{
			return AnchorPreset::TopLeft;
		}
		if (anchorMin == Anchor::TopCenter && anchorMax == Anchor::TopCenter && pivot == Anchor::TopCenter)
		{
			return AnchorPreset::TopCenter;
		}
		if (anchorMin == Anchor::TopRight && anchorMax == Anchor::TopRight && pivot == Anchor::TopRight)
		{
			return AnchorPreset::TopRight;
		}
		if (anchorMin == Anchor::MiddleLeft && anchorMax == Anchor::MiddleLeft && pivot == Anchor::MiddleLeft)
		{
			return AnchorPreset::MiddleLeft;
		}
		if (anchorMin == Anchor::MiddleCenter && anchorMax == Anchor::MiddleCenter && pivot == Anchor::MiddleCenter)
		{
			return AnchorPreset::MiddleCenter;
		}
		if (anchorMin == Anchor::MiddleRight && anchorMax == Anchor::MiddleRight && pivot == Anchor::MiddleRight)
		{
			return AnchorPreset::MiddleRight;
		}
		if (anchorMin == Anchor::BottomLeft && anchorMax == Anchor::BottomLeft && pivot == Anchor::BottomLeft)
		{
			return AnchorPreset::BottomLeft;
		}
		if (anchorMin == Anchor::BottomCenter && anchorMax == Anchor::BottomCenter && pivot == Anchor::BottomCenter)
		{
			return AnchorPreset::BottomCenter;
		}
		if (anchorMin == Anchor::BottomRight && anchorMax == Anchor::BottomRight && pivot == Anchor::BottomRight)
		{
			return AnchorPreset::BottomRight;
		}
		if (anchorMin == Anchor::TopLeft && anchorMax == Anchor::TopRight && pivot == Anchor::TopLeft)
		{
			return AnchorPreset::StretchTop;
		}
		if (anchorMin == Anchor::MiddleLeft && anchorMax == Anchor::MiddleRight && pivot == Anchor::MiddleLeft)
		{
			return AnchorPreset::StretchMiddle;
		}
		if (anchorMin == Anchor::BottomLeft && anchorMax == Anchor::BottomRight && pivot == Anchor::BottomLeft)
		{
			return AnchorPreset::StretchBottom;
		}
		if (anchorMin == Anchor::TopLeft && anchorMax == Anchor::BottomLeft && pivot == Anchor::TopLeft)
		{
			return AnchorPreset::StretchLeft;
		}
		if (anchorMin == Anchor::TopCenter && anchorMax == Anchor::BottomCenter && pivot == Anchor::TopCenter)
		{
			return AnchorPreset::StretchCenter;
		}
		if (anchorMin == Anchor::TopRight && anchorMax == Anchor::BottomRight && pivot == Anchor::TopRight)
		{
			return AnchorPreset::StretchRight;
		}
		if (anchorMin == Anchor::TopLeft && anchorMax == Anchor::BottomRight && pivot == Anchor::TopLeft)
		{
			return AnchorPreset::StretchFull;
		}
		return AnchorPreset::Custom;
	}

	[[nodiscard]]
	inline constexpr Optional<std::tuple<Vec2, Vec2, Vec2>> FromAnchorPreset(AnchorPreset preset)
	{
		switch (preset)
		{
		case AnchorPreset::TopLeft:
			return std::tuple{ Anchor::TopLeft, Anchor::TopLeft, Anchor::TopLeft };
		case AnchorPreset::TopCenter:
			return std::tuple{ Anchor::TopCenter, Anchor::TopCenter, Anchor::TopCenter };
		case AnchorPreset::TopRight:
			return std::tuple{ Anchor::TopRight, Anchor::TopRight, Anchor::TopRight };
		case AnchorPreset::MiddleLeft:
			return std::tuple{ Anchor::MiddleLeft, Anchor::MiddleLeft, Anchor::MiddleLeft };
		case AnchorPreset::MiddleCenter:
			return std::tuple{ Anchor::MiddleCenter, Anchor::MiddleCenter, Anchor::MiddleCenter };
		case AnchorPreset::MiddleRight:
			return std::tuple{ Anchor::MiddleRight, Anchor::MiddleRight, Anchor::MiddleRight };
		case AnchorPreset::BottomLeft:
			return std::tuple{ Anchor::BottomLeft, Anchor::BottomLeft, Anchor::BottomLeft };
		case AnchorPreset::BottomCenter:
			return std::tuple{ Anchor::BottomCenter, Anchor::BottomCenter, Anchor::BottomCenter };
		case AnchorPreset::BottomRight:
			return std::tuple{ Anchor::BottomRight, Anchor::BottomRight, Anchor::BottomRight };
		case AnchorPreset::StretchTop:
			return std::tuple{ Anchor::TopLeft, Anchor::TopRight, Anchor::TopLeft };
		case AnchorPreset::StretchMiddle:
			return std::tuple{ Anchor::MiddleLeft, Anchor::MiddleRight, Anchor::MiddleLeft };
		case AnchorPreset::StretchBottom:
			return std::tuple{ Anchor::BottomLeft, Anchor::BottomRight, Anchor::BottomLeft };
		case AnchorPreset::StretchLeft:
			return std::tuple{ Anchor::TopLeft, Anchor::BottomLeft, Anchor::TopLeft };
		case AnchorPreset::StretchCenter:
			return std::tuple{ Anchor::TopCenter, Anchor::BottomCenter, Anchor::TopCenter };
		case AnchorPreset::StretchRight:
			return std::tuple{ Anchor::TopRight, Anchor::BottomRight, Anchor::TopRight };
		case AnchorPreset::StretchFull:
			return std::tuple{ Anchor::TopLeft, Anchor::BottomRight, Anchor::TopLeft };
		case AnchorPreset::Custom:
			break;
		}
		return none;
	}
}
