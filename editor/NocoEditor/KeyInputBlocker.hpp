#pragma once
#include <NocoUI.hpp>

namespace noco::editor
{
	class KeyInputBlocker : public ComponentBase
	{
	public:
		KeyInputBlocker()
			: ComponentBase{ {} }
		{
		}

		void updateKeyInput(const std::shared_ptr<Node>&) override
		{
			CurrentFrame::BlockKeyInput();
		}
	};
}
