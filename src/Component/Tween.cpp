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

	void Tween::updatePosition(const std::shared_ptr<Node>& node, double progress)
	{
		const Vec2& value1 = m_value1Vec2.value();
		const Vec2& value2 = m_value2Vec2.value();
		const Vec2 interpolated = Math::Lerp(value1, value2, progress);

		auto& transform = node->transform();
		transform.position().setOverrideValue(interpolated);
	}

	void Tween::updateScale(const std::shared_ptr<Node>& node, double progress)
	{
		const Vec2& value1 = m_value1Vec2.value();
		const Vec2& value2 = m_value2Vec2.value();
		const Vec2 interpolated = Math::Lerp(value1, value2, progress);

		auto& transform = node->transform();
		transform.scale().setOverrideValue(interpolated);
	}

	void Tween::updateRotation(const std::shared_ptr<Node>& node, double progress)
	{
		const double value1 = m_value1Double.value();
		const double value2 = m_value2Double.value();
		const double interpolated = Math::Lerp(value1, value2, progress);

		auto& transform = node->transform();
		transform.rotation().setOverrideValue(interpolated);
	}

	void Tween::updateColor(const std::shared_ptr<Node>& node, double progress)
	{
		const ColorF& value1 = m_value1Color.value();
		const ColorF& value2 = m_value2_color.value();
		const ColorF interpolated = value1.lerp(value2, progress);

		auto& transform = node->transform();
		transform.color().setOverrideValue(interpolated);
	}

	void Tween::update(const std::shared_ptr<Node>& node)
	{
		const bool currentActive = m_active.value();
		if (m_restartsOnActive.value() && m_prevActive.has_value() && !m_prevActive.value() && currentActive)
		{
			// 最初から再生
			m_elapsedTime = 0.0;
			m_loopCount = 0;
			m_isForward = true;
		}
		m_prevActive = currentActive;
		
		if (!currentActive)
		{
			return;
		}
		
		const double deltaTime = Scene::DeltaTime();
		m_elapsedTime += deltaTime;
		
		// delay時間中は0%扱いで適用
		if (m_elapsedTime < m_delay.value())
		{
			const double easedProgress = applyEasing(0.0);
			switch (m_target.value())
			{
			case TweenTarget::None:
				// 何もしない
				break;
			case TweenTarget::Position:
				updatePosition(node, easedProgress);
				break;
			case TweenTarget::Scale:
				updateScale(node, easedProgress);
				break;
			case TweenTarget::Rotation:
				updateRotation(node, easedProgress);
				break;
			case TweenTarget::Color:
				updateColor(node, easedProgress);
				break;
			}
			return;
		}
		
		// アニメーション時間を計算
		const double animationTime = m_elapsedTime - m_delay.value();
		const double duration = m_duration.value();
		
		if (duration <= 0.0)
		{
			// durationが0以下の場合は即座に100%の値にする
			const double easedProgress = applyEasing(1.0);
			switch (m_target.value())
			{
			case TweenTarget::None:
				// 何もしない
				break;
			case TweenTarget::Position:
				updatePosition(node, easedProgress);
				break;
			case TweenTarget::Scale:
				updateScale(node, easedProgress);
				break;
			case TweenTarget::Rotation:
				updateRotation(node, easedProgress);
				break;
			case TweenTarget::Color:
				updateColor(node, easedProgress);
				break;
			}
			return;
		}
		
		double rawProgress = animationTime / duration;
		
		// ループ処理
		const auto loopType = m_loopType.value();
		if (loopType != TweenLoopType::None && rawProgress >= 1.0)
		{
			if (loopType == TweenLoopType::Loop)
			{
				// 通常ループ
				rawProgress = Math::Fmod(rawProgress, 1.0);
				m_loopCount = static_cast<int>(animationTime / duration);
			}
			else if (loopType == TweenLoopType::PingPong)
			{
				// PingPongループ
				const int cycle = static_cast<int>(rawProgress);
				rawProgress = Math::Fmod(rawProgress, 1.0);
				
				// 偶数サイクルは順方向、奇数サイクルは逆方向
				if (cycle % 2 == 1)
				{
					rawProgress = 1.0 - rawProgress;
				}
				
				m_loopCount = cycle;
			}
		}

		// イージング適用
		const double clampedProgress = Math::Clamp(rawProgress, 0.0, 1.0);
		const double easedProgress = applyEasing(clampedProgress);
		
		// 値を反映
		switch (m_target.value())
		{
		case TweenTarget::None:
			// 何もしない
			break;
		case TweenTarget::Position:
			updatePosition(node, easedProgress);
			break;
		case TweenTarget::Scale:
			updateScale(node, easedProgress);
			break;
		case TweenTarget::Color:
			updateColor(node, easedProgress);
			break;
		case TweenTarget::Rotation:
			updateRotation(node, easedProgress);
			break;
		}
	}

	void Tween::updateInactive(const std::shared_ptr<Node>& node)
	{
		m_prevActive = false;
	}
}
