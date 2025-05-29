#include "NocoUI/Canvas.hpp"
#include <cassert>

namespace noco
{
	Canvas::EventRegistry::EventRegistry()
	{
		m_events.reserve(InitialCapacity);
	}

	void Canvas::EventRegistry::addEvent(const Event& event)
	{
		const int32 frameCount = Scene::FrameCount();
		if (m_prevFrameCount != frameCount)
		{
			// フレームが変わったらイベントをクリア
			m_events.clear();
		}

		m_events.push_back(event);
		m_prevFrameCount = frameCount;
	}

	bool Canvas::EventRegistry::isEventFiredWithTag(StringView tag) const
	{
		if (m_events.empty() || m_prevFrameCount != Scene::FrameCount())
		{
			return false;
		}
		for (const auto& event : m_events)
		{
			if (event.tag == tag)
			{
				return true;
			}
		}
		return false;
	}

	Optional<Event> Canvas::EventRegistry::getFiredEventWithTag(StringView tag) const
	{
		if (m_events.empty() || m_prevFrameCount != Scene::FrameCount())
		{
			return none;
		}
		for (const auto& event : m_events)
		{
			if (event.tag == tag)
			{
				return event;
			}
		}
		return none;
	}

	Array<Event> Canvas::EventRegistry::getFiredEventsWithTag(StringView tag) const
	{
		if (m_events.empty() || m_prevFrameCount != Scene::FrameCount())
		{
			return {};
		}
		Array<Event> events;
		for (const auto& event : m_events)
		{
			if (event.tag == tag)
			{
				events.push_back(event);
			}
		}
		return events;
	}

	const Array<Event>& Canvas::EventRegistry::getFiredEventsAll() const
	{
		if (m_prevFrameCount != Scene::FrameCount())
		{
			return m_emptyEvents;
		}
		else
		{
			return m_events;
		}
	}

	Mat3x2 Canvas::rootEffectMat() const
	{
		if (m_scale == Vec2::One() && m_offset == Vec2::Zero())
		{
			return Mat3x2::Identity();
		}
		return Mat3x2::Scale(m_scale) * Mat3x2::Translate(m_offset);
	}

	Canvas::Canvas()
		: Canvas{ Node::Create(
			U"Canvas",
			AnchorConstraint
			{
				.anchorMin = Anchor::TopLeft,
				.anchorMax = Anchor::BottomRight,
				.posDelta = Vec2{ 0, 0 },
				.sizeDelta = Vec2{ 0, 0 },
				.sizeDeltaPivot = Anchor::MiddleCenter,
			},
			IsHitTargetYN::No) }
	{
	}

	Canvas::Canvas(const std::shared_ptr<Node>& rootNode)
		: m_rootNode{ rootNode }
	{
		refreshLayout();
	}

	std::shared_ptr<Canvas> Canvas::Create()
	{
		std::shared_ptr<Canvas> canvas{ new Canvas{} };

		// コンストラクタ内ではshared_from_this()が使えないためここで設定
		canvas->rootNode()->setCanvasRecursive(canvas);

		return canvas;
	}

	std::shared_ptr<Canvas> Canvas::Create(const std::shared_ptr<Node>& rootNode, RefreshesLayoutYN refreshesLayoutPre, RefreshesLayoutYN refreshesLayoutPost)
	{
		std::shared_ptr<Canvas> canvas{ new Canvas{ rootNode } };

		// rootNodeは同一インスタンスになる想定
		assert(canvas->rootNode().get() == rootNode.get());

		rootNode->setCanvasRecursive(canvas); // コンストラクタ内ではshared_from_this()が使えないためここで設定

		if (refreshesLayoutPre)
		{
			canvas->refreshLayout();
		}
		rootNode->resetScrollOffsetRecursive(RefreshesLayoutYN::No);
		if (refreshesLayoutPost)
		{
			canvas->refreshLayout();
		}

		return canvas;
	}

	void Canvas::refreshLayout()
	{
		const auto& rootConstraint = m_rootNode->constraint();
		if (const auto pBoxConstraint = std::get_if<BoxConstraint>(&rootConstraint))
		{
			m_rootNode->m_layoutAppliedRect = pBoxConstraint->applyConstraint(Scene::Rect(), Vec2::Zero());
		}
		else if (const auto pAnchorConstraint = std::get_if<AnchorConstraint>(&rootConstraint))
		{
			m_rootNode->m_layoutAppliedRect = pAnchorConstraint->applyConstraint(Scene::Rect(), Vec2::Zero());
		}
		else
		{
			// TODO: 実行時例外ではなくコンパイルエラーにしたい
			throw Error{ U"Unknown root node constraint" };
		}

		m_rootNode->refreshChildrenLayout();
		m_rootNode->refreshEffectedRect(rootEffectMat(), m_scale);
	}

	bool Canvas::containsNodeByName(const String& nodeName) const
	{
		if (m_rootNode->name() == nodeName)
		{
			return true;
		}
		return m_rootNode->containsChildByName(nodeName, RecursiveYN::Yes);
	}
	
	std::shared_ptr<Node> Canvas::getNodeByName(const String& nodeName) const
	{
		if (m_rootNode->name() == nodeName)
		{
			return m_rootNode;
		}
		return m_rootNode->getChildByName(nodeName, RecursiveYN::Yes);
	}
	
	JSON Canvas::toJSON() const
	{
		return m_rootNode->toJSON();
	}
	
	std::shared_ptr<Canvas> Canvas::CreateFromJSON(const JSON& json, RefreshesLayoutYN refreshesLayout)
	{
		return Create(Node::CreateFromJSON(json), refreshesLayout);
	}
	
	bool Canvas::tryReadFromJSON(const JSON& json, RefreshesLayoutYN refreshesLayoutPre, RefreshesLayoutYN refreshesLayoutPost)
	{
		m_rootNode = Node::CreateFromJSON(json);
		m_rootNode->setCanvasRecursive(shared_from_this()); // コンストラクタ内ではshared_from_this()が使えないためここで設定
		if (refreshesLayoutPre)
		{
			refreshLayout();
		}
		m_rootNode->resetScrollOffsetRecursive(RefreshesLayoutYN::No, RefreshesLayoutYN::No);
		if (refreshesLayoutPost)
		{
			refreshLayout();
		}
		return true; // TODO: 失敗したらfalseを返す
	}
	
	void Canvas::update(CanvasUpdateContext* pContext)
	{
		// ホバー中ノード取得
		const bool canHover = (pContext ? pContext->canHover && !pContext->isHovered() : true) && Window::GetState().focused; // TODO: 本来はウィンドウがアクティブでない場合もホバーさせたい
		const auto hoveredNode = canHover ? m_rootNode->hoveredNodeRecursive() : nullptr;

		// スクロール可能なホバー中ノード取得
		auto scrollableHoveredNode = hoveredNode ? hoveredNode->findContainedScrollableNode() : nullptr;
		if (scrollableHoveredNode && !scrollableHoveredNode->rect().mouseOver())
		{
			// 子がホバー中でもスクロール可能ノード自身にマウスカーソルが重なっていない場合はスクロールしない
			scrollableHoveredNode = nullptr;
		}

		// スクロール実行
		if (scrollableHoveredNode)
		{
			const double wheel = Mouse::Wheel();
			const double wheelH = Mouse::WheelH();
			if (wheel != 0.0 || wheelH != 0.0)
			{
				scrollableHoveredNode->scroll(Vec2{ wheelH * 50, wheel * 50 });
			}
		}

		if (pContext)
		{
			pContext->canHover = pContext->canHover && hoveredNode == nullptr;
			if (hoveredNode)
			{
				pContext->hoveredNode = hoveredNode;
			}
			if (scrollableHoveredNode)
			{
				pContext->scrollableHoveredNode = scrollableHoveredNode;
			}
		}

		// ノード更新
		m_rootNode->updateInput(pContext);
		m_rootNode->update(pContext, hoveredNode, scrollableHoveredNode, Scene::DeltaTime(), rootEffectMat(), m_scale, InteractableYN::Yes, InteractState::Default, InteractState::Default);
		m_rootNode->lateUpdate(pContext);
		m_rootNode->postLateUpdate();
	}
	
	void Canvas::draw() const
	{
		m_rootNode->draw();
	}
	
	const std::shared_ptr<Node>& Canvas::rootNode() const
	{
		return m_rootNode;
	}
	
	void Canvas::removeChildrenAll()
	{
		m_rootNode->removeChildrenAll();
	}

	void Canvas::setOffset(const Vec2& offset)
	{
		m_offset = offset;
		m_rootNode->refreshEffectedRect(rootEffectMat(), m_scale);
	}
	
	void Canvas::setScale(const Vec2& scale)
	{
		m_scale = scale;
		m_rootNode->refreshEffectedRect(rootEffectMat(), m_scale);
	}
	
	void Canvas::setOffsetScale(const Vec2& offset, const Vec2& scale)
	{
		m_offset = offset;
		m_scale = scale;
		m_rootNode->refreshEffectedRect(rootEffectMat(), m_scale);
	}
	
	void Canvas::resetScrollOffsetRecursive(RefreshesLayoutYN refreshesLayout)
	{
		m_rootNode->resetScrollOffsetRecursive(RefreshesLayoutYN::No, RefreshesLayoutYN::No);
		if (refreshesLayout)
		{
			refreshLayout();
		}
	}
	
	void Canvas::fireEvent(const Event& event)
	{
		m_eventRegistry.addEvent(event);
	}

	bool Canvas::isEventFiredWithTag(StringView tag) const
	{
		return m_eventRegistry.isEventFiredWithTag(tag);
	}

	Optional<Event> Canvas::getFiredEventWithTag(StringView tag) const
	{
		return m_eventRegistry.getFiredEventWithTag(tag);
	}

	Array<Event> Canvas::getFiredEventsWithTag(StringView tag) const
	{
		return m_eventRegistry.getFiredEventsWithTag(tag);
	}

	const Array<Event>& Canvas::getFiredEventsAll() const
	{
		return m_eventRegistry.getFiredEventsAll();
	}
}
