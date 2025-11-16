#include "NocoUI/Canvas.hpp"
#include "NocoUI/Init.hpp"
#include "NocoUI/ComponentFactory.hpp"
#include "NocoUI/ParamUtils.hpp"
#include "NocoUI/Serialization.hpp"
#include "NocoUI/Version.hpp"
#include "NocoUI/Component/IFontCachedComponent.hpp"
#include "NocoUI/Component/SubCanvas.hpp"
#include <cassert>

namespace noco
{
	class Node;
	
	namespace
	{
		void SortByZOrderInSiblings(Array<std::shared_ptr<Node>>& nodes)
		{
			if (nodes.size() <= 1)
			{
				return;
			}
			std::stable_sort(nodes.begin(), nodes.end(),
				[](const std::shared_ptr<Node>& a, const std::shared_ptr<Node>& b)
				{
					return a->zOrderInSiblings() < b->zOrderInSiblings();
				});
		}
	}
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

	void Canvas::updateAutoFitIfNeeded(const SizeF& sceneSize, bool force)
	{
		if (m_autoFitMode == AutoFitMode::None || m_isEditorPreview)
		{
			m_lastAutoFitSceneSize = sceneSize;
			return;
		}

		// シーンサイズが変わっていなければ処理不要(forceの場合は強制実行)
		if (!force && m_lastAutoFitSceneSize == sceneSize)
		{
			return;
		}
		
		// ゼロ除算を防ぐ
		if (m_referenceSize.x <= 0 || m_referenceSize.y <= 0)
		{
			m_lastAutoFitSceneSize = sceneSize;
			return;
		}
		
		const SizeF oldSize = m_size;
		const double scaleX = sceneSize.x / m_referenceSize.x;
		const double scaleY = sceneSize.y / m_referenceSize.y;
		switch (m_autoFitMode)
		{
			case AutoFitMode::Contain:
			{
				const double scale = Min(scaleX, scaleY);
				m_scale = Vec2{ scale, scale };
				m_size = m_referenceSize;
				const Vec2 scaledSize = m_size * scale;
				m_position = (sceneSize - scaledSize) / 2;
				break;
			}
			
			case AutoFitMode::Cover:
			{
				const double scale = Max(scaleX, scaleY);
				m_scale = Vec2{ scale, scale };
				m_size = m_referenceSize;
				const Vec2 scaledSize = m_size * scale;
				m_position = (sceneSize - scaledSize) / 2;
				break;
			}
			
			case AutoFitMode::FitWidth:
			{
				m_scale = Vec2{ scaleX, scaleX };
				m_size = m_referenceSize;
				const Vec2 scaledSize = m_size * scaleX;
				m_position = (sceneSize - scaledSize) / 2;
				break;
			}
			
			case AutoFitMode::FitHeight:
			{
				m_scale = Vec2{ scaleY, scaleY };
				m_size = m_referenceSize;
				const Vec2 scaledSize = m_size * scaleY;
				m_position = (sceneSize - scaledSize) / 2;
				break;
			}
			
			case AutoFitMode::FitWidthMatchHeight:
			{
				m_scale = Vec2{ scaleX, scaleX };
				m_size = SizeF{ m_referenceSize.x, scaleX > 0 ? sceneSize.y / scaleX : m_referenceSize.y };
				m_position = Vec2{ (sceneSize.x - m_size.x * scaleX) / 2, 0.0 };
				break;
			}
			
			case AutoFitMode::FitHeightMatchWidth:
			{
				m_scale = Vec2{ scaleY, scaleY };
				m_size = SizeF{ scaleY > 0 ? sceneSize.x / scaleY : m_referenceSize.x, m_referenceSize.y };
				m_position = Vec2{ 0.0, (sceneSize.y - m_size.y * scaleY) / 2 };
				break;
			}
			
			case AutoFitMode::MatchSize:
			{
				m_scale = Vec2::One();
				m_size = sceneSize;
				m_position = Vec2::Zero();
				break;
			}
			
			default:
				break;
		}
		
		// シーンサイズが変わったので必ず変換行列を更新
		for (const auto& child : m_children)
		{
			child->refreshTransformMat(RecursiveYN::Yes, rootPosScaleMat(), rootPosScaleMat(), m_params);
		}
		
		// サイズが変更された場合はレイアウトを再計算
		if (oldSize != m_size)
		{
			refreshLayoutImmediately(OnlyIfDirtyYN::No);
		}
		
		m_lastAutoFitSceneSize = sceneSize;
	}

	Mat3x2 Canvas::rootPosScaleMat() const
	{
		if (m_scale == Vec2::One() && m_position == Vec2::Zero() && m_rotation == 0.0)
		{
			return Mat3x2::Identity();
		}
		return Mat3x2::Scale(m_scale) * Mat3x2::Rotate(m_rotation) * Mat3x2::Translate(m_position);
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

	Canvas::Canvas()
	{
		// noco::Init()が呼ばれていない場合は例外を投げる
		if (!noco::IsInitialized())
		{
			throw Error{ U"Canvas::Canvas: NocoUI is not initialized. Call noco::Init() first." };
		}
	}

	Canvas::~Canvas()
	{
		for (const auto& child : m_children)
		{
			child->setCanvasRecursive(std::weak_ptr<Canvas>{});
			child->refreshActiveInHierarchy();
		}
	}

	std::shared_ptr<Canvas> Canvas::Create(const SizeF& referenceSize)
	{
		std::shared_ptr<Canvas> canvas{ new Canvas{} };
		canvas->m_size = referenceSize;
		canvas->m_referenceSize = referenceSize;
		return canvas;
	}
	
	std::shared_ptr<Canvas> Canvas::Create(double width, double height)
	{
		return Create(SizeF{ width, height });
	}

	void Canvas::refreshLayoutImmediately(OnlyIfDirtyYN onlyIfDirty)
	{
		if (onlyIfDirty && !m_isLayoutDirty)
		{
			return;
		}
		m_isLayoutDirty = false;

		const RectF canvasRect{ 0, 0, m_size.x, m_size.y };
		
		// Canvasのm_childrenLayoutを適用
		std::visit([this, &canvasRect](const auto& layout)
			{
				layout.execute(canvasRect, m_children, [](const std::shared_ptr<Node>& child, const RectF& rect)
					{
						child->m_regionRect = rect;
					});
			}, m_childrenLayout);
		
		for (const auto& child : m_children)
		{
			child->refreshChildrenLayout();
			child->refreshTransformMat(RecursiveYN::Yes, rootPosScaleMat(), rootPosScaleMat(), m_params);
		}
	}
	
	std::shared_ptr<Node> Canvas::findByName(StringView nodeName, RecursiveYN recursive) const
	{
		for (const auto& child : m_children)
		{
			if (child->name() == nodeName)
			{
				return child;
			}
			if (recursive)
			{
				if (auto found = child->findByName(nodeName, RecursiveYN::Yes))
				{
					return found;
				}
			}
		}
		return nullptr;
	}
	
	JSON Canvas::toJSON(detail::WithInstanceIdYN withInstanceId) const
	{
		JSON json = JSON
		{
			{ U"version", NocoUIVersion },
			{ U"serializedVersion", CurrentSerializedVersion },
			{ U"referenceSize", ToArrayJSON(m_referenceSize) },
		};

		if (m_autoFitMode != AutoFitMode::None)
		{
			json[U"autoFitMode"] = ValueToString(m_autoFitMode);
		}

		// childrenLayoutの保存
		json[U"childrenLayout"] = std::visit([](const auto& childrenLayout) { return childrenLayout.toJSON(); }, m_childrenLayout);

		Array<JSON> childrenArray;
		for (const auto& child : m_children)
		{
			childrenArray.push_back(child->toJSON(withInstanceId));
		}
		json[U"children"] = childrenArray;

		if (!m_params.empty())
		{
			JSON paramObjectDictJSON = JSON{};
			for (const auto& [name, value] : m_params)
			{
				if (!IsValidParameterName(name))
				{
					Logger << U"[NocoUI warning] Invalid parameter name '{}' found during save. Skipping."_fmt(name);
					continue;
				}
				paramObjectDictJSON[name] = ParamValueToParamObjectJSON(value);
			}
			json[U"params"] = paramObjectDictJSON;
		}

		json[U"defaultFontAssetName"] = m_defaultFontAssetName;

		return json;
	}
	
	std::shared_ptr<Canvas> Canvas::CreateFromJSON(const JSON& json, detail::WithInstanceIdYN withInstanceId)
	{
		return CreateFromJSON(json, ComponentFactory::GetBuiltinFactory(), withInstanceId);
	}
	
	std::shared_ptr<Canvas> Canvas::CreateFromJSON(const JSON& json, const ComponentFactory& componentFactory, detail::WithInstanceIdYN withInstanceId)
	{
		if (!json.contains(U"version"))
		{
			Logger << U"[NocoUI error] Canvas::CreateFromJSON: Missing required field 'version'";
			return nullptr;
		}
		
		if (!json.contains(U"serializedVersion"))
		{
			Logger << U"[NocoUI error] Canvas::CreateFromJSON: Missing required field 'serializedVersion'";
			return nullptr;
		}
		
		if (!json.contains(U"referenceSize"))
		{
			Logger << U"[NocoUI error] Canvas::CreateFromJSON: Missing required field 'referenceSize'";
			return nullptr;
		}
		
		if (!json.contains(U"children"))
		{
			Logger << U"[NocoUI error] Canvas::CreateFromJSON: Missing required field 'children'";
			return nullptr;
		}
		
		std::shared_ptr<Canvas> canvas{ new Canvas{} };
		
		// serializedVersionが将来のバージョンの場合は警告を出す
		canvas->m_serializedVersion = json[U"serializedVersion"].get<int32>();
		if (canvas->m_serializedVersion > CurrentSerializedVersion)
		{
			const String version = json[U"version"].get<String>();
			Logger << U"[NocoUI warning] Opening file created with newer NocoUI version (version: {}, serializedVersion: {}). Current serializedVersion: {}"_fmt(
				version, canvas->m_serializedVersion, CurrentSerializedVersion);
		}
		
		// referenceSizeの読み込み（必須フィールド）
		const SizeF referenceSize = FromArrayJSON<Vec2>(json[U"referenceSize"], SizeF{ 800, 600 });
		canvas->m_referenceSize = referenceSize;
		canvas->m_size = referenceSize;  // 初期サイズとしても使用
		
		if (json.contains(U"childrenLayout") && json[U"childrenLayout"].contains(U"type"))
		{
			const auto type = json[U"childrenLayout"][U"type"].getString();
			if (type == U"FlowLayout")
			{
				canvas->m_childrenLayout = FlowLayout::FromJSON(json[U"childrenLayout"]);
			}
			else if (type == U"HorizontalLayout")
			{
				canvas->m_childrenLayout = HorizontalLayout::FromJSON(json[U"childrenLayout"]);
			}
			else if (type == U"VerticalLayout")
			{
				canvas->m_childrenLayout = VerticalLayout::FromJSON(json[U"childrenLayout"]);
			}
			else
			{
				Logger << U"[NocoUI warning] Unknown childrenLayout type: {}, using default FlowLayout"_fmt(type);
				canvas->m_childrenLayout = FlowLayout{};
			}
		}
		
		for (const auto& childJson : json[U"children"].arrayView())
		{
			if (auto child = Node::CreateFromJSON(childJson, componentFactory, withInstanceId))
			{
				canvas->addChild(child);
			}
		}

		if (json.contains(U"params"))
		{
			if (json[U"params"].isObject())
			{
				for (const auto& member : json[U"params"])
				{
					const String name = member.key;
					if (!IsValidParameterName(name))
					{
						Logger << U"[NocoUI warning] Invalid parameter name '{}' found in JSON. Skipping."_fmt(name);
						continue;
					}
					const auto& paramObjectJSON = member.value;
					if (auto value = ParamValueFromParamObjectJSON(paramObjectJSON))
					{
						canvas->m_params[name] = *value;
					}
				}
			}
		}

		if (json.contains(U"autoFitMode"))
		{
			if (const auto modeOpt = StringToValueOpt<AutoFitMode>(json[U"autoFitMode"].get<String>()))
			{
				canvas->m_autoFitMode = *modeOpt;
			}
		}

		if (json.contains(U"defaultFontAssetName"))
		{
			canvas->m_defaultFontAssetName = json[U"defaultFontAssetName"].get<String>();
		}

		canvas->markLayoutAsDirty();

		return canvas;
	}

	std::shared_ptr<Canvas> Canvas::LoadFromFile(FilePathView path, AllowExceptions allowExceptions)
	{
		const JSON json = JSON::Load(path, allowExceptions);
		if (!json)
		{
			return nullptr;
		}
		return Canvas::CreateFromJSON(json);
	}

	std::shared_ptr<Canvas> Canvas::LoadFromFile(FilePathView path, const ComponentFactory& componentFactory, AllowExceptions allowExceptions)
	{
		const JSON json = JSON::Load(path, allowExceptions);
		if (!json)
		{
			return nullptr;
		}
		return Canvas::CreateFromJSON(json, componentFactory);
	}
	
	bool Canvas::tryReadFromJSON(const JSON& json, detail::WithInstanceIdYN withInstanceId)
	{
		return tryReadFromJSON(json, ComponentFactory::GetBuiltinFactory(), withInstanceId);
	}
	
	bool Canvas::tryReadFromJSON(const JSON& json, const ComponentFactory& componentFactory, detail::WithInstanceIdYN withInstanceId)
	{
		if (!json.contains(U"referenceSize"))
		{
			Logger << U"[NocoUI error] Canvas::tryReadFromJSON: Missing required field 'referenceSize'";
			return false;
		}
		
		if (!json.contains(U"children"))
		{
			Logger << U"[NocoUI error] Canvas::tryReadFromJSON: Missing required field 'children'";
			return false;
		}
		
		// referenceSizeの読み込み（必須フィールド）
		const SizeF referenceSize = FromArrayJSON<Vec2>(json[U"referenceSize"], SizeF{ 800, 600 });
		m_referenceSize = referenceSize;
		m_size = referenceSize;  // 初期サイズとしても使用

		for (const auto& child : m_children)
		{
			child->setCanvasRecursive(std::weak_ptr<Canvas>{});
			child->m_parent.reset();
			child->refreshActiveInHierarchy();
		}
		m_children.clear();

		for (const auto& childJson : json[U"children"].arrayView())
		{
			if (auto child = Node::CreateFromJSON(childJson, componentFactory, withInstanceId))
			{
				addChild(child);
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
					if (!IsValidParameterName(name))
					{
						Logger << U"[NocoUI warning] Invalid parameter name '{}' found in JSON. Skipping."_fmt(name);
						continue;
					}
					const auto& paramObjectJSON = member.value;
					if (auto value = ParamValueFromParamObjectJSON(paramObjectJSON))
					{
						m_params[name] = *value;
					}
				}
			}
		}

		if (json.contains(U"autoFitMode"))
		{
			if (const auto modeOpt = StringToValueOpt<AutoFitMode>(json[U"autoFitMode"].get<String>()))
			{
				m_autoFitMode = *modeOpt;
			}
		}

		if (json.contains(U"defaultFontAssetName"))
		{
			m_defaultFontAssetName = json[U"defaultFontAssetName"].get<String>();
		}
		else
		{
			m_defaultFontAssetName = U"";
		}

		// childrenLayoutの読み込み
		if (json.contains(U"childrenLayout") && json[U"childrenLayout"].contains(U"type"))
		{
			const auto type = json[U"childrenLayout"][U"type"].getString();
			if (type == U"FlowLayout")
			{
				m_childrenLayout = FlowLayout::FromJSON(json[U"childrenLayout"]);
			}
			else if (type == U"HorizontalLayout")
			{
				m_childrenLayout = HorizontalLayout::FromJSON(json[U"childrenLayout"]);
			}
			else if (type == U"VerticalLayout")
			{
				m_childrenLayout = VerticalLayout::FromJSON(json[U"childrenLayout"]);
			}
			else
			{
				// 不明な場合はFlowLayout扱いにする
				Logger << U"[NocoUI warning] Unknown children layout type: '{}'"_fmt(type);
				m_childrenLayout = FlowLayout{};
			}
		}

		markLayoutAsDirty();
		
		for (const auto& child : m_children)
		{
			child->resetScrollOffset(RecursiveYN::Yes);
		}
		return true;
	}
	
	void Canvas::update(HitTestEnabledYN hitTestEnabled)
	{
		update(Scene::Size(), hitTestEnabled);
	}

	void Canvas::update(const SizeF& sceneSize, HitTestEnabledYN hitTestEnabled)
	{
		static const Array<String> EmptyStringArray{};

		m_eventRegistry.clear();

		noco::detail::ClearCanvasUpdateContextIfNeeded();

		if (m_children.empty())
		{
			return;
		}
		
		const bool canHover = hitTestEnabled && !CurrentFrame::AnyNodeHovered() && Window::GetState().focused;
		std::shared_ptr<Node> hoveredNode = nullptr;
		if (canHover)
		{
			// hoveredNodeを決める時点では今回フレームのzOrderInSiblingsのステート毎の値が確定しないため、前回フレームのzOrderInSiblingsがあれば使用する
			// (siblingIndexにHovered等の値を設定した場合の挙動用)
			// なお、ライブラリユーザーがCanvasのupdate呼び出しの手前でパラメータやsetZOrderInSiblings等を経由してzOrderInSiblingsを変更した場合であっても、hoveredNode決定用のヒットテストに対しては次フレームからの反映となる。これは正常動作。
			hoveredNode = hitTest(Cursor::PosF(), detail::UsePrevZOrderInSiblingsYN::Yes);
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
			dragScrollingNode->m_dragThresholdExceeded = false;
			detail::s_canvasUpdateContext.dragScrollingNode.reset();
			// 慣性スクロールは各ノードのupdateで処理される
		}

		// ノード更新
		const bool currentDragScrollingWithThreshold = dragScrollingNode && dragScrollingNode->m_dragThresholdExceeded;
		const IsScrollingYN isScrolling{ currentDragScrollingWithThreshold || m_prevDragScrollingWithThresholdExceeded };

		// updateNodeParamsは順不同かつユーザーコードを含まないためm_childrenに対して直接実行
		for (const auto& child : m_children)
		{
			child->updateNodeParams(m_params);
		}
		// パラメータによるactiveSelf変更でレイアウトが変わる場合のためにここでも更新
		refreshLayoutImmediately(OnlyIfDirtyYN::Yes);
		
		// ステート(interactionStateとstyleState)を確定(deltaTimeはここではなくlateUpdate後の呼び出しで適用)
		// updateNodeStatesは順不同かつユーザーコードを含まないためm_childrenに対して直接実行
		for (const auto& child : m_children)
		{
			child->updateNodeStates(detail::UpdateInteractionStateYN::Yes, hoveredNode, 0.0, m_interactable, InteractionState::Default, InteractionState::Default, isScrolling, m_params, EmptyStringArray);
		}

		// updateKeyInput・update・lateUpdate中のaddChild等によるイテレータ破壊を避けるためにバッファへ複製してから処理
		m_tempChildrenBuffer.clear();
		m_tempChildrenBuffer.reserve(m_children.size());
		m_tempChildrenBuffer.assign(m_children.begin(), m_children.end());

		// updateKeyInputはzOrder降順で実行(手前から奥へ)
		// ユーザーコード内でのaddChild等の呼び出しでイテレータ破壊が起きないよう、ここでは一時バッファの使用が必須
		SortByZOrderInSiblings(m_tempChildrenBuffer); // zOrderInSiblingsはステート毎の値を持つためupdateNodeStatesより後にソートする必要がある点に注意
		for (auto it = m_tempChildrenBuffer.rbegin(); it != m_tempChildrenBuffer.rend(); ++it)
		{
			(*it)->updateKeyInput();
		}
		
		// update・lateUpdate・postLateUpdateはzOrderに関係なく順番に実行(そのため元の順番で上書きが必要)
		// ユーザーコード内でのaddChild等の呼び出しでイテレータ破壊が起きないよう、ここでは一時バッファの使用が必須
		const Mat3x2 rootMat = rootPosScaleMat();
		m_tempChildrenBuffer.assign(m_children.begin(), m_children.end());
		for (const auto& child : m_tempChildrenBuffer)
		{
			child->update(scrollableHoveredNode, Scene::DeltaTime(), rootMat, rootMat, m_params);
		}
		for (const auto& child : m_tempChildrenBuffer)
		{
			child->lateUpdate();
		}

		// update内でstyleStateがsetCurrentFrameOverrideで上書きされた場合用にステート更新はlateUpdate後に改めて実行(deltaTime適用)
		// updateNodeStatesは順不同かつユーザーコードを含まないためm_childrenに対して直接実行
		for (const auto& child : m_children)
		{
			// InteractionStateはライフサイクルの途中では変えないためNoを指定
			child->updateNodeStates(detail::UpdateInteractionStateYN::No, hoveredNode, Scene::DeltaTime(), m_interactable, InteractionState::Default, InteractionState::Default, isScrolling, m_params, EmptyStringArray);
		}

		for (const auto& child : m_tempChildrenBuffer)
		{
			child->postLateUpdate(Scene::DeltaTime(), rootMat, rootMat, m_params);
		}

		// AutoFitModeによるサイズ・スケールの更新
		updateAutoFitIfNeeded(sceneSize);

		// 同一フレーム内でのレイアウト更新はまとめて1回遅延実行
		refreshLayoutImmediately(OnlyIfDirtyYN::Yes);
		
		m_tempChildrenBuffer.clear();

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
	
	std::shared_ptr<Node> Canvas::hitTest(const Vec2& point, detail::UsePrevZOrderInSiblingsYN usePrevZOrderInSiblings) const
	{
		// hitTestはzOrder降順で実行(手前から奥へ)
		m_tempChildrenBuffer.clear();
		m_tempChildrenBuffer.reserve(m_children.size());
		m_tempChildrenBuffer.assign(m_children.begin(), m_children.end());
		SortByZOrderInSiblings(m_tempChildrenBuffer);
		for (auto it = m_tempChildrenBuffer.rbegin(); it != m_tempChildrenBuffer.rend(); ++it)
		{
			if (const auto hoveredNode = (*it)->hitTest(point, usePrevZOrderInSiblings))
			{
				m_tempChildrenBuffer.clear();
				return hoveredNode;
			}
		}
		m_tempChildrenBuffer.clear();
		return nullptr;
	}
	
	void Canvas::draw() const
	{
		// drawはzOrder昇順で実行(奥から手前へ)
		// ユーザーコード内でのaddChild等の呼び出しでイテレータ破壊が起きないよう、ここでは一時バッファの使用が必須
		m_tempChildrenBuffer.clear();
		m_tempChildrenBuffer.reserve(m_children.size());
		m_tempChildrenBuffer.assign(m_children.begin(), m_children.end());
		SortByZOrderInSiblings(m_tempChildrenBuffer);
		for (const auto& child : m_tempChildrenBuffer)
		{
			child->draw();
		}
		m_tempChildrenBuffer.clear();
	}
	
	void Canvas::removeChildrenAll()
	{
		for (const auto& child : m_children)
		{
			child->setCanvasRecursive(std::weak_ptr<Canvas>{});
			child->m_parent.reset();
			child->refreshActiveInHierarchy();
		}
		m_children.clear();
		markLayoutAsDirty();
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
		if (m_autoFitMode != AutoFitMode::None && !m_isEditorPreview)
		{
			// AutoFitModeが有効な間はsetPositionは無視
			return shared_from_this();
		}
		
		m_position = position;
		for (const auto& child : m_children)
		{
			child->refreshTransformMat(RecursiveYN::Yes, rootPosScaleMat(), rootPosScaleMat(), m_params);
		}
		return shared_from_this();
	}
	
	std::shared_ptr<Canvas> Canvas::setScale(const Vec2& scale)
	{
		if (m_autoFitMode != AutoFitMode::None && !m_isEditorPreview)
		{
			// AutoFitModeが有効な間はsetScaleは無視
			return shared_from_this();
		}
		
		m_scale = scale;
		for (const auto& child : m_children)
		{
			child->refreshTransformMat(RecursiveYN::Yes, rootPosScaleMat(), rootPosScaleMat(), m_params);
		}
		return shared_from_this();
	}
	
	std::shared_ptr<Canvas> Canvas::setPositionScale(const Vec2& position, const Vec2& scale)
	{
		if (m_autoFitMode != AutoFitMode::None && !m_isEditorPreview)
		{
			// AutoFitModeが有効な間はsetPositionScaleは無視
			return shared_from_this();
		}
		
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
	
	void Canvas::resetScrollOffsetRecursive()
	{
		for (const auto& child : m_children)
		{
			child->resetScrollOffset(RecursiveYN::Yes);
		}
		markLayoutAsDirty();
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

	void Canvas::setParamsByJSON(const JSON& json)
	{
		if (!json.isObject())
		{
			return;
		}

		for (const auto& [key, value] : json)
		{
			// 既存のパラメータが存在する場合、その型を使って値を変換
			if (auto it = m_params.find(key); it != m_params.end())
			{
				const ParamType type = GetParamType(it->second);
				if (auto paramValue = ParamValueFromJSONValue(value, type))
				{
					m_params[key] = *paramValue;
				}
			}
		}
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
		if (paramName.isEmpty())
		{
			return 0;
		}
		
		size_t count = 0;
		
		std::function<void(std::shared_ptr<Node>)> walkNode = [&](std::shared_ptr<Node> node)
		{
			// Transformコンポーネントのプロパティをチェック
			count += node->transform().countParamRefs(paramName);
			
			// Node自体のプロパティをチェック
			if (node->activeSelfParamRef() == paramName)
			{
				count++;
			}
			if (node->interactableParamRef() == paramName)
			{
				count++;
			}
			if (node->styleStateParamRef() == paramName)
			{
				count++;
			}
			if (node->zOrderInSiblingsParamRef() == paramName)
			{
				count++;
			}
			
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
		if (paramName.isEmpty())
		{
			return;
		}
		
		std::function<void(std::shared_ptr<Node>)> walkNode = [&](std::shared_ptr<Node> node)
		{
			// Transformコンポーネントのプロパティから参照を解除
			node->transform().clearParamRefs(paramName);
			
			// Node自体のプロパティから参照を解除
			if (node->activeSelfParamRef() == paramName)
			{
				node->setActiveSelfParamRef(U"");
			}
			if (node->interactableParamRef() == paramName)
			{
				node->setInteractableParamRef(U"");
			}
			if (node->styleStateParamRef() == paramName)
			{
				node->setStyleStateParamRef(U"");
			}
			if (node->zOrderInSiblingsParamRef() == paramName)
			{
				node->setZOrderInSiblingsParamRef(U"");
			}
			
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
			
			// Node自体のプロパティの無効な参照を解除
			node->activeSelfProperty().clearParamRefIfInvalid(m_params, clearedParamsSet);
			node->interactableProperty().clearParamRefIfInvalid(m_params, clearedParamsSet);
			node->styleStateProperty().clearParamRefIfInvalid(m_params, clearedParamsSet);
			node->zOrderInSiblingsProperty().clearParamRefIfInvalid(m_params, clearedParamsSet);
			
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

	const std::shared_ptr<Node>& Canvas::addChild(const std::shared_ptr<Node>& node)
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

		markLayoutAsDirty();

		return m_children.back();
	}

	void Canvas::removeChild(const std::shared_ptr<Node>& node)
	{
		if (!node)
		{
			return;
		}

		m_children.remove(node);
		node->m_parent.reset();
		
		node->setCanvasRecursive(std::weak_ptr<Canvas>{});
		node->refreshActiveInHierarchy();

		markLayoutAsDirty();
	}
	
	void Canvas::swapChildren(size_t index1, size_t index2)
	{
		if (index1 >= m_children.size() || index2 >= m_children.size())
		{
			return;
		}

		std::swap(m_children[index1], m_children[index2]);

		markLayoutAsDirty();
	}
	
	const std::shared_ptr<Node>& Canvas::emplaceChild(
		StringView name,
		const RegionVariant& region,
		IsHitTargetYN isHitTarget,
		InheritChildrenStateFlags inheritChildrenStateFlags)
	{
		auto child = Node::Create(name, region, isHitTarget, inheritChildrenStateFlags);
		child->setCanvasRecursive(shared_from_this());
		child->m_parent.reset();
		child->refreshActiveInHierarchy();
		m_children.push_back(std::move(child));
		markLayoutAsDirty();
		return m_children.back();
	}

	const std::shared_ptr<Node>& Canvas::addChildFromJSON(const JSON& json)
	{
		auto child = Node::CreateFromJSON(json);
		child->setCanvasRecursive(shared_from_this());
		child->m_parent.reset();
		child->refreshActiveInHierarchy();
		m_children.push_back(std::move(child));
		markLayoutAsDirty();
		return m_children.back();
	}
	
	const std::shared_ptr<Node>& Canvas::addChildFromJSON(const JSON& json, const ComponentFactory& factory)
	{
		auto child = Node::CreateFromJSON(json, factory);
		child->setCanvasRecursive(shared_from_this());
		child->m_parent.reset();
		child->refreshActiveInHierarchy();
		m_children.push_back(std::move(child));
		markLayoutAsDirty();
		return m_children.back();
	}

	const std::shared_ptr<Node>& Canvas::addChildAtIndex(const std::shared_ptr<Node>& child, size_t index)
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

		markLayoutAsDirty();

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

	std::shared_ptr<Node> Canvas::findByName(StringView name, RecursiveYN recursive)
	{
		for (const auto& child : m_children)
		{
			if (child->name() == name)
			{
				return child;
			}
			if (recursive)
			{
				if (auto found = child->findByName(name, recursive))
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

	void Canvas::clearCurrentFrameOverride()
	{
		for (auto& child : m_children)
		{
			child->clearCurrentFrameOverride();
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

	std::shared_ptr<SubCanvas> Canvas::getSubCanvasByTag(StringView tag) const
	{
		if (tag.isEmpty())
		{
			return nullptr;
		}

		for (const auto& child : m_children)
		{
			for (const auto& component : child->components())
			{
				if (auto subCanvas = std::dynamic_pointer_cast<SubCanvas>(component))
				{
					if (subCanvas->tag() == tag)
					{
						return subCanvas;
					}
				}
			}

			for (const auto& grandChild : child->children())
			{
				if (auto canvas = grandChild->containedCanvas())
				{
					if (auto found = canvas->getSubCanvasByTag(tag))
					{
						return found;
					}
				}
			}
		}

		return nullptr;
	}

	void Canvas::setSubCanvasParamByTag(StringView tag, const String& paramName, const ParamValue& value)
	{
		if (auto subCanvas = getSubCanvasByTag(tag))
		{
			if (auto canvas = subCanvas->canvas())
			{
				canvas->setParamValue(paramName, value);
			}
		}
	}

	void Canvas::setSubCanvasParamsByTag(StringView tag, const HashTable<String, ParamValue>& params)
	{
		if (auto subCanvas = getSubCanvasByTag(tag))
		{
			if (auto canvas = subCanvas->canvas())
			{
				for (const auto& [key, value] : params)
				{
					canvas->setParamValue(key, value);
				}
			}
		}
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
		if (m_autoFitMode != AutoFitMode::None && !m_isEditorPreview)
		{
			// AutoFitModeが有効な間はsetCenterは無視
			return shared_from_this();
		}
		
		m_position = center - Vec2{ m_size.x / 2, m_size.y / 2 };
		return shared_from_this();
	}

	Vec2 Canvas::center() const
	{
		return m_position + Vec2{ m_size.x / 2, m_size.y / 2 };
	}

	std::shared_ptr<Canvas> Canvas::setAutoFitMode(AutoFitMode mode)
	{
		if (m_autoFitMode == mode)
		{
			return shared_from_this();
		}
		m_autoFitMode = mode;
		updateAutoFitIfNeeded(m_lastAutoFitSceneSize.value_or(Scene::Size()), true);
		return shared_from_this();
	}

	std::shared_ptr<Canvas> Canvas::setChildrenLayout(const LayoutVariant& layout)
	{
		m_childrenLayout = layout;
		markLayoutAsDirty();
		return shared_from_this();
	}

	void Canvas::setContainedSubCanvas(const std::weak_ptr<SubCanvas>& subCanvas)
	{
		m_containedSubCanvas = subCanvas;
	}

	std::shared_ptr<Canvas> Canvas::setInteractable(InteractableYN interactable)
	{
		const bool prevValue = m_interactable.getBool();
		m_interactable = interactable;
		
		// interactableが変更された場合、即座に全ての子ノードのプロパティを更新
		if (prevValue != interactable.getBool())
		{
			// 全ての子ノードのInteractionStateとプロパティを更新
			for (const auto& child : m_children)
			{
				// 実効的なinteractable値を計算
				const bool childInteractableValue = child->m_interactable.value();
				const InteractableYN effectiveInteractable{ interactable.getBool() && childInteractableValue };
				
				// MouseTrackerの実効的なinteractableを更新
				child->m_mouseLTracker.setInteractable(effectiveInteractable);
				child->m_mouseRTracker.setInteractable(effectiveInteractable);
				
				// InteractionStateを更新
				if (!effectiveInteractable.getBool())
				{
					child->m_currentInteractionState = InteractionState::Disabled;
				}
				else if (child->m_currentInteractionState == InteractionState::Disabled)
				{
					child->m_currentInteractionState = InteractionState::Default;
				}
				
				// 子ノードのプロパティを更新
				child->m_transform.update(child->m_currentInteractionState, child->m_activeStyleStates, 0.0, m_params, SkipSmoothingYN::No);
				for (const auto& component : child->m_components)
				{
					component->updateProperties(child->m_currentInteractionState, child->m_activeStyleStates, 0.0, m_params, SkipSmoothingYN::No);
				}
				
				// 子ノードの子孫も更新
				child->refreshChildrenPropertiesForInteractableRecursive(effectiveInteractable, m_params, SkipSmoothingYN::No);
			}
		}
		
		return shared_from_this();
	}
	

	std::shared_ptr<Canvas> Canvas::setReferenceSize(const SizeF& size)
	{
		if (m_referenceSize == size)
		{
			return shared_from_this();
		}

		m_referenceSize = size;
		
		// AutoFitModeが有効な場合は再計算
		if (m_autoFitMode != AutoFitMode::None)
		{
			updateAutoFitIfNeeded(m_lastAutoFitSceneSize.value_or(Scene::Size()), true);
		}

		return shared_from_this();
	}

	std::shared_ptr<Canvas> Canvas::setEditorPreviewInternal(bool isEditorPreview)
	{
		m_isEditorPreview = isEditorPreview;
		return shared_from_this();
	}

	void Canvas::setTweenActiveAll(bool active)
	{
		for (auto& child : m_children)
		{
			child->setTweenActiveAll(active, RecursiveYN::Yes);
		}
	}

	void Canvas::setTweenActiveByTag(StringView tag, bool active)
	{
		if (tag.isEmpty())
		{
			return;
		}

		for (auto& child : m_children)
		{
			child->setTweenActiveByTag(tag, active, RecursiveYN::Yes);
		}
	}

	bool Canvas::isTweenPlayingByTag(StringView tag) const
	{
		if (tag.isEmpty())
		{
			return false;
		}

		for (const auto& child : m_children)
		{
			if (child->isTweenPlayingByTag(tag, RecursiveYN::Yes))
			{
				return true;
			}
		}

		return false;
	}

	String Canvas::getTextValueByTag(StringView tag) const
	{
		auto result = getTextValueByTagOpt(tag);
		return result.value_or(U"");
	}

	Optional<String> Canvas::getTextValueByTagOpt(StringView tag) const
	{
		if (tag.isEmpty())
		{
			return none;
		}

		for (const auto& child : m_children)
		{
			if (auto result = child->getTextValueByTagOpt(tag, RecursiveYN::Yes))
			{
				return result;
			}
		}

		return none;
	}

	void Canvas::setTextValueByTag(StringView tag, StringView text)
	{
		if (tag.isEmpty())
		{
			return;
		}

		for (auto& child : m_children)
		{
			child->setTextValueByTag(tag, text, RecursiveYN::Yes);
		}
	}

	bool Canvas::getToggleValueByTag(StringView tag, bool defaultValue) const
	{
		auto result = getToggleValueByTagOpt(tag);
		return result.value_or(defaultValue);
	}

	Optional<bool> Canvas::getToggleValueByTagOpt(StringView tag) const
	{
		if (tag.isEmpty())
		{
			return none;
		}

		for (const auto& child : m_children)
		{
			if (auto result = child->getToggleValueByTagOpt(tag, RecursiveYN::Yes))
			{
				return result;
			}
		}

		return none;
	}

	void Canvas::setToggleValueByTag(StringView tag, bool value)
	{
		if (tag.isEmpty())
		{
			return;
		}

		for (auto& child : m_children)
		{
			child->setToggleValueByTag(tag, value, RecursiveYN::Yes);
		}
	}

	void Canvas::clearFontCache()
	{
		for (const auto& child : m_children)
		{
			auto fontCachedComponents = child->getComponents<detail::IFontCachedComponent>(RecursiveYN::Yes);
			for (const auto& component : fontCachedComponents)
			{
				component->clearFontCache();
			}
		}
	}
}
