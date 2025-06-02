#pragma once
#include <Siv3D.hpp>
#include <unordered_map>
#include <memory>

namespace noco
{
	class Node;
	class ComponentBase;

	class IdRegistry
	{
	public:
		static void Register(uint64 id, const std::shared_ptr<Node>& ptr);
		static void Register(uint64 id, const std::shared_ptr<ComponentBase>& ptr);
		static void Unregister(uint64 id);

		[[nodiscard]]
		static std::shared_ptr<Node> GetNode(uint64 id);

		[[nodiscard]]
		static std::shared_ptr<ComponentBase> GetComponent(uint64 id);

	private:
		static inline std::unordered_map<uint64, std::weak_ptr<Node>> s_nodes;
		static inline std::unordered_map<uint64, std::weak_ptr<ComponentBase>> s_components;
	};
}