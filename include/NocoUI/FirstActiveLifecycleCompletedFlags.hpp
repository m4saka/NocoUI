#pragma once
#include <Siv3D.hpp>

namespace noco
{
	// activeInHierarchy=Yesで各ライフサイクルが初回呼び出し済みかどうかのビットフラグ
	enum class FirstActiveLifecycleCompletedFlags : uint8
	{
		None = 0,
		UpdateKeyInput = 1 << 0,
		Update = 1 << 1,
		LateUpdate = 1 << 2,
		Draw = 1 << 3,
	};

	constexpr FirstActiveLifecycleCompletedFlags operator|(FirstActiveLifecycleCompletedFlags lhs, FirstActiveLifecycleCompletedFlags rhs)
	{
		using UnderlyingType = std::underlying_type_t<FirstActiveLifecycleCompletedFlags>;
		return static_cast<FirstActiveLifecycleCompletedFlags>(static_cast<UnderlyingType>(lhs) | static_cast<UnderlyingType>(rhs));
	}

	constexpr FirstActiveLifecycleCompletedFlags operator&(FirstActiveLifecycleCompletedFlags lhs, FirstActiveLifecycleCompletedFlags rhs)
	{
		using UnderlyingType = std::underlying_type_t<FirstActiveLifecycleCompletedFlags>;
		return static_cast<FirstActiveLifecycleCompletedFlags>(static_cast<UnderlyingType>(lhs) & static_cast<UnderlyingType>(rhs));
	}

	constexpr FirstActiveLifecycleCompletedFlags operator~(FirstActiveLifecycleCompletedFlags flags)
	{
		using UnderlyingType = std::underlying_type_t<FirstActiveLifecycleCompletedFlags>;
		return static_cast<FirstActiveLifecycleCompletedFlags>(~static_cast<UnderlyingType>(flags));
	}

	constexpr FirstActiveLifecycleCompletedFlags operator|=(FirstActiveLifecycleCompletedFlags& lhs, FirstActiveLifecycleCompletedFlags rhs)
	{
		lhs = lhs | rhs;
		return lhs;
	}

	constexpr FirstActiveLifecycleCompletedFlags operator&=(FirstActiveLifecycleCompletedFlags& lhs, FirstActiveLifecycleCompletedFlags rhs)
	{
		lhs = lhs & rhs;
		return lhs;
	}

	[[nodiscard]]
	constexpr bool HasFlag(FirstActiveLifecycleCompletedFlags flags, FirstActiveLifecycleCompletedFlags flag)
	{
		return (flags & flag) == flag;
	}
}
