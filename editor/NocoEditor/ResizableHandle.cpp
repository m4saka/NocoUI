#include "ResizableHandle.hpp"

namespace noco::editor
{
	ResizableHandle::ResizableHandle(
		const std::shared_ptr<Canvas>& editorCanvas,
		ResizeDirection direction,
		double handleThickness)
		: m_editorCanvas(editorCanvas)
		, m_direction(direction)
		, m_handleThickness(handleThickness)
	{
		m_handleNode = editorCanvas->emplaceChild(
			U"ResizableHandle",
			AnchorRegion
			{
				.anchorMin = Anchor::TopLeft,
				.anchorMax = Anchor::TopLeft,
				.posDelta = Vec2{ 0, 0 },
				.sizeDelta = Vec2{ handleThickness, handleThickness },
				.sizeDeltaPivot = Anchor::TopLeft,
			});
	}

	void ResizableHandle::setPosition(const Vec2& position)
	{
		if (auto* pAnchorRegion = m_handleNode->anchorRegion())
		{
			auto newRegion = *pAnchorRegion;
			newRegion.posDelta = position;
			m_handleNode->setRegion(newRegion);
		}
		else
		{
			Logger << U"[NocoEditor warning] AnchorRegion not found in handleNode";
		}
	}

	void ResizableHandle::setSize(const Vec2& size)
	{
		if (auto* pAnchorRegion = m_handleNode->anchorRegion())
		{
			auto newRegion = *pAnchorRegion;
			newRegion.sizeDelta = size;
			m_handleNode->setRegion(newRegion);
		}
		else
		{
			Logger << U"[NocoEditor warning] AnchorRegion not found in handleNode";
		}
	}

	void ResizableHandle::setOnResize(std::function<void(double)> onResize)
	{
		m_onResize = std::move(onResize);
	}

	void ResizableHandle::setDragStartValue(double value)
	{
		m_dragStartValue = value;
	}

	void ResizableHandle::update()
	{
		const bool isHovered = m_handleNode->isHovered();
		const bool isPressed = m_handleNode->isPressed();

		// マウスカーソルの変更
		if (isHovered || m_isDragging)
		{
			if (m_direction == ResizeDirection::Horizontal)
			{
				Cursor::RequestStyle(CursorStyle::ResizeLeftRight);
			}
			else
			{
				Cursor::RequestStyle(CursorStyle::ResizeUpDown);
			}
		}

		// ドラッグ処理
		if (!m_isDragging && isPressed && MouseL.down())
		{
			m_isDragging = true;
			m_dragStartPos = Cursor::PosF();
			// 現在のマウス位置を開始値として設定
			if (m_direction == ResizeDirection::Horizontal)
			{
				m_dragStartValue = m_dragStartPos.x;
			}
			else
			{
				m_dragStartValue = m_dragStartPos.y;
			}
		}

		if (m_isDragging)
		{
			if (MouseL.pressed())
			{
				const Vec2 currentPos = Cursor::PosF();

				double currentValue = 0;
				if (m_direction == ResizeDirection::Horizontal)
				{
					currentValue = currentPos.x;
				}
				else
				{
					currentValue = currentPos.y;
				}

				if (m_onResize)
				{
					m_onResize(currentValue);
				}
			}
			else
			{
				m_isDragging = false;
			}
		}
	}

	std::shared_ptr<Node> ResizableHandle::getNode() const
	{
		return m_handleNode;
	}
}
