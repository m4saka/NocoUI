#include "NocoUI/Component/Tween.hpp"
#include "NocoUI/Node.hpp"

namespace noco
{
	double Tween::applyEasing(double t) const
	{
		switch (m_easing.value())
		{
		case TweenEasing::Linear:
			return t;
		case TweenEasing::EaseInSine:
			return EaseInSine(t);
		case TweenEasing::EaseOutSine:
			return EaseOutSine(t);
		case TweenEasing::EaseInOutSine:
			return EaseInOutSine(t);
		case TweenEasing::EaseInQuad:
			return EaseInQuad(t);
		case TweenEasing::EaseOutQuad:
			return EaseOutQuad(t);
		case TweenEasing::EaseInOutQuad:
			return EaseInOutQuad(t);
		case TweenEasing::EaseInCubic:
			return EaseInCubic(t);
		case TweenEasing::EaseOutCubic:
			return EaseOutCubic(t);
		case TweenEasing::EaseInOutCubic:
			return EaseInOutCubic(t);
		case TweenEasing::EaseInQuart:
			return EaseInQuart(t);
		case TweenEasing::EaseOutQuart:
			return EaseOutQuart(t);
		case TweenEasing::EaseInOutQuart:
			return EaseInOutQuart(t);
		case TweenEasing::EaseInQuint:
			return EaseInQuint(t);
		case TweenEasing::EaseOutQuint:
			return EaseOutQuint(t);
		case TweenEasing::EaseInOutQuint:
			return EaseInOutQuint(t);
		case TweenEasing::EaseInExpo:
			return EaseInExpo(t);
		case TweenEasing::EaseOutExpo:
			return EaseOutExpo(t);
		case TweenEasing::EaseInOutExpo:
			return EaseInOutExpo(t);
		case TweenEasing::EaseInCirc:
			return EaseInCirc(t);
		case TweenEasing::EaseOutCirc:
			return EaseOutCirc(t);
		case TweenEasing::EaseInOutCirc:
			return EaseInOutCirc(t);
		case TweenEasing::EaseInBack:
			return EaseInBack(t);
		case TweenEasing::EaseOutBack:
			return EaseOutBack(t);
		case TweenEasing::EaseInOutBack:
			return EaseInOutBack(t);
		case TweenEasing::EaseInElastic:
			return EaseInElastic(t);
		case TweenEasing::EaseOutElastic:
			return EaseOutElastic(t);
		case TweenEasing::EaseInOutElastic:
			return EaseInOutElastic(t);
		case TweenEasing::EaseInBounce:
			return EaseInBounce(t);
		case TweenEasing::EaseOutBounce:
			return EaseOutBounce(t);
		case TweenEasing::EaseInOutBounce:
			return EaseInOutBounce(t);
		default:
			Logger << U"[NocoUI warning] Unknown TweenEasing: {}"_fmt(static_cast<std::underlying_type_t<TweenEasing>>(m_easing.value()));
			return t;
		}
	}

	void Tween::updateTranslate(const std::shared_ptr<Node>& node, double progress)
	{
		const Vec2& from = m_translateFrom.value();
		const Vec2& to = m_translateTo.value();
		const Vec2 interpolated = Math::Lerp(from, to, progress);

		auto& transform = node->transform();
		transform.translate().setCurrentFrameOverride(interpolated);
	}

	void Tween::updateScale(const std::shared_ptr<Node>& node, double progress)
	{
		const Vec2& from = m_scaleFrom.value();
		const Vec2& to = m_scaleTo.value();
		const Vec2 interpolated = Math::Lerp(from, to, progress);

		auto& transform = node->transform();
		transform.scale().setCurrentFrameOverride(interpolated);
	}

	void Tween::updateRotation(const std::shared_ptr<Node>& node, double progress)
	{
		const double from = m_rotationFrom.value();
		const double to = m_rotationTo.value();
		const double interpolated = Math::Lerp(from, to, progress);

		auto& transform = node->transform();
		transform.rotation().setCurrentFrameOverride(interpolated);
	}

	void Tween::updateColor(const std::shared_ptr<Node>& node, double progress)
	{
		const Color& from = m_colorFrom.value();
		const Color& to = m_colorTo.value();
		// ColorFに変換して補間し、結果をColorに戻す
		const ColorF fromF{ from };
		const ColorF toF{ to };
		const ColorF interpolatedF = fromF.lerp(toF, progress);
		const Color interpolated{ interpolatedF };

		auto& transform = node->transform();
		transform.color().setCurrentFrameOverride(interpolated);
	}

	void Tween::onActivated(const std::shared_ptr<Node>&)
	{
		if (m_triggerType.value() != EventTriggerType::None)
		{
			// トリガー発火待ちの状態に戻す
			m_triggerFired = false;
			m_triggerFinished = false;
			m_triggerFireDetector.reset();
			return;
		}
		if (m_active.value() && m_restartOnActive.value())
		{
			m_stopwatch.restart();
		}
	}

	void Tween::update(const std::shared_ptr<Node>& node)
	{
		// トリガー再生の場合は通常再生と別処理(restartOnActive、manualModeは無視される)
		if (m_triggerType.value() != EventTriggerType::None)
		{
			updateWithTrigger(node);
			return;
		}

		const bool currentActive = m_active.value();

		if (m_restartOnActive.value() && m_prevActive.has_value() && !m_prevActive.value() && currentActive)
		{
			// 最初から再生
			m_stopwatch.restart();
		}
		m_prevActive = currentActive;

		if (!m_stopwatch.isStarted())
		{
			m_stopwatch.start();
		}

		if (!currentActive)
		{
			return;
		}

		applyAtTime(node, m_manualMode.value() ? m_manualTime.value() : m_stopwatch.sF());
	}

	void Tween::applyAtTime(const std::shared_ptr<Node>& node, double time)
	{
		const double loopDuration = m_loopDuration.value();

		// loopDurationが指定されている場合は適用
		if (loopDuration > 0.0 && m_loopType.value() != TweenLoopType::None)
		{
			time = Math::Fmod(time, loopDuration);
		}
		
		// delay時間中の処理
		if (time < m_delay.value())
		{
			if (m_applyDuringDelay.value())
			{
				const double easedProgress = applyEasing(0.0);

				// 有効なプロパティに0%の値を適用
				if (m_translateEnabled.value())
				{
					updateTranslate(node, easedProgress);
				}
				if (m_scaleEnabled.value())
				{
					updateScale(node, easedProgress);
				}
				if (m_rotationEnabled.value())
				{
					updateRotation(node, easedProgress);
				}
				if (m_colorEnabled.value())
				{
					updateColor(node, easedProgress);
				}
			}
			return;
		}
		
		// アニメーション時間を計算
		const double animationTime = time - m_delay.value();
		const double duration = m_duration.value();
			
		if (duration <= 0.0)
		{
			// durationが0以下の場合は即座に100%の値にする
			const double easedProgress = applyEasing(1.0);

			// 有効なプロパティに100%の値を適用
			if (m_translateEnabled.value())
			{
				updateTranslate(node, easedProgress);
			}
			if (m_scaleEnabled.value())
			{
				updateScale(node, easedProgress);
			}
			if (m_rotationEnabled.value())
			{
				updateRotation(node, easedProgress);
			}
			if (m_colorEnabled.value())
			{
				updateColor(node, easedProgress);
			}
			return;
		}
		
		double rawProgress = animationTime / duration;
	
		// ループ処理（loopDurationが設定されていない場合のみ）
		const auto loopType = m_loopType.value();
		if (loopDuration <= 0.0 && loopType != TweenLoopType::None && rawProgress >= 1.0)
		{
			if (loopType == TweenLoopType::Loop)
			{
				// 通常ループ
				rawProgress = Math::Fmod(rawProgress, 1.0);
			}
			else if (loopType == TweenLoopType::PingPong)
			{
				// PingPongループ
				const int32 cycle = static_cast<int32>(rawProgress);
				rawProgress = Math::Fmod(rawProgress, 1.0);

				// 偶数サイクルは順方向、奇数サイクルは逆方向
				if (cycle % 2 == 1)
				{
					rawProgress = 1.0 - rawProgress;
				}
			}
		}
		else if (loopDuration > 0.0 && loopType == TweenLoopType::PingPong)
		{
			if (duration > 0.0)
			{
				const double pingPongCycleDuration = duration * 2;
				const double animTimeInCycle = Math::Fmod(animationTime, pingPongCycleDuration);

				if (animTimeInCycle <= duration)
				{
					rawProgress = animTimeInCycle / duration;
				}
				else
				{
					rawProgress = 1.0 - ((animTimeInCycle - duration) / duration);
				}
			}
		}
		// ループしない場合、アニメーション終了後は最終値を維持
		else if (loopType == TweenLoopType::None && rawProgress > 1.0)
		{
			rawProgress = 1.0;
		}

		// イージング適用
		const double clampedProgress = Math::Clamp(rawProgress, 0.0, 1.0);
		const double easedProgress = applyEasing(clampedProgress);

		// 有効なプロパティに値を反映
		if (m_translateEnabled.value())
		{
			updateTranslate(node, easedProgress);
		}
		if (m_scaleEnabled.value())
		{
			updateScale(node, easedProgress);
		}
		if (m_rotationEnabled.value())
		{
			updateRotation(node, easedProgress);
		}
		if (m_colorEnabled.value())
		{
			updateColor(node, easedProgress);
		}
	}

	void Tween::updateWithTrigger(const std::shared_ptr<Node>& node)
	{
		// トリガー判定はEventTriggerと同様に最寄りのヒットテスト対象ノードで行う
		// (Tweenは演出対象のノード自身に付けるため、ヒットテスト対象でない場合は祖先のインタラクションに反応させる)
		std::shared_ptr<Node> targetNode = node;
		while (targetNode && !targetNode->isHitTarget())
		{
			targetNode = targetNode->parentNode();
		}

		// 発火判定は毎フレーム行う(再生中に再発火した場合は最初から再生し直す)
		const bool fired = targetNode && m_triggerFireDetector.update(targetNode, m_triggerType.value(), RecursiveYN{ m_triggerRecursive.value() }, m_triggerRepeatIntervalSec.value(), m_triggerRepeatIntervalSecFirst.value(), m_triggerHoldDurationSec.value());

		if (!m_active.value())
		{
			// 非アクティブ中の発火は無視する(発火判定の状態更新のみ行う)
			return;
		}

		if (fired)
		{
			m_stopwatch.restart();
			m_triggerFired = true;
			m_triggerFinished = false;
		}

		if (!m_triggerFired || m_triggerFinished)
		{
			return;
		}

		const double time = m_stopwatch.sF();
		if (m_loopType.value() == TweenLoopType::None)
		{
			const double totalTime = m_delay.value() + m_duration.value();
			if (time >= totalTime)
			{
				// 最終値を一度だけ適用して終了(以降は適用されなくなり、各プロパティは元の値に戻る)
				applyAtTime(node, totalTime);
				m_triggerFinished = true;
				return;
			}
		}
		applyAtTime(node, time);
	}

	std::shared_ptr<Tween> Tween::setActive(const PropertyValue<bool>& active)
	{
		bool prevActive = m_active.value();

		m_active.setPropertyValue(active);

		// トリガー再生の場合はrestartOnActiveを無視する
		if (m_triggerType.value() == EventTriggerType::None && m_restartOnActive.value() && !prevActive && m_active.value())
		{
			// restartOnActiveが有効の場合、非アクティブ→アクティブに変化した場合は最初からやり直す
			m_stopwatch.restart();
		}

		return shared_from_this();
	}

	bool Tween::isPlaying() const
	{
		if (!m_active.value())
		{
			return false;
		}

		// トリガー再生の場合は発火済みかつ終了前のみ再生中とみなす
		if (m_triggerType.value() != EventTriggerType::None)
		{
			if (!m_triggerFired || m_triggerFinished)
			{
				return false;
			}
			if (m_loopType.value() != TweenLoopType::None)
			{
				return true;
			}
			return m_stopwatch.sF() < m_delay.value() + m_duration.value();
		}

		const auto loopType = m_loopType.value();

		// ループが有効な場合は常にtrue
		if (loopType != TweenLoopType::None)
		{
			return true;
		}

		// ループしない場合は、delay + duration以内かチェック
		const double time = m_manualMode.value() ? m_manualTime.value() : m_stopwatch.sF();
		const double totalTime = m_delay.value() + m_duration.value();

		return time < totalTime;
	}
}
