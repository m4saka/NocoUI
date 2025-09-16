#pragma once
#include <Siv3D.hpp>
#include <NocoUI/Component/ComponentBase.hpp>

namespace noco::editor
{
	class TabStop : public noco::ComponentBase
	{
	private:
		std::weak_ptr<noco::Node> m_nextNode;
		std::weak_ptr<noco::Node> m_previousNode;
		
	public:
		TabStop()
			: ComponentBase{ {} }
		{
		}
		
		// 次のノードを設定
		void setNextNode(const std::shared_ptr<noco::Node>& node)
		{
			m_nextNode = node;
		}
		
		// 前のノードを設定
		void setPreviousNode(const std::shared_ptr<noco::Node>& node)
		{
			m_previousNode = node;
		}
		
		// 次のノードを取得
		std::shared_ptr<noco::Node> getNextNode() const
		{
			return m_nextNode.lock();
		}
		
		// 前のノードを取得
		std::shared_ptr<noco::Node> getPreviousNode() const
		{
			return m_previousNode.lock();
		}

		void updateKeyInput(const std::shared_ptr<noco::Node>& node) override;

		// TabStop間のリンクを設定するユーティリティ関数
		// rootNode内のすべてのTabStopコンポーネントを持つノードを探して、出現順にリンクを設定
		static void LinkAllTabStops(const std::shared_ptr<noco::Node>& rootNode, bool circular = true);

		// 指定したノードの配列をTabStopでリンクする
		static void LinkTabStops(const Array<std::shared_ptr<noco::Node>>& nodes, bool circular = true);
	};
}
