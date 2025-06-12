#include "TabStop.hpp"
#include <NocoUI/Node.hpp>
#include <NocoUI/Component/IFocusable.hpp>
#include <NocoUI/Canvas.hpp>

namespace nocoeditor
{
	void TabStop::update(const std::shared_ptr<noco::Node>& node)
	{
		// 現在フォーカスされているノードかチェック
		const bool isFocused = (noco::CurrentFrame::GetFocusedNode() == node);
		
		// フォーカスされている状態でTabキーが押されたかチェック
		if (isFocused && KeyTab.down())
		{
			const bool reverse = KeyShift.pressed();
			
			std::shared_ptr<noco::Node> nextNode;
			
			if (reverse)
			{
				// 逆方向の探索（前のTabStopを探す）
				// TODO: findPrevious実装後に最適化
				auto rootNode = getRootNode(node);
				if (rootNode)
				{
					auto tabStopNodes = collectTabStopNodes(rootNode);
					nextNode = findPreviousTabStopNode(tabStopNodes, node);
				}
			}
			else
			{
				// 順方向の探索（次のTabStopを探す、循環探索有効）
				nextNode = node->findNext([](const std::shared_ptr<noco::Node>& n) {
					return n->getComponentOrNull<TabStop>() != nullptr;
				}, noco::SkipsSelfYN::Yes, noco::IsCyclicYN::Yes);
			}
			
			if (nextNode && nextNode != node)
			{
				// 次のノードにフォーカスを設定（SetFocusedNodeが自動的にblur/focusを呼ぶ）
				noco::CurrentFrame::SetFocusedNode(nextNode);
			}
		}
		
	}

	Array<std::weak_ptr<noco::Node>> TabStop::collectTabStopNodes(const std::shared_ptr<noco::Node>& node) const
	{
		return node->findAll([](const std::shared_ptr<noco::Node>& n) {
			return n->getComponentOrNull<TabStop>() != nullptr;
		});
	}


	std::shared_ptr<noco::Node> TabStop::findPreviousTabStopNode(const Array<std::weak_ptr<noco::Node>>& nodes, const std::shared_ptr<noco::Node>& current) const
	{
		if (nodes.empty())
		{
			return nullptr;
		}
		
		// 現在のノードのインデックスを見つける
		Optional<size_t> currentIndex;
		for (size_t i = 0; i < nodes.size(); ++i)
		{
			if (auto node = nodes[i].lock())
			{
				if (node == current)
				{
					currentIndex = i;
					break;
				}
			}
		}
		
		if (!currentIndex.has_value())
		{
			// 現在のノードが見つからない場合は最初のノードを返す
			for (const auto& weakNode : nodes)
			{
				if (auto node = weakNode.lock())
				{
					return node;
				}
			}
			return nullptr;
		}
		
		// 前のインデックスを計算
		size_t prevIndex;
		if (*currentIndex == 0)
		{
			prevIndex = nodes.size() - 1;
		}
		else
		{
			prevIndex = *currentIndex - 1;
		}
		
		// 前のノードを返す
		if (auto node = nodes[prevIndex].lock())
		{
			return node;
		}
		
		return nullptr;
	}

	std::shared_ptr<noco::Node> TabStop::getRootNode(const std::shared_ptr<noco::Node>& node) const
	{
		auto current = node;
		while (current)
		{
			auto parent = current->parent().lock();
			if (!parent)
			{
				return current;
			}
			current = parent;
		}
		return nullptr;
	}
}