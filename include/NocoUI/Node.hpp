#pragma once
#include <Siv3D.hpp>
#include "YN.hpp"
#include "PropertyValue.hpp"
#include "Property.hpp"
#include "InheritChildrenStateFlags.hpp"
#include "FirstActiveLifecycleCompletedFlags.hpp"
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
		// ライブラリレベルでのマルチスレッド対応はしないが、atomicにはしておく
		static inline std::atomic<uint64> s_nextInstanceId = 1;
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
		ScrollBarType m_scrollBarType = ScrollBarType::Overlay;
		ClippingEnabledYN m_clippingEnabled = ClippingEnabledYN::No;
		PropertyNonInteractive<bool> m_activeSelf{ U"activeSelf", true };
		Property<int32> m_zOrderInSiblings{ U"zOrderInSiblings", 0 };

		/* NonSerialized */ std::weak_ptr<Canvas> m_canvas;
		/* NonSerialized */ std::weak_ptr<Node> m_parent;
		/* NonSerialized */ RectF m_regionRect{ 0.0, 0.0, 0.0, 0.0 };
		/* NonSerialized */ Quad m_transformedQuad{ Vec2::Zero(), Vec2::Zero(), Vec2::Zero(), Vec2::Zero() };
		/* NonSerialized */ Quad m_hitQuad{ Vec2::Zero(), Vec2::Zero(), Vec2::Zero(), Vec2::Zero() };
		/* NonSerialized */ Quad m_hitQuadWithPadding{ Vec2::Zero(), Vec2::Zero(), Vec2::Zero(), Vec2::Zero() };
		/* NonSerialized */ Vec2 m_scrollOffset{ 0.0, 0.0 };
		/* NonSerialized */ Smoothing<double> m_scrollBarAlpha{ 0.0 };
		/* NonSerialized */ MouseTracker m_mouseLTracker;
		/* NonSerialized */ MouseTracker m_mouseRTracker;
		/* NonSerialized */ ActiveYN m_activeInHierarchy = ActiveYN::No;
		/* NonSerialized */ ActiveYN m_activeInHierarchyForLifecycle = ActiveYN::No;
		/* NonSerialized */ PropertyNonInteractive<String> m_styleState{ U"styleState", U"" };
		/* NonSerialized */ Array<String> m_activeStyleStates;  // 現在のactiveStyleStates（親から受け取ったもの + 自身）
		/* NonSerialized */ InteractionState m_interactionStateInHierarchy = InteractionState::Default;
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
		/* NonSerialized */ Optional<int32> m_prevZOrderInSiblings; // 前回フレームのzOrderInSiblings
		/* NonSerialized */ mutable Array<std::shared_ptr<Node>> m_tempChildrenBuffer; // 子ノードの一時バッファ(update内で別のNodeのupdateが呼ばれる場合があるためthread_local staticにはできない。drawで呼ぶためmutableだが、drawはシングルスレッド前提なのでロック不要)
		/* NonSerialized */ mutable Array<std::shared_ptr<ComponentBase>> m_tempComponentsBuffer; // コンポーネントの一時バッファ(update内で別のNodeのupdateが呼ばれる場合があるためthread_local staticにはできない。drawで呼ぶためmutableだが、drawはシングルスレッド前提なのでロック不要)
		/* NonSerialized */ mutable FirstActiveLifecycleCompletedFlags m_firstActiveLifecycleCompletedFlags = FirstActiveLifecycleCompletedFlags::None; // activeInHierarchy=Yesで一度でも各種updateが呼ばれたかどうかのビットフラグ

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

		std::shared_ptr<ComponentBase> addComponentFromJSONImpl(const JSON& json, detail::WithInstanceIdYN withInstanceId);

		std::shared_ptr<ComponentBase> addComponentFromJSONImpl(const JSON& json, const ComponentFactory& factory, detail::WithInstanceIdYN withInstanceId);

		std::shared_ptr<ComponentBase> addComponentAtIndexFromJSONImpl(const JSON& json, size_t index, detail::WithInstanceIdYN withInstanceId);

		std::shared_ptr<ComponentBase> addComponentAtIndexFromJSONImpl(const JSON& json, size_t index, const ComponentFactory& factory, detail::WithInstanceIdYN withInstanceId);

		[[nodiscard]]
		InteractionState updateForCurrentInteractionState(const std::shared_ptr<Node>& hoveredNode, InteractableYN parentInteractable, IsScrollingYN isAncestorScrolling, const HashTable<String, ParamValue>& params);

		[[nodiscard]]
		InteractionState updateForCurrentInteractionStateRight(const std::shared_ptr<Node>& hoveredNode, InteractableYN parentInteractable, IsScrollingYN isAncestorScrolling, const HashTable<String, ParamValue>& params);

		void refreshActiveInHierarchy();

		void refreshPropertiesForInteractable(InteractableYN effectiveInteractable, SkipSmoothingYN skipSmoothing);

		void refreshChildrenPropertiesForInteractableRecursive(InteractableYN interactable, const HashTable<String, ParamValue>& params, SkipSmoothingYN skipSmoothing);

		void setCanvasRecursive(const std::weak_ptr<Canvas>& canvas);

		void clampScrollOffset();

		static void SortByZOrderInSiblings(Array<std::shared_ptr<Node>>& nodes, detail::UsePrevZOrderInSiblingsYN usePrevZOrderInSiblings = detail::UsePrevZOrderInSiblingsYN::No);

	public:
		/// @brief ノードを作成
		/// @param name ノード名
		/// @param region リージョン
		/// @param isHitTarget ヒットテストの対象にするかどうか
		/// @param inheritChildrenStateFlags 子ノードのインタラクションステートを受け継ぐかどうかのビットフラグ
		/// @return 生成されたノード
		[[nodiscard]]
		static std::shared_ptr<Node> Create(StringView name = U"Node", const RegionVariant& region = InlineRegion{}, IsHitTargetYN isHitTarget = IsHitTargetYN::Yes, InheritChildrenStateFlags inheritChildrenStateFlags = InheritChildrenStateFlags::None);

		/// @brief リージョン(ノードの位置やサイズ)を取得
		/// @return リージョン(InlineRegionまたはAnchorRegion)
		[[nodiscard]]
		const RegionVariant& region() const;

		/// @brief リージョン(ノードの位置やサイズ)を設定
		/// @param region リージョン(InlineRegionまたはAnchorRegion)
		std::shared_ptr<Node> setRegion(const RegionVariant& region);

		/// @brief InlineRegionを取得
		/// @return InlineRegionのポインタを返す。ノードに設定されたリージョンがInlineRegionでない場合はnullptrを返す
		[[nodiscard]]
		const InlineRegion* inlineRegion() const;

		/// @brief AnchorRegionを取得
		/// @return AnchorRegionのポインタを返す。ノードに設定されたリージョンがAnchorRegionでない場合はnullptrを返す
		[[nodiscard]]
		const AnchorRegion* anchorRegion() const;

		/// @brief トランスフォーム(ノードの変形)を取得
		/// @return トランスフォーム
		[[nodiscard]]
		Transform& transform();

		/// @brief トランスフォーム(ノードの変形)を取得
		/// @return トランスフォーム
		[[nodiscard]]
		const Transform& transform() const;

		/// @brief 子レイアウト(InlineRegionを持つ子ノードの並べ方のルール)を取得
		/// @return 子レイアウト
		[[nodiscard]]
		const LayoutVariant& childrenLayout() const override;

		/// @brief 子レイアウト(InlineRegionを持つ子ノードの並べ方のルール)を設定
		/// @param layout 子レイアウト
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> setChildrenLayout(const LayoutVariant& layout);

		/// @brief FlowLayoutを取得
		/// @return FlowLayoutのポインタを返す。ノードに設定された子レイアウトがFlowLayoutでない場合はnullptrを返す
		[[nodiscard]]
		const FlowLayout* childrenFlowLayout() const override;

		/// @brief HorizontalLayoutを取得
		/// @return HorizontalLayoutのポインタを返す。ノードに設定された子レイアウトがHorizontalLayoutでない場合はnullptrを返す
		[[nodiscard]]
		const HorizontalLayout* childrenHorizontalLayout() const override;

		/// @brief VerticalLayoutを取得
		/// @return VerticalLayoutのポインタを返す。ノードに設定された子レイアウトがVerticalLayoutでない場合はnullptrを返す
		[[nodiscard]]
		const VerticalLayout* childrenVerticalLayout() const override;

		/// @brief 子ノードの内容にフィットするサイズを取得
		/// @return サイズ(縦横のピクセル数)
		[[nodiscard]]
		SizeF getFittingSizeToChildren() const;

		/// @brief 子ノードの内容にフィットするようなInlineRegionをリージョンに設定
		/// @param fitTarget フィットさせる縦横の対象
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> setInlineRegionToFitToChildren(FitTarget fitTarget = FitTarget::Both);

		/// @brief 子レイアウトに設定されたpadding(内側の余白)を取得
		/// @return padding(内側の余白)
		[[nodiscard]]
		const LRTB& childrenLayoutPadding() const;

		/// @brief InlineRegionを持つかどうか
		/// @return ノードがInlineRegionを持つ場合はtrue、そうでなければfalseを返す
		[[nodiscard]]
		bool hasInlineRegion() const;

		/// @brief AnchorRegionを持つかどうか
		/// @return ノードがAnchorRegionを持つ場合はtrue、そうでなければfalseを返す
		[[nodiscard]]
		bool hasAnchorRegion() const;

		/// @brief ノードをJSON形式でシリアライズ
		/// @param withInstanceId 出力内容にインスタンスIDを含めるかどうか(NocoEditorの内部実装向けのため、通常は指定不要)
		/// @return JSON
		[[nodiscard]]
		JSON toJSON(detail::WithInstanceIdYN withInstanceId = detail::WithInstanceIdYN::No) const;

		/// @brief ノード配下にパラメータ参照のあるパラメータ名を列挙
		/// @param pParamRefs 挿入先のハッシュセットのポインタ
		void populateParamRefs(HashSet<String>* pParamRefs) const;

		/// @brief JSONからノードを作成
		/// @param json JSON
		/// @param withInstanceId 入力内容からインスタンスIDを読み込むどうか(NocoEditorの内部実装向けのため、通常は指定不要)
		/// @return 生成されたノード
		[[nodiscard]]
		static std::shared_ptr<Node> CreateFromJSON(const JSON& json, detail::WithInstanceIdYN withInstanceId = detail::WithInstanceIdYN::No);

		/// @brief JSONからノードを作成
		/// @param json JSON
		/// @param factory コンポーネントを生成するためのファクトリ
		/// @param withInstanceId 入力内容からインスタンスIDを読み込むどうか(NocoEditorの内部実装向けのため、通常は指定不要)
		/// @return 生成されたノード
		[[nodiscard]]
		static std::shared_ptr<Node> CreateFromJSON(const JSON& json, const ComponentFactory& factory, detail::WithInstanceIdYN withInstanceId = detail::WithInstanceIdYN::No);

		/// @brief 親ノードを取得
		/// @return 親ノードを返す。親を持たない場合(トップレベルノードやCanvas配下にないノード)はnullptrを返す
		[[nodiscard]]
		std::shared_ptr<Node> parentNode() const;

		/// @brief 親コンテナを取得
		/// @return 親コンテナ(ノードまたはCanvas)を返す。親を持たない場合(Canvas直下のノードやCanvas配下にないノード)はnullptrを返す
		[[nodiscard]]
		std::shared_ptr<INodeContainer> parentContainer() const;

		/// @brief トップレベルノード(Canvas直下のノード)かどうかを取得
		/// @return トップレベルノードの場合はtrue、そうでなければfalseを返す
		[[nodiscard]]
		bool isTopLevelNode() const;

		/// @brief 親ノードを辿り、自身に最も近いヒットテスト対象ノードを取得
		/// @return ヒットテスト対象ノードを返す。自身がヒットテスト対象の場合は自身を返す。ヒットテスト対象ノードが存在しない場合はnullptrを返す
		[[nodiscard]]
		std::shared_ptr<const Node> findHoverTargetParent() const;

		/// @brief 親ノードを辿り、自身に最も近いヒットテスト対象ノードを取得
		/// @return ヒットテスト対象ノードを返す。自身がヒットテスト対象の場合は自身を返す。ヒットテスト対象ノードが存在しない場合はnullptrを返す
		[[nodiscard]]
		std::shared_ptr<Node> findHoverTargetParent();

		/// @brief 指定したノードが自身の祖先ノードであるかどうかを取得
		/// @param node 判定対象のノード
		[[nodiscard]]
		bool isAncestorOf(const std::shared_ptr<Node>& node) const;

		/// @brief 親ノードを設定
		/// @param parent 新しい親ノード
		/// @return ノード自身(メソッドチェーンのため)
		/// @note ノードが既に親ノードを持っている場合は、既存の親ノードからは削除される
		std::shared_ptr<Node> setParent(const std::shared_ptr<Node>& parent);

		/// @brief 親コンテナ(ノードまたはCanvas)から削除
		/// @return 削除に成功した場合はtrue、元から親ノードを持っていなかった場合はfalseを返す
		bool removeFromParent();

		/// @brief 兄弟ノードの中でのインデックスを取得
		/// @return 兄弟ノードの中でのインデックス
		/// @throws Error 親コンテナを持たない場合は例外を送出する
		[[nodiscard]]
		size_t siblingIndex() const;

		/// @brief 兄弟ノードの中でのインデックスを取得
		/// @return 兄弟ノードの中でのインデックス。親コンテナを持たない場合はnoneを返す
		/// @note siblingIndexと異なり、例外を送出しない
		[[nodiscard]]
		Optional<size_t> siblingIndexOpt() const;

		/// @brief ノードが属するCanvasを取得
		/// @return ノードが属するCanvasを返す。Canvas配下にない場合はnullptrを返す
		[[nodiscard]]
		std::shared_ptr<Canvas> containedCanvas() const;

		/// @brief コンポーネントを追加
		/// @tparam TComponent 追加するコンポーネントの型
		/// @param component 追加するコンポーネント
		/// @return 追加されたコンポーネント
		template <typename TComponent>
		std::shared_ptr<TComponent> addComponent(const std::shared_ptr<TComponent>& component)
			requires std::derived_from<TComponent, ComponentBase>;

		/// @brief コンポーネントを指定したインデックスに追加
		/// @tparam TComponent 追加するコンポーネントの型
		/// @param component 追加するコンポーネント
		/// @param index 追加先のインデックス
		/// @return 追加されたコンポーネント
		/// @note indexが範囲外の場合は末尾に追加される
		template <typename TComponent>
		std::shared_ptr<TComponent> addComponentAtIndex(const std::shared_ptr<TComponent>& component, size_t index)
			requires std::derived_from<TComponent, ComponentBase>;

		/// @brief JSONからコンポーネントを追加
		/// @param json JSON
		/// @return 追加されたコンポーネント
		std::shared_ptr<ComponentBase> addComponentFromJSON(const JSON& json);
		
		/// @brief JSONからコンポーネントを追加
		/// @param json JSON
		/// @param factory コンポーネントを生成するためのファクトリ
		/// @return 追加されたコンポーネント
		std::shared_ptr<ComponentBase> addComponentFromJSON(const JSON& json, const ComponentFactory& factory);

		/// @brief JSONからコンポーネントを指定したインデックスに追加
		/// @param json JSON
		/// @param index 追加先のインデックス
		/// @return 追加されたコンポーネント
		/// @note indexが範囲外の場合は末尾に追加される
		std::shared_ptr<ComponentBase> addComponentAtIndexFromJSON(const JSON& json, size_t index);
		
		/// @brief JSONからコンポーネントを指定したインデックスに追加
		/// @param json JSON
		/// @param index 追加先のインデックス
		/// @param factory コンポーネントを生成するためのファクトリ
		/// @return 追加されたコンポーネント
		std::shared_ptr<ComponentBase> addComponentAtIndexFromJSON(const JSON& json, size_t index, const ComponentFactory& factory);

		/// @brief コンポーネントを削除
		/// @param component 削除するコンポーネント
		void removeComponent(const std::shared_ptr<ComponentBase>& component);

		/// @brief 条件に合致するコンポーネントを削除
		/// @tparam Predicate 関数オブジェクトの型
		/// @param predicate 削除条件の関数オブジェクト
		template <typename Predicate>
		void removeComponentsIf(Predicate predicate)
		{
			m_components.remove_if(std::move(predicate));
		}

		/// @brief 指定した型のコンポーネントをすべて削除
		/// @tparam TComponent 削除するコンポーネントの型
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		template <typename TComponent>
		void removeComponents(RecursiveYN recursive, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No);

		/// @brief すべてのコンポーネントを削除
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		void removeComponentsAll(RecursiveYN recursive, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No);

		/// @brief コンポーネントを構築して追加
		/// @tparam TComponent 追加するコンポーネントの型
		/// @tparam ...Args コンポーネントのコンストラクタに渡す引数の型
		/// @param ...args コンポーネントのコンストラクタに渡す引数
		/// @return 追加されたコンポーネント
		template <class TComponent, class... Args>
		std::shared_ptr<TComponent> emplaceComponent(Args&&... args)
			requires std::derived_from<TComponent, ComponentBase> && std::is_constructible_v<TComponent, Args...>;

		/// @brief コンポーネントを上に移動
		/// @param component 移動するコンポーネント
		/// @return 移動に成功した場合はtrue、既に一番上の場合やコンポーネントが見つからなかった場合はfalseを返す
		bool moveComponentUp(const std::shared_ptr<ComponentBase>& component);

		/// @brief コンポーネントを下に移動
		/// @param component 移動するコンポーネント
		/// @return 移動に成功した場合はtrue、既に一番下の場合やコンポーネントが見つからなかった場合はfalseを返す
		bool moveComponentDown(const std::shared_ptr<ComponentBase>& component);

		/// @brief 子ノードを追加
		/// @param child 追加する子ノード
		/// @return 追加されたノード
		const std::shared_ptr<Node>& addChild(std::shared_ptr<Node>&& child);

		/// @brief 子ノードを追加
		/// @param child 追加する子ノード
		/// @return 追加されたノード
		const std::shared_ptr<Node>& addChild(const std::shared_ptr<Node>& child) override;

		/// @brief 子ノードを構築して追加
		/// @param name ノード名
		/// @param region リージョン(ノードの位置やサイズ)
		/// @param isHitTarget ヒットテストの対象にするかどうか
		/// @param inheritChildrenStateFlags 子ノードのインタラクションステートを受け継ぐかどうかのビットフラグ
		/// @return 追加されたノード
		const std::shared_ptr<Node>& emplaceChild(StringView name = U"Node", const RegionVariant& region = InlineRegion{}, IsHitTargetYN isHitTarget = IsHitTargetYN::Yes, InheritChildrenStateFlags inheritChildrenStateFlags = InheritChildrenStateFlags::None) override;

		/// @brief JSONから子ノードを追加
		/// @param json JSON
		/// @return 追加されたノード
		const std::shared_ptr<Node>& addChildFromJSON(const JSON& json) override;
		
		/// @brief JSONから子ノードを追加
		/// @param json JSON
		/// @param factory コンポーネントを生成するためのファクトリ
		/// @return 追加されたノード
		const std::shared_ptr<Node>& addChildFromJSON(const JSON& json, const ComponentFactory& factory) override;

		/// @brief JSONから子ノードを指定したインデックスに追加
		/// @param json JSON
		/// @param index 追加先のインデックス
		/// @return 追加されたノード
		/// @note indexが範囲外の場合は末尾に追加される
		const std::shared_ptr<Node>& addChildAtIndexFromJSON(const JSON& json, size_t index);
		
		/// @brief JSONから子ノードを指定したインデックスに追加
		/// @param json JSON
		/// @param index 追加先のインデックス
		/// @param factory コンポーネントを生成するためのファクトリ
		/// @return 追加されたノード
		/// @note indexが範囲外の場合は末尾に追加される
		const std::shared_ptr<Node>& addChildAtIndexFromJSON(const JSON& json, size_t index, const ComponentFactory& factory);

		/// @brief 子ノードを指定したインデックスに追加
		/// @param child 追加する子ノード
		/// @param index 追加先のインデックス
		/// @return 追加されたノード
		/// @note indexが範囲外の場合は末尾に追加される
		const std::shared_ptr<Node>& addChildAtIndex(const std::shared_ptr<Node>& child, size_t index) override;

		/// @brief SubCanvasを持つノードを子として追加
		/// @param canvasPath SubCanvasファイルのパス
		/// @param params SubCanvasに設定するパラメータ
		/// @return 追加されたノード
		/// @note 作成されるノードはInlineRegionでSubCanvasのreferenceSizeと同じサイズになる
		const std::shared_ptr<Node>& addSubCanvasNodeAsChild(
			StringView canvasPath,
			std::initializer_list<std::pair<String, std::variant<bool, int32, double, const char32_t*, String, Color, ColorF, Vec2, LRTB>>> params = {});

		/// @brief 子ノードを削除
		/// @param child 削除する子ノード
		/// @throws Error 指定した子ノードが存在しなかった場合は例外を送出する
		void removeChild(const std::shared_ptr<Node>& child) override;

		/// @brief 子ノードとして持っているかどうかを取得
		/// @param child 判定対象の子ノード
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @return 子ノードとして持っている場合はtrue、そうでなければfalseを返す
		[[nodiscard]]
		bool containsChild(const std::shared_ptr<Node>& child, RecursiveYN recursive = RecursiveYN::No, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No) const override;

		/// @brief 条件に合致する子ノードをすべて取得
		/// @tparam Fty 関数オブジェクトの型
		/// @param predicate 条件の関数オブジェクト
		/// @return 条件に合致する子ノードの配列
		template <class Fty>
		[[nodiscard]]
		Array<std::shared_ptr<Node>> findAll(Fty&& predicate)
			requires std::invocable<Fty, const std::shared_ptr<Node>&>;

		/// @brief 条件に合致する子ノードをすべて取得
		/// @tparam Fty 関数オブジェクトの型
		/// @param predicate 条件の関数オブジェクト
		/// @param pResults 挿入先の配列のポインタ
		/// @param append 既存の内容に追加するかどうか(Noの場合は内容をクリアしてから追加する)
		template<class Fty>
		void findAll(Fty&& predicate, Array<std::shared_ptr<Node>>* pResults, AppendYN append = AppendYN::No)
			requires std::invocable<Fty, const std::shared_ptr<Node>&>;

		/// @brief 指定した型のコンポーネントを取得
		/// @tparam TComponent 取得するコンポーネントの型
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @return 最初に見つかったコンポーネントを返す。見つからなかった場合はnullptrを返す
		template <class TComponent>
		[[nodiscard]]
		std::shared_ptr<TComponent> getComponent(RecursiveYN recursive = RecursiveYN::No, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No) const;

		/// @brief 指定した型のコンポーネントをすべて取得
		/// @tparam TComponent 取得するコンポーネントの型
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @return 見つかったコンポーネントの配列。見つからなかった場合は空の配列を返す
		template <class TComponent>
		[[nodiscard]]
		Array<std::shared_ptr<TComponent>> getComponents(RecursiveYN recursive = RecursiveYN::No, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No) const;

		/// @brief 指定した型のコンポーネントをすべて取得
		/// @tparam TComponent 取得するコンポーネントの型
		/// @param pResults 挿入先の配列のポインタ
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @param append 既存の内容に追加するかどうか(Noの場合は内容をクリアしてから追加する)
		template <class TComponent>
		void getComponents(Array<std::shared_ptr<TComponent>>* pResults, RecursiveYN recursive = RecursiveYN::No, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No, AppendYN append = AppendYN::No) const;

		/// @brief 条件に合致するコンポーネントをすべて取得
		/// @tparam TComponent 取得するコンポーネントの型
		/// @tparam Fty 関数オブジェクトの型
		/// @param predicate 条件の関数オブジェクト
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @return 条件に合致するコンポーネントの配列
		template <class TComponent, class Fty>
		[[nodiscard]]
		Array<std::shared_ptr<TComponent>> findComponentsAll(Fty&& predicate, RecursiveYN recursive = RecursiveYN::Yes, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No)
			requires std::invocable<Fty, const std::shared_ptr<TComponent>&>;

		/// @brief 条件に合致するコンポーネントをすべて取得
		/// @tparam TComponent 取得するコンポーネントの型
		/// @tparam Fty 関数オブジェクトの型
		/// @param predicate 条件の関数オブジェクト
		/// @param pResults 挿入先の配列のポインタ
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @param append 既存の内容に追加するかどうか(Noの場合は内容をクリアしてから追加する)
		template <class TComponent, class Fty>
		void findComponentsAll(Fty&& predicate, Array<std::shared_ptr<TComponent>>* pResults, RecursiveYN recursive = RecursiveYN::Yes, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No, AppendYN append = AppendYN::No)
			requires std::invocable<Fty, const std::shared_ptr<TComponent>&>;

		/// @brief ノード名をもとに子ノードを検索
		/// @param name ノード名
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @return 最初に見つかったノードを返す。見つからなかった場合はnullptrを返す
		/// @note RecursiveYN::NoとIncludeSubCanvasYN::Yesを指定した場合、ノードが持つSubCanvasコンポーネントのトップレベルノードも子ノードと同じ扱いとして検索対象になる
		[[nodiscard]]
		std::shared_ptr<Node> findByName(StringView name, RecursiveYN recursive = RecursiveYN::Yes, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No) override;

		/// @brief 子レイアウトを明示的に更新
		/// @note 通常はレイアウト更新が必要な場合はフレームの最後に自動実行されるが、フレームの途中で即座に更新したい場合に使用する
		void refreshChildrenLayout();

		/// @brief 子ノードの内容が収まる矩形を取得
		/// @return 矩形
		[[nodiscard]]
		Optional<RectF> getChildrenContentRect() const;

		/// @brief 子ノードの内容が収まる矩形を、子レイアウトのpadding(内側の余白)を含めて取得
		/// @return 矩形
		[[nodiscard]]
		Optional<RectF> getChildrenContentRectWithPadding() const;

		/// @brief ノード配下でマウスカーソルがホバーしているノードを再帰的に取得
		/// @param onlyScrollable スクロール可能なノードのみを対象とするかどうか
		/// @param usePrevZOrderInSiblings 兄弟ノードのZオーダーに前回のフレームの値を使用するかどうか(ライブラリの内部実装用のため、通常は指定不要)
		/// @return ホバー中のノードを返す。ホバー中のノードが存在しない場合はnullptrを返す
		[[nodiscard]]
		std::shared_ptr<Node> hoveredNodeRecursive(OnlyScrollableYN onlyScrollable = OnlyScrollableYN::No, detail::UsePrevZOrderInSiblingsYN usePrevZOrderInSiblings = detail::UsePrevZOrderInSiblingsYN::No);

		/// @brief 指定した座標にあるヒットテスト対象ノードを取得
		/// @param point 座標
		/// @param onlyScrollable スクロール可能なノードのみを対象とするかどうか
		/// @param usePrevZOrderInSiblings 兄弟ノードのZオーダーに前回のフレームの値を使用するかどうか(ライブラリの内部実装用のため、通常は指定不要)
		/// @return ヒットしたノードを返す。ヒットしているノードが存在しない場合はnullptrを返す
		[[nodiscard]]
		std::shared_ptr<Node> hitTest(const Vec2& point, OnlyScrollableYN onlyScrollable = OnlyScrollableYN::No, detail::UsePrevZOrderInSiblingsYN usePrevZOrderInSiblings = detail::UsePrevZOrderInSiblingsYN::No);

		/// @brief ノード配下でマウスカーソルがホバーしているノードを再帰的に取得
		/// @param onlyScrollable スクロール可能なノードのみを対象とするかどうか
		/// @param usePrevZOrderInSiblings 兄弟ノードのZオーダーに前回のフレームの値を使用するかどうか(ライブラリの内部実装用のため、通常は指定不要)
		/// @return ホバー中のノードを返す。ホバー中のノードが存在しない場合はnullptrを返す
		[[nodiscard]]
		std::shared_ptr<const Node> hoveredNodeRecursive(OnlyScrollableYN onlyScrollable = OnlyScrollableYN::No, detail::UsePrevZOrderInSiblingsYN usePrevZOrderInSiblings = detail::UsePrevZOrderInSiblingsYN::No) const;

		/// @brief 指定した座標にヒットしているヒットテスト対象ノードを取得
		/// @param point 座標
		/// @param onlyScrollable スクロール可能なノードのみを対象とするかどうか
		/// @param usePrevZOrderInSiblings 兄弟ノードのZオーダーに前回のフレームの値を使用するかどうか(ライブラリの内部実装用のため、通常は指定不要)
		/// @return ヒットしたノードを返す。ヒットしているノードが存在しない場合はnullptrを返す
		[[nodiscard]]
		std::shared_ptr<const Node> hitTest(const Vec2& point, OnlyScrollableYN onlyScrollable = OnlyScrollableYN::No, detail::UsePrevZOrderInSiblingsYN usePrevZOrderInSiblings = detail::UsePrevZOrderInSiblingsYN::No) const;

		/// @brief 毎フレームのノードパラメータ更新(内部実装用のため、通常は使用しない)
		/// @param params ノードパラメータのハッシュテーブル
		void updateNodeParams(const HashTable<String, ParamValue>& params);

		/// @brief 毎フレームの各種ステート更新(内部実装用のため、通常は使用しない)
		/// @param updateInteractionState インタラクションステートを更新するかどうか
		/// @param hoveredNode Canvasでホバー中のノード
		/// @param deltaTime 前回フレームからの時間(秒)
		/// @param parentInteractable 親ノードがインタラクション可能かどうか
		/// @param parentInteractionState 親ノードのインタラクションステート(マウス左ボタン)
		/// @param parentInteractionStateRight 親ノードのインタラクションステート(マウス右ボタン)
		/// @param isAncestorScrolling 祖先ノードがスクロール中かどうか
		/// @param params ノードパラメータのハッシュテーブル
		/// @param parentActiveStyleStates 祖先ノードで現在有効なスタイルステート名の配列(近いものほど後ろに挿入される)
		void updateNodeStates(detail::UpdateInteractionStateYN updateInteractionState, const std::shared_ptr<Node>& hoveredNode, double deltaTime, InteractableYN parentInteractable, InteractionState parentInteractionState, InteractionState parentInteractionStateRight, IsScrollingYN isAncestorScrolling, const HashTable<String, ParamValue>& params, const Array<String>& parentActiveStyleStates);

		/// @brief 毎フレームのキー入力更新(内部実装用のため、通常は使用しない)
		void updateKeyInput();

		/// @brief 毎フレームの更新(内部実装用のため、通常は使用しない)
		/// @param scrollableHoveredNode Canvasでホバー中のスクロール可能ノード
		/// @param deltaTime 前回フレームからの時間(秒)
		/// @param parentTransformMat 親から受け継いだトランスフォームの行列
		/// @param parentHitTestMat 親から受け継いだヒットテスト用のトランスフォームの行列
		/// @param params ノードパラメータのハッシュテーブル
		void update(const std::shared_ptr<Node>& scrollableHoveredNode, double deltaTime, const Mat3x2& parentTransformMat, const Mat3x2& parentHitTestMat, const HashTable<String, ParamValue>& params);

		/// @brief 毎フレームのupdate後の更新(内部実装用のため、通常は使用しない)
		void lateUpdate();

		/// @brief 毎フレームのlateUpdate後の更新(内部実装用のため、通常は使用しない)
		/// @param deltaTime 前回フレームからの時間(秒)
		/// @param parentTransformMat 親から受け継いだトランスフォームの行列
		/// @param parentHitTestMat 親から受け継いだヒットテスト用のトランスフォームの行列
		/// @param params ノードパラメータのハッシュテーブル
		void postLateUpdate(double deltaTime, const Mat3x2& parentTransformMat, const Mat3x2& parentHitTestMat, const HashTable<String, ParamValue>& params);

		/// @brief 毎フレームのトランスフォーム行列更新(内部実装用のため、通常は使用しない)
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param parentTransformMat 親から受け継いだトランスフォームの行列
		/// @param parentHitTestMat 親から受け継いだヒットテスト用のトランスフォームの行列
		/// @param params ノードパラメータのハッシュテーブル
		void refreshTransformMat(RecursiveYN recursive, const Mat3x2& parentTransformMat, const Mat3x2& parentHitTestMat, const HashTable<String, ParamValue>& params);

		/// @brief スクロール
		/// @param offsetDelta スクロールオフセットの変化量
		void scroll(const Vec2& offsetDelta);

		/// @brief スクロールオフセットを取得
		/// @return スクロールオフセット
		[[nodiscard]]
		Vec2 scrollOffset() const;

		/// @brief スクロールオフセットをリセット
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		void resetScrollOffset(RecursiveYN recursive = RecursiveYN::No, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No);

		/// @brief スクロール可能な範囲を取得
		/// @return スクロール可能な範囲の左上と右下の座標のペア
		[[nodiscard]]
		std::pair<Vec2, Vec2> validScrollRange() const;

		/// @brief パラメータ参照名を置換
		/// @param oldName 置換対象のパラメータ参照名
		/// @param newName 新しいパラメータ参照名
		/// @param recursive 子孫ノードも対象とするかどうか
		void replaceParamRefs(const String& oldName, const String& newName, RecursiveYN recursive = RecursiveYN::Yes);

		/// @brief 現在のフレームのプロパティ値の上書き状態をクリア
		/// @param recursive 子孫ノードも対象とするかどうか
		void clearCurrentFrameOverride(RecursiveYN recursive = RecursiveYN::Yes);

		/// @brief ノードを描画(内部実装用のため、通常は使用しない)
		void draw() const;

		/// @brief ノードをクリックしたことにする
		/// @note この関数を呼ぶと、次フレームでisClickedおよびisClickRequestedがtrueを返すようになる。ノードがヒットテスト対象でない場合も有効
		void requestClick();

		/// @brief ノードを右クリックしたことにする
		/// @note この関数を呼ぶと、次フレームでisRightClickedおよびisRightClickRequestedがtrueを返すようになる。ノードがヒットテスト対象でない場合も有効
		void requestRightClick();

		/// @brief ノード名を取得
		/// @return ノード名
		[[nodiscard]]
		const String& name() const;

		/// @brief ノード名を設定
		/// @param name ノード名
		std::shared_ptr<Node> setName(StringView name);

		/// @brief 回転成分を除いた変換後の矩形を取得
		/// @return 矩形
		[[nodiscard]]
		RectF unrotatedTransformedRect() const;

		/// @brief トランスフォームの行列から回転成分を抽出
		/// @return 回転角(ラジアン)
		[[nodiscard]]
		double extractRotationFromTransformMat() const;

		/// @brief ノードのトランスフォームからスケールの値を取得
		/// @return スケール
		[[nodiscard]]
		Vec2 transformScaleInHierarchy() const;

		/// @brief ヒットテスト用のトランスフォームの行列を取得
		/// @return 行列
		[[nodiscard]]
		const Mat3x2& hitTestMatInHierarchy() const;

		/// @brief 座標をもとにヒットテスト用のトランスフォームの逆変換を行う
		/// @param point 座標
		/// @return 逆変換後の座標
		[[nodiscard]]
		Vec2 inverseTransformHitTestPoint(const Vec2& point) const;

		/// @brief トランスフォームのpivot位置を取得
		/// @return 位置
		[[nodiscard]]
		Vec2 transformPivotPos() const;

		/// @brief トランスフォーム適用後の四角形を取得
		/// @return 四角形(Quad)
		[[nodiscard]]
		const Quad& transformedQuad() const;

		/// @brief ヒットテスト用の四角形を取得
		/// @param withPadding ヒットテストの余白を含めるかどうか
		/// @return 四角形(Quad)
		[[nodiscard]]
		Quad hitQuad(WithPaddingYN withPadding = WithPaddingYN::No) const;

		/// @brief リージョン矩形(トランスフォーム適用前の矩形)を取得
		/// @return 矩形
		[[nodiscard]]
		const RectF& regionRect() const;

		/// @brief リージョン矩形(トランスフォーム適用前の矩形)を、マージンを含めて取得
		/// @return 矩形
		[[nodiscard]]
		RectF regionRectWithMargin() const;

		/// @brief トランスフォームの行列(祖先ノードの変換も再帰的に適用したもの)を取得
		/// @return 行列
		[[nodiscard]]
		const Mat3x2& transformMatInHierarchy() const;

		/// @brief 子ノードを持っているかどうかを取得
		/// @return 子ノードを持っている場合はtrue、そうでなければfalseを返す
		[[nodiscard]]
		bool hasChildren() const;

		/// @brief コンポーネントの配列を取得
		/// @return コンポーネントの配列
		[[nodiscard]]
		const Array<std::shared_ptr<ComponentBase>>& components() const;

		/// @brief インタラクション可能かどうかを取得
		/// @return インタラクション可能な場合はtrue、そうでなければfalseを返す
		[[nodiscard]]
		bool interactable() const;

		/// @brief インタラクション可能かどうかを設定
		/// @param interactable インタラクション可能かどうか
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> setInteractable(InteractableYN interactable);

		/// @brief インタラクション可能かどうかを設定
		/// @param interactable インタラクション可能かどうか
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> setInteractable(bool interactable);
		
		/// @brief インタラクション可能かどうかのパラメータ参照名を取得
		/// @return パラメータ参照名
		[[nodiscard]]
		const String& interactableParamRef() const { return m_interactable.paramRef(); }
		
		/// @brief インタラクション可能かどうかのパラメータ参照名を設定
		/// @param paramRef パラメータ参照名
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> setInteractableParamRef(const String& paramRef)
		{
			m_interactable.setParamRef(paramRef);
			return shared_from_this();
		}
		
		/// @brief インタラクション可能かどうかのプロパティの参照を取得
		/// @return プロパティの参照
		[[nodiscard]]
		PropertyNonInteractive<bool>& interactableProperty() { return m_interactable; }
		
		/// @brief インタラクション可能かどうかのプロパティの参照を取得
		/// @return プロパティの参照
		[[nodiscard]]
		const PropertyNonInteractive<bool>& interactableProperty() const { return m_interactable; }

		/// @brief インタラクション可能かどうかを祖先ノードも加味して取得
		/// @return インタラクション可能な場合はtrue、そうでなければfalseを返す
		[[nodiscard]]
		bool interactableInHierarchy() const;

		/// @brief ノードがアクティブ(表示状態)かどうかを取得
		/// @return アクティブな場合はtrue、そうでなければfalseを返す
		[[nodiscard]]
		bool activeSelf() const;

		/// @brief ノードがアクティブ(表示状態)かどうかを設定
		/// @param activeSelf ノードがアクティブかどうか
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> setActive(ActiveYN activeSelf);

		/// @brief ノードがアクティブ(表示状態)かどうかを設定
		/// @param activeSelf ノードがアクティブかどうか
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> setActive(bool activeSelf);
		
		/// @brief ノードがアクティブ(表示状態)かどうかのパラメータ参照名を取得
		/// @return パラメータ参照名
		[[nodiscard]]
		const String& activeSelfParamRef() const { return m_activeSelf.paramRef(); }
		
		/// @brief ノードがアクティブ(表示状態)かどうかのパラメータ参照名を設定
		/// @param paramRef パラメータ参照名
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> setActiveSelfParamRef(const String& paramRef)
		{
			m_activeSelf.setParamRef(paramRef);
			return shared_from_this();
		}
		
		/// @brief ノードがアクティブ(表示状態)かどうかのプロパティの参照を取得
		/// @return プロパティの参照
		[[nodiscard]]
		PropertyNonInteractive<bool>& activeSelfProperty() { return m_activeSelf; }
		
		/// @brief ノードがアクティブ(表示状態)かどうかのプロパティの参照を取得
		/// @return プロパティの参照
		[[nodiscard]]
		const PropertyNonInteractive<bool>& activeSelfProperty() const { return m_activeSelf; }

		/// @brief ノードがアクティブ(表示状態)かどうかを祖先ノードも加味して取得
		/// @return アクティブな場合はtrue、そうでなければfalseを返す
		[[nodiscard]]
		bool activeInHierarchy() const;

		/// @brief ノードがヒットテスト対象かどうかを取得
		/// @return ヒットテスト対象の場合はtrue、そうでなければfalseを返す
		[[nodiscard]]
		bool isHitTarget() const;

		/// @brief ノードがヒットテスト対象かどうかを設定
		/// @param isHitTarget ヒットテスト対象かどうか
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> setIsHitTarget(IsHitTargetYN isHitTarget);

		/// @brief ノードがヒットテスト対象かどうかを設定
		/// @param isHitTarget ヒットテスト対象かどうか
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> setIsHitTarget(bool isHitTarget);

		/// @brief ノードのヒットテストの余白を取得
		/// @return ヒットテストの余白
		[[nodiscard]]
		const LRTB& hitPadding() const;

		/// @brief ノードのヒットテストの余白を設定
		/// @param padding ヒットテストの余白
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> setHitPadding(const LRTB& padding);

		/// @brief 子ノードのインタラクションステートを受け継ぐかどうかのビットフラグを取得
		/// @return 受け継ぐかどうかのビットフラグ
		[[nodiscard]]
		InheritChildrenStateFlags inheritChildrenStateFlags() const;

		/// @brief 子ノードのインタラクションステートを受け継ぐかどうかのビットフラグを設定
		/// @param flags 受け継ぐかどうかのビットフラグ
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> setInheritChildrenStateFlags(InheritChildrenStateFlags flags);

		/// @brief 子ノードのホバー状態を受け継ぐかどうかを取得
		/// @return 受け継ぐ場合はtrue、そうでなければfalseを返す
		[[nodiscard]]
		bool inheritChildrenHover() const;

		/// @brief 子ノードのホバー状態を受け継ぐかどうかを設定
		/// @param value 受け継ぐかどうか
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> setInheritChildrenHover(bool value);

		/// @brief 子ノードの押下状態を受け継ぐかどうかを取得
		/// @return 受け継ぐ場合はtrue、そうでなければfalseを返す
		[[nodiscard]]
		bool inheritChildrenPress() const;

		/// @brief 子ノードの押下状態を受け継ぐかどうかを設定
		/// @param value 受け継ぐかどうか
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> setInheritChildrenPress(bool value);

		/// @brief スクロール可能な軸のビットフラグを取得
		/// @return スクロール可能な軸のビットフラグ
		[[nodiscard]]
		ScrollableAxisFlags scrollableAxisFlags() const;

		/// @brief スクロール可能な軸のビットフラグを設定
		/// @param flags スクロール可能な軸のビットフラグ
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> setScrollableAxisFlags(ScrollableAxisFlags flags);

		/// @brief 水平スクロール可能かどうかを取得
		/// @return 水平スクロール可能な場合はtrue、そうでなければfalseを返す
		[[nodiscard]]
		bool horizontalScrollable() const;

		/// @brief 水平スクロール可能かどうかを設定
		/// @param scrollable 水平スクロール可能かどうか
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> setHorizontalScrollable(bool scrollable);

		/// @brief 垂直スクロール可能かどうかを取得
		/// @return 垂直スクロール可能な場合はtrue、そうでなければfalseを返す
		[[nodiscard]]
		bool verticalScrollable() const;

		/// @brief 垂直スクロール可能かどうかを設定
		/// @param scrollable 垂直スクロール可能かどうか
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> setVerticalScrollable(bool scrollable);
		
		/// @brief 有効なスクロールの種類のビットフラグを取得
		/// @return スクロールの種類のビットフラグ
		[[nodiscard]]
		ScrollMethodFlags scrollMethodFlags() const;
		
		/// @brief 有効なスクロールの種類のビットフラグを設定
		/// @param flags スクロールの種類のビットフラグ
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> setScrollMethodFlags(ScrollMethodFlags flags);
		
		/// @brief ホイールスクロールが有効かどうかを取得
		/// @return ホイールスクロールが有効な場合はtrue、そうでなければfalseを返す
		[[nodiscard]]
		bool wheelScrollEnabled() const;
		
		/// @brief ホイールスクロールが有効かどうかを設定
		/// @param enabled ホイールスクロールが有効かどうか
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> setWheelScrollEnabled(bool enabled);
		
		/// @brief ドラッグスクロールが有効かどうかを取得
		/// @return ドラッグスクロールが有効な場合はtrue、そうでなければfalseを返す
		[[nodiscard]]
		bool dragScrollEnabled() const;
		
		/// @brief ドラッグスクロールが有効かどうかを設定
		/// @param enabled ドラッグスクロールが有効かどうか
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> setDragScrollEnabled(bool enabled);
		
		/// @brief 慣性スクロールの減速率を取得
		/// @return 減速率
		[[nodiscard]]
		double decelerationRate() const;
		
		/// @brief 慣性スクロールの減速率を設定
		/// @param rate 減速率
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> setDecelerationRate(double rate);

		/// @brief ラバーバンドスクロール(範囲外にスクロールでき、離すと範囲内に戻る)が有効かどうかを取得
		/// @return ラバーバンドスクロールが有効な場合はtrue、そうでなければfalseを返す
		[[nodiscard]]
		bool rubberBandScrollEnabled() const;

		/// @brief ラバーバンドスクロール(範囲外にスクロールでき、離すと範囲内に戻る)が有効かどうかを設定
		/// @param rubberBandScrollEnabled ラバーバンドスクロールが有効かどうか
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> setRubberBandScrollEnabled(RubberBandScrollEnabledYN rubberBandScrollEnabled);

		/// @brief ラバーバンドスクロール(範囲外にスクロールでき、離すと範囲内に戻る)が有効かどうかを設定
		/// @param rubberBandScrollEnabled ラバーバンドスクロールが有効かどうか
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> setRubberBandScrollEnabled(bool rubberBandScrollEnabled);

		/// @brief スクロールバー表示の種類を取得
		/// @return スクロールバー表示の種類
		[[nodiscard]]
		ScrollBarType scrollBarType() const;

		/// @brief スクロールバー表示の種類を設定
		/// @param scrollBarType スクロールバー表示の種類
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> setScrollBarType(ScrollBarType scrollBarType);

		/// @brief ドラッグスクロールを離す
		void preventDragScroll();

		/// @brief クリッピングが有効かどうかを取得
		/// @return クリッピングが有効な場合はtrue、そうでなければfalseを返す
		[[nodiscard]]
		bool clippingEnabled() const;

		/// @brief クリッピングが有効かどうかを設定
		/// @param clippingEnabled クリッピングが有効かどうか
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> setClippingEnabled(ClippingEnabledYN clippingEnabled);

		/// @brief クリッピングが有効かどうかを設定
		/// @param clippingEnabled クリッピングが有効かどうか
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> setClippingEnabled(bool clippingEnabled);

		/// @brief ノードのインタラクションステートを取得
		/// @return インタラクションステート
		[[nodiscard]]
		InteractionState interactionStateSelf() const;

		/// @brief ノードのインタラクションステートを祖先ノードのインタラクション可能かどうかを加味して取得
		/// @return インタラクションステート
		[[nodiscard]]
		InteractionState interactionStateInHierarchy() const;

		/// @brief ノードのスタイルステート名を取得
		/// @return スタイルステート名
		[[nodiscard]]
		const String& styleState() const { return m_styleState.value(); }

		/// @brief ノードのスタイルステート名を設定
		/// @param state スタイルステート名
		std::shared_ptr<Node> setStyleState(const String& state)
		{
			m_styleState.setValue(state);
			return shared_from_this();
		}

		/// @brief ノードのスタイルステート名をクリア
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> clearStyleState()
		{
			m_styleState.setValue(U"");
			return shared_from_this();
		}
		
		/// @brief スタイルステート名のパラメータ参照名を取得
		/// @return パラメータ参照名
		[[nodiscard]]
		const String& styleStateParamRef() const { return m_styleState.paramRef(); }
		
		/// @brief スタイルステート名のパラメータ参照名を設定
		/// @param paramRef パラメータ参照名
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> setStyleStateParamRef(const String& paramRef)
		{
			m_styleState.setParamRef(paramRef);
			return shared_from_this();
		}
		
		/// @brief スタイルステート名のプロパティの参照を取得
		/// @return プロパティの参照
		[[nodiscard]]
		PropertyNonInteractive<String>& styleStateProperty() { return m_styleState; }
		
		/// @brief スタイルステート名のプロパティの参照を取得
		/// @return プロパティの参照
		[[nodiscard]]
		const PropertyNonInteractive<String>& styleStateProperty() const { return m_styleState; }

		/// @brief ノードがホバー中かどうかを取得
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includingDisabled 無効化されているノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @return ホバー中の場合はtrue、そうでなければfalseを返す
		[[nodiscard]]
		bool isHovered(RecursiveYN recursive = RecursiveYN::No, IncludingDisabledYN includingDisabled = IncludingDisabledYN::No, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No) const;

		/// @brief ノードが押下中かどうかを取得
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includingDisabled 無効化されているノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @return 押下中の場合はtrue、そうでなければfalseを返す
		[[nodiscard]]
		bool isPressed(RecursiveYN recursive = RecursiveYN::No, IncludingDisabledYN includingDisabled = IncludingDisabledYN::No, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No) const;

		/// @brief ノードがホバー中かつ押下中かどうかを取得
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includingDisabled 無効化されているノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @return ホバー中かつ押下中の場合はtrue、そうでなければfalseを返す
		[[nodiscard]]
		bool isPressedHover(RecursiveYN recursive = RecursiveYN::No, IncludingDisabledYN includingDisabled = IncludingDisabledYN::No, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No) const;

		/// @brief ノード上でマウスの左ボタンを押した瞬間かどうかを取得
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includingDisabled 無効化されているノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @return ノード上でマウスの左ボタンを押した瞬間であればtrue、そうでなければfalseを返す
		[[nodiscard]]
		bool isMouseDown(RecursiveYN recursive = RecursiveYN::No, IncludingDisabledYN includingDisabled = IncludingDisabledYN::No, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No) const;

		/// @brief ノードがクリックされたかどうかを取得
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includingDisabled 無効化されているノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @return クリックされた場合はtrue、そうでなければfalseを返す
		/// @note 前回フレームでrequestClickが呼ばれた場合もtrueを返す
		[[nodiscard]]
		bool isClicked(RecursiveYN recursive = RecursiveYN::No, IncludingDisabledYN includingDisabled = IncludingDisabledYN::No, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No) const;

		/// @brief 前回フレームでrequestClickが呼ばれたかどうかを取得
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includingDisabled 無効化されているノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @return 前回フレームでrequestClickが呼ばれた場合はtrue、そうでなければfalseを返す
		[[nodiscard]]
		bool isClickRequested(RecursiveYN recursive = RecursiveYN::No, IncludingDisabledYN includingDisabled = IncludingDisabledYN::No, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No) const;

		/// @brief ノードが右クリック押下中かどうかを取得
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includingDisabled 無効化されているノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @return 右クリック押下中の場合はtrue、そうでなければfalseを返す
		[[nodiscard]]
		bool isRightPressed(RecursiveYN recursive = RecursiveYN::No, IncludingDisabledYN includingDisabled = IncludingDisabledYN::No, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No) const;

		/// @brief ノードが右クリックホバー中かどうかを取得
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includingDisabled 無効化されているノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @return ホバー中かつ右クリック押下中の場合はtrue、そうでなければfalseを返す
		[[nodiscard]]
		bool isRightPressedHover(RecursiveYN recursive = RecursiveYN::No, IncludingDisabledYN includingDisabled = IncludingDisabledYN::No, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No) const;

		/// @brief ノード上でマウスの右ボタンを押した瞬間かどうかを取得
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includingDisabled 無効化されているノードも対象とする
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @return ノード上でマウスの右ボタンを押した瞬間であればtrue、そうでなければfalseを返す
		[[nodiscard]]
		bool isRightMouseDown(RecursiveYN recursive = RecursiveYN::No, IncludingDisabledYN includingDisabled = IncludingDisabledYN::No, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No) const;

		/// @brief ノードが右クリックされたかどうかを取得
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includingDisabled 無効化されているノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @return 右クリックされた場合はtrue、そうでなければfalseを返す
		[[nodiscard]]
		bool isRightClicked(RecursiveYN recursive = RecursiveYN::No, IncludingDisabledYN includingDisabled = IncludingDisabledYN::No, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No) const;

		/// @brief 前回フレームでrequestRightClickが呼ばれたかどうかを取得
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includingDisabled 無効化されているノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @return 前回フレームでrequestRightClickが呼ばれた場合はtrue、そうでなければfalseを返す
		[[nodiscard]]
		bool isRightClickRequested(RecursiveYN recursive = RecursiveYN::No, IncludingDisabledYN includingDisabled = IncludingDisabledYN::No, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No) const;

		/// @brief すべての子ノードを削除する
		void removeChildrenAll() override;

		/// @brief 指定した子ノード同士を入れ替える
		/// @param child1 子ノード1
		/// @param child2 子ノード2
		void swapChildren(const std::shared_ptr<Node>& child1, const std::shared_ptr<Node>& child2);

		/// @brief 指定したインデックスの子ノード同士を入れ替える
		/// @param index1 子ノード1のインデックス
		/// @param index2 子ノード2のインデックス
		void swapChildren(size_t index1, size_t index2) override;

		/// @brief 指定した子ノードのインデックスを取得する
		/// @param child 子ノード
		/// @return 子ノードのインデックス
		/// @throws Error 見つからない場合は例外を送出する
		[[nodiscard]]
		size_t indexOfChild(const std::shared_ptr<Node>& child) const;

		/// @brief 指定した子ノードのインデックスを取得する(Optional版)
		/// @param child 子ノード
		/// @return 子ノードのインデックス。見つからない場合はnoneを返す
		[[nodiscard]]
		Optional<size_t> indexOfChildOpt(const std::shared_ptr<Node>& child) const override;

		/// @brief ノードを複製する
		/// @return 複製されたノード
		[[nodiscard]]
		std::shared_ptr<Node> clone() const;

		/// @brief updateKeyInput関数を毎フレーム実行するコンポーネントを追加
		/// @param keyInputUpdater updateKeyInput関数の関数オブジェクト
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> addKeyInputUpdater(std::function<void(const std::shared_ptr<Node>&)> keyInputUpdater);

		/// @brief update関数を毎フレーム実行するコンポーネントを追加
		/// @param updater update関数の関数オブジェクト
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> addUpdater(std::function<void(const std::shared_ptr<Node>&)> updater);

		/// @brief draw関数を毎フレーム実行するコンポーネントを追加
		/// @param drawer draw関数の関数オブジェクト
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> addDrawer(std::function<void(const Node&)> drawer);

		/// @brief クリック時に実行する関数を追加
		/// @param onClick クリック時に実行する関数オブジェクト
		/// @return ノード自身(メソッドチェーンのため)
		/// @note 関数は毎フレーム判定するUpdaterComponentとして追加される
		std::shared_ptr<Node> addOnClick(std::function<void(const std::shared_ptr<Node>&)> onClick);

		/// @brief クリック時に実行する関数を追加
		/// @param onClick クリック時に実行する関数オブジェクト
		/// @return ノード自身(メソッドチェーンのため)
		/// @note 関数は毎フレーム判定するUpdaterComponentとして追加される
		std::shared_ptr<Node> addOnClick(std::function<void()> onClick);

		/// @brief 右クリック時に実行する関数を追加
		/// @param onRightClick 右クリック時に実行する関数オブジェクト
		/// @return ノード自身(メソッドチェーンのため)
		/// @note 関数は毎フレーム判定するUpdaterComponentとして追加される
		std::shared_ptr<Node> addOnRightClick(std::function<void(const std::shared_ptr<Node>&)> onRightClick);

		/// @brief 右クリック時に実行する関数を追加
		/// @param onRightClick 右クリック時に実行する関数オブジェクト
		/// @return ノード自身(メソッドチェーンのため)
		/// @note 関数は毎フレーム判定するUpdaterComponentとして追加される
		std::shared_ptr<Node> addOnRightClick(std::function<void()> onRightClick);

		/// @brief クリック扱いとするホットキー入力を追加
		/// @param input 入力
		/// @param enabledWhileTextEditing テキスト編集中も有効にするかどうか
		/// @param clearInput ホットキー入力後にclearInputを呼び出すかどうか
		/// @return ノード自身(メソッドチェーンのため)
		/// @note クリックはrequestClickとして扱われる
		std::shared_ptr<Node> addClickHotKey(const Input& input, EnabledWhileTextEditingYN enabledWhileTextEditing, ClearInputYN clearInput = ClearInputYN::Yes);

		/// @brief クリック扱いとするホットキー入力を追加
		/// @param input 入力
		/// @param ctrl Ctrlキー(macOSの場合はCommandキー)の押下有無
		/// @param alt Altキーの押下有無
		/// @param shift Shiftキーの押下有無
		/// @param enabledWhileTextEditing テキスト編集中も有効にするかどうか
		/// @param clearInput ホットキー入力後にclearInputを呼び出すかどうか
		/// @return ノード自身(メソッドチェーンのため)
		/// @note クリックはrequestClickとして扱われる
		std::shared_ptr<Node> addClickHotKey(const Input& input, CtrlYN ctrl = CtrlYN::No, AltYN alt = AltYN::No, ShiftYN shift = ShiftYN::No, EnabledWhileTextEditingYN enabledWhileTextEditing = EnabledWhileTextEditingYN::No, ClearInputYN clearInput = ClearInputYN::Yes);

		/// @brief 右クリック扱いとするホットキー入力を追加
		/// @param input 入力
		/// @param enabledWhileTextEditing テキスト編集中も有効にするかどうか
		/// @param clearInput ホットキー入力後にclearInputを呼び出すかどうか
		/// @return ノード自身(メソッドチェーンのため)
		/// @note 右クリックはrequestRightClickとして扱われる
		std::shared_ptr<Node> addRightClickHotKey(const Input& input, EnabledWhileTextEditingYN enabledWhileTextEditing, ClearInputYN clearInput = ClearInputYN::Yes);

		/// @brief 右クリック扱いとするホットキー入力を追加
		/// @param input 入力
		/// @param ctrl Ctrlキー(macOSの場合はCommandキー)の押下有無
		/// @param alt Altキーの押下有無
		/// @param shift Shiftキーの押下有無
		/// @param enabledWhileTextEditing テキスト編集中も有効にするかどうか
		/// @param clearInput ホットキー入力後にclearInputを呼び出すかどうか
		/// @return ノード自身(メソッドチェーンのため)
		/// @note 右クリックはrequestRightClickとして扱われる
		std::shared_ptr<Node> addRightClickHotKey(const Input& input, CtrlYN ctrl = CtrlYN::No, AltYN alt = AltYN::No, ShiftYN shift = ShiftYN::No, EnabledWhileTextEditingYN enabledWhileTextEditing = EnabledWhileTextEditingYN::No, ClearInputYN clearInput = ClearInputYN::Yes);

		/// @brief ノードが属するCanvasのレイアウトを即座に更新する
		/// @param onlyIfDirty レイアウト更新が必要な場合のみ更新するかどうか(Noを指定すると強制的に更新する。通常はYesのままで問題ない)
		void refreshContainedCanvasLayoutImmediately(OnlyIfDirtyYN onlyIfDirty = OnlyIfDirtyYN::Yes);
		
		/// @brief ノードが属するCanvasにレイアウト更新が必要であることをマークする
		/// @note マークされたノードは、フレームの最後にレイアウト更新が実行される
		void markLayoutAsDirty();

		/// @brief ノードに任意のデータを格納する
		/// @tparam TData データの型
		/// @param value 格納するデータ
		template <typename TData>
		void storeData(const TData& value);

		/// @brief ノードに格納された任意のデータを取得する
		/// @tparam TData データの型
		/// @return 格納されたデータ
		/// @throws Error 指定された型のデータが格納されていない場合は例外を送出する
		template <typename TData>
		[[nodiscard]]
		const TData& getStoredData() const;

		/// @brief ノードに格納された任意のデータをOptionalで取得する
		/// @tparam TData データの型
		/// @return 格納されたデータ。指定された型のデータが格納されていない場合はnoneを返す
		template <typename TData>
		[[nodiscard]]
		Optional<TData> getStoredDataOpt() const;

		/// @brief ノードに格納された任意のデータをデフォルト値付きで取得する
		/// @tparam TData データの型
		/// @param defaultValue デフォルト値
		/// @return 格納されたデータ。指定された型のデータが格納されていない場合はdefaultValueを返す
		template <typename TData>
		[[nodiscard]]	
		TData getStoredDataOr(const TData& defaultValue) const;

		/// @brief ノードのインスタンスIDを取得
		/// @return インスタンスID
		/// @note インスタンスIDはノード生成時に一意に割り当てられる連番。NocoEditorで内部的に使用される
		[[nodiscard]]
		uint64 instanceId() const
		{
			return m_instanceId;
		}

		/// @brief 子ノードの配列を取得
		/// @return 子ノードの配列
		[[nodiscard]]
		const Array<std::shared_ptr<Node>>& children() const override
		{
			return m_children;
		}

		/// @brief 子ノードの数を取得
		/// @return 子ノードの数
		[[nodiscard]]
		size_t childCount() const override
		{
			return m_children.size();
		}

		/// @brief 指定したインデックスの子ノードを取得
		/// @param index 子ノードのインデックス
		/// @return 子ノード。インデックスが範囲外の場合はnullptrを返す
		[[nodiscard]]
		std::shared_ptr<Node> childAt(size_t index) const override;

		/// @brief 兄弟ノードの中でのZオーダー(前後関係の優先順位)を取得
		/// @return Zオーダー
		[[nodiscard]]
		int32 zOrderInSiblings() const;

		/// @brief 兄弟ノードの中でのZオーダー(前後関係の優先順位)を設定
		/// @param zOrderInSiblings Zオーダー
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> setZOrderInSiblings(const PropertyValue<int32>& zOrderInSiblings);

		/// @brief 兄弟ノードの中でのZオーダー(前後関係の優先順位)のプロパティの参照を取得
		/// @return プロパティの参照
		[[nodiscard]]
		const PropertyValue<int32>& zOrderInSiblingsPropertyValue() const;

		/// @brief 兄弟ノードの中でのZオーダー(前後関係の優先順位)のパラメータ参照名を取得
		/// @return パラメータ参照名
		[[nodiscard]]
		const String& zOrderInSiblingsParamRef() const { return m_zOrderInSiblings.paramRef(); }

		/// @brief 兄弟ノードの中でのZオーダー(前後関係の優先順位)のパラメータ参照名を設定
		/// @param paramRef パラメータ参照名
		/// @return ノード自身(メソッドチェーンのため)
		std::shared_ptr<Node> setZOrderInSiblingsParamRef(const String& paramRef)
		{
			m_zOrderInSiblings.setParamRef(paramRef);
			return shared_from_this();
		}

		/// @brief 兄弟ノードの中でのZオーダー(前後関係の優先順位)のプロパティの参照を取得
		/// @return プロパティの参照
		[[nodiscard]]
		Property<int32>& zOrderInSiblingsProperty() { return m_zOrderInSiblings; }
		
		/// @brief 兄弟ノードの中でのZオーダー(前後関係の優先順位)のプロパティの参照を取得
		/// @return プロパティの参照
		[[nodiscard]]
		const Property<int32>& zOrderInSiblingsProperty() const { return m_zOrderInSiblings; }

		/// @brief Tweenコンポーネントのアクティブ状態を一括設定
		/// @param active アクティブ状態
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		void setTweenActiveAll(bool active, RecursiveYN recursive = RecursiveYN::Yes, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No);

		/// @brief 指定したタグを持つTweenコンポーネントのアクティブ状態を一括設定
		/// @param tag タグ
		/// @param active アクティブ状態
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		void setTweenActiveByTag(StringView tag, bool active, RecursiveYN recursive = RecursiveYN::Yes, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No);

		/// @brief 指定したタグを持つTweenコンポーネントが再生中かどうかを取得
		/// @param tag タグ
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @return Tweenコンポーネントが再生中の場合はtrue、そうでなければfalseを返す
		/// @note 指定したタグを持つTweenコンポーネントが複数ある場合、いずれかが再生中であればtrueを返す。指定したタグを持つTweenコンポーネントが見つからない場合はfalseを返す
		[[nodiscard]]
		bool isTweenPlayingByTag(StringView tag, RecursiveYN recursive = RecursiveYN::Yes, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No) const;

		/// @brief 指定したタグを持つTextBox/TextAreaのテキストを取得
		/// @param tag タグ
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @return TextBox/TextAreaのテキスト。見つからない場合は空文字列を返す
		[[nodiscard]]
		String getTextValueByTag(StringView tag, RecursiveYN recursive = RecursiveYN::Yes, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No) const;

		/// @brief 指定したタグを持つTextBox/TextAreaのテキストを取得(Optional版)
		/// @param tag タグ
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @return TextBox/TextAreaのテキスト。見つからない場合はnoneを返す
		[[nodiscard]]
		Optional<String> getTextValueByTagOpt(StringView tag, RecursiveYN recursive = RecursiveYN::Yes, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No) const;

		/// @brief 指定したタグを持つTextBox/TextAreaのテキストを設定
		/// @param tag タグ
		/// @param text テキスト
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @note 該当するすべてのTextBox/TextAreaに設定される
		void setTextValueByTag(StringView tag, StringView text, RecursiveYN recursive = RecursiveYN::Yes, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No);

		/// @brief 指定したタグを持つToggleコンポーネントの値を取得
		/// @param tag タグ
		/// @param defaultValue 見つからなかった場合のデフォルト値
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @return Toggleコンポーネントの値
		[[nodiscard]]
		bool getToggleValueByTag(StringView tag, bool defaultValue = false, RecursiveYN recursive = RecursiveYN::Yes, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No) const;

		/// @brief 指定したタグを持つToggleコンポーネントの値を取得(Optional版)
		/// @param tag タグ
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @return Toggleコンポーネントの値。見つからなかった場合はnoneを返す
		[[nodiscard]]
		Optional<bool> getToggleValueByTagOpt(StringView tag, RecursiveYN recursive = RecursiveYN::Yes, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No) const;

		/// @brief 指定したタグを持つToggleコンポーネントの値を設定
		/// @param tag タグ
		/// @param value 値
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		void setToggleValueByTag(StringView tag, bool value, RecursiveYN recursive = RecursiveYN::Yes, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No);

		/// @brief 指定したタグを持つSubCanvasを取得
		/// @param tag タグ
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @return SubCanvas。見つからない場合はnullptrを返す
		[[nodiscard]]
		std::shared_ptr<class SubCanvas> getSubCanvasByTag(StringView tag, RecursiveYN recursive = RecursiveYN::Yes, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No) const;
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
	Array<std::shared_ptr<Node>> Node::findAll(Fty&& predicate)
		requires std::invocable<Fty, const std::shared_ptr<Node>&>
	{
		Array<std::shared_ptr<Node>> result;
		findAll(std::forward<Fty>(predicate), &result, AppendYN::No);
		return result;
	}

	template <class Fty>
	void Node::findAll(Fty&& predicate, Array<std::shared_ptr<Node>>* pResults, AppendYN append)
		requires std::invocable<Fty, const std::shared_ptr<Node>&>
	{
		if (pResults == nullptr)
		{
			throw Error{ U"Node::findAll: pResults is nullptr" };
		}

		if (!append)
		{
			pResults->clear();
		}

		// 自分自身が条件を満たすかどうか
		if (predicate(shared_from_this()))
		{
			pResults->push_back(shared_from_this());
		}

		// 子ノードを再帰的に検索
		for (const auto& child : m_children)
		{
			child->findAll(predicate, pResults, AppendYN::Yes);
		}
	}

	template <class TComponent>
	[[nodiscard]]
	std::shared_ptr<TComponent> Node::getComponent(RecursiveYN recursive, IncludeSubCanvasYN includeSubCanvas) const
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
				if (const auto component = child->getComponent<TComponent>(RecursiveYN::Yes, includeSubCanvas))
				{
					return component;
				}
			}

			// SubCanvas内のコンポーネントも収集
			if (includeSubCanvas)
			{
				for (const auto& component : m_components)
				{
					for (const auto& child : component->subCanvasChildren())
					{
						if (const auto found = child->getComponent<TComponent>(RecursiveYN::Yes, includeSubCanvas))
						{
							return found;
						}
					}
				}
			}
		}
		return nullptr;
	}

	template <class TComponent>
	[[nodiscard]]
	Array<std::shared_ptr<TComponent>> Node::getComponents(RecursiveYN recursive, IncludeSubCanvasYN includeSubCanvas) const
	{
		Array<std::shared_ptr<TComponent>> result;
		getComponents(&result, recursive, includeSubCanvas, AppendYN::No);
		return result;
	}

	template <class TComponent>
	void Node::getComponents(Array<std::shared_ptr<TComponent>>* pResults, RecursiveYN recursive, IncludeSubCanvasYN includeSubCanvas, AppendYN append) const
	{
		if (pResults == nullptr)
		{
			throw Error{ U"Node::getComponents: pResults is nullptr" };
		}

		if (!append)
		{
			pResults->clear();
		}

		// 自身のコンポーネントから該当する型のものを収集
		for (const auto& component : m_components)
		{
			if (auto concreteComponent = std::dynamic_pointer_cast<TComponent>(component))
			{
				pResults->push_back(concreteComponent);
			}
		}

		// 再帰的に子ノードのコンポーネントも収集
		if (recursive)
		{
			for (const auto& child : m_children)
			{
				child->getComponents<TComponent>(pResults, RecursiveYN::Yes, includeSubCanvas, AppendYN::Yes);
			}

			// SubCanvas内のコンポーネントも収集
			if (includeSubCanvas)
			{
				for (const auto& component : m_components)
				{
					for (const auto& child : component->subCanvasChildren())
					{
						child->getComponents<TComponent>(pResults, RecursiveYN::Yes, includeSubCanvas, AppendYN::Yes);
					}
				}
			}
		}
	}

	template <class TComponent, class Fty>
	[[nodiscard]]
	Array<std::shared_ptr<TComponent>> Node::findComponentsAll(Fty&& predicate, RecursiveYN recursive, IncludeSubCanvasYN includeSubCanvas)
		requires std::invocable<Fty, const std::shared_ptr<TComponent>&>
	{
		Array<std::shared_ptr<TComponent>> result;
		findComponentsAll<TComponent>(std::forward<Fty>(predicate), &result, recursive, includeSubCanvas, AppendYN::No);
		return result;
	}

	template <class TComponent, class Fty>
	void Node::findComponentsAll(Fty&& predicate, Array<std::shared_ptr<TComponent>>* pResults, RecursiveYN recursive, IncludeSubCanvasYN includeSubCanvas, AppendYN append)
		requires std::invocable<Fty, const std::shared_ptr<TComponent>&>
	{
		if (pResults == nullptr)
		{
			throw Error{ U"Node::findComponentsAll: pResults is nullptr" };
		}

		if (!append)
		{
			pResults->clear();
		}

		// 自身のコンポーネントから条件を満たすものを収集
		for (const auto& component : m_components)
		{
			if (auto concreteComponent = std::dynamic_pointer_cast<TComponent>(component))
			{
				if (predicate(concreteComponent))
				{
					pResults->push_back(concreteComponent);
				}
			}
		}

		// 再帰的に子ノードのコンポーネントも収集
		if (recursive)
		{
			for (const auto& child : m_children)
			{
				child->findComponentsAll<TComponent>(std::forward<Fty>(predicate), pResults, RecursiveYN::Yes, includeSubCanvas, AppendYN::Yes);
			}

			// SubCanvas内のコンポーネントも収集
			if (includeSubCanvas)
			{
				for (const auto& component : m_components)
				{
					for (const auto& child : component->subCanvasChildren())
					{
						child->findComponentsAll<TComponent>(std::forward<Fty>(predicate), pResults, RecursiveYN::Yes, includeSubCanvas, AppendYN::Yes);
					}
				}
			}
		}
	}

	template<typename TData>
	void Node::storeData(const TData& value)
	{
		if (const auto dataStore = getComponent<DataStore<TData>>(RecursiveYN::No))
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
		if (const auto dataStore = getComponent<DataStore<TData>>(RecursiveYN::No))
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
		if (const auto dataStore = getComponent<DataStore<TData>>(RecursiveYN::No))
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
		if (const auto dataStore = getComponent<DataStore<TData>>(RecursiveYN::No))
		{
			return dataStore->value();
		}
		else
		{
			return defaultValue;
		}
	}

	template <typename TComponent>
	void Node::removeComponents(RecursiveYN recursive, IncludeSubCanvasYN includeSubCanvas)
	{
		// SubCanvasの子を先に処理（m_componentsから削除する前に行う必要がある）
		if (recursive == RecursiveYN::Yes && includeSubCanvas)
		{
			for (const auto& component : m_components)
			{
				for (const auto& subCanvasChild : component->subCanvasChildren())
				{
					subCanvasChild->removeComponents<TComponent>(RecursiveYN::Yes, includeSubCanvas);
				}
			}
		}

		// 自身のコンポーネントから指定された型のものを削除
		m_components.remove_if([this](const std::shared_ptr<ComponentBase>& component)
		{
			if (std::dynamic_pointer_cast<TComponent>(component) != nullptr)
			{
				if (m_activeInHierarchy)
				{
					component->onDeactivated(shared_from_this());
				}
				return true;
			}
			return false;
		});

		// 再帰的に処理する場合は子ノードも処理
		if (recursive == RecursiveYN::Yes)
		{
			for (const auto& child : m_children)
			{
				child->removeComponents<TComponent>(RecursiveYN::Yes, includeSubCanvas);
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
				if (child->inlineRegion())
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
				if (child->inlineRegion())
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
