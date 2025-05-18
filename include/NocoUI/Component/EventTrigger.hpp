#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"
#include "../Node.hpp"
#include "../Canvas.hpp"

namespace noco
{
	class EventTrigger : public SerializableComponentBase
	{
	private:
		PropertyNonInteractive<String> m_tag;
		PropertyNonInteractive<EventType> m_eventType;
		PropertyNonInteractive<bool> m_isRecursive;

		// Hoveredのみ初期値はfalseとする
		// (初回update時に既にホバーしている場合もイベントを発火させたいため。ただし、他typeからの変更タイミングで発火させてはいけないため、HoveredもOptionalを利用する必要がある)
		/* NonSerialized */ Optional<bool> m_prevHovered = false;
		/* NonSerialized */ Optional<bool> m_prevPressed = none;
		/* NonSerialized */ Optional<bool> m_prevHoveredRecursive = false;
		/* NonSerialized */ Optional<bool> m_prevPressedRecursive = none;

	public:
		explicit EventTrigger(StringView tag = U"")
			: SerializableComponentBase{ U"EventTrigger", { &m_tag, &m_eventType, &m_isRecursive } }
			, m_tag{ U"tag", tag }
			, m_eventType{ U"eventType", EventType::Click }
			, m_isRecursive{ U"isRecursive", false }
		{
		}

		void update(CanvasUpdateContext*, const std::shared_ptr<Node>& node) override
		{
			std::shared_ptr<Canvas> canvas = node->containedCanvas();
			if (!canvas)
			{
				// Canvas配下でない場合は何もしない
				return;
			}

			const auto eventType = m_eventType.value();
			const bool isRecursive = m_isRecursive.value();
			switch (eventType)
			{
			case EventType::Click:
				if (isRecursive ? node->isClickedRecursive() : node->isClicked())
				{
					canvas->fireEvent(
						Event
						{
							.type = EventType::Click,
							.tag = m_tag.value(),
							.sourceNode = node,
						});
				}
				m_prevHovered = none;
				m_prevPressed = none;
				m_prevHoveredRecursive = none;
				m_prevPressedRecursive = none;
				break;

			case EventType::HoverStart:
				if (isRecursive)
				{
					if (node->isHoveredRecursive())
					{
						if (m_prevHoveredRecursive.has_value() && !m_prevHoveredRecursive.value())
						{
							canvas->fireEvent(
								Event
								{
									.type = EventType::HoverStart,
									.tag = m_tag.value(),
									.sourceNode = node,
								});
						}
						m_prevHoveredRecursive = true;
					}
					else
					{
						m_prevHoveredRecursive = false;
					}
					m_prevHovered = none;
					m_prevPressed = none;
					m_prevPressedRecursive = none;
				}
				else
				{
					if (node->isHovered())
					{
						if (m_prevHovered.has_value() && !m_prevHovered.value())
						{
							canvas->fireEvent(
								Event
								{
									.type = EventType::HoverStart,
									.tag = m_tag.value(),
									.sourceNode = node,
								});
						}
						m_prevHovered = true;
					}
					else
					{
						m_prevHovered = false;
					}
					m_prevHoveredRecursive = none;
					m_prevPressed = none;
					m_prevPressedRecursive = none;
				}
				break;

			case EventType::HoverEnd:
				if (isRecursive)
				{
					if (!node->isHoveredRecursive())
					{
						if (m_prevHoveredRecursive.has_value() && m_prevHoveredRecursive.value())
						{
							canvas->fireEvent(
								Event
								{
									.type = EventType::HoverEnd,
									.tag = m_tag.value(),
									.sourceNode = node,
								});
						}
						m_prevHoveredRecursive = false;
					}
					else
					{
						m_prevHoveredRecursive = true;
					}
					m_prevHovered = none;
					m_prevPressed = none;
					m_prevPressedRecursive = none;
				}
				else
				{
					if (!node->isHovered())
					{
						if (m_prevHovered.has_value() && m_prevHovered.value())
						{
							canvas->fireEvent(
								Event
								{
									.type = EventType::HoverEnd,
									.tag = m_tag.value(),
									.sourceNode = node,
								});
						}
						m_prevHovered = false;
					}
					else
					{
						m_prevHovered = true;
					}
					m_prevHoveredRecursive = none;
					m_prevPressed = none;
					m_prevPressedRecursive = none;
				}
				break;

			case EventType::PressStart:
				if (isRecursive)
				{
					if (node->isPressedRecursive())
					{
						if (m_prevPressedRecursive.has_value() && !m_prevPressedRecursive.value())
						{
							canvas->fireEvent(
								Event
								{
									.type = EventType::PressStart,
									.tag = m_tag.value(),
									.sourceNode = node,
								});
						}
						m_prevPressedRecursive = true;
					}
					else
					{
						m_prevPressedRecursive = false;
					}
					m_prevHovered = none;
					m_prevHoveredRecursive = none;
					m_prevPressed = none;
				}
				else
				{
					if (node->isPressed())
					{
						if (m_prevPressed.has_value() && !m_prevPressed.value())
						{
							canvas->fireEvent(
								Event
								{
									.type = EventType::PressStart,
									.tag = m_tag.value(),
									.sourceNode = node,
								});
						}
						m_prevPressed = true;
					}
					else
					{
						m_prevPressed = false;
					}
					m_prevHovered = none;
					m_prevHoveredRecursive = none;
					m_prevPressedRecursive = none;
				}
				break;

			case EventType::PressEnd:
				if (isRecursive)
				{
					if (!node->isPressedRecursive())
					{
						if (m_prevPressedRecursive.has_value() && m_prevPressedRecursive.value())
						{
							canvas->fireEvent(
								Event
								{
									.type = EventType::PressEnd,
									.tag = m_tag.value(),
									.sourceNode = node,
								});
						}
						m_prevPressedRecursive = false;
					}
					else
					{
						m_prevPressedRecursive = true;
					}
					m_prevHovered = none;
					m_prevHoveredRecursive = none;
					m_prevPressed = none;
				}
				else
				{
					if (!node->isPressed())
					{
						if (m_prevPressed.has_value() && m_prevPressed.value())
						{
							canvas->fireEvent(
								Event
								{
									.type = EventType::PressEnd,
									.tag = m_tag.value(),
									.sourceNode = node,
								});
						}
						m_prevPressed = false;
					}
					else
					{
						m_prevPressed = true;
					}
					m_prevHovered = none;
					m_prevHoveredRecursive = none;
					m_prevPressedRecursive = none;
				}
				break;

			default:
				m_prevHovered = none;
				m_prevHoveredRecursive = none;
				m_prevPressed = none;
				m_prevPressedRecursive = none;
				break;
			}
		}
	};
}
