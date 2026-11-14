#include "NocoUI/Component/ComponentBase.hpp"
#include "NocoUI/Node.hpp"

namespace noco
{
	void ComponentBase::setOwnerDirtyFlagToProperties()
	{
		for (IProperty* property : m_properties)
		{
			property->setOwnerDirtyFlag(&m_isPropertiesDirty);
		}
		m_isPropertiesDirty = true;
	}

	void ComponentBase::updateProperties(InteractionState interactionState, const Array<String>& activeStyleStates, double deltaTime, const HashTable<String, ParamValue>& params, SkipSmoothingYN skipSmoothing)
	{
		const bool isSameState = interactionState == m_prevInteractionState
			&& m_prevActiveStyleStates.size() == activeStyleStates.size()
			&& (activeStyleStates.empty() || m_prevActiveStyleStates == activeStyleStates);
		if (!skipSmoothing && !m_isPropertiesDirty && isSameState)
		{
			// 前回の更新から変化がないため、プロパティ更新をスキップ
			return;
		}

		if (!isSameState)
		{
			m_prevInteractionState = interactionState;
			m_prevActiveStyleStates = activeStyleStates;
		}

		// 次回も更新が必要なプロパティは、update内でm_isPropertiesDirtyをtrueに設定する
		m_isPropertiesDirty = false;
		for (IProperty* property : m_properties)
		{
			property->update(interactionState, activeStyleStates, deltaTime, params, skipSmoothing);
		}
	}
}
