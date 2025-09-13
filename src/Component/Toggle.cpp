#include "NocoUI/Component/Toggle.hpp"
#include "NocoUI/Node.hpp"

namespace noco
{
	void Toggle::update(const std::shared_ptr<Node>& node)
	{
		if (node->isClicked())
		{
			m_value.setValue(!m_value.value());
		}

		if (m_value.value())
		{
			node->styleStateProperty().setCurrentFrameOverride(U"on");
		}
		else
		{
			node->styleStateProperty().setCurrentFrameOverride(U"off");
		}
	}
}