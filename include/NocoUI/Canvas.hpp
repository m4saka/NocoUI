#pragma once
#include <Siv3D.hpp>
#include "Node.hpp"

namespace noco
{
	class ITextBox;

	struct CanvasUpdateContext
	{
		bool inputBlocked = false;
		std::weak_ptr<Node> hoveredNode;
		std::weak_ptr<Node> scrollableHoveredNode;
		std::weak_ptr<ITextBox> editingTextBox;
		std::weak_ptr<Node> draggingNode;

		void clearBeforeUpdateInput()
		{
			inputBlocked = false;
			editingTextBox.reset();
		}

		void clearBeforeUpdate()
		{
			hoveredNode.reset();
			scrollableHoveredNode.reset();
			draggingNode.reset();
		}
	};

	namespace detail
	{
		inline int32 s_lastCopiedCanvasUpdateContextToPrevFrameCount = -1;
		inline int32 s_lastUpdateInteractStateFrameCount = -1;
		inline int32 s_lastUpdateInputFrameCount = -1;
		inline int32 s_lastUpdateFrameCount = -1;
		inline CanvasUpdateContext s_canvasUpdateContext;
		inline CanvasUpdateContext s_prevCanvasUpdateContext;

		inline void CopyCanvasUpdateContextToPrevIfNeeded()
		{
			const int32 currentFrameCount = Scene::FrameCount();
			if (s_lastCopiedCanvasUpdateContextToPrevFrameCount == currentFrameCount)
			{
				return;
			}
			s_lastCopiedCanvasUpdateContextToPrevFrameCount = currentFrameCount;
			s_prevCanvasUpdateContext = s_canvasUpdateContext;
		}

		inline void ClearCanvasUpdateContextBeforeUpdateInputIfNeeded()
		{
			const int32 currentFrameCount = Scene::FrameCount();
			if (s_lastUpdateInputFrameCount == currentFrameCount)
			{
				return;
			}
			s_lastUpdateInputFrameCount = currentFrameCount;
			s_canvasUpdateContext.clearBeforeUpdateInput();
		}

		inline void ClearCanvasUpdateContextBeforeUpdateIfNeeded()
		{
			const int32 currentFrameCount = Scene::FrameCount();
			if (s_lastUpdateFrameCount == currentFrameCount)
			{
				return;
			}
			s_lastUpdateFrameCount = currentFrameCount;
			s_canvasUpdateContext.clearBeforeUpdate();
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

		inline void BlockInput()
		{
			detail::s_canvasUpdateContext.inputBlocked = true;
		}

		[[nodiscard]]
		inline bool HasInputBlocked()
		{
			return detail::s_canvasUpdateContext.inputBlocked;
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

	enum class TriggerType : uint8
	{
		None = 0,
		Click,
		HoverStart,
		HoverEnd,
		PressStart,
		PressEnd,
	};

	struct Event
	{
		TriggerType triggerType = TriggerType::None;
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
			constexpr static size_t InitialCapacity = 16;

			int32 m_prevFrameCount = -1;
			Array<Event> m_events;
			Array<Event> m_emptyEvents; // フレームが変わった際にm_eventsをclearせずに空配列のconst参照を返すためのダミー配列

		public:
			EventRegistry();

			void addEvent(const Event& event);

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
		Vec2 m_offset = Vec2::Zero();
		Vec2 m_scale = Vec2::One();
		EventRegistry m_eventRegistry;

		[[nodiscard]]
		Mat3x2 rootEffectMat() const;

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
		static std::shared_ptr<Canvas> CreateFromJSON(const JSON& json, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes);

		bool tryReadFromJSON(const JSON& json, RefreshesLayoutYN refreshesLayoutPre = RefreshesLayoutYN::Yes, RefreshesLayoutYN refreshesLayoutPost = RefreshesLayoutYN::Yes);

		void update(HitTestEnabledYN hitTestEnabled = HitTestEnabledYN::Yes);

		void draw() const;

		[[nodiscard]]
		const std::shared_ptr<Node>& rootNode() const;

		void removeChildrenAll();

		void setOffset(const Vec2& offset);

		[[nodiscard]]
		const Vec2& offset() const
		{
			return m_offset;
		}

		void setScale(const Vec2& scale);

		[[nodiscard]]
		const Vec2& scale() const
		{
			return m_scale;
		}

		void setOffsetScale(const Vec2& offset, const Vec2& scale);

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
