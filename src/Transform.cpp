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
		, m_hitTestAffected{ U"hitTestAffected", true }
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

	const Property<bool>& Transform::hitTestAffected() const
	{
		return m_hitTestAffected;
	}

	Property<bool>& Transform::hitTestAffected()
	{
		return m_hitTestAffected;
	}

	void Transform::setHitTestAffected(const PropertyValue<bool>& value)
	{
		m_hitTestAffected.setPropertyValue(value);
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

	void Transform::update(InteractionState interactionState, const Array<String>& activeStyleStates, double deltaTime, const HashTable<String, ParamValue>& params, SkipsSmoothingYN skipsSmoothing)
	{
		m_translate.update(interactionState, activeStyleStates, deltaTime, params, skipsSmoothing);
		m_scale.update(interactionState, activeStyleStates, deltaTime, params, skipsSmoothing);
		m_pivot.update(interactionState, activeStyleStates, deltaTime, params, skipsSmoothing);
		m_rotation.update(interactionState, activeStyleStates, deltaTime, params, skipsSmoothing);
		m_hitTestAffected.update(interactionState, activeStyleStates, deltaTime, params, skipsSmoothing);
		m_color.update(interactionState, activeStyleStates, deltaTime, params, skipsSmoothing);
	}

	JSON Transform::toJSON() const
	{
		JSON json;
		m_translate.appendJSON(json);
		m_scale.appendJSON(json);
		m_pivot.appendJSON(json);
		m_rotation.appendJSON(json);
		m_hitTestAffected.appendJSON(json);
		m_color.appendJSON(json);
		return json;
	}

	void Transform::readFromJSON(const JSON& json)
	{
		m_translate.readFromJSON(json);
		m_scale.readFromJSON(json);
		m_pivot.readFromJSON(json);
		m_rotation.readFromJSON(json);
		m_hitTestAffected.readFromJSON(json);
		m_color.readFromJSON(json);
	}

	size_t Transform::countParamRefs(StringView paramName) const
	{
		if (paramName.isEmpty())
		{
			return 0;
		}
		
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
		if (m_hitTestAffected.paramRef() == paramName)
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
		if (paramName.isEmpty())
		{
			return;
		}
		
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
		if (m_hitTestAffected.paramRef() == paramName)
		{
			m_hitTestAffected.setParamRef(U"");
		}
		if (m_color.paramRef() == paramName)
		{
			m_color.setParamRef(U"");
		}
	}

	void Transform::replaceParamRefs(StringView oldName, StringView newName)
	{
		if (oldName.isEmpty())
		{
			Logger << U"[NocoUI warning] Transform::replaceParamRefs called with empty oldName";
			return;
		}
		
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
		if (m_hitTestAffected.paramRef() == oldName)
		{
			m_hitTestAffected.setParamRef(String{ newName });
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
		
		// hitTestAffectedの参照をチェック
		if (!m_hitTestAffected.paramRef().isEmpty() && !validParams.contains(m_hitTestAffected.paramRef()))
		{
			clearedParamsSet.insert(m_hitTestAffected.paramRef());
			m_hitTestAffected.setParamRef(U"");
		}
		
		// colorの参照をチェック
		if (!m_color.paramRef().isEmpty() && !validParams.contains(m_color.paramRef()))
		{
			clearedParamsSet.insert(m_color.paramRef());
			m_color.setParamRef(U"");
		}
		
		return Array<String>(clearedParamsSet.begin(), clearedParamsSet.end());
	}

	void Transform::clearCurrentFrameOverride()
	{
		m_translate.clearCurrentFrameOverride();
		m_scale.clearCurrentFrameOverride();
		m_pivot.clearCurrentFrameOverride();
		m_rotation.clearCurrentFrameOverride();
		m_color.clearCurrentFrameOverride();
	}
}