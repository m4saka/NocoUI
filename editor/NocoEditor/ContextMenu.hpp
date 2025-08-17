#pragma once
#include <NocoUI.hpp>
#include "KeyInputBlocker.hpp"

namespace noco::editor
{
	using CheckedYN = YesNo<struct CheckedYN_tag>;
	using ScreenMaskEnabledYN = YesNo<struct ScreenMaskEnabledYN_tag>;

	struct MenuItem
	{
		String text;
		String hotKeyText;
		Optional<Input> mnemonicInput = none;
		std::function<void()> onClick = nullptr;
		std::function<bool()> fnIsEnabled = [] { return true; };
	};

	struct CheckableMenuItem
	{
		String text;
		String hotKeyText;
		Optional<Input> mnemonicInput = none;
		std::function<void(CheckedYN)> onClick = nullptr;
		CheckedYN checked = CheckedYN::No;
		std::function<bool()> fnIsEnabled = [] { return true; };
	};

	struct MenuSeparator
	{
	};

	using MenuElement = std::variant<MenuItem, CheckableMenuItem, MenuSeparator>;

	[[nodiscard]]
	static PropertyValue<ColorF> MenuItemRectFillColor()
	{
		return PropertyValue<ColorF>{ ColorF{ 0.8, 0.0 }, ColorF{ 0.8 }, ColorF{ 0.8 }, ColorF{ 0.8, 0.0 }, 0.05 };
	}

	class ContextMenu
	{
	public:
		static constexpr int32 DefaultMenuItemWidth = 300;
		static constexpr int32 MenuItemHeight = 30;

	private:
		std::shared_ptr<Canvas> m_editorOverlayCanvas;
		std::shared_ptr<Node> m_screenMaskNode;
		std::shared_ptr<Node> m_rootNode;

		Array<MenuElement> m_elements;
		Array<std::shared_ptr<Node>> m_elementNodes;

		std::function<void()> m_fnOnHide = nullptr;

		bool m_isFirstUpdateSinceShown = false;

	public:
		explicit ContextMenu(const std::shared_ptr<Canvas>& editorOverlayCanvas, StringView name = U"ContextMenu")
			: m_editorOverlayCanvas(editorOverlayCanvas)
			, m_screenMaskNode(editorOverlayCanvas->emplaceChild(
				U"{}_ScreenMask"_fmt(name),
				AnchorRegion
				{
					.anchorMin = Anchor::TopLeft,
					.anchorMax = Anchor::BottomRight,
					.posDelta = Vec2{ 0, 0 },
					.sizeDelta = Vec2{ 0, 0 },
					.sizeDeltaPivot = Anchor::TopLeft,
				}))
				, m_rootNode(m_screenMaskNode->emplaceChild(
					U"{}_Root"_fmt(name),
					AnchorRegion
					{
						.anchorMin = Anchor::TopLeft,
						.anchorMax = Anchor::TopLeft,
						.posDelta = Vec2{ 0, 0 },
						.sizeDelta = Vec2{ DefaultMenuItemWidth, 0 },
						.sizeDeltaPivot = Anchor::TopLeft,
					}))
		{
			m_screenMaskNode->emplaceComponent<KeyInputBlocker>();
			m_screenMaskNode->setActive(ActiveYN::No, RefreshesLayoutYN::No);

			m_rootNode->setChildrenLayout(VerticalLayout{}, RefreshesLayoutYN::No);
			m_rootNode->setVerticalScrollable(true, RefreshesLayoutYN::No);
			m_rootNode->emplaceComponent<RectRenderer>(ColorF{ 0.95 }, Palette::Black, 0.0, 0.0, ColorF{ 0.0, 0.4 }, Vec2{ 2, 2 }, 5);

			m_editorOverlayCanvas->refreshLayout();
		}

		void show(const Vec2& pos, const Array<MenuElement>& elements, int32 menuItemWidth = DefaultMenuItemWidth, ScreenMaskEnabledYN screenMaskEnabled = ScreenMaskEnabledYN::Yes, std::function<void()> fnOnHide = nullptr)
		{
			// 前回開いていたメニューを閉じてから表示
			hide(RefreshesLayoutYN::No);
			m_elements = elements;
			m_elementNodes.reserve(m_elements.size());
			m_fnOnHide = std::move(fnOnHide);

			if (const AnchorRegion* pAnchorRegion = m_rootNode->anchorRegion())
			{
				AnchorRegion newRegion = *pAnchorRegion;
				newRegion.sizeDelta.x = menuItemWidth;
				m_rootNode->setRegion(newRegion, RefreshesLayoutYN::No);
			}

			for (size_t i = 0; i < m_elements.size(); ++i)
			{
				if (const auto pItem = std::get_if<MenuItem>(&m_elements[i]))
				{
					// 項目
					const auto itemNode = m_rootNode->emplaceChild(
						U"MenuItem_{}"_fmt(i),
						InlineRegion
						{
							.sizeRatio = Vec2{ 1, 0 },
							.sizeDelta = Vec2{ 0, MenuItemHeight },
						},
						IsHitTargetYN::Yes,
						InheritChildrenStateFlags::None,
						RefreshesLayoutYN::No);
					itemNode->emplaceComponent<RectRenderer>(MenuItemRectFillColor());
					const auto label = itemNode->emplaceComponent<Label>(pItem->text, U"", 14, PropertyValue<ColorF>{ ColorF{ 0.0 } }.withDisabled(ColorF{ 0.5 }), HorizontalAlign::Left, VerticalAlign::Middle, LRTB{ 30, 10, 0, 0 });
					if (!pItem->hotKeyText.empty())
					{
						itemNode->emplaceComponent<Label>(pItem->hotKeyText, U"", 14, PropertyValue<ColorF>{ ColorF{ 0.0 } }.withDisabled(ColorF{ 0.5 }), HorizontalAlign::Right, VerticalAlign::Middle, LRTB{ 0, 10, 0, 0 });
					}
					if (pItem->mnemonicInput.has_value())
					{
						// ショートカットキー(ニーモニック)があれば追加
						const Input& input = *pItem->mnemonicInput;
						itemNode->addClickHotKey(input);
						String text = label->text().defaultValue;
						bool dot = false;
						if (text.ends_with(U"..."))
						{
							text = text.substr(0, text.size() - 3);
							dot = true;
						}
						label->setText(U"{}({}){}"_fmt(text, input.name(), dot ? U"..." : U""));
					}
					itemNode->setInteractable(pItem->fnIsEnabled());
					m_elementNodes.push_back(itemNode);
				}
				else if (const auto pCheckableItem = std::get_if<CheckableMenuItem>(&m_elements[i]))
				{
					// チェック可能な項目
					const auto itemNode = m_rootNode->emplaceChild(
						U"CheckableMenuItem_{}"_fmt(i),
						InlineRegion
						{
							.sizeRatio = Vec2{ 1, 0 },
							.sizeDelta = Vec2{ 0, MenuItemHeight },
						},
						IsHitTargetYN::Yes,
						InheritChildrenStateFlags::None,
						RefreshesLayoutYN::No);
					itemNode->emplaceComponent<RectRenderer>(MenuItemRectFillColor());
					const auto label = itemNode->emplaceComponent<Label>(pCheckableItem->text, U"", 14, PropertyValue<ColorF>{ ColorF{ 0.0 } }.withDisabled(ColorF{ 0.5 }), HorizontalAlign::Left, VerticalAlign::Middle, LRTB{ 30, 10, 0, 0 });
					if (!pCheckableItem->hotKeyText.empty())
					{
						itemNode->emplaceComponent<Label>(pCheckableItem->hotKeyText, U"", 14, PropertyValue<ColorF>{ ColorF{ 0.0 } }.withDisabled(ColorF{ 0.5 }), HorizontalAlign::Right, VerticalAlign::Middle, LRTB{ 0, 10, 0, 0 });
					}
					if (pCheckableItem->mnemonicInput.has_value())
					{
						// ショートカットキー(ニーモニック)があれば追加
						const Input& input = *pCheckableItem->mnemonicInput;
						itemNode->addClickHotKey(input);
						String text = label->text().defaultValue;
						bool dot = false;
						if (text.ends_with(U"..."))
						{
							text = text.substr(0, text.size() - 3);
							dot = true;
						}
						label->setText(U"{}({}){}"_fmt(text, input.name(), dot ? U"..." : U""));
					}
					itemNode->emplaceComponent<Label>(
						pCheckableItem->checked ? U"✔" : U"",
						U"",
						14,
						PropertyValue<ColorF>{ ColorF{ 0.0 } }.withDisabled(ColorF{ 0.5 }),
						HorizontalAlign::Left,
						VerticalAlign::Middle,
						LRTB{ 10, 10, 0, 0 });
					itemNode->setInteractable(pCheckableItem->fnIsEnabled());
					m_elementNodes.push_back(itemNode);
				}
				else if (std::holds_alternative<MenuSeparator>(m_elements[i]))
				{
					// セパレータ
					const auto separatorNode = m_rootNode->emplaceChild(
						U"Separator",
						InlineRegion
						{
							.sizeRatio = Vec2{ 1, 0 },
							.sizeDelta = Vec2{ 0, 8 },
						},
						IsHitTargetYN::No,
						InheritChildrenStateFlags::None,
						RefreshesLayoutYN::No);
					separatorNode->emplaceChild(
						U"SeparatorLine",
						AnchorRegion
						{
							.anchorMin = Anchor::MiddleLeft,
							.anchorMax = Anchor::MiddleRight,
							.sizeDelta = Vec2{ -10, 1 },
							.sizeDeltaPivot = Anchor::MiddleCenter,
						},
						IsHitTargetYN::No)
						->emplaceComponent<RectRenderer>(ColorF{ 0.7 });

					m_elementNodes.push_back(separatorNode);
				}
			}

			double x = pos.x;
			double y = pos.y;

			// 右端にはみ出す場合は左に寄せる
			if (x + menuItemWidth > Scene::Width())
			{
				x = Scene::Width() - menuItemWidth;
			}

			// メニューの高さを計算
			const double contentHeight = m_rootNode->getFittingSizeToChildren().y;
			const double maxMenuHeight = Scene::Height();
			const double menuHeight = std::min(contentHeight, maxMenuHeight);

			if (const auto pAnchorRegion = m_rootNode->anchorRegion())
			{
				auto newRegion = *pAnchorRegion;
				newRegion.sizeDelta.y = menuHeight;
				m_rootNode->setRegion(newRegion, RefreshesLayoutYN::No);
			}

			// 下端にはみ出す場合は上に寄せる
			if (y + menuHeight > Scene::Height())
			{
				y = Scene::Height() - menuHeight;
			}

			// 上端にはみ出す場合は下に寄せる
			if (y < 0)
			{
				y = 0;
			}

			if (const auto pAnchorRegion = m_rootNode->anchorRegion())
			{
				auto newRegion = *pAnchorRegion;
				newRegion.posDelta = Vec2{ x, y };
				m_rootNode->setRegion(newRegion, RefreshesLayoutYN::No);
			}

			m_screenMaskNode->setIsHitTarget(screenMaskEnabled.getBool());
			m_screenMaskNode->setActive(ActiveYN::Yes, RefreshesLayoutYN::Yes);

			m_isFirstUpdateSinceShown = true;
		}

		void hide(RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes)
		{
			if (m_fnOnHide)
			{
				m_fnOnHide();
				m_fnOnHide = nullptr;
			}
			m_elements.clear();
			m_elementNodes.clear();
			m_screenMaskNode->setActive(ActiveYN::No, RefreshesLayoutYN::No);
			m_rootNode->removeChildrenAll(RefreshesLayoutYN::No);
			m_isFirstUpdateSinceShown = false;
			if (refreshesLayout)
			{
				m_editorOverlayCanvas->refreshLayout();
			}
		}

		void update()
		{
			// 初回フレームは閉じる判定しない
			if (!m_screenMaskNode->activeSelf() || m_isFirstUpdateSinceShown)
			{
				m_isFirstUpdateSinceShown = false;
				return;
			}

			// メニュー項目クリック判定
			for (size_t i = 0; i < m_elements.size(); ++i)
			{
				if (const auto pItem = std::get_if<MenuItem>(&m_elements[i]))
				{
					if (m_elementNodes[i]->isClicked())
					{
						if (pItem->onClick)
						{
							pItem->onClick();
							hide();
							break;
						}
					}
				}
				else if (const auto pCheckableItem = std::get_if<CheckableMenuItem>(&m_elements[i]))
				{
					if (m_elementNodes[i]->isClicked())
					{
						if (pCheckableItem->onClick)
						{
							pCheckableItem->onClick(CheckedYN{ !pCheckableItem->checked });
							// pCheckableItemは下記のhideで即座に解放されるため、ここでのpCheckableItemのチェック状態の書き換えは不要
							hide();
							break;
						}
					}
				}
			}

			if (!m_rootNode->isHovered(RecursiveYN::Yes) && !m_rootNode->hitQuad().mouseOver() && (MouseL.down() || MouseM.down() || MouseR.down()))
			{
				// メニュー外クリックで閉じる
				// (無効項目のクリックで消えないようにするため、rectがmouseOverしていないこともチェック)
				hide();
			}
			else if (KeyEscape.down())
			{
				// Escキーで閉じる
				// (無効項目のクリックで消えないようにするため、rectがmouseOverしていないこともチェック)
				hide();
			}
			else if (!Window::GetState().focused)
			{
				// ウィンドウが非アクティブになった場合は閉じる
				hide();
			}
		}

		[[nodiscard]]
		bool isHoveredRecursive() const
		{
			return m_rootNode->isHovered(RecursiveYN::Yes);
		}
	};

	class ContextMenuOpener : public ComponentBase
	{
	private:
		std::shared_ptr<ContextMenu> m_contextMenu;
		Array<MenuElement> m_menuElements;
		std::function<void()> m_fnBeforeOpen;
		RecursiveYN m_recursive;

	public:
		explicit ContextMenuOpener(const std::shared_ptr<ContextMenu>& contextMenu, Array<MenuElement> menuElements, std::function<void()> fnBeforeOpen = nullptr, RecursiveYN recursive = RecursiveYN::No)
			: ComponentBase{ {} }
			, m_contextMenu{ contextMenu }
			, m_menuElements{ std::move(menuElements) }
			, m_fnBeforeOpen{ std::move(fnBeforeOpen) }
			, m_recursive{ recursive }
		{
		}

		void update(const std::shared_ptr<Node>& node) override
		{
			if (node->isRightClicked(RecursiveYN{ m_recursive }))
			{
				openManually();
			}
		}

		void draw(const Node&) const override
		{
		}

		void openManually(const Vec2& pos = Cursor::PosF()) const
		{
			if (m_fnBeforeOpen)
			{
				m_fnBeforeOpen();
			}
			m_contextMenu->show(pos, m_menuElements);
		}
	};
}
