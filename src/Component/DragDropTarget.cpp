#include "NocoUI/Component/DragDropTarget.hpp"
#include "NocoUI/Component/DragDropSource.hpp"

namespace noco
{
	bool DragDropTarget::isDroppableNodeList(const Array<std::shared_ptr<Node>>& nodes) const
	{
		if (m_isDroppableNodeList)
		{
			return m_isDroppableNodeList(nodes);
		}
		return true;
	}
	
	void DragDropTarget::lateUpdate(const std::shared_ptr<Node>& targetNode)
	{
		const auto draggingNode = detail::s_canvasUpdateContext.draggingNode.lock();
		if (!draggingNode)
		{
			m_dropFocused = false;
			return;
		}
		const auto dragDropSource = draggingNode->getComponentOrNull<DragDropSource>();
		if (!dragDropSource)
		{
			m_dropFocused = false;
			return;
		}
		const bool isDraggingOtherNode = draggingNode && draggingNode != targetNode;
		const bool isHovered = detail::s_canvasUpdateContext.hoveredNode.lock() == targetNode;
		m_dropFocused = isDraggingOtherNode && isHovered && isDroppableNodeList(dragDropSource->draggingNodeList());
		if (m_dropFocused)
		{
			Cursor::RequestStyle(CursorStyle::Hand);
		}
	}
	
	void DragDropTarget::draw(const Node& node) const
	{
		if (m_dropFocused)
		{
			if (m_dragFocusedDrawer)
			{
				m_dragFocusedDrawer(node);
			}
			else
			{
				node.rect().draw(ColorF{ Palette::White, 0.3 });
			}
		}
	}
	
	void DragDropTarget::onDrop(const Array<std::shared_ptr<Node>>& draggingNodeList)
	{
		if (!isDroppableNodeList(draggingNodeList))
		{
			return;
		}
		if (m_onDrop)
		{
			m_onDrop(draggingNodeList);
		}
	}
}
