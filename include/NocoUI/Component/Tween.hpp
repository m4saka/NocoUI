#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"

namespace noco
{
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

		// Translate用プロパティ
		PropertyNonInteractive<bool> m_translateEnabled;
		PropertyNonInteractive<Vec2> m_translateFrom;
		PropertyNonInteractive<Vec2> m_translateTo;

		// Scale用プロパティ
		PropertyNonInteractive<bool> m_scaleEnabled;
		PropertyNonInteractive<Vec2> m_scaleFrom;
		PropertyNonInteractive<Vec2> m_scaleTo;

		// Rotation用プロパティ
		PropertyNonInteractive<bool> m_rotationEnabled;
		PropertyNonInteractive<double> m_rotationFrom;
		PropertyNonInteractive<double> m_rotationTo;

		// Color用プロパティ
		PropertyNonInteractive<bool> m_colorEnabled;
		PropertyNonInteractive<Color> m_colorFrom;
		PropertyNonInteractive<Color> m_colorTo;
		
		PropertyNonInteractive<TweenEasing> m_easing;
		PropertyNonInteractive<double> m_duration;
		PropertyNonInteractive<double> m_delay;
		PropertyNonInteractive<TweenLoopType> m_loopType;
		PropertyNonInteractive<double> m_loopDuration;
		PropertyNonInteractive<bool> m_restartOnActive;
		PropertyNonInteractive<bool> m_applyDuringDelay;
		Property<bool> m_manualMode;
		SmoothProperty<double> m_manualTime;
		PropertyNonInteractive<String> m_tag;

		/* NonSerialized */ Stopwatch m_stopwatch;
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
			TweenEasing easing = TweenEasing::EaseOutQuad,
			double duration = 1.0)
			: SerializableComponentBase{ U"Tween", {
				&m_active,
				&m_translateEnabled, &m_translateFrom, &m_translateTo,
				&m_scaleEnabled, &m_scaleFrom, &m_scaleTo,
				&m_rotationEnabled, &m_rotationFrom, &m_rotationTo,
				&m_colorEnabled, &m_colorFrom, &m_colorTo,
				&m_easing, &m_duration, &m_delay, &m_loopType, &m_loopDuration, &m_restartOnActive,
				&m_applyDuringDelay, &m_manualMode, &m_manualTime, &m_tag
			} }
			, m_active{ U"active", active }
			, m_translateEnabled{ U"translateEnabled", false }
			, m_translateFrom{ U"translateFrom", Vec2{ 0.0, 0.0 } }
			, m_translateTo{ U"translateTo", Vec2{ 0.0, 0.0 } }
			, m_scaleEnabled{ U"scaleEnabled", false }
			, m_scaleFrom{ U"scaleFrom", Vec2{ 1.0, 1.0 } }
			, m_scaleTo{ U"scaleTo", Vec2{ 1.0, 1.0 } }
			, m_rotationEnabled{ U"rotationEnabled", false }
			, m_rotationFrom{ U"rotationFrom", 0.0 }
			, m_rotationTo{ U"rotationTo", 0.0 }
			, m_colorEnabled{ U"colorEnabled", false }
			, m_colorFrom{ U"colorFrom", Color{ 255, 255, 255, 255 } }
			, m_colorTo{ U"colorTo", Color{ 255, 255, 255, 255 } }
			, m_easing{ U"easing", easing }
			, m_duration{ U"duration", duration }
			, m_delay{ U"delay", 0.0 }
			, m_loopType{ U"loopType", TweenLoopType::None }
			, m_loopDuration{ U"loopDuration", 0.0 }
			, m_restartOnActive{ U"restartOnActive", true }
			, m_applyDuringDelay{ U"applyDuringDelay", false }
			, m_manualMode{ U"manualMode", false }
			, m_manualTime{ U"manualTime", 0.0 }
			, m_tag{ U"tag", U"" }
		{
		}

		void onActivated(const std::shared_ptr<Node>& node) override;
		void update(const std::shared_ptr<Node>& node) override;

		[[nodiscard]]
		const PropertyValue<bool>& active() const
		{
			return m_active.propertyValue();
		}

		std::shared_ptr<Tween> setActive(const PropertyValue<bool>& active);

		// Translate関連のgetter/setter
		[[nodiscard]]
		bool translateEnabled() const
		{
			return m_translateEnabled.value();
		}

		std::shared_ptr<Tween> setTranslateEnabled(bool enabled)
		{
			m_translateEnabled.setValue(enabled);
			return shared_from_this();
		}

		[[nodiscard]]
		const Vec2& translateFrom() const
		{
			return m_translateFrom.value();
		}

		std::shared_ptr<Tween> setTranslateFrom(const Vec2& value)
		{
			m_translateFrom.setValue(value);
			return shared_from_this();
		}

		[[nodiscard]]
		const Vec2& translateTo() const
		{
			return m_translateTo.value();
		}

		std::shared_ptr<Tween> setTranslateTo(const Vec2& value)
		{
			m_translateTo.setValue(value);
			return shared_from_this();
		}

		// Scale関連のgetter/setter
		[[nodiscard]]
		bool scaleEnabled() const
		{
			return m_scaleEnabled.value();
		}

		std::shared_ptr<Tween> setScaleEnabled(bool enabled)
		{
			m_scaleEnabled.setValue(enabled);
			return shared_from_this();
		}

		[[nodiscard]]
		const Vec2& scaleFrom() const
		{
			return m_scaleFrom.value();
		}

		std::shared_ptr<Tween> setScaleFrom(const Vec2& value)
		{
			m_scaleFrom.setValue(value);
			return shared_from_this();
		}

		[[nodiscard]]
		const Vec2& scaleTo() const
		{
			return m_scaleTo.value();
		}

		std::shared_ptr<Tween> setScaleTo(const Vec2& value)
		{
			m_scaleTo.setValue(value);
			return shared_from_this();
		}

		// Rotation関連のgetter/setter
		[[nodiscard]]
		bool rotationEnabled() const
		{
			return m_rotationEnabled.value();
		}

		std::shared_ptr<Tween> setRotationEnabled(bool enabled)
		{
			m_rotationEnabled.setValue(enabled);
			return shared_from_this();
		}

		[[nodiscard]]
		double rotationFrom() const
		{
			return m_rotationFrom.value();
		}

		std::shared_ptr<Tween> setRotationFrom(double value)
		{
			m_rotationFrom.setValue(value);
			return shared_from_this();
		}

		[[nodiscard]]
		double rotationTo() const
		{
			return m_rotationTo.value();
		}

		std::shared_ptr<Tween> setRotationTo(double value)
		{
			m_rotationTo.setValue(value);
			return shared_from_this();
		}

		// Color関連のgetter/setter
		[[nodiscard]]
		bool colorEnabled() const
		{
			return m_colorEnabled.value();
		}

		std::shared_ptr<Tween> setColorEnabled(bool enabled)
		{
			m_colorEnabled.setValue(enabled);
			return shared_from_this();
		}

		[[nodiscard]]
		const Color& colorFrom() const
		{
			return m_colorFrom.value();
		}

		std::shared_ptr<Tween> setColorFrom(const Color& value)
		{
			m_colorFrom.setValue(value);
			return shared_from_this();
		}

		[[nodiscard]]
		const Color& colorTo() const
		{
			return m_colorTo.value();
		}

		std::shared_ptr<Tween> setColorTo(const Color& value)
		{
			m_colorTo.setValue(value);
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
		double loopDuration() const
		{
			return m_loopDuration.value();
		}

		std::shared_ptr<Tween> setLoopDuration(double loopDuration)
		{
			m_loopDuration.setValue(loopDuration);
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
		const PropertyValue<bool>& manualMode() const
		{
			return m_manualMode.propertyValue();
		}

		std::shared_ptr<Tween> setManualMode(const PropertyValue<bool>& manualMode)
		{
			m_manualMode.setPropertyValue(manualMode);
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

		[[nodiscard]]
		const String& tag() const
		{
			return m_tag.value();
		}

		std::shared_ptr<Tween> setTag(const String& tag)
		{
			m_tag.setValue(tag);
			return shared_from_this();
		}
	};
}
