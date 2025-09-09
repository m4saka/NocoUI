#pragma once
#include <Siv3D.hpp>
#include "INodeContainer.hpp"
#include "Node.hpp"
#include "Component/IFocusable.hpp"
#include "Param.hpp"
#include "ParamUtils.hpp"

namespace noco
{
	class ITextBox;
	class IFocusable;
	class ComponentFactory;

	struct CanvasUpdateContext
	{
		bool keyInputBlocked = false;
		std::weak_ptr<Node> hoveredNode;
		std::weak_ptr<Node> scrollableHoveredNode;
		std::weak_ptr<ITextBox> editingTextBox;
		std::weak_ptr<Node> draggingNode;
		std::weak_ptr<Node> focusedNode;

		// フレーム間で引き継ぐためresetしない
		std::weak_ptr<Node> dragScrollingNode;

		void clear()
		{
			keyInputBlocked = false;
			editingTextBox.reset();
			hoveredNode.reset();
			scrollableHoveredNode.reset();
			draggingNode.reset();
			// focusedNodeはフレーム間で保持するためクリアしない
		}
	};

	namespace detail
	{
		inline int32 s_lastCopiedCanvasUpdateContextToPrevFrameCount = -1;
		inline int32 s_lastUpdateInteractionStateFrameCount = -1;
		inline int32 s_lastUpdateInputFrameCount = -1;
		inline int32 s_lastUpdateFrameCount = -1;
		inline CanvasUpdateContext s_canvasUpdateContext;
		inline CanvasUpdateContext s_prevCanvasUpdateContext;
		inline bool s_isEditorMode = false;

		inline void ClearCanvasUpdateContextIfNeeded()
		{
			const int32 currentFrameCount = Scene::FrameCount();
			if (s_lastCopiedCanvasUpdateContextToPrevFrameCount == currentFrameCount)
			{
				return;
			}

			// 前のフレームの状態を保存
			s_lastCopiedCanvasUpdateContextToPrevFrameCount = currentFrameCount;
			s_prevCanvasUpdateContext = s_canvasUpdateContext;

			// 現在のフレームの状態をクリア
			s_lastUpdateInputFrameCount = currentFrameCount;
			s_canvasUpdateContext.clear();
		}
	}

	namespace CurrentFrame
	{
		[[nodiscard]]
		inline bool AnyNodeHovered()
		{
			return !detail::s_canvasUpdateContext.hoveredNode.expired();
		}

		[[nodiscard]]
		inline std::shared_ptr<Node> GetHoveredNode()
		{
			return detail::s_canvasUpdateContext.hoveredNode.lock();
		}

		[[nodiscard]]
		inline bool AnyScrollableNodeHovered()
		{
			return !detail::s_canvasUpdateContext.scrollableHoveredNode.expired();
		}

		[[nodiscard]]
		inline std::shared_ptr<Node> GetScrollableHoveredNode()
		{
			return detail::s_canvasUpdateContext.scrollableHoveredNode.lock();
		}

		[[nodiscard]]
		inline bool IsEditingTextBox()
		{
			return !detail::s_canvasUpdateContext.editingTextBox.expired();
		}

		[[nodiscard]]
		inline std::shared_ptr<ITextBox> GetEditingTextBox()
		{
			return detail::s_canvasUpdateContext.editingTextBox.lock();
		}

		[[nodiscard]]
		inline bool IsDraggingNode()
		{
			return !detail::s_canvasUpdateContext.draggingNode.expired();
		}

		[[nodiscard]]
		inline std::shared_ptr<Node> GetDraggingNode()
		{
			return detail::s_canvasUpdateContext.draggingNode.lock();
		}

		inline void BlockKeyInput()
		{
			detail::s_canvasUpdateContext.keyInputBlocked = true;
		}

		[[nodiscard]]
		inline bool HasKeyInputBlocked()
		{
			return detail::s_canvasUpdateContext.keyInputBlocked;
		}
		
		[[nodiscard]]
		inline bool IsFocused()
		{
			return !detail::s_canvasUpdateContext.focusedNode.expired();
		}
		
		[[nodiscard]]
		inline std::shared_ptr<Node> GetFocusedNode()
		{
			return detail::s_canvasUpdateContext.focusedNode.lock();
		}
		
		inline void SetFocusedNode(const std::shared_ptr<Node>& node)
		{
			auto currentFocused = detail::s_canvasUpdateContext.focusedNode.lock();
			
			// 同じノードの場合は何もしない
			if (currentFocused == node)
			{
				return;
			}
			
			// 既存のフォーカスがある場合はblurを呼ぶ
			if (currentFocused)
			{
				// ITextBoxがIFocusableを継承しているので、TextBoxを取得してblurを呼ぶ
				for (const auto& component : currentFocused->components())
				{
					if (auto focusable = std::dynamic_pointer_cast<IFocusable>(component))
					{
						focusable->blur(currentFocused);
					}
				}
			}
			
			// 新しいフォーカスを設定
			detail::s_canvasUpdateContext.focusedNode = node;
			
			// 新しいノードがある場合はfocusを呼ぶ
			if (node)
			{
				// ITextBoxがIFocusableを継承しているので、TextBoxを取得してfocusを呼ぶ
				for (const auto& component : node->components())
				{
					if (auto focusable = std::dynamic_pointer_cast<IFocusable>(component))
					{
						focusable->focus(node);
					}
				}
			}
		}

		inline bool UnfocusNodeIfFocused(const std::shared_ptr<Node>& node)
		{
			if (detail::s_canvasUpdateContext.focusedNode.lock() == node)
			{
				SetFocusedNode(nullptr);
				return true;
			}
			return false;
		}
	}

	namespace PrevFrame
	{
		[[nodiscard]]
		inline bool AnyNodeHovered()
		{
			return !detail::s_prevCanvasUpdateContext.hoveredNode.expired();
		}
		
		[[nodiscard]]
		inline std::shared_ptr<Node> GetHoveredNode()
		{
			return detail::s_prevCanvasUpdateContext.hoveredNode.lock();
		}
		
		[[nodiscard]]
		inline bool AnyScrollableNodeHovered()
		{
			return !detail::s_prevCanvasUpdateContext.scrollableHoveredNode.expired();
		}
		
		[[nodiscard]]
		inline std::shared_ptr<Node> GetScrollableHoveredNode()
		{
			return detail::s_prevCanvasUpdateContext.scrollableHoveredNode.lock();
		}

		[[nodiscard]]
		inline bool IsEditingTextBox()
		{
			return !detail::s_prevCanvasUpdateContext.editingTextBox.expired();
		}

		[[nodiscard]]
		inline std::shared_ptr<ITextBox> GetEditingTextBox()
		{
			return detail::s_prevCanvasUpdateContext.editingTextBox.lock();
		}

		[[nodiscard]]
		inline bool IsDraggingNode()
		{
			return !detail::s_prevCanvasUpdateContext.draggingNode.expired();
		}

		[[nodiscard]]
		inline std::shared_ptr<Node> GetDraggingNode()
		{
			return detail::s_prevCanvasUpdateContext.draggingNode.lock();
		}
	}

	[[nodiscard]]
	inline bool AnyNodeHovered()
	{
		return CurrentFrame::AnyNodeHovered() || PrevFrame::AnyNodeHovered();
	}

	[[nodiscard]]
	inline std::shared_ptr<Node> GetHoveredNode()
	{
		if (auto node = CurrentFrame::GetHoveredNode())
		{
			return node;
		}
		return PrevFrame::GetHoveredNode();
	}

	[[nodiscard]]
	inline bool AnyScrollableNodeHovered()
	{
		return CurrentFrame::AnyScrollableNodeHovered() || PrevFrame::AnyScrollableNodeHovered();
	}

	[[nodiscard]]
	inline std::shared_ptr<Node> GetScrollableHoveredNode()
	{
		if (auto node = CurrentFrame::GetScrollableHoveredNode())
		{
			return node;
		}
		return PrevFrame::GetScrollableHoveredNode();
	}

	[[nodiscard]]
	inline bool IsEditingTextBox()
	{
		return CurrentFrame::IsEditingTextBox() || PrevFrame::IsEditingTextBox();
	}

	[[nodiscard]]
	inline std::shared_ptr<ITextBox> GetEditingTextBox()
	{
		if (auto textBox = CurrentFrame::GetEditingTextBox())
		{
			return textBox;
		}
		return PrevFrame::GetEditingTextBox();
	}

	[[nodiscard]]
	inline bool IsDraggingNode()
	{
		return CurrentFrame::IsDraggingNode() || PrevFrame::IsDraggingNode();
	}

	[[nodiscard]]
	inline std::shared_ptr<Node> GetDraggingNode()
	{
		if (auto node = CurrentFrame::GetDraggingNode())
		{
			return node;
		}
		return PrevFrame::GetDraggingNode();
	}

	namespace detail
	{
		inline void SetEditorMode(bool isEditorMode)
		{
			s_isEditorMode = isEditorMode;
		}

		[[nodiscard]]
		inline bool IsEditorMode()
		{
			return s_isEditorMode;
		}
	}

	enum class AutoFitMode : uint8
	{
		None,                 // 自動調整なし
		Contain,              // アスペクト比維持で全体が見える
		Cover,                // アスペクト比維持で画面を埋める
		FitWidth,             // 幅に合わせてスケール（アスペクト比維持）
		FitHeight,            // 高さに合わせてスケール（アスペクト比維持）
		FitWidthMatchHeight,  // 幅はスケール、高さはサイズ変更
		FitHeightMatchWidth,  // 高さはスケール、幅はサイズ変更
		MatchSize,            // Canvasサイズをシーンサイズに変更
	};

	enum class EventTriggerType : uint8
	{
		None,
		Click,
		RightClick,
		HoverStart,
		HoverEnd,
		PressStart,
		PressEnd,
		RightPressStart,
		RightPressEnd,
	};

	struct Event
	{
		EventTriggerType triggerType = EventTriggerType::None;
		String tag;
		std::weak_ptr<Node> sourceNode;
	};

	class Canvas : public INodeContainer, public std::enable_shared_from_this<Canvas>
	{
		friend class Node;

	private:
		class EventRegistry
		{
		private:
			Array<Event> m_events;

		public:
			EventRegistry() = default;

			void addEvent(const Event& event);

			void clear();

			[[nodiscard]]
			bool isEventFiredWithTag(StringView tag) const;

			[[nodiscard]]
			Optional<Event> getFiredEventWithTag(StringView tag) const;

			[[nodiscard]]
			Array<Event> getFiredEventsWithTag(StringView tag) const;

			[[nodiscard]]
			const Array<Event>& getFiredEventsAll() const;
		};

		SizeF m_referenceSize = DefaultSize;
		LayoutVariant m_childrenLayout = FlowLayout{};
		Array<std::shared_ptr<Node>> m_children;
		AutoFitMode m_autoFitMode = AutoFitMode::None;
		HashTable<String, ParamValue> m_params;
		/* NonSerialized */ SizeF m_size = DefaultSize;
		/* NonSerialized */ Vec2 m_position = Vec2::Zero();
		/* NonSerialized */ Vec2 m_scale = Vec2::One();
		/* NonSerialized */ double m_rotation = 0.0;
		/* NonSerialized */ EventRegistry m_eventRegistry;
		/* NonSerialized */ bool m_prevDragScrollingWithThresholdExceeded = false;
		/* NonSerialized */ Optional<SizeF> m_lastSceneSize;
		/* NonSerialized */ bool m_isEditorPreview = false;
		/* NonSerialized */ int32 m_serializedVersion = CurrentSerializedVersion; // これは読み込んだバージョンで、シリアライズ時はこの変数の値ではなくCurrentSerializedVersionが固定で出力される
		/* NonSerialized */ bool m_isLayoutDirty = false; // レイアウト更新が必要かどうか
		/* NonSerialized */ InteractableYN m_interactable = InteractableYN::Yes;
		/* NonSerialized */ mutable Array<std::shared_ptr<Node>> m_tempChildrenBuffer; // 子ノードの一時バッファ(update内で別のCanvasのupdateが呼ばれる場合があるためthread_local staticにはできない。drawで呼ぶためmutableだが、drawはシングルスレッド前提なのでロック不要)

		[[nodiscard]]
		Mat3x2 rootPosScaleMat() const;

		void updateAutoFitIfNeeded();

		Canvas();

	public:
		~Canvas();

		static constexpr SizeF DefaultSize{ 800, 600 };
		
		[[nodiscard]]
		static std::shared_ptr<Canvas> Create(const SizeF& referenceSize = DefaultSize);
		
		[[nodiscard]]
		static std::shared_ptr<Canvas> Create(double width, double height);

		void refreshLayoutImmediately(OnlyIfDirtyYN onlyIfDirty = OnlyIfDirtyYN::Yes);
		
		void markLayoutAsDirty() { m_isLayoutDirty = true; }

		bool containsNodeByName(const String& nodeName) const;

		[[nodiscard]]
		std::shared_ptr<Node> getNodeByName(const String& nodeName) const;

		[[nodiscard]]
		JSON toJSON(detail::WithInstanceIdYN withInstanceId = detail::WithInstanceIdYN::No) const;

		[[nodiscard]]
		static std::shared_ptr<Canvas> CreateFromJSON(const JSON& json, detail::WithInstanceIdYN withInstanceId = detail::WithInstanceIdYN::No);
		
		[[nodiscard]]
		static std::shared_ptr<Canvas> CreateFromJSON(const JSON& json, const ComponentFactory& factory, detail::WithInstanceIdYN withInstanceId = detail::WithInstanceIdYN::No);

		[[nodiscard]]
		static std::shared_ptr<Canvas> LoadFromFile(FilePathView path, AllowExceptions allowExceptions = AllowExceptions::No);

		[[nodiscard]]
		static std::shared_ptr<Canvas> LoadFromFile(FilePathView path, const ComponentFactory& factory, AllowExceptions allowExceptions = AllowExceptions::No);

		bool tryReadFromJSON(const JSON& json, detail::WithInstanceIdYN withInstanceId = detail::WithInstanceIdYN::No);
		
		bool tryReadFromJSON(const JSON& json, const ComponentFactory& factory, detail::WithInstanceIdYN withInstanceId);

		void update(HitTestEnabledYN hitTestEnabled = HitTestEnabledYN::Yes);

		[[nodiscard]]
		std::shared_ptr<Node> hitTest(const Vec2& point) const;

		void draw() const;

		void clearAll();

		std::shared_ptr<Canvas> setPosition(const Vec2& position);

		[[nodiscard]]
		const Vec2& position() const
		{
			return m_position;
		}

		std::shared_ptr<Canvas> setCenter(const Vec2& center);

		[[nodiscard]]
		Vec2 center() const;

		[[nodiscard]]
		const SizeF& referenceSize() const
		{
			return m_referenceSize;
		}
		
		std::shared_ptr<Canvas> setReferenceSize(const SizeF& size);

		[[nodiscard]]
		AutoFitMode autoFitMode() const
		{
			return m_autoFitMode;
		}

		std::shared_ptr<Canvas> setAutoFitMode(AutoFitMode mode);

		std::shared_ptr<Canvas> setEditorPreviewInternal(bool isEditorPreview);

		[[nodiscard]]
		bool isEditorPreviewInternal() const
		{
			return m_isEditorPreview;
		}

		std::shared_ptr<Canvas> setScale(const Vec2& scale);

		[[nodiscard]]
		const Vec2& scale() const
		{
			return m_scale;
		}

		std::shared_ptr<Canvas> setPositionScale(const Vec2& position, const Vec2& scale);

		std::shared_ptr<Canvas> setRotation(double rotation);

		[[nodiscard]]
		double rotation() const
		{
			return m_rotation;
		}

		void resetScrollOffsetRecursive();

		void fireEvent(const Event& event);

		[[nodiscard]]
		bool isEventFiredWithTag(StringView tag) const;

		[[nodiscard]]
		Optional<Event> getFiredEventWithTag(StringView tag) const;

		[[nodiscard]]
		Array<Event> getFiredEventsWithTag(StringView tag) const;

		[[nodiscard]]
		const Array<Event>& getFiredEventsAll() const;


		[[nodiscard]]
		const HashTable<String, ParamValue>& params() const
		{
			return m_params;
		}

		[[nodiscard]]
		HashTable<String, ParamValue>& params()
		{
			return m_params;
		}

		template<typename T>
		void setParamValue(const String& name, const T& value)
		{
			if (!IsValidParameterName(name))
			{
				Logger << U"[NocoUI warning] Invalid parameter name '{}' rejected. Parameter names must start with a letter or underscore and contain only letters, digits, and underscores."_fmt(name);
				return;
			}
			m_params[name] = MakeParamValue(value);
		}

		template<typename... Args>
		void setParamValues(const std::pair<String, Args>&... params)
		{
			(setParamValue(params.first, params.second), ...);
		}
		
		void setParamValues(std::initializer_list<std::pair<const char32_t*, std::variant<bool, int32, double, const char32_t*, String, ColorF, Vec2, LRTB>>> params)
		{
			for (const auto& [name, value] : params)
			{
				std::visit([this, n = String{name}](const auto& v) {
					if constexpr (std::is_same_v<std::decay_t<decltype(v)>, const char32_t*>)
						setParamValue(n, String{v});
					else
						setParamValue(n, v);
				}, value);
			}
		}

		[[nodiscard]]
		Optional<ParamValue> param(const String& name) const;

		template<typename T>
		[[nodiscard]]
		Optional<T> paramValueOpt(const String& name) const
		{
			if (auto p = param(name))
			{
				return GetParamValueAs<T>(*p);
			}
			return none;
		}

		[[nodiscard]]
		bool hasParam(const String& name) const;

		[[nodiscard]]
		bool hasParamOfType(const String& name, ParamType paramType) const;

		void removeParam(const String& name);

		void clearParams();

		[[nodiscard]]
		size_t countParamRefs(StringView paramName) const;

		void clearParamRefs(StringView paramName);

		Array<String> removeInvalidParamRefs();

		void setSize(double width, double height)
		{
			m_size = SizeF{ width, height };
			markLayoutAsDirty();
		}
		
		void setSize(const SizeF& size)
		{
			m_size = size;
			markLayoutAsDirty();
		}
		
		void setWidth(double width)
		{
			m_size.x = width;
			markLayoutAsDirty();
		}
		
		void setHeight(double height)
		{
			m_size.y = height;
			markLayoutAsDirty();
		}

		[[nodiscard]]
		double width() const
		{
			return m_size.x;
		}

		[[nodiscard]]
		double height() const
		{
			return m_size.y;
		}
		
		[[nodiscard]]
		const SizeF& size() const
		{
			return m_size;
		}

		[[nodiscard]]
		Quad quad() const;

		[[nodiscard]]
		int32 serializedVersion() const
		{
			return m_serializedVersion;
		}

		[[nodiscard]]
		const LayoutVariant& childrenLayout() const override
		{
			return m_childrenLayout;
		}

		std::shared_ptr<Canvas> setChildrenLayout(const LayoutVariant& layout);

		[[nodiscard]]
		bool interactable() const
		{
			return m_interactable.getBool();
		}

		std::shared_ptr<Canvas> setInteractable(InteractableYN interactable);

		std::shared_ptr<Canvas> setInteractable(bool interactable)
		{
			return setInteractable(InteractableYN{ interactable });
		}

		[[nodiscard]]
		const FlowLayout* childrenFlowLayout() const override
		{
			return std::get_if<FlowLayout>(&m_childrenLayout);
		}

		[[nodiscard]]
		const HorizontalLayout* childrenHorizontalLayout() const override
		{
			return std::get_if<HorizontalLayout>(&m_childrenLayout);
		}

		[[nodiscard]]
		const VerticalLayout* childrenVerticalLayout() const override
		{
			return std::get_if<VerticalLayout>(&m_childrenLayout);
		}

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
			return false;
		}

		[[nodiscard]]
		bool isCanvas() const override
		{
			return true;
		}

		const std::shared_ptr<Node>& addChild(const std::shared_ptr<Node>& node) override;
		void removeChild(const std::shared_ptr<Node>& node) override;
		
		void removeChildrenAll() override;
		
		const std::shared_ptr<Node>& addChildAtIndex(
			const std::shared_ptr<Node>& child,
			size_t index) override;
		
		void swapChildren(size_t index1, size_t index2) override;
		
		[[nodiscard]]
		bool containsChild(
			const std::shared_ptr<Node>& child,
			RecursiveYN recursive = RecursiveYN::No) const override;
		
		[[nodiscard]]
		bool containsChildByName(
			StringView name,
			RecursiveYN recursive = RecursiveYN::No) const override;
		
		[[nodiscard]]
		std::shared_ptr<Node> getChildByName(
			StringView name,
			RecursiveYN recursive = RecursiveYN::No) override;
		
		[[nodiscard]]
		std::shared_ptr<Node> getChildByNameOrNull(
			StringView name,
			RecursiveYN recursive = RecursiveYN::No) override;
		
		[[nodiscard]]
		Optional<size_t> indexOfChildOpt(
			const std::shared_ptr<Node>& child) const override;
		
		const std::shared_ptr<Node>& emplaceChild(
			StringView name = U"Node",
			const RegionVariant& region = InlineRegion{},
			IsHitTargetYN isHitTarget = IsHitTargetYN::Yes,
			InheritChildrenStateFlags inheritChildrenStateFlags = InheritChildrenStateFlags::None) override;

	// JSONから子ノードを追加
	const std::shared_ptr<Node>& addChildFromJSON(const JSON& json) override;
	
	const std::shared_ptr<Node>& addChildFromJSON(const JSON& json, const ComponentFactory& factory) override;

	// 全ノードのパラメータ参照を一括更新
	void replaceParamRefs(const String& oldName, const String& newName);

	void clearCurrentFrameOverride();

	// instanceIdによるノード検索
	[[nodiscard]]
	std::shared_ptr<Node> findNodeByInstanceId(uint64 instanceId) const;

private:
	// ノードツリー内でinstanceIdによるノード検索（再帰）
	[[nodiscard]]
	std::shared_ptr<Node> findNodeByInstanceIdRecursive(const std::shared_ptr<Node>& node, uint64 instanceId) const;
	};
}
