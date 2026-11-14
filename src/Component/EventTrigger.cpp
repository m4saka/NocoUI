#include "NocoUI/Component/EventTrigger.hpp"

namespace noco
{
	EventTrigger::EventTrigger(StringView tag, EventTriggerType triggerType, RecursiveYN recursive, double repeatIntervalSec, double repeatIntervalSecFirst, double holdDurationSec)
		: SerializableComponentBase{ U"EventTrigger", { &m_tag, &m_triggerType, &m_recursive, &m_repeatIntervalSec, &m_repeatIntervalSecFirst, &m_holdDurationSec } }
		, m_tag{ U"tag", tag }
		, m_triggerType{ U"triggerType", triggerType }
		, m_recursive{ U"recursive", recursive.getBool() }
		, m_repeatIntervalSec{ U"repeatIntervalSec", repeatIntervalSec }
		, m_repeatIntervalSecFirst{ U"repeatIntervalSecFirst", repeatIntervalSecFirst }
		, m_holdDurationSec{ U"holdDurationSec", holdDurationSec }
	{
	}

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
