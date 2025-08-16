#include "Tooltip.hpp"

namespace noco::editor
{
	TooltipOpener::TooltipOpener(const std::shared_ptr<Canvas>& overlayCanvas, StringView tooltipText, StringView tooltipDetailText)
		: ComponentBase{ {} }
		, m_overlayCanvas{ overlayCanvas }
		, m_tooltipText{ tooltipText }
		, m_tooltipDetailText{ tooltipDetailText }
	{
	}

	TooltipOpener::~TooltipOpener()
	{
		destroyTooltip();
	}

	void TooltipOpener::createTooltip()
	{
		if (!m_tooltipText.empty() && !m_tooltipNode)
		{
			// ツールチップノードを生成
			m_tooltipNode = m_overlayCanvas->emplaceChild(
				U"Tooltip",
				AnchorRegion
				{
					.anchorMin = Anchor::TopLeft,
					.anchorMax = Anchor::TopLeft,
					.posDelta = Vec2{ 0, 0 },
					.sizeDelta = Vec2{ 0, 0 }, // サイズは後で調整
					.sizeDeltaPivot = Anchor::TopLeft,
				},
				IsHitTargetYN::No);
			m_tooltipNode->emplaceComponent<RectRenderer>(ColorF{ 0.1, 0.9 }, ColorF{ 0.3 }, 1.0, 4.0);
			m_tooltipNode->setChildrenLayout(
				VerticalLayout
				{
					.padding = LRTB{ 10, 10, 5, 5 },
					.spacing = 5,
					.horizontalAlign = HorizontalAlign::Left,
				});

			// メインテキスト用ノード
			const auto mainTextNode = m_tooltipNode->emplaceChild(
				U"MainText",
				InlineRegion
				{
					.sizeDelta = Vec2{ 0, 0 }, // サイズは後で調整
				},
				IsHitTargetYN::No);
			mainTextNode->emplaceComponent<Label>(
				m_tooltipText,
				U"",
				12,
				ColorF{ 1.0 },
				HorizontalAlign::Left,
				VerticalAlign::Top,
				LRTB::Zero(),
				HorizontalOverflow::Overflow);

			// 詳細説明がある場合は追加
			if (!m_tooltipDetailText.empty())
			{
				const auto detailTextNode = m_tooltipNode->emplaceChild(
					U"DetailText",
					InlineRegion
					{
						.sizeDelta = Vec2{ 0, 0 }, // サイズは後で調整
					},
					IsHitTargetYN::No);
				detailTextNode->emplaceComponent<Label>(
					m_tooltipDetailText,
					U"",
					11,
					ColorF{ 0.8 },
					HorizontalAlign::Left,
					VerticalAlign::Top,
					LRTB::Zero(),
					HorizontalOverflow::Overflow);
			}

			// サイズを内容に合わせて調整
			updateTooltipSize();
		}
	}

	void TooltipOpener::destroyTooltip()
	{
		if (m_tooltipNode)
		{
			m_overlayCanvas->removeChild(m_tooltipNode, RefreshesLayoutYN::No);
			m_tooltipNode = nullptr;
		}
	}

	void TooltipOpener::updateTooltipSize()
	{
		if (!m_tooltipNode)
		{
			return;
		}

		// 子要素のサイズを更新
		for (const auto& child : m_tooltipNode->children())
		{
			if (const auto label = child->getComponent<Label>())
			{
				const Vec2 contentSize = label->contentSize();
				if (const auto* pInlineRegion = child->inlineRegion())
				{
					auto newRegion = *pInlineRegion;
					newRegion.sizeDelta = contentSize;
					child->setRegion(newRegion, RefreshesLayoutYN::No);
				}
			}
		}

		// ツールチップ全体のサイズを更新
		if (const auto pRegion = m_tooltipNode->anchorRegion())
		{
			const Vec2 newSize = m_tooltipNode->getFittingSizeToChildren();
			if (pRegion->sizeDelta != newSize)
			{
				AnchorRegion newRegion = *pRegion;
				newRegion.sizeDelta = newSize;
				m_tooltipNode->setRegion(newRegion);
			}
		}
	}

	void TooltipOpener::updateKeyInput(const std::shared_ptr<Node>& node)
	{
		// インプットがブロックされている場合やクリックされた瞬間は何もしない
		if (CurrentFrame::HasKeyInputBlocked() || MouseL.down() || MouseM.down() || MouseR.down())
		{
			// ホバーが外れたらツールチップを破棄
			if (m_isShowing)
			{
				destroyTooltip();
				m_isShowing = false;
			}
			m_hoverTime = 0.0;
			return;
		}

		// Disabled時も含むホバー判定を使用
		const bool isHovered = node->isHovered(RecursiveYN::Yes, IncludingDisabledYN::Yes);

		if (isHovered)
		{
			m_hoverTime += Scene::DeltaTime();

			if (m_hoverTime >= ShowDelay && !m_isShowing)
			{
				// ツールチップを生成して表示
				createTooltip();
				m_isShowing = true;
			}

			if (m_isShowing && m_tooltipNode)
			{
				// マウス位置にツールチップを移動
				if (const auto pRegion = m_tooltipNode->anchorRegion())
				{
					Vec2 newPos = Cursor::Pos() + Vec2{ 0, 20 };
					const Vec2 tooltipSize = pRegion->sizeDelta;

					// 右端にはみ出す場合は左に寄せる
					if (newPos.x + tooltipSize.x > Scene::Width())
					{
						newPos.x = Scene::Width() - tooltipSize.x;
					}

					// 下端にはみ出す場合は上に寄せる
					if (newPos.y + tooltipSize.y > Scene::Height())
					{
						// マウスカーソルの上側に表示
						newPos.y = Cursor::Pos().y - tooltipSize.y - 5;
					}

					if (pRegion->posDelta != newPos)
					{
						AnchorRegion newRegion = *pRegion;
						newRegion.posDelta = newPos;
						m_tooltipNode->setRegion(newRegion);
					}
				}
			}
		}
		else
		{
			// ホバーが外れたらツールチップを破棄
			if (m_isShowing)
			{
				destroyTooltip();
				m_isShowing = false;
			}
			m_hoverTime = 0.0;
		}
	}

	void TooltipOpener::setTooltipText(StringView text, StringView detailText)
	{
		m_tooltipText = text;
		m_tooltipDetailText = detailText;

		// 表示中の場合は一度破棄して再生成
		if (m_isShowing)
		{
			destroyTooltip();
			createTooltip();
		}
	}
}
