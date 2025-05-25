#pragma once
#include <Siv3D.hpp>
#include "YN.hpp"
#include "PropertyValue.hpp"
#include "Property.hpp"
#include "InheritChildrenStateFlags.hpp"
#include "ScrollableAxisFlags.hpp"
#include "InteractState.hpp"
#include "MouseTracker.hpp"
#include "TransformEffect.hpp"
#include "Constraint/Constraint.hpp"
#include "Layout/Layout.hpp"
#include "Component/ComponentBase.hpp"
#include "Component/Placeholder.hpp"
#include "Component/DataStore.hpp"
#include "Enums.hpp"

namespace noco
{
	class Canvas;

	struct CanvasUpdateContext;

	class Node : public std::enable_shared_from_this<Node>
	{
		friend class Canvas;

	private:
		String m_name;
		ConstraintVariant m_constraint;
		TransformEffect m_transformEffect;
		LayoutVariant m_childrenLayout;
		Array<std::shared_ptr<Node>> m_children;
		Array<std::shared_ptr<ComponentBase>> m_components;
		IsHitTargetYN m_isHitTarget;
		InheritChildrenStateFlags m_inheritChildrenStateFlags = InheritChildrenStateFlags::None;
		InteractableYN m_interactable = InteractableYN::Yes;
		ScrollableAxisFlags m_scrollableAxisFlags = ScrollableAxisFlags::None;
		ClippingEnabledYN m_clippingEnabled = ClippingEnabledYN::No;
		ActiveYN m_activeSelf = ActiveYN::Yes;

		/* NonSerialized */ std::weak_ptr<Canvas> m_canvas;
		/* NonSerialized */ std::weak_ptr<Node> m_parent;
		/* NonSerialized */ RectF m_layoutAppliedRect{ 0.0, 0.0, 0.0, 0.0 };
		/* NonSerialized */ RectF m_effectedRect{ 0.0, 0.0, 0.0, 0.0 };
		/* NonSerialized */ Vec2 m_effectScale{ 1.0, 1.0 };
		/* NonSerialized */ Vec2 m_scrollOffset{ 0.0, 0.0 };
		/* NonSerialized */ Smoothing<double> m_scrollBarAlpha{ 0.0 };
		/* NonSerialized */ MouseTracker m_mouseLTracker;
		/* NonSerialized */ MouseTracker m_mouseRTracker;
		/* NonSerialized */ ActiveYN m_activeInHierarchy = ActiveYN::Yes;
		/* NonSerialized */ Optional<ActiveYN> m_prevActiveInHierarchy = none;
		/* NonSerialized */ SelectedYN m_selected = SelectedYN::No;
		/* NonSerialized */ InteractState m_currentInteractState = InteractState::Default;
		/* NonSerialized */ InteractState m_currentInteractStateRight = InteractState::Default;
		/* NonSerialized */ Array<std::shared_ptr<ComponentBase>> m_componentTempBuffer; // 一時バッファ
		/* NonSerialized */ Array<std::shared_ptr<Node>> m_childrenTempBuffer; // 一時バッファ

		[[nodiscard]]
		explicit Node(StringView name = U"Node", const ConstraintVariant& constraint = BoxConstraint{}, IsHitTargetYN isHitTarget = IsHitTargetYN::Yes, InheritChildrenStateFlags inheritChildrenStateFlags = InheritChildrenStateFlags::None)
			: m_name{ name }
			, m_constraint{ constraint }
			, m_isHitTarget{ isHitTarget }
			, m_inheritChildrenStateFlags{ inheritChildrenStateFlags }
			, m_mouseLTracker{ MouseL, m_interactable }
			, m_mouseRTracker{ MouseR, m_interactable }
		{
		}

		[[nodiscard]]
		InteractState updateForCurrentInteractState(const std::shared_ptr<Node>& hoveredNode, InteractableYN parentInteractable);

		[[nodiscard]]
		InteractState updateForCurrentInteractStateRight(const std::shared_ptr<Node>& hoveredNode, InteractableYN parentInteractable);

		void refreshActiveInHierarchy();

		void setCanvasRecursive(const std::weak_ptr<Canvas>& canvas);

		void clampScrollOffset();

	public:
		static std::shared_ptr<Node> Create(StringView name = U"Node", const ConstraintVariant& constraint = BoxConstraint{}, IsHitTargetYN isHitTarget = IsHitTargetYN::Yes, InheritChildrenStateFlags inheritChildrenStateFlags = InheritChildrenStateFlags::None);

		[[nodiscard]]
		const ConstraintVariant& constraint() const;

		void setConstraint(const ConstraintVariant& constraint, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		[[nodiscard]]
		const BoxConstraint* boxConstraint() const;

		[[nodiscard]]
		const AnchorConstraint* anchorConstraint() const;

		[[nodiscard]]
		TransformEffect& transformEffect();

		[[nodiscard]]
		const TransformEffect& transformEffect() const;

		[[nodiscard]]
		const LayoutVariant& childrenLayout() const;

		void setChildrenLayout(const LayoutVariant& layout, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		[[nodiscard]]
		const FlowLayout* childrenFlowLayout() const;

		[[nodiscard]]
		const HorizontalLayout* childrenHorizontalLayout() const;

		[[nodiscard]]
		const VerticalLayout* childrenVerticalLayout() const;

		[[nodiscard]]
		SizeF getFittingSizeToChildren() const;

		void setBoxConstraintToFitToChildren(FitTarget fitTarget = FitTarget::Both, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		[[nodiscard]]
		const LRTB& layoutPadding() const;

		[[nodiscard]]
		bool isLayoutAffected() const;

		[[nodiscard]]
		JSON toJSON() const;

		[[nodiscard]]
		static std::shared_ptr<Node> CreateFromJSON(const JSON& json);

		[[nodiscard]]
		std::shared_ptr<Node> parent() const;

		[[nodiscard]]
		std::shared_ptr<const Node> findHoverTargetParent() const;

		[[nodiscard]]
		std::shared_ptr<Node> findHoverTargetParent();

		[[nodiscard]]
		bool isAncestorOf(const std::shared_ptr<Node>& node) const;

		void setParent(const std::shared_ptr<Node>& parent, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		bool removeFromParent(RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		[[nodiscard]]
		size_t siblingIndex() const;

		[[nodiscard]]
		Optional<size_t> siblingIndexOpt() const;

		[[nodiscard]]
		std::shared_ptr<Canvas> containedCanvas() const;

		void addComponent(std::shared_ptr<ComponentBase>&& component);

		void addComponent(const std::shared_ptr<ComponentBase>& component);

		void removeComponent(const std::shared_ptr<ComponentBase>& component);

		template <class TComponent, class... Args>
		std::shared_ptr<TComponent> emplaceComponent(Args&&... args)
			requires std::derived_from<TComponent, ComponentBase>&& std::is_constructible_v<TComponent, Args...>;

		bool moveComponentUp(const std::shared_ptr<ComponentBase>& component);

		bool moveComponentDown(const std::shared_ptr<ComponentBase>& component);

		const std::shared_ptr<Node>& addChild(std::shared_ptr<Node>&& child, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		const std::shared_ptr<Node>& addChild(const std::shared_ptr<Node>& child, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		const std::shared_ptr<Node>& emplaceChild(StringView name = U"Rect", const ConstraintVariant& constraint = BoxConstraint{}, IsHitTargetYN isHitTarget = IsHitTargetYN::Yes, InheritChildrenStateFlags inheritChildrenStateFlags = InheritChildrenStateFlags::None, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		template <class... Args>
		const std::shared_ptr<Node>& addChildFromJSON(const JSON& json, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		const std::shared_ptr<Node>& addChildAtIndex(const std::shared_ptr<Node>& child, size_t index, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		void removeChild(const std::shared_ptr<Node>& child, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		[[nodiscard]]
		bool containsChild(const std::shared_ptr<Node>& child, RecursiveYN recursive = RecursiveYN::No) const;

		[[nodiscard]]
		bool containsChildByName(StringView name, RecursiveYN recursive = RecursiveYN::No) const;

		template <class Fty>
		Array<std::weak_ptr<Node>> findAll(Fty&& predicate);

		template <class TComponent>
		[[nodiscard]]
		std::shared_ptr<TComponent> getComponent() const
			requires std::derived_from<TComponent, ComponentBase>;

		template <class TComponent>
		[[nodiscard]]
		std::shared_ptr<TComponent> getComponentOrNull() const
			requires std::derived_from<TComponent, ComponentBase>;

		template <class TComponent>
		[[nodiscard]]
		std::shared_ptr<TComponent> getComponentRecursive() const
			requires std::derived_from<TComponent, ComponentBase>;

		template <class TComponent>
		[[nodiscard]]
		std::shared_ptr<TComponent> getComponentRecursiveOrNull() const
			requires std::derived_from<TComponent, ComponentBase>;

		[[nodiscard]]
		std::shared_ptr<Node> getChildByName(StringView name, RecursiveYN recursive = RecursiveYN::No);

		[[nodiscard]]
		std::shared_ptr<Node> getChildByNameOrNull(StringView name, RecursiveYN recursive = RecursiveYN::No);

		void refreshChildrenLayout();

		[[nodiscard]]
		Optional<RectF> getChildrenContentRect() const;

		Optional<RectF> getChildrenContentRectWithPadding() const;

		[[nodiscard]]
		std::shared_ptr<Node> hoveredNodeInChildren();

		[[nodiscard]]
		std::shared_ptr<Node> findContainedScrollableNode();

		void update(CanvasUpdateContext* pContext, const std::shared_ptr<Node>& hoveredNode, const std::shared_ptr<Node>& scrollableHoveredNode, double deltaTime, const Mat3x2& parentEffectMat, const Vec2& parentEffectScale, InteractableYN parentInteractable, InteractState parentInteractState, InteractState parentInteractStateRight);

		void lateUpdate(CanvasUpdateContext* pContext);

		void refreshEffectedRect(const Mat3x2& parentEffectMat, const Vec2& parentEffectScale);

		void scroll(const Vec2& offsetDelta, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		Vec2 scrollOffset() const;

		void resetScrollOffset(RefreshesLayoutYN refreshesLayoutPre = RefreshesLayoutYN::Yes, RefreshesLayoutYN refreshesLayoutPost = RefreshesLayoutYN::Yes);

		void resetScrollOffsetRecursive(RefreshesLayoutYN refreshesLayoutPre = RefreshesLayoutYN::Yes, RefreshesLayoutYN refreshesLayoutPost = RefreshesLayoutYN::Yes);

		void draw() const;

		[[nodiscard]]
		const String& name() const;

		void setName(StringView name);

		[[nodiscard]]
		const RectF& rect() const;

		[[nodiscard]]
		const Vec2& effectScale() const;

		[[nodiscard]]
		const RectF& layoutAppliedRect() const;

		[[nodiscard]]
		const RectF& layoutAppliedRectWithMargin() const;

		[[nodiscard]]
		const Array<std::shared_ptr<Node>>& children() const;

		[[nodiscard]]
		bool hasChildren() const;

		[[nodiscard]]
		const Array<std::shared_ptr<ComponentBase>>& components() const;

		[[nodiscard]]
		InteractableYN interactable() const;

		void setInteractable(InteractableYN interactable);

		void setInteractable(bool interactable);

		[[nodiscard]]
		ActiveYN activeSelf() const;

		void setActive(ActiveYN activeSelf, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		void setActive(bool activeSelf, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		[[nodiscard]]
		ActiveYN activeInHierarchy() const;

		[[nodiscard]]
		IsHitTargetYN isHitTarget() const;

		void setIsHitTarget(IsHitTargetYN isHitTarget);

		void setIsHitTarget(bool isHitTarget);

		[[nodiscard]]
		InheritChildrenStateFlags inheritChildrenStateFlags() const;

		void setInheritChildrenStateFlags(InheritChildrenStateFlags flags);

		[[nodiscard]]
		bool inheritsChildrenHoveredState() const;

		void setInheritsChildrenHoveredState(bool value);

		[[nodiscard]]
		bool inheritsChildrenPressedState() const;

		void setInheritsChildrenPressedState(bool value);

		[[nodiscard]]
		ScrollableAxisFlags scrollableAxisFlags() const;

		void setScrollableAxisFlags(ScrollableAxisFlags flags, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		[[nodiscard]]
		bool horizontalScrollable() const;

		void setHorizontalScrollable(bool scrollable, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		[[nodiscard]]
		bool verticalScrollable() const;

		void setVerticalScrollable(bool scrollable, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		[[nodiscard]]
		ClippingEnabledYN clippingEnabled() const;

		void setClippingEnabled(ClippingEnabledYN clippingEnabled);

		void setClippingEnabled(bool clippingEnabled);

		[[nodiscard]]
		InteractState interactStateSelf() const;

		[[nodiscard]]
		InteractState currentInteractState() const;

		[[nodiscard]]
		SelectedYN selected() const;

		void setSelected(SelectedYN selected);

		void setSelected(bool selected);

		[[nodiscard]]
		bool isHovered() const;

		[[nodiscard]]
		bool isHoveredRecursive() const;

		[[nodiscard]]
		bool isPressed() const;

		[[nodiscard]]
		bool isPressedRecursive() const;

		[[nodiscard]]
		bool isPressedHover() const;

		[[nodiscard]]
		bool isPressedHoverRecursive() const;

		[[nodiscard]]
		bool isMouseDown() const;

		[[nodiscard]]
		bool isMouseDownRecursive() const;

		[[nodiscard]]
		bool isClicked() const;

		[[nodiscard]]
		bool isClickedRecursive() const;

		[[nodiscard]]
		bool isRightPressed() const;

		[[nodiscard]]
		bool isRightPressedRecursive() const;

		[[nodiscard]]
		bool isRightPressedHover() const;

		[[nodiscard]]
		bool isRightPressedHoverRecursive() const;

		[[nodiscard]]
		bool isRightMouseDown() const;

		[[nodiscard]]
		bool isRightMouseDownRecursive() const;

		[[nodiscard]]
		bool isRightClicked() const;

		[[nodiscard]]
		bool isRightClickedRecursive() const;

		void removeChildrenAll(RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		void swapChildren(const std::shared_ptr<Node>& child1, const std::shared_ptr<Node>& child2, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		void swapChildren(size_t index1, size_t index2, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		[[nodiscard]]
		size_t indexOfChild(const std::shared_ptr<Node>& child) const;

		[[nodiscard]]
		Optional<size_t> indexOfChildOpt(const std::shared_ptr<Node>& child) const;

		[[nodiscard]]
		std::shared_ptr<Node> clone() const;

		void addUpdater(std::function<void(const std::shared_ptr<Node>&)> updater);

		void addDrawer(std::function<void(const Node&)> drawer);

		void addOnClick(std::function<void(const std::shared_ptr<Node>&)> onClick);

		void addOnRightClick(std::function<void(const std::shared_ptr<Node>&)> onRightClick);

		void refreshContainedCanvasLayout();

		template <class Fty>
		void enumeratePlaceholdersWithTag(StringView tag, Fty&& func) const
			requires std::invocable<Fty, const std::shared_ptr<Node>&>;

		template <class Fty>
		void enumeratePlaceholdersWithTag(StringView tag, Fty&& func) const
			requires std::invocable<Fty, const std::shared_ptr<Node>&, const String&>;

		template <class Fty>
		void enumeratePlaceholdersWithTagRecursive(StringView tag, Fty&& func) const
			requires std::invocable<Fty, const std::shared_ptr<Node>&>;

		template <class Fty>
		void enumeratePlaceholdersWithTagRecursive(StringView tag, Fty&& func) const
			requires std::invocable<Fty, const std::shared_ptr<Node>&, const String&>;

		template <typename TData>
		void storeData(const TData& value);

		template <typename TData>
		[[nodiscard]]
		const TData& getStoredData() const;

		template <typename TData>
		[[nodiscard]]
		Optional<TData> getStoredDataOpt() const;

		template <typename TData>
		[[nodiscard]]	
		TData getStoredDataOr(const TData& defaultValue) const;
	};

	template<class TComponent, class ...Args>
	std::shared_ptr<TComponent> Node::emplaceComponent(Args && ...args)
		requires std::derived_from<TComponent, ComponentBase>&& std::is_constructible_v<TComponent, Args...>
	{
		auto component = std::make_shared<TComponent>(std::forward<Args>(args)...);
		addComponent(component);
		return component;
	}

	template<class ...Args>
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

	template <class Fty>
	Array<std::weak_ptr<Node>> Node::findAll(Fty&& predicate)
	{
		// 自分自身が条件を満たすかどうか
		Array<std::weak_ptr<Node>> result;
		if (predicate(shared_from_this()))
		{
			result.push_back(weak_from_this());
		}

		// 子ノードを再帰的に検索
		// TODO: Array生成を減らす
		for (const auto& child : m_children)
		{
			const auto found = child->findAll(predicate);
			result.insert(result.end(), found.begin(), found.end());
		}

		return result;
	}

	template <class TComponent>
	[[nodiscard]]
	std::shared_ptr<TComponent> Node::getComponent() const
		requires std::derived_from<TComponent, ComponentBase>
	{
		for (const auto& component : m_components)
		{
			if (auto concreteComponent = std::dynamic_pointer_cast<TComponent>(component))
			{
				return concreteComponent;
			}
		}
		throw Error{ U"Component not found in node '{}'"_fmt(m_name) };
	}

	template <class TComponent>
	[[nodiscard]]
	std::shared_ptr<TComponent> Node::getComponentOrNull() const
		requires std::derived_from<TComponent, ComponentBase>
	{
		for (const auto& component : m_components)
		{
			if (auto concreteComponent = std::dynamic_pointer_cast<TComponent>(component))
			{
				return concreteComponent;
			}
		}
		return nullptr;
	}

	template <class TComponent>
	[[nodiscard]]
	std::shared_ptr<TComponent> Node::getComponentRecursive() const
		requires std::derived_from<TComponent, ComponentBase>
	{
		if (const auto component = getComponentOrNull<TComponent>())
		{
			return component;
		}
		for (const auto& child : m_children)
		{
			if (const auto component = child->getComponentRecursive<TComponent>())
			{
				return component;
			}
		}
		throw Error{ U"Component not found in node '{}'"_fmt(m_name) };
	}

	template <class TComponent>
	[[nodiscard]]
	std::shared_ptr<TComponent> Node::getComponentRecursiveOrNull() const
		requires std::derived_from<TComponent, ComponentBase>
	{
		if (const auto component = getComponentOrNull<TComponent>())
		{
			return component;
		}
		for (const auto& child : m_children)
		{
			if (const auto component = child->getComponentRecursiveOrNull<TComponent>())
			{
				return component;
			}
		}
		return nullptr;
	}

	template <class Fty>
	void Node::enumeratePlaceholdersWithTag(StringView tag, Fty&& func) const
		requires std::invocable<Fty, const std::shared_ptr<Node>&>
	{
		for (const auto& child : m_children)
		{
			for (const auto& component : child->m_components)
			{
				if (const auto placeholder = std::dynamic_pointer_cast<Placeholder>(component))
				{
					if (placeholder->tag() == tag)
					{
						func(child);
					}
				}
			}
		}
	}

	template <class Fty>
	void Node::enumeratePlaceholdersWithTag(StringView tag, Fty&& func) const
		requires std::invocable<Fty, const std::shared_ptr<Node>&, const String&>
	{
		for (const auto& child : m_children)
		{
			for (const auto& component : child->m_components)
			{
				if (const auto placeholder = std::dynamic_pointer_cast<Placeholder>(component))
				{
					if (placeholder->tag() == tag)
					{
						func(child, placeholder->data());
					}
				}
			}
		}
	}

	template <class Fty>
	void Node::enumeratePlaceholdersWithTagRecursive(StringView tag, Fty&& func) const
		requires std::invocable<Fty, const std::shared_ptr<Node>&>
	{
		for (const auto& child : m_children)
		{
			child->enumeratePlaceholdersWithTagRecursive(tag, func);
			for (const auto& component : child->m_components)
			{
				if (const auto placeholder = std::dynamic_pointer_cast<Placeholder>(component))
				{
					if (placeholder->tag() == tag)
					{
						func(child);
					}
				}
			}
		}
	}

	template <class Fty>
	void Node::enumeratePlaceholdersWithTagRecursive(StringView tag, Fty&& func) const
		requires std::invocable<Fty, const std::shared_ptr<Node>&, const String&>
	{
		for (const auto& child : m_children)
		{
			child->enumeratePlaceholdersWithTagRecursive(tag, func);
			for (const auto& component : child->m_components)
			{
				if (const auto placeholder = std::dynamic_pointer_cast<Placeholder>(component))
				{
					if (placeholder->tag() == tag)
					{
						func(child, placeholder->data());
					}
				}
			}
		}
	}

	template<typename TData>
	void Node::storeData(const TData& value)
	{
		if (const auto dataStore = getComponentOrNull<DataStore<TData>>())
		{
			dataStore->setValue(value);
		}
		else
		{
			emplaceComponent<DataStore<TData>>(value);
		}
	}

	template<typename TData>
	const TData& Node::getStoredData() const
	{
		if (const auto dataStore = getComponentOrNull<DataStore<TData>>())
		{
			return dataStore->value();
		}
		else
		{
			throw Error{ U"Node::getStoredData: Node '{}' has no DataStore component of specified data type"_fmt(m_name) };
		}
	}

	template<typename TData>
	Optional<TData> Node::getStoredDataOpt() const
	{
		if (const auto dataStore = getComponentOrNull<DataStore<TData>>())
		{
			return dataStore->value();
		}
		else
		{
			return none;
		}
	}

	template<typename TData>
	TData Node::getStoredDataOr(const TData& defaultValue) const
	{
		if (const auto dataStore = getComponentOrNull<DataStore<TData>>())
		{
			return dataStore->value();
		}
		else
		{
			return defaultValue;
		}
	}

	template <class Fty>
	void HorizontalLayout::execute(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children, Fty fnSetRect) const
		requires std::invocable<Fty, const std::shared_ptr<Node>&, const RectF&>
	{
		// TODO: 都度生成しないよう外部に持ちたい
		Array<SizeF> sizes;
		sizes.reserve(children.size());
		Array<LRTB> margins;
		margins.reserve(children.size());

		double totalWidth = 0.0;
		double maxHeight = 0.0;
		double totalFlexibleWeight = 0.0;
		const double availableWidth = parentRect.w - (padding.left + padding.right);
		const double availableHeight = parentRect.h - (padding.top + padding.bottom);

		for (const auto& child : children)
		{
			if (!child->activeSelf()) // 親の影響を受けないようactiveSelfを使う
			{
				sizes.push_back(SizeF::Zero());
				margins.push_back(LRTB::Zero());
				continue;
			}
			if (const auto pBoxConstraint = std::get_if<BoxConstraint>(&child->constraint()))
			{
				const RectF measuredRect = pBoxConstraint->applyConstraint(
					RectF{ 0, 0, availableWidth, availableHeight }, // 計測用に親サイズだけ渡す
					Vec2::Zero());
				sizes.push_back(measuredRect.size);
				margins.push_back(pBoxConstraint->margin);

				const double childW = measuredRect.w + pBoxConstraint->margin.left + pBoxConstraint->margin.right;
				const double childH = measuredRect.h + pBoxConstraint->margin.top + pBoxConstraint->margin.bottom;
				totalWidth += childW;
				maxHeight = Max(maxHeight, childH);
				totalFlexibleWeight += Max(pBoxConstraint->flexibleWeight, 0.0);
			}
			else
			{
				// BoxConstraint以外は計測不要
				sizes.push_back(SizeF::Zero());
				margins.push_back(LRTB::Zero());
			}
		}

		if (totalFlexibleWeight > 0.0)
		{
			// flexibleWeightが設定されている場合は残りの幅を分配
			const double widthRemain = availableWidth - totalWidth;
			if (widthRemain > 0.0)
			{
				for (size_t i = 0; i < children.size(); ++i)
				{
					const auto& child = children[i];
					if (!child->activeSelf()) // 親の影響を受けないようactiveSelfを使う
					{
						continue;
					}
					if (const auto pBoxConstraint = std::get_if<BoxConstraint>(&child->constraint()))
					{
						if (pBoxConstraint->flexibleWeight <= 0.0)
						{
							continue;
						}
						sizes[i].x += widthRemain * pBoxConstraint->flexibleWeight / totalFlexibleWeight;
					}
				}
			}
			totalWidth = availableWidth;
		}

		double offsetX = padding.left;
		if (horizontalAlign == HorizontalAlign::Center)
		{
			offsetX += (availableWidth - totalWidth) / 2;
		}
		else if (horizontalAlign == HorizontalAlign::Right)
		{
			offsetX += availableWidth - totalWidth;
		}

		double offsetY = padding.top;
		if (verticalAlign == VerticalAlign::Middle)
		{
			offsetY += (availableHeight - maxHeight) / 2;
		}
		else if (verticalAlign == VerticalAlign::Bottom)
		{
			offsetY += availableHeight - maxHeight;
		}

		double currentX = parentRect.x + offsetX;
		for (size_t i = 0; i < children.size(); ++i)
		{
			const auto& child = children[i];
			if (!child->activeSelf()) // 親の影響を受けないようactiveSelfを使う
			{
				continue;
			}
			const SizeF& childSize = sizes[i];
			const LRTB& margin = margins[i];
			if (const auto pBoxConstraint = child->boxConstraint())
			{
				const double childX = currentX + margin.left;
				const double childTotalHeight = childSize.y + margin.top + margin.bottom;
				const double shiftY = maxHeight - childTotalHeight;
				double verticalRatio;
				switch (verticalAlign)
				{
				case VerticalAlign::Top:
					verticalRatio = 0.0;
					break;
				case VerticalAlign::Middle:
					verticalRatio = 0.5;
					break;
				case VerticalAlign::Bottom:
					verticalRatio = 1.0;
					break;
				default:
					throw Error{ U"HorizontalLayout::execute: Invalid verticalAlign" };
				}
				const double childY = parentRect.y + offsetY + margin.top + shiftY * verticalRatio;
				const RectF finalRect{ childX, childY, childSize.x, childSize.y };
				fnSetRect(child, finalRect);
				currentX += childSize.x + margin.left + margin.right;
			}
			else if (const auto pAnchorConstraint = child->anchorConstraint())
			{
				// AnchorConstraintはオフセット無視
				const RectF finalRect = pAnchorConstraint->applyConstraint(parentRect, Vec2::Zero());
				fnSetRect(child, finalRect);
			}
			else
			{
				throw Error{ U"HorizontalLayout::execute: Unknown constraint" };
			}
		}
	}

	template <class Fty>
	void VerticalLayout::execute(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children, Fty fnSetRect) const
		requires std::invocable<Fty, const std::shared_ptr<Node>&, const RectF&>
	{
		// TODO: 都度生成しないよう外部に持ちたい
		Array<SizeF> sizes;
		sizes.reserve(children.size());
		Array<LRTB> margins;
		margins.reserve(children.size());

		double totalHeight = 0.0;
		double maxWidth = 0.0;
		double totalFlexibleWeight = 0.0;
		const double availableWidth = parentRect.w - (padding.left + padding.right);
		const double availableHeight = parentRect.h - (padding.top + padding.bottom);

		for (const auto& child : children)
		{
			if (!child->activeSelf()) // 親の影響を受けないようactiveSelfを使う
			{
				sizes.push_back(SizeF::Zero());
				margins.push_back(LRTB::Zero());
				continue;
			}
			if (const auto pBoxConstraint = std::get_if<BoxConstraint>(&child->constraint()))
			{
				const RectF measuredRect = pBoxConstraint->applyConstraint(
					RectF{ 0, 0, availableWidth, availableHeight }, // 計測用に親サイズだけ渡す
					Vec2::Zero());
				sizes.push_back(measuredRect.size);
				margins.push_back(pBoxConstraint->margin);

				const double childW = measuredRect.w + pBoxConstraint->margin.left + pBoxConstraint->margin.right;
				const double childH = measuredRect.h + pBoxConstraint->margin.top + pBoxConstraint->margin.bottom;
				totalHeight += childH;
				maxWidth = Max(maxWidth, childW);
				totalFlexibleWeight += Max(pBoxConstraint->flexibleWeight, 0.0);
			}
			else
			{
				// BoxConstraint以外は計測不要
				sizes.push_back(SizeF::Zero());
				margins.push_back(LRTB::Zero());
			}
		}

		if (totalFlexibleWeight > 0.0)
		{
			// flexibleWeightが設定されている場合は残りの高さを分配
			const double heightRemain = availableHeight - totalHeight;
			if (heightRemain > 0.0)
			{
				for (size_t i = 0; i < children.size(); ++i)
				{
					const auto& child = children[i];
					if (!child->activeSelf()) // 親の影響を受けないようactiveSelfを使う
					{
						continue;
					}
					if (const auto pBoxConstraint = std::get_if<BoxConstraint>(&child->constraint()))
					{
						if (pBoxConstraint->flexibleWeight <= 0.0)
						{
							continue;
						}
						sizes[i].y += heightRemain * pBoxConstraint->flexibleWeight / totalFlexibleWeight;
					}
				}
			}
			totalHeight = availableHeight;
		}

		double offsetY = padding.top;
		if (verticalAlign == VerticalAlign::Middle)
		{
			offsetY += (availableHeight - totalHeight) / 2;
		}
		else if (verticalAlign == VerticalAlign::Bottom)
		{
			offsetY += availableHeight - totalHeight;
		}

		double offsetX = padding.left;
		if (horizontalAlign == HorizontalAlign::Center)
		{
			offsetX += (availableWidth - maxWidth) / 2;
		}
		else if (horizontalAlign == HorizontalAlign::Right)
		{
			offsetX += availableWidth - maxWidth;
		}

		double currentY = parentRect.y + offsetY;
		for (size_t i = 0; i < children.size(); ++i)
		{
			const auto& child = children[i];
			if (!child->activeSelf()) // 親の影響を受けないようactiveSelfを使う
			{
				continue;
			}
			const SizeF& childSize = sizes[i];
			const LRTB& margin = margins[i];
			if (const auto pBoxConstraint = child->boxConstraint())
			{
				const double childY = currentY + margin.top;
				const double childTotalWidth = childSize.x + margin.left + margin.right;
				const double shiftX = maxWidth - childTotalWidth;
				double horizontalRatio;
				switch (horizontalAlign)
				{
				case HorizontalAlign::Left:
					horizontalRatio = 0.0;
					break;
				case HorizontalAlign::Center:
					horizontalRatio = 0.5;
					break;
				case HorizontalAlign::Right:
					horizontalRatio = 1.0;
					break;
				default:
					throw Error{ U"VerticalLayout::execute: Invalid horizontalAlign" };
				}
				const double childX = parentRect.x + offsetX + margin.left + shiftX * horizontalRatio;
				const RectF finalRect{ childX, childY, childSize.x, childSize.y };
				fnSetRect(child, finalRect);
				currentY += childSize.y + margin.top + margin.bottom;
			}
			else if (const auto pAnchorConstraint = child->anchorConstraint())
			{
				// AnchorConstraintはオフセット無視
				const RectF finalRect = pAnchorConstraint->applyConstraint(parentRect, Vec2::Zero());
				fnSetRect(child, finalRect);
			}
			else
			{
				throw Error{ U"VerticalLayout::execute: Unknown constraint" };
			}
		}
	}
}
