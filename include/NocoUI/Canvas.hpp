#pragma once
#include <Siv3D.hpp>
#include "INodeContainer.hpp"
#include "Node.hpp"
#include "Component/IFocusable.hpp"
#include "Param.hpp"

namespace noco
{
	class ITextBox;
	class IFocusable;

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

	enum class AutoScaleMode : uint8
	{
		None,           // スケールしない
		ShrinkToFit,    // Canvas全体がシーン内に収まるよう調整
		ExpandToFill,   // シーン全体をCanvasで埋めるよう調整
		FitHeight,      // シーンの高さに合わせる
		FitWidth,       // シーンの幅に合わせる
	};

	enum class AutoResizeMode : uint8
	{
		None,           // リサイズしない
		MatchSceneSize, // シーンサイズに合わせる
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
		Array<std::shared_ptr<Node>> m_children;
		SizeF m_size = DefaultSize;
		AutoScaleMode m_autoScaleMode = AutoScaleMode::None;
		AutoResizeMode m_autoResizeMode = AutoResizeMode::None;

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

		HashTable<String, ParamValue> m_params;
		/* NonSerialized */ Vec2 m_position = Vec2::Zero();
		/* NonSerialized */ Vec2 m_scale = Vec2::One();
		/* NonSerialized */ double m_rotation = 0.0;
		/* NonSerialized */ EventRegistry m_eventRegistry;
		/* NonSerialized */ bool m_prevDragScrollingWithThresholdExceeded = false;
		/* NonSerialized */ Array<std::shared_ptr<Node>> m_childrenTempBuffer;
		/* NonSerialized */ Optional<SizeF> m_lastSceneSize;
		/* NonSerialized */ bool m_isEditorPreview = false;

		[[nodiscard]]
		Mat3x2 rootPosScaleMat() const;

		void updateAutoResizeIfNeeded();
		
		[[nodiscard]]
		Vec2 calculateAutoScale() const;

		Canvas();

	public:
		static constexpr SizeF DefaultSize{ 800, 600 };
		
		[[nodiscard]]
		static std::shared_ptr<Canvas> Create(const SizeF& size = DefaultSize);
		
		[[nodiscard]]
		static std::shared_ptr<Canvas> Create(double width, double height);

		void refreshLayout();

		bool containsNodeByName(const String& nodeName) const;

		[[nodiscard]]
		std::shared_ptr<Node> getNodeByName(const String& nodeName) const;

		[[nodiscard]]
		JSON toJSON() const;
		
		[[nodiscard]]
		JSON toJSONImpl(detail::IncludesInstanceIdYN includesInstanceId) const;

		[[nodiscard]]
		static std::shared_ptr<Canvas> CreateFromJSON(const JSON& json, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);
		
		[[nodiscard]]
		static std::shared_ptr<Canvas> CreateFromJSONImpl(const JSON& json, detail::IncludesInstanceIdYN includesInstanceId, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		bool tryReadFromJSON(const JSON& json, RefreshesLayoutYN refreshesLayoutPre = RefreshesLayoutYN::Yes, RefreshesLayoutYN refreshesLayoutPost = RefreshesLayoutYN::Yes);
		
		bool tryReadFromJSONImpl(const JSON& json, detail::IncludesInstanceIdYN includesInstanceId, RefreshesLayoutYN refreshesLayoutPre = RefreshesLayoutYN::Yes, RefreshesLayoutYN refreshesLayoutPost = RefreshesLayoutYN::Yes);

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
		AutoScaleMode autoScaleMode() const
		{
			return m_autoScaleMode;
		}

		std::shared_ptr<Canvas> setAutoScaleMode(AutoScaleMode mode);

		[[nodiscard]]
		AutoResizeMode autoResizeMode() const
		{
			return m_autoResizeMode;
		}

		std::shared_ptr<Canvas> setAutoResizeMode(AutoResizeMode mode);

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

		std::shared_ptr<Canvas> setTransform(const Vec2& position, const Vec2& scale, double rotation);

		void resetScrollOffsetRecursive(RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		void fireEvent(const Event& event);

		[[nodiscard]]
		bool isEventFiredWithTag(StringView tag) const;

		[[nodiscard]]
		Optional<Event> getFiredEventWithTag(StringView tag) const;

		[[nodiscard]]
		Array<Event> getFiredEventsWithTag(StringView tag) const;

		[[nodiscard]]
		const Array<Event>& getFiredEventsAll() const;

		template <class Fty>
		void walkPlaceholders(StringView tag, Fty&& func) const
			requires std::invocable<Fty, const std::shared_ptr<Node>&>;

		template <class Fty>
		void walkPlaceholders(StringView tag, Fty&& func) const
			requires std::invocable<Fty, const std::shared_ptr<Node>&, const String&>;

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

		// 新しいAPI
		template<typename T>
		void setParamValue(const String& name, const T& value)
		{
			m_params[name] = MakeParamValue(value);
		}

		// 複数パラメータの一括設定
		template<typename... Args>
		void setParamValues(const std::pair<String, Args>&... params)
		{
			(setParamValue(params.first, params.second), ...);
		}
		
		// 初期化リスト版（より簡潔な記述用）
		void setParamValues(std::initializer_list<std::pair<const char32_t*, std::variant<bool, int, double, const char32_t*, String, ColorF, Vec2, LRTB>>> params)
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

		void setSize(double width, double height, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes)
		{
			m_size = SizeF{ width, height };
			if (refreshesLayout)
			{
				refreshLayout();
			}
		}
		
		void setSize(const SizeF& size, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes)
		{
			m_size = size;
			if (refreshesLayout)
			{
				refreshLayout();
			}
		}
		
		void setWidth(double width, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes)
		{
			m_size.x = width;
			if (refreshesLayout)
			{
				refreshLayout();
			}
		}
		
		void setHeight(double height, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes)
		{
			m_size.y = height;
			if (refreshesLayout)
			{
				refreshLayout();
			}
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
			return false;
		}

		[[nodiscard]]
		bool isCanvas() const override
		{
			return true;
		}

		const std::shared_ptr<Node>& addChild(const std::shared_ptr<Node>& node, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);
		void removeChild(const std::shared_ptr<Node>& node, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);
		
		void removeChildrenAll(RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);
		
		const std::shared_ptr<Node>& addChildAtIndex(
			const std::shared_ptr<Node>& child,
			size_t index,
			RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);
		
		void swapChildren(size_t index1, size_t index2, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);
		
		[[nodiscard]]
		bool containsChild(
			const std::shared_ptr<Node>& child,
			RecursiveYN recursive = RecursiveYN::No) const;
		
		[[nodiscard]]
		bool containsChildByName(
			StringView name,
			RecursiveYN recursive = RecursiveYN::No) const;
		
		[[nodiscard]]
		std::shared_ptr<Node> getChildByNameOrNull(
			StringView name,
			RecursiveYN recursive = RecursiveYN::No);
		
		[[nodiscard]]
		Optional<size_t> indexOfChildOpt(
			const std::shared_ptr<Node>& child) const;
		
		const std::shared_ptr<Node>& emplaceChild(
			StringView name = U"Node",
			const RegionVariant& region = InlineRegion{},
			IsHitTargetYN isHitTarget = IsHitTargetYN::Yes,
			InheritChildrenStateFlags inheritChildrenStateFlags = InheritChildrenStateFlags::None,
			RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

	// JSONから子ノードを追加
	const std::shared_ptr<Node>& addChildFromJSON(const JSON& json, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

	// 全ノードのパラメータ参照を一括更新
	void replaceParamRefs(const String& oldName, const String& newName);

	// instanceIdによるノード検索
	[[nodiscard]]
	std::shared_ptr<Node> findNodeByInstanceId(uint64 instanceId) const;

private:
	// ノードツリー内でinstanceIdによるノード検索（再帰）
	[[nodiscard]]
	std::shared_ptr<Node> findNodeByInstanceIdRecursive(const std::shared_ptr<Node>& node, uint64 instanceId) const;
	};

	template <class Fty>
	void Canvas::walkPlaceholders(StringView tag, Fty&& func) const
		requires std::invocable<Fty, const std::shared_ptr<Node>&>
	{
		for (const auto& child : m_children)
		{
			child->walkPlaceholders(tag, std::move(func), RecursiveYN::Yes);
		}
	}

	template <class Fty>
	void Canvas::walkPlaceholders(StringView tag, Fty&& func) const
		requires std::invocable<Fty, const std::shared_ptr<Node>&, const String&>
	{
		for (const auto& child : m_children)
		{
			child->walkPlaceholders(tag, std::move(func), RecursiveYN::Yes);
		}
	}
}
