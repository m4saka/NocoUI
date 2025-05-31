#include "NocoUI/Component/DragDropSource.hpp"
#include "NocoUI/Component/DragDropTarget.hpp"

namespace noco
{
	Array<std::shared_ptr<Node>> DragDropSource::onStartDragging(const std::shared_ptr<Node>& node) const
	{
		if (m_onStartDragging)
		{
			// 複数ノードのドラッグ用にArrayで返す
			return m_onStartDragging();
		}

		// onStartDraggingの指定がない場合は自身のノードのみをドラッグ
		return { node };
	}
	
	void DragDropSource::update(const std::shared_ptr<Node>& sourceNode)
	{
		const bool canDrag = detail::s_canvasUpdateContext.draggingNode.expired();

		if (canDrag && MouseL.pressed())
		{
			if (!m_isPressed && sourceNode->isMouseDown())
			{
				m_dragStartPosition = Cursor::PosF();
				m_isPressed = true;
			}
			if (m_isPressed && !m_isDragging && (Cursor::PosF() - m_dragStartPosition).length() > m_dragThreshold)
			{
				m_draggingNodeList = onStartDragging(sourceNode);

				m_originalIsHitTargets.clear();
				m_originalIsHitTargets.reserve(m_draggingNodeList.size());
				for (const auto& draggingNode : m_draggingNodeList)
				{
					m_originalIsHitTargets.push_back(draggingNode->isHitTarget());
				}

				m_isDragging = true;
			}
			if (m_isPressed)
			{
				detail::s_canvasUpdateContext.draggingNode = sourceNode;
			}
		}
		else
		{
			if (m_isDragging && !MouseL.pressed())
			{
				const auto dropTargetNode = detail::s_canvasUpdateContext.hoveredNode.lock();
				if (dropTargetNode && dropTargetNode != sourceNode)
				{
					if (const auto dropTarget = dropTargetNode->getComponentOrNull<DragDropTarget>())
					{
						dropTarget->onDrop(m_draggingNodeList);
					}
				}
			}
			m_isPressed = false;
			m_isDragging = false;
		}

		if (m_movesTransformEffectPosition)
		{
			if (m_isDragging)
			{
				for (size_t i = 0; i < m_draggingNodeList.size(); ++i)
				{
					const auto& draggingNode = m_draggingNodeList[i];
					draggingNode->transformEffect().setPosition(Vec2{ Cursor::PosF() - m_dragStartPosition });
					draggingNode->setIsHitTarget(false); // カーソルに合わせて動かすとドロップ先にホバーできないので一時的にホバー無効にする
				}
			}
			else if (m_prevIsDragging)
			{
				for (size_t i = 0; i < m_draggingNodeList.size(); ++i)
				{
					const auto& draggingNode = m_draggingNodeList[i];
					draggingNode->transformEffect().setPosition(Vec2::Zero());
					draggingNode->setIsHitTarget(m_originalIsHitTargets[i]);
				}
			}
		}

		if (!m_isDragging && m_prevIsDragging)
		{
			m_draggingNodeList.clear();
			m_originalIsHitTargets.clear();
		}

		m_prevIsDragging = m_isDragging;
	}
}
