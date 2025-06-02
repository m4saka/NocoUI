#include <Siv3D.hpp>
#include <NocoUI/IdRegistry.hpp>
#include <NocoUI/Node.hpp>
#include <NocoUI/Component/ComponentBase.hpp>

namespace noco
{
	void IdRegistry::Register(uint64 id, const std::shared_ptr<Node>& ptr)
	{
		if (ptr)
		{
			s_nodes[id] = ptr;
		}
	}

	void IdRegistry::Register(uint64 id, const std::shared_ptr<ComponentBase>& ptr)
	{
		if (ptr)
		{
			s_components[id] = ptr;
		}
	}

	void IdRegistry::Unregister(uint64 id)
	{
		s_nodes.erase(id);
		s_components.erase(id);
	}

	std::shared_ptr<Node> IdRegistry::GetNode(uint64 id)
	{
		auto it = s_nodes.find(id);
		if (it != s_nodes.end())
		{
			return it->second.lock();
		}
		return nullptr;
	}

	std::shared_ptr<ComponentBase> IdRegistry::GetComponent(uint64 id)
	{
		auto it = s_components.find(id);
		if (it != s_components.end())
		{
			return it->second.lock();
		}
		return nullptr;
	}
}