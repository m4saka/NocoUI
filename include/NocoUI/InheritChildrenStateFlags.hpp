#pragma once
#include <Siv3D.hpp>

namespace noco
{
	// 子ノードのInteractionStateを継承するかどうかのビットフラグ
	enum class InheritChildrenStateFlags : uint8
	{
		None = 0,
		Hovered = 1 << 0,
		Pressed = 1 << 1,
	};

	constexpr InheritChildrenStateFlags operator|(InheritChildrenStateFlags lhs, InheritChildrenStateFlags rhs)
	{
		using UnderlyingType = std::underlying_type_t<InheritChildrenStateFlags>;
		return static_cast<InheritChildrenStateFlags>(static_cast<UnderlyingType>(lhs) | static_cast<UnderlyingType>(rhs));
	}

	constexpr InheritChildrenStateFlags operator&(InheritChildrenStateFlags lhs, InheritChildrenStateFlags rhs)
	{
		using UnderlyingType = std::underlying_type_t<InheritChildrenStateFlags>;
		return static_cast<InheritChildrenStateFlags>(static_cast<UnderlyingType>(lhs) & static_cast<UnderlyingType>(rhs));
	}

	constexpr InheritChildrenStateFlags operator~(InheritChildrenStateFlags flags)
	{
		using UnderlyingType = std::underlying_type_t<InheritChildrenStateFlags>;
		return static_cast<InheritChildrenStateFlags>(~static_cast<UnderlyingType>(flags));
	}

	constexpr InheritChildrenStateFlags operator|=(InheritChildrenStateFlags& lhs, InheritChildrenStateFlags rhs)
	{
		lhs = lhs | rhs;
		return lhs;
	}

	constexpr InheritChildrenStateFlags operator&=(InheritChildrenStateFlags& lhs, InheritChildrenStateFlags rhs)
	{
		lhs = lhs & rhs;
		return lhs;
	}

	[[nodiscard]]
	constexpr bool HasFlag(InheritChildrenStateFlags flags, InheritChildrenStateFlags flag)
	{
		return (flags & flag) == flag;
	}
}
