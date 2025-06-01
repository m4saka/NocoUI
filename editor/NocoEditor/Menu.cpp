#include "Menu.hpp"

using namespace noco;

[[nodiscard]]
PropertyValue<ColorF> MenuItemRectFillColor()
{
	return PropertyValue<ColorF>{ ColorF{ 0.8, 0.0 }, ColorF{ 0.8 }, ColorF{ 0.8 }, ColorF{ 0.8, 0.0 }, 0.05 };
}

ContextMenu::ContextMenu(const std::shared_ptr<Canvas>& editorOverlayCanvas, StringView name)
	: m_editorOverlayCanvas(editorOverlayCanvas)
	, m_screenMaskNode(editorOverlayCanvas->rootNode()->emplaceChild(
		U"{}_ScreenMask"_fmt(name),
		AnchorConstraint
		{
			.anchorMin = Anchor::TopLeft,
			.anchorMax = Anchor::BottomRight,
			.posDelta = Vec2{ 0, 0 },
			.sizeDelta = Vec2{ 0, 0 },
			.sizeDeltaPivot = Anchor::TopLeft,
		}))
	, m_rootNode(m_screenMaskNode->emplaceChild(
		U"{}_Root"_fmt(name),
		AnchorConstraint
		{
			.anchorMin = Anchor::TopLeft,
			.anchorMax = Anchor::TopLeft,
			.posDelta = Vec2{ 0, 0 },
			.sizeDelta = Vec2{ DefaultMenuItemWidth, 0 },
			.sizeDeltaPivot = Anchor::TopLeft,
		}))
{
	m_screenMaskNode->emplaceComponent<InputBlocker>();
	m_screenMaskNode->setActive(ActiveYN::No, RefreshesLayoutYN::No);

	m_rootNode->setBoxChildrenLayout(VerticalLayout{}, RefreshesLayoutYN::No);
	m_rootNode->setVerticalScrollable(true, RefreshesLayoutYN::No);
	m_rootNode->emplaceComponent<RectRenderer>(ColorF{ 0.95 }, Palette::Black, 0.0, 0.0, ColorF{ 0.0, 0.4 }, Vec2{ 2, 2 }, 5);

	m_editorOverlayCanvas->refreshLayout();
}

void ContextMenu::show(const Vec2& pos, const Array<MenuElement>& elements, int32 menuItemWidth, ScreenMaskEnabledYN screenMaskEnabled, std::function<void()> fnOnHide)
{
	// 前回開いていたメニューを閉じてから表示
	hide(RefreshesLayoutYN::No);
	m_elements = elements;
	m_elementNodes.reserve(m_elements.size());
	m_fnOnHide = std::move(fnOnHide);

	if (const AnchorConstraint* pAnchorConstraint = m_rootNode->anchorConstraint())
	{
		AnchorConstraint newConstraint = *pAnchorConstraint;
		newConstraint.sizeDelta.x = menuItemWidth;
		m_rootNode->setConstraint(newConstraint, RefreshesLayoutYN::No);
	}

	for (size_t i = 0; i < m_elements.size(); ++i)
	{
		if (const auto pItem = std::get_if<MenuItem>(&m_elements[i]))
		{
			// 項目
			const auto itemNode = m_rootNode->emplaceChild(
				U"MenuItem_{}"_fmt(i),
				BoxConstraint
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
				BoxConstraint
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
				BoxConstraint
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 8 },
				},
				IsHitTargetYN::No,
				InheritChildrenStateFlags::None,
				RefreshesLayoutYN::No);
			separatorNode->emplaceChild(
				U"SeparatorLine",
				AnchorConstraint
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

	if (const auto pAnchorConstraint = m_rootNode->anchorConstraint())
	{
		auto newConstraint = *pAnchorConstraint;
		newConstraint.sizeDelta.y = m_rootNode->getFittingSizeToChildren().y;
		m_rootNode->setConstraint(newConstraint, RefreshesLayoutYN::No);
	}

	// 下端にはみ出す場合は上に寄せる
	const double menuHeight = m_rootNode->anchorConstraint()->sizeDelta.y;
	if (y + menuHeight > Scene::Height())
	{
		y = Scene::Height() - menuHeight;
	}

	if (const auto pAnchorConstraint = m_rootNode->anchorConstraint())
	{
		auto newConstraint = *pAnchorConstraint;
		newConstraint.posDelta = Vec2{ x, y };
		m_rootNode->setConstraint(newConstraint, RefreshesLayoutYN::No);
	}

	m_screenMaskNode->setIsHitTarget(screenMaskEnabled.getBool());
	m_screenMaskNode->setActive(ActiveYN::Yes, RefreshesLayoutYN::Yes);

	m_isFirstUpdateSinceShown = true;
}

void ContextMenu::hide(RefreshesLayoutYN refreshesLayout)
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

void ContextMenu::update()
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

	if (!m_rootNode->isHoveredRecursive() && !m_rootNode->rect().mouseOver() && (MouseL.down() || MouseM.down() || MouseR.down()))
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
bool ContextMenu::isHoveredRecursive() const
{
	return m_rootNode->isHoveredRecursive();
}

ContextMenuOpener::ContextMenuOpener(const std::shared_ptr<ContextMenu>& contextMenu, Array<MenuElement> menuElements, std::function<void()> fnBeforeOpen, RecursiveYN recursive)
	: ComponentBase{ {} }
	, m_contextMenu{ contextMenu }
	, m_menuElements{ std::move(menuElements) }
	, m_fnBeforeOpen{ std::move(fnBeforeOpen) }
	, m_recursive{ recursive }
{
}

void ContextMenuOpener::update(const std::shared_ptr<Node>& node)
{
	if (m_recursive ? node->isRightClickedRecursive() : node->isRightClicked())
	{
		openManually();
	}
}

void ContextMenuOpener::draw(const Node&) const
{
}

void ContextMenuOpener::openManually(const Vec2& pos) const
{
	if (m_fnBeforeOpen)
	{
		m_fnBeforeOpen();
	}
	m_contextMenu->show(pos, m_menuElements);
}

MenuBar::MenuBar(const std::shared_ptr<Canvas>& editorCanvas, const std::shared_ptr<ContextMenu>& contextMenu)
	: m_editorCanvas(editorCanvas)
	, m_menuBarRootNode(editorCanvas->rootNode()->emplaceChild(
		U"MenuBar",
		AnchorConstraint
		{
			.anchorMin = Anchor::TopLeft,
			.anchorMax = Anchor::TopRight,
			.posDelta = Vec2{ 0, 0 },
			.sizeDelta = Vec2{ 0, MenuBarHeight },
			.sizeDeltaPivot = Anchor::TopLeft,
		}))
	, m_contextMenu(contextMenu)
{
	m_menuBarRootNode->setBoxChildrenLayout(HorizontalLayout{});
	m_menuBarRootNode->emplaceComponent<RectRenderer>(ColorF{ 0.95 });
}

void MenuBar::addMenuCategory(StringView name, StringView text, const Input& mnemonicInput, const Array<MenuElement>& elements, int32 width, int32 subMenuWidth)
{
	const auto node = m_menuBarRootNode->emplaceChild(
		name,
		BoxConstraint
		{
			.sizeRatio = Vec2{ 0, 1 },
			.sizeDelta = Vec2{ width, 0 },
		});
	node->emplaceComponent<RectRenderer>(MenuItemRectFillColor());
	const String labelText = U"{}({})"_fmt(text, mnemonicInput.name());
	node->emplaceComponent<Label>(labelText, U"", 14, PropertyValue<ColorF>{ ColorF{ 0.0 } }.withDisabled(ColorF{ 0.0, 0.5 }), HorizontalAlign::Center, VerticalAlign::Middle);
	node->addClickHotKey(mnemonicInput, CtrlYN::No, AltYN::Yes, ShiftYN::No, EnabledWhileTextEditingYN::Yes);

	m_menuCategories.push_back(MenuCategory
	{
		.elements = elements,
		.node = node,
		.subMenuWidth = subMenuWidth,
	});
}

void MenuBar::update()
{
	bool hasMenuOpened = false;
	for (const auto& menuCategory : m_menuCategories)
	{
		if (menuCategory.node->isMouseDown() || menuCategory.node->isClickRequested())
		{
			if (m_activeMenuCategoryNode == menuCategory.node)
			{
				// 同じメニューが再度クリックされた場合は非表示
				m_contextMenu->hide();
			}
			else
			{
				// メニューがクリックされた場合は表示を切り替え
				m_contextMenu->show(menuCategory.node->rect().bl(), menuCategory.elements, menuCategory.subMenuWidth, ScreenMaskEnabledYN::No, [this] { m_hasMenuClosed = true; });
				m_activeMenuCategoryNode = menuCategory.node;
				hasMenuOpened = true;
			}
		}
		else if (menuCategory.node->isHoveredRecursive() && m_activeMenuCategoryNode && m_activeMenuCategoryNode != menuCategory.node)
		{
			// カーソルが他のメニューに移動した場合はサブメニューを切り替える
			m_contextMenu->show(menuCategory.node->rect().bl(), menuCategory.elements, menuCategory.subMenuWidth, ScreenMaskEnabledYN::No, [this] { m_hasMenuClosed = true; });
			m_activeMenuCategoryNode = menuCategory.node;
			hasMenuOpened = true;
		}
	}

	if (m_hasMenuClosed && !hasMenuOpened)
	{
		m_activeMenuCategoryNode = nullptr;
	}
	m_hasMenuClosed = false;
}