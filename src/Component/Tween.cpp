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
		if (m_active.value() && m_restartOnActive.value())
		{
			m_stopwatch.restart();
		}
	}

	void Tween::update(const std::shared_ptr<Node>& node)
	{
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

		double time;
		const double loopDuration = m_loopDuration.value();

		if (m_manualMode.value())
		{
			time = m_manualTime.value();
			// マニュアルモードでloopDurationが設定されている場合、manualTimeを直接fmod
			if (loopDuration > 0.0 && m_loopType.value() != TweenLoopType::None)
			{
				time = Math::Fmod(time, loopDuration);
			}
		}
		else
		{
			time = m_stopwatch.sF();
			// 自動モードでloopDurationが設定されている場合
			if (loopDuration > 0.0 && m_loopType.value() != TweenLoopType::None)
			{
				time = Math::Fmod(time, loopDuration);
			}
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
}
