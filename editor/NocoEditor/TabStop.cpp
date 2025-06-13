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
				// 逆方向の探索（前のTabStopを探す、循環探索有効）
				nextNode = node->findPrevious([](const noco::Node& n) {
					return n.getComponentOrNull<TabStop>() != nullptr;
				}, noco::SkipsSelfYN::Yes, noco::IsCyclicYN::Yes);
			}
			else
			{
				// 順方向の探索（次のTabStopを探す、循環探索有効）
				nextNode = node->findNext([](const noco::Node& n) {
					return n.getComponentOrNull<TabStop>() != nullptr;
				}, noco::SkipsSelfYN::Yes, noco::IsCyclicYN::Yes);
			}
			
			if (nextNode && nextNode != node)
			{
				// 次のノードにフォーカスを設定（SetFocusedNodeが自動的にblur/focusを呼ぶ）
				noco::CurrentFrame::SetFocusedNode(nextNode);
			}
		}
		
	}
}