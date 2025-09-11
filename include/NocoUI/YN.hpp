#pragma once
#include <Siv3D.hpp>

namespace noco
{
	using IgnoreIsChangedYN = YesNo<struct IgnoreIsChangedYN_tag>;
	using InteractableYN = YesNo<struct InteractableYN_tag>;
	using ActiveYN = YesNo<struct ActiveYN_tag>;
	using RecursiveYN = YesNo<struct RecursiveYN_tag>;
	using IsHitTargetYN = YesNo<struct IsHitTargetYN_tag>;
	using ClippingEnabledYN = YesNo<struct ClippingEnabledYN_tag>;
	using ApplyDisabledStateYN = YesNo<struct ApplyDisabledStateYN_tag>;
	using FoldedYN = YesNo<struct FoldedYN_tag>;
	using ClearInputYN = YesNo<struct ClearInputYN_tag>;
	using EnabledWhileTextEditingYN = YesNo<struct EnabledWhileTextEditingYN_tag>;
	using ClearArrayYN = YesNo<struct ClearArrayYN_tag>;
	using HitTestEnabledYN = YesNo<struct HitTestEnabledYN_tag>;
	using AltYN = YesNo<struct AltYN_tag>;
	using CtrlYN = YesNo<struct CtrlYN_tag>;
	using ShiftYN = YesNo<struct ShiftYN_tag>;
	using IncludingDisabledYN = YesNo<struct IncludingDisabledYN_tag>;
	using IsScrollingYN = YesNo<struct IsScrollingYN_tag>;
	using RubberBandScrollEnabledYN = YesNo<struct RubberBandScrollEnabledYN_tag>;
	using WithPaddingYN = YesNo<struct WithPaddingYN_tag>;
	using OnlyIfDirtyYN = YesNo<struct OnlyIfDirtyYN_tag>;
	using SkipSmoothingYN = YesNo<struct SkipSmoothingYN_tag>;

	namespace detail
	{
		using WithInstanceIdYN = YesNo<struct WithInstanceIdYN_tag>;
		using UsePrevSiblingZOrderYN = YesNo<struct UsePrevSiblingZOrderYN_tag>;
	}
}
