#pragma once
#include <Siv3D.hpp>
#include "../YN.hpp"

namespace noco
{
	class Node;
	enum class EventTriggerType : uint8;
}

namespace noco::detail
{
	/// @brief トリガー発火判定の状態機械(EventTriggerとTweenで共用)
	class TriggerFireDetector
	{
	private:
		// Hoveredのみ初期値はfalseとする
		// (初回update時に既にホバーしている場合も発火させたいため。ただし、他triggerTypeからの変更タイミングで発火させてはいけないため、HoveredもOptionalを利用する必要がある)
		Optional<bool> m_prevHovered = false;
		Optional<bool> m_prevPressed = none;
		Optional<bool> m_prevRightPressed = none;
		Optional<bool> m_prevRecursive = none;
		Stopwatch m_pressRepeatStopwatch;
		double m_prevPressRepeatTimeSec = 0.0;
		Stopwatch m_rightPressRepeatStopwatch;
		double m_prevRightPressRepeatTimeSec = 0.0;
		Stopwatch m_pressHoldStopwatch;
		bool m_pressHoldFired = false;
		Stopwatch m_rightPressHoldStopwatch;
		bool m_rightPressHoldFired = false;

		/// @brief 押下状態からリピート発火の有無を判定
		[[nodiscard]]
		static bool DetectPressRepeatFire(bool pressedHover, double intervalSec, double intervalSecFirst, Stopwatch* pStopwatch, double* pPrevTimeSec);

		/// @brief 押下継続時間から長押し発火の有無を判定(離すまで1回のみ発火)
		[[nodiscard]]
		static bool DetectPressHoldFire(bool pressedHover, double durationSec, Stopwatch* pStopwatch, bool* pFired);

	public:
		/// @brief トリガー発火の有無を判定(毎フレーム呼び出す)
		[[nodiscard]]
		bool update(const std::shared_ptr<Node>& node, EventTriggerType triggerType, RecursiveYN recursive, double repeatIntervalSec, double repeatIntervalSecFirst, double holdDurationSec);

		/// @brief 発火判定の状態を初期状態へ戻す
		void reset();
	};
}
