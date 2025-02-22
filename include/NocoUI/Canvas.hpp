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

		Mat3x2 rootEffectMat() const
		{
			if (m_scale == Vec2::One() && m_offset == Vec2::Zero())
			{
				return Mat3x2::Identity();
			}
			return Mat3x2::Scale(m_scale) * Mat3x2::Translate(m_offset);
		}

		explicit Canvas()
			: Canvas{ Node::Create(
				U"Canvas",
				AnchorConstraint
				{
					.anchorMin = Anchor::TopLeft,
					.anchorMax = Anchor::BottomRight,
					.posDelta = Vec2{ 0, 0 },
					.sizeDelta = Vec2{ 0, 0 },
					.sizeDeltaPivot = Anchor::MiddleCenter,
				},
				IsHitTargetYN::No) }
		{
		}

		explicit Canvas(const std::shared_ptr<Node>& rootNode)
			: m_rootNode{ rootNode }
		{
			refreshLayout();
		}

	public:
		[[nodiscard]]
		static std::shared_ptr<Canvas> Create()
		{
			std::shared_ptr<Canvas> canvas{ new Canvas() };

			// コンストラクタ内ではshared_from_this()が使えないためここで設定
			canvas->rootNode()->setCanvasRecursive(canvas);

			return canvas;
		}

		[[nodiscard]]
		static std::shared_ptr<Canvas> Create(const std::shared_ptr<Node>& rootNode, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes)
		{
			std::shared_ptr<Canvas> canvas{ new Canvas(rootNode) };

			// コンストラクタ内ではshared_from_this()が使えないためここで設定
			canvas->rootNode()->setCanvasRecursive(canvas);

			if (refreshesLayout)
			{
				canvas->refreshLayout();
			}

			return canvas;
		}

		void refreshLayout()
		{
			const auto& rootConstraint = m_rootNode->constraint();
			if (const auto pBoxConstraint = std::get_if<BoxConstraint>(&rootConstraint))
			{
				m_rootNode->m_layoutAppliedRect = pBoxConstraint->applyConstraint(Scene::Rect(), Vec2::Zero());
			}
			else if (const auto pAnchorConstraint = std::get_if<AnchorConstraint>(&rootConstraint))
			{
				m_rootNode->m_layoutAppliedRect = pAnchorConstraint->applyConstraint(Scene::Rect(), Vec2::Zero());
			}
			else
			{
				// TODO: 実行時例外ではなくコンパイルエラーにしたい
				throw Error{ U"Unknown root node constraint" };
			}

			m_rootNode->refreshChildrenLayout();
			m_rootNode->refreshEffectedRect(rootEffectMat(), m_scale);
		}

		bool containsNodeByName(const String& nodeName) const
		{
			return m_rootNode->containsChildByName(nodeName, RecursiveYN::Yes);
		}

		[[nodiscard]]
		std::shared_ptr<Node> getNodeByName(const String& nodeName) const
		{
			return m_rootNode->getChildByName(nodeName, RecursiveYN::Yes);
		}

		[[nodiscard]]
		JSON toJSON() const
		{
			return m_rootNode->toJSON();
		}

		[[nodiscard]]
		static std::shared_ptr<Canvas> CreateFromJSON(const JSON& json, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes)
		{
			return Create(Node::CreateFromJSON(json), refreshesLayout);
		}

		bool tryReadFromJSON(const JSON& json, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes)
		{
			m_rootNode = Node::CreateFromJSON(json);
			m_rootNode->setCanvasRecursive(shared_from_this());
			if (refreshesLayout)
			{
				refreshLayout();
			}
			return true; // TODO: 失敗したらfalseを返す
		}

		void update(CanvasUpdateContext* pContext = nullptr)
		{
			// ホバー中ノード取得
			const bool canHover = (pContext ? pContext->canHover && !pContext->isHovered() : true) && Window::GetState().focused; // TODO: 本来はウィンドウがアクティブでない場合もホバーさせたい
			const auto hoveredNode = canHover ? m_rootNode->hoveredNodeInChildren() : nullptr;

			// スクロール可能なホバー中ノード取得
			auto scrollableHoveredNode = hoveredNode ? hoveredNode->findContainedScrollableNode() : nullptr;
			if (scrollableHoveredNode && !scrollableHoveredNode->rect().mouseOver())
			{
				// 子がホバー中でもスクロール可能ノード自身にマウスカーソルが重なっていない場合はスクロールしない
				scrollableHoveredNode = nullptr;
			}

			// スクロール実行
			if (scrollableHoveredNode)
			{
				const double wheel = Mouse::Wheel();
				const double wheelH = Mouse::WheelH();
				if (wheel != 0.0 || wheelH != 0.0)
				{
					scrollableHoveredNode->scroll(Vec2{ wheelH * 50, wheel * 50 });
				}
			}

			// ノード更新
			m_rootNode->update(pContext, hoveredNode, scrollableHoveredNode, Scene::DeltaTime(), rootEffectMat(), m_scale, InteractableYN::Yes, InteractState::Default, InteractState::Default);

			if (pContext)
			{
				pContext->canHover = pContext->canHover && hoveredNode == nullptr;
				if (hoveredNode)
				{
					pContext->hoveredNode = hoveredNode;
				}
				if (scrollableHoveredNode)
				{
					pContext->scrollableHoveredNode = scrollableHoveredNode;
				}
			}
		}

		void draw() const
		{
			m_rootNode->draw();
		}

		[[nodiscard]]
		const std::shared_ptr<Node>& rootNode() const
		{
			return m_rootNode;
		}

		void removeChildrenAll()
		{
			m_rootNode->removeChildrenAll();
		}

		void setOffset(const Vec2& offset)
		{
			m_offset = offset;
			m_rootNode->refreshEffectedRect(rootEffectMat(), m_scale);
		}

		[[nodiscard]]
		const Vec2& offset() const
		{
			return m_offset;
		}

		void setScale(const Vec2& scale)
		{
			m_scale = scale;
			m_rootNode->refreshEffectedRect(rootEffectMat(), m_scale);
		}

		[[nodiscard]]
		const Vec2& scale() const
		{
			return m_scale;
		}

		void setOffsetScale(const Vec2& offset, const Vec2& scale)
		{
			m_offset = offset;
			m_scale = scale;
			m_rootNode->refreshEffectedRect(rootEffectMat(), m_scale);
		}
	};
}
