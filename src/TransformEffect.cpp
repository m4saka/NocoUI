#include "NocoUI/TransformEffect.hpp"

namespace noco
{
	TransformEffect::TransformEffect(
		const PropertyValue<Vec2>& position,
		const PropertyValue<Vec2>& scale,
		const PropertyValue<Vec2>& pivot,
		const PropertyValue<double>& rotation,
		const PropertyValue<ColorF>& color)
		: m_position{ U"position", position }
		, m_scale{ U"scale", scale }
		, m_pivot{ U"pivot", pivot }
		, m_rotation{ U"rotation", rotation }
		, m_appliesToHitTest{ U"appliesToHitTest", false }
		, m_color{ U"color", color }
	{
	}

	const SmoothProperty<Vec2>& TransformEffect::position() const
	{
		return m_position;
	}

	SmoothProperty<Vec2>& TransformEffect::position()
	{
		return m_position;
	}

	void TransformEffect::setPosition(const PropertyValue<Vec2>& position)
	{
		m_position.setPropertyValue(position);
	}

	const SmoothProperty<Vec2>& TransformEffect::scale() const
	{
		return m_scale;
	}

	SmoothProperty<Vec2>& TransformEffect::scale()
	{
		return m_scale;
	}

	void TransformEffect::setScale(const PropertyValue<Vec2>& scale)
	{
		m_scale.setPropertyValue(scale);
	}

	const SmoothProperty<Vec2>& TransformEffect::pivot() const
	{
		return m_pivot;
	}

	SmoothProperty<Vec2>& TransformEffect::pivot()
	{
		return m_pivot;
	}

	void TransformEffect::setPivot(const PropertyValue<Vec2>& pivot)
	{
		m_pivot.setPropertyValue(pivot);
	}

	const SmoothProperty<double>& TransformEffect::rotation() const
	{
		return m_rotation;
	}

	SmoothProperty<double>& TransformEffect::rotation()
	{
		return m_rotation;
	}

	void TransformEffect::setRotation(const PropertyValue<double>& rotation)
	{
		m_rotation.setPropertyValue(rotation);
	}

	const Property<bool>& TransformEffect::appliesToHitTest() const
	{
		return m_appliesToHitTest;
	}

	Property<bool>& TransformEffect::appliesToHitTest()
	{
		return m_appliesToHitTest;
	}

	void TransformEffect::setAppliesToHitTest(const PropertyValue<bool>& value)
	{
		m_appliesToHitTest.setPropertyValue(value);
	}

	const SmoothProperty<ColorF>& TransformEffect::color() const
	{
		return m_color;
	}

	SmoothProperty<ColorF>& TransformEffect::color()
	{
		return m_color;
	}

	void TransformEffect::setColor(const PropertyValue<ColorF>& color)
	{
		m_color.setPropertyValue(color);
	}

	void TransformEffect::update(InteractionState interactionState, const Array<String>& activeStyleStates, double deltaTime)
	{
		m_position.update(interactionState, activeStyleStates, deltaTime);
		m_scale.update(interactionState, activeStyleStates, deltaTime);
		m_pivot.update(interactionState, activeStyleStates, deltaTime);
		m_rotation.update(interactionState, activeStyleStates, deltaTime);
		m_appliesToHitTest.update(interactionState, activeStyleStates, deltaTime);
		m_color.update(interactionState, activeStyleStates, deltaTime);
	}

	Mat3x2 TransformEffect::posScaleMat(const Mat3x2& parentPosScaleMat, const RectF& rect, double parentRotation) const
	{

		const Vec2& scale = m_scale.value();
		const Vec2& pivot = m_pivot.value();

		const Vec2 pivotPos = rect.pos + rect.size * pivot;
		const Mat3x2 localScaleMat = Mat3x2::Scale(scale, pivotPos);
		const Mat3x2 baseMat = localScaleMat * parentPosScaleMat;

		// positionに親のscaleとrotationを適用
		const Mat3x2 parentLinearMat
		{
			parentPosScaleMat._11, parentPosScaleMat._12,
			parentPosScaleMat._21, parentPosScaleMat._22,
			0.0, 0.0
		};
		const Mat3x2 parentLinearMatWithRotation = parentLinearMat * Mat3x2::Rotate(Math::ToRadians(parentRotation));
		const Vec2& position = m_position.value();
		const Vec2 transformedPosition = parentLinearMatWithRotation.transformPoint(position);

		return baseMat.translated(transformedPosition);
	}

	double TransformEffect::rotationInHierarchy(double parentRotation) const
	{
		return parentRotation + m_rotation.value();
	}

	JSON TransformEffect::toJSON() const
	{
		JSON json;
		m_position.appendJSON(json);
		m_scale.appendJSON(json);
		m_pivot.appendJSON(json);
		m_rotation.appendJSON(json);
		m_appliesToHitTest.appendJSON(json);
		m_color.appendJSON(json);
		return json;
	}

	void TransformEffect::readFromJSON(const JSON& json)
	{
		m_position.readFromJSON(json);
		m_scale.readFromJSON(json);
		m_pivot.readFromJSON(json);
		m_rotation.readFromJSON(json);
		m_appliesToHitTest.readFromJSON(json);
		m_color.readFromJSON(json);
	}
}