#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"
#include "../Node.hpp"
#include "../Canvas.hpp"
#include "../YN.hpp"

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

		// Hoveredのみ初期値はfalseとする
		// (初回update時に既にホバーしている場合もイベントを発火させたいため。ただし、他triggerTypeからの変更タイミングで発火させてはいけないため、HoveredもOptionalを利用する必要がある)
		/* NonSerialized */ Optional<bool> m_prevHovered = false;
		/* NonSerialized */ Optional<bool> m_prevPressed = none;
		/* NonSerialized */ Optional<bool> m_prevRightPressed = none;
		/* NonSerialized */ Optional<bool> m_prevHoveredRecursive = false;
		/* NonSerialized */ Optional<bool> m_prevPressedRecursive = none;
		/* NonSerialized */ Optional<bool> m_prevRightPressedRecursive = none;
		/* NonSerialized */ Stopwatch m_pressRepeatStopwatch;
		/* NonSerialized */ double m_prevPressRepeatTimeSec = 0.0;
		/* NonSerialized */ Stopwatch m_rightPressRepeatStopwatch;
		/* NonSerialized */ double m_prevRightPressRepeatTimeSec = 0.0;

	public:
		explicit EventTrigger(StringView tag = U"", EventTriggerType triggerType = EventTriggerType::Click, RecursiveYN recursive = RecursiveYN::No, double repeatIntervalSec = 0.1, double repeatIntervalSecFirst = 0.5)
			: SerializableComponentBase{ U"EventTrigger", { &m_tag, &m_triggerType, &m_recursive, &m_repeatIntervalSec, &m_repeatIntervalSecFirst } }
			, m_tag{ U"tag", tag }
			, m_triggerType{ U"triggerType", triggerType }
			, m_recursive{ U"recursive", recursive.getBool() }
			, m_repeatIntervalSec{ U"repeatIntervalSec", repeatIntervalSec }
			, m_repeatIntervalSecFirst{ U"repeatIntervalSecFirst", repeatIntervalSecFirst }
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
