#pragma once
#include <Siv3D.hpp>
#include "NocoUI.hpp"

namespace noco::editor
{
	enum class ResizeDirection
	{
		Horizontal,
		Vertical,
	};

	class ResizableHandle
	{
	private:
		std::shared_ptr<Canvas> m_editorCanvas;
		std::shared_ptr<Node> m_handleNode;
		ResizeDirection m_direction;
		double m_handleThickness;
		std::function<void(double)> m_onResize;

		bool m_isDragging = false;
		Vec2 m_dragStartPos = Vec2::Zero();
		double m_dragStartValue = 0.0;

	public:
		explicit ResizableHandle(
			const std::shared_ptr<Canvas>& editorCanvas,
			ResizeDirection direction,
			double handleThickness = 8.0);

		void setPosition(const Vec2& position);

		void setSize(const Vec2& size);

		void setOnResize(std::function<void(double)> onResize);

		void setDragStartValue(double value);

		void update();

		[[nodiscard]]
		std::shared_ptr<Node> getNode() const;
	};
}
