#pragma once
#include <NocoUI.hpp>

namespace noco::editor
{
	class InputBlocker : public ComponentBase
	{
	public:
		InputBlocker()
			: ComponentBase{ {} }
		{
		}

		void updateInput(const std::shared_ptr<Node>&) override
		{
			CurrentFrame::BlockInput();
		}
	};
}
