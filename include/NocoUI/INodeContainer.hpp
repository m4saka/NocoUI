#pragma once
#include <Siv3D.hpp>
#include <memory>
#include "YN.hpp"
#include "Enums.hpp"
#include "InheritChildrenStateFlags.hpp"
#include "Anchor.hpp"
#include "Region/Region.hpp"
#include "Layout/Layout.hpp"

namespace noco
{
	class Node;
	class ComponentFactory;

	class INodeContainer
	{
	public:
		virtual ~INodeContainer() = default;

		[[nodiscard]]
		virtual const Array<std::shared_ptr<Node>>& children() const = 0;

		[[nodiscard]]
		virtual size_t childCount() const = 0;

		[[nodiscard]]
		virtual std::shared_ptr<Node> childAt(size_t index) const = 0;

		virtual const std::shared_ptr<Node>& addChild(
			const std::shared_ptr<Node>& child) = 0;

		virtual void removeChild(
			const std::shared_ptr<Node>& child) = 0;

		virtual void removeChildrenAll() = 0;

		virtual const std::shared_ptr<Node>& addChildAtIndex(
			const std::shared_ptr<Node>& child,
			size_t index) = 0;

		virtual void swapChildren(
			size_t index1,
			size_t index2) = 0;

		[[nodiscard]]
		virtual bool containsChild(
			const std::shared_ptr<Node>& child,
			RecursiveYN recursive = RecursiveYN::No,
			IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No) const = 0;

		[[nodiscard]]
		virtual std::shared_ptr<Node> findByName(
			StringView name,
			RecursiveYN recursive = RecursiveYN::Yes,
			IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No) = 0;

		[[nodiscard]]
		virtual Optional<size_t> indexOfChildOpt(
			const std::shared_ptr<Node>& child) const = 0;

		virtual const std::shared_ptr<Node>& emplaceChild(
			StringView name = U"Node",
			const RegionVariant& region = InlineRegion{},
			IsHitTargetYN isHitTarget = IsHitTargetYN::Yes,
			InheritChildrenStateFlags inheritChildrenStateFlags = InheritChildrenStateFlags::None) = 0;

		virtual const std::shared_ptr<Node>& addChildFromJSON(
			const JSON& json) = 0;
		
		virtual const std::shared_ptr<Node>& addChildFromJSON(
			const JSON& json, const ComponentFactory& factory) = 0;

		[[nodiscard]]
		virtual const LayoutVariant& childrenLayout() const = 0;

		[[nodiscard]]
		virtual const FlowLayout* childrenFlowLayout() const = 0;

		[[nodiscard]]
		virtual const HorizontalLayout* childrenHorizontalLayout() const = 0;

		[[nodiscard]]
		virtual const VerticalLayout* childrenVerticalLayout() const = 0;
	};
}
