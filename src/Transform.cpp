#include "NocoUI/Transform.hpp"

namespace noco
{
	Transform::Transform(
		const PropertyValue<Vec2>& translate,
		const PropertyValue<Vec2>& scale,
		const PropertyValue<Vec2>& pivot,
		const PropertyValue<double>& rotation,
		const PropertyValue<ColorF>& color)
		: m_translate{ U"translate", translate }
		, m_scale{ U"scale", scale }
		, m_pivot{ U"pivot", pivot }
		, m_rotation{ U"rotation", rotation }
		, m_appliesToHitTest{ U"appliesToHitTest", false }
		, m_color{ U"color", color }
	{
	}

	const SmoothProperty<Vec2>& Transform::translate() const
	{
		return m_translate;
	}

	SmoothProperty<Vec2>& Transform::translate()
	{
		return m_translate;
	}

	void Transform::setTranslate(const PropertyValue<Vec2>& translate)
	{
		m_translate.setPropertyValue(translate);
	}

	const SmoothProperty<Vec2>& Transform::scale() const
	{
		return m_scale;
	}

	SmoothProperty<Vec2>& Transform::scale()
	{
		return m_scale;
	}

	void Transform::setScale(const PropertyValue<Vec2>& scale)
	{
		m_scale.setPropertyValue(scale);
	}

	const SmoothProperty<Vec2>& Transform::pivot() const
	{
		return m_pivot;
	}

	SmoothProperty<Vec2>& Transform::pivot()
	{
		return m_pivot;
	}

	void Transform::setPivot(const PropertyValue<Vec2>& pivot)
	{
		m_pivot.setPropertyValue(pivot);
	}

	const SmoothProperty<double>& Transform::rotation() const
	{
		return m_rotation;
	}

	SmoothProperty<double>& Transform::rotation()
	{
		return m_rotation;
	}

	void Transform::setRotation(const PropertyValue<double>& rotation)
	{
		m_rotation.setPropertyValue(rotation);
	}

	const Property<bool>& Transform::appliesToHitTest() const
	{
		return m_appliesToHitTest;
	}

	Property<bool>& Transform::appliesToHitTest()
	{
		return m_appliesToHitTest;
	}

	void Transform::setAppliesToHitTest(const PropertyValue<bool>& value)
	{
		m_appliesToHitTest.setPropertyValue(value);
	}

	const SmoothProperty<ColorF>& Transform::color() const
	{
		return m_color;
	}

	SmoothProperty<ColorF>& Transform::color()
	{
		return m_color;
	}

	void Transform::setColor(const PropertyValue<ColorF>& color)
	{
		m_color.setPropertyValue(color);
	}

	void Transform::update(InteractionState interactionState, const Array<String>& activeStyleStates, double deltaTime)
	{
		m_translate.update(interactionState, activeStyleStates, deltaTime);
		m_scale.update(interactionState, activeStyleStates, deltaTime);
		m_pivot.update(interactionState, activeStyleStates, deltaTime);
		m_rotation.update(interactionState, activeStyleStates, deltaTime);
		m_appliesToHitTest.update(interactionState, activeStyleStates, deltaTime);
		m_color.update(interactionState, activeStyleStates, deltaTime);
	}

	JSON Transform::toJSON() const
	{
		JSON json;
		m_translate.appendJSON(json);
		m_scale.appendJSON(json);
		m_pivot.appendJSON(json);
		m_rotation.appendJSON(json);
		m_appliesToHitTest.appendJSON(json);
		m_color.appendJSON(json);
		return json;
	}

	void Transform::readFromJSON(const JSON& json)
	{
		m_translate.readFromJSON(json);
		m_scale.readFromJSON(json);
		m_pivot.readFromJSON(json);
		m_rotation.readFromJSON(json);
		m_appliesToHitTest.readFromJSON(json);
		m_color.readFromJSON(json);
	}
}