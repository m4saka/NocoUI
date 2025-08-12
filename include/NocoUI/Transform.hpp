#pragma once
#include <Siv3D.hpp>
#include "Anchor.hpp"
#include "InteractionState.hpp"
#include "PropertyValue.hpp"
#include "Property.hpp"
#include "YN.hpp"

namespace noco
{
	class Transform
	{
	private:
		SmoothProperty<Vec2> m_translate;
		SmoothProperty<Vec2> m_scale;
		SmoothProperty<Vec2> m_pivot;
		SmoothProperty<double> m_rotation;
		Property<bool> m_appliesToHitTest;
		SmoothProperty<ColorF> m_color;

	public:
		Transform(
			const PropertyValue<Vec2>& translate = Vec2::Zero(),
			const PropertyValue<Vec2>& scale = Vec2::One(),
			const PropertyValue<Vec2>& pivot = Anchor::MiddleCenter,
			const PropertyValue<double>& rotation = 0.0,
			const PropertyValue<ColorF>& color = ColorF{ 1.0 });

		[[nodiscard]]
		const SmoothProperty<Vec2>& translate() const;

		[[nodiscard]]
		SmoothProperty<Vec2>& translate();

		void setTranslate(const PropertyValue<Vec2>& translate);

		[[nodiscard]]
		const SmoothProperty<Vec2>& scale() const;

		[[nodiscard]]
		SmoothProperty<Vec2>& scale();

		void setScale(const PropertyValue<Vec2>& scale);

		[[nodiscard]]
		const SmoothProperty<Vec2>& pivot() const;

		[[nodiscard]]
		SmoothProperty<Vec2>& pivot();

		void setPivot(const PropertyValue<Vec2>& pivot);

		[[nodiscard]]
		const SmoothProperty<double>& rotation() const;

		[[nodiscard]]
		SmoothProperty<double>& rotation();

		void setRotation(const PropertyValue<double>& rotation);

		[[nodiscard]]
		const Property<bool>& appliesToHitTest() const;

		[[nodiscard]]
		Property<bool>& appliesToHitTest();

		void setAppliesToHitTest(const PropertyValue<bool>& value);

		[[nodiscard]]
		const SmoothProperty<ColorF>& color() const;

		[[nodiscard]]
		SmoothProperty<ColorF>& color();

		void setColor(const PropertyValue<ColorF>& color);

		void update(InteractionState interactionState, const Array<String>& activeStyleStates, double deltaTime, const HashTable<String, ParamValue>& params);

		[[nodiscard]]
		JSON toJSON() const;

		void readFromJSON(const JSON& json);

		[[nodiscard]]
		size_t countParamRef(StringView paramName) const;

		void clearParamRef(StringView paramName);

		Array<String> clearInvalidParamRefs(const HashTable<String, ParamValue>& validParams);
	};
}
