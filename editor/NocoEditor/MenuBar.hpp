#pragma once
#include <NocoUI.hpp>
#include "ContextMenu.hpp"

namespace noco::editor
{
	constexpr int32 MenuBarHeight = 26;

	struct MenuCategory
	{
		Array<MenuElement> elements;
		std::shared_ptr<Node> node;
		int32 subMenuWidth;
	};

	class MenuBar
	{
	private:
		static constexpr int32 DefaultSubMenuWidth = 300;

		std::shared_ptr<Canvas> m_editorCanvas;
		std::shared_ptr<Node> m_menuBarRootNode;
		Array<MenuCategory> m_menuCategories;
		std::shared_ptr<ContextMenu> m_contextMenu;
		std::shared_ptr<Node> m_activeMenuCategoryNode;
		bool m_hasMenuClosed = false;

	public:
		explicit MenuBar(const std::shared_ptr<Canvas>& editorCanvas, const std::shared_ptr<ContextMenu>& contextMenu)
			: m_editorCanvas(editorCanvas)
			, m_menuBarRootNode(editorCanvas->rootNode()->emplaceChild(
				U"MenuBar",
				AnchorRegion
				{
					.anchorMin = Anchor::TopLeft,
					.anchorMax = Anchor::TopRight,
					.posDelta = Vec2{ 0, 0 },
					.sizeDelta = Vec2{ 0, MenuBarHeight },
					.sizeDeltaPivot = Anchor::TopLeft,
				}))
			, m_contextMenu(contextMenu)
		{
			m_menuBarRootNode->setChildrenLayout(HorizontalLayout{});
			m_menuBarRootNode->emplaceComponent<RectRenderer>(ColorF{ 0.95 });
		}

		void addMenuCategory(StringView name, StringView text, const Input& mnemonicInput, const Array<MenuElement>& elements, int32 width = 80, int32 subMenuWidth = DefaultSubMenuWidth)
		{
			const auto node = m_menuBarRootNode->emplaceChild(
				name,
				InlineRegion
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

		void update()
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
						m_contextMenu->show(menuCategory.node->regionRect().bl(), menuCategory.elements, menuCategory.subMenuWidth, ScreenMaskEnabledYN::No, [this] { m_hasMenuClosed = true; });
						m_activeMenuCategoryNode = menuCategory.node;
						hasMenuOpened = true;
					}
				}
				else if (menuCategory.node->isHovered(RecursiveYN::Yes) && m_activeMenuCategoryNode && m_activeMenuCategoryNode != menuCategory.node)
				{
					// カーソルが他のメニューに移動した場合はサブメニューを切り替える
					m_contextMenu->show(menuCategory.node->regionRect().bl(), menuCategory.elements, menuCategory.subMenuWidth, ScreenMaskEnabledYN::No, [this] { m_hasMenuClosed = true; });
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
	};
}
