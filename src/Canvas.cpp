#include "NocoUI/Canvas.hpp"
#include <cassert>

namespace noco
{
	void Canvas::EventRegistry::addEvent(const Event& event)
	{
		m_events.push_back(event);
	}

	void Canvas::EventRegistry::clear()
	{
		m_events.clear();
	}

	bool Canvas::EventRegistry::isEventFiredWithTag(StringView tag) const
	{
		if (m_events.empty())
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
		if (m_events.empty())
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
		if (m_events.empty())
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
		return m_events;
	}

	Mat3x2 Canvas::rootPosScaleMat() const
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
		rootNode->resetScrollOffset(RecursiveYN::Yes, RefreshesLayoutYN::No);
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

		m_rootNode->refreshBoxChildrenLayout();
		m_rootNode->refreshPosScaleAppliedRect(RecursiveYN::Yes, rootPosScaleMat(), m_scale, 0.0, rootPosScaleMat(), 0.0, rootPosScaleMat());
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
	
	JSON Canvas::toJSONImpl(detail::IncludesInternalIdYN includesInternalId) const
	{
		return m_rootNode->toJSONImpl(includesInternalId);
	}
	
	std::shared_ptr<Canvas> Canvas::CreateFromJSON(const JSON& json, RefreshesLayoutYN refreshesLayout)
	{
		return Create(Node::CreateFromJSON(json), refreshesLayout);
	}
	
	std::shared_ptr<Canvas> Canvas::CreateFromJSONImpl(const JSON& json, detail::IncludesInternalIdYN includesInternalId, RefreshesLayoutYN refreshesLayout)
	{
		return Create(Node::CreateFromJSONImpl(json, includesInternalId), refreshesLayout);
	}
	
	bool Canvas::tryReadFromJSON(const JSON& json, RefreshesLayoutYN refreshesLayoutPre, RefreshesLayoutYN refreshesLayoutPost)
	{
		m_rootNode = Node::CreateFromJSON(json);
		m_rootNode->setCanvasRecursive(shared_from_this()); // コンストラクタ内ではshared_from_this()が使えないためここで設定
		if (refreshesLayoutPre)
		{
			refreshLayout();
		}
		m_rootNode->resetScrollOffset(RecursiveYN::Yes, RefreshesLayoutYN::No, RefreshesLayoutYN::No);
		if (refreshesLayoutPost)
		{
			refreshLayout();
		}
		return true; // TODO: 失敗したらfalseを返す
	}
	
	bool Canvas::tryReadFromJSONImpl(const JSON& json, detail::IncludesInternalIdYN includesInternalId, RefreshesLayoutYN refreshesLayoutPre, RefreshesLayoutYN refreshesLayoutPost)
	{
		m_rootNode = Node::CreateFromJSONImpl(json, includesInternalId);
		m_rootNode->setCanvasRecursive(shared_from_this()); // コンストラクタ内ではshared_from_this()が使えないためここで設定
		if (refreshesLayoutPre)
		{
			refreshLayout();
		}
		m_rootNode->resetScrollOffset(RecursiveYN::Yes, RefreshesLayoutYN::No, RefreshesLayoutYN::No);
		if (refreshesLayoutPost)
		{
			refreshLayout();
		}
		return true; // TODO: 失敗したらfalseを返す
	}
	
	void Canvas::update(HitTestEnabledYN hitTestEnabled)
	{
		m_eventRegistry.clear();

		noco::detail::ClearCanvasUpdateContextIfNeeded();

		if (!m_rootNode)
		{
			return;
		}
		
		// ホバー中ノード取得
		const bool canHover = hitTestEnabled && !CurrentFrame::AnyNodeHovered() && Window::GetState().focused; // TODO: 本来はウィンドウがアクティブでない場合もホバーさせたいが、重なった他ウィンドウクリック時に押下扱いになってしまうため除外している
		const auto hoveredNode = canHover ? m_rootNode->hoveredNodeRecursive() : nullptr;
		if (hoveredNode)
		{
			detail::s_canvasUpdateContext.hoveredNode = hoveredNode;
		}

		// スクロール可能なホバー中ノード取得
		auto scrollableHoveredNode = hoveredNode ? hoveredNode->findContainedScrollableNode() : nullptr;
		if (scrollableHoveredNode && !scrollableHoveredNode->rect().mouseOver())
		{
			// 子がホバー中でもスクロール可能ノード自身にマウスカーソルが重なっていない場合はスクロールしない
			scrollableHoveredNode = nullptr;
		}
		if (scrollableHoveredNode)
		{
			detail::s_canvasUpdateContext.scrollableHoveredNode = scrollableHoveredNode;
		}

		// ホイールスクロール実行
		if (scrollableHoveredNode && scrollableHoveredNode->wheelScrollEnabled())
		{
			const double wheel = Mouse::Wheel();
			const double wheelH = Mouse::WheelH();
			if (wheel != 0.0 || wheelH != 0.0)
			{
				scrollableHoveredNode->scroll(Vec2{ wheelH * 50, wheel * 50 });
			}
		}
		
		// ドラッグスクロール中の処理
		const auto dragScrollingNode = detail::s_canvasUpdateContext.dragScrollingNode.lock();
		if (dragScrollingNode && dragScrollingNode->dragScrollEnabled() && MouseL.pressed())
		{
			if (dragScrollingNode->m_dragStartPos)
			{
				// effectScaleを考慮してドラッグ量を計算(ゼロ除算を防ぐ)
				const Vec2 effectScale = dragScrollingNode->effectScale();
				const Vec2 screenDragDelta = Cursor::PosF() - *dragScrollingNode->m_dragStartPos;
				const Vec2 dragDelta = Vec2{
					effectScale.x > 0.0 ? screenDragDelta.x / effectScale.x : 0.0,
					effectScale.y > 0.0 ? screenDragDelta.y / effectScale.y : 0.0
				};
				
				// ドラッグ閾値の判定(画面座標での判定)
				constexpr double DragThreshold = 4.0;
				if (!dragScrollingNode->m_dragThresholdExceeded)
				{
					if (screenDragDelta.length() >= DragThreshold)
					{
						dragScrollingNode->m_dragThresholdExceeded = true;
					}
				}
				
				// 閾値を超えた場合のみスクロール処理を実行
				if (dragScrollingNode->m_dragThresholdExceeded)
				{
					Vec2 newScrollOffset = dragScrollingNode->m_dragStartScrollOffset - dragDelta;
					
					// ラバーバンドスクロールが有効な場合、範囲外での抵抗を適用
					if (dragScrollingNode->rubberBandScrollEnabled())
					{
						const auto [minScroll, maxScroll] = dragScrollingNode->getValidScrollRange();
						constexpr double RubberBandResistance = 0.3; // 抵抗係数（0.0～1.0、小さいほど抵抗が強い）
						
						// X軸の抵抗
						if (newScrollOffset.x < minScroll.x)
						{
							const double overshoot = minScroll.x - newScrollOffset.x;
							newScrollOffset.x = minScroll.x - overshoot * RubberBandResistance;
						}
						else if (newScrollOffset.x > maxScroll.x)
						{
							const double overshoot = newScrollOffset.x - maxScroll.x;
							newScrollOffset.x = maxScroll.x + overshoot * RubberBandResistance;
						}
						
						// Y軸の抵抗
						if (newScrollOffset.y < minScroll.y)
						{
							const double overshoot = minScroll.y - newScrollOffset.y;
							newScrollOffset.y = minScroll.y - overshoot * RubberBandResistance;
						}
						else if (newScrollOffset.y > maxScroll.y)
						{
							const double overshoot = newScrollOffset.y - maxScroll.y;
							newScrollOffset.y = maxScroll.y + overshoot * RubberBandResistance;
						}
					}
					
					// scroll関数側でスクロール方向の制限がかかるので、ここでは特に制限しない
					// (後からスクロール可能になった場合にその軸のスクロール分は即座に反映したいため)
					Vec2 scrollDelta = newScrollOffset - dragScrollingNode->scrollOffset();
					if (scrollDelta != Vec2::Zero())
					{
						dragScrollingNode->scroll(scrollDelta);
						
						// 速度計算
						const double deltaTime = dragScrollingNode->m_dragVelocityStopwatch.sF();
						constexpr double MinDeltaTime = 0.001; // 最小時間を設定して過度な速度を防ぐ
						if (deltaTime > MinDeltaTime)
						{
							// スクロール方向はドラッグ方向と同じ
							const Vec2 newVelocity = scrollDelta / deltaTime;
							// 速度を適切な範囲に制限
							constexpr double MaxVelocity = 2000.0;
							if (newVelocity.length() > MaxVelocity)
							{
								dragScrollingNode->m_scrollVelocity = newVelocity.normalized() * MaxVelocity;
							}
							else
							{
								// 移動平均を使用して滑らかに
								constexpr double SmoothingFactor = 0.2;
								dragScrollingNode->m_scrollVelocity = dragScrollingNode->m_scrollVelocity * (1.0 - SmoothingFactor) + newVelocity * SmoothingFactor;
							}
						}
						dragScrollingNode->m_dragVelocityStopwatch.restart();
					}
				}
			}
		}
		
		// ドラッグスクロール終了判定
		if (dragScrollingNode && (MouseL.up() || !MouseL.pressed()))
		{
			dragScrollingNode->m_dragStartPos.reset();
			dragScrollingNode->m_dragVelocityStopwatch.reset();
			dragScrollingNode->m_dragThresholdExceeded = false; // 閾値フラグをリセット
			detail::s_canvasUpdateContext.dragScrollingNode.reset();
			// 慣性スクロールは各ノードがupdateで処理される
		}

		// ノード更新
		const bool currentDragScrollingWithThreshold = dragScrollingNode && dragScrollingNode->m_dragThresholdExceeded;
		const IsScrollingYN isScrolling{ currentDragScrollingWithThreshold || m_prevDragScrollingWithThresholdExceeded };
		m_rootNode->updateInteractionState(hoveredNode, Scene::DeltaTime(), InteractableYN::Yes, InteractionState::Default, InteractionState::Default, isScrolling);
		m_rootNode->updateInput();
		m_rootNode->update(scrollableHoveredNode, Scene::DeltaTime(), rootPosScaleMat(), m_scale, rootPosScaleMat(), 0.0, 0.0, rootPosScaleMat(), {});
		m_rootNode->lateUpdate();
		m_rootNode->postLateUpdate(Scene::DeltaTime());

		// ドラッグスクロール開始判定
		// (ドラッグアンドドロップと競合しないよう、フレームの最後に実施)
		if (!IsDraggingNode() && scrollableHoveredNode && scrollableHoveredNode->dragScrollEnabled() && detail::s_canvasUpdateContext.dragScrollingNode.expired() && MouseL.down())
		{
			scrollableHoveredNode->m_dragStartPos = Cursor::PosF();
			scrollableHoveredNode->m_dragStartScrollOffset = scrollableHoveredNode->scrollOffset();
			scrollableHoveredNode->m_scrollVelocity = Vec2::Zero(); // ドラッグ開始時に慣性をリセット
			scrollableHoveredNode->m_dragVelocityStopwatch.restart();
			scrollableHoveredNode->m_dragThresholdExceeded = false; // 閾値フラグを初期化
			detail::s_canvasUpdateContext.dragScrollingNode = scrollableHoveredNode;
		}
		
		// 前フレームの状態を更新
		m_prevDragScrollingWithThresholdExceeded = currentDragScrollingWithThreshold;
	}
	
	void Canvas::draw() const
	{
		if (m_rootNode)
		{
			m_rootNode->draw();
		}
	}
	
	const std::shared_ptr<Node>& Canvas::rootNode() const
	{
		return m_rootNode;
	}
	
	void Canvas::removeChildrenAll()
	{
		m_rootNode->removeChildrenAll();
	}

	std::shared_ptr<Canvas> Canvas::setOffset(const Vec2& offset)
	{
		m_offset = offset;
		m_rootNode->refreshPosScaleAppliedRect(RecursiveYN::Yes, rootPosScaleMat(), m_scale, 0.0, rootPosScaleMat(), 0.0, rootPosScaleMat());
		return shared_from_this();
	}
	
	std::shared_ptr<Canvas> Canvas::setScale(const Vec2& scale)
	{
		m_scale = scale;
		m_rootNode->refreshPosScaleAppliedRect(RecursiveYN::Yes, rootPosScaleMat(), m_scale, 0.0, rootPosScaleMat(), 0.0, rootPosScaleMat());
		return shared_from_this();
	}
	
	std::shared_ptr<Canvas> Canvas::setOffsetScale(const Vec2& offset, const Vec2& scale)
	{
		m_offset = offset;
		m_scale = scale;
		m_rootNode->refreshPosScaleAppliedRect(RecursiveYN::Yes, rootPosScaleMat(), m_scale, 0.0, rootPosScaleMat(), 0.0, rootPosScaleMat());
		return shared_from_this();
	}
	
	void Canvas::resetScrollOffsetRecursive(RefreshesLayoutYN refreshesLayout)
	{
		m_rootNode->resetScrollOffset(RecursiveYN::Yes, RefreshesLayoutYN::No, RefreshesLayoutYN::No);
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
