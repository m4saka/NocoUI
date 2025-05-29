#pragma once
#include <Siv3D.hpp>
#include "../YN.hpp"
#include "ComponentBase.hpp"

namespace noco
{
	class KeyInputClearer : public ComponentBase
	{
	public:
		explicit KeyInputClearer()
			: ComponentBase{ {} }
		{
		}

		void updateInput(CanvasUpdateContext*, const std::shared_ptr<Node>&) override
		{
			for (const auto& input : Keyboard::GetAllInputs())
			{
				input.clearInput();
			}
		}
	};
}
