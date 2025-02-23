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

	class Canvas : public std::enable_shared_from_this<Canvas>
	{
		friend class Node;

	private:
		std::shared_ptr<Node> m_rootNode;
		Vec2 m_offset = Vec2::Zero();
		Vec2 m_scale = Vec2::One();

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
	};
}
