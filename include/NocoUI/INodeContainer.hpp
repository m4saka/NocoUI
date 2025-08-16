#pragma once
#include <Siv3D.hpp>
#include <memory>
#include "YN.hpp"
#include "Enums.hpp"
#include "InheritChildrenStateFlags.hpp"
#include "Anchor.hpp"
#include "Region/Region.hpp"

namespace noco
{
	class Node;

	class INodeContainer
	{
	public:
		virtual ~INodeContainer() = default;

		// 基本アクセス
		[[nodiscard]]
		virtual const Array<std::shared_ptr<Node>>& children() const = 0;

		[[nodiscard]]
		virtual size_t childCount() const = 0;

		[[nodiscard]]
		virtual std::shared_ptr<Node> childAt(size_t index) const = 0;

		// 統一された子ノード操作
		virtual const std::shared_ptr<Node>& addChild(
			const std::shared_ptr<Node>& child,
			RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes) = 0;

		virtual void removeChild(
			const std::shared_ptr<Node>& child,
			RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes) = 0;

		virtual void removeChildrenAll(
			RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes) = 0;

		// インデックス指定操作
		virtual const std::shared_ptr<Node>& addChildAtIndex(
			const std::shared_ptr<Node>& child,
			size_t index,
			RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes) = 0;

		virtual void swapChildren(
			size_t index1,
			size_t index2,
			RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes) = 0;

		// 検索・包含チェック
		[[nodiscard]]
		virtual bool containsChild(
			const std::shared_ptr<Node>& child,
			RecursiveYN recursive = RecursiveYN::No) const = 0;

		[[nodiscard]]
		virtual bool containsChildByName(
			StringView name,
			RecursiveYN recursive = RecursiveYN::No) const = 0;

		[[nodiscard]]
		virtual std::shared_ptr<Node> getChildByNameOrNull(
			StringView name,
			RecursiveYN recursive = RecursiveYN::No) = 0;

		[[nodiscard]]
		virtual Optional<size_t> indexOfChildOpt(
			const std::shared_ptr<Node>& child) const = 0;

		// 子ノード作成
		virtual const std::shared_ptr<Node>& emplaceChild(
			StringView name = U"Node",
			const RegionVariant& region = InlineRegion{},
			IsHitTargetYN isHitTarget = IsHitTargetYN::Yes,
			InheritChildrenStateFlags inheritChildrenStateFlags = InheritChildrenStateFlags::None,
			RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes) = 0;

		// JSON操作
		virtual const std::shared_ptr<Node>& addChildFromJSON(
			const JSON& json,
			RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes) = 0;

		// 型判定
		[[nodiscard]]
		virtual bool isNode() const = 0;

		[[nodiscard]]
		virtual bool isCanvas() const = 0;
	};
}