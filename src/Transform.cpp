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

	void Transform::update(InteractionState interactionState, const Array<String>& activeStyleStates, double deltaTime, const HashTable<String, ParamValue>& params)
	{
		m_translate.update(interactionState, activeStyleStates, deltaTime, params);
		m_scale.update(interactionState, activeStyleStates, deltaTime, params);
		m_pivot.update(interactionState, activeStyleStates, deltaTime, params);
		m_rotation.update(interactionState, activeStyleStates, deltaTime, params);
		m_appliesToHitTest.update(interactionState, activeStyleStates, deltaTime, params);
		m_color.update(interactionState, activeStyleStates, deltaTime, params);
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

	size_t Transform::countParamRefs(StringView paramName) const
	{
		size_t count = 0;
		
		if (m_translate.paramRef() == paramName)
		{
			count++;
		}
		if (m_scale.paramRef() == paramName)
		{
			count++;
		}
		if (m_pivot.paramRef() == paramName)
		{
			count++;
		}
		if (m_rotation.paramRef() == paramName)
		{
			count++;
		}
		if (m_appliesToHitTest.paramRef() == paramName)
		{
			count++;
		}
		if (m_color.paramRef() == paramName)
		{
			count++;
		}
		
		return count;
	}


	void Transform::clearParamRefs(StringView paramName)
	{
		if (m_translate.paramRef() == paramName)
		{
			m_translate.setParamRef(U"");
		}
		if (m_scale.paramRef() == paramName)
		{
			m_scale.setParamRef(U"");
		}
		if (m_pivot.paramRef() == paramName)
		{
			m_pivot.setParamRef(U"");
		}
		if (m_rotation.paramRef() == paramName)
		{
			m_rotation.setParamRef(U"");
		}
		if (m_appliesToHitTest.paramRef() == paramName)
		{
			m_appliesToHitTest.setParamRef(U"");
		}
		if (m_color.paramRef() == paramName)
		{
			m_color.setParamRef(U"");
		}
	}

	void Transform::replaceParamRefs(StringView oldName, StringView newName)
	{
		if (m_translate.paramRef() == oldName)
		{
			m_translate.setParamRef(String{ newName });
		}
		if (m_scale.paramRef() == oldName)
		{
			m_scale.setParamRef(String{ newName });
		}
		if (m_pivot.paramRef() == oldName)
		{
			m_pivot.setParamRef(String{ newName });
		}
		if (m_rotation.paramRef() == oldName)
		{
			m_rotation.setParamRef(String{ newName });
		}
		if (m_appliesToHitTest.paramRef() == oldName)
		{
			m_appliesToHitTest.setParamRef(String{ newName });
		}
		if (m_color.paramRef() == oldName)
		{
			m_color.setParamRef(String{ newName });
		}
	}

	Array<String> Transform::removeInvalidParamRefs(const HashTable<String, ParamValue>& validParams)
	{
		HashSet<String> clearedParamsSet;
		
		// translateの参照をチェック
		if (!m_translate.paramRef().isEmpty() && !validParams.contains(m_translate.paramRef()))
		{
			clearedParamsSet.insert(m_translate.paramRef());
			m_translate.setParamRef(U"");
		}
		
		// scaleの参照をチェック
		if (!m_scale.paramRef().isEmpty() && !validParams.contains(m_scale.paramRef()))
		{
			clearedParamsSet.insert(m_scale.paramRef());
			m_scale.setParamRef(U"");
		}
		
		// pivotの参照をチェック
		if (!m_pivot.paramRef().isEmpty() && !validParams.contains(m_pivot.paramRef()))
		{
			clearedParamsSet.insert(m_pivot.paramRef());
			m_pivot.setParamRef(U"");
		}
		
		// rotationの参照をチェック
		if (!m_rotation.paramRef().isEmpty() && !validParams.contains(m_rotation.paramRef()))
		{
			clearedParamsSet.insert(m_rotation.paramRef());
			m_rotation.setParamRef(U"");
		}
		
		// appliesToHitTestの参照をチェック
		if (!m_appliesToHitTest.paramRef().isEmpty() && !validParams.contains(m_appliesToHitTest.paramRef()))
		{
			clearedParamsSet.insert(m_appliesToHitTest.paramRef());
			m_appliesToHitTest.setParamRef(U"");
		}
		
		// colorの参照をチェック
		if (!m_color.paramRef().isEmpty() && !validParams.contains(m_color.paramRef()))
		{
			clearedParamsSet.insert(m_color.paramRef());
			m_color.setParamRef(U"");
		}
		
		return Array<String>(clearedParamsSet.begin(), clearedParamsSet.end());
	}
}