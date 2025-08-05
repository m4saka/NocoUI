#include "NocoUI/Node.hpp"
#include "NocoUI/Serialization.hpp"
#include "NocoUI/Canvas.hpp"
#include "NocoUI/Component/Component.hpp"
#include "NocoUI/detail/ScopedScissorRect.hpp"

namespace
{
	RectF CreateRectFromPoints(const Vec2& point1, const Vec2& point2)
	{
		const double minX = Min(point1.x, point2.x);
		const double maxX = Max(point1.x, point2.x);
		const double minY = Min(point1.y, point2.y);
		const double maxY = Max(point1.y, point2.y);
		return RectF{ minX, minY, maxX - minX, maxY - minY };
	}
}

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
					if (!inheritsChildrenPressedState() && childInteractionState == InteractionState::Pressed)
					{
						childInteractionState = InteractionState::Hovered;
					}
					if (!inheritsChildrenHoveredState() && childInteractionState == InteractionState::Hovered)
					{
						childInteractionState = InteractionState::Default;
					}
					if (inheritsChildrenPressedState() && child->isClicked())
					{
						inheritedIsClicked = true;
					}
					inheritedInteractionState = ApplyOtherInteractionState(inheritedInteractionState, childInteractionState, AppliesDisabledStateYN::No);
				}
			}
		}
		const bool onClientRect = Cursor::OnClientRect();
		const bool mouseOverForHovered = onClientRect && m_activeInHierarchy && (hoveredNode.get() == this || (inheritsChildrenHoveredState() && (inheritedInteractionState == InteractionState::Hovered || inheritedInteractionState == InteractionState::Pressed)));
		const bool mouseOverForPressed = onClientRect && m_activeInHierarchy && (hoveredNode.get() == this || (inheritsChildrenPressedState() && (inheritedInteractionState == InteractionState::Pressed || inheritedIsClicked))); // クリック判定用に離した瞬間もホバー扱いにする必要があるため、子のisClickedも加味している
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
					if (!inheritsChildrenPressedState() && childInteractionState == InteractionState::Pressed)
					{
						childInteractionState = InteractionState::Hovered;
					}
					if (!inheritsChildrenHoveredState() && childInteractionState == InteractionState::Hovered)
					{
						childInteractionState = InteractionState::Default;
					}
					if (inheritsChildrenPressedState() && child->isRightClicked())
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
			const bool mouseOverForHovered = onClientRect && m_activeInHierarchy && (hoveredNode.get() == this || (inheritsChildrenHoveredState() && (inheritedInteractionState == InteractionState::Hovered || inheritedInteractionState == InteractionState::Pressed)));
			const bool mouseOverForPressed = onClientRect && m_activeInHierarchy && (hoveredNode.get() == this || (inheritsChildrenPressedState() && (inheritedInteractionState == InteractionState::Pressed || inheritedIsRightClicked))); // クリック判定用に離した瞬間もホバー扱いにする必要があるため、子のisRightClickedも加味している
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
		if (const auto parent = m_parent.lock())
		{
			m_activeInHierarchy = ActiveYN{ m_activeSelf && parent->m_activeInHierarchy };
		}
		else
		{
			m_activeInHierarchy = m_activeSelf;
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

		const Optional<RectF> contentRectOpt = getBoxChildrenContentRectWithPadding();
		if (!contentRectOpt)
		{
			m_scrollOffset = Vec2::Zero();
			return;
		}

		const RectF& contentRect = *contentRectOpt;

		const double viewWidth = m_layoutAppliedRect.w;
		const double viewHeight = m_layoutAppliedRect.h;
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
			const Vec2 scrollOffsetAnchor = std::visit([](const auto& layout) { return layout.scrollOffsetAnchor(); }, m_boxChildrenLayout);
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
				const Optional<RectF> contentRectOpt = getBoxChildrenContentRectWithPadding();
				if (contentRectOpt)
				{
					const RectF& contentRect = *contentRectOpt;
					const double viewWidth = m_layoutAppliedRect.w;
					const double viewHeight = m_layoutAppliedRect.h;
					const double maxScrollX = Max(contentRect.w - viewWidth, 0.0);
					const double maxScrollY = Max(contentRect.h - viewHeight, 0.0);
					
					const Vec2 scrollOffsetAnchor = std::visit([](const auto& layout) { return layout.scrollOffsetAnchor(); }, m_boxChildrenLayout);
					
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

	std::shared_ptr<Node> Node::Create(StringView name, const ConstraintVariant& constraint, IsHitTargetYN isHitTarget, InheritChildrenStateFlags inheritChildrenStateFlags)
	{
		return std::shared_ptr<Node>{ new Node{ s_nextInternalId++, name, constraint, isHitTarget, inheritChildrenStateFlags } };
	}

	const ConstraintVariant& Node::constraint() const
	{
		return m_constraint;
	}

	std::shared_ptr<Node> Node::setConstraint(const ConstraintVariant& constraint, RefreshesLayoutYN refreshesLayout)
	{
		m_constraint = constraint;
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
		return shared_from_this();
	}

	const BoxConstraint* Node::boxConstraint() const
	{
		return std::get_if<BoxConstraint>(&m_constraint);
	}

	const AnchorConstraint* Node::anchorConstraint() const
	{
		return std::get_if<AnchorConstraint>(&m_constraint);
	}

	TransformEffect& Node::transformEffect()
	{
		return m_transformEffect;
	}

	const TransformEffect& Node::transformEffect() const
	{
		return m_transformEffect;
	}

	const LayoutVariant& Node::boxChildrenLayout() const
	{
		return m_boxChildrenLayout;
	}

	std::shared_ptr<Node> Node::setBoxChildrenLayout(const LayoutVariant& layout, RefreshesLayoutYN refreshesLayout)
	{
		m_boxChildrenLayout = layout;
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
		return shared_from_this();
	}

	const FlowLayout* Node::childrenFlowLayout() const
	{
		return std::get_if<FlowLayout>(&m_boxChildrenLayout);
	}

	const HorizontalLayout* Node::childrenHorizontalLayout() const
	{
		return std::get_if<HorizontalLayout>(&m_boxChildrenLayout);
	}

	const VerticalLayout* Node::childrenVerticalLayout() const
	{
		return std::get_if<VerticalLayout>(&m_boxChildrenLayout);
	}

	SizeF Node::getFittingSizeToChildren() const
	{
		return std::visit([this](const auto& layout) { return layout.getFittingSizeToChildren(m_layoutAppliedRect, m_children); }, m_boxChildrenLayout);
	}

	std::shared_ptr<Node> Node::setBoxConstraintToFitToChildren(FitTarget fitTarget, RefreshesLayoutYN refreshesLayout)
	{
		std::visit([this, fitTarget, refreshesLayout](auto& layout) { layout.setBoxConstraintToFitToChildren(m_layoutAppliedRect, m_children, *this, fitTarget, refreshesLayout); }, m_boxChildrenLayout);
		return shared_from_this();
	}

	const LRTB& Node::boxChildrenLayoutPadding() const
	{
		return std::visit([](const auto& layout) -> const LRTB& { return layout.padding; }, m_boxChildrenLayout);
	}

	bool Node::hasBoxConstraint() const
	{
		return std::holds_alternative<BoxConstraint>(m_constraint);
	}

	bool Node::hasAnchorConstraint() const
	{
		return std::holds_alternative<AnchorConstraint>(m_constraint);
	}

	JSON Node::toJSON() const
	{
		return toJSONImpl(detail::IncludesInternalIdYN::No);
	}

	JSON Node::toJSONImpl(detail::IncludesInternalIdYN includesInternalId) const
	{
		Array<JSON> childrenJSON;
		for (const auto& child : m_children)
		{
			childrenJSON.push_back(child->toJSONImpl(includesInternalId));
		}

		JSON result
		{
			{ U"name", m_name },
			{ U"constraint", std::visit([](const auto& constraint) { return constraint.toJSON(); }, m_constraint) },
			{ U"transformEffect", m_transformEffect.toJSON() },
			{ U"boxChildrenLayout", std::visit([](const auto& boxChildrenLayout) { return boxChildrenLayout.toJSON(); }, m_boxChildrenLayout) },
			{ U"components", Array<JSON>{} },
			{ U"children", childrenJSON },
			{ U"isHitTarget", m_isHitTarget.getBool() },
			{ U"hitTestPadding", m_hitTestPadding.toJSON() },
			{ U"inheritsChildrenHoveredState", inheritsChildrenHoveredState() },
			{ U"inheritsChildrenPressedState", inheritsChildrenPressedState() },
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

		if (includesInternalId)
		{
			result[U"_internalId"] = m_internalId;
		}

		for (const std::shared_ptr<ComponentBase>& component : m_components)
		{
			if (const auto serializableComponent = std::dynamic_pointer_cast<SerializableComponentBase>(component))
			{
				result[U"components"].push_back(serializableComponent->toJSONImpl(includesInternalId));
			}
		}

		return result;
	}

	std::shared_ptr<Node> Node::CreateFromJSON(const JSON& json)
	{
		return CreateFromJSONImpl(json, detail::IncludesInternalIdYN::No);
	}

	std::shared_ptr<Node> Node::CreateFromJSONImpl(const JSON& json, detail::IncludesInternalIdYN includesInternalId)
	{
		auto node = Node::Create();
		if (json.contains(U"name"))
		{
			node->m_name = json[U"name"].getOr<String>(U"");
		}
		if (json.contains(U"constraint") && json[U"constraint"].contains(U"type"))
		{
			const auto type = json[U"constraint"][U"type"].getString();
			if (type == U"AnchorConstraint")
			{
				node->m_constraint = AnchorConstraint::FromJSON(json[U"constraint"]);
			}
			else if (type == U"BoxConstraint")
			{
				node->m_constraint = BoxConstraint::FromJSON(json[U"constraint"]);
			}
			else
			{
				// 不明な場合はBoxConstraint扱いにする
				Logger << U"[NocoUI warning] Unknown constraint type: '{}'"_fmt(type);
				node->m_constraint = BoxConstraint{};
			}
		}
		if (json.contains(U"transformEffect"))
		{
			node->m_transformEffect.readFromJSON(json[U"transformEffect"]);
		}
		if (json.contains(U"boxChildrenLayout") && json[U"boxChildrenLayout"].contains(U"type"))
		{
			const auto type = json[U"boxChildrenLayout"][U"type"].getString();
			if (type == U"FlowLayout")
			{
				node->m_boxChildrenLayout = FlowLayout::FromJSON(json[U"boxChildrenLayout"]);
			}
			else if (type == U"HorizontalLayout")
			{
				node->m_boxChildrenLayout = HorizontalLayout::FromJSON(json[U"boxChildrenLayout"]);
			}
			else if (type == U"VerticalLayout")
			{
				node->m_boxChildrenLayout = VerticalLayout::FromJSON(json[U"boxChildrenLayout"]);
			}
			else
			{
				// 不明な場合はFlowLayout扱いにする
				Logger << U"[NocoUI warning] Unknown box children layout type: '{}'"_fmt(type);
				node->m_boxChildrenLayout = FlowLayout{};
			}
		}
		if (json.contains(U"isHitTarget"))
		{
			node->m_isHitTarget = IsHitTargetYN{ json[U"isHitTarget"].getOr<bool>(true) };
		}
		if (json.contains(U"hitTestPadding"))
		{
			node->m_hitTestPadding = LRTB::fromJSON(json[U"hitTestPadding"]);
		}
		if (json.contains(U"inheritsChildrenHoveredState"))
		{
			node->setInheritsChildrenHoveredState(json[U"inheritsChildrenHoveredState"].getOr<bool>(false));
		}
		if (json.contains(U"inheritsChildrenPressedState"))
		{
			node->setInheritsChildrenPressedState(json[U"inheritsChildrenPressedState"].getOr<bool>(false));
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
		if (includesInternalId && json.contains(U"_internalId"))
		{
			node->m_internalId = json[U"_internalId"].get<uint64>();
		}

		if (json.contains(U"components") && json[U"components"].isArray())
		{
			for (const auto& componentJSON : json[U"components"].arrayView())
			{
				node->addComponentFromJSONImpl(componentJSON, includesInternalId);
			}
		}

		if (json.contains(U"children") && json[U"children"].isArray())
		{
			for (const auto& childJSON : json[U"children"].arrayView())
			{
				auto child = CreateFromJSONImpl(childJSON, includesInternalId);
				node->addChild(child, RefreshesLayoutYN::No);
			}
		}
		return node;
	}


	std::shared_ptr<Node> Node::parent() const
	{
		return m_parent.lock();
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
		if (!m_parent.expired())
		{
			removeFromParent(RefreshesLayoutYN{ refreshesLayout && parent->m_canvas.lock() != m_canvas.lock() });
		}
		parent->addChild(shared_from_this(), refreshesLayout);
		return shared_from_this();
	}

	bool Node::removeFromParent(RefreshesLayoutYN refreshesLayout)
	{
		if (const auto parent = m_parent.lock())
		{
			parent->removeChild(shared_from_this(), refreshesLayout);
			return true;
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
		else
		{
			throw Error{ U"Node::siblingIndex: Node '{}' has no parent"_fmt(m_name) };
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
		return none;
	}

	std::shared_ptr<Canvas> Node::containedCanvas() const
	{
		return m_canvas.lock();
	}

	std::shared_ptr<ComponentBase> Node::addComponentFromJSON(const JSON& json)
	{
		return addComponentFromJSONImpl(json, detail::IncludesInternalIdYN::No);
	}

	std::shared_ptr<ComponentBase> Node::addComponentAtIndexFromJSON(const JSON& json, size_t index)
	{
		return addComponentAtIndexFromJSONImpl(json, index, detail::IncludesInternalIdYN::No);
	}

	std::shared_ptr<ComponentBase> Node::addComponentFromJSONImpl(const JSON& json, detail::IncludesInternalIdYN includesInternalId)
	{
		auto component = CreateComponentFromJSONImpl(json, includesInternalId);
		if (component)
		{
			addComponent(component);
		}
		return component;
	}

	std::shared_ptr<ComponentBase> Node::addComponentAtIndexFromJSONImpl(const JSON& json, size_t index, detail::IncludesInternalIdYN includesInternalId)
	{
		auto component = CreateComponentFromJSONImpl(json, includesInternalId);
		if (component)
		{
			addComponentAtIndex(component, index);
		}
		return component;
	}

	void Node::removeComponent(const std::shared_ptr<ComponentBase>& component)
	{
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
		if (!child->m_parent.expired())
		{
			throw Error{ U"addChild: Child node '{}' already has a parent"_fmt(child->m_name) };
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
		if (!child->m_parent.expired())
		{
			throw Error{ U"addChild: Child node '{}' already has a parent"_fmt(child->m_name) };
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

	const std::shared_ptr<Node>& Node::emplaceChild(StringView name, const ConstraintVariant& constraint, IsHitTargetYN isHitTarget, InheritChildrenStateFlags inheritChildrenStateFlags, RefreshesLayoutYN refreshesLayout)
	{
		auto child = Node::Create(name, constraint, isHitTarget, inheritChildrenStateFlags);
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
		if (!child->m_parent.expired())
		{
			throw Error{ U"addChildAtIndex: Child node '{}' already has a parent"_fmt(child->m_name) };
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

	void Node::refreshBoxChildrenLayout()
	{
		std::visit([this](const auto& layout)
			{
				layout.execute(m_layoutAppliedRect, m_children, [this](const std::shared_ptr<Node>& child, const RectF& rect)
					{
						child->m_layoutAppliedRect = rect;
						if (child->hasBoxConstraint())
						{
							child->m_layoutAppliedRect.moveBy(-m_scrollOffset);
						}
					});
			}, m_boxChildrenLayout);
		for (const auto& child : m_children)
		{
			child->refreshBoxChildrenLayout();
		}

		// レイアウト更新後の状態でスクロールオフセットを制限し、変化があれば反映
		// (子ノードの大きさに変更があった場合にもスクロールオフセットの制限を更新する必要があるため)
		const Vec2 prevScrollOffset = m_scrollOffset;
		clampScrollOffset();
		if (m_scrollOffset != prevScrollOffset)
		{
			std::visit([this](const auto& layout)
				{
					layout.execute(m_layoutAppliedRect, m_children, [this](const std::shared_ptr<Node>& child, const RectF& rect)
						{
							child->m_layoutAppliedRect = rect;
							if (child->hasBoxConstraint())
							{
								child->m_layoutAppliedRect.moveBy(-m_scrollOffset);
							}
						});
				}, m_boxChildrenLayout);
			for (const auto& child : m_children)
			{
				child->refreshBoxChildrenLayout();
			}
		}
	}

	Optional<RectF> Node::getBoxChildrenContentRect() const
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
			if (!child->hasBoxConstraint())
			{
				continue;
			}
			exists = true;

			const RectF& childRect = child->layoutAppliedRectWithMargin();
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

	Optional<RectF> Node::getBoxChildrenContentRectWithPadding() const
	{
		const auto contentRectOpt = getBoxChildrenContentRect();
		if (!contentRectOpt)
		{
			return none;
		}
		const RectF& contentRect = *contentRectOpt;
		const LRTB& padding = boxChildrenLayoutPadding();
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
		// hitTestPaddingを考慮した当たり判定領域を計算
		const bool hit = hitTestQuad(IncludingPaddingYN::Yes).contains(point);
		if (m_clippingEnabled && !m_posScaleAppliedRect.contains(point))
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
		// hitTestPaddingを考慮した当たり判定領域を計算
		const bool hit = hitTestQuad(IncludingPaddingYN::Yes).contains(point);
		if (m_clippingEnabled && !m_posScaleAppliedRect.contains(point))
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
			if (const Optional<RectF> contentRectOpt = getBoxChildrenContentRectWithPadding())
			{
				const RectF& contentRectLocal = *contentRectOpt;
				if ((horizontalScrollable() && contentRectLocal.w > m_layoutAppliedRect.w) ||
					(verticalScrollable() && contentRectLocal.h > m_layoutAppliedRect.h))
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

	void Node::updateInput()
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
			(*it)->updateInput();
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
			if (m_activeInHierarchy && (!detail::s_canvasUpdateContext.inputBlocked)) // updateInput内で更新される場合があるためループ内で分岐が必要
			{
				(*it)->updateInput(thisNode);
			}
			else
			{
				(*it)->updateInputInactive(thisNode);
			}
		}
		m_componentTempBuffer.clear();
	}

	void Node::update(const std::shared_ptr<Node>& scrollableHoveredNode, double deltaTime, const Mat3x2& parentPosScaleMat, const Vec2& parentEffectScale, const Mat3x2& parentHitTestMat, double parentRotation, double parentHitTestRotation, const Mat3x2& parentHitTestPosScaleMat, const Array<String>& parentActiveStyleStates)
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
			refreshPosScaleAppliedRect(RecursiveYN::No, parentPosScaleMat, parentEffectScale, parentRotation, parentHitTestMat, parentHitTestRotation, parentHitTestPosScaleMat);
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
			// 基本のposScaleMat（回転による位置変化なし）
			const Mat3x2 baseChildPosScaleMat = m_transformEffect.posScaleMat(parentPosScaleMat, m_layoutAppliedRect, parentRotation);
			const Vec2 effectScale = m_transformEffect.scale().value() * parentEffectScale;
			
			// HitTest用のMatrixを計算
			const Mat3x2 baseChildHitTestMat = m_transformEffect.appliesToHitTest().value() ? 
				m_transformEffect.posScaleMat(parentHitTestMat, m_layoutAppliedRect, parentHitTestRotation) : parentHitTestMat;
			
			// 自身の回転がある場合、子ノードの位置に対する影響を計算
			const double myRotation = m_transformEffect.rotation().value();
			if (myRotation != 0.0)
			{
				const Vec2 myPivotPos = effectPivotPosWithoutParentPosScale();
				
				// 各子ノードに対して位置補正を適用
				for (const auto& child : m_childrenTempBuffer)
				{
					// 描画用の位置補正（子のpivotを考慮）
					const Vec2 relativePos = child->effectPivotPosWithoutParentPosScale() - myPivotPos;
					// 親のスケールを適用
					const Vec2& parentScale = m_transformEffect.scale().value();
					const Vec2 scaledRelativePos = relativePos * parentScale;
					const double rad = Math::ToRadians(myRotation);
					const Vec2 rotatedScaledRelativePos{
						scaledRelativePos.x * std::cos(rad) - scaledRelativePos.y * std::sin(rad),
						scaledRelativePos.x * std::sin(rad) + scaledRelativePos.y * std::cos(rad)
					};
					// スケール適用後のオフセットを計算
					const Vec2 offsetInParentLocal = rotatedScaledRelativePos - scaledRelativePos;
					const Mat3x2 parentLinearMat{
						parentPosScaleMat._11, parentPosScaleMat._12,
						parentPosScaleMat._21, parentPosScaleMat._22,
						0.0, 0.0
					};
					const Vec2 finalOffset = parentLinearMat.transformPoint(offsetInParentLocal);
					const Mat3x2 childPosScaleMat = baseChildPosScaleMat.translated(finalOffset);
					
					// HitTest用の位置補正
					Mat3x2 childHitTestMatForChild;
					if (m_transformEffect.appliesToHitTest().value())
					{
						// 親のappliesToHitTest=trueの場合のみ位置補正を適用
						if (child->transformEffect().appliesToHitTest().value())
						{
							// 子のappliesToHitTest=trueなら描画と同じ補正を適用
							childHitTestMatForChild = baseChildHitTestMat.translated(finalOffset);
						}
						else
						{
							// 子のappliesToHitTest=falseならレイアウト矩形の左上を基準に補正
							const Vec2 childLayoutPos = child->layoutAppliedRect().pos;
							const Vec2 relativeLayoutPos = childLayoutPos - myPivotPos;
							// 親のスケールを適用
							const Vec2 scaledRelativeLayoutPos = relativeLayoutPos * parentScale;
							const Vec2 rotatedScaledRelativeLayoutPos{
								scaledRelativeLayoutPos.x * std::cos(rad) - scaledRelativeLayoutPos.y * std::sin(rad),
								scaledRelativeLayoutPos.x * std::sin(rad) + scaledRelativeLayoutPos.y * std::cos(rad)
							};
							// スケール適用後のオフセットを計算
							const Vec2 layoutOffsetInParentLocal = rotatedScaledRelativeLayoutPos - scaledRelativeLayoutPos;
							const Vec2 layoutFinalOffset = parentLinearMat.transformPoint(layoutOffsetInParentLocal);
							childHitTestMatForChild = baseChildHitTestMat.translated(layoutFinalOffset);
						}
					}
					else
					{
						// 親のappliesToHitTest=falseなら位置補正を適用しない
						childHitTestMatForChild = baseChildHitTestMat;
					}
					
					// 子に渡すHitTestPosScaleMat
					const Mat3x2 childHitTestPosScaleMat = m_transformEffect.appliesToHitTest().value() ? 
						m_transformEffect.posScaleMat(parentHitTestMat, m_layoutAppliedRect, parentHitTestRotation) : parentHitTestPosScaleMat;
					
					child->update(scrollableHoveredNode, deltaTime, childPosScaleMat, effectScale, childHitTestMatForChild, m_rotationInHierarchy, m_hitTestRotation, childHitTestPosScaleMat, m_activeStyleStates);
				}
			}
			else
			{
				// 回転がない場合は通常通り
				for (const auto& child : m_childrenTempBuffer)
				{
					// 子に渡すHitTestPosScaleMat
					const Mat3x2 childHitTestPosScaleMat = m_transformEffect.appliesToHitTest().value() ? 
						m_transformEffect.posScaleMat(parentHitTestMat, m_layoutAppliedRect, parentHitTestRotation) : parentHitTestPosScaleMat;
					
					child->update(scrollableHoveredNode, deltaTime, baseChildPosScaleMat, effectScale, baseChildHitTestMat, m_rotationInHierarchy, m_hitTestRotation, childHitTestPosScaleMat, m_activeStyleStates);
				}
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

	void Node::postLateUpdate(double deltaTime)
	{
		// postLateUpdateはユーザーコードを含まずaddChildやaddComponentによるイテレータ破壊は起きないため、一時バッファは使用不要

		// コンポーネントのプロパティ値更新
		for (const auto& component : m_components)
		{
			// m_activeStyleStatesはupdateで構築済み
			component->updateProperties(m_currentInteractionState, m_activeStyleStates, deltaTime);
		}

		// 子ノードのpostLateUpdate実行
		for (const auto& child : m_children)
		{
			child->postLateUpdate(deltaTime);
		}

		m_prevActiveInHierarchy = m_activeInHierarchy;
	}

	void Node::refreshPosScaleAppliedRect(RecursiveYN recursive, const Mat3x2& parentPosScaleMat, const Vec2& parentEffectScale, double parentRotation, const Mat3x2& parentHitTestMat, double parentHitTestRotation, const Mat3x2& parentHitTestPosScaleMat)
	{
		m_transformEffect.update(m_currentInteractionState, m_activeStyleStates, 0.0);

		const Mat3x2 posScaleMat = m_transformEffect.posScaleMat(parentPosScaleMat, m_layoutAppliedRect, parentRotation);
		const Vec2 posLeftTop = posScaleMat.transformPoint(m_layoutAppliedRect.pos);
		const Vec2 posRightBottom = posScaleMat.transformPoint(m_layoutAppliedRect.br());
		m_posScaleAppliedRect = RectF{ posLeftTop, posRightBottom - posLeftTop };
		m_effectScale = parentEffectScale * m_transformEffect.scale().value();
		
		// 回転適用後のQuadを計算
		m_rotationInHierarchy = m_transformEffect.rotationInHierarchy(parentRotation);
		if (m_rotationInHierarchy != 0.0)
		{
			m_rotatedQuad = m_posScaleAppliedRect.rotatedAt(effectPivotPos(), Math::ToRadians(m_rotationInHierarchy));
		}
		else
		{
			// 回転がない場合は矩形からQuadを作成
			m_rotatedQuad = Quad{ m_posScaleAppliedRect };
		}
		
		// HitTest用の矩形を計算
		// appliesToHitTestの値に応じてposScaleMatを構築
		if (m_transformEffect.appliesToHitTest().value())
		{
			// TransformEffectを適用
			const Mat3x2 hitTestPosScaleMat = m_transformEffect.posScaleMat(parentHitTestMat, m_layoutAppliedRect, parentHitTestRotation);
			const Vec2 hitTestPosLeftTop = hitTestPosScaleMat.transformPoint(m_layoutAppliedRect.pos);
			const Vec2 hitTestPosRightBottom = hitTestPosScaleMat.transformPoint(m_layoutAppliedRect.br());
			
			m_hitTestRect = CreateRectFromPoints(hitTestPosLeftTop, hitTestPosRightBottom);
		}
		else
		{
			// appliesToHitTestがfalseの場合、親から受け取ったHitTest行列をそのまま使用
			const Vec2 hitTestPosLeftTop = parentHitTestMat.transformPoint(m_layoutAppliedRect.pos);
			const Vec2 hitTestPosRightBottom = parentHitTestMat.transformPoint(m_layoutAppliedRect.br());
			
			m_hitTestRect = CreateRectFromPoints(hitTestPosLeftTop, hitTestPosRightBottom);
		}
		
		// ヒット判定用の回転角度を計算
		m_hitTestRotation = m_transformEffect.appliesToHitTest().value() ? 
			(parentHitTestRotation + m_transformEffect.rotation().value()) : 
			parentHitTestRotation;
		
		// ヒット判定用のQuadを計算
		if (m_hitTestRotation != 0.0)
		{
			// 回転の中心点を決定
			Vec2 rotationCenter;
			if (m_transformEffect.appliesToHitTest().value())
			{
				// 自身の変換が適用される場合は自身のpivot位置
				const Vec2& pivot = m_transformEffect.pivot().value();
				rotationCenter = m_hitTestRect.pos + m_hitTestRect.size * pivot;
			}
			else
			{
				// appliesToHitTest=falseの場合は、親がレイアウト矩形の左上を基準に計算しているため
				// 同じく矩形の左上を回転中心とする
				rotationCenter = m_hitTestRect.pos;
			}
			
			m_hitTestQuad = m_hitTestRect.rotatedAt(rotationCenter, Math::ToRadians(m_hitTestRotation));
			
			// パディング有りのQuadも計算
			const Vec2 effectScale = m_transformEffect.appliesToHitTest().value() ? m_effectScale : Vec2::One();
			const RectF paddedRect{
				m_hitTestRect.x - m_hitTestPadding.left * effectScale.x,
				m_hitTestRect.y - m_hitTestPadding.top * effectScale.y,
				m_hitTestRect.w + m_hitTestPadding.totalWidth() * effectScale.x,
				m_hitTestRect.h + m_hitTestPadding.totalHeight() * effectScale.y
			};
			m_hitTestQuadWithPadding = paddedRect.rotatedAt(rotationCenter, Math::ToRadians(m_hitTestRotation));
		}
		else
		{
			m_hitTestQuad = Quad{ m_hitTestRect };
			
			// パディング有りのQuadも計算
			const Vec2 effectScale = m_transformEffect.appliesToHitTest().value() ? m_effectScale : Vec2::One();
			const RectF paddedRect{
				m_hitTestRect.x - m_hitTestPadding.left * effectScale.x,
				m_hitTestRect.y - m_hitTestPadding.top * effectScale.y,
				m_hitTestRect.w + m_hitTestPadding.totalWidth() * effectScale.x,
				m_hitTestRect.h + m_hitTestPadding.totalHeight() * effectScale.y
			};
			m_hitTestQuadWithPadding = Quad{ paddedRect };
		}
		
		if (recursive)
		{
			// TODO: update側と同じ計算なのでまとめたい

			// 子ノードに渡すHitTest用のMatrix
			const Mat3x2 childHitTestMat = m_transformEffect.appliesToHitTest().value() ? 
				m_transformEffect.posScaleMat(parentHitTestMat, m_layoutAppliedRect, parentHitTestRotation) : parentHitTestMat;

			// 自身の回転がある場合、子ノードの位置に対する影響を計算
			const double myRotation = m_transformEffect.rotation().value();
			if (myRotation != 0.0)
			{
				const Vec2 myPivotPos = effectPivotPosWithoutParentPosScale();

				for (const auto& child : m_children)
				{
					// 描画用の位置補正（子のpivotを考慮）
					const Vec2 relativePos = child->effectPivotPosWithoutParentPosScale() - myPivotPos;
					// 親のスケールを適用
					const Vec2& parentScale = m_transformEffect.scale().value();
					const Vec2 scaledRelativePos = relativePos * parentScale;
					const double rad = Math::ToRadians(myRotation);
					const Vec2 rotatedScaledRelativePos{
						scaledRelativePos.x * std::cos(rad) - scaledRelativePos.y * std::sin(rad),
						scaledRelativePos.x * std::sin(rad) + scaledRelativePos.y * std::cos(rad)
					};
					// スケール適用後のオフセットを計算
					const Vec2 offsetInParentLocal = rotatedScaledRelativePos - scaledRelativePos;
					const Mat3x2 parentLinearMat{
						parentPosScaleMat._11, parentPosScaleMat._12,
						parentPosScaleMat._21, parentPosScaleMat._22,
						0.0, 0.0
					};
					const Vec2 finalOffset = parentLinearMat.transformPoint(offsetInParentLocal);
					const Mat3x2 childPosScaleMat = posScaleMat.translated(finalOffset);

					// HitTest用の位置補正
					Mat3x2 childHitTestMatForChild;
					if (m_transformEffect.appliesToHitTest().value())
					{
						// 親のappliesToHitTest=trueの場合のみ位置補正を適用
						if (child->transformEffect().appliesToHitTest().value())
						{
							// 子のappliesToHitTest=trueなら描画と同じ補正を適用
							childHitTestMatForChild = childHitTestMat.translated(finalOffset);
						}
						else
						{
							// 子のappliesToHitTest=falseならレイアウト矩形の左上を基準に補正
							const Vec2 childLayoutPos = child->layoutAppliedRect().pos;
							const Vec2 relativeLayoutPos = childLayoutPos - myPivotPos;
							// 親のスケールを適用
							const Vec2 scaledRelativeLayoutPos = relativeLayoutPos * parentScale;
							const Vec2 rotatedScaledRelativeLayoutPos{
								scaledRelativeLayoutPos.x * std::cos(rad) - scaledRelativeLayoutPos.y * std::sin(rad),
								scaledRelativeLayoutPos.x * std::sin(rad) + scaledRelativeLayoutPos.y * std::cos(rad)
							};
							// スケール適用後のオフセットを計算
							const Vec2 layoutOffsetInParentLocal = rotatedScaledRelativeLayoutPos - scaledRelativeLayoutPos;
							const Vec2 layoutFinalOffset = parentLinearMat.transformPoint(layoutOffsetInParentLocal);
							childHitTestMatForChild = childHitTestMat.translated(layoutFinalOffset);
						}
					}
					else
					{
						// 親のappliesToHitTest=falseなら位置補正を適用しない
						childHitTestMatForChild = childHitTestMat;
					}

					// 子に渡すHitTestPosScaleMat
					const Mat3x2 childHitTestPosScaleMat = m_transformEffect.appliesToHitTest().value() ? 
						m_transformEffect.posScaleMat(parentHitTestMat, m_layoutAppliedRect, parentHitTestRotation) : parentHitTestPosScaleMat;
					
					child->refreshPosScaleAppliedRect(RecursiveYN::Yes, childPosScaleMat, m_effectScale, m_rotationInHierarchy, childHitTestMatForChild, m_hitTestRotation, childHitTestPosScaleMat);
				}
			}
			else
			{
				// 回転がない場合
				for (const auto& child : m_children)
				{
					// 子に渡すHitTestPosScaleMat
					const Mat3x2 childHitTestPosScaleMat = m_transformEffect.appliesToHitTest().value() ? 
						m_transformEffect.posScaleMat(parentHitTestMat, m_layoutAppliedRect, parentHitTestRotation) : parentHitTestPosScaleMat;
					
					child->refreshPosScaleAppliedRect(RecursiveYN::Yes, posScaleMat, m_effectScale, m_rotationInHierarchy, childHitTestMat, m_hitTestRotation, childHitTestPosScaleMat);
				}
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
			scissorRect.emplace(m_posScaleAppliedRect.asRect());
		}

		// TransformEffectの乗算カラーを適用
		const ColorF effectColor = m_transformEffect.color().value();
		Optional<ScopedColorMul2D> colorMul;
		const ColorF currentColor = ColorF{ Graphics2D::GetColorMul() };
		const ColorF newColor = currentColor * effectColor;
		if (effectColor != ColorF{ 1.0 })
		{
			colorMul.emplace(newColor);
		}

		// 回転行列を計算（描画用）
		const Vec2 pivotPos = effectPivotPos();
		const Mat3x2 rotationMat = Mat3x2::Rotate(Math::ToRadians(m_rotationInHierarchy), pivotPos);

		// draw関数はconstのため、addComponentやaddChild等によるイテレータ破壊は考慮不要とする
		{
			// 回転適用
			Optional<Transformer2D> transformer;
			if (m_rotationInHierarchy != 0.0)
			{
				transformer.emplace(rotationMat);
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
			// スクロールバーは回転を適用（上で計算済みの行列を再利用）
			Optional<Transformer2D> transformer;
			if (m_rotationInHierarchy != 0.0)
			{
				transformer.emplace(rotationMat);
			}

			const bool needHorizontalScrollBar = horizontalScrollable();
			const bool needVerticalScrollBar = verticalScrollable();
			if (needHorizontalScrollBar || needVerticalScrollBar)
			{
				if (const Optional<RectF> contentRectOpt = getBoxChildrenContentRectWithPadding())
				{
					const RectF& contentRectLocal = *contentRectOpt;
					const Vec2 scale = m_effectScale;
					const Vec2 scrollOffsetAnchor = std::visit([](const auto& layout) { return layout.scrollOffsetAnchor(); }, m_boxChildrenLayout);
					const double roundRadius = 2.0 * (scale.x + scale.y) / 2;

					// 背景より手前にするためにハンドル部分は後で描画
					Optional<RectF> horizontalHandleRect = none;
					Optional<RectF> verticalHandleRect = none;

					// 横スクロールバー
					if (needHorizontalScrollBar)
					{
						const double viewWidth = m_layoutAppliedRect.w * scale.x;
						const double contentWidth = contentRectLocal.w * scale.x;
						const double maxScrollX = (contentWidth > viewWidth) ? (contentWidth - viewWidth) : 0.0;
						if (maxScrollX > 0.0)
						{
							const double w = (viewWidth * viewWidth) / contentWidth;
							const double scrolledRatio = (m_scrollOffset.x * scale.x + maxScrollX * scrollOffsetAnchor.x) / maxScrollX;
							const double x = scrolledRatio * (viewWidth - w);
							const double thickness = 4.0 * scale.y;

							const RectF backgroundRect
							{
								m_posScaleAppliedRect.x,
								m_posScaleAppliedRect.y + m_posScaleAppliedRect.h - thickness,
								m_posScaleAppliedRect.w,
								thickness
							};
							backgroundRect.rounded(roundRadius).draw(ColorF{ 0.0, m_scrollBarAlpha.currentValue() });

							horizontalHandleRect = RectF
							{
								m_posScaleAppliedRect.x + x,
								m_posScaleAppliedRect.y + m_posScaleAppliedRect.h - thickness,
								w,
								thickness
							};
						}
					}

					// 縦スクロールバー
					if (needVerticalScrollBar)
					{
						const double viewHeight = m_layoutAppliedRect.h * scale.y;
						const double contentHeight = contentRectLocal.h * scale.y;
						const double maxScrollY = (contentHeight > viewHeight) ? (contentHeight - viewHeight) : 0.0;
						if (maxScrollY > 0.0)
						{
							const double h = (viewHeight * viewHeight) / contentHeight;
							const double scrolledRatio = (m_scrollOffset.y * scale.y + maxScrollY * scrollOffsetAnchor.y) / maxScrollY;
							const double y = scrolledRatio * (viewHeight - h);
							const double thickness = 4.0 * scale.x;

							const RectF backgroundRect
							{
								m_posScaleAppliedRect.x + m_posScaleAppliedRect.w - thickness,
								m_posScaleAppliedRect.y,
								thickness,
								m_posScaleAppliedRect.h
							};
							backgroundRect.rounded(roundRadius).draw(ColorF{ 0.0, m_scrollBarAlpha.currentValue() });

							verticalHandleRect = RectF
							{
								m_posScaleAppliedRect.x + m_posScaleAppliedRect.w - thickness,
								m_posScaleAppliedRect.y + y,
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

	const RectF& Node::rect() const
	{
		return m_posScaleAppliedRect;
	}

	RectF Node::hitTestRect(IncludingPaddingYN includingPadding) const
	{
		if (includingPadding == IncludingPaddingYN::No)
		{
			return m_hitTestRect;
		}
		
		// パディングを含める場合
		const Vec2 effectScale = m_transformEffect.appliesToHitTest().value() ? m_effectScale : Vec2::One();
		return RectF{
			m_hitTestRect.x - m_hitTestPadding.left * effectScale.x,
			m_hitTestRect.y - m_hitTestPadding.top * effectScale.y,
			m_hitTestRect.w + m_hitTestPadding.totalWidth() * effectScale.x,
			m_hitTestRect.h + m_hitTestPadding.totalHeight() * effectScale.y
		};
	}

	const Vec2& Node::effectScale() const
	{
		return m_effectScale;
	}

	double Node::rotationInHierarchy() const
	{
		return m_rotationInHierarchy;
	}

	Vec2 Node::effectPivotPos() const
	{
		const Vec2& pivot = m_transformEffect.pivot().value();
		return m_posScaleAppliedRect.pos + m_posScaleAppliedRect.size * pivot;
	}

	Vec2 Node::effectPivotPosWithoutParentPosScale() const
	{
		const Vec2& pivot = m_transformEffect.pivot().value();
		return m_layoutAppliedRect.pos + m_layoutAppliedRect.size * pivot;
	}

	const Quad& Node::rotatedQuad() const
	{
		return m_rotatedQuad;
	}

	Quad Node::hitTestQuad(IncludingPaddingYN includingPadding) const
	{
		if (includingPadding == IncludingPaddingYN::Yes)
		{
			// パディング有りの場合は事前計算済みのQuadを返す
			return m_hitTestQuadWithPadding;
		}
		else
		{
			// パディングなしの場合は事前計算済みのQuadを返す
			return m_hitTestQuad;
		}
	}

	const RectF& Node::layoutAppliedRect() const
	{
		return m_layoutAppliedRect;
	}

	RectF Node::layoutAppliedRectWithMargin() const
	{
		if (const BoxConstraint* pBoxConstraint = boxConstraint())
		{
			const LRTB& margin = pBoxConstraint->margin;
			return RectF
			{
				m_layoutAppliedRect.x - margin.left,
				m_layoutAppliedRect.y - margin.top,
				m_layoutAppliedRect.w + margin.left + margin.right,
				m_layoutAppliedRect.h + margin.top + margin.bottom,
			};
		}
		else
		{
			return m_layoutAppliedRect;
		}
	}

	const Array<std::shared_ptr<Node>>& Node::children() const
	{
		return m_children;
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

	const LRTB& Node::hitTestPadding() const
	{
		return m_hitTestPadding;
	}

	std::shared_ptr<Node> Node::setHitTestPadding(const LRTB& padding)
	{
		m_hitTestPadding = padding;
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

	bool Node::inheritsChildrenHoveredState() const
	{
		return HasFlag(m_inheritChildrenStateFlags, InheritChildrenStateFlags::Hovered);
	}

	std::shared_ptr<Node> Node::setInheritsChildrenHoveredState(bool value)
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

	bool Node::inheritsChildrenPressedState() const
	{
		return HasFlag(m_inheritChildrenStateFlags, InheritChildrenStateFlags::Pressed);
	}

	std::shared_ptr<Node> Node::setInheritsChildrenPressedState(bool value)
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

	std::shared_ptr<Node> Node::addInputUpdater(std::function<void(const std::shared_ptr<Node>&)> inputUpdater)
	{
		emplaceComponent<InputUpdaterComponent>(std::move(inputUpdater));
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
