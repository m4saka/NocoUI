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
		const bool recursive = m_recursive.value();
		switch (triggerType)
		{
		case EventTriggerType::Click:
			if (recursive ? node->isClickedRecursive() : node->isClicked())
			{
				canvas->fireEvent({ .triggerType = EventTriggerType::Click, .tag = m_tag.value(), .sourceNode = node });
			}
			m_prevHovered = none;
			m_prevPressed = none;
			m_prevHoveredRecursive = none;
			m_prevPressedRecursive = none;
			m_prevRightPressed = none;
			m_prevRightPressedRecursive = none;
			break;

		case EventTriggerType::RightClick:
			if (recursive ? node->isRightClickedRecursive() : node->isRightClicked())
			{
				canvas->fireEvent({ .triggerType = EventTriggerType::RightClick, .tag = m_tag.value(), .sourceNode = node });
			}
			m_prevHovered = none;
			m_prevPressed = none;
			m_prevHoveredRecursive = none;
			m_prevPressedRecursive = none;
			m_prevRightPressed = none;
			m_prevRightPressedRecursive = none;
			break;

		case EventTriggerType::HoverStart:
			if (recursive)
			{
				if (node->isHoveredRecursive())
				{
					if (m_prevHoveredRecursive.has_value() && !m_prevHoveredRecursive.value())
					{
						canvas->fireEvent({ .triggerType = EventTriggerType::HoverStart, .tag = m_tag.value(), .sourceNode = node });
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
				m_prevRightPressed = none;
				m_prevRightPressedRecursive = none;
			}
			else
			{
				if (node->isHovered())
				{
					if (m_prevHovered.has_value() && !m_prevHovered.value())
					{
						canvas->fireEvent({ .triggerType = EventTriggerType::HoverStart, .tag = m_tag.value(), .sourceNode = node });
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
				m_prevRightPressed = none;
				m_prevRightPressedRecursive = none;
			}
			break;

		case EventTriggerType::HoverEnd:
			if (recursive)
			{
				if (!node->isHoveredRecursive())
				{
					if (m_prevHoveredRecursive.has_value() && m_prevHoveredRecursive.value())
					{
						canvas->fireEvent({ .triggerType = EventTriggerType::HoverEnd, .tag = m_tag.value(), .sourceNode = node });
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
				m_prevRightPressed = none;
				m_prevRightPressedRecursive = none;
			}
			else
			{
				if (!node->isHovered())
				{
					if (m_prevHovered.has_value() && m_prevHovered.value())
					{
						canvas->fireEvent({ .triggerType = EventTriggerType::HoverEnd, .tag = m_tag.value(), .sourceNode = node });
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
				m_prevRightPressed = none;
				m_prevRightPressedRecursive = none;
			}
			break;

		case EventTriggerType::PressStart:
			if (recursive)
			{
				if (node->isPressedRecursive())
				{
					if (m_prevPressedRecursive.has_value() && !m_prevPressedRecursive.value())
					{
						canvas->fireEvent({ .triggerType = EventTriggerType::PressStart, .tag = m_tag.value(), .sourceNode = node });
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
				m_prevRightPressed = none;
				m_prevRightPressedRecursive = none;
			}
			else
			{
				if (node->isPressed())
				{
					if (m_prevPressed.has_value() && !m_prevPressed.value())
					{
						canvas->fireEvent({ .triggerType = EventTriggerType::PressStart, .tag = m_tag.value(), .sourceNode = node });
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
				m_prevRightPressed = none;
				m_prevRightPressedRecursive = none;
			}
			break;

		case EventTriggerType::PressEnd:
			if (recursive)
			{
				if (!node->isPressedRecursive())
				{
					if (m_prevPressedRecursive.has_value() && m_prevPressedRecursive.value())
					{
						canvas->fireEvent({ .triggerType = EventTriggerType::PressEnd, .tag = m_tag.value(), .sourceNode = node });
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
				m_prevRightPressed = none;
				m_prevRightPressedRecursive = none;
			}
			else
			{
				if (!node->isPressed())
				{
					if (m_prevPressed.has_value() && m_prevPressed.value())
					{
						canvas->fireEvent({ .triggerType = EventTriggerType::PressEnd, .tag = m_tag.value(), .sourceNode = node });
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
				m_prevRightPressed = none;
				m_prevRightPressedRecursive = none;
			}
			break;

		case EventTriggerType::RightPressStart:
			if (recursive)
			{
				if (node->isRightPressedRecursive())
				{
					if (m_prevRightPressedRecursive.has_value() && !m_prevRightPressedRecursive.value())
					{
						canvas->fireEvent({ .triggerType = EventTriggerType::RightPressStart, .tag = m_tag.value(), .sourceNode = node });
					}
					m_prevRightPressedRecursive = true;
				}
				else
				{
					m_prevRightPressedRecursive = false;
				}
				m_prevHovered = none;
				m_prevHoveredRecursive = none;
				m_prevPressed = none;
				m_prevPressedRecursive = none;
				m_prevRightPressed = none;
			}
			else
			{
				if (node->isRightPressed())
				{
					if (m_prevRightPressed.has_value() && !m_prevRightPressed.value())
					{
						canvas->fireEvent({ .triggerType = EventTriggerType::RightPressStart, .tag = m_tag.value(), .sourceNode = node });
					}
					m_prevRightPressed = true;
				}
				else
				{
					m_prevRightPressed = false;
				}
				m_prevHovered = none;
				m_prevHoveredRecursive = none;
				m_prevPressed = none;
				m_prevPressedRecursive = none;
				m_prevRightPressedRecursive = none;
			}
			break;

		case EventTriggerType::RightPressEnd:
			if (recursive)
			{
				if (!node->isRightPressedRecursive())
				{
					if (m_prevRightPressedRecursive.has_value() && m_prevRightPressedRecursive.value())
					{
						canvas->fireEvent({ .triggerType = EventTriggerType::RightPressEnd, .tag = m_tag.value(), .sourceNode = node });
					}
					m_prevRightPressedRecursive = false;
				}
				else
				{
					m_prevRightPressedRecursive = true;
				}
				m_prevHovered = none;
				m_prevHoveredRecursive = none;
				m_prevPressed = none;
				m_prevPressedRecursive = none;
				m_prevRightPressed = none;
			}
			else
			{
				if (!node->isRightPressed())
				{
					if (m_prevRightPressed.has_value() && m_prevRightPressed.value())
					{
						canvas->fireEvent({ .triggerType = EventTriggerType::RightPressEnd, .tag = m_tag.value(), .sourceNode = node });
					}
					m_prevRightPressed = false;
				}
				else
				{
					m_prevRightPressed = true;
				}
				m_prevHovered = none;
				m_prevHoveredRecursive = none;
				m_prevPressed = none;
				m_prevPressedRecursive = none;
				m_prevRightPressedRecursive = none;
			}
			break;

		default:
			m_prevHovered = none;
			m_prevHoveredRecursive = none;
			m_prevPressed = none;
			m_prevPressedRecursive = none;
			m_prevRightPressed = none;
			m_prevRightPressedRecursive = none;
			break;
		}
	}
}