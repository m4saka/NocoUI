#pragma once
#include <Siv3D.hpp>

namespace noco
{
	// スクロール可能な方向を示すビットフラグ
	enum class ScrollableAxisFlags : uint8
	{
		None = 0,
		Horizontal = 1 << 0,
		Vertical = 1 << 1,
	};
	
	// スクロール方法を示すビットフラグ
	enum class ScrollMethodFlags : uint8
	{
		None = 0,
		Wheel = 1 << 0,
		Drag = 1 << 1,
	};

	constexpr ScrollableAxisFlags operator|(ScrollableAxisFlags lhs, ScrollableAxisFlags rhs)
	{
		using UnderlyingType = std::underlying_type_t<ScrollableAxisFlags>;
		return static_cast<ScrollableAxisFlags>(static_cast<UnderlyingType>(lhs) | static_cast<UnderlyingType>(rhs));
	}

	constexpr ScrollableAxisFlags operator&(ScrollableAxisFlags lhs, ScrollableAxisFlags rhs)
	{
		using UnderlyingType = std::underlying_type_t<ScrollableAxisFlags>;
		return static_cast<ScrollableAxisFlags>(static_cast<UnderlyingType>(lhs) & static_cast<UnderlyingType>(rhs));
	}

	constexpr ScrollableAxisFlags operator~(ScrollableAxisFlags flags)
	{
		using UnderlyingType = std::underlying_type_t<ScrollableAxisFlags>;
		return static_cast<ScrollableAxisFlags>(~static_cast<UnderlyingType>(flags));
	}

	constexpr ScrollableAxisFlags operator|=(ScrollableAxisFlags& lhs, ScrollableAxisFlags rhs)
	{
		lhs = lhs | rhs;
		return lhs;
	}

	constexpr ScrollableAxisFlags operator&=(ScrollableAxisFlags& lhs, ScrollableAxisFlags rhs)
	{
		lhs = lhs & rhs;
		return lhs;
	}

	[[nodiscard]]
	constexpr bool HasFlag(ScrollableAxisFlags flags, ScrollableAxisFlags flag)
	{
		return (flags & flag) == flag;
	}
	
	constexpr ScrollMethodFlags operator|(ScrollMethodFlags lhs, ScrollMethodFlags rhs)
	{
		using UnderlyingType = std::underlying_type_t<ScrollMethodFlags>;
		return static_cast<ScrollMethodFlags>(static_cast<UnderlyingType>(lhs) | static_cast<UnderlyingType>(rhs));
	}

	constexpr ScrollMethodFlags operator&(ScrollMethodFlags lhs, ScrollMethodFlags rhs)
	{
		using UnderlyingType = std::underlying_type_t<ScrollMethodFlags>;
		return static_cast<ScrollMethodFlags>(static_cast<UnderlyingType>(lhs) & static_cast<UnderlyingType>(rhs));
	}

	constexpr ScrollMethodFlags operator~(ScrollMethodFlags flags)
	{
		using UnderlyingType = std::underlying_type_t<ScrollMethodFlags>;
		return static_cast<ScrollMethodFlags>(~static_cast<UnderlyingType>(flags));
	}

	constexpr ScrollMethodFlags operator|=(ScrollMethodFlags& lhs, ScrollMethodFlags rhs)
	{
		lhs = lhs | rhs;
		return lhs;
	}

	constexpr ScrollMethodFlags operator&=(ScrollMethodFlags& lhs, ScrollMethodFlags rhs)
	{
		lhs = lhs & rhs;
		return lhs;
	}

	[[nodiscard]]
	constexpr bool HasFlag(ScrollMethodFlags flags, ScrollMethodFlags flag)
	{
		return (flags & flag) == flag;
	}
}
