#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"
#include "../Node.hpp"
#include "../Canvas.hpp"
#include "../YN.hpp"
#include "../detail/TriggerFireDetector.hpp"

namespace noco
{
	class EventTrigger : public SerializableComponentBase
	{
	private:
		PropertyNonInteractive<String> m_tag;
		PropertyNonInteractive<EventTriggerType> m_triggerType;
		PropertyNonInteractive<bool> m_recursive;
		PropertyNonInteractive<double> m_repeatIntervalSec;
		PropertyNonInteractive<double> m_repeatIntervalSecFirst;
		PropertyNonInteractive<double> m_holdDurationSec;

		/* NonSerialized */ detail::TriggerFireDetector m_fireDetector;

	public:
		explicit EventTrigger(StringView tag = U"", EventTriggerType triggerType = EventTriggerType::Click, RecursiveYN recursive = RecursiveYN::No, double repeatIntervalSec = 0.1, double repeatIntervalSecFirst = 0.5, double holdDurationSec = 0.5)
			: SerializableComponentBase{ U"EventTrigger", { &m_tag, &m_triggerType, &m_recursive, &m_repeatIntervalSec, &m_repeatIntervalSecFirst, &m_holdDurationSec } }
			, m_tag{ U"tag", tag }
			, m_triggerType{ U"triggerType", triggerType }
			, m_recursive{ U"recursive", recursive.getBool() }
			, m_repeatIntervalSec{ U"repeatIntervalSec", repeatIntervalSec }
			, m_repeatIntervalSecFirst{ U"repeatIntervalSecFirst", repeatIntervalSecFirst }
			, m_holdDurationSec{ U"holdDurationSec", holdDurationSec }
		{
		}

		void update(const std::shared_ptr<Node>& node) override;

		[[nodiscard]]
		EventTriggerType triggerType() const
		{
			return m_triggerType.value();
		}
	};
}
