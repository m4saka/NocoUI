#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"

namespace noco
{
	enum class TweenTarget : uint8
	{
		None,
		Position,
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
		
		// Vec2用（Position/Scale）
		PropertyNonInteractive<Vec2> m_value1_vec2;
		PropertyNonInteractive<Vec2> m_value2_vec2;
		
		// double用（Rotation）
		PropertyNonInteractive<double> m_value1_double;
		PropertyNonInteractive<double> m_value2_double;
		
		// ColorF用
		PropertyNonInteractive<ColorF> m_value1_color;
		PropertyNonInteractive<ColorF> m_value2_color;
		
		PropertyNonInteractive<TweenEasing> m_easing;
		PropertyNonInteractive<double> m_duration;
		PropertyNonInteractive<double> m_delay;
		PropertyNonInteractive<TweenLoopType> m_loopType;
		PropertyNonInteractive<bool> m_restartsOnActive;

		/* NonSerialized */ double m_elapsedTime = 0.0;
		/* NonSerialized */ bool m_isForward = true;  // PingPong用
		/* NonSerialized */ int m_loopCount = 0;
		/* NonSerialized */ Optional<bool> m_prevActive = none;

		[[nodiscard]]
		double applyEasing(double t) const;

		void updatePosition(const std::shared_ptr<Node>& node, double progress);
		void updateScale(const std::shared_ptr<Node>& node, double progress);
		void updateRotation(const std::shared_ptr<Node>& node, double progress);
		void updateColor(const std::shared_ptr<Node>& node, double progress);

	public:
		explicit Tween(
			const PropertyValue<bool>& active = true,
			TweenTarget target = TweenTarget::None,
			const Vec2& value1_vec2 = Vec2::Zero(),
			const Vec2& value2_vec2 = Vec2::One(),
			double value1_double = 0.0,
			double value2_double = 0.0,
			const ColorF& value1_color = ColorF{ 1.0, 1.0, 1.0, 1.0 },
			const ColorF& value2_color = ColorF{ 1.0, 1.0, 1.0, 0.0 },
			TweenEasing easing = TweenEasing::EaseOutQuad,
			double duration = 1.0,
			double delay = 0.0,
			TweenLoopType loopType = TweenLoopType::None,
			ActiveYN restartsOnActive = ActiveYN::Yes)
			: SerializableComponentBase{ U"Tween", { 
				&m_active, &m_target, 
				&m_value1_vec2, &m_value2_vec2, 
				&m_value1_double, &m_value2_double,
				&m_value1_color, &m_value2_color,
				&m_easing, &m_duration, &m_delay, &m_loopType, &m_restartsOnActive 
			} }
			, m_active{ U"active", active }
			, m_target{ U"target", target }
			, m_value1_vec2{ U"value1_vec2", value1_vec2 }
			, m_value2_vec2{ U"value2_vec2", value2_vec2 }
			, m_value1_double{ U"value1_double", value1_double }
			, m_value2_double{ U"value2_double", value2_double }
			, m_value1_color{ U"value1_color", value1_color }
			, m_value2_color{ U"value2_color", value2_color }
			, m_easing{ U"easing", easing }
			, m_duration{ U"duration", duration }
			, m_delay{ U"delay", delay }
			, m_loopType{ U"loopType", loopType }
			, m_restartsOnActive{ U"restartsOnActive", restartsOnActive.getBool() }
		{
		}

		void update(const std::shared_ptr<Node>& node) override;
		void updateInactive(const std::shared_ptr<Node>& node) override;

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
		const Vec2& value1_vec2() const
		{
			return m_value1_vec2.value();
		}

		std::shared_ptr<Tween> setValue1Vec2(const Vec2& value)
		{
			m_value1_vec2.setValue(value);
			return shared_from_this();
		}

		[[nodiscard]]
		const Vec2& value2_vec2() const
		{
			return m_value2_vec2.value();
		}

		std::shared_ptr<Tween> setValue2Vec2(const Vec2& value)
		{
			m_value2_vec2.setValue(value);
			return shared_from_this();
		}

		[[nodiscard]]
		double value1_double() const
		{
			return m_value1_double.value();
		}

		std::shared_ptr<Tween> setValue1Double(double value)
		{
			m_value1_double.setValue(value);
			return shared_from_this();
		}

		[[nodiscard]]
		double value2_double() const
		{
			return m_value2_double.value();
		}

		std::shared_ptr<Tween> setValue2Double(double value)
		{
			m_value2_double.setValue(value);
			return shared_from_this();
		}

		[[nodiscard]]
		const ColorF& value1_color() const
		{
			return m_value1_color.value();
		}

		std::shared_ptr<Tween> setValue1Color(const ColorF& value)
		{
			m_value1_color.setValue(value);
			return shared_from_this();
		}

		[[nodiscard]]
		const ColorF& value2_color() const
		{
			return m_value2_color.value();
		}

		std::shared_ptr<Tween> setValue2Color(const ColorF& value)
		{
			m_value2_color.setValue(value);
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
		bool restartsOnActive() const
		{
			return m_restartsOnActive.value();
		}

		std::shared_ptr<Tween> setRestartsOnActive(bool restartsOnActive)
		{
			m_restartsOnActive.setValue(restartsOnActive);
			return shared_from_this();
		}
	};
}