#pragma once
#include "YN.hpp"

namespace noco
{
	enum class InteractState
	{
		Default,
		Hovered,
		Pressed,
		Disabled,
	};

	inline InteractState ApplyOtherInteractState(InteractState state, InteractState otherState, AppliesDisabledStateYN appliesDisabledState = AppliesDisabledStateYN::Yes)
	{
		if (!appliesDisabledState && otherState == InteractState::Disabled)
		{
			return state;
		}

		if (state == InteractState::Disabled)
		{
			return InteractState::Disabled;
		}
		else if (state == InteractState::Pressed)
		{
			return InteractState::Pressed;
		}
		else if (state == InteractState::Hovered)
		{
			return otherState == InteractState::Pressed ? InteractState::Pressed : InteractState::Hovered;
		}
		else
		{
			return otherState;
		}
	}
}
