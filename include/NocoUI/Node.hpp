#pragma once
#include <Siv3D.hpp>
#include "YN.hpp"
#include "PropertyValue.hpp"
#include "Property.hpp"
#include "InheritChildrenStateFlags.hpp"
#include "ScrollableAxisFlags.hpp"
#include "InteractionState.hpp"
#include "MouseTracker.hpp"
#include "Transform.hpp"
#include "Region/Region.hpp"
#include "Layout/Layout.hpp"
#include "Component/ComponentBase.hpp"
#include "Component/DataStore.hpp"
#include "Enums.hpp"
#include "Param.hpp"
#include "INodeContainer.hpp"

namespace noco
{
	class Canvas;
	class ComponentFactory;

	struct CanvasUpdateContext;

	class Node : public INodeContainer, public std::enable_shared_from_this<Node>
	{
		friend class Canvas;

	private:
		static inline uint64_t s_nextInstanceId = 1;
		uint64 m_instanceId;
		String m_name;
		RegionVariant m_region;
		Transform m_transform;
		LayoutVariant m_childrenLayout = FlowLayout{};
		Array<std::shared_ptr<Node>> m_children;
		Array<std::shared_ptr<ComponentBase>> m_components;
		IsHitTargetYN m_isHitTarget;
		LRTB m_hitPadding{ 0.0, 0.0, 0.0, 0.0 };
		InheritChildrenStateFlags m_inheritChildrenStateFlags = InheritChildrenStateFlags::None;
		PropertyNonInteractive<bool> m_interactable{ U"interactable", true };
		ScrollableAxisFlags m_scrollableAxisFlags = ScrollableAxisFlags::None;
		ScrollMethodFlags m_scrollMethodFlags = ScrollMethodFlags::Wheel | ScrollMethodFlags::Drag;
		double m_decelerationRate = 0.2; // 慣性スクロールの減衰率
		RubberBandScrollEnabledYN m_rubberBandScrollEnabled = RubberBandScrollEnabledYN::Yes; // ラバーバンドスクロールを有効にするか
		ClippingEnabledYN m_clippingEnabled = ClippingEnabledYN::No;
		PropertyNonInteractive<bool> m_activeSelf{ U"activeSelf", true };
		Property<int32> m_siblingZIndex{ U"siblingZIndex", 0 };

		/* NonSerialized */ std::weak_ptr<Canvas> m_canvas;
		/* NonSerialized */ std::weak_ptr<Node> m_parent;
		/* NonSerialized */ RectF m_regionRect{ 0.0, 0.0, 0.0, 0.0 };
		/* NonSerialized */ Quad m_transformedQuad;
		/* NonSerialized */ Quad m_hitQuad;
		/* NonSerialized */ Quad m_hitQuadWithPadding;
		/* NonSerialized */ Vec2 m_scrollOffset{ 0.0, 0.0 };
		/* NonSerialized */ Smoothing<double> m_scrollBarAlpha{ 0.0 };
		/* NonSerialized */ MouseTracker m_mouseLTracker;
		/* NonSerialized */ MouseTracker m_mouseRTracker;
		/* NonSerialized */ ActiveYN m_activeInHierarchy = ActiveYN::No;
		/* NonSerialized */ ActiveYN m_prevActiveInHierarchy = ActiveYN::No;
		/* NonSerialized */ ActiveYN m_activeInHierarchyForDraw = ActiveYN::No;
		/* NonSerialized */ PropertyNonInteractive<String> m_styleState{ U"styleState", U"" };
		/* NonSerialized */ Array<String> m_activeStyleStates;  // 現在のactiveStyleStates（親から受け取ったもの + 自身）
		/* NonSerialized */ InteractionState m_currentInteractionState = InteractionState::Default;
		/* NonSerialized */ InteractionState m_currentInteractionStateRight = InteractionState::Default;
		/* NonSerialized */ bool m_clickRequested = false;
		/* NonSerialized */ bool m_rightClickRequested = false;
		/* NonSerialized */ bool m_prevClickRequested = false;
		/* NonSerialized */ bool m_prevRightClickRequested = false;
		/* NonSerialized */ Optional<Vec2> m_dragStartPos; // ドラッグ開始位置
		/* NonSerialized */ Vec2 m_dragStartScrollOffset{ 0.0, 0.0 }; // ドラッグ開始時のスクロールオフセット
		/* NonSerialized */ Vec2 m_scrollVelocity{ 0.0, 0.0 }; // スクロール速度
		/* NonSerialized */ Stopwatch m_dragVelocityStopwatch; // ドラッグ速度計算用ストップウォッチ
		/* NonSerialized */ bool m_dragThresholdExceeded = false; // ドラッグ閾値を超えたかどうか
		/* NonSerialized */ Optional<Vec2> m_rubberBandTargetOffset; // ラバーバンドスクロールの戻り先
		/* NonSerialized */ double m_rubberBandAnimationTime = 0.0; // ラバーバンドアニメーション経過時間
		/* NonSerialized */ bool m_preventDragScroll = false; // ドラッグスクロールを阻止するか
		/* NonSerialized */ Mat3x2 m_transformMatInHierarchy = Mat3x2::Identity(); // 階層内での変換行列
		/* NonSerialized */ Mat3x2 m_hitTestMatInHierarchy = Mat3x2::Identity(); // 階層内でのヒットテスト用変換行列
		/* NonSerialized */ Optional<bool> m_prevActiveSelfAfterUpdateNodeParams; // 前回のupdateNodeParams後のactiveSelf
		/* NonSerialized */ Optional<bool> m_prevActiveSelfParamOverrideAfterUpdateNodeParams; // 前回のupdateNodeParams後のactiveSelfの上書き値
		/* NonSerialized */ mutable Array<std::shared_ptr<Node>> m_tempChildrenBuffer; // 子ノードの一時バッファ(update内で別のNodeのupdateが呼ばれる場合があるためthread_local staticにはできない。drawで呼ぶためmutableだが、drawはシングルスレッド前提なのでロック不要)
		/* NonSerialized */ mutable Array<std::shared_ptr<ComponentBase>> m_tempComponentsBuffer; // コンポーネントの一時バッファ(update内で別のNodeのupdateが呼ばれる場合があるためthread_local staticにはできない。drawで呼ぶためmutableだが、drawはシングルスレッド前提なのでロック不要)

		[[nodiscard]]
		Mat3x2 calculateHitTestMat(const Mat3x2& parentHitTestMat) const;

		[[nodiscard]]
		explicit Node(uint64 instanceId, StringView name, const RegionVariant& region, IsHitTargetYN isHitTarget, InheritChildrenStateFlags inheritChildrenStateFlags)
			: m_instanceId{ instanceId }
			, m_name{ name }
			, m_region{ region }
			, m_isHitTarget{ isHitTarget }
			, m_inheritChildrenStateFlags{ inheritChildrenStateFlags }
			, m_mouseLTracker{ MouseL, InteractableYN{ m_interactable.value() } }
			, m_mouseRTracker{ MouseR, InteractableYN{ m_interactable.value() } }
		{
		}

		[[nodiscard]]
		InteractionState updateForCurrentInteractionState(const std::shared_ptr<Node>& hoveredNode, InteractableYN parentInteractable, IsScrollingYN isAncestorScrolling, const HashTable<String, ParamValue>& params);

		[[nodiscard]]
		InteractionState updateForCurrentInteractionStateRight(const std::shared_ptr<Node>& hoveredNode, InteractableYN parentInteractable, IsScrollingYN isAncestorScrolling, const HashTable<String, ParamValue>& params);

		void refreshActiveInHierarchy();

		void refreshPropertiesForInteractable(InteractableYN effectiveInteractable, SkipsSmoothingYN skipsSmoothing);

		void refreshChildrenPropertiesForInteractableRecursive(InteractableYN interactable, const HashTable<String, ParamValue>& params, SkipsSmoothingYN skipsSmoothing);

		void setCanvasRecursive(const std::weak_ptr<Canvas>& canvas);

		void clampScrollOffset();

		[[nodiscard]]
		std::pair<Vec2, Vec2> getValidScrollRange() const;

	public:
		[[nodiscard]]
		static std::shared_ptr<Node> Create(StringView name = U"Node", const RegionVariant& region = InlineRegion{}, IsHitTargetYN isHitTarget = IsHitTargetYN::Yes, InheritChildrenStateFlags inheritChildrenStateFlags = InheritChildrenStateFlags::None);

		[[nodiscard]]
		const RegionVariant& region() const;

		std::shared_ptr<Node> setRegion(const RegionVariant& region);

		[[nodiscard]]
		const InlineRegion* inlineRegion() const;

		[[nodiscard]]
		const AnchorRegion* anchorRegion() const;

		[[nodiscard]]
		Transform& transform();

		[[nodiscard]]
		const Transform& transform() const;

		[[nodiscard]]
		const LayoutVariant& childrenLayout() const override;

		std::shared_ptr<Node> setChildrenLayout(const LayoutVariant& layout);

		[[nodiscard]]
		const FlowLayout* childrenFlowLayout() const override;

		[[nodiscard]]
		const HorizontalLayout* childrenHorizontalLayout() const override;

		[[nodiscard]]
		const VerticalLayout* childrenVerticalLayout() const override;

		[[nodiscard]]
		SizeF getFittingSizeToChildren() const;

		std::shared_ptr<Node> setInlineRegionToFitToChildren(FitTarget fitTarget = FitTarget::Both);

		[[nodiscard]]
		const LRTB& childrenLayoutPadding() const;

		[[nodiscard]]
		bool hasInlineRegion() const;

		[[nodiscard]]
		bool hasAnchorRegion() const;

		[[nodiscard]]
		JSON toJSON(detail::WithInstanceIdYN withInstanceId = detail::WithInstanceIdYN::No) const;

		[[nodiscard]]
		static std::shared_ptr<Node> CreateFromJSON(const JSON& json, detail::WithInstanceIdYN withInstanceId = detail::WithInstanceIdYN::No);

		[[nodiscard]]
		static std::shared_ptr<Node> CreateFromJSON(const JSON& json, const ComponentFactory& factory, detail::WithInstanceIdYN withInstanceId = detail::WithInstanceIdYN::No);

		[[nodiscard]]
		std::shared_ptr<Node> parentNode() const;

		[[nodiscard]]
		std::shared_ptr<INodeContainer> parentContainer() const;

		[[nodiscard]]
		bool isTopLevelNode() const;

		[[nodiscard]]
		std::shared_ptr<const Node> findHoverTargetParent() const;

		[[nodiscard]]
		std::shared_ptr<Node> findHoverTargetParent();

		[[nodiscard]]
		bool isAncestorOf(const std::shared_ptr<Node>& node) const;

		std::shared_ptr<Node> setParent(const std::shared_ptr<Node>& parent);

		bool removeFromParent();

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
		
		std::shared_ptr<ComponentBase> addComponentFromJSON(const JSON& json, const ComponentFactory& factory);

		std::shared_ptr<ComponentBase> addComponentAtIndexFromJSON(const JSON& json, size_t index);
		
		std::shared_ptr<ComponentBase> addComponentAtIndexFromJSON(const JSON& json, size_t index, const ComponentFactory& factory);

		std::shared_ptr<ComponentBase> addComponentFromJSONImpl(const JSON& json, detail::WithInstanceIdYN withInstanceId);
		
		std::shared_ptr<ComponentBase> addComponentFromJSONImpl(const JSON& json, const ComponentFactory& factory, detail::WithInstanceIdYN withInstanceId);

		std::shared_ptr<ComponentBase> addComponentAtIndexFromJSONImpl(const JSON& json, size_t index, detail::WithInstanceIdYN withInstanceId);
		
		std::shared_ptr<ComponentBase> addComponentAtIndexFromJSONImpl(const JSON& json, size_t index, const ComponentFactory& factory, detail::WithInstanceIdYN withInstanceId);

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

		const std::shared_ptr<Node>& addChild(std::shared_ptr<Node>&& child);

		const std::shared_ptr<Node>& addChild(const std::shared_ptr<Node>& child) override;

		const std::shared_ptr<Node>& emplaceChild(StringView name = U"Node", const RegionVariant& region = InlineRegion{}, IsHitTargetYN isHitTarget = IsHitTargetYN::Yes, InheritChildrenStateFlags inheritChildrenStateFlags = InheritChildrenStateFlags::None) override;

		const std::shared_ptr<Node>& addChildFromJSON(const JSON& json) override;
		
		const std::shared_ptr<Node>& addChildFromJSON(const JSON& json, const ComponentFactory& factory) override;

		const std::shared_ptr<Node>& addChildAtIndexFromJSON(const JSON& json, size_t index);
		
		const std::shared_ptr<Node>& addChildAtIndexFromJSON(const JSON& json, size_t index, const ComponentFactory& factory);

		const std::shared_ptr<Node>& addChildAtIndex(const std::shared_ptr<Node>& child, size_t index) override;

		void removeChild(const std::shared_ptr<Node>& child) override;

		[[nodiscard]]
		bool containsChild(const std::shared_ptr<Node>& child, RecursiveYN recursive = RecursiveYN::No) const override;

		[[nodiscard]]
		bool containsChildByName(StringView name, RecursiveYN recursive = RecursiveYN::No) const override;

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
		std::shared_ptr<Node> getChildByName(StringView name, RecursiveYN recursive = RecursiveYN::No) override;

		[[nodiscard]]
		std::shared_ptr<Node> getChildByNameOrNull(StringView name, RecursiveYN recursive = RecursiveYN::No) override;

		void refreshChildrenLayout();

		[[nodiscard]]
		Optional<RectF> getChildrenContentRect() const;

		[[nodiscard]]
		Optional<RectF> getChildrenContentRectWithPadding() const;

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

		void updateNodeParams(const HashTable<String, ParamValue>& params);

		void updateInteractionState(const std::shared_ptr<Node>& hoveredNode, double deltaTime, InteractableYN parentInteractable, InteractionState parentInteractionState, InteractionState parentInteractionStateRight, IsScrollingYN isAncestorScrolling, const HashTable<String, ParamValue>& params, const Array<String>& parentActiveStyleStates);

		void updateKeyInput();

		void update(const std::shared_ptr<Node>& scrollableHoveredNode, double deltaTime, const Mat3x2& parentTransformMat, const Mat3x2& parentHitTestMat, const HashTable<String, ParamValue>& params);

		void lateUpdate();

		void postLateUpdate(double deltaTime, const HashTable<String, ParamValue>& params);

		void refreshTransformMat(RecursiveYN recursive, const Mat3x2& parentTransformMat, const Mat3x2& parentHitTestMat, const HashTable<String, ParamValue>& params);

		void scroll(const Vec2& offsetDelta);

		Vec2 scrollOffset() const;

		void resetScrollOffset(RecursiveYN recursive = RecursiveYN::No);

		// パラメータ参照を置換
		void replaceParamRefs(const String& oldName, const String& newName, RecursiveYN recursive = RecursiveYN::Yes);

		void clearCurrentFrameOverride(RecursiveYN recursive = RecursiveYN::Yes);

		void draw() const;

		void requestClick();

		void requestRightClick();

		[[nodiscard]]
		const String& name() const;

		std::shared_ptr<Node> setName(StringView name);

		// 回転を除いた矩形を取得
		[[nodiscard]]
		RectF unrotatedTransformedRect() const;

		[[nodiscard]]
		double extractRotationFromTransformMat() const;

		[[nodiscard]]
		Vec2 transformScaleInHierarchy() const;

		[[nodiscard]]
		const Mat3x2& hitTestMatInHierarchy() const;

		[[nodiscard]]
		Vec2 inverseTransformHitTestPoint(const Vec2& point) const;

		[[nodiscard]]
		Vec2 transformPivotPos() const;

		[[nodiscard]]
		const Quad& transformedQuad() const;

		[[nodiscard]]
		Quad hitQuad(WithPaddingYN withPadding = WithPaddingYN::No) const;

		[[nodiscard]]
		const RectF& regionRect() const;

		[[nodiscard]]
		RectF regionRectWithMargin() const;

		[[nodiscard]]
		const Mat3x2& transformMatInHierarchy() const;

		[[nodiscard]]
		bool hasChildren() const;

		[[nodiscard]]
		const Array<std::shared_ptr<ComponentBase>>& components() const;

		[[nodiscard]]
		bool interactable() const;

		std::shared_ptr<Node> setInteractable(InteractableYN interactable);

		std::shared_ptr<Node> setInteractable(bool interactable);
		
		[[nodiscard]]
		const String& interactableParamRef() const { return m_interactable.paramRef(); }
		
		std::shared_ptr<Node> setInteractableParamRef(const String& paramRef)
		{
			m_interactable.setParamRef(paramRef);
			return shared_from_this();
		}
		
		[[nodiscard]]
		PropertyNonInteractive<bool>& interactableProperty() { return m_interactable; }
		
		[[nodiscard]]
		const PropertyNonInteractive<bool>& interactableProperty() const { return m_interactable; }

		[[nodiscard]]
		bool interactableInHierarchy() const;

		[[nodiscard]]
		bool activeSelf() const;

		std::shared_ptr<Node> setActive(ActiveYN activeSelf);

		std::shared_ptr<Node> setActive(bool activeSelf);
		
		[[nodiscard]]
		const String& activeSelfParamRef() const { return m_activeSelf.paramRef(); }
		
		std::shared_ptr<Node> setActiveSelfParamRef(const String& paramRef)
		{
			m_activeSelf.setParamRef(paramRef);
			return shared_from_this();
		}
		
		[[nodiscard]]
		PropertyNonInteractive<bool>& activeSelfProperty() { return m_activeSelf; }
		
		[[nodiscard]]
		const PropertyNonInteractive<bool>& activeSelfProperty() const { return m_activeSelf; }

		[[nodiscard]]
		bool activeInHierarchy() const;

		[[nodiscard]]
		bool isHitTarget() const;

		std::shared_ptr<Node> setIsHitTarget(IsHitTargetYN isHitTarget);

		std::shared_ptr<Node> setIsHitTarget(bool isHitTarget);

		[[nodiscard]]
		const LRTB& hitPadding() const;

		std::shared_ptr<Node> setHitPadding(const LRTB& padding);

		[[nodiscard]]
		InheritChildrenStateFlags inheritChildrenStateFlags() const;

		std::shared_ptr<Node> setInheritChildrenStateFlags(InheritChildrenStateFlags flags);

		[[nodiscard]]
		bool inheritChildrenHover() const;

		std::shared_ptr<Node> setInheritChildrenHover(bool value);

		[[nodiscard]]
		bool inheritChildrenPress() const;

		std::shared_ptr<Node> setInheritChildrenPress(bool value);

		[[nodiscard]]
		ScrollableAxisFlags scrollableAxisFlags() const;

		std::shared_ptr<Node> setScrollableAxisFlags(ScrollableAxisFlags flags);

		[[nodiscard]]
		bool horizontalScrollable() const;

		std::shared_ptr<Node> setHorizontalScrollable(bool scrollable);

		[[nodiscard]]
		bool verticalScrollable() const;

		std::shared_ptr<Node> setVerticalScrollable(bool scrollable);
		
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
		bool rubberBandScrollEnabled() const;

		std::shared_ptr<Node> setRubberBandScrollEnabled(RubberBandScrollEnabledYN rubberBandScrollEnabled);

		std::shared_ptr<Node> setRubberBandScrollEnabled(bool rubberBandScrollEnabled);

		// ドラッグスクロール阻止
		void preventDragScroll();

		[[nodiscard]]
		bool clippingEnabled() const;

		std::shared_ptr<Node> setClippingEnabled(ClippingEnabledYN clippingEnabled);

		std::shared_ptr<Node> setClippingEnabled(bool clippingEnabled);

		[[nodiscard]]
		InteractionState interactionStateSelf() const;

		[[nodiscard]]
		InteractionState currentInteractionState() const;

		[[nodiscard]]
		const String& styleState() const { return m_styleState.value(); }

		std::shared_ptr<Node> setStyleState(const String& state)
		{
			m_styleState.setValue(state);
			return shared_from_this();
		}

		std::shared_ptr<Node> clearStyleState()
		{
			m_styleState.setValue(U"");
			return shared_from_this();
		}
		
		[[nodiscard]]
		const String& styleStateParamRef() const { return m_styleState.paramRef(); }
		
		std::shared_ptr<Node> setStyleStateParamRef(const String& paramRef)
		{
			m_styleState.setParamRef(paramRef);
			return shared_from_this();
		}
		
		[[nodiscard]]
		PropertyNonInteractive<String>& styleStateProperty() { return m_styleState; }
		
		[[nodiscard]]
		const PropertyNonInteractive<String>& styleStateProperty() const { return m_styleState; }

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

		void removeChildrenAll() override;

		void swapChildren(const std::shared_ptr<Node>& child1, const std::shared_ptr<Node>& child2);

		void swapChildren(size_t index1, size_t index2) override;

		[[nodiscard]]
		size_t indexOfChild(const std::shared_ptr<Node>& child) const;

		[[nodiscard]]
		Optional<size_t> indexOfChildOpt(const std::shared_ptr<Node>& child) const override;

		[[nodiscard]]
		std::shared_ptr<Node> clone() const;

		std::shared_ptr<Node> addKeyInputUpdater(std::function<void(const std::shared_ptr<Node>&)> keyInputUpdater);

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

		void refreshContainedCanvasLayoutImmediately(OnlyIfDirtyYN onlyIfDirty = OnlyIfDirtyYN::Yes);
		
		void markLayoutAsDirty();

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

		uint64 instanceId() const
		{
			return m_instanceId;
		}

		// INodeContainer interface implementation
		[[nodiscard]]
		const Array<std::shared_ptr<Node>>& children() const override
		{
			return m_children;
		}

		[[nodiscard]]
		size_t childCount() const override
		{
			return m_children.size();
		}

		[[nodiscard]]
		std::shared_ptr<Node> childAt(size_t index) const override;

		[[nodiscard]]
		bool isNode() const override
		{
			return true;
		}

		[[nodiscard]]
		bool isCanvas() const override
		{
			return false;
		}

		[[nodiscard]]
		int32 siblingZIndex() const;

		std::shared_ptr<Node> setSiblingZIndex(const PropertyValue<int32>& siblingZIndex);

		[[nodiscard]]
		const PropertyValue<int32>& siblingZIndexPropertyValue() const;

		[[nodiscard]]
		const String& siblingZIndexParamRef() const { return m_siblingZIndex.paramRef(); }

		std::shared_ptr<Node> setSiblingZIndexParamRef(const String& paramRef)
		{
			m_siblingZIndex.setParamRef(paramRef);
			return shared_from_this();
		}

		[[nodiscard]]
		Property<int32>& siblingZIndexProperty() { return m_siblingZIndex; }

		[[nodiscard]]
		const Property<int32>& siblingZIndexProperty() const { return m_siblingZIndex; }

	};

	template <typename TComponent>
	std::shared_ptr<TComponent> Node::addComponent(const std::shared_ptr<TComponent>& component)
		requires std::derived_from<TComponent, ComponentBase>
	{
		m_components.push_back(component);
		
		if (m_activeInHierarchy)
		{
			component->onActivated(shared_from_this());
		}
		
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
		
		if (m_activeInHierarchy)
		{
			component->onActivated(shared_from_this());
		}
		
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
		t_tempSizes.clear();
		t_tempMargins.clear();
		t_tempSizes.reserve(children.size());
		t_tempMargins.reserve(children.size());
		
		auto& sizes = t_tempSizes;
		auto& margins = t_tempMargins;

		double totalWidth = 0.0;
		double maxHeight = 0.0;
		double totalFlexibleWeight = 0.0;
		const double availableWidth = parentRect.w - (padding.left + padding.right);
		const double availableHeight = parentRect.h - (padding.top + padding.bottom);

		{
			bool isFirstInlineRegionChild = true;
			for (const auto& child : children)
			{
				if (!child->activeSelf()) // 親の影響を受けないようactiveSelfを使う
				{
					sizes.push_back(SizeF::Zero());
					margins.push_back(LRTB::Zero());
					continue;
				}
				if (const auto pInlineRegion = std::get_if<InlineRegion>(&child->region()))
				{
					const RectF measuredRect = pInlineRegion->applyRegion(
						RectF{ 0, 0, availableWidth, availableHeight }, // 計測用に親サイズだけ渡す
						Vec2::Zero());
					sizes.push_back(measuredRect.size);
					margins.push_back(pInlineRegion->margin);

					const double childW = measuredRect.w + pInlineRegion->margin.left + pInlineRegion->margin.right;
					const double childH = measuredRect.h + pInlineRegion->margin.top + pInlineRegion->margin.bottom;
					if (!isFirstInlineRegionChild)
					{
						totalWidth += spacing;
					}
					totalWidth += childW;
					maxHeight = Max(maxHeight, childH);
					totalFlexibleWeight += Max(pInlineRegion->flexibleWeight, 0.0);
					isFirstInlineRegionChild = false;
				}
				else
				{
					// InlineRegion以外は計測不要
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
					if (const auto pInlineRegion = std::get_if<InlineRegion>(&child->region()))
					{
						if (pInlineRegion->flexibleWeight <= 0.0)
						{
							continue;
						}
						sizes[i].x += widthRemain * pInlineRegion->flexibleWeight / totalFlexibleWeight;
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
			bool isFirstInlineRegionChild = true;
			for (size_t i = 0; i < children.size(); ++i)
			{
				const auto& child = children[i];
				if (!child->activeSelf()) // 親の影響を受けないようactiveSelfを使う
				{
					continue;
				}
				const SizeF& childSize = sizes[i];
				const LRTB& margin = margins[i];
				if (const auto pInlineRegion = child->inlineRegion())
				{
					if (!isFirstInlineRegionChild)
					{
						currentX += spacing;
					}
					isFirstInlineRegionChild = false;
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
				else if (const auto pAnchorRegion = child->anchorRegion())
				{
					// AnchorRegionはオフセット無視
					const RectF finalRect = pAnchorRegion->applyRegion(parentRect, Vec2::Zero());
					fnSetRect(child, finalRect);
				}
				else
				{
					throw Error{ U"HorizontalLayout::execute: Unknown region" };
				}
			}
		}
	}

	template <class Fty>
	void VerticalLayout::execute(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children, Fty fnSetRect) const
		requires std::invocable<Fty, const std::shared_ptr<Node>&, const RectF&>
	{
		t_tempSizes.clear();
		t_tempMargins.clear();
		t_tempSizes.reserve(children.size());
		t_tempMargins.reserve(children.size());
		
		auto& sizes = t_tempSizes;
		auto& margins = t_tempMargins;

		double totalHeight = 0.0;
		double maxWidth = 0.0;
		double totalFlexibleWeight = 0.0;
		const double availableWidth = parentRect.w - (padding.left + padding.right);
		const double availableHeight = parentRect.h - (padding.top + padding.bottom);

		{
			bool isFirstInlineRegionChild = true;
			for (const auto& child : children)
			{
				if (!child->activeSelf()) // 親の影響を受けないようactiveSelfを使う
				{
					sizes.push_back(SizeF::Zero());
					margins.push_back(LRTB::Zero());
					continue;
				}
				if (const auto pInlineRegion = std::get_if<InlineRegion>(&child->region()))
				{
					const RectF measuredRect = pInlineRegion->applyRegion(
						RectF{ 0, 0, availableWidth, availableHeight }, // 計測用に親サイズだけ渡す
						Vec2::Zero());
					sizes.push_back(measuredRect.size);
					margins.push_back(pInlineRegion->margin);

					const double childW = measuredRect.w + pInlineRegion->margin.left + pInlineRegion->margin.right;
					const double childH = measuredRect.h + pInlineRegion->margin.top + pInlineRegion->margin.bottom;
					if (!isFirstInlineRegionChild)
					{
						totalHeight += spacing;
					}
					totalHeight += childH;
					maxWidth = Max(maxWidth, childW);
					totalFlexibleWeight += Max(pInlineRegion->flexibleWeight, 0.0);
					isFirstInlineRegionChild = false;
				}
				else
				{
					// InlineRegion以外は計測不要
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
					if (const auto pInlineRegion = std::get_if<InlineRegion>(&child->region()))
					{
						if (pInlineRegion->flexibleWeight <= 0.0)
						{
							continue;
						}
						sizes[i].y += heightRemain * pInlineRegion->flexibleWeight / totalFlexibleWeight;
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
			bool isFirstInlineRegionChild = true;
			for (size_t i = 0; i < children.size(); ++i)
			{
				const auto& child = children[i];
				if (!child->activeSelf()) // 親の影響を受けないようactiveSelfを使う
				{
					continue;
				}
				const SizeF& childSize = sizes[i];
				const LRTB& margin = margins[i];
				if (const auto pInlineRegion = child->inlineRegion())
				{
					if (!isFirstInlineRegionChild)
					{
						currentY += spacing;
					}
					isFirstInlineRegionChild = false;
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
				else if (const auto pAnchorRegion = child->anchorRegion())
				{
					// AnchorRegionはオフセット無視
					const RectF finalRect = pAnchorRegion->applyRegion(parentRect, Vec2::Zero());
					fnSetRect(child, finalRect);
				}
				else
				{
					throw Error{ U"VerticalLayout::execute: Unknown region" };
				}
			}
		}
	}

	template <class Fty>
	void FlowLayout::execute(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children, Fty fnSetRect) const
		requires std::invocable<Fty, const std::shared_ptr<Node>&, const RectF&>
	{
		static thread_local FlowLayout::MeasureInfo t_measureInfoBuffer;
		measure(parentRect, children, &t_measureInfoBuffer);
		const auto& measureInfo = t_measureInfoBuffer;
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
			bool inlineRegionChildPlaced = false;
			for (const size_t index : line.childIndices)
			{
				const auto& child = children[index];
				if (child->hasInlineRegion())
				{
					if (inlineRegionChildPlaced)
					{
						offsetX += spacing.x;
					}
					inlineRegionChildPlaced = true;
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
