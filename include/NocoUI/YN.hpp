#pragma once
#include <Siv3D.hpp>

namespace noco
{
	using IgnoreIsChangedYN = YesNo<struct IgnoreIsChangedYN_tag>;
	using InteractableYN = YesNo<struct InteractableYN_tag>;
	using ActiveYN = YesNo<struct ActiveYN_tag>;
	using EditorHoveredYN = YesNo<struct EditorHoveredYN_tag>;
	using EditorSelectedYN = YesNo<struct EditorSelectedYN_tag>;
	using RecursiveYN = YesNo<struct RecursiveYN_tag>;
	using IsHitTargetYN = YesNo<struct IsHitTargetYN_tag>;
	using ClippingEnabledYN = YesNo<struct ClippingEnabledYN_tag>;
	using AppliesDisabledStateYN = YesNo<struct AppliesDisabledStateYN_tag>;
	using RefreshesLayoutYN = YesNo<struct RefreshesLayoutYN_tag>;
	using FitsWidthYN = YesNo<struct FitsWidthYN_tag>;
	using FitsHeightYN = YesNo<struct FitsHeightYN_tag>;
	using FoldedYN = YesNo<struct FoldedYN_tag>;
	using ClearsInputYN = YesNo<struct ClearsInputYN_tag>;
	using EnabledWhileTextEditingYN = YesNo<struct EnabledWhileTextEditingYN_tag>;
	using ClearsArrayYN = YesNo<struct ClearsArrayYN_tag>;
	using HitTestEnabledYN = YesNo<struct HitTestEnabledYN_tag>;
	using AltYN = YesNo<struct AltYN_tag>;
	using CtrlYN = YesNo<struct CtrlYN_tag>;
	using ShiftYN = YesNo<struct ShiftYN_tag>;
	using IncludingDisabledYN = YesNo<struct IncludingDisabledYN_tag>;
	using IsScrollingYN = YesNo<struct IsScrollingYN_tag>;
	using RubberBandScrollEnabledYN = YesNo<struct RubberBandScrollEnabledYN_tag>;
	using WithPaddingYN = YesNo<struct WithPaddingYN_tag>;
	using OnlyIfDirtyYN = YesNo<struct OnlyIfDirtyYN_tag>;

	namespace detail
	{
		using WithInstanceIdYN = YesNo<struct WithInstanceIdYN_tag>;
	}
}
