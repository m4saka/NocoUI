#pragma once
#include <Siv3D.hpp>
#include "Anchor.hpp"
#include "InteractionState.hpp"
#include "PropertyValue.hpp"
#include "Property.hpp"
#include "YN.hpp"

namespace noco
{
	class TransformEffect
	{
	private:
		SmoothProperty<Vec2> m_position;
		SmoothProperty<Vec2> m_scale;
		SmoothProperty<Vec2> m_pivot;
		SmoothProperty<double> m_rotation;
		Property<bool> m_appliesToHitTest;
		SmoothProperty<ColorF> m_color;

	public:
		TransformEffect(
			const PropertyValue<Vec2>& position = Vec2::Zero(),
			const PropertyValue<Vec2>& scale = Vec2::One(),
			const PropertyValue<Vec2>& pivot = Anchor::MiddleCenter,
			const PropertyValue<double>& rotation = 0.0,
			const PropertyValue<ColorF>& color = ColorF{ 1.0 })
			: m_position{ U"position", position }
			, m_scale{ U"scale", scale }
			, m_pivot{ U"pivot", pivot }
			, m_rotation{ U"rotation", rotation }
			, m_appliesToHitTest{ U"appliesToHitTest", false }
			, m_color{ U"color", color }
		{
		}

		[[nodiscard]]
		const SmoothProperty<Vec2>& position() const
		{
			return m_position;
		}

		[[nodiscard]]
		SmoothProperty<Vec2>& position()
		{
			return m_position;
		}

		void setPosition(const PropertyValue<Vec2>& position)
		{
			m_position.setPropertyValue(position);
		}

		[[nodiscard]]
		const SmoothProperty<Vec2>& scale() const
		{
			return m_scale;
		}

		[[nodiscard]]
		SmoothProperty<Vec2>& scale()
		{
			return m_scale;
		}

		void setScale(const PropertyValue<Vec2>& scale)
		{
			m_scale.setPropertyValue(scale);
		}

		[[nodiscard]]
		const SmoothProperty<Vec2>& pivot() const
		{
			return m_pivot;
		}

		[[nodiscard]]
		SmoothProperty<Vec2>& pivot()
		{
			return m_pivot;
		}

		void setPivot(const PropertyValue<Vec2>& pivot)
		{
			m_pivot.setPropertyValue(pivot);
		}

		[[nodiscard]]
		const SmoothProperty<double>& rotation() const
		{
			return m_rotation;
		}

		[[nodiscard]]
		SmoothProperty<double>& rotation()
		{
			return m_rotation;
		}

		void setRotation(const PropertyValue<double>& rotation)
		{
			m_rotation.setPropertyValue(rotation);
		}

		[[nodiscard]]
		const Property<bool>& appliesToHitTest() const
		{
			return m_appliesToHitTest;
		}

		[[nodiscard]]
		Property<bool>& appliesToHitTest()
		{
			return m_appliesToHitTest;
		}

		void setAppliesToHitTest(const PropertyValue<bool>& value)
		{
			m_appliesToHitTest.setPropertyValue(value);
		}

		[[nodiscard]]
		const SmoothProperty<ColorF>& color() const
		{
			return m_color;
		}

		[[nodiscard]]
		SmoothProperty<ColorF>& color()
		{
			return m_color;
		}

		void setColor(const PropertyValue<ColorF>& color)
		{
			m_color.setPropertyValue(color);
		}

		void update(InteractionState interactionState, const Array<String>& activeStyleStates, double deltaTime)
		{
			m_position.update(interactionState, activeStyleStates, deltaTime);
			m_scale.update(interactionState, activeStyleStates, deltaTime);
			m_pivot.update(interactionState, activeStyleStates, deltaTime);
			m_rotation.update(interactionState, activeStyleStates, deltaTime);
			m_appliesToHitTest.update(interactionState, activeStyleStates, deltaTime);
			m_color.update(interactionState, activeStyleStates, deltaTime);
		}

		[[nodiscard]]
		Mat3x2 posScaleMat(const Mat3x2& parentMat, const RectF& rect) const
		{
			const Vec2& position = m_position.value();
			const Vec2& scale = m_scale.value();
			const Vec2& pivot = m_pivot.value();
			const Vec2 pivotPos = rect.pos + rect.size * pivot;
			return Mat3x2::Scale(scale, pivotPos).translated(position) * parentMat;
		}

		[[nodiscard]]
		double rotationInHierarchy(double parentRotation) const
		{
			return parentRotation + m_rotation.value();
		}

		[[nodiscard]]
		JSON toJSON() const
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

		void readFromJSON(const JSON& json)
		{
			m_position.readFromJSON(json);
			m_scale.readFromJSON(json);
			m_pivot.readFromJSON(json);
			m_rotation.readFromJSON(json);
			m_appliesToHitTest.readFromJSON(json);
			m_color.readFromJSON(json);
		}
	};
}
