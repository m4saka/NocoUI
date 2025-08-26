#include "NocoUI/Node.hpp"
#include "NocoUI/ComponentFactory.hpp"
#include "NocoUI/Serialization.hpp"
#include "NocoUI/Canvas.hpp"
#include "NocoUI/Component/Component.hpp"
#include "NocoUI/detail/ScopedScissorRect.hpp"

namespace noco
{
	InteractionState Node::updateForCurrentInteractionState(const std::shared_ptr<Node>& hoveredNode, InteractableYN parentInteractable, IsScrollingYN isAncestorScrolling)
	{
		const InteractableYN interactable{ m_interactable && parentInteractable };
		InteractionState inheritedInteractionState = InteractionState::Default;
		bool inheritedIsClicked = false;
		if (m_inheritChildrenStateFlags != InheritChildrenStateFlags::None)
		{
			for (const auto& child : m_children)
			{
				InteractionState childInteractionState = child->updateForCurrentInteractionState(hoveredNode, interactable, isAncestorScrolling);
				if (interactable)
				{
					if (!inheritsChildrenPress() && childInteractionState == InteractionState::Pressed)
					{
						childInteractionState = InteractionState::Hovered;
					}
					if (!inheritsChildrenHover() && childInteractionState == InteractionState::Hovered)
					{
						childInteractionState = InteractionState::Default;
					}
					if (inheritsChildrenPress() && child->isClicked())
					{
						inheritedIsClicked = true;
					}
					inheritedInteractionState = ApplyOtherInteractionState(inheritedInteractionState, childInteractionState, AppliesDisabledStateYN::No);
				}
			}
		}
		const bool onClientRect = Cursor::OnClientRect();
		const bool mouseOverForHovered = onClientRect && m_activeInHierarchy && (hoveredNode.get() == this || (inheritsChildrenHover() && (inheritedInteractionState == InteractionState::Hovered || inheritedInteractionState == InteractionState::Pressed)));
		const bool mouseOverForPressed = onClientRect && m_activeInHierarchy && (hoveredNode.get() == this || (inheritsChildrenPress() && (inheritedInteractionState == InteractionState::Pressed || inheritedIsClicked))); // クリック判定用に離した瞬間もホバー扱いにする必要があるため、子のisClickedも加味している
		m_mouseLTracker.update(mouseOverForHovered, mouseOverForPressed, isAncestorScrolling);
		
		if (interactable)
		{
			return ApplyOtherInteractionState(m_mouseLTracker.interactionStateSelf(), inheritedInteractionState);
		}
		else
		{
			return InteractionState::Disabled;
		}
	}

	InteractionState Node::updateForCurrentInteractionStateRight(const std::shared_ptr<Node>& hoveredNode, InteractableYN parentInteractable, IsScrollingYN isAncestorScrolling)
	{
		const InteractableYN interactable{ m_interactable && parentInteractable };
		InteractionState inheritedInteractionState = InteractionState::Default;
		bool inheritedIsRightClicked = false;
		if (m_inheritChildrenStateFlags != InheritChildrenStateFlags::None)
		{
			for (const auto& child : m_children)
			{
				InteractionState childInteractionState = child->updateForCurrentInteractionStateRight(hoveredNode, interactable, isAncestorScrolling);
				if (interactable)
				{
					if (!inheritsChildrenPress() && childInteractionState == InteractionState::Pressed)
					{
						childInteractionState = InteractionState::Hovered;
					}
					if (!inheritsChildrenHover() && childInteractionState == InteractionState::Hovered)
					{
						childInteractionState = InteractionState::Default;
					}
					if (inheritsChildrenPress() && child->isRightClicked())
					{
						inheritedIsRightClicked = true;
					}
					inheritedInteractionState = ApplyOtherInteractionState(inheritedInteractionState, childInteractionState, AppliesDisabledStateYN::No);
				}
			}
		}
		if (interactable)
		{
			const bool onClientRect = Cursor::OnClientRect();
			const bool mouseOverForHovered = onClientRect && m_activeInHierarchy && (hoveredNode.get() == this || (inheritsChildrenHover() && (inheritedInteractionState == InteractionState::Hovered || inheritedInteractionState == InteractionState::Pressed)));
			const bool mouseOverForPressed = onClientRect && m_activeInHierarchy && (hoveredNode.get() == this || (inheritsChildrenPress() && (inheritedInteractionState == InteractionState::Pressed || inheritedIsRightClicked))); // クリック判定用に離した瞬間もホバー扱いにする必要があるため、子のisRightClickedも加味している
			m_mouseRTracker.update(mouseOverForHovered, mouseOverForPressed, isAncestorScrolling);
			return ApplyOtherInteractionState(m_mouseRTracker.interactionStateSelf(), inheritedInteractionState);
		}
		else
		{
			m_mouseRTracker.update(false, false);
			return InteractionState::Disabled;
		}
	}

	void Node::refreshActiveInHierarchy()
	{
		const bool prevActiveInHierarchy = m_activeInHierarchy.getBool();
		
		if (const auto parent = m_parent.lock())
		{
			m_activeInHierarchy = ActiveYN{ m_activeSelf && parent->m_activeInHierarchy };
		}
		else if (!m_canvas.expired())
		{
			// Canvas配下の直接の子の場合はactiveSelfに依存
			m_activeInHierarchy = m_activeSelf;
		}
		else
		{
			// Canvas配下にない場合はactiveInHierarchyはfalse
			m_activeInHierarchy = ActiveYN::No;
		}
		
		// activeInHierarchyがfalseからtrueに変わった時にonActivatedを呼び出し
		if (m_activeInHierarchy && !prevActiveInHierarchy)
		{
			for (const auto& component : m_components)
			{
				component->onActivated(shared_from_this());
			}
		}
		// activeInHierarchyがtrueからfalseに変わった時にonDeactivatedを呼び出し
		else if (!m_activeInHierarchy && prevActiveInHierarchy)
		{
			for (const auto& component : m_components)
			{
				component->onDeactivated(shared_from_this());
			}
		}
		
		for (const auto& child : m_children)
		{
			child->refreshActiveInHierarchy();
		}
	}

	void Node::setCanvasRecursive(const std::weak_ptr<Canvas>& canvas)
	{
		m_canvas = canvas;
		for (const auto& child : m_children)
		{
			child->setCanvasRecursive(canvas);
		}
	}

	void Node::clampScrollOffset()
	{
		if (m_scrollOffset == Vec2::Zero())
		{
			return;
		}

		if (m_children.empty())
		{
			m_scrollOffset = Vec2::Zero();
			return;
		}

		const bool horizontalScrollableValue = horizontalScrollable();
		const bool verticalScrollableValue = verticalScrollable();
		if (!horizontalScrollableValue && !verticalScrollableValue)
		{
			m_scrollOffset = Vec2::Zero();
			return;
		}

		if (!horizontalScrollableValue)
		{
			m_scrollOffset.x = 0.0;
		}
		if (!verticalScrollableValue)
		{
			m_scrollOffset.y = 0.0;
		}

		const Optional<RectF> contentRectOpt = getChildrenContentRectWithPadding();
		if (!contentRectOpt)
		{
			m_scrollOffset = Vec2::Zero();
			return;
		}

		const RectF& contentRect = *contentRectOpt;

		const double viewWidth = m_regionRect.w;
		const double viewHeight = m_regionRect.h;
		const double maxScrollX = Max(contentRect.w - viewWidth, 0.0);
		const double maxScrollY = Max(contentRect.h - viewHeight, 0.0);
		if (maxScrollX <= 0.0 && maxScrollY <= 0.0)
		{
			m_scrollOffset = Vec2::Zero();
			return;
		}

		// ラバーバンドスクロールが無効な場合のみクランプする
		if (!m_rubberBandScrollEnabled)
		{
			const Vec2 scrollOffsetAnchor = std::visit([](const auto& layout) { return layout.scrollOffsetAnchor(); }, m_childrenLayout);
			m_scrollOffset.x = Clamp(m_scrollOffset.x, -maxScrollX * scrollOffsetAnchor.x, maxScrollX * (1.0 - scrollOffsetAnchor.x));
			m_scrollOffset.y = Clamp(m_scrollOffset.y, -maxScrollY * scrollOffsetAnchor.y, maxScrollY * (1.0 - scrollOffsetAnchor.y));
		}
	}

	std::pair<Vec2, Vec2> Node::getValidScrollRange() const
	{
		Vec2 minScroll = Vec2::Zero();
		Vec2 maxScroll = Vec2::Zero();

		if (!m_children.empty())
		{
			const bool horizontalScrollableValue = horizontalScrollable();
			const bool verticalScrollableValue = verticalScrollable();
			
			if (horizontalScrollableValue || verticalScrollableValue)
			{
				const Optional<RectF> contentRectOpt = getChildrenContentRectWithPadding();
				if (contentRectOpt)
				{
					const RectF& contentRect = *contentRectOpt;
					const double viewWidth = m_regionRect.w;
					const double viewHeight = m_regionRect.h;
					const double maxScrollX = Max(contentRect.w - viewWidth, 0.0);
					const double maxScrollY = Max(contentRect.h - viewHeight, 0.0);
					
					const Vec2 scrollOffsetAnchor = std::visit([](const auto& layout) { return layout.scrollOffsetAnchor(); }, m_childrenLayout);
					
					if (horizontalScrollableValue)
					{
						minScroll.x = -maxScrollX * scrollOffsetAnchor.x;
						maxScroll.x = maxScrollX * (1.0 - scrollOffsetAnchor.x);
					}
					
					if (verticalScrollableValue)
					{
						minScroll.y = -maxScrollY * scrollOffsetAnchor.y;
						maxScroll.y = maxScrollY * (1.0 - scrollOffsetAnchor.y);
					}
				}
			}
		}
		
		return { minScroll, maxScroll };
	}

	std::shared_ptr<Node> Node::Create(StringView name, const RegionVariant& region, IsHitTargetYN isHitTarget, InheritChildrenStateFlags inheritChildrenStateFlags)
	{
		return std::shared_ptr<Node>{ new Node{ s_nextInstanceId++, name, region, isHitTarget, inheritChildrenStateFlags } };
	}

	const RegionVariant& Node::region() const
	{
		return m_region;
	}

	std::shared_ptr<Node> Node::setRegion(const RegionVariant& region, RefreshesLayoutYN refreshesLayout)
	{
		m_region = region;
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
		return shared_from_this();
	}

	const InlineRegion* Node::inlineRegion() const
	{
		return std::get_if<InlineRegion>(&m_region);
	}

	const AnchorRegion* Node::anchorRegion() const
	{
		return std::get_if<AnchorRegion>(&m_region);
	}

	Transform& Node::transform()
	{
		return m_transform;
	}

	const Transform& Node::transform() const
	{
		return m_transform;
	}

	const LayoutVariant& Node::childrenLayout() const
	{
		return m_childrenLayout;
	}

	std::shared_ptr<Node> Node::setChildrenLayout(const LayoutVariant& layout, RefreshesLayoutYN refreshesLayout)
	{
		m_childrenLayout = layout;
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
		return shared_from_this();
	}

	const FlowLayout* Node::childrenFlowLayout() const
	{
		return std::get_if<FlowLayout>(&m_childrenLayout);
	}

	const HorizontalLayout* Node::childrenHorizontalLayout() const
	{
		return std::get_if<HorizontalLayout>(&m_childrenLayout);
	}

	const VerticalLayout* Node::childrenVerticalLayout() const
	{
		return std::get_if<VerticalLayout>(&m_childrenLayout);
	}

	SizeF Node::getFittingSizeToChildren() const
	{
		return std::visit([this](const auto& layout) { return layout.getFittingSizeToChildren(m_regionRect, m_children); }, m_childrenLayout);
	}

	std::shared_ptr<Node> Node::setInlineRegionToFitToChildren(FitTarget fitTarget, RefreshesLayoutYN refreshesLayout)
	{
		std::visit([this, fitTarget, refreshesLayout](auto& layout) { layout.setInlineRegionToFitToChildren(m_regionRect, m_children, *this, fitTarget, refreshesLayout); }, m_childrenLayout);
		return shared_from_this();
	}

	const LRTB& Node::childrenLayoutPadding() const
	{
		return std::visit([](const auto& layout) -> const LRTB& { return layout.padding; }, m_childrenLayout);
	}

	bool Node::hasInlineRegion() const
	{
		return std::holds_alternative<InlineRegion>(m_region);
	}

	bool Node::hasAnchorRegion() const
	{
		return std::holds_alternative<AnchorRegion>(m_region);
	}

	JSON Node::toJSON(detail::WithInstanceIdYN withInstanceId) const
	{
		Array<JSON> childrenJSON;
		for (const auto& child : m_children)
		{
			childrenJSON.push_back(child->toJSON(withInstanceId));
		}

		JSON result
		{
			{ U"name", m_name },
			{ U"region", std::visit([](const auto& region) { return region.toJSON(); }, m_region) },
			{ U"transform", m_transform.toJSON() },
			{ U"childrenLayout", std::visit([](const auto& childrenLayout) { return childrenLayout.toJSON(); }, m_childrenLayout) },
			{ U"components", Array<JSON>{} },
			{ U"children", childrenJSON },
			{ U"isHitTarget", m_isHitTarget.getBool() },
			{ U"hitPadding", m_hitPadding.toJSON() },
			{ U"inheritsChildrenHover", inheritsChildrenHover() },
			{ U"inheritsChildrenPress", inheritsChildrenPress() },
			{ U"interactable", m_interactable.getBool() },
			{ U"horizontalScrollable", horizontalScrollable() },
			{ U"verticalScrollable", verticalScrollable() },
			{ U"wheelScrollEnabled", wheelScrollEnabled() },
			{ U"dragScrollEnabled", dragScrollEnabled() },
			{ U"decelerationRate", m_decelerationRate },
			{ U"rubberBandScrollEnabled", m_rubberBandScrollEnabled.getBool() },
			{ U"clippingEnabled", m_clippingEnabled.getBool() },
			{ U"activeSelf", m_activeSelf.getBool() },
		};
		
		// styleStateをシリアライズ（空でない場合のみ）
		if (!m_styleState.empty())
		{
			result[U"styleState"] = m_styleState;
		}

		if (withInstanceId)
		{
			result[U"_instanceId"] = m_instanceId;
		}

		for (const std::shared_ptr<ComponentBase>& component : m_components)
		{
			if (const auto serializableComponent = std::dynamic_pointer_cast<SerializableComponentBase>(component))
			{
				result[U"components"].push_back(serializableComponent->toJSON(withInstanceId));
			}
		}

		return result;
	}

	std::shared_ptr<Node> Node::CreateFromJSON(const JSON& json, detail::WithInstanceIdYN withInstanceId)
	{
		return CreateFromJSON(json, ComponentFactory::GetBuiltinFactory(), withInstanceId);
	}
	
	std::shared_ptr<Node> Node::CreateFromJSON(const JSON& json, const ComponentFactory& componentFactory, detail::WithInstanceIdYN withInstanceId)
	{
		auto node = Node::Create();
		if (json.contains(U"name"))
		{
			node->m_name = json[U"name"].getOr<String>(U"");
		}
		if (json.contains(U"region") && json[U"region"].contains(U"type"))
		{
			const auto type = json[U"region"][U"type"].getString();
			if (type == U"AnchorRegion")
			{
				node->m_region = AnchorRegion::FromJSON(json[U"region"]);
			}
			else if (type == U"InlineRegion")
			{
				node->m_region = InlineRegion::FromJSON(json[U"region"]);
			}
			else
			{
				// 不明な場合はInlineRegion扱いにする
				Logger << U"[NocoUI warning] Unknown region type: '{}'"_fmt(type);
				node->m_region = InlineRegion{};
			}
		}
		if (json.contains(U"transform"))
		{
			node->m_transform.readFromJSON(json[U"transform"]);
		}
		if (json.contains(U"childrenLayout") && json[U"childrenLayout"].contains(U"type"))
		{
			const auto type = json[U"childrenLayout"][U"type"].getString();
			if (type == U"FlowLayout")
			{
				node->m_childrenLayout = FlowLayout::FromJSON(json[U"childrenLayout"]);
			}
			else if (type == U"HorizontalLayout")
			{
				node->m_childrenLayout = HorizontalLayout::FromJSON(json[U"childrenLayout"]);
			}
			else if (type == U"VerticalLayout")
			{
				node->m_childrenLayout = VerticalLayout::FromJSON(json[U"childrenLayout"]);
			}
			else
			{
				// 不明な場合はFlowLayout扱いにする
				Logger << U"[NocoUI warning] Unknown inline children layout type: '{}'"_fmt(type);
				node->m_childrenLayout = FlowLayout{};
			}
		}
		if (json.contains(U"isHitTarget"))
		{
			node->m_isHitTarget = IsHitTargetYN{ json[U"isHitTarget"].getOr<bool>(true) };
		}
		if (json.contains(U"hitPadding"))
		{
			node->m_hitPadding = LRTB::fromJSON(json[U"hitPadding"]);
		}
		if (json.contains(U"inheritsChildrenHover"))
		{
			node->setInheritsChildrenHover(json[U"inheritsChildrenHover"].getOr<bool>(false));
		}
		if (json.contains(U"inheritsChildrenPress"))
		{
			node->setInheritsChildrenPress(json[U"inheritsChildrenPress"].getOr<bool>(false));
		}
		if (json.contains(U"interactable"))
		{
			node->setInteractable(InteractableYN{ json[U"interactable"].getOr<bool>(true) });
		}
		if (json.contains(U"horizontalScrollable"))
		{
			node->setHorizontalScrollable(json[U"horizontalScrollable"].getOr<bool>(false), RefreshesLayoutYN::No);
		}
		if (json.contains(U"verticalScrollable"))
		{
			node->setVerticalScrollable(json[U"verticalScrollable"].getOr<bool>(false), RefreshesLayoutYN::No);
		}
		if (json.contains(U"wheelScrollEnabled"))
		{
			node->setWheelScrollEnabled(json[U"wheelScrollEnabled"].getOr<bool>(true));
		}
		if (json.contains(U"dragScrollEnabled"))
		{
			node->setDragScrollEnabled(json[U"dragScrollEnabled"].getOr<bool>(false));
		}
		if (json.contains(U"decelerationRate"))
		{
			node->setDecelerationRate(json[U"decelerationRate"].getOr<double>(0.2));
		}
		if (json.contains(U"rubberBandScrollEnabled"))
		{
			node->setRubberBandScrollEnabled(RubberBandScrollEnabledYN{ json[U"rubberBandScrollEnabled"].getOr<bool>(false) });
		}
		if (json.contains(U"clippingEnabled"))
		{
			node->setClippingEnabled(ClippingEnabledYN{ json[U"clippingEnabled"].getOr<bool>(false) });
		}
		if (json.contains(U"activeSelf"))
		{
			node->setActive(ActiveYN{ json[U"activeSelf"].getOr<bool>(true) }, RefreshesLayoutYN::No);
		}
		if (json.contains(U"styleState"))
		{
			node->setStyleState(json[U"styleState"].getOr<String>(U""));
		}
		if (withInstanceId && json.contains(U"_instanceId"))
		{
			node->m_instanceId = json[U"_instanceId"].get<uint64>();
		}

		if (json.contains(U"components") && json[U"components"].isArray())
		{
			for (const auto& componentJSON : json[U"components"].arrayView())
			{
				node->addComponentFromJSONImpl(componentJSON, componentFactory, withInstanceId);
			}
		}

		if (json.contains(U"children") && json[U"children"].isArray())
		{
			for (const auto& childJSON : json[U"children"].arrayView())
			{
				auto child = CreateFromJSON(childJSON, componentFactory, withInstanceId);
				node->addChild(child, RefreshesLayoutYN::No);
			}
		}
		return node;
	}

	std::shared_ptr<Node> Node::parentNode() const
	{
		return m_parent.lock();
	}

	std::shared_ptr<INodeContainer> Node::parentContainer() const
	{
		if (auto parentNodePtr = m_parent.lock())
		{
			return parentNodePtr;
		}
		if (auto canvasPtr = m_canvas.lock())
		{
			return canvasPtr;
		}
		return nullptr;
	}

	bool Node::isTopLevelNode() const
	{
		return m_parent.expired() && !m_canvas.expired();
	}

	std::shared_ptr<const Node> Node::findHoverTargetParent() const
	{
		if (m_isHitTarget)
		{
			return shared_from_this();
		}
		if (const auto parent = m_parent.lock())
		{
			return parent->findHoverTargetParent();
		}
		return nullptr;
	}

	std::shared_ptr<Node> Node::findHoverTargetParent()
	{
		if (m_isHitTarget)
		{
			return shared_from_this();
		}
		if (const auto parent = m_parent.lock())
		{
			return parent->findHoverTargetParent();
		}
		return nullptr;
	}

	bool Node::isAncestorOf(const std::shared_ptr<Node>& node) const
	{
		if (node->m_parent.expired())
		{
			return false;
		}
		const auto nodeParent = node->m_parent.lock();
		if (nodeParent.get() == this)
		{
			return true;
		}
		return isAncestorOf(nodeParent);
	}

	std::shared_ptr<Node> Node::setParent(const std::shared_ptr<Node>& parent, RefreshesLayoutYN refreshesLayout)
	{
		// 既に同じ親の場合は何もしない
		if (m_parent.lock() == parent)
		{
			return shared_from_this();
		}
		
		if (parent.get() == this)
		{
			throw Error{ U"setParent: Cannot set self as parent" };
		}
		if (isAncestorOf(parent))
		{
			throw Error{ U"setParent: Cannot create circular reference by setting ancestor as parent" };
		}
		
		if (!m_parent.expired() || isTopLevelNode())
		{
			removeFromParent(RefreshesLayoutYN::No);
		}
		
		parent->addChild(shared_from_this(), RefreshesLayoutYN::No);
		
		if (refreshesLayout)
		{
			refreshChildrenLayout();
		}
		
		return shared_from_this();
	}

	bool Node::removeFromParent(RefreshesLayoutYN refreshesLayout)
	{
		if (const auto parent = m_parent.lock())
		{
			parent->removeChild(shared_from_this(), refreshesLayout);
			return true;
		}
		// Canvasの直下にある場合
		else if (const auto canvas = m_canvas.lock())
		{
			// isTopLevelNode()でトップレベルかチェック
			if (isTopLevelNode())
			{
				canvas->removeChild(shared_from_this(), refreshesLayout);
				return true;
			}
		}
		return false;
	}

	size_t Node::siblingIndex() const
	{
		if (const auto parent = m_parent.lock())
		{
			const auto it = std::find(parent->m_children.begin(), parent->m_children.end(), shared_from_this());
			if (it != parent->m_children.end())
			{
				return static_cast<size_t>(std::distance(parent->m_children.begin(), it));
			}
			else
			{
				throw Error{ U"Node::siblingIndex: Node '{}' is not a child of its parent"_fmt(m_name) };
			}
		}
		else if (const auto canvas = m_canvas.lock())
		{
			const auto it = std::find(canvas->children().begin(), canvas->children().end(), shared_from_this());
			if (it != canvas->children().end())
			{
				return static_cast<size_t>(std::distance(canvas->children().begin(), it));
			}
			else
			{
				throw Error{ U"Node::siblingIndex: Node '{}' is not a child of its canvas"_fmt(m_name) };
			}
		}
		else
		{
			throw Error{ U"Node::siblingIndex: Node '{}' has no parent or canvas"_fmt(m_name) };
		}
	}

	Optional<size_t> Node::siblingIndexOpt() const
	{
		if (const auto parent = m_parent.lock())
		{
			const auto it = std::find_if(parent->m_children.begin(), parent->m_children.end(),
				[this](const std::shared_ptr<Node>& child) { return child.get() == this; });
			if (it != parent->m_children.end())
			{
				return static_cast<size_t>(std::distance(parent->m_children.begin(), it));
			}
		}
		else if (const auto canvas = m_canvas.lock())
		{
			const auto it = std::find(canvas->children().begin(), canvas->children().end(), shared_from_this());
			if (it != canvas->children().end())
			{
				return static_cast<size_t>(std::distance(canvas->children().begin(), it));
			}
		}
		return none;
	}

	std::shared_ptr<Canvas> Node::containedCanvas() const
	{
		return m_canvas.lock();
	}

	std::shared_ptr<ComponentBase> Node::addComponentFromJSON(const JSON& json)
	{
		return addComponentFromJSON(json, ComponentFactory::GetBuiltinFactory());
	}
	
	std::shared_ptr<ComponentBase> Node::addComponentFromJSON(const JSON& json, const ComponentFactory& componentFactory)
	{
		return addComponentFromJSONImpl(json, componentFactory, detail::WithInstanceIdYN::No);
	}

	std::shared_ptr<ComponentBase> Node::addComponentAtIndexFromJSON(const JSON& json, size_t index)
	{
		return addComponentAtIndexFromJSON(json, index, ComponentFactory::GetBuiltinFactory());
	}
	
	std::shared_ptr<ComponentBase> Node::addComponentAtIndexFromJSON(const JSON& json, size_t index, const ComponentFactory& componentFactory)
	{
		return addComponentAtIndexFromJSONImpl(json, index, componentFactory, detail::WithInstanceIdYN::No);
	}

	std::shared_ptr<ComponentBase> Node::addComponentFromJSONImpl(const JSON& json, detail::WithInstanceIdYN withInstanceId)
	{
		return addComponentFromJSONImpl(json, ComponentFactory::GetBuiltinFactory(), withInstanceId);
	}
	
	std::shared_ptr<ComponentBase> Node::addComponentFromJSONImpl(const JSON& json, const ComponentFactory& componentFactory, detail::WithInstanceIdYN withInstanceId)
	{
		auto component = componentFactory.createComponentFromJSON(json, withInstanceId);
		if (component)
		{
			addComponent(component);
		}
		return component;
	}

	std::shared_ptr<ComponentBase> Node::addComponentAtIndexFromJSONImpl(const JSON& json, size_t index, detail::WithInstanceIdYN withInstanceId)
	{
		return addComponentAtIndexFromJSONImpl(json, index, ComponentFactory::GetBuiltinFactory(), withInstanceId);
	}
	
	std::shared_ptr<ComponentBase> Node::addComponentAtIndexFromJSONImpl(const JSON& json, size_t index, const ComponentFactory& componentFactory, detail::WithInstanceIdYN withInstanceId)
	{
		auto component = componentFactory.createComponentFromJSON(json, withInstanceId);
		if (component)
		{
			addComponentAtIndex(component, index);
		}
		return component;
	}

	void Node::removeComponent(const std::shared_ptr<ComponentBase>& component)
	{
		// activeInHierarchyがtrueの場合、onDeactivatedを呼び出し
		if (m_activeInHierarchy)
		{
			component->onDeactivated(shared_from_this());
		}
		m_components.remove(component);
	}

	bool Node::moveComponentUp(const std::shared_ptr<ComponentBase>& component)
	{
		const auto it = std::find(m_components.begin(), m_components.end(), component);
		if (it == m_components.end())
		{
			// 見つからない
			return false;
		}
		if (it == m_components.begin())
		{
			// 一番上にあるため上移動は不可
			return false;
		}
		std::iter_swap(it, std::prev(it));
		return true;
	}

	bool Node::moveComponentDown(const std::shared_ptr<ComponentBase>& component)
	{
		const auto it = std::find(m_components.begin(), m_components.end(), component);
		if (it == m_components.end())
		{
			// 見つからない
			return false;
		}
		if (it == std::prev(m_components.end()))
		{
			// 一番下にあるため下移動は不可
			return false;
		}
		std::iter_swap(it, std::next(it));
		return true;
	}

	const std::shared_ptr<Node>& Node::addChild(std::shared_ptr<Node>&& child, RefreshesLayoutYN refreshesLayout)
	{
		if (!child->m_parent.expired() || child->isTopLevelNode())
		{
			throw Error{ U"addChild: Child node '{}' already has a parent node or parent canvas"_fmt(child->m_name) };
		}
		if (child.get() == this)
		{
			throw Error{ U"addChild: Cannot add child to itself" };
		}
		if (child->isAncestorOf(shared_from_this()))
		{
			throw Error{ U"addChild: Cannot add child to its descendant" };
		}
		child->setCanvasRecursive(m_canvas);
		child->m_parent = shared_from_this();
		child->refreshActiveInHierarchy();
		m_children.push_back(std::move(child));
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
		return m_children.back();
	}

	const std::shared_ptr<Node>& Node::addChild(const std::shared_ptr<Node>& child, RefreshesLayoutYN refreshesLayout)
	{
		if (!child->m_parent.expired() || child->isTopLevelNode())
		{
			throw Error{ U"addChild: Child node '{}' already has a parent node or parent canvas"_fmt(child->m_name) };
		}
		if (child.get() == this)
		{
			throw Error{ U"addChild: Cannot add child to itself" };
		}
		if (child->isAncestorOf(shared_from_this()))
		{
			throw Error{ U"addChild: Cannot add child to its descendant" };
		}
		child->setCanvasRecursive(m_canvas);
		child->m_parent = shared_from_this();
		child->refreshActiveInHierarchy();
		m_children.push_back(child);
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
		return m_children.back();
	}

	const std::shared_ptr<Node>& Node::emplaceChild(StringView name, const RegionVariant& region, IsHitTargetYN isHitTarget, InheritChildrenStateFlags inheritChildrenStateFlags, RefreshesLayoutYN refreshesLayout)
	{
		auto child = Node::Create(name, region, isHitTarget, inheritChildrenStateFlags);
		child->setCanvasRecursive(m_canvas);
		child->m_parent = shared_from_this();
		child->refreshActiveInHierarchy();
		m_children.push_back(std::move(child));
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
		return m_children.back();
	}

	const std::shared_ptr<Node>& Node::emplaceChild(RefreshesLayoutYN refreshesLayout)
	{
		auto child = Node::Create();
		child->setCanvasRecursive(m_canvas);
		child->m_parent = shared_from_this();
		child->refreshActiveInHierarchy();
		m_children.push_back(std::move(child));
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
		return m_children.back();
	}

	const std::shared_ptr<Node>& Node::addChildFromJSON(const JSON& json, RefreshesLayoutYN refreshesLayout)
	{
		auto child = CreateFromJSON(json);
		child->setCanvasRecursive(m_canvas);
		child->m_parent = shared_from_this();
		child->refreshActiveInHierarchy();
		m_children.push_back(std::move(child));
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
		return m_children.back();
	}

	const std::shared_ptr<Node>& Node::addChildAtIndexFromJSON(const JSON& json, size_t index, RefreshesLayoutYN refreshesLayout)
	{
		if (index > m_children.size())
		{
			index = m_children.size();
		}

		auto child = CreateFromJSON(json);
		child->setCanvasRecursive(m_canvas);
		child->m_parent = shared_from_this();
		child->refreshActiveInHierarchy();
		const auto it = m_children.insert(m_children.begin() + index, std::move(child));
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
		return *it;
	}

	const std::shared_ptr<Node>& Node::addChildAtIndex(const std::shared_ptr<Node>& child, size_t index, RefreshesLayoutYN refreshesLayout)
	{
		if (!child->m_parent.expired() || child->isTopLevelNode())
		{
			throw Error{ U"addChildAtIndex: Child node '{}' already has a parent node or parent canvas"_fmt(child->m_name) };
		}
		if (child.get() == this)
		{
			throw Error{ U"addChildAtIndex: Cannot add child to itself" };
		}
		if (child->isAncestorOf(shared_from_this()))
		{
			throw Error{ U"addChildAtIndex: Cannot add child to its descendant" };
		}

		if (index > m_children.size())
		{
			index = m_children.size();
		}

		child->setCanvasRecursive(m_canvas);
		child->m_parent = shared_from_this();
		child->refreshActiveInHierarchy();
		m_children.insert(m_children.begin() + index, child);

		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}

		return m_children[index];
	}

	void Node::removeChild(const std::shared_ptr<Node>& child, RefreshesLayoutYN refreshesLayout)
	{
		if (!m_children.contains(child))
		{
			throw Error{ U"removeChild: Child node '{}' not found in node '{}'"_fmt(child->m_name, m_name) };
		}
		child->setCanvasRecursive(std::weak_ptr<Canvas>{});
		child->m_parent.reset();
		child->refreshActiveInHierarchy();
		m_children.remove(child);
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
	}

	bool Node::containsChild(const std::shared_ptr<Node>& child, RecursiveYN recursive) const
	{
		if (m_children.contains(child))
		{
			return true;
		}
		if (recursive)
		{
			for (const auto& c : m_children)
			{
				if (c->containsChild(child, RecursiveYN::Yes))
				{
					return true;
				}
			}
		}
		return false;
	}

	bool Node::containsChildByName(StringView name, RecursiveYN recursive) const
	{
		for (const auto& child : m_children)
		{
			if (child->m_name == name)
			{
				return true;
			}
		}
		if (recursive)
		{
			for (const auto& child : m_children)
			{
				if (child->containsChildByName(name, RecursiveYN::Yes))
				{
					return true;
				}
			}
		}
		return false;
	}

	std::shared_ptr<Node> Node::getChildByName(StringView name, RecursiveYN recursive)
	{
		for (const auto& child : m_children)
		{
			if (child->m_name == name)
			{
				return child;
			}
		}
		if (recursive)
		{
			for (const auto& child : m_children)
			{
				// 例外を投げられると途中で終了してしまうため、getChildByNameではなくgetChildByNameOrNullを使う必要がある
				if (const auto found = child->getChildByNameOrNull(name, RecursiveYN::Yes))
				{
					return found;
				}
			}
		}
		throw Error{ U"Child node '{}' not found in node '{}'"_fmt(name, m_name) };
	}

	std::shared_ptr<Node> Node::getChildByNameOrNull(StringView name, RecursiveYN recursive)
	{
		for (const auto& child : m_children)
		{
			if (child->m_name == name)
			{
				return child;
			}
		}
		if (recursive)
		{
			for (const auto& child : m_children)
			{
				if (const auto found = child->getChildByNameOrNull(name, RecursiveYN::Yes))
				{
					return found;
				}
			}
		}
		return nullptr;
	}

	void Node::refreshChildrenLayout()
	{
		std::visit([this](const auto& layout)
			{
				layout.execute(m_regionRect, m_children, [this](const std::shared_ptr<Node>& child, const RectF& rect)
					{
						child->m_regionRect = rect;
						if (child->hasInlineRegion())
						{
							child->m_regionRect.moveBy(-m_scrollOffset);
						}
					});
			}, m_childrenLayout);
		for (const auto& child : m_children)
		{
			child->refreshChildrenLayout();
		}

		// レイアウト更新後の状態でスクロールオフセットを制限し、変化があれば反映
		// (子ノードの大きさに変更があった場合にもスクロールオフセットの制限を更新する必要があるため)
		const Vec2 prevScrollOffset = m_scrollOffset;
		clampScrollOffset();
		if (m_scrollOffset != prevScrollOffset)
		{
			std::visit([this](const auto& layout)
				{
					layout.execute(m_regionRect, m_children, [this](const std::shared_ptr<Node>& child, const RectF& rect)
						{
							child->m_regionRect = rect;
							if (child->hasInlineRegion())
							{
								child->m_regionRect.moveBy(-m_scrollOffset);
							}
						});
				}, m_childrenLayout);
			for (const auto& child : m_children)
			{
				child->refreshChildrenLayout();
			}
		}
	}

	Optional<RectF> Node::getChildrenContentRect() const
	{
		if (m_children.empty())
		{
			return none;
		}

		bool exists = false;
		double left = std::numeric_limits<double>::infinity();
		double top = std::numeric_limits<double>::infinity();
		double right = -std::numeric_limits<double>::infinity();
		double bottom = -std::numeric_limits<double>::infinity();
		for (const auto& child : m_children)
		{
			if (!child->hasInlineRegion())
			{
				continue;
			}
			exists = true;

			const RectF& childRect = child->regionRectWithMargin();
			left = Min(left, childRect.x);
			top = Min(top, childRect.y);
			right = Max(right, childRect.x + childRect.w);
			bottom = Max(bottom, childRect.y + childRect.h);
		}

		if (!exists)
		{
			return none;
		}

		// 無効な値の場合はnoneを返す
		if (!std::isfinite(left) || !std::isfinite(top) || !std::isfinite(right) || !std::isfinite(bottom))
		{
			return none;
		}

		const double width = right - left;
		const double height = bottom - top;
		if (width < 0.0 || height < 0.0)
		{
			return none;
		}

		return RectF{ left, top, width, height };
	}

	Optional<RectF> Node::getChildrenContentRectWithPadding() const
	{
		const auto contentRectOpt = getChildrenContentRect();
		if (!contentRectOpt)
		{
			return none;
		}
		const RectF& contentRect = *contentRectOpt;
		const LRTB& padding = childrenLayoutPadding();
		return RectF{ contentRect.x - padding.left, contentRect.y - padding.top, contentRect.w + padding.left + padding.right, contentRect.h + padding.top + padding.bottom };
	}

	std::shared_ptr<Node> Node::hoveredNodeRecursive()
	{
		return hitTest(Cursor::PosF());
	}

	std::shared_ptr<Node> Node::hitTest(const Vec2& point)
	{
		// interactableはチェック不要(無効時も裏側をクリック不可にするためにホバー扱いにする必要があるため)
		if (!m_activeSelf)
		{
			return nullptr;
		}
		// hitPaddingを考慮した当たり判定領域を計算
		const bool hit = hitQuad(WithPaddingYN::Yes).contains(point);
		if (m_clippingEnabled && !m_transformedQuad.contains(point))
		{
			return nullptr;
		}
		for (auto it = m_children.rbegin(); it != m_children.rend(); ++it)
		{
			if (const auto hoveredNode = (*it)->hitTest(point))
			{
				return hoveredNode;
			}
		}
		if (m_isHitTarget && hit)
		{
			return shared_from_this();
		}
		return nullptr;
	}

	std::shared_ptr<const Node> Node::hoveredNodeRecursive() const
	{
		return hitTest(Cursor::PosF());
	}

	std::shared_ptr<const Node> Node::hitTest(const Vec2& point) const
	{
		// interactableはチェック不要(無効時も裏側をクリック不可にするためにホバー扱いにする必要があるため)
		if (!m_activeSelf)
		{
			return nullptr;
		}
		// hitPaddingを考慮した当たり判定領域を計算
		const bool hit = hitQuad(WithPaddingYN::Yes).contains(point);
		if (m_clippingEnabled && !m_transformedQuad.contains(point))
		{
			return nullptr;
		}
		for (auto it = m_children.rbegin(); it != m_children.rend(); ++it)
		{
			if (const auto hoveredNode = (*it)->hitTest(point))
			{
				return hoveredNode;
			}
		}
		if (m_isHitTarget && hit)
		{
			return shared_from_this();
		}
		return nullptr;
	}

	std::shared_ptr<Node> Node::findContainedScrollableNode()
	{
		if (horizontalScrollable() || verticalScrollable())
		{
			if (const Optional<RectF> contentRectOpt = getChildrenContentRectWithPadding())
			{
				const RectF& contentRectLocal = *contentRectOpt;
				if ((horizontalScrollable() && contentRectLocal.w > m_regionRect.w) ||
					(verticalScrollable() && contentRectLocal.h > m_regionRect.h))
				{
					return shared_from_this();
				}
			}
		}
		if (const auto parent = m_parent.lock())
		{
			return parent->findContainedScrollableNode();
		}
		return nullptr;
	}

	void Node::updateInteractionState(const std::shared_ptr<Node>& hoveredNode, double deltaTime, InteractableYN parentInteractable, InteractionState parentInteractionState, InteractionState parentInteractionStateRight, IsScrollingYN isAncestorScrolling)
	{
		// updateInteractionStateはユーザーコードを含まずaddChildやaddComponentによるイテレータ破壊が起きないため、一時バッファは使用不要

		const auto thisNode = shared_from_this();

		m_prevClickRequested = m_clickRequested;
		m_prevRightClickRequested = m_rightClickRequested;
		m_clickRequested = false;
		m_rightClickRequested = false;

		m_currentInteractionState = updateForCurrentInteractionState(hoveredNode, parentInteractable, isAncestorScrolling);
		m_currentInteractionStateRight = updateForCurrentInteractionStateRight(hoveredNode, parentInteractable, isAncestorScrolling);
		if (!m_isHitTarget)
		{
			// HitTargetでない場合は親のinteractionStateを引き継ぐ
			m_currentInteractionState = ApplyOtherInteractionState(m_currentInteractionState, parentInteractionState);
			m_currentInteractionStateRight = ApplyOtherInteractionState(m_currentInteractionStateRight, parentInteractionStateRight);
		}

		if (!m_children.empty())
		{
			// 子ノードのupdateInteractionState実行
			const InteractableYN interactable{ m_interactable && parentInteractable };
			for (const auto& child : m_children)
			{
				child->updateInteractionState(hoveredNode, deltaTime, interactable, m_currentInteractionState, m_currentInteractionStateRight, isAncestorScrolling);
			}
		}
	}

	void Node::updateKeyInput()
	{
		const auto thisNode = shared_from_this();

		// addChild等によるイテレータ破壊を避けるためにバッファへ複製してから処理
		m_childrenTempBuffer.clear();
		m_childrenTempBuffer.reserve(m_children.size());
		for (const auto& child : m_children)
		{
			m_childrenTempBuffer.push_back(child);
		}
		for (auto it = m_childrenTempBuffer.rbegin(); it != m_childrenTempBuffer.rend(); ++it)
		{
			(*it)->updateKeyInput();
		}
		m_childrenTempBuffer.clear();

		// addComponent等によるイテレータ破壊を避けるためにバッファへ複製してから処理
		m_componentTempBuffer.clear();
		m_componentTempBuffer.reserve(m_components.size());
		for (const auto& component : m_components)
		{
			m_componentTempBuffer.push_back(component);
		}
		for (auto it = m_componentTempBuffer.rbegin(); it != m_componentTempBuffer.rend(); ++it)
		{
			if (m_activeInHierarchy && (!detail::s_canvasUpdateContext.keyInputBlocked)) // updateKeyInput内で更新される場合があるためループ内で分岐が必要
			{
				(*it)->updateKeyInput(thisNode);
			}
			else
			{
				(*it)->updateKeyInputInactive(thisNode);
			}
		}
		m_componentTempBuffer.clear();
	}

	void Node::update(const std::shared_ptr<Node>& scrollableHoveredNode, double deltaTime, const Mat3x2& parentTransformMat, const Mat3x2& parentHitTestMat, const HashTable<String, ParamValue>& params, const Array<String>& parentActiveStyleStates)
	{
		const auto thisNode = shared_from_this();
		
		// 慣性スクロール処理
		constexpr double MinInertiaVelocity = 1.0;
		constexpr double MinActualDelta = 0.001;
		if (m_scrollVelocity.length() > 0.0 && !m_dragStartPos.has_value())
		{	
			const Vec2 scrollDelta = m_scrollVelocity * deltaTime;
			if (m_scrollVelocity.length() > MinInertiaVelocity) // 速度が十分小さい場合は停止
			{
				const Vec2 oldOffset = m_scrollOffset;
				scroll(scrollDelta);
				const Vec2 actualDelta = m_scrollOffset - oldOffset;
				if (actualDelta.length() < MinActualDelta) // 実際にスクロールできなかった場合
				{
					m_scrollVelocity = Vec2::Zero();
				}
				else
				{
					// 減衰
					m_scrollVelocity *= Math::Pow(m_decelerationRate, deltaTime);
					
					// 範囲外に到達した場合は慣性を消す
					const auto [minScroll, maxScroll] = getValidScrollRange();
					if ((m_scrollOffset.x <= minScroll.x && m_scrollVelocity.x < 0) ||
						(m_scrollOffset.x >= maxScroll.x && m_scrollVelocity.x > 0))
					{
						m_scrollVelocity.x = 0.0;
					}
					if ((m_scrollOffset.y <= minScroll.y && m_scrollVelocity.y < 0) ||
						(m_scrollOffset.y >= maxScroll.y && m_scrollVelocity.y > 0))
					{
						m_scrollVelocity.y = 0.0;
					}
				}
			}
			else
			{
				// 速度が十分小さくなったら停止
				m_scrollVelocity = Vec2::Zero();
			}
		}
		
		if (m_preventDragScroll && !MouseL.pressed())
		{
			m_preventDragScroll = false;
		}
		
		// ラバーバンドスクロール処理(ドラッグしていない時に範囲外なら戻す)
		if (m_rubberBandScrollEnabled && !m_dragStartPos.has_value())
		{
			const auto [minScroll, maxScroll] = getValidScrollRange();
			Vec2 targetOffset = m_scrollOffset;
			bool needsRubberBand = false;
			
			// 範囲外かチェック
			if (m_scrollOffset.x < minScroll.x)
			{
				targetOffset.x = minScroll.x;
				needsRubberBand = true;
			}
			else if (m_scrollOffset.x > maxScroll.x)
			{
				targetOffset.x = maxScroll.x;
				needsRubberBand = true;
			}
			
			if (m_scrollOffset.y < minScroll.y)
			{
				targetOffset.y = minScroll.y;
				needsRubberBand = true;
			}
			else if (m_scrollOffset.y > maxScroll.y)
			{
				targetOffset.y = maxScroll.y;
				needsRubberBand = true;
			}
			
			// 範囲外なら戻す
			if (needsRubberBand)
			{
				constexpr double RubberBandSpeed = 10.0; // 戻る速度係数
				const Vec2 diff = targetOffset - m_scrollOffset;
				const Vec2 scrollDelta = diff * RubberBandSpeed * deltaTime;
				
				// 十分近くなったら完全に合わせる
				constexpr double SnapThreshold = 0.5;
				if (diff.length() < SnapThreshold)
				{
					m_scrollOffset = targetOffset;
				}
				else
				{
					m_scrollOffset += scrollDelta;
				}
				
				// レイアウトを更新
				refreshContainedCanvasLayout();
			}
		}
		
		// addComponent等によるイテレータ破壊を避けるためにバッファへ複製してから処理
		m_componentTempBuffer.clear();
		m_componentTempBuffer.reserve(m_components.size());
		for (const auto& component : m_components)
		{
			m_componentTempBuffer.push_back(component);
		}

		// コンポーネントのupdate実行
		for (const auto& component : m_componentTempBuffer)
		{
			if (m_activeInHierarchy) // update内で更新される場合があるためループ内で分岐が必要
			{
				component->update(thisNode);
			}
			else
			{
				component->updateInactive(thisNode);
			}
		}
		m_componentTempBuffer.clear();

		// activeStyleStatesを構築（親から受け取ったもの + 自身のstyleState）
		// 非アクティブな場合でも子ノードに渡すために構築する
		m_activeStyleStates = parentActiveStyleStates;
		
		// 自身のstyleStateを追加
		if (!m_styleState.empty())
		{
			m_activeStyleStates.push_back(m_styleState);
		}
		
		if (m_activeInHierarchy)
		{
			refreshTransformMat(RecursiveYN::No, parentTransformMat, parentHitTestMat, params);
		}

		// ホバー中、ドラッグスクロール中、または慣性スクロール中はスクロールバーを表示
		const auto dragScrollingNode = detail::s_canvasUpdateContext.dragScrollingNode.lock();
		const bool isInertialScrolling = m_scrollVelocity.length() > 0.0; // 速度が十分小さくなると0.0に設定されるため0.0との比較で問題ない
		if (thisNode == scrollableHoveredNode || 
			(dragScrollingNode == thisNode && dragScrollingNode->m_dragThresholdExceeded) ||
			isInertialScrolling)
		{
			m_scrollBarAlpha.update(0.5, 0.1, deltaTime);
		}
		else
		{
			m_scrollBarAlpha.update(0.0, 0.1, deltaTime);
		}

		if (!m_children.empty())
		{
			// addChild等によるイテレータ破壊を避けるためにバッファへ複製してから処理
			m_childrenTempBuffer.clear();
			m_childrenTempBuffer.reserve(m_children.size());
			for (const auto& child : m_children)
			{
				m_childrenTempBuffer.push_back(child);
			}

			// 子ノードのupdate実行
			const Mat3x2 childHitTestMat = calculateHitTestMat(parentHitTestMat);
			
			for (const auto& child : m_childrenTempBuffer)
			{
				child->update(scrollableHoveredNode, deltaTime, m_transformMatInHierarchy, childHitTestMat, params, m_activeStyleStates);
			}

			m_childrenTempBuffer.clear();
		}
	}

	void Node::lateUpdate()
	{
		const auto thisNode = shared_from_this();

		if (!m_components.empty())
		{
			// addComponent等によるイテレータ破壊を避けるためにバッファへ複製してから処理
			m_componentTempBuffer.clear();
			m_componentTempBuffer.reserve(m_components.size());
			for (const auto& component : m_components)
			{
				m_componentTempBuffer.push_back(component);
			}

			// コンポーネントのlateUpdate実行
			if (m_activeInHierarchy)
			{
				for (const auto& component : m_componentTempBuffer)
				{
					component->lateUpdate(thisNode);
				}
			}
			else
			{
				for (const auto& component : m_componentTempBuffer)
				{
					component->lateUpdateInactive(thisNode);
				}
			}
			m_componentTempBuffer.clear();
		}

		if (!m_children.empty())
		{
			// addChild等によるイテレータ破壊を避けるためにバッファへ複製してから処理
			m_childrenTempBuffer.clear();
			m_childrenTempBuffer.reserve(m_children.size());
			for (const auto& child : m_children)
			{
				m_childrenTempBuffer.push_back(child);
			}

			// 子ノードのlateUpdate実行
			for (const auto& child : m_childrenTempBuffer)
			{
				child->lateUpdate();
			}
			m_childrenTempBuffer.clear();
		}
	}

	void Node::postLateUpdate(double deltaTime, const HashTable<String, ParamValue>& params)
	{
		// postLateUpdateはユーザーコードを含まずaddChildやaddComponentによるイテレータ破壊は起きないため、一時バッファは使用不要

		// Transformのプロパティ値更新
		m_transform.update(m_currentInteractionState, m_activeStyleStates, deltaTime, params);

		// コンポーネントのプロパティ値更新
		for (const auto& component : m_components)
		{
			// m_activeStyleStatesはupdateで構築済み
			component->updateProperties(m_currentInteractionState, m_activeStyleStates, deltaTime, params);
		}

		// 子ノードのpostLateUpdate実行
		for (const auto& child : m_children)
		{
			child->postLateUpdate(deltaTime, params);
		}

		m_prevActiveInHierarchy = m_activeInHierarchy;
	}

	void Node::refreshTransformMat(RecursiveYN recursive, const Mat3x2& parentTransformMat, const Mat3x2& parentHitTestMat, const HashTable<String, ParamValue>& params)
	{
		m_transform.update(m_currentInteractionState, m_activeStyleStates, 0.0, params);
		
		const Vec2& scale = m_transform.scale().value();
		const Vec2& pivot = m_transform.pivot().value();
		const Vec2& translate = m_transform.translate().value();
		const double rotation = m_transform.rotation().value();
		
		const Vec2 pivotPos = m_regionRect.pos + m_regionRect.size * pivot;
		
		// 自身の変換行列を構築（適用順: Scale → Rotate → Translate）
		Mat3x2 selfTransform = Mat3x2::Scale(scale, pivotPos);
		
		if (rotation != 0.0)
		{
			selfTransform = selfTransform * Mat3x2::Rotate(Math::ToRadians(rotation), pivotPos);
		}
		
		selfTransform = selfTransform * Mat3x2::Translate(translate);
		
		// 親の変換と合成（親の変換が先に適用される）
		m_transformMatInHierarchy = selfTransform * parentTransformMat;
		
		// transformedQuadを計算
		const Vec2 topLeft = m_transformMatInHierarchy.transformPoint(m_regionRect.pos);
		const Vec2 topRight = m_transformMatInHierarchy.transformPoint(m_regionRect.pos + Vec2{m_regionRect.w, 0});
		const Vec2 bottomRight = m_transformMatInHierarchy.transformPoint(m_regionRect.br());
		const Vec2 bottomLeft = m_transformMatInHierarchy.transformPoint(m_regionRect.pos + Vec2{0, m_regionRect.h});
		
		// 負のスケールの場合、Quadの頂点順序を調整
		const Vec2& visualScale = m_transform.scale().value();
		if (visualScale.x < 0 || visualScale.y < 0)
		{
			if (visualScale.x < 0 && visualScale.y >= 0)
			{
				// X軸のみ反転
				m_transformedQuad = Quad{ topRight, topLeft, bottomLeft, bottomRight };
			}
			else if (visualScale.x >= 0 && visualScale.y < 0)
			{
				// Y軸のみ反転
				m_transformedQuad = Quad{ bottomLeft, bottomRight, topRight, topLeft };
			}
			else
			{
				// XY両軸反転
				m_transformedQuad = Quad{ bottomRight, bottomLeft, topLeft, topRight };
			}
		}
		else
		{
			m_transformedQuad = Quad{ topLeft, topRight, bottomRight, bottomLeft };
		}
		
		// HitTest用の変換行列を計算
		m_hitTestMatInHierarchy = calculateHitTestMat(parentHitTestMat);
		
		// HitTest用のQuadを計算
		const Vec2 hitTopLeft = m_hitTestMatInHierarchy.transformPoint(m_regionRect.pos);
		const Vec2 hitTopRight = m_hitTestMatInHierarchy.transformPoint(m_regionRect.pos + Vec2{m_regionRect.w, 0});
		const Vec2 hitBottomRight = m_hitTestMatInHierarchy.transformPoint(m_regionRect.br());
		const Vec2 hitBottomLeft = m_hitTestMatInHierarchy.transformPoint(m_regionRect.pos + Vec2{0, m_regionRect.h});
		
		// 負のスケールの場合、Quadの頂点順序を調整
		if (m_transform.affectsHitTest().value() && (scale.x < 0 || scale.y < 0))
		{
			if (scale.x < 0 && scale.y >= 0)
			{
				// X軸のみ反転
				m_hitQuad = Quad{ hitTopRight, hitTopLeft, hitBottomLeft, hitBottomRight };
			}
			else if (scale.x >= 0 && scale.y < 0)
			{
				// Y軸のみ反転
				m_hitQuad = Quad{ hitBottomLeft, hitBottomRight, hitTopRight, hitTopLeft };
			}
			else
			{
				// XY両軸反転
				m_hitQuad = Quad{ hitBottomRight, hitBottomLeft, hitTopLeft, hitTopRight };
			}
		}
		else
		{
			m_hitQuad = Quad{ hitTopLeft, hitTopRight, hitBottomRight, hitBottomLeft };
		}
		
		// パディング有りのHitTestQuadを計算
		const RectF paddedRect{
			m_regionRect.x - m_hitPadding.left,
			m_regionRect.y - m_hitPadding.top,
			m_regionRect.w + m_hitPadding.totalWidth(),
			m_regionRect.h + m_hitPadding.totalHeight()
		};
		const Vec2 paddedTopLeft = m_hitTestMatInHierarchy.transformPoint(paddedRect.pos);
		const Vec2 paddedTopRight = m_hitTestMatInHierarchy.transformPoint(paddedRect.pos + Vec2{paddedRect.w, 0});
		const Vec2 paddedBottomRight = m_hitTestMatInHierarchy.transformPoint(paddedRect.br());
		const Vec2 paddedBottomLeft = m_hitTestMatInHierarchy.transformPoint(paddedRect.pos + Vec2{0, paddedRect.h});
		
		// 負のスケールの場合、Quadの頂点順序を調整
		if (m_transform.affectsHitTest().value() && (scale.x < 0 || scale.y < 0))
		{
			if (scale.x < 0 && scale.y >= 0)
			{
				// X軸のみ反転
				m_hitQuadWithPadding = Quad{ paddedTopRight, paddedTopLeft, paddedBottomLeft, paddedBottomRight };
			}
			else if (scale.x >= 0 && scale.y < 0)
			{
				// Y軸のみ反転
				m_hitQuadWithPadding = Quad{ paddedBottomLeft, paddedBottomRight, paddedTopRight, paddedTopLeft };
			}
			else
			{
				// XY両軸反転
				m_hitQuadWithPadding = Quad{ paddedBottomRight, paddedBottomLeft, paddedTopLeft, paddedTopRight };
			}
		}
		else
		{
			m_hitQuadWithPadding = Quad{ paddedTopLeft, paddedTopRight, paddedBottomRight, paddedBottomLeft };
		}
		
		if (recursive)
		{
			// 子ノードの変換行列を更新
			for (const auto& child : m_children)
			{
				child->refreshTransformMat(RecursiveYN::Yes, m_transformMatInHierarchy, m_hitTestMatInHierarchy, params);
			}
		}
	}

	void Node::scroll(const Vec2& offsetDelta, RefreshesLayoutYN refreshesLayout)
	{
		if (m_preventDragScroll)
		{
			return;
		}
		
		bool scrolledH = false;
		if (horizontalScrollable() && offsetDelta.x != 0.0)
		{
			m_scrollOffset.x += offsetDelta.x;
			scrolledH = true;
		}
		bool scrolledV = false;
		if (verticalScrollable() && offsetDelta.y != 0.0)
		{
			m_scrollOffset.y += offsetDelta.y;
			scrolledV = true;
		}
		if (scrolledH || scrolledV)
		{
			clampScrollOffset();
			if (refreshesLayout)
			{
				refreshContainedCanvasLayout();
			}
		}
	}

	Vec2 Node::scrollOffset() const
	{
		return m_scrollOffset;
	}

	void Node::resetScrollOffset(RecursiveYN recursive, RefreshesLayoutYN refreshesLayoutPre, RefreshesLayoutYN refreshesLayoutPost)
	{
		if (refreshesLayoutPre)
		{
			refreshContainedCanvasLayout();
		}

		if (m_children.empty())
		{
			m_scrollOffset = Vec2::Zero();
		}
		else
		{
			// 最小値を設定してからClamp
			m_scrollOffset = Vec2::All(std::numeric_limits<double>::lowest());
			clampScrollOffset();
		}

		if (recursive)
		{
			for (const auto& child : m_children)
			{
				child->resetScrollOffset(RecursiveYN::Yes, RefreshesLayoutYN::No, RefreshesLayoutYN::No);
			}
		}

		if (refreshesLayoutPost)
		{
			refreshContainedCanvasLayout();
		}
	}

	void Node::replaceParamRefs(const String& oldName, const String& newName, RecursiveYN recursive)
	{
		m_transform.replaceParamRefs(oldName, newName);
		
		for (const auto& component : m_components)
		{
			if (const auto serializableComponent = std::dynamic_pointer_cast<SerializableComponentBase>(component))
			{
				serializableComponent->replaceParamRefs(oldName, newName);
			}
		}
		
		if (recursive)
		{
			for (const auto& child : m_children)
			{
				child->replaceParamRefs(oldName, newName, RecursiveYN::Yes);
			}
		}
	}

	void Node::draw() const
	{
		if (!m_activeSelf || !m_activeInHierarchy)
		{
			return;
		}

		// クリッピング有効の場合はクリッピング範囲を設定
		Optional<detail::ScopedScissorRect> scissorRect;
		if (m_clippingEnabled)
		{
			scissorRect.emplace(unrotatedTransformedRect().asRect());
		}

		// Transformの乗算カラーを適用
		const ColorF transformColor = m_transform.color().value();
		Optional<ScopedColorMul2D> colorMul;
		const ColorF currentColor = ColorF{ Graphics2D::GetColorMul() };
		const ColorF newColor = currentColor * transformColor;
		if (transformColor != ColorF{ 1.0 })
		{
			colorMul.emplace(newColor);
		}

		// draw関数はconstのため、addComponentやaddChild等によるイテレータ破壊は考慮不要とする
		{
			Optional<Transformer2D> transformer;
			if (m_transformMatInHierarchy != Mat3x2::Identity())
			{
				transformer.emplace(m_transformMatInHierarchy);
			}

			for (const auto& component : m_components)
			{
				component->draw(*this);
			}
		}
		{
			for (const auto& child : m_children)
			{
				child->draw();
			}
		}

		// スクロールバー描画
		if (m_scrollBarAlpha.currentValue() > 0.0)
		{
			// スクロールバーは回転を適用
			Optional<Transformer2D> transformer;
			const double currentRotation = extractRotationFromTransformMat();
			if (Math::Abs(currentRotation) > 0.0001)
			{
				const Vec2 pivotPos = transformPivotPos();
				const Mat3x2 rotationMat = Mat3x2::Rotate(currentRotation, pivotPos);
				transformer.emplace(rotationMat);
			}

			const bool needHorizontalScrollBar = horizontalScrollable();
			const bool needVerticalScrollBar = verticalScrollable();
			if (needHorizontalScrollBar || needVerticalScrollBar)
			{
				if (const Optional<RectF> contentRectOpt = getChildrenContentRectWithPadding())
				{
					const RectF& contentRectLocal = *contentRectOpt;
					const Vec2 scrollOffsetAnchor = std::visit([](const auto& layout) { return layout.scrollOffsetAnchor(); }, m_childrenLayout);
					const double roundRadius = 2.0;

					// 背景より手前にするためにハンドル部分は後で描画
					Optional<RectF> horizontalHandleRect = none;
					Optional<RectF> verticalHandleRect = none;
					
					const Vec2 scale = transformScaleInHierarchy();

					// 横スクロールバー
					if (needHorizontalScrollBar)
					{
						const double viewWidth = m_regionRect.w * scale.x;
						const double contentWidth = contentRectLocal.w * scale.x;
						const double maxScrollX = (contentWidth > viewWidth) ? (contentWidth - viewWidth) : 0.0;
						if (maxScrollX > 0.0)
						{
							const double w = (viewWidth * viewWidth) / contentWidth;
							const double scrolledRatio = (m_scrollOffset.x * scale.x + maxScrollX * scrollOffsetAnchor.x) / maxScrollX;
							const double x = scrolledRatio * (viewWidth - w);
							const double thickness = 4.0 * scale.y;

							const RectF unrotated = unrotatedTransformedRect();
							const RectF backgroundRect
							{
								unrotated.x,
								unrotated.y + unrotated.h - thickness,
								unrotated.w,
								thickness
							};
							backgroundRect.rounded(roundRadius).draw(ColorF{ 0.0, m_scrollBarAlpha.currentValue() });

							horizontalHandleRect = RectF
							{
								unrotated.x + x,
								unrotated.y + unrotated.h - thickness,
								w,
								thickness
							};
						}
					}

					// 縦スクロールバー
					if (needVerticalScrollBar)
					{
						const double viewHeight = m_regionRect.h * scale.y;
						const double contentHeight = contentRectLocal.h * scale.y;
						const double maxScrollY = (contentHeight > viewHeight) ? (contentHeight - viewHeight) : 0.0;
						if (maxScrollY > 0.0)
						{
							const double h = (viewHeight * viewHeight) / contentHeight;
							const double scrolledRatio = (m_scrollOffset.y * scale.y + maxScrollY * scrollOffsetAnchor.y) / maxScrollY;
							const double y = scrolledRatio * (viewHeight - h);
							const double thickness = 4.0 * scale.x;

							const RectF unrotated = unrotatedTransformedRect();
							const RectF backgroundRect
							{
								unrotated.x + unrotated.w - thickness,
								unrotated.y,
								thickness,
								unrotated.h
							};
							backgroundRect.rounded(roundRadius).draw(ColorF{ 0.0, m_scrollBarAlpha.currentValue() });

							verticalHandleRect = RectF
							{
								unrotated.x + unrotated.w - thickness,
								unrotated.y + y,
								thickness,
								h
							};
						}
					}

					// ハンドル部分を描画
					if (horizontalHandleRect)
					{
						horizontalHandleRect->rounded(roundRadius).draw(ColorF{ 1.0, m_scrollBarAlpha.currentValue() });
					}
					if (verticalHandleRect)
					{
						verticalHandleRect->rounded(roundRadius).draw(ColorF{ 1.0, m_scrollBarAlpha.currentValue() });
					}
				}
			}
		}
	}

	void Node::requestClick()
	{
		// m_isHitTargetがfalseでも効く仕様とする
		if (!m_activeInHierarchy)
		{
			return;
		}
		m_clickRequested = true;
	}

	void Node::requestRightClick()
	{
		// m_isHitTargetがfalseでも効く仕様とする
		if (!m_activeInHierarchy)
		{
			return;
		}
		m_rightClickRequested = true;
	}

	const String& Node::name() const
	{
		return m_name;
	}

	std::shared_ptr<Node> Node::setName(StringView name)
	{
		m_name = name;
		return shared_from_this();
	}

	double Node::extractRotationFromTransformMat() const
	{
		// 変換行列から回転角度を抽出
		const double scaleX = Math::Sqrt(m_transformMatInHierarchy._11 * m_transformMatInHierarchy._11 + 
		                                  m_transformMatInHierarchy._21 * m_transformMatInHierarchy._21);
		if (scaleX < 0.0001) 
		{
			return 0.0;
		}
		
		// atan2で角度を計算（ラジアン）
		return Math::Atan2(m_transformMatInHierarchy._21 / scaleX, m_transformMatInHierarchy._11 / scaleX);
	}
	
	RectF Node::unrotatedTransformedRect() const
	{	
		// 変換行列から回転成分を抽出
		const double rotation = Math::Atan2(m_transformMatInHierarchy._21, m_transformMatInHierarchy._11);
		
		// Quadの中心を軸に逆回転
		const Vec2 center = (m_transformedQuad.p0 + m_transformedQuad.p1 + m_transformedQuad.p2 + m_transformedQuad.p3) / 4;
		const Mat3x2 inverseRotationMat = Mat3x2::Rotate(-rotation, center);
		const Vec2 p0 = inverseRotationMat.transformPoint(m_transformedQuad.p0);
		const Vec2 p1 = inverseRotationMat.transformPoint(m_transformedQuad.p1);
		const Vec2 p2 = inverseRotationMat.transformPoint(m_transformedQuad.p2);
		const Vec2 p3 = inverseRotationMat.transformPoint(m_transformedQuad.p3);
		
		// スケールの縦横比が1:1でなく平行四辺形になる場合用に、幅と高さは対辺の中点を結んで算出
		const Vec2 midTop = (p0 + p1) / 2;
		const Vec2 midBottom = (p2 + p3) / 2;
		const Vec2 midLeft = (p0 + p3) / 2;
		const Vec2 midRight = (p1 + p2) / 2;
		const double width = (midRight - midLeft).length();
		const double height = (midBottom - midTop).length();
		
		return RectF{ Arg::center = center, width, height };
	}

	Mat3x2 Node::calculateHitTestMat(const Mat3x2& parentHitTestMat) const
	{
		if (m_transform.affectsHitTest().value())
		{
			// TransformをHitTestに適用
			const Vec2& scale = m_transform.scale().value();
			const Vec2& pivot = m_transform.pivot().value();
			const Vec2& translate = m_transform.translate().value();
			const double rotation = m_transform.rotation().value();
			
			const Vec2 pivotPos = m_regionRect.pos + m_regionRect.size * pivot;
			
			// 自身の変換行列を構築（適用順: Scale → Rotate → Translate）
			Mat3x2 selfTransform = Mat3x2::Scale(scale, pivotPos);
			
			if (rotation != 0.0)
			{
				selfTransform = selfTransform * Mat3x2::Rotate(Math::ToRadians(rotation), pivotPos);
			}
			
			selfTransform = selfTransform * Mat3x2::Translate(translate);
			
			return selfTransform * parentHitTestMat;
		}
		else
		{
			return parentHitTestMat;
		}
	}

	Vec2 Node::transformScaleInHierarchy() const
	{
		// 変換行列からスケールを抽出
		const double scaleX = std::sqrt(m_transformMatInHierarchy._11 * m_transformMatInHierarchy._11 + 
		                                m_transformMatInHierarchy._21 * m_transformMatInHierarchy._21);
		const double scaleY = std::sqrt(m_transformMatInHierarchy._12 * m_transformMatInHierarchy._12 + 
		                                m_transformMatInHierarchy._22 * m_transformMatInHierarchy._22);
		
		return Vec2{ scaleX, scaleY };
	}

	const Mat3x2& Node::hitTestMatInHierarchy() const
	{
		return m_hitTestMatInHierarchy;
	}

	Vec2 Node::inverseTransformHitTestPoint(const Vec2& point) const
	{
		const Mat3x2& hitTestMat = m_hitTestMatInHierarchy;
		if (hitTestMat != Mat3x2::Identity())
		{
			const Mat3x2 invHitTestMat = hitTestMat.inverse();
			return invHitTestMat.transformPoint(point);
		}
		else
		{
			return point;
		}
	}

	Vec2 Node::transformPivotPos() const
	{
		const Vec2& pivot = m_transform.pivot().value();
		const RectF unrotated = unrotatedTransformedRect();
		return unrotated.pos + unrotated.size * pivot;
	}

	const Quad& Node::transformedQuad() const
	{
		return m_transformedQuad;
	}

	Quad Node::hitQuad(WithPaddingYN withPadding) const
	{
		if (withPadding == WithPaddingYN::Yes)
		{
			return m_hitQuadWithPadding;
		}
		else
		{
			return m_hitQuad;
		}
	}

	const RectF& Node::regionRect() const
	{
		return m_regionRect;
	}

	const Mat3x2& Node::transformMatInHierarchy() const
	{
		return m_transformMatInHierarchy;
	}

	RectF Node::regionRectWithMargin() const
	{
		if (const InlineRegion* pInlineRegion = inlineRegion())
		{
			const LRTB& margin = pInlineRegion->margin;
			return RectF
			{
				m_regionRect.x - margin.left,
				m_regionRect.y - margin.top,
				m_regionRect.w + margin.left + margin.right,
				m_regionRect.h + margin.top + margin.bottom,
			};
		}
		else
		{
			return m_regionRect;
		}
	}


	bool Node::hasChildren() const
	{
		return !m_children.isEmpty();
	}

	const Array<std::shared_ptr<ComponentBase>>& Node::components() const
	{
		return m_components;
	}

	InteractableYN Node::interactable() const
	{
		return m_interactable;
	}

	std::shared_ptr<Node> Node::setInteractable(InteractableYN interactable)
	{
		m_interactable = interactable;
		m_mouseLTracker.setInteractable(interactable);
		m_mouseRTracker.setInteractable(interactable);
		return shared_from_this();
	}

	std::shared_ptr<Node> Node::setInteractable(bool interactable)
	{
		return setInteractable(InteractableYN{ interactable });
	}

	bool Node::interactableInHierarchy() const
	{
		return m_currentInteractionState != InteractionState::Disabled;
	}

	// TODO: boolで返す方が良さそう
	ActiveYN Node::activeSelf() const
	{
		return m_activeSelf;
	}

	std::shared_ptr<Node> Node::setActive(ActiveYN activeSelf, RefreshesLayoutYN refreshesLayout)
	{
		m_activeSelf = activeSelf;
		refreshActiveInHierarchy();
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
		return shared_from_this();
	}

	std::shared_ptr<Node> Node::setActive(bool activeSelf, RefreshesLayoutYN refreshesLayout)
	{
		return setActive(ActiveYN{ activeSelf }, refreshesLayout);
	}

	ActiveYN Node::activeInHierarchy() const
	{
		return m_activeInHierarchy;
	}

	IsHitTargetYN Node::isHitTarget() const
	{
		return m_isHitTarget;
	}

	std::shared_ptr<Node> Node::setIsHitTarget(IsHitTargetYN isHitTarget)
	{
		m_isHitTarget = isHitTarget;
		return shared_from_this();
	}

	std::shared_ptr<Node> Node::setIsHitTarget(bool isHitTarget)
	{
		return setIsHitTarget(IsHitTargetYN{ isHitTarget });
	}

	const LRTB& Node::hitPadding() const
	{
		return m_hitPadding;
	}

	std::shared_ptr<Node> Node::setHitPadding(const LRTB& padding)
	{
		m_hitPadding = padding;
		return shared_from_this();
	}

	InheritChildrenStateFlags Node::inheritChildrenStateFlags() const
	{
		return m_inheritChildrenStateFlags;
	}

	std::shared_ptr<Node> Node::setInheritChildrenStateFlags(InheritChildrenStateFlags flags)
	{
		m_inheritChildrenStateFlags = flags;
		return shared_from_this();
	}

	bool Node::inheritsChildrenHover() const
	{
		return HasFlag(m_inheritChildrenStateFlags, InheritChildrenStateFlags::Hovered);
	}

	std::shared_ptr<Node> Node::setInheritsChildrenHover(bool value)
	{
		if (value)
		{
			m_inheritChildrenStateFlags |= InheritChildrenStateFlags::Hovered;
		}
		else
		{
			m_inheritChildrenStateFlags &= ~InheritChildrenStateFlags::Hovered;
		}
		return shared_from_this();
	}

	bool Node::inheritsChildrenPress() const
	{
		return HasFlag(m_inheritChildrenStateFlags, InheritChildrenStateFlags::Pressed);
	}

	std::shared_ptr<Node> Node::setInheritsChildrenPress(bool value)
	{
		if (value)
		{
			m_inheritChildrenStateFlags |= InheritChildrenStateFlags::Pressed;
		}
		else
		{
			m_inheritChildrenStateFlags &= ~InheritChildrenStateFlags::Pressed;
		}
		return shared_from_this();
	}

	ScrollableAxisFlags Node::scrollableAxisFlags() const
	{
		return m_scrollableAxisFlags;
	}

	std::shared_ptr<Node> Node::setScrollableAxisFlags(ScrollableAxisFlags flags, RefreshesLayoutYN refreshesLayout)
	{
		m_scrollableAxisFlags = flags;
		if (!horizontalScrollable())
		{
			m_scrollOffset.x = 0.0;
		}
		if (!verticalScrollable())
		{
			m_scrollOffset.y = 0.0;
		}
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
		return shared_from_this();
	}

	bool Node::horizontalScrollable() const
	{
		return HasFlag(m_scrollableAxisFlags, ScrollableAxisFlags::Horizontal);
	}

	std::shared_ptr<Node> Node::setHorizontalScrollable(bool scrollable, RefreshesLayoutYN refreshesLayout)
	{
		if (scrollable)
		{
			m_scrollableAxisFlags |= ScrollableAxisFlags::Horizontal;
		}
		else
		{
			m_scrollableAxisFlags &= ~ScrollableAxisFlags::Horizontal;
			m_scrollOffset.x = 0.0;
		}
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
		return shared_from_this();
	}

	bool Node::verticalScrollable() const
	{
		return HasFlag(m_scrollableAxisFlags, ScrollableAxisFlags::Vertical);
	}

	std::shared_ptr<Node> Node::setVerticalScrollable(bool scrollable, RefreshesLayoutYN refreshesLayout)
	{
		if (scrollable)
		{
			m_scrollableAxisFlags |= ScrollableAxisFlags::Vertical;
		}
		else
		{
			m_scrollableAxisFlags &= ~ScrollableAxisFlags::Vertical;
			m_scrollOffset.y = 0.0;
		}
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
		return shared_from_this();
	}

	ClippingEnabledYN Node::clippingEnabled() const
	{
		return m_clippingEnabled;
	}

	std::shared_ptr<Node> Node::setClippingEnabled(ClippingEnabledYN clippingEnabled)
	{
		m_clippingEnabled = clippingEnabled;
		return shared_from_this();
	}

	std::shared_ptr<Node> Node::setClippingEnabled(bool clippingEnabled)
	{
		return setClippingEnabled(ClippingEnabledYN{ clippingEnabled });
	}
	
	ScrollMethodFlags Node::scrollMethodFlags() const
	{
		return m_scrollMethodFlags;
	}
	
	std::shared_ptr<Node> Node::setScrollMethodFlags(ScrollMethodFlags flags)
	{
		m_scrollMethodFlags = flags;
		return shared_from_this();
	}
	
	bool Node::wheelScrollEnabled() const
	{
		return HasFlag(m_scrollMethodFlags, ScrollMethodFlags::Wheel);
	}
	
	std::shared_ptr<Node> Node::setWheelScrollEnabled(bool enabled)
	{
		if (enabled)
		{
			m_scrollMethodFlags |= ScrollMethodFlags::Wheel;
		}
		else
		{
			m_scrollMethodFlags &= ~ScrollMethodFlags::Wheel;
		}
		return shared_from_this();
	}
	
	bool Node::dragScrollEnabled() const
	{
		return HasFlag(m_scrollMethodFlags, ScrollMethodFlags::Drag);
	}
	
	std::shared_ptr<Node> Node::setDragScrollEnabled(bool enabled)
	{
		if (enabled)
		{
			m_scrollMethodFlags |= ScrollMethodFlags::Drag;
		}
		else
		{
			m_scrollMethodFlags &= ~ScrollMethodFlags::Drag;
		}
		return shared_from_this();
	}
	
	double Node::decelerationRate() const
	{
		return m_decelerationRate;
	}
	
	std::shared_ptr<Node> Node::setDecelerationRate(double rate)
	{
		m_decelerationRate = rate;
		return shared_from_this();
	}

	RubberBandScrollEnabledYN Node::rubberBandScrollEnabled() const
	{
		return m_rubberBandScrollEnabled;
	}

	std::shared_ptr<Node> Node::setRubberBandScrollEnabled(RubberBandScrollEnabledYN rubberBandScrollEnabled)
	{
		m_rubberBandScrollEnabled = rubberBandScrollEnabled;
		return shared_from_this();
	}

	std::shared_ptr<Node> Node::setRubberBandScrollEnabled(bool rubberBandScrollEnabled)
	{
		return setRubberBandScrollEnabled(rubberBandScrollEnabled ? RubberBandScrollEnabledYN::Yes : RubberBandScrollEnabledYN::No);
	}

	void Node::preventDragScroll()
	{
		if (!MouseL.pressed())
		{
			return;
		}
		
		m_preventDragScroll = true;
		
		// 親ノードにも再帰的に適用
		if (const auto parentNode = m_parent.lock())
		{
			parentNode->preventDragScroll();
		}
	}

	InteractionState Node::interactionStateSelf() const
	{
		return m_mouseLTracker.interactionStateSelf();
	}

	InteractionState Node::currentInteractionState() const
	{
		return m_currentInteractionState;
	}


	bool Node::isHovered(RecursiveYN recursive, IncludingDisabledYN includingDisabled) const
	{
		if (!includingDisabled && m_currentInteractionState == InteractionState::Disabled)
		{
			return false;
		}
		if (m_mouseLTracker.isHovered(includingDisabled))
		{
			return true;
		}
		if (recursive)
		{
			return m_children.any([includingDisabled](const auto& child) { return child->isHovered(RecursiveYN::Yes, includingDisabled); });
		}
		return false;
	}

	bool Node::isPressed(RecursiveYN recursive, IncludingDisabledYN includingDisabled) const
	{
		if (!includingDisabled && m_currentInteractionState == InteractionState::Disabled)
		{
			return false;
		}
		if (m_mouseLTracker.isPressed(includingDisabled))
		{
			return true;
		}
		if (recursive)
		{
			return m_children.any([includingDisabled](const auto& child) { return child->isPressed(RecursiveYN::Yes, includingDisabled); });
		}
		return false;
	}

	bool Node::isPressedHover(RecursiveYN recursive, IncludingDisabledYN includingDisabled) const
	{
		if (!includingDisabled && m_currentInteractionState == InteractionState::Disabled)
		{
			return false;
		}
		if (m_mouseLTracker.isPressedHover(includingDisabled))
		{
			return true;
		}
		if (recursive)
		{
			return m_children.any([includingDisabled](const auto& child) { return child->isPressedHover(RecursiveYN::Yes, includingDisabled); });
		}
		return false;
	}

	bool Node::isMouseDown(RecursiveYN recursive, IncludingDisabledYN includingDisabled) const
	{
		if (!includingDisabled && m_currentInteractionState == InteractionState::Disabled)
		{
			return false;
		}
		if (m_mouseLTracker.isHovered(includingDisabled) && MouseL.down())
		{
			return true;
		}
		if (recursive)
		{
			return m_children.any([includingDisabled](const auto& child) { return child->isMouseDown(RecursiveYN::Yes, includingDisabled); });
		}
		return false;
	}

	bool Node::isClicked(RecursiveYN recursive, IncludingDisabledYN includingDisabled) const
	{
		if (!includingDisabled && m_currentInteractionState == InteractionState::Disabled)
		{
			return false;
		}
		if (m_prevClickRequested || m_mouseLTracker.isClicked(includingDisabled))
		{
			return true;
		}
		if (recursive)
		{
			return m_children.any([includingDisabled](const auto& child) { return child->isClicked(RecursiveYN::Yes, includingDisabled); });
		}
		return false;
	}

	bool Node::isClickRequested(RecursiveYN recursive, IncludingDisabledYN includingDisabled) const
	{
		if (!includingDisabled && m_currentInteractionState == InteractionState::Disabled)
		{
			return false;
		}
		if (m_prevClickRequested)
		{
			return true;
		}
		if (recursive)
		{
			return m_children.any([includingDisabled](const auto& child) { return child->isClickRequested(RecursiveYN::Yes, includingDisabled); });
		}
		return false;
	}

	bool Node::isRightPressed(RecursiveYN recursive, IncludingDisabledYN includingDisabled) const
	{
		if (!includingDisabled && m_currentInteractionState == InteractionState::Disabled)
		{
			return false;
		}
		if (m_mouseRTracker.isPressed(includingDisabled))
		{
			return true;
		}
		if (recursive)
		{
			return m_children.any([includingDisabled](const auto& child) { return child->isRightPressed(RecursiveYN::Yes, includingDisabled); });
		}
		return false;
	}

	bool Node::isRightPressedHover(RecursiveYN recursive, IncludingDisabledYN includingDisabled) const
	{
		if (!includingDisabled && m_currentInteractionState == InteractionState::Disabled)
		{
			return false;
		}
		if (m_mouseRTracker.isPressedHover(includingDisabled))
		{
			return true;
		}
		if (recursive)
		{
			return m_children.any([includingDisabled](const auto& child) { return child->isRightPressedHover(RecursiveYN::Yes, includingDisabled); });
		}
		return false;
	}

	bool Node::isRightMouseDown(RecursiveYN recursive, IncludingDisabledYN includingDisabled) const
	{
		if (!includingDisabled && m_currentInteractionState == InteractionState::Disabled)
		{
			return false;
		}
		if (m_mouseRTracker.isHovered(includingDisabled) && MouseR.down())
		{
			return true;
		}
		if (recursive)
		{
			return m_children.any([includingDisabled](const auto& child) { return child->isRightMouseDown(RecursiveYN::Yes, includingDisabled); });
		}
		return false;
	}

	bool Node::isRightClicked(RecursiveYN recursive, IncludingDisabledYN includingDisabled) const
	{
		if (!includingDisabled && m_currentInteractionState == InteractionState::Disabled)
		{
			return false;
		}
		if (m_prevRightClickRequested || m_mouseRTracker.isClicked(includingDisabled))
		{
			return true;
		}
		if (recursive)
		{
			return m_children.any([includingDisabled](const auto& child) { return child->isRightClicked(RecursiveYN::Yes, includingDisabled); });
		}
		return false;
	}

	bool Node::isRightClickRequested(RecursiveYN recursive, IncludingDisabledYN includingDisabled) const
	{
		if (!includingDisabled && m_currentInteractionState == InteractionState::Disabled)
		{
			return false;
		}
		if (m_prevRightClickRequested)
		{
			return true;
		}
		if (recursive)
		{
			return m_children.any([includingDisabled](const auto& child) { return child->isRightClickRequested(RecursiveYN::Yes, includingDisabled); });
		}
		return false;
	}

	void Node::removeChildrenAll(RefreshesLayoutYN refreshesLayout)
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
			refreshContainedCanvasLayout();
		}
	}

	void Node::swapChildren(const std::shared_ptr<Node>& child1, const std::shared_ptr<Node>& child2, RefreshesLayoutYN refreshesLayout)
	{
		const auto it1 = std::find(m_children.begin(), m_children.end(), child1);
		const auto it2 = std::find(m_children.begin(), m_children.end(), child2);
		if (it1 == m_children.end() || it2 == m_children.end())
		{
			throw Error{ U"swapChildren: Child node not found in node '{}'"_fmt(m_name) };
		}
		std::iter_swap(it1, it2);
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
	}

	void Node::swapChildren(size_t index1, size_t index2, RefreshesLayoutYN refreshesLayout)
	{
		if (index1 >= m_children.size() || index2 >= m_children.size())
		{
			throw Error{ U"swapChildren: Index out of range" };
		}
		std::iter_swap(m_children.begin() + index1, m_children.begin() + index2);
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
	}

	size_t Node::indexOfChild(const std::shared_ptr<Node>& child) const
	{
		const auto it = std::find(m_children.begin(), m_children.end(), child);
		if (it == m_children.end())
		{
			throw Error{ U"indexOfChild: Child node not found in node '{}'"_fmt(m_name) };
		}
		return static_cast<size_t>(std::distance(m_children.begin(), it));
	}

	Optional<size_t> Node::indexOfChildOpt(const std::shared_ptr<Node>& child) const
	{
		const auto it = std::find(m_children.begin(), m_children.end(), child);
		if (it == m_children.end())
		{
			return none;
		}
		return static_cast<size_t>(std::distance(m_children.begin(), it));
	}

	std::shared_ptr<Node> Node::clone() const
	{
		return CreateFromJSON(toJSON());
	}

	std::shared_ptr<Node> Node::childAt(size_t index) const
	{
		if (index >= m_children.size())
		{
			return nullptr;
		}
		return m_children[index];
	}

	std::shared_ptr<Node> Node::addKeyInputUpdater(std::function<void(const std::shared_ptr<Node>&)> keyInputUpdater)
	{
		emplaceComponent<KeyInputUpdaterComponent>(std::move(keyInputUpdater));
		return shared_from_this();
	}

	std::shared_ptr<Node> Node::addUpdater(std::function<void(const std::shared_ptr<Node>&)> updater)
	{
		emplaceComponent<UpdaterComponent>(std::move(updater));
		return shared_from_this();
	}

	std::shared_ptr<Node> Node::addDrawer(std::function<void(const Node&)> drawer)
	{
		emplaceComponent<DrawerComponent>(std::move(drawer));
		return shared_from_this();
	}

	std::shared_ptr<Node> Node::addOnClick(std::function<void(const std::shared_ptr<Node>&)> onClick)
	{
		emplaceComponent<UpdaterComponent>([onClick = std::move(onClick)](const std::shared_ptr<Node>& node)
			{
				if (node->isClicked())
				{
					onClick(node);
				}
			});
		return shared_from_this();
	}

	std::shared_ptr<Node> Node::addOnClick(std::function<void()> onClick)
	{
		emplaceComponent<UpdaterComponent>([onClick = std::move(onClick)](const std::shared_ptr<Node>& node)
			{
				if (node->isClicked())
				{
					onClick();
				}
			});
		return shared_from_this();
	}

	std::shared_ptr<Node> Node::addOnRightClick(std::function<void(const std::shared_ptr<Node>&)> onRightClick)
	{
		emplaceComponent<UpdaterComponent>([onRightClick = std::move(onRightClick)](const std::shared_ptr<Node>& node)
			{
				if (node->isRightClicked())
				{
					onRightClick(node);
				}
			});
		return shared_from_this();
	}

	std::shared_ptr<Node> Node::addOnRightClick(std::function<void()> onRightClick)
	{
		emplaceComponent<UpdaterComponent>([onRightClick = std::move(onRightClick)](const std::shared_ptr<Node>& node)
			{
				if (node->isRightClicked())
				{
					onRightClick();
				}
			});
		return shared_from_this();
	}

	std::shared_ptr<Node> Node::addClickHotKey(const Input& input, EnabledWhileTextEditingYN enabledWhileTextEditing, ClearsInputYN clearsInput)
	{
		addClickHotKey(input, CtrlYN::No, AltYN::No, ShiftYN::No, enabledWhileTextEditing, clearsInput);
		return shared_from_this();
	}

	std::shared_ptr<Node> Node::addClickHotKey(const Input& input, CtrlYN ctrl, AltYN alt, ShiftYN shift, EnabledWhileTextEditingYN enabledWhileTextEditing, ClearsInputYN clearsInput)
	{
		emplaceComponent<HotKeyInputHandler>(input, ctrl, alt, shift, HotKeyTarget::Click, enabledWhileTextEditing, clearsInput);
		return shared_from_this();
	}

	std::shared_ptr<Node> Node::addRightClickHotKey(const Input& input, EnabledWhileTextEditingYN enabledWhileTextEditing, ClearsInputYN clearsInput)
	{
		addRightClickHotKey(input, CtrlYN::No, AltYN::No, ShiftYN::No, enabledWhileTextEditing, clearsInput);
		return shared_from_this();
	}

	std::shared_ptr<Node> Node::addRightClickHotKey(const Input& input, CtrlYN ctrl, AltYN alt, ShiftYN shift, EnabledWhileTextEditingYN enabledWhileTextEditing, ClearsInputYN clearsInput)
	{
		emplaceComponent<HotKeyInputHandler>(input, ctrl, alt, shift, HotKeyTarget::RightClick, enabledWhileTextEditing, clearsInput);
		return shared_from_this();
	}

	void Node::refreshContainedCanvasLayout()
	{
		if (const auto canvas = m_canvas.lock())
		{
			canvas->refreshLayout();
		}
	}
}
