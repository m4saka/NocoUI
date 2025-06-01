#pragma once
#include <Siv3D.hpp>

// YesNo type aliases for editor-specific functionality
// Note: These require NocoUI.hpp to be included before this file
using CheckedYN = YesNo<struct CheckedYN_tag>;
using ScreenMaskEnabledYN = YesNo<struct ScreenMaskEnabledYN_tag>;
using PreserveScrollYN = YesNo<struct PreserveScrollYN_tag>;
using HasInteractivePropertyValueYN = YesNo<struct HasInteractivePropertyValueYN_tag>;
using IsFoldedYN = YesNo<struct IsFoldedYN_tag>;
using AppendsMnemonicKeyTextYN = YesNo<struct AppendsMnemonicKeyText_tag>;
using IsDefaultButtonYN = YesNo<struct IsDefaultButtonYN_tag>;
using IsCancelButtonYN = YesNo<struct IsCancelButtonYN_tag>;
using RecursionYN = YesNo<struct RecursionYN_tag>;

// Constants
constexpr int32 MenuBarHeight = 26;

// Constraint type enum (used in Inspector)
enum class ConstraintType : uint8
{
	AnchorConstraint,
	BoxConstraint,
};