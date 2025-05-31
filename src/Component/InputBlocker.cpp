#include "NocoUI/Component/InputBlocker.hpp"
#include "NocoUI/Canvas.hpp"

namespace noco
{
	void InputBlocker::updateInput(const std::shared_ptr<Node>&)
	{
		CurrentFrame::BlockInput();
	}
}
