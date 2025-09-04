#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"

namespace noco
{
	enum class TweenTarget : uint8
	{
		None,
		Translate,
		Scale,
		Rotation,
		Color,
	};

	enum class TweenEasing : uint8
	{
		Linear,
		EaseInSine,
		EaseOutSine,
		EaseInOutSine,
		EaseInQuad,
		EaseOutQuad,
		EaseInOutQuad,
		EaseInCubic,
		EaseOutCubic,
		EaseInOutCubic,
		EaseInQuart,
		EaseOutQuart,
		EaseInOutQuart,
		EaseInQuint,
		EaseOutQuint,
		EaseInOutQuint,
		EaseInExpo,
		EaseOutExpo,
		EaseInOutExpo,
		EaseInCirc,
		EaseOutCirc,
		EaseInOutCirc,
		EaseInBack,
		EaseOutBack,
		EaseInOutBack,
		EaseInElastic,
		EaseOutElastic,
		EaseInOutElastic,
		EaseInBounce,
		EaseOutBounce,
		EaseInOutBounce,
	};

	enum class TweenLoopType : uint8
	{
		None,
		Loop,
		PingPong,
	};

	class Tween : public SerializableComponentBase, public std::enable_shared_from_this<Tween>
	{
	private:
		Property<bool> m_active;
		PropertyNonInteractive<TweenTarget> m_target;
		
		// Vec2用（Translate/Scale）
		PropertyNonInteractive<Vec2> m_fromVec2;
		PropertyNonInteractive<Vec2> m_toVec2;
		
		// double用（Rotation）
		PropertyNonInteractive<double> m_fromDouble;
		PropertyNonInteractive<double> m_toDouble;
		
		// ColorF用
		PropertyNonInteractive<ColorF> m_fromColor;
		PropertyNonInteractive<ColorF> m_toColor;
		
		PropertyNonInteractive<TweenEasing> m_easing;
		PropertyNonInteractive<double> m_duration;
		PropertyNonInteractive<double> m_delay;
		PropertyNonInteractive<TweenLoopType> m_loopType;
		PropertyNonInteractive<bool> m_restartOnActive;
		PropertyNonInteractive<bool> m_applyDuringDelay;
		Property<bool> m_isManual;
		SmoothProperty<double> m_manualTime;

		/* NonSerialized */ double m_elapsedTime = 0.0;
		/* NonSerialized */ bool m_isForward = true;  // PingPong用
		/* NonSerialized */ int32 m_loopCount = 0;
		/* NonSerialized */ Optional<bool> m_prevActive = none;

		[[nodiscard]]
		double applyEasing(double t) const;

		void updateTranslate(const std::shared_ptr<Node>& node, double progress);
		void updateScale(const std::shared_ptr<Node>& node, double progress);
		void updateRotation(const std::shared_ptr<Node>& node, double progress);
		void updateColor(const std::shared_ptr<Node>& node, double progress);

	public:
		explicit Tween(
			const PropertyValue<bool>& active = true,
			TweenTarget target = TweenTarget::None,
			TweenEasing easing = TweenEasing::EaseOutQuad,
			double duration = 1.0)
			: SerializableComponentBase{ U"Tween", { 
				&m_active, &m_target, 
				&m_fromVec2, &m_toVec2, 
				&m_fromDouble, &m_toDouble,
				&m_fromColor, &m_toColor,
				&m_easing, &m_duration, &m_delay, &m_loopType, &m_restartOnActive,
				&m_applyDuringDelay, &m_isManual, &m_manualTime
			} }
			, m_active{ U"active", active }
			, m_target{ U"target", target }
			, m_fromVec2{ U"fromVec2", Vec2::Zero() }
			, m_toVec2{ U"toVec2", Vec2::One() }
			, m_fromDouble{ U"fromDouble", 0.0 }
			, m_toDouble{ U"toDouble", 0.0 }
			, m_fromColor{ U"fromColor", ColorF{ 1.0, 1.0, 1.0, 1.0 } }
			, m_toColor{ U"toColor", ColorF{ 1.0, 1.0, 1.0, 0.0 } }
			, m_easing{ U"easing", easing }
			, m_duration{ U"duration", duration }
			, m_delay{ U"delay", 0.0 }
			, m_loopType{ U"loopType", TweenLoopType::None }
			, m_restartOnActive{ U"restartOnActive", true }
			, m_applyDuringDelay{ U"applyDuringDelay", false }
			, m_isManual{ U"isManual", false }
			, m_manualTime{ U"manualTime", 0.0 }
		{
		}

		void onActivated(const std::shared_ptr<Node>& node) override;
		void update(const std::shared_ptr<Node>& node) override;

		[[nodiscard]]
		const PropertyValue<bool>& active() const
		{
			return m_active.propertyValue();
		}

		std::shared_ptr<Tween> setActive(const PropertyValue<bool>& active)
		{
			m_active.setPropertyValue(active);
			return shared_from_this();
		}

		[[nodiscard]]
		TweenTarget target() const
		{
			return m_target.value();
		}

		std::shared_ptr<Tween> setTarget(TweenTarget target)
		{
			m_target.setValue(target);
			return shared_from_this();
		}

		[[nodiscard]]
		const Vec2& fromVec2() const
		{
			return m_fromVec2.value();
		}

		std::shared_ptr<Tween> setFromVec2(const Vec2& value)
		{
			m_fromVec2.setValue(value);
			return shared_from_this();
		}

		[[nodiscard]]
		const Vec2& toVec2() const
		{
			return m_toVec2.value();
		}

		std::shared_ptr<Tween> setToVec2(const Vec2& value)
		{
			m_toVec2.setValue(value);
			return shared_from_this();
		}

		[[nodiscard]]
		double fromDouble() const
		{
			return m_fromDouble.value();
		}

		std::shared_ptr<Tween> setFromDouble(double value)
		{
			m_fromDouble.setValue(value);
			return shared_from_this();
		}

		[[nodiscard]]
		double toDouble() const
		{
			return m_toDouble.value();
		}

		std::shared_ptr<Tween> setToDouble(double value)
		{
			m_toDouble.setValue(value);
			return shared_from_this();
		}

		[[nodiscard]]
		const ColorF& fromColor() const
		{
			return m_fromColor.value();
		}

		std::shared_ptr<Tween> setFromColor(const ColorF& value)
		{
			m_fromColor.setValue(value);
			return shared_from_this();
		}

		[[nodiscard]]
		const ColorF& toColor() const
		{
			return m_toColor.value();
		}

		std::shared_ptr<Tween> setToColor(const ColorF& value)
		{
			m_toColor.setValue(value);
			return shared_from_this();
		}

		[[nodiscard]]
		TweenEasing easing() const
		{
			return m_easing.value();
		}

		std::shared_ptr<Tween> setEasing(TweenEasing easing)
		{
			m_easing.setValue(easing);
			return shared_from_this();
		}

		[[nodiscard]]
		double duration() const
		{
			return m_duration.value();
		}

		std::shared_ptr<Tween> setDuration(double duration)
		{
			m_duration.setValue(duration);
			return shared_from_this();
		}

		[[nodiscard]]
		double delay() const
		{
			return m_delay.value();
		}

		std::shared_ptr<Tween> setDelay(double delay)
		{
			m_delay.setValue(delay);
			return shared_from_this();
		}

		[[nodiscard]]
		TweenLoopType loopType() const
		{
			return m_loopType.value();
		}

		std::shared_ptr<Tween> setLoopType(TweenLoopType loopType)
		{
			m_loopType.setValue(loopType);
			return shared_from_this();
		}

		[[nodiscard]]
		bool restartOnActive() const
		{
			return m_restartOnActive.value();
		}

		std::shared_ptr<Tween> setRestartOnActive(bool restartOnActive)
		{
			m_restartOnActive.setValue(restartOnActive);
			return shared_from_this();
		}

		[[nodiscard]]
		bool applyDuringDelay() const
		{
			return m_applyDuringDelay.value();
		}

		std::shared_ptr<Tween> setApplyDuringDelay(bool applyDuringDelay)
		{
			m_applyDuringDelay.setValue(applyDuringDelay);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<bool>& isManual() const
		{
			return m_isManual.propertyValue();
		}

		std::shared_ptr<Tween> setIsManual(const PropertyValue<bool>& isManual)
		{
			m_isManual.setPropertyValue(isManual);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<double>& manualTime() const
		{
			return m_manualTime.propertyValue();
		}

		std::shared_ptr<Tween> setManualTime(const PropertyValue<double>& manualTime)
		{
			m_manualTime.setPropertyValue(manualTime);
			return shared_from_this();
		}
	};
}