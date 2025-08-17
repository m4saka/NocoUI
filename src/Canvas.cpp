#include "NocoUI/Canvas.hpp"
#include "NocoUI/Version.hpp"
#include <cassert>

namespace noco
{
	class Node;
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

	void Canvas::updateAutoResizeIfNeeded()
	{
		if (m_autoResizeMode == AutoResizeMode::None || m_isEditorPreview)
		{
			m_lastSceneSize = none;
			return;
		}
		
		const SizeF currentSceneSize = Scene::Size();
		
		if (!m_lastSceneSize || *m_lastSceneSize != currentSceneSize)
		{
			m_lastSceneSize = currentSceneSize;
			
			if (m_autoResizeMode == AutoResizeMode::MatchSceneSize)
			{
				setSize(currentSceneSize, RefreshesLayoutYN::Yes);
				m_position = Vec2::Zero();
			}
		}
	}

	Vec2 Canvas::calculateAutoScale() const
	{
		if (m_autoScaleMode == AutoScaleMode::None || m_isEditorPreview)
		{
			return Vec2::One();
		}
		
		const SizeF sceneSize = Scene::Size();
		const double scaleX = sceneSize.x / m_size.x;
		const double scaleY = sceneSize.y / m_size.y;
		
		switch (m_autoScaleMode)
		{
			case AutoScaleMode::ShrinkToFit:
			{
				const double scale = Min(scaleX, scaleY);
				return Vec2{ scale, scale };
			}
				
			case AutoScaleMode::ExpandToFill:
			{
				const double scale = Max(scaleX, scaleY);
				return Vec2{ scale, scale };
			}
				
			case AutoScaleMode::FitHeight:
				return Vec2{ scaleY, scaleY };
				
			case AutoScaleMode::FitWidth:
				return Vec2{ scaleX, scaleX };
				
			default:
				return Vec2::One();
		}
	}

	Mat3x2 Canvas::rootPosScaleMat() const
	{
		const Vec2 autoScale = calculateAutoScale();
		const Vec2 finalScale = m_scale * autoScale;
		
		if (finalScale == Vec2::One() && m_position == Vec2::Zero() && m_rotation == 0.0)
		{
			return Mat3x2::Identity();
		}
		return Mat3x2::Scale(finalScale) * Mat3x2::Rotate(m_rotation) * Mat3x2::Translate(m_position);
	}

	Canvas::Canvas()
	{
	}

	Canvas::~Canvas()
	{
		for (const auto& child : m_children)
		{
			child->setCanvasRecursive(std::weak_ptr<Canvas>{});
			child->refreshActiveInHierarchy();
		}
	}

	std::shared_ptr<Canvas> Canvas::Create(const SizeF& size)
	{
		std::shared_ptr<Canvas> canvas{ new Canvas{} };
		canvas->m_size = size;
		return canvas;
	}
	
	std::shared_ptr<Canvas> Canvas::Create(double width, double height)
	{
		return Create(SizeF{ width, height });
	}

	void Canvas::refreshLayout()
	{
		const RectF canvasRect{ 0, 0, m_size.x, m_size.y };
		
		for (const auto& child : m_children)
		{
			const auto& childRegion = child->region();
			if (const auto pInlineRegion = std::get_if<InlineRegion>(&childRegion))
			{
				child->m_regionRect = pInlineRegion->applyRegion(canvasRect, Vec2::Zero());
			}
			else if (const auto pAnchorRegion = std::get_if<AnchorRegion>(&childRegion))
			{
				child->m_regionRect = pAnchorRegion->applyRegion(canvasRect, Vec2::Zero());
			}
			else
			{
				throw Error{ U"Unknown child node region" };
			}

			child->refreshChildrenLayout();
			child->refreshTransformMat(RecursiveYN::Yes, rootPosScaleMat(), rootPosScaleMat(), m_params);
		}
	}

	bool Canvas::containsNodeByName(const String& nodeName) const
	{
		for (const auto& child : m_children)
		{
			if (child->name() == nodeName)
			{
				return true;
			}
			if (child->containsChildByName(nodeName, RecursiveYN::Yes))
			{
				return true;
			}
		}
		return false;
	}
	
	std::shared_ptr<Node> Canvas::getNodeByName(const String& nodeName) const
	{
		for (const auto& child : m_children)
		{
			if (child->name() == nodeName)
			{
				return child;
			}
			if (auto found = child->getChildByNameOrNull(nodeName, RecursiveYN::Yes))
			{
				return found;
			}
		}
		return nullptr;
	}
	
	JSON Canvas::toJSON() const
	{
		JSON json = JSON
		{
			{ U"version", NocoUIVersion },
			{ U"size", ValueToString(m_size) },
		};

		if (m_autoScaleMode != AutoScaleMode::None)
		{
			json[U"autoScaleMode"] = ValueToString(m_autoScaleMode);
		}
		if (m_autoResizeMode != AutoResizeMode::None)
		{
			json[U"autoResizeMode"] = ValueToString(m_autoResizeMode);
		}

		Array<JSON> childrenArray;
		for (const auto& child : m_children)
		{
			childrenArray.push_back(child->toJSON());
		}
		json[U"children"] = childrenArray;

		if (!m_params.empty())
		{
			JSON paramsObj = JSON{};
			for (const auto& [name, value] : m_params)
			{
				paramsObj[name] = ParamValueToJSON(value);
			}
			json[U"params"] = paramsObj;
		}

		return json;
	}
	
	JSON Canvas::toJSONImpl(detail::WithInstanceIdYN withInstanceId) const
	{
		JSON json = JSON
		{
			{ U"version", NocoUIVersion },
			{ U"size", ValueToString(m_size) },
		};

		if (m_autoScaleMode != AutoScaleMode::None)
		{
			json[U"autoScaleMode"] = ValueToString(m_autoScaleMode);
		}
		if (m_autoResizeMode != AutoResizeMode::None)
		{
			json[U"autoResizeMode"] = ValueToString(m_autoResizeMode);
		}

		Array<JSON> childrenArray;
		for (const auto& child : m_children)
		{
			childrenArray.push_back(child->toJSONImpl(withInstanceId));
		}
		json[U"children"] = childrenArray;

		if (!m_params.empty())
		{
			JSON paramsObj = JSON{};
			for (const auto& [name, value] : m_params)
			{
				paramsObj[name] = ParamValueToJSON(value);
			}
			json[U"params"] = paramsObj;
		}

		return json;
	}
	
	std::shared_ptr<Canvas> Canvas::CreateFromJSON(const JSON& json, RefreshesLayoutYN refreshesLayout)
	{
		if (!json.contains(U"size"))
		{
			Logger << U"[NocoUI error] Canvas::CreateFromJSON: Missing required field 'size'";
			return nullptr;
		}
		
		if (!json.contains(U"children"))
		{
			Logger << U"[NocoUI error] Canvas::CreateFromJSON: Missing required field 'children'";
			return nullptr;
		}
		
		std::shared_ptr<Canvas> canvas{ new Canvas{} };
		
		if (auto sizeOpt = StringToValueOpt<SizeF>(json[U"size"].get<String>()))
		{
			canvas->m_size = *sizeOpt;
		}
		else
		{
			Logger << U"[NocoUI warning] Canvas::CreateFromJSON: Failed to parse size, using default";
		}
		
		for (const auto& childJson : json[U"children"].arrayView())
		{
			if (auto child = Node::CreateFromJSON(childJson))
			{
				canvas->addChild(child, RefreshesLayoutYN::No);
			}
		}

		if (json.contains(U"params"))
		{
			if (json[U"params"].isObject())
			{
				for (const auto& member : json[U"params"])
				{
					const String name = member.key;
					const auto& paramJson = member.value;
					if (auto value = ParamValueFromJSON(paramJson))
					{
						canvas->m_params[name] = *value;
					}
				}
			}
		}

		if (json.contains(U"autoScaleMode"))
		{
			if (const auto modeOpt = StringToValueOpt<AutoScaleMode>(json[U"autoScaleMode"].get<String>()))
			{
				canvas->m_autoScaleMode = *modeOpt;
			}
		}
		if (json.contains(U"autoResizeMode"))
		{
			if (const auto modeOpt = StringToValueOpt<AutoResizeMode>(json[U"autoResizeMode"].get<String>()))
			{
				canvas->m_autoResizeMode = *modeOpt;
				if (canvas->m_autoResizeMode != AutoResizeMode::None)
				{
					canvas->m_lastSceneSize = Scene::Size();
				}
			}
		}

		if (refreshesLayout)
		{
			canvas->refreshLayout();
		}

		return canvas;
	}
	
	std::shared_ptr<Canvas> Canvas::CreateFromJSONImpl(const JSON& json, detail::WithInstanceIdYN withInstanceId, RefreshesLayoutYN refreshesLayout)
	{
		if (!json.contains(U"size"))
		{
			Logger << U"[NocoUI error] Canvas::CreateFromJSONImpl: Missing required field 'size'";
			return nullptr;
		}
		
		if (!json.contains(U"children"))
		{
			Logger << U"[NocoUI error] Canvas::CreateFromJSONImpl: Missing required field 'children'";
			return nullptr;
		}
		
		std::shared_ptr<Canvas> canvas{ new Canvas{} };
		
		if (auto sizeOpt = StringToValueOpt<SizeF>(json[U"size"].get<String>()))
		{
			canvas->m_size = *sizeOpt;
		}
		else
		{
			Logger << U"[NocoUI warning] Canvas::CreateFromJSONImpl: Failed to parse size, using default";
		}
		
		for (const auto& childJson : json[U"children"].arrayView())
		{
			if (auto child = Node::CreateFromJSONImpl(childJson, withInstanceId))
			{
				canvas->addChild(child, RefreshesLayoutYN::No);
			}
		}

		if (json.contains(U"params"))
		{
			if (json[U"params"].isObject())
			{
				for (const auto& member : json[U"params"])
				{
					const String name = member.key;
					const auto& paramJson = member.value;
					if (auto value = ParamValueFromJSON(paramJson))
					{
						canvas->m_params[name] = *value;
					}
				}
			}
		}

		if (json.contains(U"autoScaleMode"))
		{
			if (const auto modeOpt = StringToValueOpt<AutoScaleMode>(json[U"autoScaleMode"].get<String>()))
			{
				canvas->m_autoScaleMode = *modeOpt;
			}
		}
		if (json.contains(U"autoResizeMode"))
		{
			if (const auto modeOpt = StringToValueOpt<AutoResizeMode>(json[U"autoResizeMode"].get<String>()))
			{
				canvas->m_autoResizeMode = *modeOpt;
				if (canvas->m_autoResizeMode != AutoResizeMode::None)
				{
					canvas->m_lastSceneSize = Scene::Size();
				}
			}
		}

		if (refreshesLayout)
		{
			canvas->refreshLayout();
		}

		return canvas;
	}
	
	bool Canvas::tryReadFromJSON(const JSON& json, RefreshesLayoutYN refreshesLayoutPre, RefreshesLayoutYN refreshesLayoutPost)
	{
		if (!json.contains(U"size"))
		{
			Logger << U"[NocoUI error] Canvas::tryReadFromJSON: Missing required field 'size'";
			return false;
		}
		
		if (!json.contains(U"children"))
		{
			Logger << U"[NocoUI error] Canvas::tryReadFromJSON: Missing required field 'children'";
			return false;
		}
		
		if (auto sizeOpt = StringToValueOpt<SizeF>(json[U"size"].get<String>()))
		{
			m_size = *sizeOpt;
		}
		else
		{
			Logger << U"[NocoUI warning] Canvas::tryReadFromJSON: Failed to parse size";
		}

		for (const auto& child : m_children)
		{
			child->setCanvasRecursive(std::weak_ptr<Canvas>{});
			child->m_parent.reset();
			child->refreshActiveInHierarchy();
		}
		m_children.clear();

		for (const auto& childJson : json[U"children"].arrayView())
		{
			if (auto child = Node::CreateFromJSON(childJson))
			{
				addChild(child, RefreshesLayoutYN::No);
			}
		}

		m_params.clear();
		if (json.contains(U"params"))
		{
			if (json[U"params"].isObject())
			{
				for (const auto& member : json[U"params"])
				{
					const String name = member.key;
					const auto& paramJson = member.value;
					if (auto value = ParamValueFromJSON(paramJson))
					{
						m_params[name] = *value;
					}
				}
			}
		}

		if (json.contains(U"autoScaleMode"))
		{
			if (const auto modeOpt = StringToValueOpt<AutoScaleMode>(json[U"autoScaleMode"].get<String>()))
			{
				m_autoScaleMode = *modeOpt;
			}
		}
		if (json.contains(U"autoResizeMode"))
		{
			if (const auto modeOpt = StringToValueOpt<AutoResizeMode>(json[U"autoResizeMode"].get<String>()))
			{
				m_autoResizeMode = *modeOpt;
				if (m_autoResizeMode != AutoResizeMode::None)
				{
					m_lastSceneSize = Scene::Size();
				}
			}
		}

		if (refreshesLayoutPre)
		{
			refreshLayout();
		}
		for (const auto& child : m_children)
		{
			child->resetScrollOffset(RecursiveYN::Yes, RefreshesLayoutYN::No, RefreshesLayoutYN::No);
		}
		if (refreshesLayoutPost)
		{
			refreshLayout();
		}
		return true;
	}
	
	bool Canvas::tryReadFromJSONImpl(const JSON& json, detail::WithInstanceIdYN withInstanceId, RefreshesLayoutYN refreshesLayoutPre, RefreshesLayoutYN refreshesLayoutPost)
	{
		if (!json.contains(U"size"))
		{
			Logger << U"[NocoUI error] Canvas::tryReadFromJSONImpl: Missing required field 'size'";
			return false;
		}
		
		if (!json.contains(U"children"))
		{
			Logger << U"[NocoUI error] Canvas::tryReadFromJSONImpl: Missing required field 'children'";
			return false;
		}
		
		if (auto sizeOpt = StringToValueOpt<SizeF>(json[U"size"].get<String>()))
		{
			m_size = *sizeOpt;
		}
		else
		{
			Logger << U"[NocoUI warning] Canvas::tryReadFromJSONImpl: Failed to parse size";
		}

		for (const auto& child : m_children)
		{
			child->setCanvasRecursive(std::weak_ptr<Canvas>{});
			child->m_parent.reset();
			child->refreshActiveInHierarchy();
		}
		m_children.clear();

		for (const auto& childJson : json[U"children"].arrayView())
		{
			if (auto child = Node::CreateFromJSONImpl(childJson, withInstanceId))
			{
				addChild(child, RefreshesLayoutYN::No);
			}
		}

		m_params.clear();
		if (json.contains(U"params"))
		{
			if (json[U"params"].isObject())
			{
				for (const auto& member : json[U"params"])
				{
					const String name = member.key;
					const auto& paramJson = member.value;
					if (auto value = ParamValueFromJSON(paramJson))
					{
						m_params[name] = *value;
					}
				}
			}
		}

		if (json.contains(U"autoScaleMode"))
		{
			if (const auto modeOpt = StringToValueOpt<AutoScaleMode>(json[U"autoScaleMode"].get<String>()))
			{
				m_autoScaleMode = *modeOpt;
			}
		}
		if (json.contains(U"autoResizeMode"))
		{
			if (const auto modeOpt = StringToValueOpt<AutoResizeMode>(json[U"autoResizeMode"].get<String>()))
			{
				m_autoResizeMode = *modeOpt;
				if (m_autoResizeMode != AutoResizeMode::None)
				{
					m_lastSceneSize = Scene::Size();
				}
			}
		}

		if (refreshesLayoutPre)
		{
			refreshLayout();
		}
		for (const auto& child : m_children)
		{
			child->resetScrollOffset(RecursiveYN::Yes, RefreshesLayoutYN::No, RefreshesLayoutYN::No);
		}
		if (refreshesLayoutPost)
		{
			refreshLayout();
		}
		return true;
	}
	
	void Canvas::update(HitTestEnabledYN hitTestEnabled)
	{
		m_eventRegistry.clear();

		updateAutoResizeIfNeeded();

		noco::detail::ClearCanvasUpdateContextIfNeeded();

		if (m_children.empty())
		{
			return;
		}
		
		const bool canHover = hitTestEnabled && !CurrentFrame::AnyNodeHovered() && Window::GetState().focused;
		std::shared_ptr<Node> hoveredNode = nullptr;
		if (canHover)
		{
			for (auto it = m_children.rbegin(); it != m_children.rend(); ++it)
			{
				if (auto node = (*it)->hoveredNodeRecursive())
				{
					hoveredNode = node;
					break;
				}
			}
		}
		if (hoveredNode)
		{
			detail::s_canvasUpdateContext.hoveredNode = hoveredNode;
		}

		// スクロール可能なホバー中ノード取得
		auto scrollableHoveredNode = hoveredNode ? hoveredNode->findContainedScrollableNode() : nullptr;
		if (scrollableHoveredNode && !scrollableHoveredNode->hitQuad().mouseOver())
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
				// transformScaleInHierarchyを考慮してドラッグ量を計算(ゼロ除算を防ぐ)
				const Vec2 transformScale = dragScrollingNode->transformScaleInHierarchy();
				const Vec2 screenDragDelta = Cursor::PosF() - *dragScrollingNode->m_dragStartPos;
				const Vec2 dragDelta = Vec2{
					transformScale.x > 0.0 ? screenDragDelta.x / transformScale.x : 0.0,
					transformScale.y > 0.0 ? screenDragDelta.y / transformScale.y : 0.0
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
		
		// addChild等によるイテレータ破壊を避けるためにバッファへ複製してから処理
		m_childrenTempBuffer.clear();
		m_childrenTempBuffer.reserve(m_children.size());
		for (const auto& child : m_children)
		{
			m_childrenTempBuffer.push_back(child);
		}
		
		for (const auto& child : m_childrenTempBuffer)
		{
			child->updateInteractionState(hoveredNode, Scene::DeltaTime(), InteractableYN::Yes, InteractionState::Default, InteractionState::Default, isScrolling);
			child->updateKeyInput();
			child->update(scrollableHoveredNode, Scene::DeltaTime(), rootPosScaleMat(), rootPosScaleMat(), m_params, {});
			child->lateUpdate();
			child->postLateUpdate(Scene::DeltaTime(), m_params);
		}
		
		m_childrenTempBuffer.clear();

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
		
		m_prevDragScrollingWithThresholdExceeded = currentDragScrollingWithThreshold;
	}
	
	std::shared_ptr<Node> Canvas::hitTest(const Vec2& point) const
	{
		for (auto it = m_children.rbegin(); it != m_children.rend(); ++it)
		{
			if (const auto hoveredNode = (*it)->hitTest(point))
			{
				return hoveredNode;
			}
		}
		return nullptr;
	}
	
	void Canvas::draw() const
	{
		for (const auto& child : m_children)
		{
			child->draw();
		}
	}
	
	void Canvas::removeChildrenAll(RefreshesLayoutYN refreshesLayout)
	{
		for (const auto& child : m_children)
		{
			child->setCanvasRecursive(std::weak_ptr<Canvas>{});
			child->m_parent.reset();
			child->refreshActiveInHierarchy();
		}
		m_children.clear();
		if (refreshesLayout)
		{
			refreshLayout();
		}
	}

	void Canvas::clearAll()
	{
		for (const auto& child : m_children)
		{
			child->setCanvasRecursive(std::weak_ptr<Canvas>{});
			child->m_parent.reset();
			child->refreshActiveInHierarchy();
		}
		m_children.clear();
		m_params.clear();
	}

	std::shared_ptr<Canvas> Canvas::setPosition(const Vec2& position)
	{
		m_position = position;
		for (const auto& child : m_children)
		{
			child->refreshTransformMat(RecursiveYN::Yes, rootPosScaleMat(), rootPosScaleMat(), m_params);
		}
		return shared_from_this();
	}
	
	std::shared_ptr<Canvas> Canvas::setScale(const Vec2& scale)
	{
		m_scale = scale;
		for (const auto& child : m_children)
		{
			child->refreshTransformMat(RecursiveYN::Yes, rootPosScaleMat(), rootPosScaleMat(), m_params);
		}
		return shared_from_this();
	}
	
	std::shared_ptr<Canvas> Canvas::setPositionScale(const Vec2& position, const Vec2& scale)
	{
		m_position = position;
		m_scale = scale;
		for (const auto& child : m_children)
		{
			child->refreshTransformMat(RecursiveYN::Yes, rootPosScaleMat(), rootPosScaleMat(), m_params);
		}
		return shared_from_this();
	}

	std::shared_ptr<Canvas> Canvas::setRotation(double rotation)
	{
		m_rotation = rotation;
		for (const auto& child : m_children)
		{
			child->refreshTransformMat(RecursiveYN::Yes, rootPosScaleMat(), rootPosScaleMat(), m_params);
		}
		return shared_from_this();
	}

	std::shared_ptr<Canvas> Canvas::setTransform(const Vec2& position, const Vec2& scale, double rotation)
	{
		m_position = position;
		m_scale = scale;
		m_rotation = rotation;
		for (const auto& child : m_children)
		{
			child->refreshTransformMat(RecursiveYN::Yes, rootPosScaleMat(), rootPosScaleMat(), m_params);
		}
		return shared_from_this();
	}
	
	void Canvas::resetScrollOffsetRecursive(RefreshesLayoutYN refreshesLayout)
	{
		for (const auto& child : m_children)
		{
			child->resetScrollOffset(RecursiveYN::Yes, RefreshesLayoutYN::No, RefreshesLayoutYN::No);
		}
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

	Optional<ParamValue> Canvas::param(const String& name) const
	{
		if (auto it = m_params.find(name); it != m_params.end())
		{
			return it->second;
		}
		return none;
	}

	bool Canvas::hasParam(const String& name) const
	{
		return m_params.contains(name);
	}

	bool Canvas::hasParamOfType(const String& name, ParamType paramType) const
	{
		if (const auto it = m_params.find(name); it != m_params.end())
		{
			return GetParamType(it->second) == paramType;
		}
		return false;
	}

	void Canvas::removeParam(const String& name)
	{
		m_params.erase(name);
	}

	void Canvas::clearParams()
	{
		m_params.clear();
	}

	size_t Canvas::countParamRefs(StringView paramName) const
	{
		size_t count = 0;
		
		std::function<void(std::shared_ptr<Node>)> walkNode = [&](std::shared_ptr<Node> node)
		{
			// Transformコンポーネントのプロパティをチェック
			count += node->transform().countParamRefs(paramName);
			
			// コンポーネントのプロパティをチェック
			for (const auto& component : node->components())
			{
				for (IProperty* property : component->properties())
				{
					if (property->paramRef() == paramName)
					{
						count++;
					}
				}
			}

			for (const auto& child : node->children())
			{
				walkNode(child);
			}
		};

		for (const auto& child : m_children)
		{
			walkNode(child);
		}
		return count;
	}


	void Canvas::clearParamRefs(StringView paramName)
	{
		std::function<void(std::shared_ptr<Node>)> walkNode = [&](std::shared_ptr<Node> node)
		{
			// Transformコンポーネントのプロパティから参照を解除
			node->transform().clearParamRefs(paramName);
			
			// コンポーネントのプロパティから参照を解除
			for (const auto& component : node->components())
			{
				for (IProperty* property : component->properties())
				{
					if (property->paramRef() == paramName)
					{
						property->setParamRef(U"");
					}
				}
			}

			for (const auto& child : node->children())
			{
				walkNode(child);
			}
		};

		for (const auto& child : m_children)
		{
			walkNode(child);
		}
	}

	Array<String> Canvas::removeInvalidParamRefs()
	{
		HashSet<String> clearedParamsSet;
		
		std::function<void(std::shared_ptr<Node>)> walkNode = [&](std::shared_ptr<Node> node)
		{
			// Transformの無効な参照を解除
			const auto clearedFromTransform = node->transform().removeInvalidParamRefs(m_params);
			for (const auto& paramName : clearedFromTransform)
			{
				clearedParamsSet.insert(paramName);
			}
			
			// コンポーネントのプロパティの無効な参照を解除
			for (const auto& component : node->components())
			{
				for (IProperty* property : component->properties())
				{
					property->clearParamRefIfInvalid(m_params, clearedParamsSet);
				}
			}

			for (const auto& child : node->children())
			{
				walkNode(child);
			}
		};

		for (const auto& child : m_children)
		{
			walkNode(child);
		}
		return Array<String>(clearedParamsSet.begin(), clearedParamsSet.end());
	}

	std::shared_ptr<Node> Canvas::childAt(size_t index) const
	{
		if (index >= m_children.size())
		{
			return nullptr;
		}
		return m_children[index];
	}

	const std::shared_ptr<Node>& Canvas::addChild(const std::shared_ptr<Node>& node, RefreshesLayoutYN refreshesLayout)
	{
		if (!node)
		{
			throw Error{ U"Canvas::addChild: node is nullptr" };
		}
		if (node->parentNode() || node->isTopLevelNode())
		{
			throw Error{ U"Canvas::addChild: Child node '{}' already has a parent or is already a top-level node"_fmt(node->name()) };
		}

		node->setCanvasRecursive(shared_from_this());
		node->m_parent.reset();
		node->refreshActiveInHierarchy();
		m_children.push_back(node);

		if (refreshesLayout)
		{
			refreshLayout();
		}

		return m_children.back();
	}

	void Canvas::removeChild(const std::shared_ptr<Node>& node, RefreshesLayoutYN refreshesLayout)
	{
		if (!node)
		{
			return;
		}

		m_children.remove(node);
		node->m_parent.reset();
		
		node->setCanvasRecursive(std::weak_ptr<Canvas>{});
		node->refreshActiveInHierarchy();

		if (refreshesLayout)
		{
			refreshLayout();
		}
	}
	
	void Canvas::swapChildren(size_t index1, size_t index2, RefreshesLayoutYN refreshesLayout)
	{
		if (index1 >= m_children.size() || index2 >= m_children.size())
		{
			return;
		}

		std::swap(m_children[index1], m_children[index2]);

		if (refreshesLayout)
		{
			refreshLayout();
		}
	}
	
	const std::shared_ptr<Node>& Canvas::emplaceChild(
		StringView name,
		const RegionVariant& region,
		IsHitTargetYN isHitTarget,
		InheritChildrenStateFlags inheritChildrenStateFlags,
		RefreshesLayoutYN refreshesLayout)
	{
		auto child = Node::Create(name, region, isHitTarget, inheritChildrenStateFlags);
		child->setCanvasRecursive(shared_from_this());
		child->m_parent.reset();
		child->refreshActiveInHierarchy();
		m_children.push_back(std::move(child));
		if (refreshesLayout)
		{
			refreshLayout();
		}
		return m_children.back();
	}

	const std::shared_ptr<Node>& Canvas::addChildFromJSON(const JSON& json, RefreshesLayoutYN refreshesLayout)
	{
		auto child = Node::CreateFromJSON(json);
		child->setCanvasRecursive(shared_from_this());
		child->m_parent.reset();
		child->refreshActiveInHierarchy();
		m_children.push_back(std::move(child));
		if (refreshesLayout)
		{
			refreshLayout();
		}
		return m_children.back();
	}

	const std::shared_ptr<Node>& Canvas::addChildAtIndex(const std::shared_ptr<Node>& child, size_t index, RefreshesLayoutYN refreshesLayout)
	{
		if (!child)
		{
			throw Error{ U"Canvas::addChildAtIndex: node is nullptr" };
		}
		if (child->parentNode() || child->isTopLevelNode())
		{
			throw Error{ U"Canvas::addChildAtIndex: Child node '{}' already has a parent or is already a top-level node"_fmt(child->name()) };
		}

		if (index > m_children.size())
		{
			index = m_children.size();
		}

		child->setCanvasRecursive(shared_from_this());
		child->m_parent.reset();
		child->refreshActiveInHierarchy();
		m_children.insert(m_children.begin() + index, child);

		if (refreshesLayout)
		{
			refreshLayout();
		}

		return m_children[index];
	}

	bool Canvas::containsChild(const std::shared_ptr<Node>& child, RecursiveYN recursive) const
	{
		if (m_children.contains(child))
		{
			return true;
		}
		if (recursive)
		{
			for (const auto& myChild : m_children)
			{
				if (myChild->containsChild(child, recursive))
				{
					return true;
				}
			}
		}
		return false;
	}

	bool Canvas::containsChildByName(StringView name, RecursiveYN recursive) const
	{
		for (const auto& child : m_children)
		{
			if (child->name() == name)
			{
				return true;
			}
			if (recursive && child->containsChildByName(name, recursive))
			{
				return true;
			}
		}
		return false;
	}

	std::shared_ptr<Node> Canvas::getChildByNameOrNull(StringView name, RecursiveYN recursive)
	{
		for (const auto& child : m_children)
		{
			if (child->name() == name)
			{
				return child;
			}
			if (recursive)
			{
				if (auto found = child->getChildByNameOrNull(name, recursive))
				{
					return found;
				}
			}
		}
		return nullptr;
	}

	Optional<size_t> Canvas::indexOfChildOpt(const std::shared_ptr<Node>& child) const
	{
		const auto it = std::find(m_children.begin(), m_children.end(), child);
		if (it == m_children.end())
		{
			return none;
		}
		return static_cast<size_t>(std::distance(m_children.begin(), it));
	}

	void Canvas::replaceParamRefs(const String& oldName, const String& newName)
	{
		for (const auto& child : m_children)
		{
			child->replaceParamRefs(oldName, newName, RecursiveYN::Yes);
		}
	}

	std::shared_ptr<Node> Canvas::findNodeByInstanceId(uint64 instanceId) const
	{
		for (const auto& child : m_children)
		{
			if (auto result = findNodeByInstanceIdRecursive(child, instanceId))
			{
				return result;
			}
		}
		return nullptr;
	}

	std::shared_ptr<Node> Canvas::findNodeByInstanceIdRecursive(const std::shared_ptr<Node>& node, uint64 instanceId) const
	{
		if (!node)
		{
			return nullptr;
		}
		
		if (node->instanceId() == instanceId)
		{
			return node;
		}
		
		for (const auto& child : node->children())
		{
			if (auto result = findNodeByInstanceIdRecursive(child, instanceId))
			{
				return result;
			}
		}
		
		return nullptr;
	}

	Quad Canvas::quad() const
	{
		const RectF rect{ 0, 0, m_size.x, m_size.y };
		const Mat3x2 transform = rootPosScaleMat();
		
		const Vec2 topLeft = transform.transformPoint(rect.tl());
		const Vec2 topRight = transform.transformPoint(rect.tr());
		const Vec2 bottomLeft = transform.transformPoint(rect.bl());
		const Vec2 bottomRight = transform.transformPoint(rect.br());
		
		return Quad{ topLeft, topRight, bottomRight, bottomLeft };
	}

	std::shared_ptr<Canvas> Canvas::setCenter(const Vec2& center)
	{
		m_position = center - Vec2{ m_size.x / 2, m_size.y / 2 };
		return shared_from_this();
	}

	Vec2 Canvas::center() const
	{
		return m_position + Vec2{ m_size.x / 2, m_size.y / 2 };
	}

	std::shared_ptr<Canvas> Canvas::setAutoScaleMode(AutoScaleMode mode)
	{
		m_autoScaleMode = mode;
		if (mode != AutoScaleMode::None)
		{
			m_position = Vec2::Zero();
		}
		return shared_from_this();
	}

	std::shared_ptr<Canvas> Canvas::setAutoResizeMode(AutoResizeMode mode)
	{
		m_autoResizeMode = mode;
		
		if (mode == AutoResizeMode::None)
		{
			m_lastSceneSize = none;
		}
		else
		{
			m_lastSceneSize = Scene::Size();
			m_position = Vec2::Zero();
		}
		
		return shared_from_this();
	}

	std::shared_ptr<Canvas> Canvas::setEditorPreviewInternal(bool isEditorPreview)
	{
		m_isEditorPreview = isEditorPreview;
		return shared_from_this();
	}
}
