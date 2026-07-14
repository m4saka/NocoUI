#include "NocoUI/Component/EventTrigger.hpp"

namespace noco
{
	void EventTrigger::update(const std::shared_ptr<Node>& node)
	{
		std::shared_ptr<Canvas> canvas = node->containedCanvas();
		if (!canvas)
		{
			// Canvas配下でない場合は何もしない
			return;
		}

		const auto triggerType = m_triggerType.value();
		if (m_fireDetector.update(node, triggerType, RecursiveYN{ m_recursive.value() }, m_repeatIntervalSec.value(), m_repeatIntervalSecFirst.value(), m_holdDurationSec.value()))
		{
			canvas->fireEvent({ .triggerType = triggerType, .tag = m_tag.value(), .sourceNode = node });
		}
	}
}
