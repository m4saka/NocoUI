#include "NocoUI/Node.hpp"
#include "NocoUI/Canvas.hpp"
#include "NocoUI/Component/Component.hpp"
#include "NocoUI/detail/ScopedScissorRect.hpp"

namespace noco
{
	InteractState Node::updateForCurrentInteractState(const std::shared_ptr<Node>& hoveredNode, InteractableYN parentInteractable)
	{
		const InteractableYN interactable{ m_interactable && parentInteractable };
		InteractState inheritedInteractState = InteractState::Default;
		bool inheritedIsClicked = false;
		if (m_inheritChildrenStateFlags != InheritChildrenStateFlags::None)
		{
			for (const auto& child : m_children)
			{
				InteractState childInteractState = child->updateForCurrentInteractState(hoveredNode, interactable);
				if (interactable)
				{
					if (!inheritsChildrenPressedState() && childInteractState == InteractState::Pressed)
					{
						childInteractState = InteractState::Hovered;
					}
					if (!inheritsChildrenHoveredState() && childInteractState == InteractState::Hovered)
					{
						childInteractState = InteractState::Default;
					}
					if (inheritsChildrenPressedState() && child->isClicked())
					{
						inheritedIsClicked = true;
					}
					inheritedInteractState = ApplyOtherInteractState(inheritedInteractState, childInteractState, AppliesDisabledStateYN::No);
				}
			}
		}
		if (interactable)
		{
			const bool onClientRect = Cursor::OnClientRect();
			const bool mouseOverForHovered = onClientRect && m_activeInHierarchy && (hoveredNode.get() == this || (inheritsChildrenHoveredState() && (inheritedInteractState == InteractState::Hovered || inheritedInteractState == InteractState::Pressed)));
			const bool mouseOverForPressed = onClientRect && m_activeInHierarchy && (hoveredNode.get() == this || (inheritsChildrenPressedState() && (inheritedInteractState == InteractState::Pressed || inheritedIsClicked))); // クリック判定用に離した瞬間もホバー扱いにする必要があるため、子のisClickedも加味している
			m_mouseLTracker.update(mouseOverForHovered, mouseOverForPressed);
			return ApplyOtherInteractState(m_mouseLTracker.interactStateSelf(), inheritedInteractState);
		}
		else
		{
			m_mouseLTracker.update(false, false);
			return InteractState::Disabled;
		}
	}

	InteractState Node::updateForCurrentInteractStateRight(const std::shared_ptr<Node>& hoveredNode, InteractableYN parentInteractable)
	{
		const InteractableYN interactable{ m_interactable && parentInteractable };
		InteractState inheritedInteractState = InteractState::Default;
		bool inheritedIsRightClicked = false;
		if (m_inheritChildrenStateFlags != InheritChildrenStateFlags::None)
		{
			for (const auto& child : m_children)
			{
				InteractState childInteractState = child->updateForCurrentInteractStateRight(hoveredNode, interactable);
				if (interactable)
				{
					if (!inheritsChildrenPressedState() && childInteractState == InteractState::Pressed)
					{
						childInteractState = InteractState::Hovered;
					}
					if (!inheritsChildrenHoveredState() && childInteractState == InteractState::Hovered)
					{
						childInteractState = InteractState::Default;
					}
					if (inheritsChildrenPressedState() && child->isRightClicked())
					{
						inheritedIsRightClicked = true;
					}
					inheritedInteractState = ApplyOtherInteractState(inheritedInteractState, childInteractState, AppliesDisabledStateYN::No);
				}
			}
		}
		if (interactable)
		{
			const bool onClientRect = Cursor::OnClientRect();
			const bool mouseOverForHovered = onClientRect && m_activeInHierarchy && (hoveredNode.get() == this || (inheritsChildrenHoveredState() && (inheritedInteractState == InteractState::Hovered || inheritedInteractState == InteractState::Pressed)));
			const bool mouseOverForPressed = onClientRect && m_activeInHierarchy && (hoveredNode.get() == this || (inheritsChildrenPressedState() && (inheritedInteractState == InteractState::Pressed || inheritedIsRightClicked))); // クリック判定用に離した瞬間もホバー扱いにする必要があるため、子のisRightClickedも加味している
			m_mouseRTracker.update(mouseOverForHovered, mouseOverForPressed);
			return ApplyOtherInteractState(m_mouseRTracker.interactStateSelf(), inheritedInteractState);
		}
		else
		{
			m_mouseRTracker.update(false, false);
			return InteractState::Disabled;
		}
	}

	void Node::refreshActiveInHierarchy()
	{
		m_activeInHierarchy = ActiveYN{ m_activeSelf && (m_parent.expired() || m_parent.lock()->m_activeInHierarchy) };
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

		const double viewWidth = m_layoutAppliedRect.w;
		const double viewHeight = m_layoutAppliedRect.h;
		const double maxScrollX = Max(contentRect.w - viewWidth, 0.0);
		const double maxScrollY = Max(contentRect.h - viewHeight, 0.0);
		if (maxScrollX <= 0.0 && maxScrollY <= 0.0)
		{
			m_scrollOffset = Vec2::Zero();
			return;
		}

		const Vec2 scrollOffsetAnchor = std::visit([](const auto& layout) { return layout.scrollOffsetAnchor(); }, m_childrenLayout);
		m_scrollOffset.x = Clamp(m_scrollOffset.x, -maxScrollX * scrollOffsetAnchor.x, maxScrollX * (1.0 - scrollOffsetAnchor.x));
		m_scrollOffset.y = Clamp(m_scrollOffset.y, -maxScrollY * scrollOffsetAnchor.y, maxScrollY * (1.0 - scrollOffsetAnchor.y));
	}

	std::shared_ptr<Node> Node::Create(StringView name, const ConstraintVariant& constraint, IsHitTargetYN isHitTarget, InheritChildrenStateFlags inheritChildrenStateFlags)
	{
		return std::shared_ptr<Node>{ new Node{ name, constraint, isHitTarget, inheritChildrenStateFlags } };
	}

	const ConstraintVariant& Node::constraint() const
	{
		return m_constraint;
	}

	void Node::setConstraint(const ConstraintVariant& constraint, RefreshesLayoutYN refreshesLayout)
	{
		m_constraint = constraint;
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
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

	const LayoutVariant& Node::childrenLayout() const
	{
		return m_childrenLayout;
	}

	void Node::setChildrenLayout(const LayoutVariant& layout, RefreshesLayoutYN refreshesLayout)
	{
		m_childrenLayout = layout;
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
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
		return std::visit([this](const auto& layout) { return layout.getFittingSizeToChildren(m_layoutAppliedRect, m_children); }, m_childrenLayout);
	}

	void Node::setBoxConstraintToFitToChildren(FitTarget fitTarget, RefreshesLayoutYN refreshesLayout)
	{
		std::visit([this, fitTarget, refreshesLayout](auto& layout) { layout.setBoxConstraintToFitToChildren(m_layoutAppliedRect, m_children, *this, fitTarget, refreshesLayout); }, m_childrenLayout);
	}

	const LRTB& Node::layoutPadding() const
	{
		return std::visit([](const auto& layout) -> const LRTB& { return layout.padding; }, m_childrenLayout);
	}

	bool Node::isLayoutAffected() const
	{
		// AnchorConstraintの場合のみ親のレイアウトの影響を受けない
		return !std::holds_alternative<AnchorConstraint>(m_constraint);
	}

	JSON Node::toJSON() const
	{
		Array<JSON> childrenJSON;
		for (const auto& child : m_children)
		{
			childrenJSON.push_back(child->toJSON());
		}

		JSON result
		{
			{ U"name", m_name },
			{ U"constraint", std::visit([](const auto& constraint) { return constraint.toJSON(); }, m_constraint) },
			{ U"transformEffect", m_transformEffect.toJSON() },
			{ U"childrenLayout", std::visit([](const auto& childrenLayout) { return childrenLayout.toJSON(); }, m_childrenLayout) },
			{ U"components", Array<JSON>{} },
			{ U"children", childrenJSON },
			{ U"isHitTarget", m_isHitTarget.getBool() },
			{ U"inheritsChildrenHoveredState", inheritsChildrenHoveredState() },
			{ U"inheritsChildrenPressedState", inheritsChildrenPressedState() },
			{ U"interactable", m_interactable.getBool() },
			{ U"horizontalScrollable", horizontalScrollable() },
			{ U"verticalScrollable", verticalScrollable() },
			{ U"clippingEnabled", m_clippingEnabled.getBool() },
			{ U"activeSelf", m_activeSelf.getBool() },
		};

		for (const std::shared_ptr<ComponentBase>& component : m_components)
		{
			if (const auto serializableComponent = std::dynamic_pointer_cast<SerializableComponentBase>(component))
			{
				result[U"components"].push_back(serializableComponent->toJSON());
			}
		}

		return result;
	}

	std::shared_ptr<Node> Node::CreateFromJSON(const JSON& json)
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
				Logger << U"[NocoUI warning] Unknown children layout type: '{}'"_fmt(type);
				node->m_childrenLayout = FlowLayout{};
			}
		}
		if (json.contains(U"isHitTarget"))
		{
			node->m_isHitTarget = IsHitTargetYN{ json[U"isHitTarget"].getOr<bool>(true) };
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
		if (json.contains(U"clippingEnabled"))
		{
			node->setClippingEnabled(ClippingEnabledYN{ json[U"clippingEnabled"].getOr<bool>(false) });
		}
		if (json.contains(U"activeSelf"))
		{
			node->setActive(ActiveYN{ json[U"activeSelf"].getOr<bool>(true) }, RefreshesLayoutYN::No);
		}

		if (json.contains(U"components") && json[U"components"].isArray())
		{
			for (const auto& componentJSON : json[U"components"].arrayView())
			{
				const auto type = componentJSON[U"type"].getString();

				if (type == U"Label")
				{
					auto component = std::make_shared<Label>();
					if (!component->tryReadFromJSON(componentJSON))
					{
						throw Error{ U"Failed to read Label component from JSON" };
					}
					node->addComponent(std::move(component));
					continue;
				}

				if (type == U"Sprite")
				{
					auto component = std::make_shared<Sprite>();
					if (!component->tryReadFromJSON(componentJSON))
					{
						throw Error{ U"Failed to read Sprite component from JSON" };
					}
					node->addComponent(std::move(component));
					continue;
				}

				if (type == U"RectRenderer")
				{
					auto component = std::make_shared<RectRenderer>();
					if (!component->tryReadFromJSON(componentJSON))
					{
						throw Error{ U"Failed to read RectRenderer component from JSON" };
					}
					node->addComponent(std::move(component));
					continue;
				}

				if (type == U"TextBox")
				{
					auto component = std::make_shared<TextBox>();
					if (!component->tryReadFromJSON(componentJSON))
					{
						throw Error{ U"Failed to read TextBox component from JSON" };
					}
					node->addComponent(std::move(component));
					continue;
				}

				if (type == U"InputBlocker")
				{
					auto component = std::make_shared<InputBlocker>();
					if (!component->tryReadFromJSON(componentJSON))
					{
						throw Error{ U"Failed to read InputBlocker component from JSON" };
					}
					node->addComponent(std::move(component));
					continue;
				}

				if (type == U"EventTrigger")
				{
					auto component = std::make_shared<EventTrigger>();
					if (!component->tryReadFromJSON(componentJSON))
					{
						throw Error{ U"Failed to read EventTrigger component from JSON" };
					}
					node->addComponent(std::move(component));
					continue;
				}

				if (type == U"Placeholder")
				{
					auto placeholder = std::make_shared<Placeholder>();
					if (!placeholder->tryReadFromJSON(componentJSON))
					{
						throw Error{ U"Failed to read Placeholder component from JSON" };
					}
					node->addComponent(std::move(placeholder));
					continue;
				}

				Logger << U"[NocoUI warning] Unknown component type: '{}'"_fmt(type);
			}
		}

		if (json.contains(U"children") && json[U"children"].isArray())
		{
			for (const auto& childJSON : json[U"children"].arrayView())
			{
				node->addChildFromJSON(childJSON, RefreshesLayoutYN::No);
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

	void Node::setParent(const std::shared_ptr<Node>& parent, RefreshesLayoutYN refreshesLayout)
	{
		if (!m_parent.expired())
		{
			removeFromParent(RefreshesLayoutYN{ refreshesLayout && parent->m_canvas.lock() != m_canvas.lock() });
		}
		parent->addChild(shared_from_this(), refreshesLayout);
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
			const auto it = std::find(parent->m_children.begin(), parent->m_children.end(), shared_from_this());
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

	void Node::addComponent(std::shared_ptr<ComponentBase>&& component)
	{
		m_components.push_back(std::move(component));
	}

	void Node::addComponent(const std::shared_ptr<ComponentBase>& component)
	{
		m_components.push_back(component);
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

	void Node::refreshChildrenLayout()
	{
		std::visit([this](const auto& layout)
			{
				layout.execute(m_layoutAppliedRect, m_children, [this](const std::shared_ptr<Node>& child, const RectF& rect)
					{
						child->m_layoutAppliedRect = rect;
						if (child->isLayoutAffected())
						{
							child->m_layoutAppliedRect.moveBy(-m_scrollOffset);
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
					layout.execute(m_layoutAppliedRect, m_children, [this](const std::shared_ptr<Node>& child, const RectF& rect)
						{
							child->m_layoutAppliedRect = rect;
							if (child->isLayoutAffected())
							{
								child->m_layoutAppliedRect.moveBy(-m_scrollOffset);
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
			if (!child->isLayoutAffected())
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

		return RectF{ left, top, right - left, bottom - top };
	}

	Optional<RectF> Node::getChildrenContentRectWithPadding() const
	{
		const auto contentRectOpt = getChildrenContentRect();
		if (!contentRectOpt)
		{
			return none;
		}
		const RectF& contentRect = *contentRectOpt;
		const LRTB& padding = layoutPadding();
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
		const bool hit = m_effectedRect.contains(point);
		if (m_clippingEnabled && !hit)
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
		const bool hit = m_effectedRect.contains(point);
		if (m_clippingEnabled && !hit)
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

	void Node::updateInteractState(const std::shared_ptr<Node>& hoveredNode, double deltaTime, InteractableYN parentInteractable, InteractState parentInteractState, InteractState parentInteractStateRight)
	{
		// updateInteractStateはユーザーコードを含まずaddChildやaddComponentによるイテレータ破壊が起きないため、一時バッファは使用不要

		const auto thisNode = shared_from_this();

		m_clickRequested = false;
		m_rightClickRequested = false;

		m_currentInteractState = updateForCurrentInteractState(hoveredNode, parentInteractable);
		m_currentInteractStateRight = updateForCurrentInteractStateRight(hoveredNode, parentInteractable);
		if (!m_isHitTarget)
		{
			// HitTargetでない場合は親のinteractStateを引き継ぐ
			m_currentInteractState = ApplyOtherInteractState(m_currentInteractState, parentInteractState);
			m_currentInteractStateRight = ApplyOtherInteractState(m_currentInteractStateRight, parentInteractStateRight);
		}

		if (!m_children.empty())
		{
			// 子ノードのupdateInteractState実行
			const InteractableYN interactable{ m_interactable && parentInteractable };
			for (const auto& child : m_children)
			{
				child->updateInteractState(hoveredNode, deltaTime, interactable, m_currentInteractState, m_currentInteractStateRight);
			}
		}
	}

	void Node::updateInput()
	{
		noco::detail::CopyCanvasUpdateContextToPrevIfNeeded();
		noco::detail::ClearCanvasUpdateContextBeforeUpdateInputIfNeeded();

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

	void Node::update(const std::shared_ptr<Node>& scrollableHoveredNode, double deltaTime, const Mat3x2& parentEffectMat, const Vec2& parentEffectScale)
	{
		noco::detail::CopyCanvasUpdateContextToPrevIfNeeded();
		noco::detail::ClearCanvasUpdateContextBeforeUpdateIfNeeded();
		
		const auto thisNode = shared_from_this();
		
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

		if (m_activeInHierarchy)
		{
			m_transformEffect.update(m_currentInteractState, m_selected, deltaTime);
			refreshEffectedRect(parentEffectMat, parentEffectScale);
		}

		// ホバー中はスクロールバーを表示
		if (thisNode == scrollableHoveredNode)
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
			const Mat3x2 effectMat = m_transformEffect.effectMat(parentEffectMat, m_layoutAppliedRect);
			const Vec2 effectScale = m_transformEffect.scale().value() * parentEffectScale;
			for (const auto& child : m_childrenTempBuffer)
			{
				child->update(scrollableHoveredNode, deltaTime, effectMat, effectScale);
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
			component->updateProperties(m_currentInteractState, m_selected, deltaTime);
		}

		// 子ノードのpostLateUpdate実行
		for (const auto& child : m_children)
		{
			child->postLateUpdate(deltaTime);
		}
	}

	void Node::refreshEffectedRect(const Mat3x2& parentEffectMat, const Vec2& parentEffectScale)
	{
		const Mat3x2 effectMat = m_transformEffect.effectMat(parentEffectMat, m_layoutAppliedRect);
		const Vec2 posLeftTop = effectMat.transformPoint(m_layoutAppliedRect.pos);
		const Vec2 posRightBottom = effectMat.transformPoint(m_layoutAppliedRect.br());
		m_effectedRect = RectF{ posLeftTop, posRightBottom - posLeftTop };
		m_effectScale = parentEffectScale * m_transformEffect.scale().value();
		for (const auto& child : m_children)
		{
			child->refreshEffectedRect(effectMat, m_effectScale);
		}
	}

	void Node::scroll(const Vec2& offsetDelta, RefreshesLayoutYN refreshesLayout)
	{
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

	void Node::resetScrollOffset(RefreshesLayoutYN refreshesLayoutPre, RefreshesLayoutYN refreshesLayoutPost)
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

		if (refreshesLayoutPost)
		{
			refreshContainedCanvasLayout();
		}
	}

	void Node::resetScrollOffsetRecursive(RefreshesLayoutYN refreshesLayoutPre, RefreshesLayoutYN refreshesLayoutPost)
	{
		if (refreshesLayoutPre)
		{
			refreshContainedCanvasLayout();
		}

		resetScrollOffset(RefreshesLayoutYN::No, RefreshesLayoutYN::No);
		for (const auto& child : m_children)
		{
			child->resetScrollOffsetRecursive(RefreshesLayoutYN::No, RefreshesLayoutYN::No);
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
			scissorRect.emplace(m_effectedRect.asRect());
		}

		// draw関数はconstのため、addComponentやaddChild等によるイテレータ破壊は考慮不要とする
		{
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
			const bool needHorizontalScrollBar = horizontalScrollable();
			const bool needVerticalScrollBar = verticalScrollable();
			if (needHorizontalScrollBar || needVerticalScrollBar)
			{
				if (const Optional<RectF> contentRectOpt = getChildrenContentRectWithPadding())
				{
					const RectF& contentRectLocal = *contentRectOpt;
					const Vec2 scale = m_effectScale;
					const Vec2 scrollOffsetAnchor = std::visit([](const auto& layout) { return layout.scrollOffsetAnchor(); }, m_childrenLayout);
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
								m_effectedRect.x,
								m_effectedRect.y + m_effectedRect.h - thickness,
								m_effectedRect.w,
								thickness
							};
							backgroundRect.rounded(roundRadius).draw(ColorF{ 0.0, m_scrollBarAlpha.currentValue() });

							horizontalHandleRect = RectF
							{
								m_effectedRect.x + x,
								m_effectedRect.y + m_effectedRect.h - thickness,
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
								m_effectedRect.x + m_effectedRect.w - thickness,
								m_effectedRect.y,
								thickness,
								m_effectedRect.h
							};
							backgroundRect.rounded(roundRadius).draw(ColorF{ 0.0, m_scrollBarAlpha.currentValue() });

							verticalHandleRect = RectF
							{
								m_effectedRect.x + m_effectedRect.w - thickness,
								m_effectedRect.y + y,
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
		if (!m_activeInHierarchy || !m_interactable)
		{
			return;
		}
		m_clickRequested = true;
	}

	void Node::requestRightClick()
	{
		// m_isHitTargetがfalseでも効く仕様とする
		if (!m_activeInHierarchy || !m_interactable)
		{
			return;
		}
		m_rightClickRequested = true;
	}

	const String& Node::name() const
	{
		return m_name;
	}

	void Node::setName(StringView name)
	{
		m_name = name;
	}

	const RectF& Node::rect() const
	{
		return m_effectedRect;
	}

	const Vec2& Node::effectScale() const
	{
		return m_effectScale;
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

	void Node::setInteractable(InteractableYN interactable)
	{
		m_interactable = interactable;
		m_mouseLTracker.setInteractable(interactable);
		m_mouseRTracker.setInteractable(interactable);
	}

	void Node::setInteractable(bool interactable)
	{
		setInteractable(InteractableYN{ interactable });
	}

	ActiveYN Node::activeSelf() const
	{
		return m_activeSelf;
	}

	void Node::setActive(ActiveYN activeSelf, RefreshesLayoutYN refreshesLayout)
	{
		m_activeSelf = activeSelf;
		refreshActiveInHierarchy();
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
	}

	void Node::setActive(bool activeSelf, RefreshesLayoutYN refreshesLayout)
	{
		setActive(ActiveYN{ activeSelf }, refreshesLayout);
	}

	ActiveYN Node::activeInHierarchy() const
	{
		return m_activeInHierarchy;
	}

	IsHitTargetYN Node::isHitTarget() const
	{
		return m_isHitTarget;
	}

	void Node::setIsHitTarget(IsHitTargetYN isHitTarget)
	{
		m_isHitTarget = isHitTarget;
	}

	void Node::setIsHitTarget(bool isHitTarget)
	{
		setIsHitTarget(IsHitTargetYN{ isHitTarget });
	}

	InheritChildrenStateFlags Node::inheritChildrenStateFlags() const
	{
		return m_inheritChildrenStateFlags;
	}

	void Node::setInheritChildrenStateFlags(InheritChildrenStateFlags flags)
	{
		m_inheritChildrenStateFlags = flags;
	}

	bool Node::inheritsChildrenHoveredState() const
	{
		return HasFlag(m_inheritChildrenStateFlags, InheritChildrenStateFlags::Hovered);
	}

	void Node::setInheritsChildrenHoveredState(bool value)
	{
		if (value)
		{
			m_inheritChildrenStateFlags |= InheritChildrenStateFlags::Hovered;
		}
		else
		{
			m_inheritChildrenStateFlags &= ~InheritChildrenStateFlags::Hovered;
		}
	}

	bool Node::inheritsChildrenPressedState() const
	{
		return HasFlag(m_inheritChildrenStateFlags, InheritChildrenStateFlags::Pressed);
	}

	void Node::setInheritsChildrenPressedState(bool value)
	{
		if (value)
		{
			m_inheritChildrenStateFlags |= InheritChildrenStateFlags::Pressed;
		}
		else
		{
			m_inheritChildrenStateFlags &= ~InheritChildrenStateFlags::Pressed;
		}
	}

	ScrollableAxisFlags Node::scrollableAxisFlags() const
	{
		return m_scrollableAxisFlags;
	}

	void Node::setScrollableAxisFlags(ScrollableAxisFlags flags, RefreshesLayoutYN refreshesLayout)
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
	}

	bool Node::horizontalScrollable() const
	{
		return HasFlag(m_scrollableAxisFlags, ScrollableAxisFlags::Horizontal);
	}

	void Node::setHorizontalScrollable(bool scrollable, RefreshesLayoutYN refreshesLayout)
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
	}

	bool Node::verticalScrollable() const
	{
		return HasFlag(m_scrollableAxisFlags, ScrollableAxisFlags::Vertical);
	}

	void Node::setVerticalScrollable(bool scrollable, RefreshesLayoutYN refreshesLayout)
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
	}

	ClippingEnabledYN Node::clippingEnabled() const
	{
		return m_clippingEnabled;
	}

	void Node::setClippingEnabled(ClippingEnabledYN clippingEnabled)
	{
		m_clippingEnabled = clippingEnabled;
	}

	void Node::setClippingEnabled(bool clippingEnabled)
	{
		setClippingEnabled(ClippingEnabledYN{ clippingEnabled });
	}

	InteractState Node::interactStateSelf() const
	{
		return m_mouseLTracker.interactStateSelf();
	}

	InteractState Node::currentInteractState() const
	{
		return m_currentInteractState;
	}

	SelectedYN Node::selected() const
	{
		return m_selected;
	}

	void Node::setSelected(SelectedYN selected)
	{
		m_selected = selected;
	}

	void Node::setSelected(bool selected)
	{
		setSelected(SelectedYN{ selected });
	}

	bool Node::isHovered() const
	{
		return m_mouseLTracker.isHovered();
	}

	bool Node::isHoveredRecursive() const
	{
		return isHovered() || m_children.any([](const auto& child) { return child->isHoveredRecursive(); });
	}

	bool Node::isPressed() const
	{
		return m_mouseLTracker.isPressed();
	}

	bool Node::isPressedRecursive() const
	{
		return isPressed() || m_children.any([](const auto& child) { return child->isPressedRecursive(); });
	}

	bool Node::isPressedHover() const
	{
		return m_mouseLTracker.isPressedHover();
	}

	bool Node::isPressedHoverRecursive() const
	{
		return isPressedHover() || m_children.any([](const auto& child) { return child->isPressedHoverRecursive(); });
	}

	bool Node::isMouseDown() const
	{
		return m_mouseLTracker.isHovered() && MouseL.down();
	}

	bool Node::isMouseDownRecursive() const
	{
		return isMouseDown() || m_children.any([](const auto& child) { return child->isMouseDownRecursive(); });
	}

	bool Node::isClicked() const
	{
		return m_clickRequested || m_mouseLTracker.isClicked();
	}

	bool Node::isClickedRecursive() const
	{
		return isClicked() || m_children.any([](const auto& child) { return child->isClickedRecursive(); });
	}

	bool Node::isClickRequested() const
	{
		return m_clickRequested;
	}

	bool Node::isClickRequestedRecursive() const
	{
		return m_clickRequested || m_children.any([](const auto& child) { return child->isClickRequestedRecursive(); });
	}

	bool Node::isRightPressed() const
	{
		return m_mouseRTracker.isPressed();
	}

	bool Node::isRightPressedRecursive() const
	{
		return isRightPressed() || m_children.any([](const auto& child) { return child->isRightPressedRecursive(); });
	}

	bool Node::isRightPressedHover() const
	{
		return m_mouseRTracker.isPressedHover();
	}

	bool Node::isRightPressedHoverRecursive() const
	{
		return isRightPressedHover() || m_children.any([](const auto& child) { return child->isRightPressedHoverRecursive(); });
	}

	bool Node::isRightMouseDown() const
	{
		return m_mouseRTracker.isHovered() && MouseR.down();
	}

	bool Node::isRightMouseDownRecursive() const
	{
		return isRightMouseDown() || m_children.any([](const auto& child) { return child->isRightMouseDownRecursive(); });
	}

	bool Node::isRightClicked() const
	{
		return m_rightClickRequested || m_mouseRTracker.isClicked();
	}

	bool Node::isRightClickedRecursive() const
	{
		return isRightClicked() || m_children.any([](const auto& child) { return child->isRightClickedRecursive(); });
	}

	bool Node::isRightClickRequested() const
	{
		return m_rightClickRequested;
	}

	bool Node::isRightClickRequestedRecursive() const
	{
		return m_rightClickRequested || m_children.any([](const auto& child) { return child->isRightClickRequestedRecursive(); });
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

	void Node::addInputUpdater(std::function<void(const std::shared_ptr<Node>&)> inputUpdater)
	{
		emplaceComponent<InputUpdaterComponent>(std::move(inputUpdater));
	}

	void Node::addUpdater(std::function<void(const std::shared_ptr<Node>&)> updater)
	{
		emplaceComponent<UpdaterComponent>(std::move(updater));
	}

	void Node::addDrawer(std::function<void(const Node&)> drawer)
	{
		emplaceComponent<DrawerComponent>(std::move(drawer));
	}

	void Node::addOnClick(std::function<void(const std::shared_ptr<Node>&)> onClick)
	{
		emplaceComponent<UpdaterComponent>([onClick = std::move(onClick)](const std::shared_ptr<Node>& node)
			{
				if (node->isClicked())
				{
					onClick(node);
				}
			});
	}

	void Node::addOnRightClick(std::function<void(const std::shared_ptr<Node>&)> onRightClick)
	{
		emplaceComponent<UpdaterComponent>([onRightClick = std::move(onRightClick)](const std::shared_ptr<Node>& node)
			{
				if (node->isRightClicked())
				{
					onRightClick(node);
				}
			});
	}

	void Node::addClickHotKey(const Input& input, EnabledWhileTextEditingYN enabledWhileTextEditing, ClearsInputYN clearsInput)
	{
		addClickHotKey(input, CtrlYN::No, AltYN::No, ShiftYN::No, enabledWhileTextEditing, clearsInput);
	}

	void Node::addClickHotKey(const Input& input, CtrlYN ctrl, AltYN alt, ShiftYN shift, EnabledWhileTextEditingYN enabledWhileTextEditing, ClearsInputYN clearsInput)
	{
		emplaceComponent<HotKeyInputHandler>(input, ctrl, alt, shift, HotKeyTarget::Click, enabledWhileTextEditing, clearsInput);
	}

	void Node::addRightClickHotKey(const Input& input, EnabledWhileTextEditingYN enabledWhileTextEditing, ClearsInputYN clearsInput)
	{
		addRightClickHotKey(input, CtrlYN::No, AltYN::No, ShiftYN::No, enabledWhileTextEditing, clearsInput);
	}

	void Node::addRightClickHotKey(const Input& input, CtrlYN ctrl, AltYN alt, ShiftYN shift, EnabledWhileTextEditingYN enabledWhileTextEditing, ClearsInputYN clearsInput)
	{
		emplaceComponent<HotKeyInputHandler>(input, ctrl, alt, shift, HotKeyTarget::RightClick, enabledWhileTextEditing, clearsInput);
	}

	void Node::refreshContainedCanvasLayout()
	{
		if (const auto canvas = m_canvas.lock())
		{
			canvas->refreshLayout();
		}
	}
}
