﻿#pragma once
#include <Siv3D.hpp>
#include "YN.hpp"
#include "PropertyValue.hpp"
#include "Property.hpp"
#include "InheritChildrenStateFlags.hpp"
#include "ScrollableAxisFlags.hpp"
#include "InteractionState.hpp"
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
		static inline uint64_t s_nextInternalId = 1;
		uint64 m_internalId;
		String m_name;
		ConstraintVariant m_constraint;
		TransformEffect m_transformEffect;
		LayoutVariant m_boxChildrenLayout;
		Array<std::shared_ptr<Node>> m_children;
		Array<std::shared_ptr<ComponentBase>> m_components;
		IsHitTargetYN m_isHitTarget;
		LRTB m_hitTestPadding{ 0.0, 0.0, 0.0, 0.0 };
		InheritChildrenStateFlags m_inheritChildrenStateFlags = InheritChildrenStateFlags::None;
		InteractableYN m_interactable = InteractableYN::Yes;
		ScrollableAxisFlags m_scrollableAxisFlags = ScrollableAxisFlags::None;
		ScrollMethodFlags m_scrollMethodFlags = ScrollMethodFlags::Wheel | ScrollMethodFlags::Drag;
		double m_decelerationRate = 0.2; // 慣性スクロールの減衰率
		RubberBandScrollEnabledYN m_rubberBandScrollEnabled = RubberBandScrollEnabledYN::Yes; // ラバーバンドスクロールを有効にするか
		ClippingEnabledYN m_clippingEnabled = ClippingEnabledYN::No;
		
		ActiveYN m_activeSelf = ActiveYN::Yes;

		/* NonSerialized */ std::weak_ptr<Canvas> m_canvas;
		/* NonSerialized */ std::weak_ptr<Node> m_parent;
		/* NonSerialized */ RectF m_layoutAppliedRect{ 0.0, 0.0, 0.0, 0.0 };
		/* NonSerialized */ RectF m_posScaleAppliedRect{ 0.0, 0.0, 0.0, 0.0 };
		/* NonSerialized */ RectF m_hitTestRect{ 0.0, 0.0, 0.0, 0.0 };
		/* NonSerialized */ Vec2 m_effectScale{ 1.0, 1.0 };
		/* NonSerialized */ Vec2 m_scrollOffset{ 0.0, 0.0 };
		/* NonSerialized */ Smoothing<double> m_scrollBarAlpha{ 0.0 };
		/* NonSerialized */ MouseTracker m_mouseLTracker;
		/* NonSerialized */ MouseTracker m_mouseRTracker;
		/* NonSerialized */ ActiveYN m_activeInHierarchy = ActiveYN::Yes;
		/* NonSerialized */ ActiveYN m_prevActiveInHierarchy = ActiveYN::No;
		/* NonSerialized */ String m_styleState = U"";
		/* NonSerialized */ Array<String> m_activeStyleStates;  // 現在のactiveStyleStates（親から受け取ったもの + 自身）
		/* NonSerialized */ InteractionState m_currentInteractionState = InteractionState::Default;
		/* NonSerialized */ InteractionState m_currentInteractionStateRight = InteractionState::Default;
		/* NonSerialized */ bool m_clickRequested = false;
		/* NonSerialized */ bool m_rightClickRequested = false;
		/* NonSerialized */ bool m_prevClickRequested = false;
		/* NonSerialized */ bool m_prevRightClickRequested = false;
		/* NonSerialized */ Array<std::shared_ptr<ComponentBase>> m_componentTempBuffer; // 一時バッファ
		/* NonSerialized */ Array<std::shared_ptr<Node>> m_childrenTempBuffer; // 一時バッファ
		/* NonSerialized */ Optional<Vec2> m_dragStartPos; // ドラッグ開始位置
		/* NonSerialized */ Vec2 m_dragStartScrollOffset{ 0.0, 0.0 }; // ドラッグ開始時のスクロールオフセット
		/* NonSerialized */ Vec2 m_scrollVelocity{ 0.0, 0.0 }; // スクロール速度
		/* NonSerialized */ Stopwatch m_dragVelocityStopwatch; // ドラッグ速度計算用ストップウォッチ
		/* NonSerialized */ bool m_dragThresholdExceeded = false; // ドラッグ閾値を超えたかどうか
		/* NonSerialized */ Optional<Vec2> m_rubberBandTargetOffset; // ラバーバンドスクロールの戻り先
		/* NonSerialized */ double m_rubberBandAnimationTime = 0.0; // ラバーバンドアニメーション経過時間
		/* NonSerialized */ bool m_preventDragScroll = false; // ドラッグスクロールを阻止するか

		[[nodiscard]]
		explicit Node(uint64 internalId, StringView name, const ConstraintVariant& constraint, IsHitTargetYN isHitTarget, InheritChildrenStateFlags inheritChildrenStateFlags)
			: m_internalId{ internalId }
			, m_name{ name }
			, m_constraint{ constraint }
			, m_isHitTarget{ isHitTarget }
			, m_inheritChildrenStateFlags{ inheritChildrenStateFlags }
			, m_mouseLTracker{ MouseL, m_interactable }
			, m_mouseRTracker{ MouseR, m_interactable }
		{
		}

		[[nodiscard]]
		InteractionState updateForCurrentInteractionState(const std::shared_ptr<Node>& hoveredNode, InteractableYN parentInteractable, IsScrollingYN isAncestorScrolling);

		[[nodiscard]]
		InteractionState updateForCurrentInteractionStateRight(const std::shared_ptr<Node>& hoveredNode, InteractableYN parentInteractable, IsScrollingYN isAncestorScrolling);

		void refreshActiveInHierarchy();

		void setCanvasRecursive(const std::weak_ptr<Canvas>& canvas);

		void clampScrollOffset();

		[[nodiscard]]
		std::pair<Vec2, Vec2> getValidScrollRange() const;

	public:
		[[nodiscard]]
		static std::shared_ptr<Node> Create(StringView name = U"Node", const ConstraintVariant& constraint = BoxConstraint{}, IsHitTargetYN isHitTarget = IsHitTargetYN::Yes, InheritChildrenStateFlags inheritChildrenStateFlags = InheritChildrenStateFlags::None);

		[[nodiscard]]
		const ConstraintVariant& constraint() const;

		std::shared_ptr<Node> setConstraint(const ConstraintVariant& constraint, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		[[nodiscard]]
		const BoxConstraint* boxConstraint() const;

		[[nodiscard]]
		const AnchorConstraint* anchorConstraint() const;

		[[nodiscard]]
		TransformEffect& transformEffect();

		[[nodiscard]]
		const TransformEffect& transformEffect() const;

		[[nodiscard]]
		const LayoutVariant& boxChildrenLayout() const;

		std::shared_ptr<Node> setBoxChildrenLayout(const LayoutVariant& layout, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		[[nodiscard]]
		const FlowLayout* childrenFlowLayout() const;

		[[nodiscard]]
		const HorizontalLayout* childrenHorizontalLayout() const;

		[[nodiscard]]
		const VerticalLayout* childrenVerticalLayout() const;

		[[nodiscard]]
		SizeF getFittingSizeToChildren() const;

		std::shared_ptr<Node> setBoxConstraintToFitToChildren(FitTarget fitTarget = FitTarget::Both, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		[[nodiscard]]
		const LRTB& boxChildrenLayoutPadding() const;

		[[nodiscard]]
		bool hasBoxConstraint() const;

		[[nodiscard]]
		bool hasAnchorConstraint() const;

		[[nodiscard]]
		JSON toJSON() const;

		[[nodiscard]]
		JSON toJSONImpl(detail::IncludesInternalIdYN includesInternalId) const;

		[[nodiscard]]
		static std::shared_ptr<Node> CreateFromJSON(const JSON& json);

		[[nodiscard]]
		static std::shared_ptr<Node> CreateFromJSONImpl(const JSON& json, detail::IncludesInternalIdYN includesInternalId);

		[[nodiscard]]
		std::shared_ptr<Node> parent() const;

		[[nodiscard]]
		std::shared_ptr<const Node> findHoverTargetParent() const;

		[[nodiscard]]
		std::shared_ptr<Node> findHoverTargetParent();

		[[nodiscard]]
		bool isAncestorOf(const std::shared_ptr<Node>& node) const;

		std::shared_ptr<Node> setParent(const std::shared_ptr<Node>& parent, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		bool removeFromParent(RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		[[nodiscard]]
		size_t siblingIndex() const;

		[[nodiscard]]
		Optional<size_t> siblingIndexOpt() const;

		[[nodiscard]]
		std::shared_ptr<Canvas> containedCanvas() const;

		template <typename TComponent>
		std::shared_ptr<TComponent> addComponent(const std::shared_ptr<TComponent>& component)
			requires std::derived_from<TComponent, ComponentBase>;

		template <typename TComponent>
		std::shared_ptr<TComponent> addComponentAtIndex(const std::shared_ptr<TComponent>& component, size_t index)
			requires std::derived_from<TComponent, ComponentBase>;

		std::shared_ptr<ComponentBase> addComponentFromJSON(const JSON& json);

		std::shared_ptr<ComponentBase> addComponentAtIndexFromJSON(const JSON& json, size_t index);

		std::shared_ptr<ComponentBase> addComponentFromJSONImpl(const JSON& json, detail::IncludesInternalIdYN includesInternalId);

		std::shared_ptr<ComponentBase> addComponentAtIndexFromJSONImpl(const JSON& json, size_t index, detail::IncludesInternalIdYN includesInternalId);

		void removeComponent(const std::shared_ptr<ComponentBase>& component);

		template <typename Predicate>
		void removeComponentsIf(Predicate predicate)
		{
			m_components.remove_if(std::move(predicate));
		}

		template <typename TComponent>
		void removeComponentsAll(RecursiveYN recursive);

		template <class TComponent, class... Args>
		std::shared_ptr<TComponent> emplaceComponent(Args&&... args)
			requires std::derived_from<TComponent, ComponentBase> && std::is_constructible_v<TComponent, Args...>;

		bool moveComponentUp(const std::shared_ptr<ComponentBase>& component);

		bool moveComponentDown(const std::shared_ptr<ComponentBase>& component);

		const std::shared_ptr<Node>& addChild(std::shared_ptr<Node>&& child, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		const std::shared_ptr<Node>& addChild(const std::shared_ptr<Node>& child, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		const std::shared_ptr<Node>& emplaceChild(StringView name = U"Node", const ConstraintVariant& constraint = BoxConstraint{}, IsHitTargetYN isHitTarget = IsHitTargetYN::Yes, InheritChildrenStateFlags inheritChildrenStateFlags = InheritChildrenStateFlags::None, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		const std::shared_ptr<Node>& emplaceChild(RefreshesLayoutYN refreshesLayout);

		const std::shared_ptr<Node>& addChildFromJSON(const JSON& json, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		const std::shared_ptr<Node>& addChildAtIndexFromJSON(const JSON& json, size_t index, RefreshesLayoutYN refreshesLayout);

		const std::shared_ptr<Node>& addChildAtIndex(const std::shared_ptr<Node>& child, size_t index, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		void removeChild(const std::shared_ptr<Node>& child, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		[[nodiscard]]
		bool containsChild(const std::shared_ptr<Node>& child, RecursiveYN recursive = RecursiveYN::No) const;

		[[nodiscard]]
		bool containsChildByName(StringView name, RecursiveYN recursive = RecursiveYN::No) const;

		template <class Fty>
		[[nodiscard]]
		Array<std::weak_ptr<Node>> findAll(Fty&& predicate)
			requires std::invocable<Fty, const std::shared_ptr<Node>&>;

		template<class Fty>
		void findAll(Fty&& predicate, Array<std::weak_ptr<Node>>* pResults, ClearsArrayYN clearsArray = ClearsArrayYN::Yes)
			requires std::invocable<Fty, const std::shared_ptr<Node>&>;

		template <class TComponent>
		[[nodiscard]]
		std::shared_ptr<TComponent> getComponent(RecursiveYN recursive = RecursiveYN::No) const;

		template <class TComponent>
		[[nodiscard]]
		std::shared_ptr<TComponent> getComponentOrNull(RecursiveYN recursive = RecursiveYN::No) const;

		[[nodiscard]]
		std::shared_ptr<Node> getChildByName(StringView name, RecursiveYN recursive = RecursiveYN::No);

		[[nodiscard]]
		std::shared_ptr<Node> getChildByNameOrNull(StringView name, RecursiveYN recursive = RecursiveYN::No);

		void refreshBoxChildrenLayout();

		[[nodiscard]]
		Optional<RectF> getBoxChildrenContentRect() const;

		[[nodiscard]]
		Optional<RectF> getBoxChildrenContentRectWithPadding() const;

		[[nodiscard]]
		std::shared_ptr<Node> hoveredNodeRecursive();

		[[nodiscard]]
		std::shared_ptr<Node> hitTest(const Vec2& point);

		[[nodiscard]]
		std::shared_ptr<const Node> hoveredNodeRecursive() const;

		[[nodiscard]]
		std::shared_ptr<const Node> hitTest(const Vec2& point) const;

		[[nodiscard]]
		std::shared_ptr<Node> findContainedScrollableNode();

		void updateInteractionState(const std::shared_ptr<Node>& hoveredNode, double deltaTime, InteractableYN parentInteractable, InteractionState parentInteractionState, InteractionState parentInteractionStateRight, IsScrollingYN isAncestorScrolling = IsScrollingYN::No);

		void updateInput();

		void update(const std::shared_ptr<Node>& scrollableHoveredNode, double deltaTime, const Mat3x2& parentPosScaleMat, const Vec2& parentEffectScale, const Mat3x2& parentHitTestMat, const Array<String>& parentActiveStyleStates = {});

		void lateUpdate();

		void postLateUpdate(double deltaTime);

		void refreshPosScaleAppliedRect(const Mat3x2& parentPosScaleMat, const Vec2& parentEffectScale, const Mat3x2& parentHitTestMat);

		void scroll(const Vec2& offsetDelta, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		Vec2 scrollOffset() const;

		void resetScrollOffset(RecursiveYN recursive = RecursiveYN::No, RefreshesLayoutYN refreshesLayoutPre = RefreshesLayoutYN::Yes, RefreshesLayoutYN refreshesLayoutPost = RefreshesLayoutYN::Yes);

		void draw() const;

		void requestClick();

		void requestRightClick();

		[[nodiscard]]
		const String& name() const;

		std::shared_ptr<Node> setName(StringView name);

		[[nodiscard]]
		const RectF& rect() const;

		[[nodiscard]]
		const RectF& hitTestRect() const;

		[[nodiscard]]
		const Vec2& effectScale() const;

		[[nodiscard]]
		const RectF& layoutAppliedRect() const;

		[[nodiscard]]
		RectF layoutAppliedRectWithMargin() const;

		[[nodiscard]]
		const Array<std::shared_ptr<Node>>& children() const;

		[[nodiscard]]
		bool hasChildren() const;

		[[nodiscard]]
		const Array<std::shared_ptr<ComponentBase>>& components() const;

		[[nodiscard]]
		InteractableYN interactable() const;

		std::shared_ptr<Node> setInteractable(InteractableYN interactable);

		std::shared_ptr<Node> setInteractable(bool interactable);

		[[nodiscard]]
		bool interactableInHierarchy() const;

		[[nodiscard]]
		ActiveYN activeSelf() const;

		std::shared_ptr<Node> setActive(ActiveYN activeSelf, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		std::shared_ptr<Node> setActive(bool activeSelf, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		[[nodiscard]]
		ActiveYN activeInHierarchy() const;

		[[nodiscard]]
		IsHitTargetYN isHitTarget() const;

		std::shared_ptr<Node> setIsHitTarget(IsHitTargetYN isHitTarget);

		std::shared_ptr<Node> setIsHitTarget(bool isHitTarget);

		[[nodiscard]]
		const LRTB& hitTestPadding() const;

		std::shared_ptr<Node> setHitTestPadding(const LRTB& padding);

		[[nodiscard]]
		InheritChildrenStateFlags inheritChildrenStateFlags() const;

		std::shared_ptr<Node> setInheritChildrenStateFlags(InheritChildrenStateFlags flags);

		[[nodiscard]]
		bool inheritsChildrenHoveredState() const;

		std::shared_ptr<Node> setInheritsChildrenHoveredState(bool value);

		[[nodiscard]]
		bool inheritsChildrenPressedState() const;

		std::shared_ptr<Node> setInheritsChildrenPressedState(bool value);

		[[nodiscard]]
		ScrollableAxisFlags scrollableAxisFlags() const;

		std::shared_ptr<Node> setScrollableAxisFlags(ScrollableAxisFlags flags, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		[[nodiscard]]
		bool horizontalScrollable() const;

		std::shared_ptr<Node> setHorizontalScrollable(bool scrollable, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		[[nodiscard]]
		bool verticalScrollable() const;

		std::shared_ptr<Node> setVerticalScrollable(bool scrollable, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);
		
		[[nodiscard]]
		ScrollMethodFlags scrollMethodFlags() const;
		
		std::shared_ptr<Node> setScrollMethodFlags(ScrollMethodFlags flags);
		
		[[nodiscard]]
		bool wheelScrollEnabled() const;
		
		std::shared_ptr<Node> setWheelScrollEnabled(bool enabled);
		
		[[nodiscard]]
		bool dragScrollEnabled() const;
		
		std::shared_ptr<Node> setDragScrollEnabled(bool enabled);
		
		[[nodiscard]]
		double decelerationRate() const;
		
		std::shared_ptr<Node> setDecelerationRate(double rate);

		[[nodiscard]]
		RubberBandScrollEnabledYN rubberBandScrollEnabled() const;

		std::shared_ptr<Node> setRubberBandScrollEnabled(RubberBandScrollEnabledYN rubberBandScrollEnabled);

		std::shared_ptr<Node> setRubberBandScrollEnabled(bool rubberBandScrollEnabled);

		// ドラッグスクロール阻止
		void preventDragScroll();

		[[nodiscard]]
		ClippingEnabledYN clippingEnabled() const;

		std::shared_ptr<Node> setClippingEnabled(ClippingEnabledYN clippingEnabled);

		std::shared_ptr<Node> setClippingEnabled(bool clippingEnabled);

		[[nodiscard]]
		InteractionState interactionStateSelf() const;

		[[nodiscard]]
		InteractionState currentInteractionState() const;

		[[nodiscard]]
		const String& styleState() const { return m_styleState; }

		std::shared_ptr<Node> setStyleState(const String& state)
		{
			m_styleState = state;
			return shared_from_this();
		}

		[[nodiscard]]
		bool isHovered(RecursiveYN recursive = RecursiveYN::No, IncludingDisabledYN includingDisabled = IncludingDisabledYN::No) const;

		[[nodiscard]]
		bool isPressed(RecursiveYN recursive = RecursiveYN::No, IncludingDisabledYN includingDisabled = IncludingDisabledYN::No) const;

		[[nodiscard]]
		bool isPressedHover(RecursiveYN recursive = RecursiveYN::No, IncludingDisabledYN includingDisabled = IncludingDisabledYN::No) const;

		[[nodiscard]]
		bool isMouseDown(RecursiveYN recursive = RecursiveYN::No, IncludingDisabledYN includingDisabled = IncludingDisabledYN::No) const;

		[[nodiscard]]
		bool isClicked(RecursiveYN recursive = RecursiveYN::No, IncludingDisabledYN includingDisabled = IncludingDisabledYN::No) const;

		[[nodiscard]]
		bool isClickRequested(RecursiveYN recursive = RecursiveYN::No, IncludingDisabledYN includingDisabled = IncludingDisabledYN::No) const;

		[[nodiscard]]
		bool isRightPressed(RecursiveYN recursive = RecursiveYN::No, IncludingDisabledYN includingDisabled = IncludingDisabledYN::No) const;

		[[nodiscard]]
		bool isRightPressedHover(RecursiveYN recursive = RecursiveYN::No, IncludingDisabledYN includingDisabled = IncludingDisabledYN::No) const;

		[[nodiscard]]
		bool isRightMouseDown(RecursiveYN recursive = RecursiveYN::No, IncludingDisabledYN includingDisabled = IncludingDisabledYN::No) const;

		[[nodiscard]]
		bool isRightClicked(RecursiveYN recursive = RecursiveYN::No, IncludingDisabledYN includingDisabled = IncludingDisabledYN::No) const;

		[[nodiscard]]
		bool isRightClickRequested(RecursiveYN recursive = RecursiveYN::No, IncludingDisabledYN includingDisabled = IncludingDisabledYN::No) const;

		void removeChildrenAll(RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		void swapChildren(const std::shared_ptr<Node>& child1, const std::shared_ptr<Node>& child2, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		void swapChildren(size_t index1, size_t index2, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		[[nodiscard]]
		size_t indexOfChild(const std::shared_ptr<Node>& child) const;

		[[nodiscard]]
		Optional<size_t> indexOfChildOpt(const std::shared_ptr<Node>& child) const;

		[[nodiscard]]
		std::shared_ptr<Node> clone() const;

		std::shared_ptr<Node> addInputUpdater(std::function<void(const std::shared_ptr<Node>&)> inputUpdater);

		std::shared_ptr<Node> addUpdater(std::function<void(const std::shared_ptr<Node>&)> updater);

		std::shared_ptr<Node> addDrawer(std::function<void(const Node&)> drawer);

		std::shared_ptr<Node> addOnClick(std::function<void(const std::shared_ptr<Node>&)> onClick);

		std::shared_ptr<Node> addOnClick(std::function<void()> onClick);

		std::shared_ptr<Node> addOnRightClick(std::function<void(const std::shared_ptr<Node>&)> onRightClick);

		std::shared_ptr<Node> addOnRightClick(std::function<void()> onRightClick);

		std::shared_ptr<Node> addClickHotKey(const Input& input, EnabledWhileTextEditingYN enabledWhileTextEditing, ClearsInputYN clearsInput = ClearsInputYN::Yes);

		std::shared_ptr<Node> addClickHotKey(const Input& input, CtrlYN ctrl = CtrlYN::No, AltYN alt = AltYN::No, ShiftYN shift = ShiftYN::No, EnabledWhileTextEditingYN enabledWhileTextEditing = EnabledWhileTextEditingYN::No, ClearsInputYN clearsInput = ClearsInputYN::Yes);

		std::shared_ptr<Node> addRightClickHotKey(const Input& input, EnabledWhileTextEditingYN enabledWhileTextEditing, ClearsInputYN clearsInput = ClearsInputYN::Yes);

		std::shared_ptr<Node> addRightClickHotKey(const Input& input, CtrlYN ctrl = CtrlYN::No, AltYN alt = AltYN::No, ShiftYN shift = ShiftYN::No, EnabledWhileTextEditingYN enabledWhileTextEditing = EnabledWhileTextEditingYN::No, ClearsInputYN clearsInput = ClearsInputYN::Yes);

		void refreshContainedCanvasLayout();

		template <class Fty>
		void walkPlaceholders(StringView tag, Fty&& func, RecursiveYN recursive = RecursiveYN::Yes) const
			requires std::invocable<Fty, const std::shared_ptr<Node>&>;

		template <class Fty>
		void walkPlaceholders(StringView tag, Fty&& func, RecursiveYN recursive = RecursiveYN::Yes) const
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

		uint64 internalId() const
		{
			return m_internalId;
		}
	};

	template <typename TComponent>
	std::shared_ptr<TComponent> Node::addComponent(const std::shared_ptr<TComponent>& component)
		requires std::derived_from<TComponent, ComponentBase>
	{
		m_components.push_back(component);
		return component;
	}

	template <typename TComponent>
	std::shared_ptr<TComponent> Node::addComponentAtIndex(const std::shared_ptr<TComponent>& component, size_t index)
		requires std::derived_from<TComponent, ComponentBase>
	{
		if (index > m_components.size())
		{
			index = m_components.size();
		}
		m_components.insert(m_components.begin() + index, component);
		return component;
	}

	template<class TComponent, class ...Args>
	std::shared_ptr<TComponent> Node::emplaceComponent(Args && ...args)
		requires std::derived_from<TComponent, ComponentBase>&& std::is_constructible_v<TComponent, Args...>
	{
		auto component = std::make_shared<TComponent>(std::forward<Args>(args)...);
		addComponent(component);
		return component;
	}

	template <class Fty>
	Array<std::weak_ptr<Node>> Node::findAll(Fty&& predicate)
		requires std::invocable<Fty, const std::shared_ptr<Node>&>
	{
		Array<std::weak_ptr<Node>> result;
		findAll(std::forward<Fty>(predicate), &result, ClearsArrayYN::Yes);
		return result;
	}

	template <class Fty>
	void Node::findAll(Fty&& predicate, Array<std::weak_ptr<Node>>* pResults, ClearsArrayYN clearsArray)
		requires std::invocable<Fty, const std::shared_ptr<Node>&>
	{
		if (pResults == nullptr)
		{
			throw Error{ U"Node::findAll: pResults is nullptr" };
		}

		if (clearsArray)
		{
			pResults->clear();
		}

		// 自分自身が条件を満たすかどうか
		if (predicate(shared_from_this()))
		{
			pResults->push_back(weak_from_this());
		}

		// 子ノードを再帰的に検索
		for (const auto& child : m_children)
		{
			child->findAll(predicate, pResults, ClearsArrayYN::No);
		}
	}

	template <class TComponent>
	[[nodiscard]]
	std::shared_ptr<TComponent> Node::getComponent(RecursiveYN recursive) const
	{
		if (const auto component = getComponentOrNull<TComponent>(RecursiveYN::No))
		{
			return component;
		}
		if (recursive)
		{
			for (const auto& child : m_children)
			{
				if (const auto component = child->getComponent<TComponent>(RecursiveYN::Yes))
				{
					return component;
				}
			}
		}
		throw Error{ U"Component not found in node '{}'"_fmt(m_name) };
	}

	template <class TComponent>
	[[nodiscard]]
	std::shared_ptr<TComponent> Node::getComponentOrNull(RecursiveYN recursive) const
	{
		for (const auto& component : m_components)
		{
			if (auto concreteComponent = std::dynamic_pointer_cast<TComponent>(component))
			{
				return concreteComponent;
			}
		}
		if (recursive)
		{
			for (const auto& child : m_children)
			{
				if (const auto component = child->getComponentOrNull<TComponent>(RecursiveYN::Yes))
				{
					return component;
				}
			}
		}
		return nullptr;
	}


	template <class Fty>
	void Node::walkPlaceholders(StringView tag, Fty&& func, RecursiveYN recursive) const
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
			if (recursive)
			{
				child->walkPlaceholders(tag, func, RecursiveYN::Yes);
			}
		}
	}

	template <class Fty>
	void Node::walkPlaceholders(StringView tag, Fty&& func, RecursiveYN recursive) const
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
			if (recursive)
			{
				child->walkPlaceholders(tag, func, RecursiveYN::Yes);
			}
		}
	}

	template<typename TData>
	void Node::storeData(const TData& value)
	{
		if (const auto dataStore = getComponentOrNull<DataStore<TData>>(RecursiveYN::No))
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
		if (const auto dataStore = getComponentOrNull<DataStore<TData>>(RecursiveYN::No))
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
		if (const auto dataStore = getComponentOrNull<DataStore<TData>>(RecursiveYN::No))
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
		if (const auto dataStore = getComponentOrNull<DataStore<TData>>(RecursiveYN::No))
		{
			return dataStore->value();
		}
		else
		{
			return defaultValue;
		}
	}

	template <typename TComponent>
	void Node::removeComponentsAll(RecursiveYN recursive)
	{
		// 自身のコンポーネントから指定された型のものを削除
		m_components.remove_if([](const std::shared_ptr<ComponentBase>& component)
		{
			return std::dynamic_pointer_cast<TComponent>(component) != nullptr;
		});

		// 再帰的に処理する場合は子ノードも処理
		if (recursive == RecursiveYN::Yes)
		{
			for (const auto& child : m_children)
			{
				child->removeComponentsAll<TComponent>(RecursiveYN::Yes);
			}
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

		{
			bool isFirstBoxConstraintChild = true;
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
					if (!isFirstBoxConstraintChild)
					{
						totalWidth += spacing;
					}
					totalWidth += childW;
					maxHeight = Max(maxHeight, childH);
					totalFlexibleWeight += Max(pBoxConstraint->flexibleWeight, 0.0);
					isFirstBoxConstraintChild = false;
				}
				else
				{
					// BoxConstraint以外は計測不要
					sizes.push_back(SizeF::Zero());
					margins.push_back(LRTB::Zero());
				}
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

		{
			double currentX = parentRect.x + offsetX;
			bool isFirstBoxConstraintChild = true;
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
					if (!isFirstBoxConstraintChild)
					{
						currentX += spacing;
					}
					isFirstBoxConstraintChild = false;
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

		{
			bool isFirstBoxConstraintChild = true;
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
					if (!isFirstBoxConstraintChild)
					{
						totalHeight += spacing;
					}
					totalHeight += childH;
					maxWidth = Max(maxWidth, childW);
					totalFlexibleWeight += Max(pBoxConstraint->flexibleWeight, 0.0);
					isFirstBoxConstraintChild = false;
				}
				else
				{
					// BoxConstraint以外は計測不要
					sizes.push_back(SizeF::Zero());
					margins.push_back(LRTB::Zero());
				}
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

		{
			double currentY = parentRect.y + offsetY;
			bool isFirstBoxConstraintChild = true;
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
					if (!isFirstBoxConstraintChild)
					{
						currentY += spacing;
					}
					isFirstBoxConstraintChild = false;
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

	template <class Fty>
	void FlowLayout::execute(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children, Fty fnSetRect) const
		requires std::invocable<Fty, const std::shared_ptr<Node>&, const RectF&>
	{
		const auto measureInfo = measure(parentRect, children);
		const double availableWidth = parentRect.w - (padding.left + padding.right);

		// 実際に配置していく
		double offsetY = padding.top;
		if (verticalAlign == VerticalAlign::Middle || verticalAlign == VerticalAlign::Bottom)
		{
			// spacing.y を考慮した高さを計算
			double totalHeight = 0.0;
			for (size_t i = 0; i < measureInfo.lines.size(); ++i)
			{
				totalHeight += measureInfo.lines[i].maxHeight;
				if (i < measureInfo.lines.size() - 1)
				{
					totalHeight += spacing.y;
				}
			}
			const double availableHeight = parentRect.h - (padding.top + padding.bottom);
			if (verticalAlign == VerticalAlign::Middle)
			{
				offsetY += (availableHeight - totalHeight) / 2;
			}
			else if (verticalAlign == VerticalAlign::Bottom)
			{
				offsetY += availableHeight - totalHeight;
			}
		}
		for (size_t lineIndex = 0; lineIndex < measureInfo.lines.size(); ++lineIndex)
		{
			auto& line = measureInfo.lines[lineIndex];
			double offsetX = padding.left;
			if (horizontalAlign == HorizontalAlign::Center)
			{
				offsetX += (availableWidth - line.totalWidth) / 2;
			}
			else if (horizontalAlign == HorizontalAlign::Right)
			{
				offsetX += availableWidth - line.totalWidth;
			}
			const double lineHeight = line.maxHeight;
			bool boxConstraintChildPlaced = false;
			for (const size_t index : line.childIndices)
			{
				const auto& child = children[index];
				if (child->hasBoxConstraint())
				{
					if (boxConstraintChildPlaced)
					{
						offsetX += spacing.x;
					}
					boxConstraintChildPlaced = true;
				}
				const RectF finalRect = executeChild(parentRect, child, measureInfo.measuredChildren[index], offsetY, lineHeight, &offsetX);
				fnSetRect(child, finalRect);
			}
			offsetY += lineHeight;
			// 最後の行以外は spacing.y を追加
			if (lineIndex < measureInfo.lines.size() - 1)
			{
				offsetY += spacing.y;
			}
		}
	}
}
