#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"
#include "../Node.hpp"

namespace noco
{
	class DragDropTarget : public ComponentBase
	{
	private:
		std::function<void(const Array<std::shared_ptr<Node>>&)> m_onDrop;
		std::function<bool(const Array<std::shared_ptr<Node>>&)> m_isDroppableNodeList;
		std::function<void(const Node&)> m_dragFocusedDrawer;
		bool m_dropFocused = false;

		[[nodiscard]]
		bool isDroppableNodeList(const Array<std::shared_ptr<Node>>& nodes) const;

	public:
		explicit DragDropTarget(
			std::function<void(const Array<std::shared_ptr<Node>>&)> onDrop,
			std::function<bool(const Array<std::shared_ptr<Node>>&)> isDroppableNodeList = nullptr,
			std::function<void(const Node&)> dragFocusedDrawer = nullptr)
			: ComponentBase{ {} }
			, m_onDrop{ std::move(onDrop) }
			, m_isDroppableNodeList{ std::move(isDroppableNodeList) }
			, m_dragFocusedDrawer{ std::move(dragFocusedDrawer) }
		{
		}

		void lateUpdate(const std::shared_ptr<Node>& targetNode) override;

		void draw(const Node& node) const override;

		[[nodiscard]]
		bool dropFocused() const
		{
			return m_dropFocused;
		}

		void onDrop(const Array<std::shared_ptr<Node>>& draggingNodeList);
	};
}
