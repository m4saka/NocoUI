#pragma once
#include <Siv3D.hpp>
#include "Anchor.hpp"
#include "InteractionState.hpp"
#include "PropertyValue.hpp"
#include "Property.hpp"

namespace noco
{
	class TransformEffect
	{
	private:
		SmoothProperty<Vec2> m_position;
		SmoothProperty<Vec2> m_scale;
		SmoothProperty<Vec2> m_pivot;
		Property<bool> m_appliesToHitTest;

	public:
		TransformEffect(
			const PropertyValue<Vec2>& position = Vec2::Zero(),
			const PropertyValue<Vec2>& scale = Vec2::One(),
			const PropertyValue<Vec2>& pivot = Anchor::MiddleCenter)
			: m_position{ U"position", position }
			, m_scale{ U"scale", scale }
			, m_pivot{ U"pivot", pivot }
			, m_appliesToHitTest{ U"appliesToHitTest", false }
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

		void update(InteractionState interactionState, SelectedYN selected, double deltaTime)
		{
			m_position.update(interactionState, selected, deltaTime);
			m_scale.update(interactionState, selected, deltaTime);
			m_pivot.update(interactionState, selected, deltaTime);
			m_appliesToHitTest.update(interactionState, selected, deltaTime);
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
		JSON toJSON() const
		{
			JSON json;
			m_position.appendJSON(json);
			m_scale.appendJSON(json);
			m_pivot.appendJSON(json);
			m_appliesToHitTest.appendJSON(json);
			return json;
		}

		void readFromJSON(const JSON& json)
		{
			m_position.readFromJSON(json);
			m_scale.readFromJSON(json);
			m_pivot.readFromJSON(json);
			m_appliesToHitTest.readFromJSON(json);
		}
	};
}
