#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"
#include "DragDropTarget.hpp"
#include "../Canvas.hpp"

namespace noco
{
	class DragDropSource : public ComponentBase
	{
	private:
		double m_dragThreshold;
		bool m_movesTransformTranslate;
		bool m_isPressed = false;
		bool m_isDragging = false;
		bool m_prevIsDragging = false;
		Vec2 m_dragStartPosition = Vec2::Zero();
		Array<std::shared_ptr<Node>> m_draggingNodeList;
		Array<IsHitTargetYN> m_originalIsHitTargets;
		std::function<Array<std::shared_ptr<Node>>()> m_onStartDragging;

		[[nodiscard]]
		Array<std::shared_ptr<Node>> onStartDragging(const std::shared_ptr<Node>& node) const;

	public:
		explicit DragDropSource(std::function<Array<std::shared_ptr<Node>>()> onStartDragging = nullptr, double dragThreshold = 5.0, bool movesTransformTranslate = true)
			: ComponentBase{ {} }
			, m_dragThreshold{ dragThreshold }
			, m_movesTransformTranslate{ movesTransformTranslate }
			, m_onStartDragging{ std::move(onStartDragging) }
		{
		}

		void update(const std::shared_ptr<Node>& sourceNode) override;

		// TODO: 本来は破棄時にも実行すべき内容なので、onDeactivatedを実装可能にする
		void updateInactive(const std::shared_ptr<Node>& sourceNode) override;

		[[nodiscard]]
		const Array<std::shared_ptr<Node>>& draggingNodeList() const
		{
			return m_draggingNodeList;
		}
	};
}
