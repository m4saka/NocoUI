#include "NocoUI/Component/ComponentBase.hpp"
#include "NocoUI/Node.hpp"

namespace noco
{
	void ComponentBase::updateProperties(InteractionState interactionState, const Array<String>& activeStyleStates, double deltaTime, const HashTable<String, Param>& params)
	{
		for (IProperty* property : m_properties)
		{
			property->update(interactionState, activeStyleStates, deltaTime, params);
		}
	}
}