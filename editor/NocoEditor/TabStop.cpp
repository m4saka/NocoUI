﻿#include "TabStop.hpp"
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
		if (node->activeInHierarchy() != noco::ActiveYN::Yes)
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
	
	void TabStop::updateInput(const std::shared_ptr<noco::Node>& node)
	{
		// 現在フォーカスされているノードかチェック
		const bool isFocused = (noco::CurrentFrame::GetFocusedNode() == node);
		
		// フォーカスされている状態でTabキーが押されたかチェック
		if (isFocused && KeyTab.down())
		{
			KeyTab.clearInput();
			CurrentFrame::BlockInput();

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
				if (auto tabStop = targetNode->getComponentOrNull<TabStop>())
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
}
