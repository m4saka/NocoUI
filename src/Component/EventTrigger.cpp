#include "NocoUI/Component/EventTrigger.hpp"

namespace noco
{
	namespace
	{
		/// @brief 押下状態からリピート発火の有無を判定
		[[nodiscard]]
		bool DetectPressRepeatFire(bool pressedHover, double intervalSec, double intervalSecFirst, Stopwatch* pStopwatch, double* pPrevTimeSec)
		{
			if (!pressedHover)
			{
				pStopwatch->reset();
				*pPrevTimeSec = 0.0;
				return false;
			}

			if (!pStopwatch->isRunning())
			{
				// 押下開始フレームで1回目を発火
				pStopwatch->restart();
				*pPrevTimeSec = 0.0;
				return true;
			}

			if (intervalSec <= 0.0)
			{
				return false;
			}

			const double effectiveIntervalSecFirst = intervalSecFirst <= 0.0 ? intervalSec : intervalSecFirst;
			const double timeSec = pStopwatch->sF();
			const int32 tickCount = static_cast<int32>(Max((timeSec - effectiveIntervalSecFirst + intervalSec) / intervalSec, 0.0));
			const int32 tickCountPrev = static_cast<int32>(Max((*pPrevTimeSec - effectiveIntervalSecFirst + intervalSec) / intervalSec, 0.0));
			*pPrevTimeSec = timeSec;
			return tickCount > tickCountPrev;
		}

		/// @brief 押下継続時間から長押し発火の有無を判定(離すまで1回のみ発火)
		[[nodiscard]]
		bool DetectPressHoldFire(bool pressedHover, double durationSec, Stopwatch* pStopwatch, bool* pFired)
		{
			if (!pressedHover)
			{
				pStopwatch->reset();
				*pFired = false;
				return false;
			}

			if (!pStopwatch->isRunning())
			{
				pStopwatch->restart();
				*pFired = false;
			}

			if (!*pFired && pStopwatch->sF() >= durationSec)
			{
				*pFired = true;
				return true;
			}

			return false;
		}
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
		const bool recursive = m_recursive.value();
		switch (triggerType)
		{
		case EventTriggerType::Click:
			if (node->isClicked(RecursiveYN{ recursive }))
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
			if (node->isRightClicked(RecursiveYN{ recursive }))
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
				if (node->isHovered(RecursiveYN::Yes))
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
				if (!node->isHovered(RecursiveYN::Yes))
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
				if (node->isPressed(RecursiveYN::Yes))
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
				if (!node->isPressed(RecursiveYN::Yes))
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
				if (node->isRightPressed(RecursiveYN::Yes))
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
				if (!node->isRightPressed(RecursiveYN::Yes))
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

		case EventTriggerType::PressRepeat:
			if (DetectPressRepeatFire(node->isPressedHover(RecursiveYN{ recursive }), m_repeatIntervalSec.value(), m_repeatIntervalSecFirst.value(), &m_pressRepeatStopwatch, &m_prevPressRepeatTimeSec))
			{
				canvas->fireEvent({ .triggerType = EventTriggerType::PressRepeat, .tag = m_tag.value(), .sourceNode = node });
			}
			m_prevHovered = none;
			m_prevHoveredRecursive = none;
			m_prevPressed = none;
			m_prevPressedRecursive = none;
			m_prevRightPressed = none;
			m_prevRightPressedRecursive = none;
			break;

		case EventTriggerType::PressHold:
			if (DetectPressHoldFire(node->isPressedHover(RecursiveYN{ recursive }), m_holdDurationSec.value(), &m_pressHoldStopwatch, &m_pressHoldFired))
			{
				canvas->fireEvent({ .triggerType = EventTriggerType::PressHold, .tag = m_tag.value(), .sourceNode = node });
			}
			m_prevHovered = none;
			m_prevHoveredRecursive = none;
			m_prevPressed = none;
			m_prevPressedRecursive = none;
			m_prevRightPressed = none;
			m_prevRightPressedRecursive = none;
			break;

		case EventTriggerType::RightPressRepeat:
			if (DetectPressRepeatFire(node->isRightPressedHover(RecursiveYN{ recursive }), m_repeatIntervalSec.value(), m_repeatIntervalSecFirst.value(), &m_rightPressRepeatStopwatch, &m_prevRightPressRepeatTimeSec))
			{
				canvas->fireEvent({ .triggerType = EventTriggerType::RightPressRepeat, .tag = m_tag.value(), .sourceNode = node });
			}
			m_prevHovered = none;
			m_prevHoveredRecursive = none;
			m_prevPressed = none;
			m_prevPressedRecursive = none;
			m_prevRightPressed = none;
			m_prevRightPressedRecursive = none;
			break;

		case EventTriggerType::RightPressHold:
			if (DetectPressHoldFire(node->isRightPressedHover(RecursiveYN{ recursive }), m_holdDurationSec.value(), &m_rightPressHoldStopwatch, &m_rightPressHoldFired))
			{
				canvas->fireEvent({ .triggerType = EventTriggerType::RightPressHold, .tag = m_tag.value(), .sourceNode = node });
			}
			m_prevHovered = none;
			m_prevHoveredRecursive = none;
			m_prevPressed = none;
			m_prevPressedRecursive = none;
			m_prevRightPressed = none;
			m_prevRightPressedRecursive = none;
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
