#pragma once
#include <Siv3D.hpp>
#include <NocoUI/Component/ComponentBase.hpp>

namespace nocoeditor
{
	// タブキーでのフォーカス移動を可能にするコンポーネント（NocoEditor専用、非シリアライズ）
	class TabStop : public noco::ComponentBase
	{
	private:
		// 同じルートノード内のTabStopを持つノードを収集（逆方向探索用）
		Array<std::weak_ptr<noco::Node>> collectTabStopNodes(const std::shared_ptr<noco::Node>& node) const;
		
		// 前のフォーカス可能なノードを探す
		std::shared_ptr<noco::Node> findPreviousTabStopNode(const Array<std::weak_ptr<noco::Node>>& nodes, const std::shared_ptr<noco::Node>& current) const;
		
		// ルートノードを取得
		std::shared_ptr<noco::Node> getRootNode(const std::shared_ptr<noco::Node>& node) const;

	public:
		TabStop()
			: ComponentBase{ U"TabStop" }
		{
		}

		void update(const std::shared_ptr<noco::Node>& node) override;
	};
}