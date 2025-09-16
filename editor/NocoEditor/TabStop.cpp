#include "TabStop.hpp"
#include <NocoUI/Node.hpp>
#include <NocoUI/Component/IFocusable.hpp>
#include <NocoUI/Canvas.hpp>
#include <NocoUI/InteractionState.hpp>
#include <NocoUI/YN.hpp>

namespace noco::editor
{
	// ノードがフォーカス可能かチェックする関数
	static bool isFocusable(const std::shared_ptr<noco::Node>& node)
	{
		if (!node)
		{
			return false;
		}
		
		// 非アクティブなノードはフォーカス不可
		if (!node->activeInHierarchy())
		{
			return false;
		}
		
		// 無効ノードはフォーカス不可
		if (!node->interactableInHierarchy())
		{
			return false;
		}
		
		return true;
	}
	
	void TabStop::updateKeyInput(const std::shared_ptr<noco::Node>& node)
	{
		// 現在フォーカスされているノードかチェック
		const bool isFocused = (noco::CurrentFrame::GetFocusedNode() == node);
		
		// フォーカスされている状態でTabキーが押されたかチェック
		if (isFocused && KeyTab.down())
		{
			KeyTab.clearInput();
			CurrentFrame::BlockKeyInput();

			const bool reverse = KeyShift.pressed();
			
			// 最初のターゲットノードを取得
			std::shared_ptr<noco::Node> targetNode = reverse ? getPreviousNode() : getNextNode();
			std::shared_ptr<noco::Node> startNode = node;
			
			// フォーカス可能なノードが見つかるまで探索
			while (targetNode && targetNode != startNode)
			{
				if (isFocusable(targetNode))
				{
					// フォーカス可能なノードが見つかった
					noco::CurrentFrame::SetFocusedNode(targetNode);
					return;
				}
				
				// 次のノードを探す
				if (auto tabStop = targetNode->getComponent<TabStop>())
				{
					targetNode = reverse ? tabStop->getPreviousNode() : tabStop->getNextNode();
				}
				else
				{
					// TabStopがない場合は探索を中止
					break;
				}
			}
		}
	}

	void TabStop::LinkAllTabStops(const std::shared_ptr<noco::Node>& rootNode, bool circular)
	{
		if (!rootNode)
		{
			return;
		}

		// TabStopコンポーネントを持つすべてのノードを収集
		Array<std::shared_ptr<noco::Node>> tabStopNodes;

		std::function<void(const std::shared_ptr<noco::Node>&)> collectNodes = [&](const std::shared_ptr<noco::Node>& node)
		{
			if (!node)
			{
				return;
			}

			// TabStopを持っているかチェック
			if (node->getComponent<TabStop>())
			{
				// アクティブかつインタラクタブルなノードのみ追加
				if (node->activeInHierarchy() && node->interactableInHierarchy())
				{
					tabStopNodes.push_back(node);
				}
			}

			// 子ノードも再帰的に探索
			for (const auto& child : node->children())
			{
				collectNodes(child);
			}
		};

		collectNodes(rootNode);

		// 収集したノードをリンク
		LinkTabStops(tabStopNodes, circular);
	}

	void TabStop::LinkTabStops(const Array<std::shared_ptr<noco::Node>>& nodes, bool circular)
	{
		if (nodes.size() < 2)
		{
			// ノードが1つ以下の場合はリンク不要
			return;
		}

		// 各ノード間のリンクを設定
		for (size_t i = 0; i < nodes.size(); ++i)
		{
			auto currentTabStop = nodes[i]->getComponent<TabStop>();
			if (!currentTabStop)
			{
				continue;
			}

			// 次のノードを設定
			if (i < nodes.size() - 1)
			{
				currentTabStop->setNextNode(nodes[i + 1]);
			}
			else if (circular)
			{
				// 最後のノードの次は最初のノード（循環）
				currentTabStop->setNextNode(nodes[0]);
			}

			// 前のノードを設定
			if (i > 0)
			{
				currentTabStop->setPreviousNode(nodes[i - 1]);
			}
			else if (circular)
			{
				// 最初のノードの前は最後のノード（循環）
				currentTabStop->setPreviousNode(nodes.back());
			}
		}
	}
}
