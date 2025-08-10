#pragma once
#include <Siv3D.hpp>
#include "Node.hpp"
#include "Component/IFocusable.hpp"

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

	class Canvas : public std::enable_shared_from_this<Canvas>
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

		std::shared_ptr<Node> m_rootNode;
		/* NonSerialized */ Vec2 m_position = Vec2::Zero();
		/* NonSerialized */ Vec2 m_scale = Vec2::One();
		/* NonSerialized */ double m_rotation = 0.0;
		/* NonSerialized */ EventRegistry m_eventRegistry;
		/* NonSerialized */ bool m_prevDragScrollingWithThresholdExceeded = false;

		[[nodiscard]]
		Mat3x2 rootPosScaleMat() const;

		Canvas();

		explicit Canvas(const std::shared_ptr<Node>& rootNode);

	public:
		[[nodiscard]]
		static std::shared_ptr<Canvas> Create();

		[[nodiscard]]
		static std::shared_ptr<Canvas> Create(const std::shared_ptr<Node>& rootNode, RefreshesLayoutYN refreshesLayoutPre = RefreshesLayoutYN::Yes, RefreshesLayoutYN refreshesLayoutPost = RefreshesLayoutYN::Yes);

		void refreshLayout();

		bool containsNodeByName(const String& nodeName) const;

		[[nodiscard]]
		std::shared_ptr<Node> getNodeByName(const String& nodeName) const;

		[[nodiscard]]
		JSON toJSON() const;
		
		[[nodiscard]]
		JSON toJSONImpl(detail::IncludesInternalIdYN includesInternalId) const;

		[[nodiscard]]
		static std::shared_ptr<Canvas> CreateFromJSON(const JSON& json, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);
		
		[[nodiscard]]
		static std::shared_ptr<Canvas> CreateFromJSONImpl(const JSON& json, detail::IncludesInternalIdYN includesInternalId, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		bool tryReadFromJSON(const JSON& json, RefreshesLayoutYN refreshesLayoutPre = RefreshesLayoutYN::Yes, RefreshesLayoutYN refreshesLayoutPost = RefreshesLayoutYN::Yes);
		
		bool tryReadFromJSONImpl(const JSON& json, detail::IncludesInternalIdYN includesInternalId, RefreshesLayoutYN refreshesLayoutPre = RefreshesLayoutYN::Yes, RefreshesLayoutYN refreshesLayoutPost = RefreshesLayoutYN::Yes);

		void update(HitTestEnabledYN hitTestEnabled = HitTestEnabledYN::Yes);

		void draw() const;

		[[nodiscard]]
		const std::shared_ptr<Node>& rootNode() const;

		void removeChildrenAll();

		void resetWithNewRootNode(
			const RegionVariant& region,
			const String& name = U"Canvas",
			RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		std::shared_ptr<Canvas> setPosition(const Vec2& position);

		[[nodiscard]]
		const Vec2& position() const
		{
			return m_position;
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
	};

	template <class Fty>
	void Canvas::walkPlaceholders(StringView tag, Fty&& func) const
		requires std::invocable<Fty, const std::shared_ptr<Node>&>
	{
		if (m_rootNode)
		{
			m_rootNode->walkPlaceholders(tag, std::move(func), RecursiveYN::Yes);
		}
	}

	template <class Fty>
	void Canvas::walkPlaceholders(StringView tag, Fty&& func) const
		requires std::invocable<Fty, const std::shared_ptr<Node>&, const String&>
	{
		if (m_rootNode)
		{
			m_rootNode->walkPlaceholders(tag, std::move(func), RecursiveYN::Yes);
		}
	}
}
