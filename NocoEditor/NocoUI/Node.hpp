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
#include "Enums.hpp"

namespace noco
{
	struct CanvasUpdateContext;

	class Node : public std::enable_shared_from_this<Node>
	{
		friend class Canvas;

	private:
		String m_name;
		ConstraintVariant m_constraint;
		TransformEffect m_transformEffect;
		LayoutVariant m_layout;
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
		/* NonSerialized */ Timer m_scrollBarTimerH{ 0.5s };
		/* NonSerialized */ Timer m_scrollBarTimerV{ 0.5s };
		/* NonSerialized */ MouseTracker m_mouseLTracker;
		/* NonSerialized */ MouseTracker m_mouseRTracker;
		/* NonSerialized */ ActiveYN m_activeInHierarchy = ActiveYN::Yes;
		/* NonSerialized */ Optional<ActiveYN> m_prevActiveInHierarchy = none;
		/* NonSerialized */ SelectedYN m_selected = SelectedYN::No;
		/* NonSerialized */ InteractState m_currentInteractState = InteractState::Default;
		/* NonSerialized */ InteractState m_currentInteractStateRight = InteractState::Default;

		// イテレーション中の追加・削除で例外を送出するためのガード
		// (ユーザーコードの呼び出しを含むonActivated/onDeactivated/update/drawのみ対応。シングルスレッドのみ想定)
		struct IterationGuard
		{
			size_t count = 0;

			struct ScopedIterationGuard
			{
				IterationGuard& guard;

				ScopedIterationGuard(IterationGuard& _guard)
					: guard{ _guard }
				{
					++guard.count;
				}

				~ScopedIterationGuard()
				{
					--guard.count;
				}
			};

			[[nodiscard]]
			bool isIterating() const
			{
				return count > 0;
			}

			[[nodiscard]]
			ScopedIterationGuard scoped()
			{
				return ScopedIterationGuard{ *this };
			}
		};
		/* NonSerialized */ mutable IterationGuard m_childrenIterGuard;
		/* NonSerialized */ mutable IterationGuard m_componentsIterGuard;

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
		const LayoutVariant& layout() const;

		void setLayout(const LayoutVariant& layout, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		[[nodiscard]]
		const FlowLayout* flowLayout() const;

		[[nodiscard]]
		const HorizontalLayout* horizontalLayout() const;

		[[nodiscard]]
		const VerticalLayout* verticalLayout() const;

		void setBoxConstraintToFitToChildren(FitTarget fitTarget = FitTarget::Both, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		[[nodiscard]]
		const LRTB& layoutPadding() const;

		[[nodiscard]]
		bool isParentLayoutAffected() const;

		[[nodiscard]]
		JSON toJSON() const;

		[[nodiscard]]
		static std::shared_ptr<Node> FromJSON(const JSON& json);

		[[nodiscard]]
		std::shared_ptr<Node> parent() const;

		bool removeFromParent();

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
		Array<std::weak_ptr<Node>> findAll(Fty predicate) const;

		template <class TComponent>
		[[nodiscard]]
		std::shared_ptr<TComponent> getComponent()
			requires std::derived_from<TComponent, ComponentBase>;

		template <class TComponent>
		[[nodiscard]]
		std::shared_ptr<TComponent> getComponentOrNull()
			requires std::derived_from<TComponent, ComponentBase>;

		template <class TComponent>
		[[nodiscard]]
		std::shared_ptr<TComponent> getComponentRecursive()
			requires std::derived_from<TComponent, ComponentBase>;

		template <class TComponent>
		[[nodiscard]]
		std::shared_ptr<TComponent> getComponentRecursiveOrNull()
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

		void update(CanvasUpdateContext* pContext, const std::shared_ptr<Node>& hoveredNode, double deltaTime, const Mat3x2& parentEffectMat, const Vec2& parentEffectScale, InteractableYN parentInteractable, InteractState parentInteractState, InteractState parentInteractStateRight);

		void refreshEffectedRect(const Mat3x2& parentEffectMat, const Vec2& parentEffectScale);

		void scroll(const Vec2& offsetDelta, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

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
		std::shared_ptr<Node> clone() const;

		void addUpdater(std::function<void(const std::shared_ptr<Node>&)> updater);

		void addDrawer(std::function<void(const Node&)> drawer);

		void addOnClick(std::function<void(const std::shared_ptr<Node>&)> onClick);

		void addOnRightClick(std::function<void(const std::shared_ptr<Node>&)> onRightClick);

		void refreshContainedCanvasLayout();
	};

	template<class TComponent, class ...Args>
	std::shared_ptr<TComponent> Node::emplaceComponent(Args && ...args)
		requires std::derived_from<TComponent, ComponentBase>&& std::is_constructible_v<TComponent, Args...>
	{
		if (m_componentsIterGuard.isIterating())
		{
			throw Error{ U"emplaceComponent: Cannot emplace component while iterating" };
		}
		auto component = std::make_shared<TComponent>(std::forward<Args>(args)...);
		addComponent(component);
		return component;
	}

	template<class ...Args>
	const std::shared_ptr<Node>& Node::addChildFromJSON(const JSON& json, RefreshesLayoutYN refreshesLayout)
	{
		if (m_childrenIterGuard.isIterating())
		{
			throw Error{ U"addChildFromJSON: Cannot add child while iterating" };
		}
		auto child = FromJSON(json);
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
	Array<std::weak_ptr<Node>> Node::findAll(Fty predicate) const
	{
		// 自分自身が条件を満たすかどうか
		Array<std::weak_ptr<Node>> result;
		if (predicate(shared_from_this()))
		{
			result.push_back(shared_from_this());
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
	std::shared_ptr<TComponent> Node::getComponent()
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
	std::shared_ptr<TComponent> Node::getComponentOrNull()
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
	std::shared_ptr<TComponent> Node::getComponentRecursive()
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
	std::shared_ptr<TComponent> Node::getComponentRecursiveOrNull()
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
					RectF{ 0, 0, parentRect.w - (padding.left + padding.right), parentRect.h - (padding.top + padding.bottom) }, // 計測用に親サイズだけ渡す
					Vec2::Zero());
				sizes.push_back(measuredRect.size);
				margins.push_back(pBoxConstraint->margin);

				const double childW = measuredRect.w + pBoxConstraint->margin.left + pBoxConstraint->margin.right;
				const double childH = measuredRect.h + pBoxConstraint->margin.top + pBoxConstraint->margin.bottom;
				totalWidth += childW;
				maxHeight = Max(maxHeight, childH);
			}
			else
			{
				// BoxConstraint以外は計測不要
				sizes.push_back(SizeF::Zero());
				margins.push_back(LRTB::Zero());
			}
		}

		const double heightRemain = parentRect.h - (maxHeight + padding.top + padding.bottom);
		double baseY;
		if (heightRemain > 0.0)
		{
			switch (verticalAlign)
			{
			case VerticalAlign::Top:
				baseY = parentRect.y;
				break;
			case VerticalAlign::Middle:
				baseY = parentRect.y + (heightRemain / 2.0);
				break;
			case VerticalAlign::Bottom:
				baseY = parentRect.y + heightRemain;
				break;
			default:
				throw Error{ U"HorizontalLayout::execute: Invalid verticalAlign" };
			}
		}
		else
		{
			baseY = parentRect.y;
		}

		double currentX = parentRect.x;
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
				const double shiftY = (maxHeight - childTotalHeight);

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
				const double childY = baseY + margin.top + shiftY * verticalRatio;
				const RectF parentRectInsidePadding
				{
					parentRect.x + padding.left,
					parentRect.y + padding.top,
					parentRect.w - (padding.left + padding.right),
					parentRect.h - (padding.top + padding.bottom)
				};
				const RectF finalRect = pBoxConstraint->applyConstraint(parentRectInsidePadding, Vec2{ childX - parentRect.x, childY - parentRect.y });
				fnSetRect(child, finalRect);
				currentX += (childSize.x + margin.left + margin.right);
			}
			else if (const auto pAnchorConstraint = child->anchorConstraint())
			{
				// AnchorConstraintはオフセット無視
				const RectF finalRect = pAnchorConstraint->applyConstraint(parentRect, Vec2::Zero());
				fnSetRect(child, finalRect);
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
					RectF{ 0, 0, parentRect.w - (padding.left + padding.right), parentRect.h - (padding.top + padding.bottom) }, // 計測用に親サイズだけ渡す
					Vec2::Zero());
				sizes.push_back(measuredRect.size);
				margins.push_back(pBoxConstraint->margin);

				const double childW = measuredRect.w + pBoxConstraint->margin.left + pBoxConstraint->margin.right;
				const double childH = measuredRect.h + pBoxConstraint->margin.top + pBoxConstraint->margin.bottom;
				totalHeight += childH;
				maxWidth = Max(maxWidth, childW);
			}
			else
			{
				// BoxConstraint以外は計測不要
				sizes.push_back(SizeF::Zero());
				margins.push_back(LRTB::Zero());
			}
		}

		const double widthRemain = parentRect.w - (maxWidth + padding.left + padding.right);
		double baseX;
		if (widthRemain > 0.0)
		{
			switch (horizontalAlign)
			{
			case HorizontalAlign::Left:
				baseX = parentRect.x;
				break;
			case HorizontalAlign::Center:
				baseX = parentRect.x + (widthRemain / 2.0);
				break;
			case HorizontalAlign::Right:
				baseX = parentRect.x + widthRemain;
				break;
			default:
				throw Error{ U"VerticalLayout::execute: Invalid horizontalAlign" };
			}
		}
		else
		{
			baseX = parentRect.x;
		}

		double currentY = parentRect.y;
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
				const double shiftX = (maxWidth - childTotalWidth);

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
				const double childX = baseX + margin.left + shiftX * horizontalRatio;
				const RectF parentRectInsidePadding
				{
					parentRect.x + padding.left,
					parentRect.y + padding.top,
					parentRect.w - (padding.left + padding.right),
					parentRect.h - (padding.top + padding.bottom)
				};
				const RectF finalRect = pBoxConstraint->applyConstraint(parentRectInsidePadding, Vec2{ childX - parentRect.x, childY - parentRect.y });
				fnSetRect(child, finalRect);
				currentY += (childSize.y + margin.top + margin.bottom);
			}
			else if (const auto pAnchorConstraint = child->anchorConstraint())
			{
				// AnchorConstraintはオフセット無視
				const RectF finalRect = pAnchorConstraint->applyConstraint(parentRect, Vec2::Zero());
				fnSetRect(child, finalRect);
			}
		}
	}
}
