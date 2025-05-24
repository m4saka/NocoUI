#pragma once
#include <Siv3D.hpp>
#include "Node.hpp"

namespace noco
{
	class TextBox;

	struct CanvasUpdateContext
	{
		bool canHover = true;
		std::weak_ptr<Node> hoveredNode;
		std::weak_ptr<Node> scrollableHoveredNode;
		std::weak_ptr<TextBox> editingTextBox;
		std::weak_ptr<Node> draggingNode;

		bool isScrollableHovered() const
		{
			return !scrollableHoveredNode.expired();
		}

		bool isHovered() const
		{
			return !hoveredNode.expired();
		}
	};

	enum class EventType : uint8
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
		EventType type = EventType::None;
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

		void update(CanvasUpdateContext* pContext = nullptr);

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
		void enumeratePlaceholdersWithTagRecursive(StringView tag, Fty&& func) const
			requires std::invocable<Fty, const std::shared_ptr<Node>&>;

		template <class Fty>
		void enumeratePlaceholdersWithTagRecursive(StringView tag, Fty&& func) const
			requires std::invocable<Fty, const std::shared_ptr<Node>&, const String&>;
	};

	template <class Fty>
	void Canvas::enumeratePlaceholdersWithTagRecursive(StringView tag, Fty&& func) const
		requires std::invocable<Fty, const std::shared_ptr<Node>&>
	{
		if (m_rootNode)
		{
			m_rootNode->enumeratePlaceholdersWithTagRecursive(tag, std::move(func));
		}
	}

	template <class Fty>
	void Canvas::enumeratePlaceholdersWithTagRecursive(StringView tag, Fty&& func) const
		requires std::invocable<Fty, const std::shared_ptr<Node>&, const String&>
	{
		if (m_rootNode)
		{
			m_rootNode->enumeratePlaceholdersWithTagRecursive(tag, std::move(func));
		}
	}
}
