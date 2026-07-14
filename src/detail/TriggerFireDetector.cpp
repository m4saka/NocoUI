#include "NocoUI/detail/TriggerFireDetector.hpp"
#include "NocoUI/Node.hpp"
#include "NocoUI/Canvas.hpp"

namespace noco::detail
{
	bool TriggerFireDetector::DetectPressRepeatFire(bool pressedHover, double intervalSec, double intervalSecFirst, Stopwatch* pStopwatch, double* pPrevTimeSec)
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

	bool TriggerFireDetector::DetectPressHoldFire(bool pressedHover, double durationSec, Stopwatch* pStopwatch, bool* pFired)
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

	bool TriggerFireDetector::update(const std::shared_ptr<Node>& node, EventTriggerType triggerType, RecursiveYN recursive, double repeatIntervalSec, double repeatIntervalSecFirst, double holdDurationSec)
	{
		if (m_prevRecursive.has_value() && *m_prevRecursive != recursive.getBool())
		{
			// recursiveが変更されたフレームでは発火させない
			m_prevHovered = none;
			m_prevPressed = none;
			m_prevRightPressed = none;
		}
		m_prevRecursive = recursive.getBool();

		// 使用しない前回状態は毎フレームリセットする(他triggerTypeからの変更フレームで発火させないため)
		switch (triggerType)
		{
		case EventTriggerType::Click:
			m_prevHovered = none;
			m_prevPressed = none;
			m_prevRightPressed = none;
			return node->isClicked(recursive);

		case EventTriggerType::RightClick:
			m_prevHovered = none;
			m_prevPressed = none;
			m_prevRightPressed = none;
			return node->isRightClicked(recursive);

		case EventTriggerType::HoverStart:
		{
			m_prevPressed = none;
			m_prevRightPressed = none;
			const bool hovered = node->isHovered(recursive);
			const bool fire = m_prevHovered.has_value() && !*m_prevHovered && hovered;
			m_prevHovered = hovered;
			return fire;
		}

		case EventTriggerType::HoverEnd:
		{
			m_prevPressed = none;
			m_prevRightPressed = none;
			const bool hovered = node->isHovered(recursive);
			const bool fire = m_prevHovered.has_value() && *m_prevHovered && !hovered;
			m_prevHovered = hovered;
			return fire;
		}

		case EventTriggerType::PressStart:
		{
			m_prevHovered = none;
			m_prevRightPressed = none;
			const bool pressed = node->isPressed(recursive);
			const bool fire = m_prevPressed.has_value() && !*m_prevPressed && pressed;
			m_prevPressed = pressed;
			return fire;
		}

		case EventTriggerType::PressEnd:
		{
			m_prevHovered = none;
			m_prevRightPressed = none;
			const bool pressed = node->isPressed(recursive);
			const bool fire = m_prevPressed.has_value() && *m_prevPressed && !pressed;
			m_prevPressed = pressed;
			return fire;
		}

		case EventTriggerType::PressRepeat:
			m_prevHovered = none;
			m_prevPressed = none;
			m_prevRightPressed = none;
			return DetectPressRepeatFire(node->isPressedHover(recursive), repeatIntervalSec, repeatIntervalSecFirst, &m_pressRepeatStopwatch, &m_prevPressRepeatTimeSec);

		case EventTriggerType::PressHold:
			m_prevHovered = none;
			m_prevPressed = none;
			m_prevRightPressed = none;
			return DetectPressHoldFire(node->isPressedHover(recursive), holdDurationSec, &m_pressHoldStopwatch, &m_pressHoldFired);

		case EventTriggerType::RightPressStart:
		{
			m_prevHovered = none;
			m_prevPressed = none;
			const bool rightPressed = node->isRightPressed(recursive);
			const bool fire = m_prevRightPressed.has_value() && !*m_prevRightPressed && rightPressed;
			m_prevRightPressed = rightPressed;
			return fire;
		}

		case EventTriggerType::RightPressEnd:
		{
			m_prevHovered = none;
			m_prevPressed = none;
			const bool rightPressed = node->isRightPressed(recursive);
			const bool fire = m_prevRightPressed.has_value() && *m_prevRightPressed && !rightPressed;
			m_prevRightPressed = rightPressed;
			return fire;
		}

		case EventTriggerType::RightPressRepeat:
			m_prevHovered = none;
			m_prevPressed = none;
			m_prevRightPressed = none;
			return DetectPressRepeatFire(node->isRightPressedHover(recursive), repeatIntervalSec, repeatIntervalSecFirst, &m_rightPressRepeatStopwatch, &m_prevRightPressRepeatTimeSec);

		case EventTriggerType::RightPressHold:
			m_prevHovered = none;
			m_prevPressed = none;
			m_prevRightPressed = none;
			return DetectPressHoldFire(node->isRightPressedHover(recursive), holdDurationSec, &m_rightPressHoldStopwatch, &m_rightPressHoldFired);

		default:
			m_prevHovered = none;
			m_prevPressed = none;
			m_prevRightPressed = none;
			return false;
		}
	}

	void TriggerFireDetector::reset()
	{
		m_prevHovered = false;
		m_prevPressed = none;
		m_prevRightPressed = none;
		m_prevRecursive = none;
		m_pressRepeatStopwatch.reset();
		m_prevPressRepeatTimeSec = 0.0;
		m_rightPressRepeatStopwatch.reset();
		m_prevRightPressRepeatTimeSec = 0.0;
		m_pressHoldStopwatch.reset();
		m_pressHoldFired = false;
		m_rightPressHoldStopwatch.reset();
		m_rightPressHoldFired = false;
	}
}
