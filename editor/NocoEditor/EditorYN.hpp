#pragma once
#include <Siv3D.hpp>

namespace noco::editor
{
	using PreserveScrollYN = YesNo<struct PreserveScrollYN_tag>;
	using HasInteractivePropertyValueYN = YesNo<struct HasInteractivePropertyValueYN_tag>;
	using HasParameterRefYN = YesNo<struct HasParameterRefYN_tag>;
	using IsFoldedYN = YesNo<struct IsFoldedYN_tag>;
	using AppendsMnemonicKeyTextYN = YesNo<struct AppendsMnemonicKeyText_tag>;
	using IsDefaultButtonYN = YesNo<struct IsDefaultButtonYN_tag>;
	using IsCancelButtonYN = YesNo<struct IsCancelButtonYN_tag>;
}
