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

		// Hoveredのみ初期値はfalseとする
		// (初回update時に既にホバーしている場合もイベントを発火させたいため。ただし、他triggerTypeからの変更タイミングで発火させてはいけないため、HoveredもOptionalを利用する必要がある)
		/* NonSerialized */ Optional<bool> m_prevHovered = false;
		/* NonSerialized */ Optional<bool> m_prevPressed = none;
		/* NonSerialized */ Optional<bool> m_prevRightPressed = none;
		/* NonSerialized */ Optional<bool> m_prevHoveredRecursive = false;
		/* NonSerialized */ Optional<bool> m_prevPressedRecursive = none;
		/* NonSerialized */ Optional<bool> m_prevRightPressedRecursive = none;

	public:
		explicit EventTrigger(StringView tag = U"", EventTriggerType triggerType = EventTriggerType::Click, RecursiveYN recursive = RecursiveYN::No)
			: SerializableComponentBase{ U"EventTrigger", { &m_tag, &m_triggerType, &m_recursive } }
			, m_tag{ U"tag", tag }
			, m_triggerType{ U"triggerType", triggerType }
			, m_recursive{ U"recursive", recursive.getBool() }
		{
		}

		void update(const std::shared_ptr<Node>& node) override;
	};
}
