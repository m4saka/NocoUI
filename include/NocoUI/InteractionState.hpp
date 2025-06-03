#pragma once
#include "YN.hpp"

namespace noco
{
	enum class InteractionState
	{
		Default,
		Hovered,
		Pressed,
		Disabled,
	};

	inline InteractionState ApplyOtherInteractionState(InteractionState state, InteractionState otherState, AppliesDisabledStateYN appliesDisabledState = AppliesDisabledStateYN::Yes)
	{
		if (!appliesDisabledState && otherState == InteractionState::Disabled)
		{
			return state;
		}

		if (state == InteractionState::Disabled)
		{
			return InteractionState::Disabled;
		}
		else if (state == InteractionState::Pressed)
		{
			return InteractionState::Pressed;
		}
		else if (state == InteractionState::Hovered)
		{
			return otherState == InteractionState::Pressed ? InteractionState::Pressed : InteractionState::Hovered;
		}
		else
		{
			return otherState;
		}
	}
}
