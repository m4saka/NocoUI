#include <Siv3D.hpp>
#include "NocoUI.hpp"
#include "HistorySystem.hpp"

using namespace noco;
using noco::detail::IncludesInternalIdYN;

using CheckedYN = YesNo<struct CheckedYN_tag>;
using ScreenMaskEnabledYN = YesNo<struct ScreenMaskEnabledYN_tag>;
using PreserveScrollYN = YesNo<struct PreserveScrollYN_tag>;
using HasInteractivePropertyValueYN = YesNo<struct HasInteractivePropertyValueYN_tag>;
using IsFoldedYN = YesNo<struct IsFoldedYN_tag>;
using AppendsMnemonicKeyTextYN = YesNo<struct AppendsMnemonicKeyText_tag>;
using IsDefaultButtonYN = YesNo<struct IsDefaultButtonYN_tag>;
using IsCancelButtonYN = YesNo<struct IsCancelButtonYN_tag>;

constexpr int32 MenuBarHeight = 26;

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

	void show(const Vec2& pos, const Array<MenuElement>& elements, int32 menuItemWidth = DefaultMenuItemWidth, ScreenMaskEnabledYN screenMaskEnabled = ScreenMaskEnabledYN::Yes, std::function<void()> fnOnHide = nullptr)
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

		if (!m_rootNode->isHovered(RecursiveYN::Yes) && !m_rootNode->rect().mouseOver() && (MouseL.down() || MouseM.down() || MouseR.down()))
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

class TooltipOpener : public ComponentBase
{
private:
	std::shared_ptr<Canvas> m_overlayCanvas;
	std::shared_ptr<Node> m_tooltipNode;
	String m_tooltipText;
	String m_tooltipDetailText;
	double m_hoverTime = 0.0;
	bool m_isShowing = false;
	static constexpr double ShowDelay = 0.5; // ツールチップが表示されるまでの遅延時間(秒)

	void createTooltip()
	{
		if (!m_tooltipText.empty() && !m_tooltipNode)
		{
			// ツールチップノードを生成
			m_tooltipNode = m_overlayCanvas->rootNode()->emplaceChild(
				U"Tooltip",
				AnchorConstraint
				{
					.anchorMin = Anchor::TopLeft,
					.anchorMax = Anchor::TopLeft,
					.posDelta = Vec2{ 0, 0 },
					.sizeDelta = Vec2{ 0, 0 }, // サイズは後で調整
					.sizeDeltaPivot = Anchor::TopLeft,
				},
				IsHitTargetYN::No);
			m_tooltipNode->emplaceComponent<RectRenderer>(ColorF{ 0.1, 0.9 }, ColorF{ 0.3 }, 1.0, 4.0);
			m_tooltipNode->setBoxChildrenLayout(
				VerticalLayout
				{
					.padding = LRTB{ 10, 10, 5, 5 },
					.spacing = 5,
					.horizontalAlign = HorizontalAlign::Left,
				});
			
			// メインテキスト用ノード
			const auto mainTextNode = m_tooltipNode->emplaceChild(
				U"MainText",
				BoxConstraint
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
					BoxConstraint
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

	void destroyTooltip()
	{
		if (m_tooltipNode)
		{
			m_overlayCanvas->rootNode()->removeChild(m_tooltipNode);
			m_tooltipNode = nullptr;
		}
	}

	void updateTooltipSize()
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
				if (const auto* pBoxConstraint = child->boxConstraint())
				{
					auto newConstraint = *pBoxConstraint;
					newConstraint.sizeDelta = contentSize;
					child->setConstraint(newConstraint, RefreshesLayoutYN::No);
				}
			}
		}
		
		// ツールチップ全体のサイズを更新
		if (const auto pConstraint = m_tooltipNode->anchorConstraint())
		{
			const Vec2 newSize = m_tooltipNode->getFittingSizeToChildren();
			if (pConstraint->sizeDelta != newSize)
			{
				AnchorConstraint newConstraint = *pConstraint;
				newConstraint.sizeDelta = newSize;
				m_tooltipNode->setConstraint(newConstraint);
			}
		}
	}

public:
	explicit TooltipOpener(const std::shared_ptr<Canvas>& overlayCanvas, StringView tooltipText, StringView tooltipDetailText = U"")
		: ComponentBase{ {} }
		, m_overlayCanvas{ overlayCanvas }
		, m_tooltipText{ tooltipText }
		, m_tooltipDetailText{ tooltipDetailText }
	{
	}

	~TooltipOpener()
	{
		destroyTooltip();
	}

	void updateInput(const std::shared_ptr<Node>& node) override
	{
		// インプットがブロックされている場合やクリックされた瞬間は何もしない
		if (CurrentFrame::HasInputBlocked() || MouseL.down() || MouseM.down() || MouseR.down())
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
				if (const auto pConstraint = m_tooltipNode->anchorConstraint())
				{
					Vec2 newPos = Cursor::Pos() + Vec2{ 0, 20 };
					const Vec2 tooltipSize = pConstraint->sizeDelta;
					
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
					
					if (pConstraint->posDelta != newPos)
					{
						AnchorConstraint newConstraint = *pConstraint;
						newConstraint.posDelta = newPos;
						m_tooltipNode->setConstraint(newConstraint);
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

	void setTooltipText(StringView text, StringView detailText = U"")
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
};

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

	void addMenuCategory(StringView name, StringView text, const Input& mnemonicInput, const Array<MenuElement>& elements, int32 width = 80, int32 subMenuWidth = DefaultSubMenuWidth)
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
					m_contextMenu->show(menuCategory.node->rect().bl(), menuCategory.elements, menuCategory.subMenuWidth, ScreenMaskEnabledYN::No, [this] { m_hasMenuClosed = true; });
					m_activeMenuCategoryNode = menuCategory.node;
					hasMenuOpened = true;
				}
			}
			else if (menuCategory.node->isHovered(RecursiveYN::Yes) && m_activeMenuCategoryNode && m_activeMenuCategoryNode != menuCategory.node)
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
};

class Toolbar
{
public:
	static constexpr int32 ToolbarHeight = 32;
	
private:
	static constexpr int32 ButtonSize = 28;
	static constexpr int32 ButtonMargin = 4;
	static constexpr int32 BorderLineThickness = 2;

	std::shared_ptr<Canvas> m_editorCanvas;
	std::shared_ptr<Canvas> m_editorOverlayCanvas;
	std::shared_ptr<Node> m_toolbarRootNode;
	Font m_iconFont{ FontMethod::MSDF, 18, Typeface::Icon_MaterialDesign };
	
	// ボタンノードを管理するためのマップ
	std::unordered_map<String, std::shared_ptr<Node>> m_buttonNodes;
	
	// ボタンの有効/無効を判定する関数
	struct ButtonInfo
	{
		std::shared_ptr<Node> node;
		std::function<bool()> enableCondition;
	};
	std::unordered_map<String, ButtonInfo> m_buttons;

public:
	explicit Toolbar(const std::shared_ptr<Canvas>& editorCanvas, const std::shared_ptr<Canvas>& editorOverlayCanvas)
		: m_editorCanvas(editorCanvas)
		, m_editorOverlayCanvas(editorOverlayCanvas)
		, m_toolbarRootNode(editorCanvas->rootNode()->emplaceChild(
			U"Toolbar",
			AnchorConstraint
			{
				.anchorMin = Anchor::TopLeft,
				.anchorMax = Anchor::TopRight,
				.posDelta = Vec2{ 0, MenuBarHeight },
				.sizeDelta = Vec2{ 0, ToolbarHeight },
				.sizeDeltaPivot = Anchor::TopLeft,
			}))
	{
		m_toolbarRootNode->setBoxChildrenLayout(
			HorizontalLayout
			{
				.padding = LRTB{ .left = ButtonMargin, .top = BorderLineThickness },
				.spacing = ButtonMargin,
				.verticalAlign = VerticalAlign::Middle,
			});
		m_toolbarRootNode->emplaceComponent<RectRenderer>(ColorF{ 0.95 });
		
		// MenuBarとの境界線を追加
		m_toolbarRootNode->emplaceChild(
			U"BorderLine",
			AnchorConstraint
			{
				.anchorMin = Anchor::TopLeft,
				.anchorMax = Anchor::TopRight,
				.posDelta = Vec2{ 0, 0 },
				.sizeDelta = Vec2{ 0, BorderLineThickness },
				.sizeDeltaPivot = Anchor::TopLeft,
			})
			->emplaceComponent<RectRenderer>(ColorF{ 0.8 });
	}

	std::shared_ptr<Node> addButton(StringView name, StringView icon, StringView tooltip, std::function<void()> onClick, std::function<bool()> enableCondition = nullptr)
	{
		auto buttonNode = m_toolbarRootNode->emplaceChild(
			name,
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0, 0 },
				.sizeDelta = Vec2{ ButtonSize, ButtonSize },
			});
		
		// ボタンの背景
		buttonNode->emplaceComponent<RectRenderer>(
			PropertyValue<ColorF>{ ColorF{ 0.95, 0.0 }, ColorF{ 0.88 }, ColorF{ 0.83 }, ColorF{ 0.95, 0.0 }, 0.1 },
			PropertyValue<ColorF>{ ColorF{ 0.0, 0.0 }, ColorF{ 0.4 }, ColorF{ 0.4 }, ColorF{ 0.0, 0.0 }, 0.1 },
			0.0,
			4.0);
		
		// アイコンラベル
		const auto iconLabel = buttonNode->emplaceComponent<Label>(
			icon,
			U"",
			18,
			PropertyValue<ColorF>{ ColorF{ 0.2 } }.withDisabled(ColorF{ 0.2, 0.5 }),
			HorizontalAlign::Center,
			VerticalAlign::Middle);
		iconLabel->setFont(m_iconFont);
		
		// クリック時の処理
		buttonNode->addOnClick([onClick = std::move(onClick)](const std::shared_ptr<Node>&)
		{
			if (onClick)
			{
				onClick();
			}
		});
		
		// ツールチップ
		if (!tooltip.empty())
		{
			buttonNode->emplaceComponent<TooltipOpener>(m_editorOverlayCanvas, tooltip);
		}
		
		m_buttonNodes[String{ name }] = buttonNode;
		m_buttons[String{ name }] = ButtonInfo{ buttonNode, enableCondition };
		if (enableCondition)
		{
			buttonNode->setInteractable(enableCondition());
		}

		return buttonNode;
	}

	void addSeparator()
	{
		m_toolbarRootNode->emplaceChild(
			U"Separator",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0, 0.6 },
				.sizeDelta = Vec2{ 1, 0 },
			})
			->emplaceComponent<RectRenderer>(ColorF{ 0.7 });
	}
	
	void updateButtonStates()
	{
		// 各ボタンの有効/無効状態を更新
		for (auto& [name, buttonInfo] : m_buttons)
		{
			if (buttonInfo.enableCondition)
			{
				// 判定関数がある場合は、その結果に基づいて有効/無効を設定
				buttonInfo.node->setInteractable(buttonInfo.enableCondition());
			}
		}
	}

};

// 前方宣言
class Inspector;

// プロパティのメタデータを管理するための構造
struct PropertyKey
{
	String componentName;
	String propertyName;

	bool operator==(const PropertyKey& other) const
	{
		return componentName == other.componentName && propertyName == other.propertyName;
	}
};

template <>
struct std::hash<PropertyKey>
{
	size_t operator()(const PropertyKey& key) const
	{
		return std::hash<String>{}(key.componentName) ^ (std::hash<String>{}(key.propertyName) << 1);
	}
};

// PropertyValueのいずれかの状態が指定された値と一致するかをチェックする関数
template <typename T>
[[nodiscard]]
static bool HasAnyStateEqualTo(const PropertyValue<T>& propertyValue, const T& targetValue)
{
	return propertyValue.value(InteractionState::Default, SelectedYN::No) == targetValue ||
	       propertyValue.value(InteractionState::Default, SelectedYN::Yes) == targetValue ||
	       propertyValue.value(InteractionState::Hovered, SelectedYN::No) == targetValue ||
	       propertyValue.value(InteractionState::Hovered, SelectedYN::Yes) == targetValue ||
	       propertyValue.value(InteractionState::Pressed, SelectedYN::No) == targetValue ||
	       propertyValue.value(InteractionState::Pressed, SelectedYN::Yes) == targetValue ||
	       propertyValue.value(InteractionState::Disabled, SelectedYN::No) == targetValue ||
	       propertyValue.value(InteractionState::Disabled, SelectedYN::Yes) == targetValue;
}

// PropertyValueのいずれかの状態でtrueかをチェックする便利関数
[[nodiscard]]
static bool HasAnyTrueState(const PropertyValue<bool>& propertyValue)
{
	return HasAnyStateEqualTo(propertyValue, true);
}

// PropertyValueのいずれかの状態でfalseかをチェックする便利関数
[[nodiscard]]
static bool HasAnyFalseState(const PropertyValue<bool>& propertyValue)
{
	return HasAnyStateEqualTo(propertyValue, false);
}

// プロパティのメタデータ
struct PropertyMetadata
{
	Optional<String> tooltip;
	Optional<String> tooltipDetail;  // 詳細な説明文
	std::function<bool(const ComponentBase&)> visibilityCondition;  // 表示条件
	bool refreshInspectorOnChange = false;  // 変更時にInspectorを更新するかどうか
	Optional<int32> numTextAreaLines;  // テキストエリアとして表示する場合の行数(未設定の場合はテキストボックス)
	bool refreshesEveryFrame = false;  // 毎フレームInspectorの値の更新が必要かどうか(テキストボックスなどユーザー編集を伴うコンポーネントで使用。現状コンポーネントのStringプロパティのみ対応)
};

// プロパティの可視情報
struct PropertyVisibilityData
{
	bool isVisibleByCondition = true;
};

// プロパティのメタデータを初期化する関数
[[nodiscard]]
static HashTable<PropertyKey, PropertyMetadata> InitPropertyMetadata()
{
	HashTable<PropertyKey, PropertyMetadata> metadata;
	
	// 9スライス関連プロパティの表示条件を作成
	auto nineSliceVisibilityCondition = [](const ComponentBase& component) -> bool
	{
		if (const auto* sprite = dynamic_cast<const Sprite*>(&component))
		{
			return HasAnyTrueState(sprite->nineSliceEnabled());
		}
		return true;
	};
	
	// Nodeのプロパティ
	metadata[PropertyKey{ U"Node", U"activeSelf" }] = PropertyMetadata{
		.tooltip = U"Nodeの有効/無効",
		.tooltipDetail = U"このNodeとその子要素の表示を制御します\n無効の場合、updateの代わりにupdateInactiveが実行され、drawは実行されません",
	};
	metadata[PropertyKey{ U"Node", U"isHitTarget" }] = PropertyMetadata{
		.tooltip = U"ヒットテストの対象にするどうか",
		.tooltipDetail = U"無効にすると、この要素はヒットテスト(要素にマウスカーソルがホバーしているかどうかの判定)の対象外となり、親要素のInteractionStateを受け継ぎます\n※無効の場合、ヒットテストでは要素の存在自体が無視されるため、背面にある要素にホバーが可能となります\n※無効の場合、TextBox等のマウス操作を利用するコンポーネントも入力を受け付けなくなります",
	};
	metadata[PropertyKey{ U"Node", U"hitTestPadding" }] = PropertyMetadata{
		.tooltip = U"ヒットテスト領域の拡縮 (左、右、上、下)",
		.tooltipDetail = U"ヒットテスト(要素にマウスカーソルがホバーしているかどうかの判定)に使用する領域を、指定されたピクセル数だけ拡大・縮小します\n正の値で領域を拡大、負の値で領域を縮小します\n実際の見た目よりもずれた位置にマウスカーソルがあっても反応させたい場合に使用できます",
	};
	metadata[PropertyKey{ U"Node", U"inheritsChildrenHoveredState" }] = PropertyMetadata{
		.tooltip = U"子要素のホバー状態(Hovered)を継承するかどうか",
		.tooltipDetail = U"有効にすると、子要素のInteractionStateがHoveredの場合に、このNodeのInteractionStateがHoveredになります\n※このNodeのInteractionStateがPressed・Disabledの場合は影響を受けません",
	};
	metadata[PropertyKey{ U"Node", U"inheritsChildrenPressedState" }] = PropertyMetadata{
		.tooltip = U"子要素の押下状態(Pressed)を継承するかどうか",
		.tooltipDetail = U"有効にすると、子要素のInteractionStateがPressedの場合に、このNodeのInteractionStateがPressedになります\n※このNodeのInteractionStateがDisabledの場合は影響を受けません",
	};
	metadata[PropertyKey{ U"Node", U"interactable" }] = PropertyMetadata{
		.tooltip = U"インタラクション可能かどうか",
		.tooltipDetail = U"無効にすると、InteractionStateがDisabledになり、マウスホバーやクリックイベントが無効になります\n※interactableを無効にしても、updateやdrawは実行されます",
	};
	metadata[PropertyKey{ U"Node", U"horizontalScrollable" }] = PropertyMetadata{
		.tooltip = U"水平方向のスクロール可能",
	};
	metadata[PropertyKey{ U"Node", U"verticalScrollable" }] = PropertyMetadata{
		.tooltip = U"垂直方向のスクロール可能",
	};
	metadata[PropertyKey{ U"Node", U"wheelScrollEnabled" }] = PropertyMetadata{
		.tooltip = U"ホイールスクロールの有効/無効",
		.tooltipDetail = U"有効にすると、マウスホイールでスクロールできます",
	};
	metadata[PropertyKey{ U"Node", U"dragScrollEnabled" }] = PropertyMetadata{
		.tooltip = U"ドラッグスクロールの有効/無効",
		.tooltipDetail = U"有効にすると、ドラッグ操作でスクロールできます",
	};
	metadata[PropertyKey{ U"Node", U"decelerationRate" }] = PropertyMetadata{
		.tooltip = U"慣性スクロールの減衰率",
		.tooltipDetail = U"1秒あたりの速度減衰率(0.0~1.0)。値が小さいほど早く停止します",
	};
	metadata[PropertyKey{ U"Node", U"rubberBandScrollEnabled" }] = PropertyMetadata{
		.tooltip = U"ラバーバンドスクロールの有効/無効",
		.tooltipDetail = U"有効にすると、スクロール範囲外でも一時的にドラッグでき、離すと自動的に範囲内に戻ります",
	};
	metadata[PropertyKey{ U"Node", U"clippingEnabled" }] = PropertyMetadata{
		.tooltip = U"クリッピングの有効/無効",
		.tooltipDetail = U"有効にすると、コンポーネントや子要素の描画内容が要素の矩形範囲で切り取られます",
	};
	
	// Constraint関連 - AnchorConstraint
	metadata[PropertyKey{ U"AnchorConstraint", U"type" }] = PropertyMetadata{
		.tooltip = U"Constraintの種類",
		.tooltipDetail = U"親要素に対する位置とサイズの決め方の種類を指定します\nAnchorConstraint: 親要素の四辺を基に比率と差分値で四辺の位置を決定します\n　※AnchorConstraintの要素は親要素のboxChildrenLayoutの影響を受けません\nBoxConstraint: 親要素のboxChildrenLayoutで指定されたレイアウト方法に応じて、順番に配置されます",
	};
	metadata[PropertyKey{ U"AnchorConstraint", U"anchor" }] = PropertyMetadata{
		.tooltip = U"アンカー位置",
	};
	metadata[PropertyKey{ U"AnchorConstraint", U"anchorMin" }] = PropertyMetadata{
		.tooltip = U"最小アンカー位置 (0,0)が左上、(1,1)が右下",
	};
	metadata[PropertyKey{ U"AnchorConstraint", U"anchorMax" }] = PropertyMetadata{
		.tooltip = U"最大アンカー位置 (0,0)が左上、(1,1)が右下",
	};
	metadata[PropertyKey{ U"AnchorConstraint", U"posDelta" }] = PropertyMetadata{
		.tooltip = U"位置 (アンカーからの相対位置)",
	};
	metadata[PropertyKey{ U"AnchorConstraint", U"sizeDelta" }] = PropertyMetadata{
		.tooltip = U"サイズ (差分値)",
		.tooltipDetail = U"要素の大きさをピクセル数で指定します。アンカーを基に計算された領域サイズにこのサイズが加算されます",
	};
	metadata[PropertyKey{ U"AnchorConstraint", U"sizeDeltaPivot" }] = PropertyMetadata{
		.tooltip = U"サイズ計算の起点 (X、Y)",
	};
	
	// Constraint関連 - BoxConstraint
	metadata[PropertyKey{ U"BoxConstraint", U"type" }] = PropertyMetadata{
		.tooltip = U"Constraintの種類",
		.tooltipDetail = U"親要素に対する位置とサイズの決め方の種類を指定します\nAnchorConstraint: 親要素の四辺を基に比率と差分値で四辺の位置を決定します\n　※AnchorConstraintの要素は親要素のboxChildrenLayoutの影響を受けません\nBoxConstraint: 親要素のboxChildrenLayoutで指定されたレイアウト方法に応じて、順番に配置されます",
	};
	metadata[PropertyKey{ U"BoxConstraint", U"margin" }] = PropertyMetadata{
		.tooltip = U"マージン (左、右、上、下)",
		.tooltipDetail = U"要素の外側の余白を指定します\n※全ての子要素間で共通の間隔を設定したい場合は、こちらではなく親要素のboxChildrenLayoutに対してspacingの値を指定してください",
	};
	metadata[PropertyKey{ U"BoxConstraint", U"sizeRatio" }] = PropertyMetadata{
		.tooltip = U"親要素に対するサイズ比率 (0.0～1.0)",
		.tooltipDetail = U"親要素のサイズに対する比率を指定します。0.0は親要素のサイズを無視し、1.0は親要素のサイズと同じになります\n※要素間で自動的にサイズを分配する必要がある場合、sizeRatioではなくflexibleWeightを使用してください",
	};
	metadata[PropertyKey{ U"BoxConstraint", U"sizeDelta" }] = PropertyMetadata{
		.tooltip = U"サイズ (差分値)",
		.tooltipDetail = U"要素の大きさをピクセル数で指定します。sizeRatioおよびflexibleWeightと併用した場合、このサイズが差分値として加算されます",
	};
	metadata[PropertyKey{ U"BoxConstraint", U"flexibleWeight" }] = PropertyMetadata{
		.tooltip = U"フレキシブル要素の伸縮の重み",
		.tooltipDetail = U"0以外の値を設定すると、余った領域を重みの比率に応じて他のフレキシブル要素と分け合います\n(FlowLayoutとHorizontalLayoutでは横方向、VerticalLayoutでは縦方向の領域を分け合います)\n※例1: 全てのフレキシブル要素に1を指定すると、余った領域を均等に分配します\n※例2: ある要素に2、それ以外の全ての要素に1を指定すると、2を指定した要素は他の要素の2倍の領域が割り当てられます",
	};
	
	// AnchorPreset用プロパティ
	metadata[PropertyKey{ U"AnchorConstraint", U"top" }] = PropertyMetadata{
		.tooltip = U"親要素の上端からの距離",
	};
	metadata[PropertyKey{ U"AnchorConstraint", U"left" }] = PropertyMetadata{
		.tooltip = U"親要素の左端からの距離",
	};
	metadata[PropertyKey{ U"AnchorConstraint", U"right" }] = PropertyMetadata{
		.tooltip = U"親要素の右端からの距離",
	};
	metadata[PropertyKey{ U"AnchorConstraint", U"bottom" }] = PropertyMetadata{
		.tooltip = U"親要素の下端からの距離",
	};
	metadata[PropertyKey{ U"AnchorConstraint", U"size" }] = PropertyMetadata{
		.tooltip = U"サイズ (幅、高さ)",
	};
	metadata[PropertyKey{ U"AnchorConstraint", U"width" }] = PropertyMetadata{
		.tooltip = U"幅",
	};
	metadata[PropertyKey{ U"AnchorConstraint", U"height" }] = PropertyMetadata{
		.tooltip = U"高さ",
	};
	metadata[PropertyKey{ U"AnchorConstraint", U"xDelta" }] = PropertyMetadata{
		.tooltip = U"X軸の位置",
	};
	metadata[PropertyKey{ U"AnchorConstraint", U"yDelta" }] = PropertyMetadata{
		.tooltip = U"Y軸の位置",
	};
	
	// Layout関連
	metadata[PropertyKey{ U"FlowLayout", U"type" }] = PropertyMetadata{
		.tooltip = U"レイアウトの種類",
		.tooltipDetail = U"FlowLayout: 子要素を左から右へ並べ、右端で折り返します\nHorizontalLayout: 子要素を水平方向に並べます\nVerticalLayout: 子要素を垂直方向に並べます\n※boxChildrenLayoutはBoxConstraintが指定された子要素のみに影響します。AnchorConstraintを持つ子要素に対しては影響しません",
	};
	metadata[PropertyKey{ U"FlowLayout", U"padding" }] = PropertyMetadata{
		.tooltip = U"内側の余白 (左、右、上、下)",
	};
	metadata[PropertyKey{ U"FlowLayout", U"spacing" }] = PropertyMetadata{
		.tooltip = U"子要素同士の間隔 (X、Y)",
		.tooltipDetail = U"子要素同士の間隔を指定します\n全ての子要素に共通の間隔を指定したい場合に使用します\n※子要素のBoxConstraintのmarginにも値が設定されている場合、spacingとmarginの合計値が子要素間の間隔として適用されます",
	};
	metadata[PropertyKey{ U"FlowLayout", U"horizontalAlign" }] = PropertyMetadata{
		.tooltip = U"水平方向の配置",
	};
	metadata[PropertyKey{ U"FlowLayout", U"verticalAlign" }] = PropertyMetadata{
		.tooltip = U"垂直方向の配置",
	};
	
	metadata[PropertyKey{ U"HorizontalLayout", U"type" }] = PropertyMetadata{
		.tooltip = U"レイアウトの種類",
		.tooltipDetail = U"FlowLayout: 子要素を左から右へ並べ、右端で折り返します\nHorizontalLayout: 子要素を水平方向に並べます\nVerticalLayout: 子要素を垂直方向に並べます\n※boxChildrenLayoutはBoxConstraintが指定された子要素のみに影響します。AnchorConstraintを持つ子要素に対しては影響しません",
	};
	metadata[PropertyKey{ U"HorizontalLayout", U"padding" }] = PropertyMetadata{
		.tooltip = U"内側の余白 (左、右、上、下)",
	};
	metadata[PropertyKey{ U"HorizontalLayout", U"spacing" }] = PropertyMetadata{
		.tooltip = U"子要素同士の間隔 (X、Y)",
		.tooltipDetail = U"子要素同士の間隔を指定します\n全ての子要素に共通の間隔を指定したい場合に使用します\n※子要素のBoxConstraintのmarginにも値が設定されている場合、spacingとmarginの合計値が子要素間の間隔として適用されます",
	};
	metadata[PropertyKey{ U"HorizontalLayout", U"horizontalAlign" }] = PropertyMetadata{
		.tooltip = U"水平方向の配置",
	};
	metadata[PropertyKey{ U"HorizontalLayout", U"verticalAlign" }] = PropertyMetadata{
		.tooltip = U"垂直方向の配置",
	};
	
	metadata[PropertyKey{ U"VerticalLayout", U"type" }] = PropertyMetadata{
		.tooltip = U"レイアウトの種類",
		.tooltipDetail = U"FlowLayout: 子要素を左から右へ並べ、右端で折り返します\nHorizontalLayout: 子要素を水平方向に並べます\nVerticalLayout: 子要素を垂直方向に並べます\n※boxChildrenLayoutはBoxConstraintが指定された子要素のみに影響します。AnchorConstraintを持つ子要素に対しては影響しません",
	};
	metadata[PropertyKey{ U"VerticalLayout", U"padding" }] = PropertyMetadata{
		.tooltip = U"内側の余白 (左、右、上、下)",
	};
	metadata[PropertyKey{ U"VerticalLayout", U"spacing" }] = PropertyMetadata{
		.tooltip = U"子要素同士の間隔 (X、Y)",
		.tooltipDetail = U"子要素同士の間隔を指定します\n全ての子要素に共通の間隔を指定したい場合に使用します\n※子要素のBoxConstraintのmarginにも値が設定されている場合、spacingとmarginの合計値が子要素間の間隔として適用されます",
	};
	metadata[PropertyKey{ U"VerticalLayout", U"horizontalAlign" }] = PropertyMetadata{
		.tooltip = U"水平方向の配置",
	};
	metadata[PropertyKey{ U"VerticalLayout", U"verticalAlign" }] = PropertyMetadata{
		.tooltip = U"垂直方向の配置",
	};
	
	// TransformEffect関連
	metadata[PropertyKey{ U"TransformEffect", U"position" }] = PropertyMetadata{
		.tooltip = U"位置",
		.tooltipDetail = U"要素の位置を移動させます\nこの値による位置変更はレイアウト計算に影響を与えません\n※TransformEffectはレイアウトの再計算を必要としないため、要素の位置を高速に変更できます。そのため、アニメーション等の用途で利用できます\n※マウスカーソルのホバー判定には移動後の位置が利用されます",
	};
	metadata[PropertyKey{ U"TransformEffect", U"scale" }] = PropertyMetadata{
		.tooltip = U"スケール",
		.tooltipDetail = U"要素のサイズを拡大・縮小するスケールを指定します\nこの値による拡大縮小はレイアウト計算に影響を与えません\n※TransformEffectはレイアウトの再計算を必要としないため、要素の大きさを高速に変更できます。そのため、アニメーション等の用途で利用できます\n※描画内容はスケールに応じて伸縮されます\n※マウスカーソルのホバー判定には拡大縮小後のサイズが利用されます",
	};
	metadata[PropertyKey{ U"TransformEffect", U"pivot" }] = PropertyMetadata{
		.tooltip = U"基準点 (X、Y)",
		.tooltipDetail = U"scaleによる拡大縮小の基準点となる位置を0～1の比率で指定します\n(0,0)は左上、(1,1)は右下を表します",
	};
	
	// Componentのプロパティ
	// RectRenderer
	metadata[PropertyKey{ U"RectRenderer", U"fillGradationType" }] = PropertyMetadata{
		.tooltip = U"塗りつぶしグラデーションタイプ",
		.tooltipDetail = U"塗りつぶしのグラデーションタイプを選択します\nNone: 単色塗りつぶし\nTopBottom: 上下グラデーション\nLeftRight: 左右グラデーション",
		.refreshInspectorOnChange = true,
	};
	metadata[PropertyKey{ U"RectRenderer", U"fillColor" }] = PropertyMetadata{
		.tooltip = U"塗りつぶし色",
		.visibilityCondition = [](const ComponentBase& component)
		{
			if (const auto* rectRenderer = dynamic_cast<const RectRenderer*>(&component))
			{
				return HasAnyStateEqualTo(rectRenderer->fillGradationType(), RectFillGradationType::None);
			}
			return false;
		},
	};
	metadata[PropertyKey{ U"RectRenderer", U"fillGradationColor1" }] = PropertyMetadata{
		.tooltip = U"グラデーション色 1",
		.tooltipDetail = U"TopBottom: 上側の色\nLeftRight: 左側の色",
		.visibilityCondition = [](const ComponentBase& component)
		{
			if (const auto* rectRenderer = dynamic_cast<const RectRenderer*>(&component))
			{
				return !HasAnyStateEqualTo(rectRenderer->fillGradationType(), RectFillGradationType::None);
			}
			return false;
		},
	};
	metadata[PropertyKey{ U"RectRenderer", U"fillGradationColor2" }] = PropertyMetadata{
		.tooltip = U"グラデーション色 2",
		.tooltipDetail = U"TopBottom: 下側の色\nLeftRight: 右側の色",
		.visibilityCondition = [](const ComponentBase& component)
		{
			if (const auto* rectRenderer = dynamic_cast<const RectRenderer*>(&component))
			{
				return !HasAnyStateEqualTo(rectRenderer->fillGradationType(), RectFillGradationType::None);
			}
			return false;
		},
	};
	metadata[PropertyKey{ U"RectRenderer", U"outlineColor" }] = PropertyMetadata{
		.tooltip = U"アウトライン色",
	};
	metadata[PropertyKey{ U"RectRenderer", U"outlineThickness" }] = PropertyMetadata{
		.tooltip = U"アウトラインの太さ",
	};
	metadata[PropertyKey{ U"RectRenderer", U"cornerRadius" }] = PropertyMetadata{
		.tooltip = U"角の丸み半径",
	};
	metadata[PropertyKey{ U"RectRenderer", U"shadowColor" }] = PropertyMetadata{
		.tooltip = U"影の色",
	};
	metadata[PropertyKey{ U"RectRenderer", U"shadowOffset" }] = PropertyMetadata{
		.tooltip = U"影のオフセット (位置のずらし量)",
	};
	metadata[PropertyKey{ U"RectRenderer", U"shadowBlur" }] = PropertyMetadata{
		.tooltip = U"影のぼかし度合い",
	};
	metadata[PropertyKey{ U"RectRenderer", U"shadowSpread" }] = PropertyMetadata{
		.tooltip = U"影の拡散サイズ",
	};
	
	// Label
	metadata[PropertyKey{ U"Label", U"text" }] = PropertyMetadata{
		.tooltip = U"表示するテキスト",
		.numTextAreaLines = 3,
	};
	metadata[PropertyKey{ U"Label", U"fontAssetName" }] = PropertyMetadata{
		.tooltip = U"フォントアセット名",
		.tooltipDetail = U"指定されている場合、プログラム上ではこのキー名をもとに取得したFontAssetのフォントを使用します\n※プレビューには反映されません",
	};
	metadata[PropertyKey{ U"Label", U"fontSize" }] = PropertyMetadata{
		.tooltip = U"フォントサイズ",
	};
	metadata[PropertyKey{ U"Label", U"sizingMode" }] = PropertyMetadata{
		.tooltip = U"サイズに関するモード",
		.tooltipDetail = U"Fixed: 固定フォントサイズで描画します\nShrinkToFit: ノードサイズに収まるようフォントサイズを自動縮小します\n※ShrinkToFitはテキストやその他の値に変化が発生した時のフォントサイズの再計算にかかる負荷が高いため、自動縮小が不要な場合はなるべくFixedを指定することを推奨します",
		.refreshInspectorOnChange = true,
	};
	metadata[PropertyKey{ U"Label", U"minFontSize" }] = PropertyMetadata{
		.tooltip = U"最小フォントサイズ",
		.tooltipDetail = U"ShrinkToFit時の最小フォントサイズ",
		.visibilityCondition = [](const ComponentBase& component)
		{
			if (auto label = dynamic_cast<const Label*>(&component))
			{
				return label->sizingMode().defaultValue == LabelSizingMode::ShrinkToFit;
			}
			return false;
		},
	};
	metadata[PropertyKey{ U"Label", U"color" }] = PropertyMetadata{
		.tooltip = U"テキスト色",
	};
	metadata[PropertyKey{ U"Label", U"horizontalAlign" }] = PropertyMetadata{
		.tooltip = U"水平方向の配置",
	};
	metadata[PropertyKey{ U"Label", U"verticalAlign" }] = PropertyMetadata{
		.tooltip = U"垂直方向の配置",
	};
	metadata[PropertyKey{ U"Label", U"padding" }] = PropertyMetadata{
		.tooltip = U"内側の余白 (左、右、上、下)",
	};
	metadata[PropertyKey{ U"Label", U"horizontalOverflow" }] = PropertyMetadata{
		.tooltip = U"水平方向にはみ出す場合の処理",
		.tooltipDetail = U"Wrap: 自動的に折り返します\nOverflow: 右へはみ出して描画します",
	};
	metadata[PropertyKey{ U"Label", U"verticalOverflow" }] = PropertyMetadata{
		.tooltip = U"垂直方向にはみ出す場合の処理",
		.tooltipDetail = U"Clip: 領域をはみ出した文字は描画しません\nOverflow: 下へはみ出して描画します",
	};
	metadata[PropertyKey{ U"Label", U"characterSpacing" }] = PropertyMetadata{
		.tooltip = U"文字同士の間隔 (X, Y)",
	};
	metadata[PropertyKey{ U"Label", U"underlineStyle" }] = PropertyMetadata{
		.tooltip = U"下線のスタイル",
	};
	metadata[PropertyKey{ U"Label", U"underlineColor" }] = PropertyMetadata{
		.tooltip = U"下線の色",
	};
	metadata[PropertyKey{ U"Label", U"underlineThickness" }] = PropertyMetadata{
		.tooltip = U"下線の太さ",
	};
	
	// Sprite
	metadata[PropertyKey{ U"Sprite", U"textureFilePath" }] = PropertyMetadata{
		.tooltip = U"テクスチャファイルのパス",
		.tooltipDetail = U"textureAssetName使用時は、Editor上でのプレビュー用としてのみ使用されます",
	};
	metadata[PropertyKey{ U"Sprite", U"textureAssetName" }] = PropertyMetadata{
		.tooltip = U"TextureAssetのキー名 (任意)",
		.tooltipDetail = U"指定されている場合、プログラム上ではこのキー名をもとに取得したTextureAssetのテクスチャを使用します\n※プレビューには反映されません\n※これを使用しなくてもライブラリ側で内部的にファイルパスをもとにしたキー名でTextureAssetを使用するため、\n　パフォーマンス上の利点は特にありません。TextureAssetのキー名を手動で管理したい場合のみ使用してください",
	};
	metadata[PropertyKey{ U"Sprite", U"color" }] = PropertyMetadata{
		.tooltip = U"スプライトの色",
		.tooltipDetail = U"テクスチャの色に乗算されます\nアルファ値は透明度を制御します",
	};
	metadata[PropertyKey{ U"Sprite", U"preserveAspect" }] = PropertyMetadata{
		.tooltip = U"アスペクト比を保持",
		.tooltipDetail = U"有効にすると、テクスチャの縦横比を保持してノードの領域内に収まるように描画されます",
	};
	metadata[PropertyKey{ U"Sprite", U"nineSliceEnabled" }] = PropertyMetadata{
		.tooltip = U"9スライス機能を有効にするか",
		.tooltipDetail = U"画像を9つの領域に分割し、角を固定サイズで表示しながら辺と中央を伸縮させます",
		.refreshInspectorOnChange = true,
	};
	metadata[PropertyKey{ U"Sprite", U"nineSliceMargin" }] = PropertyMetadata{
		.tooltip = U"9スライスのマージン(素材の端からの距離)",
		.tooltipDetail = U"素材画像の端から何ピクセル内側で領域分割するかを指定します",
		.visibilityCondition = nineSliceVisibilityCondition,
	};
	metadata[PropertyKey{ U"Sprite", U"nineSliceScale" }] = PropertyMetadata{
		.tooltip = U"9スライスのスケール",
		.visibilityCondition = nineSliceVisibilityCondition,
	};
	metadata[PropertyKey{ U"Sprite", U"nineSliceCenterTiled" }] = PropertyMetadata{
		.tooltip = U"中央領域をタイル表示するか",
		.visibilityCondition = nineSliceVisibilityCondition,
	};
	metadata[PropertyKey{ U"Sprite", U"nineSliceTopTiled" }] = PropertyMetadata{
		.tooltip = U"上端領域をタイル表示するか",
		.visibilityCondition = nineSliceVisibilityCondition,
	};
	metadata[PropertyKey{ U"Sprite", U"nineSliceBottomTiled" }] = PropertyMetadata{
		.tooltip = U"下端領域をタイル表示するか",
		.visibilityCondition = nineSliceVisibilityCondition,
	};
	metadata[PropertyKey{ U"Sprite", U"nineSliceLeftTiled" }] = PropertyMetadata{
		.tooltip = U"左端領域をタイル表示するか",
		.visibilityCondition = nineSliceVisibilityCondition,
	};
	metadata[PropertyKey{ U"Sprite", U"nineSliceRightTiled" }] = PropertyMetadata{
		.tooltip = U"右端領域をタイル表示するか",
		.visibilityCondition = nineSliceVisibilityCondition,
	};
	metadata[PropertyKey{ U"Sprite", U"nineSliceFallback" }] = PropertyMetadata{
		.tooltip = U"要素が9スライスのマージンより小さい場合に通常描画にフォールバックするかどうか",
		.visibilityCondition = nineSliceVisibilityCondition,
	};
	
	// TextBox
	metadata[PropertyKey{ U"TextBox", U"fontAssetName" }] = PropertyMetadata{
		.tooltip = U"FontAssetのキー名 (任意)",
		.tooltipDetail = U"指定されている場合、プログラム上ではこのキー名をもとに取得したFontAssetのフォントを使用します\n※プレビューには反映されません",
	};
	metadata[PropertyKey{ U"TextBox", U"fontSize" }] = PropertyMetadata{
		.tooltip = U"フォントサイズ",
	};
	metadata[PropertyKey{ U"TextBox", U"color" }] = PropertyMetadata{
		.tooltip = U"テキスト色",
	};
	metadata[PropertyKey{ U"TextBox", U"horizontalPadding" }] = PropertyMetadata{
		.tooltip = U"水平方向の内側の余白 (左、右)",
	};
	metadata[PropertyKey{ U"TextBox", U"verticalPadding" }] = PropertyMetadata{
		.tooltip = U"垂直方向の内側の余白 (上、下)",
	};
	metadata[PropertyKey{ U"TextBox", U"cursorColor" }] = PropertyMetadata{
		.tooltip = U"カーソルの色",
	};
	metadata[PropertyKey{ U"TextBox", U"selectionColor" }] = PropertyMetadata{
		.tooltip = U"選択範囲の色",
	};
	metadata[PropertyKey{ U"TextBox", U"text" }] = PropertyMetadata{
		.numTextAreaLines = 3,
		.refreshesEveryFrame = true,
	};
	
	// TextArea
	metadata[PropertyKey{ U"TextArea", U"fontAssetName" }] = PropertyMetadata{
		.tooltip = U"FontAssetのキー名 (任意)",
		.tooltipDetail = U"指定されている場合、プログラム上ではこのキー名をもとに取得したFontAssetのフォントを使用します\n※プレビューには反映されません",
	};
	metadata[PropertyKey{ U"TextArea", U"fontSize" }] = PropertyMetadata{
		.tooltip = U"フォントサイズ",
	};
	metadata[PropertyKey{ U"TextArea", U"color" }] = PropertyMetadata{
		.tooltip = U"テキスト色",
	};
	metadata[PropertyKey{ U"TextArea", U"horizontalPadding" }] = PropertyMetadata{
		.tooltip = U"水平方向の内側の余白 (左、右)",
	};
	metadata[PropertyKey{ U"TextArea", U"verticalPadding" }] = PropertyMetadata{
		.tooltip = U"垂直方向の内側の余白 (上、下)",
	};
	metadata[PropertyKey{ U"TextArea", U"cursorColor" }] = PropertyMetadata{
		.tooltip = U"カーソルの色",
	};
	metadata[PropertyKey{ U"TextArea", U"selectionColor" }] = PropertyMetadata{
		.tooltip = U"選択範囲の色",
	};
	metadata[PropertyKey{ U"TextArea", U"text" }] = PropertyMetadata{
		.numTextAreaLines = 3,
		.refreshesEveryFrame = true,
	};
	
	// EventTrigger
	metadata[PropertyKey{ U"EventTrigger", U"tag" }] = PropertyMetadata{
		.tooltip = U"プログラムから参照する際のタグ名",
		.tooltipDetail = U"EventTriggerはCanvas上で発生したイベントを統一的に管理するためのコンポーネントです\nプログラム上では毎フレーム、isEventFiredWithTag関数. getFiredEvent(s)WithTag関数, getFiredEventsAll関数を呼ぶことで発生したイベントを取得できます\n\nEventTriggerを使うことでプログラム上からノードを直接操作せずにイベントを受け取れるため、ノード構造の異なるCanvasでもイベント処理が再利用しやすくなります",
	};
	metadata[PropertyKey{ U"EventTrigger", U"triggerType" }] = PropertyMetadata{
		.tooltip = U"イベントを発火させる操作の種類",
	};
	metadata[PropertyKey{ U"EventTrigger", U"recursive" }] = PropertyMetadata{
		.tooltip = U"子孫要素の操作でもイベント発火するかどうか",
	};
	
	// CursorChanger
	metadata[PropertyKey{ U"CursorChanger", U"cursorStyle" }] = PropertyMetadata{
		.tooltip = U"マウスカーソルのスタイル",
		.tooltipDetail = U"要素へのマウスカーソルのホバー中に設定するカーソルスタイルを指定します",
	};
	metadata[PropertyKey{ U"CursorChanger", U"recursive" }] = PropertyMetadata{
		.tooltip = U"子孫要素のホバーでもカーソルを変更するかどうか",
	};
	metadata[PropertyKey{ U"CursorChanger", U"includingDisabled" }] = PropertyMetadata{
		.tooltip = U"InteractionStateがDisabledの要素へのホバーでもカーソルを変更するかどうか",
	};
	
	// Placeholder
	metadata[PropertyKey{ U"Placeholder", U"tag" }] = PropertyMetadata{
		.tooltip = U"プログラムから参照する際のタグ名",
		.tooltipDetail = U"Placeholderはプログラム上からコンポーネント追加や編集等の操作を行う目印として使用するコンポーネントです\nプログラム上ではwalkPlaceholders関数を使用して、タグ名をもとにPlaceholderを巡回できます\n例えば、tagに独自に作成したコンポーネントの種類名を入力し、プログラム上からそのコンポーネントを追加する用途で利用できます",
	};
	metadata[PropertyKey{ U"Placeholder", U"data" }] = PropertyMetadata{
		.tooltip = U"プレースホルダーのデータ (任意)",
		.tooltipDetail = U"自由なデータを文字列で指定できます\nプログラム上ではwalkPlaceholders関数でPlaceholderを巡回し、dataを参照できます",
		.numTextAreaLines = 3,
	};
	
	return metadata;
}

enum class ConstraintType : uint8
{
	AnchorConstraint,
	BoxConstraint,
};

struct Defaults
{
	ConstraintType constraintType;

	ConstraintVariant defaultConstraint()
	{
		switch (constraintType)
		{
		case ConstraintType::AnchorConstraint:
			return AnchorConstraint
			{
				.sizeDelta = { 100, 100 },
			};

		case ConstraintType::BoxConstraint:
			return BoxConstraint
			{
				.sizeDelta = { 100, 100 },
			};

		default:
			throw Error{ U"Unknown constraint type: {}"_fmt(static_cast<uint8>(constraintType)) };
		}
	}
};

class Hierarchy
{
private:
	std::shared_ptr<Canvas> m_canvas;
	std::shared_ptr<Node> m_hierarchyFrameNode;
	std::shared_ptr<Node> m_hierarchyInnerFrameNode;
	std::shared_ptr<Node> m_hierarchyRootNode;
	std::weak_ptr<Canvas> m_editorCanvas;
	std::weak_ptr<Node> m_editorHoveredNode;
	std::weak_ptr<Node> m_shiftSelectOriginNode;
	std::weak_ptr<Node> m_lastEditorSelectedNode;
	std::weak_ptr<Node> m_prevCheckedSelectedNode;
	bool m_prevSelectedNodeExists = false;
	std::shared_ptr<ContextMenu> m_contextMenu;
	Array<JSON> m_copiedNodeJSONs;
	bool m_prevClipboardHasContent = false;
	std::shared_ptr<Defaults> m_defaults;

	struct ElementDetail
	{
		size_t nestLevel = 0;
		std::shared_ptr<Node> node;
		std::shared_ptr<Node> hierarchyNode;
		std::shared_ptr<RectRenderer> hierarchyRectRenderer;
		std::shared_ptr<Label> hierarchyStateLabel;
		std::shared_ptr<Node> hierarchyToggleFoldedNode;
		std::shared_ptr<Label> hierarchyToggleFoldedLabel;
	};

	class Element
	{
	private:
		Hierarchy* m_pHierarchy = nullptr;
		ElementDetail m_elementDetail;
		EditorSelectedYN m_editorSelected = EditorSelectedYN::No;
		FoldedYN m_folded = FoldedYN::No;

	public:
		Element(Hierarchy* pHierarchy, const ElementDetail& elementDetail)
			: m_pHierarchy{ pHierarchy }
			, m_elementDetail{ elementDetail }
		{
			if (m_pHierarchy == nullptr)
			{
				throw Error{ U"Hierarchy is nullptr" };
			}
		}

		[[nodiscard]]
		EditorSelectedYN editorSelected() const
		{
			return m_editorSelected;
		}

		void setEditorSelected(EditorSelectedYN editorSelected)
		{
			m_editorSelected = editorSelected;
			m_elementDetail.hierarchyRectRenderer->setFillColor(HierarchyRectFillColor(m_editorSelected));
			m_elementDetail.hierarchyRectRenderer->setOutlineColor(HierarchyRectOutlineColor(m_editorSelected));
		}

		[[nodiscard]]
		const ElementDetail& elementDetail() const
		{
			return m_elementDetail;
		}

		[[nodiscard]]
		const std::shared_ptr<Node>& node() const
		{
			return m_elementDetail.node;
		}

		[[nodiscard]]
		const std::shared_ptr<Node>& hierarchyNode() const
		{
			return m_elementDetail.hierarchyNode;
		}

		void toggleFolded()
		{
			setFolded(m_folded ? FoldedYN::No : FoldedYN::Yes);
		}

		void setFolded(FoldedYN folded)
		{
			m_folded = folded;
			if (m_folded)
			{
				m_elementDetail.hierarchyToggleFoldedLabel->setText(U"▶");
			}
			else
			{
				m_elementDetail.hierarchyToggleFoldedLabel->setText(U"▼");
			}
			m_pHierarchy->applyFolding();
		}

		[[nodiscard]]
		FoldedYN folded() const
		{
			return m_folded;
		}

		[[nodiscard]]
		static PropertyValue<ColorF> HierarchyRectFillColor(EditorSelectedYN editorSelected)
		{
			if (editorSelected)
			{
				return ColorF{ Palette::Orange, 0.3 };
			}
			else
			{
				return PropertyValue<ColorF>{ ColorF{ 1.0, 0.0 } }.withHovered(ColorF{ 1.0, 0.2 });
			}
		}

		[[nodiscard]]
		static PropertyValue<ColorF> HierarchyRectOutlineColor(EditorSelectedYN editorSelected)
		{
			if (editorSelected)
			{
				return ColorF{ Palette::Orange, 0.6 };
			}
			else
			{
				return PropertyValue<ColorF>{ ColorF{ 1.0, 0.0 } }.withHovered(ColorF{ 1.0, 0.6 });
			}
		}
	};
	Array<Element> m_elements;

	void addElementRecursive(const std::shared_ptr<Node>& node, size_t nestLevel, RefreshesLayoutYN refreshesLayout)
	{
		if (node == nullptr)
		{
			throw Error{ U"Node is nullptr" };
		}

		m_elements.push_back(createElement(node, nestLevel));
		m_hierarchyRootNode->addChild(m_elements.back().elementDetail().hierarchyNode, RefreshesLayoutYN::No);

		for (const auto& child : node->children())
		{
			addElementRecursive(child, nestLevel + 1, RefreshesLayoutYN::No);
		}

		if (refreshesLayout)
		{
			m_canvas->refreshLayout();
		}
	}

	Element createElement(const std::shared_ptr<Node>& node, size_t nestLevel)
	{
		const auto hierarchyNode = Node::Create(
			U"Element",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = Vec2{ 0, 24 },
			},
			IsHitTargetYN::Yes,
			InheritChildrenStateFlags::Hovered);
		hierarchyNode->emplaceComponent<ContextMenuOpener>(
			m_contextMenu,
			Array<MenuElement>
			{
				MenuItem{ U"新規ノード", U"", KeyN, [this] { onClickNewNode(); } },
				MenuItem{ U"子として新規ノード", U"", KeyE, [this, node] { onClickNewNode(node); } },
				MenuSeparator{},
				MenuItem{ U"切り取り", U"Ctrl+X", KeyT, [this] { onClickCut(); } },
				MenuItem{ U"コピー", U"Ctrl+C", KeyC, [this] { onClickCopy(); } },
				MenuItem{ U"貼り付け", U"Ctrl+V", KeyP, [this] { onClickPaste(); }, [this] { return canPaste(); } },
				MenuItem{ U"子として貼り付け", U"", KeyA, [this, node] { onClickPaste(node); }, [this] { return canPaste(); } },
				MenuItem{ U"複製を作成", U"Ctrl+D", KeyL, [this] { onClickDuplicate(); } },
				MenuItem{ U"削除", U"Delete", none, [this] { onClickDelete(); } },
				MenuSeparator{},
				MenuItem{ U"上に移動", U"Alt+Up", KeyU, [this] { onClickMoveUp(); } },
				MenuItem{ U"下に移動", U"Alt+Down", KeyD, [this] { onClickMoveDown(); } },
				MenuSeparator{},
				MenuItem{ U"空の親ノードを作成", U"", KeyM, [this] { onClickCreateEmptyParent(); } },
			},
			[this, nodeWeak = std::weak_ptr{ node }]
			{
				const std::shared_ptr<Node> node = nodeWeak.lock();
				if (!node)
				{
					return;
				}

				Element* const pElement = getElementByNode(node);
				if (pElement == nullptr)
				{
					throw Error{ U"Element not found" };
				}
				Element& element = *pElement;

				// 既に選択中の場合は何もしない
				if (element.editorSelected())
				{
					return;
				}
				clearSelection();
				element.setEditorSelected(EditorSelectedYN::Yes);
				m_lastEditorSelectedNode = node;
				m_shiftSelectOriginNode = node;
			});
		hierarchyNode->emplaceComponent<RectRenderer>(Element::HierarchyRectFillColor(EditorSelectedYN::No), Element::HierarchyRectOutlineColor(EditorSelectedYN::No), 1.0, 3.0);
		hierarchyNode->emplaceComponent<DragDropSource>([this, hierarchyNode]() -> Array<std::shared_ptr<Node>>
			{
				// 未選択のノードをドラッグ開始した場合は単一選択
				if (const auto pElement = getElementByHierarchyNode(hierarchyNode))
				{
					if (!pElement->editorSelected())
					{
						selectSingleNode(pElement->node());
					}
				}

				// 選択中ノードを返す(親子関係にあるノードは子ノードを除く)
				return getSelectedNodesExcludingChildren().map([this](const auto& node) { return getElementByNode(node)->hierarchyNode(); });
			});

		constexpr double MoveAsSiblingThresholdPixels = 6.0;
		hierarchyNode->emplaceComponent<DragDropTarget>([this, hierarchyNode](const Array<std::shared_ptr<Node>>& sourceNodes)
			{
				const auto pTargetElement = getElementByHierarchyNode(hierarchyNode);
				if (pTargetElement == nullptr)
				{
					return;
				}
				const auto& targetElement = *pTargetElement;

				Array<std::shared_ptr<Node>> newSelection;
				newSelection.reserve(sourceNodes.size());

				const auto rect = hierarchyNode->rect();
				if (const auto moveAsSiblingRectTop = RectF{ rect.x, rect.y, rect.w, MoveAsSiblingThresholdPixels };
					moveAsSiblingRectTop.mouseOver())
				{
					// targetの上に兄弟ノードとして移動
					for (const auto& sourceNode : sourceNodes)
					{
						const auto pSourceElement = getElementByHierarchyNode(sourceNode);
						if (pSourceElement == nullptr)
						{
							return;
						}
						const auto& sourceElement = *pSourceElement;
						if (sourceElement.node() == targetElement.node())
						{
							// 自分自身には移動不可
							return;
						}
						if (sourceElement.node()->isAncestorOf(targetElement.node()))
						{
							// 子孫には移動不可
							return;
						}
						const auto pTargetParent = targetElement.node()->parent();
						if (pTargetParent == nullptr)
						{
							return;
						}
						sourceElement.node()->removeFromParent();
						const size_t index = pTargetParent->indexOfChild(targetElement.node());
						pTargetParent->addChildAtIndex(sourceElement.node(), index);

						newSelection.push_back(sourceElement.node());
					}
				}
				else if (const auto moveAsSiblingRectBottom = RectF{ rect.x, rect.y + rect.h - MoveAsSiblingThresholdPixels, rect.w, MoveAsSiblingThresholdPixels };
					moveAsSiblingRectBottom.mouseOver() && (targetElement.folded() || !targetElement.node()->hasChildren()))
				{
					// targetの下に兄弟ノードとして移動
					for (const auto& sourceNode : sourceNodes)
					{
						const auto pSourceElement = getElementByHierarchyNode(sourceNode);
						if (pSourceElement == nullptr)
						{
							return;
						}
						const auto& sourceElement = *pSourceElement;
						if (sourceElement.node() == nullptr || targetElement.node() == nullptr)
						{
							return;
						}
						if (sourceElement.node() == targetElement.node())
						{
							// 自分自身には移動不可
							return;
						}
						if (sourceElement.node()->isAncestorOf(targetElement.node()))
						{
							// 子孫には移動不可
							return;
						}
						const auto pTargetParent = targetElement.node()->parent();
						if (pTargetParent == nullptr)
						{
							return;
						}
						sourceElement.node()->removeFromParent();
						const size_t index = pTargetParent->indexOfChild(targetElement.node()) + 1;
						pTargetParent->addChildAtIndex(sourceElement.node(), index);

						newSelection.push_back(sourceElement.node());
					}
				}
				else
				{
					// 子ノードとして移動
					for (const auto& sourceNode : sourceNodes)
					{
						const auto pSourceElement = getElementByHierarchyNode(sourceNode);
						if (pSourceElement == nullptr)
						{
							return;
						}
						const auto& sourceElement = *pSourceElement;
						if (sourceElement.node() == nullptr || targetElement.node() == nullptr)
						{
							return;
						}
						if (sourceElement.node() == targetElement.node())
						{
							// 自分自身には移動不可
							return;
						}
						if (sourceElement.node()->isAncestorOf(targetElement.node()))
						{
							// 子孫には移動不可
							return;
						}
						if (sourceElement.node()->parent() == targetElement.node())
						{
							// 親子関係が既にある場合は移動不可
							return;
						}
						sourceElement.node()->setParent(targetElement.node());

						newSelection.push_back(sourceElement.node());
					}
				}
				refreshNodeList();
				selectNodes(newSelection);
			},
			[this, hierarchyNode](const Array<std::shared_ptr<Node>>& sourceNodes) -> bool
			{
				// ドラッグ中のノードが全てHierarchy上の要素の場合のみドロップ操作を受け付ける
				return sourceNodes.all([&](const auto& sourceNode)
					{
						return getElementByHierarchyNode(sourceNode) != nullptr;
					});
			},
			[this, nestLevel, hierarchyNode](const Node& node)
			{
				const auto pTargetElement = getElementByHierarchyNode(hierarchyNode);
				if (pTargetElement == nullptr)
				{
					return;
				}
				const auto& targetElement = *pTargetElement;

				constexpr double Thickness = 4.0;
				const auto rect = node.rect();
				if (const auto moveAsSiblingRectTop = RectF{ rect.x, rect.y, rect.w, MoveAsSiblingThresholdPixels };
					moveAsSiblingRectTop.mouseOver())
				{
					const Line line{ rect.tl() + Vec2::Right(15 + 20 * static_cast<double>(nestLevel)), rect.tr() };
					line.draw(Thickness, Palette::Orange);
					Circle{ line.begin, Thickness }.draw(Palette::Orange);
					Circle{ line.end, Thickness }.draw(Palette::Orange);
				}
				else if (const auto moveAsSiblingRectBottom = RectF{ rect.x, rect.y + rect.h - MoveAsSiblingThresholdPixels, rect.w, MoveAsSiblingThresholdPixels };
					moveAsSiblingRectBottom.mouseOver() && (targetElement.folded() || !targetElement.node()->hasChildren()))
				{
					const Line line{ rect.bl() + Vec2::Right(15 + 20 * static_cast<double>(nestLevel)), rect.br() };
					line.draw(Thickness, Palette::Orange);
					Circle{ line.begin, Thickness }.draw(Palette::Orange);
					Circle{ line.end, Thickness }.draw(Palette::Orange);
				}
				else
				{
					rect.draw(ColorF{ 1.0, 0.3 });
				}
			});
		const auto nameLabel = hierarchyNode->emplaceComponent<Label>(
			node->name(),
			U"",
			14,
			Palette::White,
			HorizontalAlign::Left,
			VerticalAlign::Middle,
			LRTB{ 20 + static_cast<double>(nestLevel) * 20, 5, 0, 0 },
			HorizontalOverflow::Wrap,
			VerticalOverflow::Clip);

		const auto stateLabel = hierarchyNode->emplaceComponent<Label>(
			U"",
			U"",
			14,
			Palette::White,
			HorizontalAlign::Right,
			VerticalAlign::Middle,
			LRTB{ 0, 5, 0, 0 },
			HorizontalOverflow::Overflow,
			VerticalOverflow::Clip);

		const auto toggleFoldedNode = hierarchyNode->emplaceChild(
			U"ToggleFolded",
			AnchorConstraint
			{
				.anchorMin = Anchor::TopLeft,
				.anchorMax = Anchor::BottomLeft,
				.posDelta = Vec2{ 10 + nestLevel * 20, 0 },
				.sizeDelta = Vec2{ 30, 0 },
				.sizeDeltaPivot = Anchor::MiddleCenter,
			});
		toggleFoldedNode->setActive(node->hasChildren() ? ActiveYN::Yes : ActiveYN::No);
		toggleFoldedNode->addOnClick([this, node](const std::shared_ptr<Node>&)
			{
				if (!node->hasChildren())
				{
					// 子がないので折り畳み不可
					return;
				}

				if (const auto pElement = getElementByNode(node))
				{
					pElement->toggleFolded();
				}
			});
		const auto toggleFoldedLabel = toggleFoldedNode->emplaceComponent<Label>(
			U"▼",
			U"",
			10,
			ColorF{ 1.0, 0.6 },
			HorizontalAlign::Center,
			VerticalAlign::Middle);

		return Element
		{
			this,
			ElementDetail
			{
				.nestLevel = nestLevel,
				.node = node,
				.hierarchyNode = hierarchyNode,
				.hierarchyRectRenderer = hierarchyNode->getComponent<RectRenderer>(),
				.hierarchyStateLabel = stateLabel,
				.hierarchyToggleFoldedNode = toggleFoldedNode,
				.hierarchyToggleFoldedLabel = toggleFoldedLabel,
			}
		};
	}

	Element* getElementByNode(const std::shared_ptr<Node>& node)
	{
		if (node == nullptr)
		{
			return nullptr;
		}
		const auto it = std::find_if(m_elements.begin(), m_elements.end(),
			[node](const auto& e) { return e.node() == node; });
		if (it == m_elements.end())
		{
			return nullptr;
		}
		return &(*it);
	}

	Element* getElementByHierarchyNode(const std::shared_ptr<Node>& hierarchyNode)
	{
		if (hierarchyNode == nullptr)
		{
			return nullptr;
		}
		const auto it = std::find_if(m_elements.begin(), m_elements.end(),
			[hierarchyNode](const auto& e) { return e.hierarchyNode() == hierarchyNode; });
		if (it == m_elements.end())
		{
			return nullptr;
		}
		return &(*it);
	}

	void applyFoldingRecursive(Element& element, FoldedYN parentFoldedInHierarchy)
	{
		// 親が折り畳まれている場合はHierarchy上で非表示にする
		element.hierarchyNode()->setActive(parentFoldedInHierarchy ? ActiveYN::No : ActiveYN::Yes);

		// 再帰的に適用
		for (auto& childNode : element.node()->children())
		{
			if (Element* childElement = getElementByNode(childNode))
			{
				applyFoldingRecursive(*childElement, FoldedYN{ parentFoldedInHierarchy || element.folded() });
			}
		}
	}

public:
	explicit Hierarchy(const std::shared_ptr<Canvas>& canvas, const std::shared_ptr<Canvas>& editorCanvas, const std::shared_ptr<ContextMenu>& contextMenu, const std::shared_ptr<Defaults>& defaults)
		: m_canvas(canvas)
		, m_hierarchyFrameNode(editorCanvas->rootNode()->emplaceChild(
			U"HierarchyFrame",
			AnchorConstraint
			{
				.anchorMin = Anchor::TopLeft,
				.anchorMax = Anchor::BottomLeft,
				.posDelta = Vec2{ 0, MenuBarHeight + Toolbar::ToolbarHeight },
				.sizeDelta = Vec2{ 300, -(MenuBarHeight + Toolbar::ToolbarHeight) },
				.sizeDeltaPivot = Anchor::TopLeft,
			}))
		, m_hierarchyInnerFrameNode(m_hierarchyFrameNode->emplaceChild(
			U"HierarchyInnerFrame",
			AnchorConstraint
			{
				.anchorMin = Anchor::TopLeft,
				.anchorMax = Anchor::BottomRight,
				.posDelta = Vec2{ 0, 0 },
				.sizeDelta = Vec2{ -2, -2 },
				.sizeDeltaPivot = Anchor::MiddleCenter,
			},
			IsHitTargetYN::Yes,
			InheritChildrenStateFlags::Hovered | InheritChildrenStateFlags::Pressed))
		, m_hierarchyRootNode(m_hierarchyInnerFrameNode->emplaceChild(
			U"Hierarchy",
			AnchorConstraint
			{
				.anchorMin = Anchor::TopLeft,
				.anchorMax = Anchor::BottomRight,
				.posDelta = Vec2{ 0, 0 },
				.sizeDelta = Vec2{ -10, -14 },
				.sizeDeltaPivot = Anchor::MiddleCenter,
			}))
		, m_editorCanvas(editorCanvas)
		, m_contextMenu(contextMenu)
		, m_defaults(defaults)
	{
		m_hierarchyFrameNode->emplaceComponent<RectRenderer>(ColorF{ 0.5, 0.4 }, Palette::Black, 0.0, 10.0);
		m_hierarchyInnerFrameNode->emplaceComponent<RectRenderer>(ColorF{ 0.1, 0.8 }, Palette::Black, 0.0, 10.0);
		m_hierarchyInnerFrameNode->emplaceComponent<ContextMenuOpener>(contextMenu,
			Array<MenuElement>
			{
				MenuItem{ U"新規ノード", U"", KeyN, [this] { onClickNewNode(); } },
				MenuItem{ U"貼り付け", U"Ctrl+V", KeyP, [this] { onClickPaste(); }, [this] { return canPaste(); } },
			});
		m_hierarchyRootNode->setBoxChildrenLayout(VerticalLayout{ .padding = 2 });
		m_hierarchyRootNode->setVerticalScrollable(true);

		refreshNodeList();
	}

	void refreshNodeList()
	{
		Array<std::weak_ptr<Node>> foldedNodes;
		foldedNodes.reserve(m_elements.size());
		for (const auto& element : m_elements)
		{
			if (element.folded())
			{
				foldedNodes.push_back(element.node());
			}
		}

		clearSelection();
		m_elements.clear();
		m_hierarchyRootNode->removeChildrenAll();
		addElementRecursive(m_canvas->rootNode(), 0, RefreshesLayoutYN::No);

		for (const auto& node : foldedNodes)
		{
			if (auto pElement = getElementByNode(node.lock()))
			{
				pElement->setFolded(FoldedYN::Yes);
			}
		}

		if (const auto editorCanvas = m_editorCanvas.lock())
		{
			editorCanvas->refreshLayout();
		}
	}

	void refreshNodeNames()
	{
		for (const auto& element : m_elements)
		{
			element.hierarchyNode()->getComponent<Label>()->setText(element.node()->name());
		}
	}

	void selectNodes(const Array<std::shared_ptr<Node>>& nodes)
	{
		clearSelection();
		for (const auto& node : nodes)
		{
			if (auto pElement = getElementByNode(node))
			{
				pElement->setEditorSelected(EditorSelectedYN::Yes);
				unfoldForNode(node);
			}
		}
		if (nodes.size() == 1)
		{
			m_lastEditorSelectedNode = nodes.front();
			m_shiftSelectOriginNode = nodes.front();
		}
	}

	void selectAll()
	{
		if (m_elements.empty())
		{
			return;
		}

		for (auto& element : m_elements)
		{
			element.setEditorSelected(EditorSelectedYN::Yes);
		}

		m_lastEditorSelectedNode = m_elements.back().node();
		m_shiftSelectOriginNode = m_elements.front().node();
	}

	void selectSingleNode(const std::shared_ptr<Node>& node)
	{
		clearSelection();
		if (auto it = std::find_if(m_elements.begin(), m_elements.end(), [&node](const Element& element) { return element.node() == node; });
			it != m_elements.end())
		{
			it->setEditorSelected(EditorSelectedYN::Yes);
			unfoldForNode(node);
			m_lastEditorSelectedNode = node;
			m_shiftSelectOriginNode = node;
		}
	}

	bool hasSelection() const
	{
		return std::any_of(m_elements.begin(), m_elements.end(), [](const Element& element) { return element.editorSelected(); });
	}

	void unfoldForNode(const std::shared_ptr<Node>& node)
	{
		if (auto pElement = getElementByNode(node))
		{
			pElement->setFolded(FoldedYN::No);
			if (auto parentNode = node->parent())
			{
				unfoldForNode(parentNode);
			}
		}
	}

	bool canPaste() const
	{
		return !m_copiedNodeJSONs.empty();
	}

	void onClickNewNode()
	{
		// 最後に選択したノードの兄弟として新規ノードを作成
		if (const auto lastEditorSelectedNode = m_lastEditorSelectedNode.lock())
		{
			if (const auto parentNode = lastEditorSelectedNode->parent())
			{
				onClickNewNode(parentNode);
			}
			else
			{
				onClickNewNode(m_canvas->rootNode());
			}
		}
		else
		{
			onClickNewNode(m_canvas->rootNode());
		}
	}

	void onClickNewNode(std::shared_ptr<Node> parentNode)
	{
		if (!parentNode)
		{
			throw Error{ U"Parent node is nullptr" };
		}

		// 記憶された種類のConstraintを使用してノードを作成
		std::shared_ptr<Node> newNode = parentNode->emplaceChild(
			U"Node",
			m_defaults->defaultConstraint());
		refreshNodeList();
		selectSingleNode(newNode);
	}

	void onClickDelete()
	{
		bool hasDeleted = false;
		for (auto it = m_elements.begin(); it != m_elements.end();)
		{
			if (it->editorSelected())
			{
				if (it->node()->removeFromParent())
				{
					it = m_elements.erase(it);
					hasDeleted = true;
				}
				else
				{
					++it;
				}
			}
			else
			{
				++it;
			}
		}
		if (!hasDeleted)
		{
			return;
		}
		refreshNodeList();
		clearSelection();
	}

	void onClickCut()
	{
		onClickCopy();
		onClickDelete();
	}

	[[nodiscard]]
	Array<std::shared_ptr<Node>> getSelectedNodesExcludingChildren() const
	{
		// 選択中のノードを列挙
		// ただし、親が選択中の場合は子は含めない
		Array<std::shared_ptr<Node>> selectedNodes;
		for (const auto& element : m_elements)
		{
			if (element.editorSelected())
			{
				bool parentSelected = false;
				for (const auto& parent : selectedNodes)
				{
					if (parent->containsChild(element.node(), RecursiveYN::Yes))
					{
						parentSelected = true;
						break;
					}
				}
				if (!parentSelected)
				{
					selectedNodes.push_back(element.node());
				}
			}
		}
		return selectedNodes;
	}

	void onClickCopy()
	{
		m_copiedNodeJSONs.clear();

		// 選択中のノードをコピー
		const auto selectedNodes = getSelectedNodesExcludingChildren();
		m_copiedNodeJSONs.reserve(selectedNodes.size());
		for (const auto& selectedNode : selectedNodes)
		{
			m_copiedNodeJSONs.push_back(selectedNode->toJSON());
		}
	}

	void onClickDuplicate()
	{
		const auto selectedNodes = getSelectedNodesExcludingChildren();
		if (selectedNodes.empty())
		{
			return;
		}

		// 複製実行
		Array<std::shared_ptr<Node>> newNodes;
		newNodes.reserve(selectedNodes.size());
		for (const auto& selectedNode : selectedNodes)
		{
			const auto parentNode = selectedNode->parent();
			if (!parentNode)
			{
				continue;
			}
			const auto newNode = parentNode->addChildFromJSON(selectedNode->toJSON(), RefreshesLayoutYN::No);
			newNodes.push_back(newNode);
		}
		m_canvas->refreshLayout();
		refreshNodeList();
		selectNodes(newNodes);
	}

	void onClickPaste()
	{
		// 最後に選択したノードの兄弟として貼り付け
		if (const auto lastEditorSelectedNode = m_lastEditorSelectedNode.lock())
		{
			if (lastEditorSelectedNode->parent())
			{
				onClickPaste(lastEditorSelectedNode->parent(), lastEditorSelectedNode->siblingIndex() + 1);
			}
			else
			{
				onClickPaste(m_canvas->rootNode());
			}
		}
		else
		{
			onClickPaste(m_canvas->rootNode());
		}
	}

	void onClickPaste(std::shared_ptr<Node> parentNode, Optional<size_t> index = none)
	{
		if (!parentNode)
		{
			throw Error{ U"Parent node is nullptr" };
		}

		if (m_copiedNodeJSONs.empty())
		{
			return;
		}

		// 貼り付け実行
		Array<std::shared_ptr<Node>> newNodes;
		if (index.has_value())
		{
			size_t indexValue = Min(index.value(), parentNode->children().size());
			for (const auto& copiedNodeJSON : m_copiedNodeJSONs)
			{
				newNodes.push_back(parentNode->addChildAtIndexFromJSON(copiedNodeJSON, indexValue, RefreshesLayoutYN::No));
				++indexValue;
			}
		}
		else
		{
			for (const auto& copiedNodeJSON : m_copiedNodeJSONs)
			{
				newNodes.push_back(parentNode->addChildFromJSON(copiedNodeJSON, RefreshesLayoutYN::No));
			}
		}
		m_canvas->refreshLayout();
		refreshNodeList();
		selectNodes(newNodes);
	}

	void onClickCreateEmptyParent()
	{
		const auto selectedNode = m_lastEditorSelectedNode.lock();
		if (!selectedNode)
		{
			return;
		}

		auto oldParent = selectedNode->parent();
		if (!oldParent)
		{
			return;
		}

		// selectedNodeが兄弟同士の中で何番目の要素かを調べる
		auto& siblings = oldParent->children();
		auto it = std::find(siblings.begin(), siblings.end(), selectedNode);
		if (it == siblings.end())
		{
			return;
		}
		const size_t idx = std::distance(siblings.begin(), it);

		// 親から切り離す
		selectedNode->removeFromParent();

		// 元ノードと同じインデックスに同じレイアウト設定で空の親ノードを生成
		const auto newParent = Node::Create(U"Node", selectedNode->constraint());
		oldParent->addChildAtIndex(newParent, idx);

		// 新しい親のもとへ子として追加
		newParent->addChild(selectedNode);

		// 元オブジェクトはアンカーがMiddleCenterのAnchorConstraintに変更する
		const RectF originalCalculatedRect = selectedNode->layoutAppliedRect();
		selectedNode->setConstraint(AnchorConstraint
		{
			.anchorMin = Anchor::MiddleCenter,
			.anchorMax = Anchor::MiddleCenter,
			.posDelta = Vec2{ 0, 0 },
			.sizeDelta = originalCalculatedRect.size,
			.sizeDeltaPivot = Anchor::MiddleCenter,
		});

		refreshNodeList();
		selectSingleNode(newParent);
	}

	void onClickMoveUp()
	{
		// 選択中のノードを列挙
		Array<std::shared_ptr<Node>> selectedNodes;
		for (const auto& element : m_elements)
		{
			if (element.editorSelected())
			{
				selectedNodes.push_back(element.node());
			}
		}
		if (selectedNodes.isEmpty())
		{
			return;
		}

		// 親ノードごとに処理
		HashTable<std::shared_ptr<Node>, Array<std::shared_ptr<Node>>> selectionByParent;
		selectionByParent.reserve(selectedNodes.size());
		for (const auto& child : selectedNodes)
		{
			if (auto parent = child->parent())
			{
				selectionByParent[parent].push_back(child);
			}
		}
		for (auto& [parent, childrenToMove] : selectionByParent)
		{
			const auto& siblings = parent->children();

			// 選択中の子のインデックスを取得
			Array<size_t> indices;
			indices.reserve(childrenToMove.size());
			for (const auto& child : childrenToMove)
			{
				const auto it = std::find(siblings.begin(), siblings.end(), child);
				if (it != siblings.end())
				{
					indices.push_back(std::distance(siblings.begin(), it));
				}
			}

			// インデックスが小さい順に上の要素と入れ替え
			std::sort(indices.begin(), indices.end());
			for (auto index : indices)
			{
				if (index > 0)
				{
					parent->swapChildren(index, index - 1);
				}
			}
		}
		m_canvas->refreshLayout();
		refreshNodeList();
		selectNodes(selectedNodes);
	}

	void onClickMoveDown()
	{
		// 選択中のノードを列挙
		Array<std::shared_ptr<Node>> selectedNodes;
		for (const auto& element : m_elements)
		{
			if (element.editorSelected())
			{
				selectedNodes.push_back(element.node());
			}
		}
		if (selectedNodes.isEmpty())
		{
			return;
		}

		// 親ノードごとに処理
		HashTable<std::shared_ptr<Node>, Array<std::shared_ptr<Node>>> selectionByParent;
		selectionByParent.reserve(selectedNodes.size());
		for (const auto& child : selectedNodes)
		{
			if (auto parent = child->parent())
			{
				selectionByParent[parent].push_back(child);
			}
		}
		for (auto& [parent, childrenToMove] : selectionByParent)
		{
			const auto& siblings = parent->children();

			// 選択中の子のインデックスを取得
			Array<size_t> indices;
			indices.reserve(childrenToMove.size());
			for (const auto& child : childrenToMove)
			{
				const auto it = std::find(siblings.begin(), siblings.end(), child);
				if (it != siblings.end())
				{
					indices.push_back(std::distance(siblings.begin(), it));
				}
			}

			// インデックスが大きい順に下の要素と入れ替え
			std::sort(indices.begin(), indices.end(), std::greater<size_t>());
			for (auto index : indices)
			{
				if (index < siblings.size() - 1)
				{
					parent->swapChildren(index, index + 1);
				}
			}
		}
		m_canvas->refreshLayout();
		refreshNodeList();
		selectNodes(selectedNodes);
	}

	void clearSelection(bool clearShiftSelectOrigin = true)
	{
		for (auto& element : m_elements)
		{
			element.setEditorSelected(EditorSelectedYN::No);
		}
		if (clearShiftSelectOrigin)
		{
			m_shiftSelectOriginNode.reset();
		}
		m_lastEditorSelectedNode = std::weak_ptr<Node>{};
	}

	void update()
	{
		m_editorHoveredNode.reset();
		for (size_t i = 0; i < m_elements.size(); ++i)
		{
			auto& element = m_elements[i];
			if (element.hierarchyNode()->isHovered())
			{
				m_editorHoveredNode = element.node();
			}

			if (element.node()->isHitTarget())
			{
				const InteractionState interactionState = element.node()->currentInteractionState();
				switch (interactionState)
				{
				case InteractionState::Default:
					element.elementDetail().hierarchyStateLabel->setText(element.node()->selected() ? U"[Selected]" : U"[Default]");
					break;
				case InteractionState::Hovered:
					element.elementDetail().hierarchyStateLabel->setText(element.node()->selected() ? U"[Selected, Hovered]" : U"[Hovered]");
					break;
				case InteractionState::Pressed:
					element.elementDetail().hierarchyStateLabel->setText(element.node()->selected() ? U"[Selected, Pressed]" : U"[Pressed]");
					break;
				case InteractionState::Disabled:
					element.elementDetail().hierarchyStateLabel->setText(element.node()->selected() ? U"[Selected, Disabled]" : U"[Disabled]");
					break;
				default:
					throw Error{ U"Invalid InteractionState: {}"_fmt(static_cast<std::underlying_type_t<InteractionState>>(interactionState)) };
				}
			}
			else
			{
				element.elementDetail().hierarchyStateLabel->setText(element.node()->selected() ? U"[Selected]" : U"");
			}

			if (element.hierarchyNode()->isClicked())
			{
				if (KeyShift.pressed() && !m_shiftSelectOriginNode.expired())
				{
					const auto originIt = std::find_if(m_elements.begin(), m_elements.end(), [originNode = m_shiftSelectOriginNode.lock()](const auto& e) { return e.node() == originNode; });
					if (originIt == m_elements.end())
					{
						throw Error{ U"Shift select origin node not found in m_elements" };
					}
					clearSelection(false);
					const auto originIndex = static_cast<size_t>(std::distance(m_elements.begin(), originIt));
					const auto start = Min(originIndex, i);
					const auto end = Max(originIndex, i);
					for (size_t j = start; j <= end; ++j)
					{
						m_elements[j].setEditorSelected(EditorSelectedYN::Yes);
					}
				}
				else
				{
					if (KeyControl.pressed())
					{
						// Ctrlキーを押しながらクリックした場合は選択/非選択を切り替え
						const EditorSelectedYN newSelected = EditorSelectedYN{ !element.editorSelected() };
						element.setEditorSelected(newSelected);
						if (newSelected)
						{
							m_shiftSelectOriginNode = element.node();
						}
						else
						{
							m_shiftSelectOriginNode.reset();
						}
					}
					else
					{
						// 普通にクリックした場合は1つだけ選択
						// (複数回押しても選択/非選択の切り替えはしない)
						clearSelection();
						element.setEditorSelected(EditorSelectedYN::Yes);
						m_shiftSelectOriginNode = element.node();
					}
				}

				if (m_elements.count_if([](const auto& e) { return e.editorSelected().getBool(); }) == 1)
				{
					const auto selectedNode = std::find_if(m_elements.begin(), m_elements.end(), [](const auto& e) { return e.editorSelected(); })->node();
					m_lastEditorSelectedNode = selectedNode;
				}
				else
				{
					m_lastEditorSelectedNode = std::weak_ptr<Node>{};
				}
			}

			if (m_hierarchyRootNode->isClicked())
			{
				clearSelection();
			}
		}
	}

	void applyFolding()
	{
		if (m_elements.empty())
		{
			return;
		}
		auto& rootElement = m_elements.front();
		applyFoldingRecursive(rootElement, FoldedYN::No);
	}

	[[nodiscard]]
	const std::weak_ptr<Node>& selectedNode() const
	{
		return m_lastEditorSelectedNode;
	}

	[[nodiscard]]
	bool hasSelectionChanged()
	{
		const auto currentSelectedNode = m_lastEditorSelectedNode.lock();
		const auto prevSelectedNode = m_prevCheckedSelectedNode.lock();
		
		// 選択状態が変化したかチェック
		const bool currentExists = (currentSelectedNode != nullptr);
		const bool changed = (currentSelectedNode != prevSelectedNode) || 
		                    (currentExists != m_prevSelectedNodeExists);
		
		// 状態を更新
		if (changed)
		{
			m_prevCheckedSelectedNode = m_lastEditorSelectedNode;
			m_prevSelectedNodeExists = currentExists;
		}
		
		return changed;
	}
	
	[[nodiscard]]
	bool toolbarRefreshRequested()
	{
		bool refreshNeeded = false;
		
		// クリップボードの空/非空状態が変化したかチェック
		const bool currentHasContent = !m_copiedNodeJSONs.empty();
		if (currentHasContent != m_prevClipboardHasContent)
		{
			m_prevClipboardHasContent = currentHasContent;
			refreshNeeded = true;
		}
		
		return refreshNeeded;
	}

	const std::shared_ptr<Node>& hierarchyFrameNode() const
	{
		return m_hierarchyFrameNode;
	}

	void drawSelectedNodesGizmo() const
	{
		const auto editorHoveredNode = m_editorHoveredNode.lock();

		for (const auto& element : m_elements)
		{
			const auto& node = element.node();
			if (!node->activeInHierarchy())
			{
				continue;
			}

			constexpr double Thickness = 2.0;
			const EditorSelectedYN editorSelected = element.editorSelected();
			if (editorSelected)
			{
				node->hitTestRect().stretched(Thickness / 2).drawFrame(Thickness, Palette::Orange);

				// 上下左右にリサイズハンドルを表示
				// TODO: リサイズ可能にする
				/*constexpr double HandleSize = 3.0;
				const auto rect = node->rect().stretched(Thickness / 2);
				Circle{ rect.topCenter(), HandleSize }.draw(Palette::Orange);
				Circle{ rect.bottomCenter(), HandleSize }.draw(Palette::Orange);
				Circle{ rect.leftCenter(), HandleSize }.draw(Palette::Orange);
				Circle{ rect.rightCenter(), HandleSize }.draw(Palette::Orange);*/
			}

			if (node == editorHoveredNode)
			{
				const auto& rect = node->hitTestRect();
				rect.draw(ColorF{ 1.0, 0.1 });
				if (!editorSelected)
				{
					rect.stretched(Thickness / 2).drawFrame(Thickness, ColorF{ 1.0 });
				}
			}
		}
	}
};

std::shared_ptr<Node> CreateButtonNode(StringView text, const ConstraintVariant& constraint, std::function<void(const std::shared_ptr<Node>&)> onClick, IsDefaultButtonYN isDefaultButton = IsDefaultButtonYN::No)
{
	auto buttonNode = Node::Create(
		U"Button",
		constraint,
		IsHitTargetYN::Yes);
	buttonNode->setBoxChildrenLayout(HorizontalLayout{ .horizontalAlign = HorizontalAlign::Center, .verticalAlign = VerticalAlign::Middle });
	buttonNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, isDefaultButton ? 0.6 : 0.4 } }.withHovered(ColorF{ 1.0, isDefaultButton ? 0.8 : 0.6 }).withSmoothTime(0.05), 1.0, 4.0);
	buttonNode->addOnClick([onClick](const std::shared_ptr<Node>& node) { if (onClick) onClick(node); });
	const auto labelNode = buttonNode->emplaceChild(
		U"ButtonLabel",
		BoxConstraint
		{
			.sizeRatio = Vec2{ 1, 1 },
			.margin = LRTB{ 0, 0, 0, 0 },
		},
		IsHitTargetYN::No);
	labelNode->emplaceComponent<Label>(
		text,
		U"",
		14,
		Palette::White,
		HorizontalAlign::Center,
		VerticalAlign::Middle);
	return buttonNode;
}

struct DialogButtonDesc
{
	String text = U""; // TODO: 現状は単一表示言語想定でダイアログ結果にテキストを流用しているが、将来的にはダイアログ毎の結果型として任意の型を指定可能にして、シンプルな用途のためにYes/No/Cancel用の関数を用意したい
	Optional<Input> mnemonicInput = none;
	AppendsMnemonicKeyTextYN appendsMnemonicKeyText = AppendsMnemonicKeyTextYN::Yes; // TODO: 現状は日本語想定でカッコで追加する形にしているが、将来的には「&File」「ファイル(&F)」など&を前につけるとニーモニック扱いされるようにしたい
	IsDefaultButtonYN isDefaultButton = IsDefaultButtonYN::No;
	IsCancelButtonYN isCancelButton = IsCancelButtonYN::No;
};

class IDialog
{
public:
	virtual ~IDialog() = default;

	virtual double dialogWidth() const = 0;

	virtual Array<DialogButtonDesc> buttonDescs() const = 0;

	virtual void createDialogContent(const std::shared_ptr<Node>& contentRootNode, const std::shared_ptr<ContextMenu>& dialogContextMenu) = 0;

	virtual void onResult(StringView resultButtonText) = 0;
};

class DialogFrame
{
private:
	std::shared_ptr<Canvas> m_dialogCanvas;
	std::shared_ptr<Node> m_screenMaskNode;
	std::shared_ptr<Node> m_dialogNode;
	std::shared_ptr<Node> m_contentRootNode;
	std::shared_ptr<Node> m_buttonRootNode;
	std::function<void(StringView)> m_onResult;

public:
	explicit DialogFrame(const std::shared_ptr<Canvas>& dialogCanvas, double dialogWidth, const std::function<void(StringView)>& onResult, const Array<DialogButtonDesc>& buttonDescs)
		: m_dialogCanvas(dialogCanvas)
		, m_screenMaskNode(dialogCanvas->rootNode()->emplaceChild(
			U"Dialog_ScreenMask",
			AnchorConstraint
			{
				.anchorMin = Anchor::TopLeft,
				.anchorMax = Anchor::BottomRight,
				.posDelta = Vec2{ 0, 0 },
				.sizeDelta = Vec2{ 0, 0 },
				.sizeDeltaPivot = Anchor::TopLeft,
			}))
		, m_dialogNode(m_screenMaskNode->emplaceChild(
			U"Dialog",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0, 0 },
				.sizeDelta = Vec2{ dialogWidth, 0 },
				.margin = LRTB{ 0, 0, 0, 0 },
			}))
		, m_contentRootNode(m_dialogNode->emplaceChild(
			U"Dialog_ContentRoot",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.margin = LRTB{ 0, 0, 0, 0 },
			}))
		, m_buttonRootNode(m_dialogNode->emplaceChild(
			U"Dialog_ButtonRoot",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.margin = LRTB{ 0, 0, 0, 0 },
			}))
		, m_onResult(onResult)
	{
		m_screenMaskNode->emplaceComponent<InputBlocker>();

		// ダイアログ背面を暗くする
		m_screenMaskNode->emplaceComponent<RectRenderer>(ColorF{ 0.0, 0.25 });
		m_screenMaskNode->setBoxChildrenLayout(FlowLayout{ .horizontalAlign = HorizontalAlign::Center, .verticalAlign = VerticalAlign::Middle }, RefreshesLayoutYN::No);

		m_dialogNode->setBoxChildrenLayout(VerticalLayout{ .padding = LRTB{ 8, 8, 8, 12 } }, RefreshesLayoutYN::No);
		m_dialogNode->emplaceComponent<RectRenderer>(ColorF{ 0.1, 0.8 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0, ColorF{ 0.0, 0.3 }, Vec2{ 2, 2 }, 8.0, 4.0);

		const auto buttonParentNode = m_dialogNode->emplaceChild(
			U"Dialog_ButtonParent",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.margin = LRTB{ 0, 0, 8, 0 },
			});
		buttonParentNode->setBoxChildrenLayout(HorizontalLayout{ .padding = LRTB{ 0, 0, 0, 0 }, .horizontalAlign = HorizontalAlign::Center }, RefreshesLayoutYN::No);
		for (const DialogButtonDesc& buttonDesc : buttonDescs)
		{
			const String buttonText = [](const DialogButtonDesc& buttonDesc) -> String
				{
					if (buttonDesc.mnemonicInput.has_value() && buttonDesc.appendsMnemonicKeyText)
					{
						return U"{}({})"_fmt(buttonDesc.text, buttonDesc.mnemonicInput->name());
					}
					else
					{
						return buttonDesc.text;
					}
				}(buttonDesc);

			const auto buttonNode = buttonParentNode->addChild(
				CreateButtonNode(
					buttonText,
					BoxConstraint
					{
						.sizeDelta = Vec2{ 100, 24 },
						.margin = LRTB{ 4, 4, 0, 0 },
					},
					[this, buttonDesc](const std::shared_ptr<Node>&)
					{
						m_screenMaskNode->removeFromParent();
						if (m_onResult)
						{
							m_onResult(buttonDesc.text);
						}
					},
					buttonDesc.isDefaultButton),
				RefreshesLayoutYN::No);

			if (buttonDesc.mnemonicInput.has_value())
			{
				buttonNode->addClickHotKey(*buttonDesc.mnemonicInput);
			}

			if (buttonDesc.isDefaultButton)
			{
				buttonNode->addClickHotKey(KeyEnter, EnabledWhileTextEditingYN::Yes);
			}

			if (buttonDesc.isCancelButton)
			{
				buttonNode->addClickHotKey(KeyEscape, EnabledWhileTextEditingYN::Yes);
			}
		}
		buttonParentNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly, RefreshesLayoutYN::No);
		m_dialogNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly, RefreshesLayoutYN::No);

		m_dialogCanvas->refreshLayout();
	}

	virtual ~DialogFrame() = default;

	std::shared_ptr<Node> contentRootNode() const
	{
		return m_contentRootNode;
	}

	void refreshLayoutForContent()
	{
		m_contentRootNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly, RefreshesLayoutYN::No);
		m_dialogNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly, RefreshesLayoutYN::No);
		m_dialogCanvas->refreshLayout();
	}
};

class DialogOpener
{
private:
	size_t m_nextDialogId = 1;
	std::shared_ptr<Canvas> m_dialogCanvas;
	std::shared_ptr<ContextMenu> m_dialogContextMenu;
	HashTable<size_t, std::shared_ptr<DialogFrame>> m_openedDialogFrames;

public:
	explicit DialogOpener(const std::shared_ptr<Canvas>& dialogCanvas, const std::shared_ptr<ContextMenu>& dialogContextMenu)
		: m_dialogCanvas(dialogCanvas)
		, m_dialogContextMenu(dialogContextMenu)
	{
	}

	void openDialog(const std::shared_ptr<IDialog>& dialog)
	{
		auto dialogFrame = std::make_shared<DialogFrame>(m_dialogCanvas, dialog->dialogWidth(), [this, dialogId = m_nextDialogId, dialog](StringView resultButtonText) { dialog->onResult(resultButtonText); m_openedDialogFrames.erase(dialogId); }, dialog->buttonDescs());
		dialog->createDialogContent(dialogFrame->contentRootNode(), m_dialogContextMenu);
		dialogFrame->refreshLayoutForContent();
		m_openedDialogFrames.emplace(m_nextDialogId, dialogFrame);
		++m_nextDialogId;
	}

	bool anyDialogOpened() const
	{
		return !m_openedDialogFrames.empty();
	}
};

class SimpleDialog : public IDialog
{
private:
	String m_text;
	std::function<void(StringView)> m_onResult;
	Array<DialogButtonDesc> m_buttonDescs;

public:
	SimpleDialog(StringView text, const std::function<void(StringView)>& onResult, const Array<DialogButtonDesc>& buttonDescs)
		: m_text(text)
		, m_onResult(onResult)
		, m_buttonDescs(buttonDescs)
	{
	}

	double dialogWidth() const override
	{
		return 400;
	}

	Array<DialogButtonDesc> buttonDescs() const override
	{
		return m_buttonDescs;
	}

	void createDialogContent(const std::shared_ptr<Node>& contentRootNode, const std::shared_ptr<ContextMenu>&) override
	{
		const auto labelNode = contentRootNode->emplaceChild(
			U"Label",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = SizeF{ 0, 48 },
				.margin = LRTB{ 0, 0, 16, 16 },
			});
		labelNode->emplaceComponent<Label>(
			m_text,
			U"",
			14,
			Palette::White,
			HorizontalAlign::Center,
			VerticalAlign::Middle);
	}

	void onResult(StringView resultButtonText) override
	{
		if (m_onResult)
		{
			m_onResult(resultButtonText);
		}
	}
};

class InteractivePropertyValueDialog : public IDialog
{
private:
	IProperty* m_pProperty;
	Array<String> m_buttonTexts;
	std::function<void()> m_onChange;

public:
	InteractivePropertyValueDialog(IProperty* pProperty, std::function<void()> onChange)
		: m_pProperty(pProperty)
		, m_onChange(std::move(onChange))
	{
		if (!m_pProperty)
		{
			throw Error{ U"Property is nullptr" };
		}
		if (!m_pProperty->isInteractiveProperty())
		{
			throw Error{ U"Property is not interactive" };
		}
	}

	double dialogWidth() const override
	{
		return m_pProperty->editType() == PropertyEditType::LRTB ? 640 : 500;
	}

	Array<DialogButtonDesc> buttonDescs() const override
	{
		return Array<DialogButtonDesc>
		{
			DialogButtonDesc
			{
				.text = U"OK",
				.isDefaultButton = IsDefaultButtonYN::Yes,
			}
		};
	}

	void createDialogContent(const std::shared_ptr<Node>& contentRootNode, const std::shared_ptr<ContextMenu>& dialogContextMenu) override;

	void onResult(StringView) override
	{
		if (m_onChange)
		{
			m_onChange();
		}
	}
};

class Inspector
{
private:
	std::shared_ptr<Canvas> m_canvas;
	std::shared_ptr<Canvas> m_editorCanvas;
	std::shared_ptr<Canvas> m_editorOverlayCanvas;
	std::shared_ptr<Node> m_inspectorFrameNode;
	std::shared_ptr<Node> m_inspectorInnerFrameNode;
	std::shared_ptr<Node> m_inspectorRootNode;
	std::shared_ptr<ContextMenu> m_contextMenu;
	std::shared_ptr<DialogOpener> m_dialogOpener;
	HashTable<PropertyKey, PropertyMetadata> m_propertyMetadata;
	std::weak_ptr<Node> m_targetNode;
	std::function<void()> m_onChangeNodeName;

	IsFoldedYN m_isFoldedConstraint = IsFoldedYN::No;
	IsFoldedYN m_isFoldedNodeSetting = IsFoldedYN::Yes;
	IsFoldedYN m_isFoldedLayout = IsFoldedYN::Yes;
	IsFoldedYN m_isFoldedTransformEffect = IsFoldedYN::Yes;
	Array<std::weak_ptr<ComponentBase>> m_foldedComponents;

	std::shared_ptr<Defaults> m_defaults;
	
	// コンポーネントのコピー用クリップボード
	Optional<JSON> m_copiedComponentJSON;
	Optional<String> m_copiedComponentType;

	template <class TComponent, class... Args>
	void onClickAddComponent(Args&&... args)
	{
		const auto node = m_targetNode.lock();
		if (!node)
		{
			return;
		}
		node->emplaceComponent<TComponent>(std::forward<Args>(args)...);
		refreshInspector();
	}
	
	void onClickCopyComponent(const std::shared_ptr<SerializableComponentBase>& component)
	{
		if (!component)
		{
			return;
		}
		m_copiedComponentJSON = component->toJSON();
		m_copiedComponentType = component->type();
		
		// Inspectorを更新してコンテキストメニューに貼り付けオプションを反映
		refreshInspector();
	}
	
	void onClickPasteComponentTo(const std::shared_ptr<SerializableComponentBase>& component)
	{
		if (!component || !m_copiedComponentJSON.has_value() || !m_copiedComponentType.has_value())
		{
			return;
		}
		// 同じタイプのコンポーネントにのみ貼り付け可能
		if (component->type() != *m_copiedComponentType)
		{
			return;
		}
		component->tryReadFromJSON(*m_copiedComponentJSON);
		refreshInspector();
	}
	
	void onClickPasteComponentAsNew()
	{
		const auto node = m_targetNode.lock();
		if (!node || !m_copiedComponentJSON || !m_copiedComponentType)
		{
			return;
		}
		
		// JSONからコンポーネントを作成
		auto componentJSON = *m_copiedComponentJSON;
		componentJSON[U"type"] = *m_copiedComponentType;
		
		// CreateComponentFromJSONを使用してコンポーネントを作成
		if (const auto component = CreateComponentFromJSON(componentJSON))
		{
			node->addComponent(component);
			refreshInspector();
		}
	}
	

public:
	explicit Inspector(const std::shared_ptr<Canvas>& canvas, const std::shared_ptr<Canvas>& editorCanvas, const std::shared_ptr<Canvas>& editorOverlayCanvas, const std::shared_ptr<ContextMenu>& contextMenu, const std::shared_ptr<Defaults>& defaults, const std::shared_ptr<DialogOpener>& dialogOpener, std::function<void()> onChangeNodeName)
		: m_canvas(canvas)
		, m_editorCanvas(editorCanvas)
		, m_editorOverlayCanvas(editorOverlayCanvas)
		, m_inspectorFrameNode(editorCanvas->rootNode()->emplaceChild(
			U"InspectorFrame",
			AnchorConstraint
			{
				.anchorMin = Anchor::TopRight,
				.anchorMax = Anchor::BottomRight,
				.posDelta = Vec2{ 0, MenuBarHeight + Toolbar::ToolbarHeight },
				.sizeDelta = Vec2{ 400, -(MenuBarHeight + Toolbar::ToolbarHeight) },
				.sizeDeltaPivot = Anchor::TopRight,
			}))
		, m_inspectorInnerFrameNode(m_inspectorFrameNode->emplaceChild(
			U"InspectorInnerFrame",
			AnchorConstraint
			{
				.anchorMin = Anchor::TopLeft,
				.anchorMax = Anchor::BottomRight,
				.posDelta = Vec2{ 0, 0 },
				.sizeDelta = Vec2{ -2, -2 },
				.sizeDeltaPivot = Anchor::MiddleCenter,
			},
			IsHitTargetYN::Yes,
			InheritChildrenStateFlags::Pressed))
		, m_inspectorRootNode(m_inspectorInnerFrameNode->emplaceChild(
			U"Inspector",
			AnchorConstraint
			{
				.anchorMin = Anchor::TopLeft,
				.anchorMax = Anchor::BottomRight,
				.posDelta = Vec2{ 0, 0 },
				.sizeDelta = Vec2{ -10, -10 },
				.sizeDeltaPivot = Anchor::MiddleCenter,
			},
			IsHitTargetYN::Yes))
		, m_contextMenu(contextMenu)
		, m_defaults(defaults)
		, m_dialogOpener(dialogOpener)
		, m_onChangeNodeName(std::move(onChangeNodeName))
		, m_propertyMetadata(InitPropertyMetadata())
	{
		m_inspectorFrameNode->emplaceComponent<RectRenderer>(ColorF{ 0.5, 0.4 }, Palette::Black, 0.0, 10.0);
		m_inspectorInnerFrameNode->emplaceComponent<RectRenderer>(ColorF{ 0.1, 0.8 }, Palette::Black, 0.0, 10.0);
		m_inspectorRootNode->setBoxChildrenLayout(VerticalLayout{ .padding = LRTB{ 0, 0, 4, 4 } });
		m_inspectorRootNode->setVerticalScrollable(true);
	}

	// thisをキャプチャしているのでコピー・ムーブ不可
	Inspector(const Inspector&) = delete;
	Inspector(Inspector&&) = delete;
	Inspector& operator=(const Inspector&) = delete;
	Inspector&& operator=(Inspector&&) = delete;

	void refreshInspector(PreserveScrollYN preserveScroll = PreserveScrollYN::Yes)
	{
		const double scrollY = m_inspectorRootNode->scrollOffset().y;
		setTargetNode(m_targetNode.lock());
		if (preserveScroll)
		{
			m_inspectorRootNode->resetScrollOffset(RecursiveYN::No, RefreshesLayoutYN::No, RefreshesLayoutYN::No);
			m_inspectorRootNode->scroll(Vec2{ 0, scrollY }, RefreshesLayoutYN::No);
		}
		m_editorCanvas->refreshLayout();
	}

	void setTargetNode(const std::shared_ptr<Node>& targetNode)
	{
		if (!targetNode || targetNode.get() != m_targetNode.lock().get())
		{
			// 選択ノードが変更された場合、以前のノード用の折り畳み状況をクリア
			m_foldedComponents.clear();
		}

		m_targetNode = targetNode;

		m_inspectorRootNode->removeChildrenAll();

		if (targetNode)
		{
			const auto nodeNameNode = createNodeNameNode(targetNode);
			m_inspectorRootNode->addChild(nodeNameNode);

			const auto constraintNode = createConstraintNode(targetNode);
			m_inspectorRootNode->addChild(constraintNode);

			const auto nodeSettingNode = createNodeSettingNode(targetNode);
			m_inspectorRootNode->addChild(nodeSettingNode);

			const auto layoutNode = createBoxChildrenLayoutNode(targetNode);
			m_inspectorRootNode->addChild(layoutNode);

			const auto transformEffectNode = createTransformEffectNode(&targetNode->transformEffect());
			m_inspectorRootNode->addChild(transformEffectNode);

			for (const std::shared_ptr<ComponentBase>& component : targetNode->components())
			{
				const IsFoldedYN isFolded{ m_foldedComponents.contains_if([&component](const auto& c) { return c.lock().get() == component.get(); }) };

				if (const auto serializableComponent = std::dynamic_pointer_cast<SerializableComponentBase>(component))
				{
					const auto componentNode = createComponentNode(targetNode, serializableComponent, isFolded,
						[this, componentWeak = std::weak_ptr{ component }](IsFoldedYN isFolded)
						{
							if (isFolded)
							{
								m_foldedComponents.push_back(componentWeak);
							}
							else
							{
								m_foldedComponents.remove_if([&componentWeak](const auto& c) { return c.lock().get() == componentWeak.lock().get(); });
							}
						});
					m_inspectorRootNode->addChild(componentNode);
				}
			}

			// コンポーネント追加メニューを設定
			// 既存のContextMenuOpenerを削除
			m_inspectorInnerFrameNode->removeComponentsIf([](const std::shared_ptr<ComponentBase>& component)
			{
				return std::dynamic_pointer_cast<ContextMenuOpener>(component) != nullptr;
			});
			
			Array<MenuElement> menuElements = {
				MenuItem{ U"Sprite を追加", U"", KeyS, [this] { onClickAddComponent<Sprite>(); } },
				MenuItem{ U"RectRenderer を追加", U"", KeyR, [this] { onClickAddComponent<RectRenderer>(); } },
				MenuItem{ U"TextBox を追加", U"", KeyT, [this] { onClickAddComponent<TextBox>(); } },
				MenuItem{ U"TextArea を追加", U"", KeyA, [this] { onClickAddComponent<TextArea>(); } },
				MenuItem{ U"Label を追加", U"", KeyL, [this] { onClickAddComponent<Label>(); } },
				MenuItem{ U"InputBlocker を追加", U"", KeyI, [this] { onClickAddComponent<InputBlocker>(); } },
				MenuItem{ U"EventTrigger を追加", U"", KeyE, [this] { onClickAddComponent<EventTrigger>(); } },
				MenuItem{ U"Placeholder を追加", U"", KeyP, [this] { onClickAddComponent<Placeholder>(); } },
				MenuItem{ U"CursorChanger を追加", U"", KeyC, [this] { onClickAddComponent<CursorChanger>(); } },
			};
			
			// コピーされたコンポーネントがある場合は貼り付けメニューを追加
			if (m_copiedComponentType)
			{
				menuElements.push_back(MenuSeparator{});
				menuElements.push_back(MenuItem{ U"{} を貼り付け"_fmt(*m_copiedComponentType), U"", KeyV, [this] { onClickPasteComponentAsNew(); } });
			}
			
			m_inspectorInnerFrameNode->emplaceComponent<ContextMenuOpener>(m_contextMenu, menuElements);

			m_inspectorRootNode->addChild(CreateButtonNode(
				U"＋ コンポーネントを追加(A)",
				BoxConstraint
				{
					.sizeDelta = Vec2{ 200, 24 },
					.margin = LRTB{ 0, 0, 24, 24 },
				},
				[this] (const std::shared_ptr<Node>& node)
				{
					m_inspectorInnerFrameNode->getComponent<ContextMenuOpener>()->openManually(node->rect().center());
				}))->addClickHotKey(KeyA, CtrlYN::No, AltYN::Yes, ShiftYN::No, EnabledWhileTextEditingYN::Yes);
		}
	}

	[[nodiscard]]
	static std::shared_ptr<Node> CreateHeadingNode(StringView name, const ColorF& color, IsFoldedYN isFolded, std::function<void(IsFoldedYN)> onToggleFold = nullptr)
	{
		auto headingNode = Node::Create(U"Heading", BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = Vec2{ 0, 24 },
				.margin = LRTB{ 0, 0, 0, 0 },
			});
		headingNode->emplaceComponent<RectRenderer>(
			PropertyValue<ColorF>(ColorF{ color, 0.8 }).withHovered(ColorF{ color + ColorF{ 0.05 }, 0.8 }).withPressed(ColorF{ color - ColorF{ 0.05 }, 0.8 }),
			Palette::Black,
			0.0,
			3.0);
		const auto arrowLabel = headingNode->emplaceComponent<Label>(
			isFolded ? U"▶" : U"▼",
			U"",
			14,
			ColorF{ 1.0, 0.6 },
			HorizontalAlign::Left,
			VerticalAlign::Middle,
			LRTB{ 5, 5, 0, 0 },
			HorizontalOverflow::Wrap,
			VerticalOverflow::Clip);
		headingNode->emplaceComponent<Label>(
			name,
			U"",
			14,
			Palette::White,
			HorizontalAlign::Left,
			VerticalAlign::Middle,
			LRTB{ 25, 5, 0, 0 },
			HorizontalOverflow::Wrap,
			VerticalOverflow::Clip);
		headingNode->addOnClick([arrowLabel, onToggleFold = std::move(onToggleFold)](const std::shared_ptr<Node>& node)
			{
				if (const auto parent = node->parent())
				{
					bool inactiveNodeExists = false;

					// 現在の折り畳み状態
					bool currentlyFolded = false;
					for (const auto& child : parent->children())
					{
						if (child != node)
						{
							currentlyFolded = !child->activeSelf();
							break;
						}
					}
					
					// 折り畳み状態を反転
					const bool willBeFolded = !currentlyFolded;
					inactiveNodeExists = willBeFolded;
					
					// 各子ノードの表示状態を更新
					for (const auto& child : parent->children())
					{
						if (child != node)
						{
							// 保存された可視情報を取得(デフォルトは表示可能)
							const auto visibilityData = child->getStoredDataOr<PropertyVisibilityData>({ .isVisibleByCondition = true });
							
							// 折り畳まれていない場合は、可視条件に従って表示
							// 折り畳まれている場合は、可視条件に関わらず非表示
							if (willBeFolded || !visibilityData.isVisibleByCondition)
							{
								child->setActive(false);
							}
							else
							{
								child->setActive(true);
							}
						}
					}

					// 矢印を回転
					arrowLabel->setText(inactiveNodeExists ? U"▶" : U"▼");

					// 折り畳み時はpaddingを付けない
					LayoutVariant layout = parent->boxChildrenLayout();
					if (auto pVerticalLayout = std::get_if<VerticalLayout>(&layout))
					{
						pVerticalLayout->padding = inactiveNodeExists ? LRTB::Zero() : LRTB{ 0, 0, 0, 8 };
					}
					parent->setBoxChildrenLayout(layout, RefreshesLayoutYN::No);

					// 高さをフィットさせる
					parent->setBoxConstraintToFitToChildren(FitTarget::HeightOnly, RefreshesLayoutYN::Yes);

					// トグル時処理があれば実行
					if (onToggleFold)
					{
						onToggleFold(IsFoldedYN{ inactiveNodeExists });
					}
				}
			});

		return headingNode;
	}

	class PropertyTextBox : public ComponentBase
	{
	private:
		std::shared_ptr<TextBox> m_textBox;
		std::function<void(StringView)> m_fnSetValue;
		std::function<String()> m_fnGetValue;
		String m_prevExternalValue;

		void update(const std::shared_ptr<Node>&) override
		{
			// 外部からの値の変更をチェック
			if (m_fnGetValue)
			{
				const String currentExternalValue = String(m_fnGetValue());
				if (!m_textBox->isEditing() && currentExternalValue != m_prevExternalValue)
				{
					m_textBox->setText(currentExternalValue, IgnoreIsChangedYN::Yes);
					m_prevExternalValue = currentExternalValue;
				}
			}

			// ユーザーによる変更をチェック
			if (m_textBox->isChanged())
			{
				m_fnSetValue(m_textBox->text());
				if (m_fnGetValue)
				{
					m_prevExternalValue = String(m_fnGetValue());
				}
			}
		}

		void draw(const Node&) const override
		{
		}

	public:
		explicit PropertyTextBox(const std::shared_ptr<TextBox>& textBox, std::function<void(StringView)> fnSetValue, std::function<String()> fnGetValue = nullptr)
			: ComponentBase{ {} }
			, m_textBox(textBox)
			, m_fnSetValue(std::move(fnSetValue))
			, m_fnGetValue(std::move(fnGetValue))
			, m_prevExternalValue(fnGetValue ? String(fnGetValue()) : U"")
		{
		}
	};

	[[nodiscard]]
	static std::shared_ptr<Node> CreateNodeNameTextboxNode(StringView name, StringView value, std::function<void(StringView)> fnSetValue)
	{
		const auto propertyNode = Node::Create(
			name,
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = Vec2{ -24, 32 },
			},
			IsHitTargetYN::Yes);
		const auto textBoxNode = propertyNode->emplaceChild(
			U"TextBox",
			AnchorConstraint
			{
				.anchorMin = Anchor::MiddleLeft,
				.anchorMax = Anchor::MiddleRight,
				.posDelta = Vec2{ 0, 0 },
				.sizeDelta = Vec2{ -16, 26 },
				.sizeDeltaPivot = Anchor::MiddleCenter,
			});
		textBoxNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textBox = textBoxNode->emplaceComponent<TextBox>(U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textBox->setText(value, IgnoreIsChangedYN::Yes);
		textBoxNode->addComponent(std::make_shared<PropertyTextBox>(textBox, std::move(fnSetValue)));
		textBoxNode->addClickHotKey(KeyF2);
		return propertyNode;
	}

	[[nodiscard]]
	std::shared_ptr<Node> createPropertyNodeWithTooltip(StringView componentName, StringView propertyName, StringView value, std::function<void(StringView)> fnSetValue, HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No, std::function<String()> fnGetValue = nullptr)
	{
		// メタデータをチェックしてTextAreaを使うかどうか判断
		std::shared_ptr<Node> propertyNode;
		if (const auto it = m_propertyMetadata.find(PropertyKey{ String(componentName), String(propertyName) }); it != m_propertyMetadata.end())
		{
			const auto& metadata = it->second;
			if (metadata.numTextAreaLines.has_value())
			{
				// TextAreaを使用
				propertyNode = CreatePropertyNodeWithTextArea(propertyName, value, std::move(fnSetValue), hasInteractivePropertyValue, *metadata.numTextAreaLines, std::move(fnGetValue));
			}
			else
			{
				// 通常のTextBoxを使用
				propertyNode = CreatePropertyNode(propertyName, value, std::move(fnSetValue), hasInteractivePropertyValue, std::move(fnGetValue));
			}
			
			// ツールチップを追加
			if (metadata.tooltip)
			{
				if (const auto labelNode = propertyNode->getChildByNameOrNull(U"Label", RecursiveYN::Yes))
				{
					labelNode->emplaceComponent<::TooltipOpener>(m_editorOverlayCanvas, *metadata.tooltip, metadata.tooltipDetail.value_or(U""));
				}
			}
		}
		else
		{
			// メタデータがない場合は通常のTextBoxを使用
			propertyNode = CreatePropertyNode(propertyName, value, std::move(fnSetValue), hasInteractivePropertyValue, std::move(fnGetValue));
		}
		
		return propertyNode;
	}


	[[nodiscard]]
	std::shared_ptr<Node> createVec2PropertyNodeWithTooltip(StringView componentName, StringView propertyName, const Vec2& currentValue, std::function<void(const Vec2&)> fnSetValue, HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No)
	{
		const auto propertyNode = CreateVec2PropertyNode(propertyName, currentValue, std::move(fnSetValue), hasInteractivePropertyValue);
		
		// メタデータに基づいてツールチップを追加
		if (const auto it = m_propertyMetadata.find(PropertyKey{ String(componentName), String(propertyName) }); it != m_propertyMetadata.end())
		{
			const auto& metadata = it->second;
			if (metadata.tooltip)
			{
				if (const auto labelNode = propertyNode->getChildByNameOrNull(U"Label", RecursiveYN::Yes))
				{
					labelNode->emplaceComponent<::TooltipOpener>(m_editorOverlayCanvas, *metadata.tooltip, metadata.tooltipDetail.value_or(U""));
				}
			}
		}
		
		return propertyNode;
	}

	[[nodiscard]]
	std::shared_ptr<Node> createEnumPropertyNodeWithTooltip(StringView componentName, StringView propertyName, StringView value, std::function<void(StringView)> fnSetValue, const std::shared_ptr<ContextMenu>& contextMenu, const Array<String>& enumValues, HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No)
	{
		const auto propertyNode = CreateEnumPropertyNode(propertyName, value, std::move(fnSetValue), contextMenu, enumValues, hasInteractivePropertyValue);
		
		// メタデータに基づいてツールチップを追加
		if (const auto it = m_propertyMetadata.find(PropertyKey{ String(componentName), String(propertyName) }); it != m_propertyMetadata.end())
		{
			const auto& metadata = it->second;
			if (metadata.tooltip)
			{
				if (const auto labelNode = propertyNode->getChildByNameOrNull(U"Label", RecursiveYN::Yes))
				{
					labelNode->emplaceComponent<::TooltipOpener>(m_editorOverlayCanvas, *metadata.tooltip, metadata.tooltipDetail.value_or(U""));
				}
			}
		}
		
		return propertyNode;
	}

	[[nodiscard]]
	std::shared_ptr<Node> createLRTBPropertyNodeWithTooltip(StringView componentName, StringView propertyName, const LRTB& currentValue, std::function<void(const LRTB&)> fnSetValue, HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No)
	{
		const auto propertyNode = CreateLRTBPropertyNode(propertyName, currentValue, std::move(fnSetValue), hasInteractivePropertyValue);
		
		// メタデータに基づいてツールチップを追加
		if (const auto it = m_propertyMetadata.find(PropertyKey{ String(componentName), String(propertyName) }); it != m_propertyMetadata.end())
		{
			const auto& metadata = it->second;
			if (metadata.tooltip)
			{
				// Line1とLine2の両方のLabelノードにツールチップを追加
				if (const auto line1 = propertyNode->getChildByNameOrNull(U"Line1", RecursiveYN::No))
				{
					if (const auto labelNode = line1->getChildByNameOrNull(U"Label", RecursiveYN::No))
					{
						labelNode->emplaceComponent<::TooltipOpener>(m_editorOverlayCanvas, *metadata.tooltip, metadata.tooltipDetail.value_or(U""));
					}
				}
				if (const auto line2 = propertyNode->getChildByNameOrNull(U"Line2", RecursiveYN::No))
				{
					if (const auto labelNode = line2->getChildByNameOrNull(U"Label", RecursiveYN::No))
					{
						labelNode->emplaceComponent<::TooltipOpener>(m_editorOverlayCanvas, *metadata.tooltip, metadata.tooltipDetail.value_or(U""));
					}
				}
			}
		}
		
		return propertyNode;
	}

	[[nodiscard]]
	std::shared_ptr<Node> createBoolPropertyNodeWithTooltip(StringView componentName, StringView propertyName, bool currentValue, std::function<void(bool)> fnSetValue, HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No)
	{
		const auto propertyNode = CreateBoolPropertyNode(propertyName, currentValue, std::move(fnSetValue), hasInteractivePropertyValue);
		
		// メタデータに基づいてツールチップを追加
		if (const auto it = m_propertyMetadata.find(PropertyKey{ String(componentName), String(propertyName) }); it != m_propertyMetadata.end())
		{
			const auto& metadata = it->second;
			if (metadata.tooltip)
			{
				// boolプロパティの場合は、propertyNode全体にツールチップを追加
				propertyNode->emplaceComponent<::TooltipOpener>(m_editorOverlayCanvas, *metadata.tooltip, metadata.tooltipDetail.value_or(U""));
			}
		}
		
		return propertyNode;
	}

	[[nodiscard]]
	std::shared_ptr<Node> createColorPropertyNodeWithTooltip(StringView componentName, StringView propertyName, const ColorF& currentValue, std::function<void(const ColorF&)> fnSetValue, HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No)
	{
		const auto propertyNode = CreateColorPropertyNode(propertyName, currentValue, std::move(fnSetValue), hasInteractivePropertyValue);
		
		// メタデータに基づいてツールチップを追加
		if (const auto it = m_propertyMetadata.find(PropertyKey{ String(componentName), String(propertyName) }); it != m_propertyMetadata.end())
		{
			const auto& metadata = it->second;
			if (metadata.tooltip)
			{
				if (const auto labelNode = propertyNode->getChildByNameOrNull(U"Label", RecursiveYN::Yes))
				{
					labelNode->emplaceComponent<::TooltipOpener>(m_editorOverlayCanvas, *metadata.tooltip, metadata.tooltipDetail.value_or(U""));
				}
			}
		}
		
		return propertyNode;
	}

	[[nodiscard]]
	static std::shared_ptr<Node> CreatePropertyNode(StringView name, StringView value, std::function<void(StringView)> fnSetValue, HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No, std::function<String()> fnGetValue = nullptr)
	{
		const auto propertyNode = Node::Create(
			name,
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = Vec2{ 0, 32 },
			},
			IsHitTargetYN::Yes,
			InheritChildrenStateFlags::Hovered);
		propertyNode->setBoxChildrenLayout(HorizontalLayout{ .padding = LRTB{ 10, 8, 0, 0 } });
		propertyNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }), Palette::Black, 0.0, 3.0);
		const auto labelNode = propertyNode->emplaceChild(
			U"Label",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0, 1 },
				.flexibleWeight = 0.85,
			});
		labelNode->emplaceComponent<Label>(
			name,
			U"",
			14,
			Palette::White,
			HorizontalAlign::Left,
			VerticalAlign::Middle,
			LRTB{ 5, 5, 5, 5 },
			HorizontalOverflow::Wrap,
			VerticalOverflow::Clip,
			Vec2::Zero(),
			hasInteractivePropertyValue ? LabelUnderlineStyle::Solid : LabelUnderlineStyle::None,
			ColorF{ Palette::Yellow, 0.5 },
			2.0,
			LabelSizingMode::ShrinkToFit,
			8.0);
		const auto textBoxNode = propertyNode->emplaceChild(
			U"TextBox",
			BoxConstraint
			{
				.sizeDelta = Vec2{ 0, 26 },
				.flexibleWeight = 1,
			});
		textBoxNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textBox = textBoxNode->emplaceComponent<TextBox>(U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textBox->setText(value, IgnoreIsChangedYN::Yes);
		textBoxNode->addComponent(std::make_shared<PropertyTextBox>(textBox, std::move(fnSetValue), std::move(fnGetValue)));
		return propertyNode;
	}

	[[nodiscard]]
	static std::shared_ptr<Node> CreatePropertyNodeWithTextArea(StringView name, StringView value, std::function<void(StringView)> fnSetValue, HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No, int32 numLines = 3, std::function<String()> fnGetValue = nullptr)
	{
		const double textAreaHeight = numLines * 20.0 + 14.0;
		const double nodeHeight = textAreaHeight + 6.0;
		
		const auto propertyNode = Node::Create(
			name,
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = Vec2{ 0, nodeHeight },
			},
			IsHitTargetYN::Yes,
			InheritChildrenStateFlags::Hovered);
		propertyNode->setBoxChildrenLayout(HorizontalLayout{ .padding = LRTB{ 10, 8, 0, 0 } });
		propertyNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }), Palette::Black, 0.0, 3.0);
		const auto labelNode = propertyNode->emplaceChild(
			U"Label",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0, 1 },
				.flexibleWeight = 0.85,
			});
		labelNode->emplaceComponent<Label>(
			name,
			U"",
			14,
			Palette::White,
			HorizontalAlign::Left,
			VerticalAlign::Middle,
			LRTB{ 5, 5, 5, 5 },
			HorizontalOverflow::Wrap,
			VerticalOverflow::Clip,
			Vec2::Zero(),
			hasInteractivePropertyValue ? LabelUnderlineStyle::Solid : LabelUnderlineStyle::None,
			ColorF{ Palette::Yellow, 0.5 },
			2.0,
			LabelSizingMode::ShrinkToFit,
			8.0);
		const auto textAreaNode = propertyNode->emplaceChild(
			U"TextArea",
			BoxConstraint
			{
				.sizeDelta = Vec2{ 0, textAreaHeight },
				.flexibleWeight = 1,
			});
		textAreaNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textArea = textAreaNode->emplaceComponent<TextArea>(U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textArea->setText(value, IgnoreIsChangedYN::Yes);
		
		// PropertyTextAreaコンポーネントを追加
		class PropertyTextArea : public ComponentBase
		{
		private:
			std::shared_ptr<TextArea> m_textArea;
			std::function<void(StringView)> m_fnSetValue;
			std::function<String()> m_fnGetValue;
			String m_prevExternalValue;

		public:
			PropertyTextArea(const std::shared_ptr<TextArea>& textArea, std::function<void(StringView)> fnSetValue, std::function<String()> fnGetValue = nullptr)
				: ComponentBase{ {} }
				, m_textArea{ textArea }
				, m_fnSetValue{ std::move(fnSetValue) }
				, m_fnGetValue{ std::move(fnGetValue) }
				, m_prevExternalValue{ fnGetValue ? String(fnGetValue()) : U"" }
			{
			}

			void update(const std::shared_ptr<Node>&) override
			{
				// 外部からの値の変更をチェック
				if (m_fnGetValue)
				{
					const String currentExternalValue = String(m_fnGetValue());
					if (!m_textArea->isEditing() && currentExternalValue != m_prevExternalValue)
					{
						m_textArea->setText(currentExternalValue, IgnoreIsChangedYN::Yes);
						m_prevExternalValue = currentExternalValue;
					}
				}

				// ユーザーによる変更をチェック
				if (m_textArea->isChanged())
				{
					m_fnSetValue(m_textArea->text());
					if (m_fnGetValue)
					{
						m_prevExternalValue = String(m_fnGetValue());
					}
				}
			}

			void draw(const Node&) const override
			{
			}
		};
		
		textAreaNode->addComponent(std::make_shared<PropertyTextArea>(textArea, std::move(fnSetValue), std::move(fnGetValue)));
		return propertyNode;
	}

	[[nodiscard]]
	static std::shared_ptr<Node> CreateVec2PropertyNode(
		StringView name,
		const Vec2& currentValue,
		std::function<void(const Vec2&)> fnSetValue,
		HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No)
	{
		const auto propertyNode = Node::Create(
			name,
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = Vec2{ 0, 32 },
			},
			IsHitTargetYN::Yes,
			InheritChildrenStateFlags::Hovered);
		propertyNode->setBoxChildrenLayout(HorizontalLayout{ .padding = LRTB{ 10, 8, 0, 0 } });
		propertyNode->emplaceComponent<RectRenderer>(
			PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }),
			Palette::Black,
			0.0,
			3.0);

		const auto labelNode = propertyNode->emplaceChild(
			U"Label",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0, 1 },
				.flexibleWeight = 0.85,
			});
		labelNode->emplaceComponent<Label>(
			name,
			U"",
			14,
			Palette::White,
			HorizontalAlign::Left,
			VerticalAlign::Middle,
			LRTB{ 5, 5, 5, 5 },
			HorizontalOverflow::Wrap,
			VerticalOverflow::Clip,
			Vec2::Zero(),
			hasInteractivePropertyValue ? LabelUnderlineStyle::Solid : LabelUnderlineStyle::None,
			ColorF{ Palette::Yellow, 0.5 },
			2.0,
			LabelSizingMode::ShrinkToFit,
			8.0);

		const auto textBoxParentNode = propertyNode->emplaceChild(
			U"TextBoxParent",
			BoxConstraint
			{
				.sizeDelta = Vec2{ 0, 26 },
				.flexibleWeight = 1,
			},
			IsHitTargetYN::No,
			InheritChildrenStateFlags::Hovered);
		textBoxParentNode->setBoxChildrenLayout(HorizontalLayout{});

		// X
		const auto textBoxXNode = textBoxParentNode->emplaceChild(
			U"TextBoxX",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0, 1 },
				.flexibleWeight = 1,
				.margin = LRTB{ 0, 2, 0, 0 },
			});
		textBoxXNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textBoxX = textBoxXNode->emplaceComponent<TextBox>(
			U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textBoxX->setText(Format(currentValue.x), IgnoreIsChangedYN::Yes);

		// Y
		const auto textBoxYNode = textBoxParentNode->emplaceChild(
			U"TextBoxY",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0, 1 },
				.flexibleWeight = 1,
				.margin = LRTB{ 2, 0, 0, 0 },
			});
		textBoxYNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textBoxY = textBoxYNode->emplaceComponent<TextBox>(
			U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textBoxY->setText(Format(currentValue.y), IgnoreIsChangedYN::Yes);

		class Vec2PropertyTextBox : public ComponentBase
		{
		private:
			std::shared_ptr<TextBox> m_textBoxX;
			std::shared_ptr<TextBox> m_textBoxY;
			std::function<void(const Vec2&)> m_fnSetValue;
			Vec2 m_prevValue;

		public:
			Vec2PropertyTextBox(
				const std::shared_ptr<TextBox>& textBoxX,
				const std::shared_ptr<TextBox>& textBoxY,
				std::function<void(const Vec2&)> fnSetValue,
				const Vec2& initialValue)
				: ComponentBase{ {} }
				, m_textBoxX(textBoxX)
				, m_textBoxY(textBoxY)
				, m_fnSetValue(std::move(fnSetValue))
				, m_prevValue(initialValue)
			{
			}

			void update(const std::shared_ptr<Node>&) override
			{
				const double x = ParseOpt<double>(m_textBoxX->text()).value_or(m_prevValue.x);
				const double y = ParseOpt<double>(m_textBoxY->text()).value_or(m_prevValue.y);

				const Vec2 newValue{ x, y };
				if (newValue != m_prevValue)
				{
					m_prevValue = newValue;
					if (m_fnSetValue)
					{
						m_fnSetValue(newValue);
					}
				}
			}

			void draw(const Node&) const override
			{
			}
		};

		propertyNode->addComponent(std::make_shared<Vec2PropertyTextBox>(
			textBoxX,
			textBoxY,
			fnSetValue,
			currentValue
		));

		return propertyNode;
	}

	[[nodiscard]]
	static std::shared_ptr<Node> CreateVec4PropertyNode(
		StringView name,
		const Vec4& currentValue,
		std::function<void(const Vec4&)> fnSetValue,
		HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No)
	{
		const auto propertyNode = Node::Create(
			name,
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = Vec2{ 0, 32 },
			},
			IsHitTargetYN::Yes,
			InheritChildrenStateFlags::Hovered);
		propertyNode->emplaceComponent<RectRenderer>(
			PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }),
			Palette::Black,
			0.0,
			3.0);

		const auto labelNode = propertyNode->emplaceChild(
			U"Label",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0, 1 },
				.flexibleWeight = 0.85,
			});
		labelNode->emplaceComponent<Label>(
			name,
			U"",
			14,
			Palette::White,
			HorizontalAlign::Left,
			VerticalAlign::Middle,
			LRTB{ 5, 5, 5, 5 },
			HorizontalOverflow::Wrap,
			VerticalOverflow::Clip,
			Vec2::Zero(),
			hasInteractivePropertyValue ? LabelUnderlineStyle::Solid : LabelUnderlineStyle::None,
			ColorF{ Palette::Yellow, 0.5 },
			2.0,
			LabelSizingMode::ShrinkToFit,
			8.0);

		const auto textBoxParentNode = propertyNode->emplaceChild(
			U"TextBoxParent",
			BoxConstraint
			{
				.sizeDelta = Vec2{ 0, 26 },
				.flexibleWeight = 1,
			},
			IsHitTargetYN::No,
			InheritChildrenStateFlags::Hovered);
		textBoxParentNode->setBoxChildrenLayout(HorizontalLayout{});

		// X
		const auto textBoxXNode = textBoxParentNode->emplaceChild(
			U"TextBoxX",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0, 1 },
				.flexibleWeight = 1,
				.margin = LRTB{ 0, 2, 0, 0 },
			});
		textBoxXNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textBoxX = textBoxXNode->emplaceComponent<TextBox>(
			U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textBoxX->setText(Format(currentValue.x), IgnoreIsChangedYN::Yes);

		// Y
		const auto textBoxYNode = textBoxParentNode->emplaceChild(
			U"TextBoxY",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0, 1 },
				.flexibleWeight = 1,
				.margin = LRTB{ 2, 2, 0, 0 },
			});
		textBoxYNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textBoxY = textBoxYNode->emplaceComponent<TextBox>(
			U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textBoxY->setText(Format(currentValue.y), IgnoreIsChangedYN::Yes);

		// Z
		const auto textBoxZNode = textBoxParentNode->emplaceChild(
			U"TextBoxZ",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0, 1 },
				.flexibleWeight = 1,
				.margin = LRTB{ 2, 2, 0, 0 },
			});
		textBoxZNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textBoxZ = textBoxZNode->emplaceComponent<TextBox>(
			U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textBoxZ->setText(Format(currentValue.z), IgnoreIsChangedYN::Yes);

		// W
		const auto textBoxWNode = textBoxParentNode->emplaceChild(
			U"TextBoxW",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0, 1 },
				.flexibleWeight = 1,
				.margin = LRTB{ 2, 0, 0, 0 },
			});
		textBoxWNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textBoxW = textBoxWNode->emplaceComponent<TextBox>(
			U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textBoxW->setText(Format(currentValue.w), IgnoreIsChangedYN::Yes);

		class Vec4PropertyTextBox : public ComponentBase
		{
		private:
			std::shared_ptr<TextBox> m_textBoxX;
			std::shared_ptr<TextBox> m_textBoxY;
			std::shared_ptr<TextBox> m_textBoxZ;
			std::shared_ptr<TextBox> m_textBoxW;
			std::function<void(const Vec4&)> m_fnSetValue;
			Vec4 m_prevValue;

		public:
			Vec4PropertyTextBox(
				const std::shared_ptr<TextBox>& textBoxX,
				const std::shared_ptr<TextBox>& textBoxY,
				const std::shared_ptr<TextBox>& textBoxZ,
				const std::shared_ptr<TextBox>& textBoxW,
				std::function<void(const Vec4&)> fnSetValue,
				const Vec4& initialValue)
				: ComponentBase{ {} }
				, m_textBoxX(textBoxX)
				, m_textBoxY(textBoxY)
				, m_textBoxZ(textBoxZ)
				, m_textBoxW(textBoxW)
				, m_fnSetValue(std::move(fnSetValue))
				, m_prevValue(initialValue)
			{
			}

			void update(const std::shared_ptr<Node>&) override
			{
				const double x = ParseOpt<double>(m_textBoxX->text()).value_or(m_prevValue.x);
				const double y = ParseOpt<double>(m_textBoxY->text()).value_or(m_prevValue.y);
				const double z = ParseOpt<double>(m_textBoxZ->text()).value_or(m_prevValue.z);
				const double w = ParseOpt<double>(m_textBoxW->text()).value_or(m_prevValue.w);

				const Vec4 newValue{ x, y, z, w };
				if (newValue != m_prevValue)
				{
					m_prevValue = newValue;
					if (m_fnSetValue)
					{
						m_fnSetValue(newValue);
					}
				}
			}

			void draw(const Node&) const override
			{
			}
		};

		propertyNode->addComponent(std::make_shared<Vec4PropertyTextBox>(
			textBoxX,
			textBoxY,
			textBoxZ,
			textBoxW,
			fnSetValue,
			currentValue));

		return propertyNode;
	}

	[[nodiscard]]
	static std::shared_ptr<Node> CreateLRTBPropertyNode(
		StringView name,
		const LRTB& currentValue,
		std::function<void(const LRTB&)> fnSetValue,
		HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No)
	{
		constexpr int32 LineHeight = 32;
		const auto propertyNode = Node::Create(
			name,
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = Vec2{ 0, LineHeight * 2 },
			},
			IsHitTargetYN::Yes,
			InheritChildrenStateFlags::Hovered);
		propertyNode->setBoxChildrenLayout(VerticalLayout{ .padding = LRTB{ 10, 8, 0, 0 } });
		propertyNode->emplaceComponent<RectRenderer>(
			PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }),
			Palette::Black,
			0.0,
			3.0);

		const auto line1 = propertyNode->emplaceChild(
			U"Line1",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.flexibleWeight = 1,
			},
			IsHitTargetYN::No,
			InheritChildrenStateFlags::Hovered);
		line1->setBoxChildrenLayout(HorizontalLayout{});

		const auto line1LabelNode =
			line1->emplaceChild(
				U"Label",
				BoxConstraint
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 0.85,
				});
		line1LabelNode->emplaceComponent<Label>(
			U"{} (L, R)"_fmt(name),
			U"",
			14,
			Palette::White,
			HorizontalAlign::Left,
			VerticalAlign::Middle,
			LRTB{ 5, 5, 5, 5 },
			HorizontalOverflow::Wrap,
			VerticalOverflow::Clip,
			Vec2::Zero(),
			hasInteractivePropertyValue ? LabelUnderlineStyle::Solid : LabelUnderlineStyle::None,
			ColorF{ Palette::Yellow, 0.5 },
			2.0,
			LabelSizingMode::ShrinkToFit,
			8.0);

		const auto line1TextBoxParentNode = line1->emplaceChild(
			U"TextBoxParent",
			BoxConstraint
			{
				.sizeDelta = Vec2{ 0, 26 },
				.flexibleWeight = 1,
			},
			IsHitTargetYN::No,
			InheritChildrenStateFlags::Hovered);
		line1TextBoxParentNode->setBoxChildrenLayout(HorizontalLayout{});

		// L
		const auto textBoxLNode = line1TextBoxParentNode->emplaceChild(
			U"TextBoxL",
			BoxConstraint
			{
				.sizeDelta = Vec2{ 0, 26 },
				.flexibleWeight = 1,
				.margin = LRTB{ 0, 2, 0, 6 },
			});
		textBoxLNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textBoxL = textBoxLNode->emplaceComponent<TextBox>(
			U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textBoxL->setText(Format(currentValue.left), IgnoreIsChangedYN::Yes);

		// R
		const auto textBoxRNode = line1TextBoxParentNode->emplaceChild(
			U"TextBoxR",
			BoxConstraint
			{
				.sizeDelta = Vec2{ 0, 26 },
				.flexibleWeight = 1,
				.margin = LRTB{ 2, 0, 0, 6 },
			});
		textBoxRNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textBoxR = textBoxRNode->emplaceComponent<TextBox>(
			U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textBoxR->setText(Format(currentValue.right), IgnoreIsChangedYN::Yes);

		const auto line2 = propertyNode->emplaceChild(
			U"Line2",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.flexibleWeight = 1,
			},
			IsHitTargetYN::No,
			InheritChildrenStateFlags::Hovered);
		line2->setBoxChildrenLayout(HorizontalLayout{});

		const auto line2LabelNode =
			line2->emplaceChild(
				U"Label",
				BoxConstraint
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 0.85,
				});
		line2LabelNode->emplaceComponent<Label>(
			U"{} (T, B)"_fmt(name),
			U"",
			14,
			Palette::White,
			HorizontalAlign::Left,
			VerticalAlign::Middle,
			LRTB{ 5, 5, 5, 5 },
			HorizontalOverflow::Wrap,
			VerticalOverflow::Clip,
			Vec2::Zero(),
			hasInteractivePropertyValue ? LabelUnderlineStyle::Solid : LabelUnderlineStyle::None,
			ColorF{ Palette::Yellow, 0.5 },
			2.0,
			LabelSizingMode::ShrinkToFit,
			8.0);

		const auto line2TextBoxParentNode = line2->emplaceChild(
			U"TextBoxParent",
			BoxConstraint
			{
				.sizeDelta = Vec2{ 0, 26 },
				.flexibleWeight = 1,
			},
			IsHitTargetYN::No,
			InheritChildrenStateFlags::Hovered);
		line2TextBoxParentNode->setBoxChildrenLayout(HorizontalLayout{});

		// T
		const auto textBoxTNode = line2TextBoxParentNode->emplaceChild(
			U"TextBoxT",
			BoxConstraint
			{
				.sizeDelta = Vec2{ 0, 26 },
				.flexibleWeight = 1,
				.margin = LRTB{ 0, 2, 0, 0 },
			});
		textBoxTNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textBoxT = textBoxTNode->emplaceComponent<TextBox>(
			U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textBoxT->setText(Format(currentValue.top), IgnoreIsChangedYN::Yes);

		// B
		const auto textBoxBNode = line2TextBoxParentNode->emplaceChild(
			U"TextBoxB",
			BoxConstraint
			{
				.sizeDelta = Vec2{ 0, 26 },
				.flexibleWeight = 1,
				.margin = LRTB{ 2, 0, 0, 0 },
			});
		textBoxBNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textBoxB = textBoxBNode->emplaceComponent<TextBox>(
			U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textBoxB->setText(Format(currentValue.bottom), IgnoreIsChangedYN::Yes);

		class LRTBPropertyTextBox : public ComponentBase
		{
		private:
			std::shared_ptr<TextBox> m_textBoxL;
			std::shared_ptr<TextBox> m_textBoxR;
			std::shared_ptr<TextBox> m_textBoxT;
			std::shared_ptr<TextBox> m_textBoxB;
			std::function<void(const LRTB&)> m_fnSetValue;
			LRTB m_prevValue;

		public:
			LRTBPropertyTextBox(
				const std::shared_ptr<TextBox>& textBoxL,
				const std::shared_ptr<TextBox>& textBoxR,
				const std::shared_ptr<TextBox>& textBoxT,
				const std::shared_ptr<TextBox>& textBoxB,
				std::function<void(const LRTB&)> fnSetValue,
				const LRTB& initialValue)
				: ComponentBase{ {} }
				, m_textBoxL(textBoxL)
				, m_textBoxR(textBoxR)
				, m_textBoxT(textBoxT)
				, m_textBoxB(textBoxB)
				, m_fnSetValue(std::move(fnSetValue))
				, m_prevValue(initialValue)
			{
			}

			void update(const std::shared_ptr<Node>&) override
			{
				const double l = ParseOpt<double>(m_textBoxL->text()).value_or(m_prevValue.left);
				const double r = ParseOpt<double>(m_textBoxR->text()).value_or(m_prevValue.right);
				const double t = ParseOpt<double>(m_textBoxT->text()).value_or(m_prevValue.top);
				const double b = ParseOpt<double>(m_textBoxB->text()).value_or(m_prevValue.bottom);
				const LRTB newValue{ l, r, t, b };
				if (newValue != m_prevValue)
				{
					m_prevValue = newValue;
					if (m_fnSetValue)
					{
						m_fnSetValue(newValue);
					}
				}
			}

			void draw(const Node&) const override
			{
			}
		};

		propertyNode->addComponent(std::make_shared<LRTBPropertyTextBox>(
			textBoxL,
			textBoxR,
			textBoxT,
			textBoxB,
			fnSetValue,
			currentValue));

		return propertyNode;
	}

	[[nodiscard]]
	static std::shared_ptr<Node> CreateColorPropertyNode(
		StringView name,
		const ColorF& currentValue,
		std::function<void(const ColorF&)> fnSetValue,
		HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No)
	{
		const auto propertyNode = Node::Create(
			name,
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = Vec2{ 0, 36 },
			},
			IsHitTargetYN::Yes,
			InheritChildrenStateFlags::Hovered);
		propertyNode->setBoxChildrenLayout(HorizontalLayout{ .padding = LRTB{ 10, 8, 0, 0 } });
		propertyNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }), Palette::Black, 0.0, 3.0);

		const auto labelNode = propertyNode->emplaceChild(
			U"Label",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0, 1 },
				.flexibleWeight = 0.85,
			});
		labelNode->emplaceComponent<Label>(
			name,
			U"",
			14,
			Palette::White,
			HorizontalAlign::Left,
			VerticalAlign::Middle,
			LRTB{ 5, 5, 5, 5 },
			HorizontalOverflow::Wrap,
			VerticalOverflow::Clip,
			Vec2::Zero(),
			hasInteractivePropertyValue ? LabelUnderlineStyle::Solid : LabelUnderlineStyle::None,
			ColorF{ Palette::Yellow, 0.5 },
			2.0,
			LabelSizingMode::ShrinkToFit,
			8.0);

		const auto rowNode = propertyNode->emplaceChild(
			U"ColorPropertyRow",
			BoxConstraint
			{
				.sizeDelta = Vec2{ 0, 26 },
				.flexibleWeight = 1,
			},
			IsHitTargetYN::No,
			InheritChildrenStateFlags::Hovered);
		rowNode->setBoxChildrenLayout(HorizontalLayout{});

		const auto previewRootNode = rowNode->emplaceChild(
			U"ColorPreviewRoot",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0, 1 },
				.sizeDelta = Vec2{ 26, 0 },
				.margin = LRTB{ 0, 2, 0, 0 },
			},
			IsHitTargetYN::No);

		// 透明の市松模様
		constexpr int32 GridSize = 3;
		for (int32 y = 0; y < GridSize; ++y)
		{
			for (int32 x = 0; x < GridSize; ++x)
			{
				const bool isOdd = (x + y) % 2 == 1;
				const auto previewNode = previewRootNode->emplaceChild(
					U"Transparent",
					AnchorConstraint
					{
						.anchorMin = { static_cast<double>(x) / GridSize, static_cast<double>(y) / GridSize },
						.anchorMax = { static_cast<double>(x + 1) / GridSize, static_cast<double>(y + 1) / GridSize },
						.sizeDeltaPivot = Anchor::TopLeft,
					},
					IsHitTargetYN::No)
					->emplaceComponent<RectRenderer>(ColorF{ isOdd ? 0.9 : 1.0 });
			}
		}

		// 色プレビュー
		const auto previewNode = previewRootNode->emplaceChild(
			U"ColorPreview",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0, 1 },
				.sizeDelta = Vec2{ 26, 0 },
				.margin = LRTB{ 0, 2, 0, 0 },
			},
			IsHitTargetYN::No);
		const auto previewRectRenderer = previewNode->emplaceComponent<RectRenderer>(currentValue, ColorF{ 1.0, 0.3 }, 1.0, 0.0);

		const auto textBoxParentNode = rowNode->emplaceChild(
			U"TextBoxParent",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0, 1 },
				.flexibleWeight = 1,
			},
			IsHitTargetYN::No,
			InheritChildrenStateFlags::Hovered);

		// R
		const auto textBoxRNode = textBoxParentNode->emplaceChild(
			U"TextBoxR",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0, 1 },
				.flexibleWeight = 1,
				.margin = LRTB{ 2, 2, 0, 0 },
			});
		textBoxRNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textBoxR = textBoxRNode->emplaceComponent<TextBox>(
			U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textBoxR->setText(Format(currentValue.r), IgnoreIsChangedYN::Yes);

		// G
		const auto textBoxGNode = textBoxParentNode->emplaceChild(
			U"TextBoxG",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0, 1 },
				.flexibleWeight = 1,
				.margin = LRTB{ 2, 2, 0, 0 },
			});
		textBoxGNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textBoxG = textBoxGNode->emplaceComponent<TextBox>(
			U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textBoxG->setText(Format(currentValue.g), IgnoreIsChangedYN::Yes);

		// B
		const auto textBoxBNode = textBoxParentNode->emplaceChild(
			U"TextBoxB",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0, 1 },
				.flexibleWeight = 1,
				.margin = LRTB{ 2, 2, 0, 0 },
			});
		textBoxBNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textBoxB = textBoxBNode->emplaceComponent<TextBox>(
			U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textBoxB->setText(Format(currentValue.b), IgnoreIsChangedYN::Yes);

		// A
		const auto textBoxANode = textBoxParentNode->emplaceChild(
			U"TextBoxA",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0, 1 },
				.flexibleWeight = 1,
				.margin = LRTB{ 2, 0, 0, 0 },
			});
		textBoxANode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textBoxA = textBoxANode->emplaceComponent<TextBox>(
			U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textBoxA->setText(Format(currentValue.a), IgnoreIsChangedYN::Yes);

		class ColorPropertyTextBox : public ComponentBase
		{
		private:
			std::shared_ptr<TextBox> m_textBoxR;
			std::shared_ptr<TextBox> m_textBoxG;
			std::shared_ptr<TextBox> m_textBoxB;
			std::shared_ptr<TextBox> m_textBoxA;
			std::shared_ptr<RectRenderer> m_previewRect;
			std::function<void(const ColorF&)> m_fnSetValue;

			// 前回の色
			ColorF m_prevColor;

		public:
			ColorPropertyTextBox(
				const std::shared_ptr<TextBox>& r,
				const std::shared_ptr<TextBox>& g,
				const std::shared_ptr<TextBox>& b,
				const std::shared_ptr<TextBox>& a,
				const std::shared_ptr<RectRenderer>& previewRect,
				std::function<void(const ColorF&)> fnSetValue,
				const ColorF& initialColor)
				: ComponentBase{ {} }
				, m_textBoxR(r)
				, m_textBoxG(g)
				, m_textBoxB(b)
				, m_textBoxA(a)
				, m_previewRect(previewRect)
				, m_fnSetValue(std::move(fnSetValue))
				, m_prevColor(initialColor)
			{
			}

			void update(const std::shared_ptr<Node>&) override
			{
				const double r = Clamp(ParseOpt<double>(m_textBoxR->text()).value_or(m_prevColor.r), 0.0, 1.0);
				const double g = Clamp(ParseOpt<double>(m_textBoxG->text()).value_or(m_prevColor.g), 0.0, 1.0);
				const double b = Clamp(ParseOpt<double>(m_textBoxB->text()).value_or(m_prevColor.b), 0.0, 1.0);
				const double a = Clamp(ParseOpt<double>(m_textBoxA->text()).value_or(m_prevColor.a), 0.0, 1.0);

				const ColorF newColor{ r, g, b, a };
				if (newColor != m_prevColor)
				{
					m_prevColor = newColor;
					if (m_fnSetValue)
					{
						m_fnSetValue(newColor);
					}
					m_previewRect->setFillColor(newColor);
				}
			}
		};

		propertyNode->addComponent(std::make_shared<ColorPropertyTextBox>(
			textBoxR,
			textBoxG,
			textBoxB,
			textBoxA,
			previewRectRenderer,
			fnSetValue,
			currentValue));

		return propertyNode;
	}

	[[nodiscard]]
	static std::shared_ptr<Node> CreateEnumPropertyNode(
		StringView name,
		StringView currentValue,
		std::function<void(StringView)> fnSetValue,
		const std::shared_ptr<ContextMenu>& contextMenu,
		const Array<String>& enumCandidates,
		HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No)
	{
		const auto propertyNode = Node::Create(
			name,
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = Vec2{ 0, 32 },
			},
			IsHitTargetYN::Yes,
			InheritChildrenStateFlags::Hovered);
		propertyNode->setBoxChildrenLayout(HorizontalLayout{ .padding = LRTB{ 10, 8, 0, 0 } });
		propertyNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }), Palette::Black, 0.0, 3.0);

		const auto labelNode =
			propertyNode->emplaceChild(
				U"Label",
				BoxConstraint
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 0.85,
				});
		labelNode->emplaceComponent<Label>(
			name,
			U"",
			14,
			Palette::White,
			HorizontalAlign::Left,
			VerticalAlign::Middle,
			LRTB{ 5, 5, 5, 5 },
			HorizontalOverflow::Wrap,
			VerticalOverflow::Clip,
			Vec2::Zero(),
			hasInteractivePropertyValue ? LabelUnderlineStyle::Solid : LabelUnderlineStyle::None,
			ColorF{ Palette::Yellow, 0.5 },
			2.0,
			LabelSizingMode::ShrinkToFit,
			8.0);

		const auto comboBoxNode = propertyNode->emplaceChild(
			U"ComboBox",
			BoxConstraint
			{
				.sizeDelta = Vec2{ 0, 26 },
				.flexibleWeight = 1,
			});
		comboBoxNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(ColorF{ 1.0, 0.6 }).withSmoothTime(0.05), 1.0, 4.0);

		const auto enumLabel = comboBoxNode->emplaceComponent<Label>(
			currentValue,
			U"",
			14,
			Palette::White,
			HorizontalAlign::Left,
			VerticalAlign::Middle,
			LRTB{ 5, 5, 5, 5 });

		class EnumPropertyComboBox : public ComponentBase
		{
		private:
			String m_currentValue;
			std::function<void(StringView)> m_onSetValue;
			std::shared_ptr<Label> m_label;
			std::shared_ptr<ContextMenu> m_contextMenu;
			Array<String> m_enumCandidates;

		public:
			EnumPropertyComboBox(
				StringView initialValue,
				std::function<void(StringView)> onSetValue,
				const std::shared_ptr<Label>& label,
				const std::shared_ptr<ContextMenu>& contextMenu,
				const Array<String>& enumCandidates)
				: ComponentBase{ {} }
				, m_currentValue(initialValue)
				, m_onSetValue(std::move(onSetValue))
				, m_label(label)
				, m_contextMenu(contextMenu)
				, m_enumCandidates(enumCandidates)
			{
			}

			void update(const std::shared_ptr<Node>& node) override
			{
				if (node->isClicked())
				{
					Array<MenuElement> menuElements;
					for (const auto& name : m_enumCandidates)
					{
						menuElements.push_back(
							MenuItem
							{
								name,
								U"",
								none,
								[this, name]
								{
									m_currentValue = name;
									m_label->setText(name);
									m_onSetValue(m_currentValue);
								}
							});
					}
					m_contextMenu->show(node->rect().bl(), menuElements);
				}
			}
		};

		comboBoxNode->addComponent(std::make_shared<EnumPropertyComboBox>(
			currentValue,
			fnSetValue,
			enumLabel,
			contextMenu,
			enumCandidates));

		comboBoxNode->emplaceComponent<Label>(
			U"▼",
			U"",
			10,
			Palette::White,
			HorizontalAlign::Right,
			VerticalAlign::Middle,
			LRTB{ 5, 7, 5, 5 });

		return propertyNode;
	}

	[[nodiscard]]
	static std::shared_ptr<Node> CreateCheckboxNode(
		bool initialValue,
		std::function<void(bool)> fnSetValue,
		bool useParentHoverState = false)
	{
		auto checkboxNode = Node::Create(
			U"Checkbox",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0, 0 },
				.sizeDelta = Vec2{ 18, 18 },
			},
			useParentHoverState ? IsHitTargetYN::No : IsHitTargetYN::Yes);

		checkboxNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);

		const auto checkLabel = checkboxNode->emplaceComponent<Label>(
			initialValue ? U"✓" : U"",
			U"",
			14,
			Palette::White,
			HorizontalAlign::Center,
			VerticalAlign::Middle);

		class CheckboxToggler : public ComponentBase
		{
		private:
			bool m_value;
			std::function<void(bool)> m_fnSetValue;
			std::shared_ptr<Label> m_checkLabel;
			bool m_useParentHoverState;

		public:
			CheckboxToggler(bool initialValue,
				std::function<void(bool)> fnSetValue,
				const std::shared_ptr<Label>& checkLabel,
				bool useParentHoverState)
				: ComponentBase{ {} }
				, m_value(initialValue)
				, m_fnSetValue(std::move(fnSetValue))
				, m_checkLabel(checkLabel)
				, m_useParentHoverState(useParentHoverState)
			{
			}

			void update(const std::shared_ptr<Node>& node) override
			{
				// クリックでON/OFFをトグル
				bool isClicked = false;
				if (m_useParentHoverState)
				{
					if (const auto parent = node->findHoverTargetParent())
					{
						isClicked = parent->isClicked();
					}
				}
				else
				{
					isClicked = node->isClicked();
				}
				if (isClicked)
				{
					m_value = !m_value;
					m_checkLabel->setText(m_value ? U"✓" : U"");
					if (m_fnSetValue)
					{
						m_fnSetValue(m_value);
					}
				}
			}
		};

		checkboxNode->addComponent(std::make_shared<CheckboxToggler>(
			initialValue,
			std::move(fnSetValue),
			checkLabel,
			useParentHoverState));

		return checkboxNode;
	}

	[[nodiscard]]
	static std::shared_ptr<Node> CreateBoolPropertyNode(
		StringView name,
		bool currentValue,
		std::function<void(bool)> fnSetValue,
		HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No)
	{
		auto propertyNode = Node::Create(
			name,
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = Vec2{ 0, 32 },
			});
		propertyNode->setBoxChildrenLayout(HorizontalLayout{ .padding = LRTB{ 10, 8, 0, 0 } });
		propertyNode->emplaceComponent<RectRenderer>(
			PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }),
			Palette::Black,
			0.0,
			3.0);

		const auto labelNode = propertyNode->emplaceChild(
			U"Label",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0, 1 },
				.flexibleWeight = 0.85,
			},
			IsHitTargetYN::No);
		labelNode->emplaceComponent<Label>(
			name,
			U"",
			14,
			Palette::White,
			HorizontalAlign::Left,
			VerticalAlign::Middle,
			LRTB{ 5, 5, 5, 5 },
			HorizontalOverflow::Overflow,
			VerticalOverflow::Clip,
			Vec2::Zero(),
			hasInteractivePropertyValue ? LabelUnderlineStyle::Solid : LabelUnderlineStyle::None,
			ColorF{ Palette::Yellow, 0.5 },
			2.0,
			LabelSizingMode::ShrinkToFit,
			8.0);

		const auto checkboxParentNode = propertyNode->emplaceChild(
			U"CheckboxParent",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0, 1 },
				.flexibleWeight = 1,
			},
			IsHitTargetYN::No);
		const auto checkboxNode = CreateCheckboxNode(currentValue, fnSetValue, true);
		checkboxNode->setConstraint(
			AnchorConstraint
			{
				.anchorMin = Anchor::MiddleRight,
				.anchorMax = Anchor::MiddleRight,
				.posDelta = Vec2{ -6, 0 },
				.sizeDelta = Vec2{ 18, 18 },
				.sizeDeltaPivot = Anchor::MiddleRight,
			});
		checkboxParentNode->addChild(checkboxNode);

		return propertyNode;
	}

	[[nodiscard]]
	std::shared_ptr<Node> createNodeNameNode(const std::shared_ptr<Node>& node)
	{
		const auto nodeNameNode = Node::Create(
			U"NodeName",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = Vec2{ 0, 40 },
				.margin = LRTB{ 0, 0, 0, 8 },
			});
		nodeNameNode->setBoxChildrenLayout(HorizontalLayout{ .padding = 6 });
		nodeNameNode->emplaceComponent<RectRenderer>(ColorF{ 0.3, 0.3 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0);

		// Activeチェックボックスを追加
		const auto activeCheckboxNode = CreateCheckboxNode(node->activeSelf().getBool(), [node](bool value) { node->setActive(value); });
		// Activeチェックボックスにツールチップを追加
		{
			const PropertyKey key{ U"Node", U"activeSelf" };
			if (const auto it = m_propertyMetadata.find(key); it != m_propertyMetadata.end())
			{
				const auto& metadata = it->second;
				if (metadata.tooltip)
				{
					activeCheckboxNode->emplaceComponent<::TooltipOpener>(m_editorOverlayCanvas, *metadata.tooltip, metadata.tooltipDetail.value_or(U""));
				}
			}
		}
		nodeNameNode->addChild(activeCheckboxNode);
		
		// Nameテキストボックスを追加
		const auto nameTextboxNode = CreateNodeNameTextboxNode(U"name", node->name(),
			[this, node](StringView value)
			{
				if (value.empty())
				{
					node->setName(U"Node");
				}
				else
				{
					node->setName(value);
				}
				m_onChangeNodeName();
			});
		// Nameテキストボックスにツールチップを追加
		{
			const PropertyKey key{ U"Node", U"name" };
			if (const auto it = m_propertyMetadata.find(key); it != m_propertyMetadata.end())
			{
				const auto& metadata = it->second;
				if (metadata.tooltip)
				{
					// CreateNodeNameTextboxNodeはLabelを含むNodeを返すので、そのLabelを探す
					if (const auto labelNode = nameTextboxNode->getChildByNameOrNull(U"Label", RecursiveYN::Yes))
					{
						labelNode->emplaceComponent<::TooltipOpener>(m_editorOverlayCanvas, *metadata.tooltip, metadata.tooltipDetail.value_or(U""));
					}
				}
			}
		}
		nodeNameNode->addChild(nameTextboxNode);

		return nodeNameNode;
	}

	[[nodiscard]]
	std::shared_ptr<Node> createNodeSettingNode(const std::shared_ptr<Node>& node)
	{
		auto nodeSettingNode = Node::Create(
			U"NodeSetting",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.margin = LRTB{ 0, 0, 0, 8 },
			});
		nodeSettingNode->setBoxChildrenLayout(VerticalLayout{ .padding = m_isFoldedNodeSetting ? LRTB::Zero() : LRTB{ 0, 0, 0, 8 } });
		nodeSettingNode->emplaceComponent<RectRenderer>(ColorF{ 0.3, 0.3 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0);

		nodeSettingNode->addChild(CreateHeadingNode(U"Node Settings", ColorF{ 0.5, 0.3, 0.3 }, m_isFoldedNodeSetting,
			[this](IsFoldedYN isFolded)
			{
				m_isFoldedNodeSetting = isFolded;
			}));

		nodeSettingNode->addChild(
			Node::Create(
				U"TopPadding",
				BoxConstraint
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 8 },
				}))->setActive(!m_isFoldedNodeSetting.getBool());

		const auto fnAddBoolChild =
			[this, &nodeSettingNode](StringView name, bool currentValue, auto fnSetValue)
			{
				nodeSettingNode->addChild(createBoolPropertyNodeWithTooltip(U"Node", name, currentValue, fnSetValue))->setActive(!m_isFoldedNodeSetting.getBool());
			};
		const auto fnAddLRTBChild =
			[this, &nodeSettingNode](StringView name, const LRTB& currentValue, auto fnSetValue)
			{
				nodeSettingNode->addChild(createLRTBPropertyNodeWithTooltip(U"Node", name, currentValue, fnSetValue))->setActive(!m_isFoldedNodeSetting.getBool());
			};
		fnAddBoolChild(U"isHitTarget", node->isHitTarget().getBool(), [this, node](bool value) { 
			node->setIsHitTarget(value); 
			refreshInspector();
		});
		// isHitTargetがtrueの場合のみhitTestPaddingを表示
		if (node->isHitTarget())
		{
			fnAddLRTBChild(U"hitTestPadding", node->hitTestPadding(), [node](const LRTB& value) { node->setHitTestPadding(value); });
		}
		fnAddBoolChild(U"inheritsChildrenHoveredState", node->inheritsChildrenHoveredState(), [node](bool value) { node->setInheritsChildrenHoveredState(value); });
		fnAddBoolChild(U"inheritsChildrenPressedState", node->inheritsChildrenPressedState(), [node](bool value) { node->setInheritsChildrenPressedState(value); });
		fnAddBoolChild(U"interactable", node->interactable().getBool(), [node](bool value) { node->setInteractable(value); });
		fnAddBoolChild(U"horizontalScrollable", node->horizontalScrollable(), [node](bool value) { node->setHorizontalScrollable(value); });
		fnAddBoolChild(U"verticalScrollable", node->verticalScrollable(), [node](bool value) { node->setVerticalScrollable(value); });
		fnAddBoolChild(U"wheelScrollEnabled", node->wheelScrollEnabled(), [this, node](bool value) { 
			node->setWheelScrollEnabled(value); 
			refreshInspector();
		});
		fnAddBoolChild(U"dragScrollEnabled", node->dragScrollEnabled(), [this, node](bool value) { 
			node->setDragScrollEnabled(value); 
			refreshInspector();
		});
		// dragScrollEnabledが有効な場合のみ表示
		if (node->dragScrollEnabled())
		{
			const auto fnAddDoubleChild =
				[this, &nodeSettingNode](StringView name, double currentValue, auto fnSetValue)
				{
					nodeSettingNode->addChild(createPropertyNodeWithTooltip(U"Node", name, Format(currentValue), [fnSetValue = std::move(fnSetValue)](StringView value) { fnSetValue(ParseOpt<double>(value).value_or(0.0)); }))->setActive(!m_isFoldedNodeSetting.getBool());
				};
			fnAddDoubleChild(U"decelerationRate", node->decelerationRate(), [node](double value) { node->setDecelerationRate(Clamp(value, 0.0, 1.0)); });
		}
		// wheelScrollEnabledまたはdragScrollEnabledが有効な場合のみ表示
		if (node->wheelScrollEnabled() || node->dragScrollEnabled())
		{
			fnAddBoolChild(U"rubberBandScrollEnabled", node->rubberBandScrollEnabled().getBool(), [node](bool value) { node->setRubberBandScrollEnabled(value); });
		}
		fnAddBoolChild(U"clippingEnabled", node->clippingEnabled().getBool(), [node](bool value) { node->setClippingEnabled(value); });

		nodeSettingNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly);

		return nodeSettingNode;
	}

	enum class LayoutType
	{
		FlowLayout,
		HorizontalLayout,
		VerticalLayout,
	};

	[[nodiscard]]
	std::shared_ptr<Node> createBoxChildrenLayoutNode(const std::shared_ptr<Node>& node)
	{
		auto layoutNode = Node::Create(
			U"BoxChildrenLayout",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.margin = LRTB{ 0, 0, 0, 8 },
			});
		layoutNode->setBoxChildrenLayout(VerticalLayout{ .padding = m_isFoldedLayout ? LRTB::Zero() : LRTB{ 0, 0, 0, 8 } });
		layoutNode->emplaceComponent<RectRenderer>(ColorF{ 0.3, 0.3 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0);
		layoutNode->addChild(CreateHeadingNode(U"Box Children Layout", ColorF{ 0.5, 0.3, 0.3 }, m_isFoldedLayout,
			[this](IsFoldedYN isFolded)
			{
				m_isFoldedLayout = isFolded;
			}));
		// 現在のLayoutタイプを取得
		String layoutTypeName;
		if (node->childrenFlowLayout()) layoutTypeName = U"FlowLayout";
		else if (node->childrenHorizontalLayout()) layoutTypeName = U"HorizontalLayout";
		else if (node->childrenVerticalLayout()) layoutTypeName = U"VerticalLayout";
		
		const auto fnAddChild =
			[this, &layoutNode, &layoutTypeName](StringView name, const auto& value, auto fnSetValue)
			{
				layoutNode->addChild(createPropertyNodeWithTooltip(layoutTypeName, name, Format(value), fnSetValue))->setActive(!m_isFoldedLayout.getBool());
			};
		const auto fnAddVec2Child =
			[this, &layoutNode, &layoutTypeName](StringView name, const Vec2& currentValue, auto fnSetValue)
			{
				layoutNode->addChild(createVec2PropertyNodeWithTooltip(layoutTypeName, name, currentValue, fnSetValue))->setActive(!m_isFoldedLayout.getBool());
			};
		const auto fnAddDoubleChild =
			[this, &layoutNode, &layoutTypeName](StringView name, double currentValue, auto fnSetValue)
			{
				layoutNode->addChild(createPropertyNodeWithTooltip(layoutTypeName, name, Format(currentValue), [fnSetValue = std::move(fnSetValue)](StringView value) { fnSetValue(ParseOpt<double>(value).value_or(0.0)); }))->setActive(!m_isFoldedLayout.getBool());
			};
		const auto fnAddLRTBChild =
			[this, &layoutNode, &layoutTypeName](StringView name, const LRTB& currentValue, auto fnSetValue)
			{
				layoutNode->addChild(createLRTBPropertyNodeWithTooltip(layoutTypeName, name, currentValue, fnSetValue))->setActive(!m_isFoldedLayout.getBool());
			};
		const auto fnAddEnumChild =
			[this, &layoutNode, &layoutTypeName]<typename EnumType>(const String & name, EnumType currentValue, auto fnSetValue)
			{
				auto fnSetEnumValue = [fnSetValue = std::move(fnSetValue), currentValue](StringView value) { fnSetValue(StringToEnum<EnumType>(value, currentValue)); };
				layoutNode->addChild(createEnumPropertyNodeWithTooltip(layoutTypeName, name, EnumToString(currentValue), fnSetEnumValue, m_contextMenu, EnumNames<EnumType>()))->setActive(!m_isFoldedLayout.getBool());
			};
		if (const auto pFlowLayout = node->childrenFlowLayout())
		{
			fnAddEnumChild(
				U"type",
				LayoutType::FlowLayout,
				[this, node](LayoutType type)
				{
					switch (type)
					{
					case LayoutType::FlowLayout:
						break;
					case LayoutType::HorizontalLayout:
						node->setBoxChildrenLayout(HorizontalLayout{});
						refreshInspector(); // 項目に変更があるため更新
						break;
					case LayoutType::VerticalLayout:
						node->setBoxChildrenLayout(VerticalLayout{});
						refreshInspector(); // 項目に変更があるため更新
						break;
					}
				});
			fnAddLRTBChild(U"padding", pFlowLayout->padding, [this, node](const LRTB& value) { auto newLayout = *node->childrenFlowLayout(); newLayout.padding = value; node->setBoxChildrenLayout(newLayout); });
			fnAddVec2Child(U"spacing", pFlowLayout->spacing, [this, node](const Vec2& value) { auto newLayout = *node->childrenFlowLayout(); newLayout.spacing = value; node->setBoxChildrenLayout(newLayout); });
			fnAddEnumChild(U"horizontalAlign", pFlowLayout->horizontalAlign, [this, node](HorizontalAlign value) { auto newLayout = *node->childrenFlowLayout(); newLayout.horizontalAlign = value; node->setBoxChildrenLayout(newLayout); });
			fnAddEnumChild(U"verticalAlign", pFlowLayout->verticalAlign, [this, node](VerticalAlign value) { auto newLayout = *node->childrenFlowLayout(); newLayout.verticalAlign = value; node->setBoxChildrenLayout(newLayout); });
		}
		else if (const auto pHorizontalLayout = node->childrenHorizontalLayout())
		{
			fnAddEnumChild(
				U"type",
				LayoutType::HorizontalLayout,
				[this, node](LayoutType type)
				{
					switch (type)
					{
					case LayoutType::FlowLayout:
						node->setBoxChildrenLayout(FlowLayout{});
						refreshInspector(); // 項目に変更があるため更新
						break;
					case LayoutType::HorizontalLayout:
						break;
					case LayoutType::VerticalLayout:
						node->setBoxChildrenLayout(VerticalLayout{});
						refreshInspector(); // 項目に変更があるため更新
						break;
					}
				});
			fnAddLRTBChild(U"padding", pHorizontalLayout->padding, [this, node](const LRTB& value) { auto newLayout = *node->childrenHorizontalLayout(); newLayout.padding = value; node->setBoxChildrenLayout(newLayout); });
			fnAddDoubleChild(U"spacing", pHorizontalLayout->spacing, [this, node](double value) { auto newLayout = *node->childrenHorizontalLayout(); newLayout.spacing = value; node->setBoxChildrenLayout(newLayout); });
			fnAddEnumChild(U"horizontalAlign", pHorizontalLayout->horizontalAlign, [this, node](HorizontalAlign value) { auto newLayout = *node->childrenHorizontalLayout(); newLayout.horizontalAlign = value; node->setBoxChildrenLayout(newLayout); });
			fnAddEnumChild(U"verticalAlign", pHorizontalLayout->verticalAlign, [this, node](VerticalAlign value) { auto newLayout = *node->childrenHorizontalLayout(); newLayout.verticalAlign = value; node->setBoxChildrenLayout(newLayout); });
		}
		else if (const auto pVerticalLayout = node->childrenVerticalLayout())
		{
			fnAddEnumChild(
				U"type",
				LayoutType::VerticalLayout,
				[this, node](LayoutType type)
				{
					switch (type)
					{
					case LayoutType::FlowLayout:
						node->setBoxChildrenLayout(FlowLayout{});
						refreshInspector(); // 項目に変更があるため更新
						break;
					case LayoutType::HorizontalLayout:
						node->setBoxChildrenLayout(HorizontalLayout{});
						refreshInspector(); // 項目に変更があるため更新
						break;
					case LayoutType::VerticalLayout:
						break;
					}
				});
			fnAddLRTBChild(U"padding", pVerticalLayout->padding, [this, node](const LRTB& value) { auto newLayout = *node->childrenVerticalLayout(); newLayout.padding = value; node->setBoxChildrenLayout(newLayout); });
			fnAddDoubleChild(U"spacing", pVerticalLayout->spacing, [this, node](double value) { auto newLayout = *node->childrenVerticalLayout(); newLayout.spacing = value; node->setBoxChildrenLayout(newLayout); });
			fnAddEnumChild(U"horizontalAlign", pVerticalLayout->horizontalAlign, [this, node](HorizontalAlign value) { auto newLayout = *node->childrenVerticalLayout(); newLayout.horizontalAlign = value; node->setBoxChildrenLayout(newLayout); });
			fnAddEnumChild(U"verticalAlign", pVerticalLayout->verticalAlign, [this, node](VerticalAlign value) { auto newLayout = *node->childrenVerticalLayout(); newLayout.verticalAlign = value; node->setBoxChildrenLayout(newLayout); });
		}
		else
		{
			throw Error{ U"Unknown layout type" };
		}

		layoutNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly);

		return layoutNode;
	}

	[[nodiscard]]
	std::shared_ptr<Node> createConstraintNode(const std::shared_ptr<Node>& node)
	{
		auto constraintNode = Node::Create(
			U"Constraint",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.margin = LRTB{ 0, 0, 0, 8 },
			});
		constraintNode->setBoxChildrenLayout(VerticalLayout{ .padding = m_isFoldedConstraint ? LRTB::Zero() : LRTB{ 0, 0, 0, 8 } });
		constraintNode->emplaceComponent<RectRenderer>(ColorF{ 0.3, 0.3 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0);

		constraintNode->addChild(CreateHeadingNode(U"Constraint", ColorF{ 0.5, 0.3, 0.3 }, m_isFoldedConstraint,
			[this](IsFoldedYN isFolded)
			{
				m_isFoldedConstraint = isFolded;
			}));

		// 現在のConstraintタイプを取得
		const String constraintTypeName = node->boxConstraint() ? U"BoxConstraint" : U"AnchorConstraint";
		
		const auto fnAddChild =
			[this, &constraintNode, &constraintTypeName](StringView name, const auto& value, auto fnSetValue)
			{
				constraintNode->addChild(createPropertyNodeWithTooltip(constraintTypeName, name, Format(value), fnSetValue))->setActive(!m_isFoldedConstraint.getBool());
			};
		const auto fnAddDoubleChild =
			[this, &constraintNode, &constraintTypeName](StringView name, double currentValue, auto fnSetValue)
			{
				constraintNode->addChild(createPropertyNodeWithTooltip(constraintTypeName, name, Format(currentValue), [fnSetValue = std::move(fnSetValue)](StringView value) { fnSetValue(ParseOpt<double>(value).value_or(0.0)); }))->setActive(!m_isFoldedConstraint.getBool());
			};
		const auto fnAddEnumChild =
			[this, &constraintNode, &constraintTypeName]<typename EnumType>(const String & name, EnumType currentValue, auto fnSetValue)
			{
				auto fnSetEnumValue = [fnSetValue = std::move(fnSetValue), currentValue](StringView value) { fnSetValue(StringToEnum<EnumType>(value, currentValue)); };
				constraintNode->addChild(createEnumPropertyNodeWithTooltip(constraintTypeName, name, EnumToString(currentValue), fnSetEnumValue, m_contextMenu, EnumNames<EnumType>()))->setActive(!m_isFoldedConstraint.getBool());
			};
		const auto fnAddVec2Child =
			[this, &constraintNode, &constraintTypeName](StringView name, const Vec2& currentValue, auto fnSetValue)
			{
				constraintNode->addChild(createVec2PropertyNodeWithTooltip(constraintTypeName, name, currentValue, fnSetValue))->setActive(!m_isFoldedConstraint.getBool());
			};

		if (const auto pBoxConstraint = node->boxConstraint())
		{
			fnAddEnumChild(
				U"type",
				ConstraintType::BoxConstraint,
				[this, node](ConstraintType type)
				{
					switch (type)
					{
					case ConstraintType::AnchorConstraint:
						node->setConstraint(AnchorConstraint
						{
							.anchorMin = Anchor::MiddleCenter,
							.anchorMax = Anchor::MiddleCenter,
							.posDelta = Vec2::Zero(),
							.sizeDelta = node->layoutAppliedRect().size,
							.sizeDeltaPivot = Vec2{ 0.5, 0.5 },
						});
						m_defaults->constraintType = ConstraintType::AnchorConstraint; // 次回のデフォルト値として記憶
						refreshInspector(); // 項目に変更があるため更新
						break;
					case ConstraintType::BoxConstraint:
						break;
					}
				});
			fnAddVec2Child(U"sizeRatio", pBoxConstraint->sizeRatio, [this, node](const Vec2& value) { auto newConstraint = *node->boxConstraint(); newConstraint.sizeRatio = value; node->setConstraint(newConstraint); });
			fnAddVec2Child(U"sizeDelta", pBoxConstraint->sizeDelta, [this, node](const Vec2& value) { auto newConstraint = *node->boxConstraint(); newConstraint.sizeDelta = value; node->setConstraint(newConstraint); });
			fnAddDoubleChild(U"flexibleWeight", pBoxConstraint->flexibleWeight, [this, node](double value) { auto newConstraint = *node->boxConstraint(); newConstraint.flexibleWeight = value; node->setConstraint(newConstraint); });
			fnAddVec2Child(U"margin (L, R)", Vec2{ pBoxConstraint->margin.left, pBoxConstraint->margin.right }, [this, node](const Vec2& value) { auto newConstraint = *node->boxConstraint(); newConstraint.margin.left = value.x; newConstraint.margin.right = value.y; node->setConstraint(newConstraint); });
			fnAddVec2Child(U"margin (T, B)", Vec2{ pBoxConstraint->margin.top, pBoxConstraint->margin.bottom }, [this, node](const Vec2& value) { auto newConstraint = *node->boxConstraint(); newConstraint.margin.top = value.x; newConstraint.margin.bottom = value.y; node->setConstraint(newConstraint); });
		}
		else if (const auto pAnchorConstraint = node->anchorConstraint())
		{
			auto setDouble =
				[this, node](auto setter)
				{
					return
						[this, node, setter](StringView s)
						{
							if (auto optVal = ParseOpt<double>(s))
							{
								if (auto ac = node->anchorConstraint())
								{
									auto copy = *ac;
									setter(copy, *optVal);
									node->setConstraint(copy);
									m_canvas->refreshLayout();
								}
							}
						};
				};
			auto setVec2 =
				[this, node](auto setter)
				{
					return
						[this, node, setter](const Vec2& val)
						{
							if (auto ac = node->anchorConstraint())
							{
								auto copy = *ac;
								setter(copy, val);
								node->setConstraint(copy);
								m_canvas->refreshLayout();
							}
						};
				};

			fnAddEnumChild(
				U"type",
				ConstraintType::AnchorConstraint,
				[this, node](ConstraintType type)
				{
					switch (type)
					{
					case ConstraintType::AnchorConstraint:
						break;
					case ConstraintType::BoxConstraint:
						node->setConstraint(BoxConstraint
						{
							.sizeRatio = Vec2::Zero(),
							.sizeDelta = node->rect().size,
						});
						m_defaults->constraintType = ConstraintType::BoxConstraint; // 次回のデフォルト値として記憶
						refreshInspector(); // 項目に変更があるため更新
						break;
					}
				}
			);

			const AnchorPreset anchorPreset =
				pAnchorConstraint->isCustomAnchorInEditor
					? AnchorPreset::Custom
					: ToAnchorPreset(pAnchorConstraint->anchorMin, pAnchorConstraint->anchorMax, pAnchorConstraint->sizeDeltaPivot);

			fnAddEnumChild(
				U"anchor",
				anchorPreset,
				[this, node](AnchorPreset preset)
				{
					if (const auto pAnchorConstraint = node->anchorConstraint())
					{
						auto copy = *pAnchorConstraint;
						if (const auto tuple = FromAnchorPreset(preset))
						{
							// プリセットを選んだ場合
							std::tie(copy.anchorMin, copy.anchorMax, copy.sizeDeltaPivot) = *tuple;
							copy.isCustomAnchorInEditor = false;
						}
						else
						{
							// Customを選んだ場合
							copy.isCustomAnchorInEditor = true;
						}

						// 変更がある場合のみ更新
						if (copy != *pAnchorConstraint)
						{
							if (!copy.isCustomAnchorInEditor)
							{
								const auto beforePreset = ToAnchorPreset(pAnchorConstraint->anchorMin, pAnchorConstraint->anchorMax, pAnchorConstraint->sizeDeltaPivot);

								// 横ストレッチに変更した場合はleftとrightを0にする
								const auto fnIsHorizontalStretch = [](AnchorPreset preset)
									{
										return preset == AnchorPreset::StretchTop ||
											preset == AnchorPreset::StretchMiddle ||
											preset == AnchorPreset::StretchBottom ||
											preset == AnchorPreset::StretchFull;
									};
								if (!fnIsHorizontalStretch(beforePreset) && fnIsHorizontalStretch(preset))
								{
									copy.posDelta.x = 0;
									copy.sizeDelta.x = 0;
								}

								// 縦ストレッチに変更した場合はtopとbottomを0にする
								const auto fnIsVerticalStretch = [](AnchorPreset preset)
									{
										return preset == AnchorPreset::StretchLeft ||
											preset == AnchorPreset::StretchCenter ||
											preset == AnchorPreset::StretchRight ||
											preset == AnchorPreset::StretchFull;
									};
								if (!fnIsVerticalStretch(beforePreset) && fnIsVerticalStretch(preset))
								{
									copy.posDelta.y = 0;
									copy.sizeDelta.y = 0;
								}
							}

							node->setConstraint(copy);
							m_canvas->refreshLayout();
							refreshInspector(); // 項目に変更があるため更新
						}
					}
				}
			);
			switch (anchorPreset)
			{
			case AnchorPreset::TopLeft:
				fnAddChild(U"top", pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = v; }));
				fnAddChild(U"left", pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = v; }));
				fnAddVec2Child(U"size", pAnchorConstraint->sizeDelta, setVec2([](AnchorConstraint& c, const Vec2& v) { c.sizeDelta = v; }));
				break;

			case AnchorPreset::TopCenter:
				fnAddChild(U"top", pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = v; }));
				fnAddVec2Child(U"size", pAnchorConstraint->sizeDelta, setVec2([](AnchorConstraint& c, const Vec2& v) { c.sizeDelta = v; }));
				fnAddChild(U"xDelta", pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = v; }));
				break;

			case AnchorPreset::TopRight:
				fnAddChild(U"top", pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = v; }));
				fnAddChild(U"right", -pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = -v; }));
				fnAddVec2Child(U"size", pAnchorConstraint->sizeDelta, setVec2([](AnchorConstraint& c, const Vec2& v) { c.sizeDelta = v; }));
				break;

			case AnchorPreset::MiddleLeft:
				fnAddChild(U"left", pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = v; }));
				fnAddVec2Child(U"size", pAnchorConstraint->sizeDelta, setVec2([](AnchorConstraint& c, const Vec2& v) { c.sizeDelta = v; }));
				fnAddChild(U"yDelta", pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = v; }));
				break;

			case AnchorPreset::MiddleCenter:
				fnAddVec2Child(U"size", pAnchorConstraint->sizeDelta, setVec2([](AnchorConstraint& c, const Vec2& v) { c.sizeDelta = v; }));
				fnAddVec2Child(U"posDelta", pAnchorConstraint->posDelta, setVec2([](AnchorConstraint& c, const Vec2& v) { c.posDelta = v; }));
				break;

			case AnchorPreset::MiddleRight:
				fnAddChild(U"right", -pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = -v; }));
				fnAddVec2Child(U"size", pAnchorConstraint->sizeDelta, setVec2([](AnchorConstraint& c, const Vec2& v) { c.sizeDelta = v; }));
				fnAddChild(U"yDelta", pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = v; }));
				break;

			case AnchorPreset::BottomLeft:
				fnAddChild(U"left", pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = v; }));
				fnAddChild(U"bottom", -pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = -v; }));
				fnAddVec2Child(U"size", pAnchorConstraint->sizeDelta, setVec2([](AnchorConstraint& c, const Vec2& v) { c.sizeDelta = v; }));
				break;

			case AnchorPreset::BottomCenter:
				fnAddChild(U"bottom", -pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = -v; }));
				fnAddVec2Child(U"size", pAnchorConstraint->sizeDelta, setVec2([](AnchorConstraint& c, const Vec2& v) { c.sizeDelta = v; }));
				fnAddChild(U"xDelta", pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = v; }));
				break;

			case AnchorPreset::BottomRight:
				fnAddChild(U"right", -pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = -v; }));
				fnAddChild(U"bottom", -pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = -v; }));
				fnAddVec2Child(U"size", pAnchorConstraint->sizeDelta, setVec2([](AnchorConstraint& c, const Vec2& v) { c.sizeDelta = v; }));
				break;

			case AnchorPreset::StretchTop:
				fnAddChild(U"top", pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = v; }));
				fnAddChild(U"left", pAnchorConstraint->posDelta.x,
					setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
						{
							double oldLeft = pAnchorConstraint->posDelta.x;
							double delta = oldLeft - v;
							c.posDelta.x = v;
							c.sizeDelta.x += delta;
						}));
				fnAddChild(U"right", -(pAnchorConstraint->posDelta.x + pAnchorConstraint->sizeDelta.x),
					setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
						{
							double oldRight = -(pAnchorConstraint->posDelta.x + pAnchorConstraint->sizeDelta.x);
							double delta = v - oldRight;
							c.sizeDelta.x -= delta;
						}));
				fnAddChild(U"height", pAnchorConstraint->sizeDelta.y, setDouble([](AnchorConstraint& c, double v) { c.sizeDelta.y = v; }));
				break;

			case AnchorPreset::StretchMiddle:
				fnAddChild(U"left", pAnchorConstraint->posDelta.x,
					setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
						{
							double oldLeft = pAnchorConstraint->posDelta.x;
							double delta = oldLeft - v;
							c.posDelta.x = v;
							c.sizeDelta.x += delta;
						}));
				fnAddChild(U"right", -(pAnchorConstraint->posDelta.x + pAnchorConstraint->sizeDelta.x),
					setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
						{
							double oldRight = -(pAnchorConstraint->posDelta.x + pAnchorConstraint->sizeDelta.x);
							double delta = v - oldRight;
							c.sizeDelta.x -= delta;
						}));
				fnAddChild(U"height", pAnchorConstraint->sizeDelta.y, setDouble([](AnchorConstraint& c, double v) { c.sizeDelta.y = v; }));
				fnAddChild(U"yDelta", pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = v; }));
				break;

			case AnchorPreset::StretchBottom:
				fnAddChild(U"left", pAnchorConstraint->posDelta.x,
					setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
						{
							double oldLeft = pAnchorConstraint->posDelta.x;
							double delta = oldLeft - v;
							c.posDelta.x = v;
							c.sizeDelta.x += delta;
						}));
				fnAddChild(U"right", -(pAnchorConstraint->posDelta.x + pAnchorConstraint->sizeDelta.x),
					setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
						{
							double oldRight = -(pAnchorConstraint->posDelta.x + pAnchorConstraint->sizeDelta.x);
							double delta = v - oldRight;
							c.sizeDelta.x -= delta;
						}));
				fnAddChild(U"bottom", pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = -v; }));
				fnAddChild(U"height", pAnchorConstraint->sizeDelta.y, setDouble([](AnchorConstraint& c, double v) { c.sizeDelta.y = v; }));
				break;

			case AnchorPreset::StretchLeft:
				fnAddChild(U"top", pAnchorConstraint->posDelta.y,
					setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
						{
							double oldTop = pAnchorConstraint->posDelta.y;
							double delta = oldTop - v;
							c.posDelta.y = v;
							c.sizeDelta.y += delta;
						}));
				fnAddChild(U"bottom", -(pAnchorConstraint->posDelta.y + pAnchorConstraint->sizeDelta.y),
					setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
						{
							double oldBottom = -(pAnchorConstraint->posDelta.y + pAnchorConstraint->sizeDelta.y);
							double delta = v - oldBottom;
							c.sizeDelta.y -= delta;
						}));
				fnAddChild(U"left", pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = v; }));
				fnAddChild(U"width", pAnchorConstraint->sizeDelta.x, setDouble([](AnchorConstraint& c, double v) { c.sizeDelta.x = v; }));
				break;

			case AnchorPreset::StretchCenter:
				fnAddChild(U"top", pAnchorConstraint->posDelta.y,
					setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
						{
							double oldTop = pAnchorConstraint->posDelta.y;
							double delta = oldTop - v;
							c.posDelta.y = v;
							c.sizeDelta.y += delta;
						}));
				fnAddChild(U"bottom", -(pAnchorConstraint->posDelta.y + pAnchorConstraint->sizeDelta.y),
					setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
						{
							double oldBottom = -(pAnchorConstraint->posDelta.y + pAnchorConstraint->sizeDelta.y);
							double delta = v - oldBottom;
							c.sizeDelta.y -= delta;
						}));
				fnAddChild(U"width", pAnchorConstraint->sizeDelta.x, setDouble([](AnchorConstraint& c, double v) { c.sizeDelta.x = v; }));
				fnAddChild(U"xDelta", pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = v; }));
				break;

			case AnchorPreset::StretchRight:
				fnAddChild(U"top", pAnchorConstraint->posDelta.y,
					setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
						{
							double oldTop = pAnchorConstraint->posDelta.y;
							double delta = oldTop - v;
							c.posDelta.y = v;
							c.sizeDelta.y += delta;
						}));
				fnAddChild(U"bottom", -(pAnchorConstraint->posDelta.y + pAnchorConstraint->sizeDelta.y),
					setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
						{
							double oldBottom = -(pAnchorConstraint->posDelta.y + pAnchorConstraint->sizeDelta.y);
							double delta = v - oldBottom;
							c.sizeDelta.y -= delta;
						}));
				fnAddChild(U"right", pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = -v; }));
				fnAddChild(U"width", pAnchorConstraint->sizeDelta.x, setDouble([](AnchorConstraint& c, double v) { c.sizeDelta.x = v; }));
				break;

			case AnchorPreset::StretchFull:
				fnAddChild(U"left", pAnchorConstraint->posDelta.x,
					setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
						{
							double oldLeft = pAnchorConstraint->posDelta.x;
							double delta = oldLeft - v;
							c.posDelta.x = v;
							c.sizeDelta.x += delta;
						}));
				fnAddChild(U"right", -(pAnchorConstraint->posDelta.x + pAnchorConstraint->sizeDelta.x),
					setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
						{
							double oldRight = -(pAnchorConstraint->posDelta.x + pAnchorConstraint->sizeDelta.x);
							double delta = v - oldRight;
							c.sizeDelta.x -= delta;
						}));
				fnAddChild(U"top", pAnchorConstraint->posDelta.y,
					setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
						{
							double oldTop = pAnchorConstraint->posDelta.y;
							double delta = oldTop - v;
							c.posDelta.y = v;
							c.sizeDelta.y += delta;
						}));
				fnAddChild(U"bottom", -(pAnchorConstraint->posDelta.y + pAnchorConstraint->sizeDelta.y),
					setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
						{
							double oldBottom = -(pAnchorConstraint->posDelta.y + pAnchorConstraint->sizeDelta.y);
							double delta = v - oldBottom;
							c.sizeDelta.y -= delta;
						}));
				break;

			default:
				fnAddVec2Child(U"anchorMin", pAnchorConstraint->anchorMin, setVec2([](AnchorConstraint& c, const Vec2& val) { c.anchorMin = val; }));
				fnAddVec2Child(U"anchorMax", pAnchorConstraint->anchorMax, setVec2([](AnchorConstraint& c, const Vec2& val) { c.anchorMax = val; }));
				fnAddVec2Child(U"sizeDeltaPivot", pAnchorConstraint->sizeDeltaPivot, setVec2([](AnchorConstraint& c, const Vec2& val) { c.sizeDeltaPivot = val; }));
				fnAddVec2Child(U"posDelta", pAnchorConstraint->posDelta, setVec2([](AnchorConstraint& c, const Vec2& val) { c.posDelta = val; }));
				fnAddVec2Child(U"sizeDelta", pAnchorConstraint->sizeDelta, setVec2([](AnchorConstraint& c, const Vec2& val) { c.sizeDelta = val; }));
				break;
			}
		}
		else
		{
			throw Error{ U"Unknown constraint type" };
		}

		constraintNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly);

		return constraintNode;
	}

	[[nodiscard]]
	std::shared_ptr<Node> createTransformEffectNode(TransformEffect* const pTransformEffect)
	{
		auto transformEffectNode = Node::Create(
			U"TransformEffect",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.margin = LRTB{ 0, 0, 0, 8 },
			});
		transformEffectNode->setBoxChildrenLayout(VerticalLayout{ .padding = m_isFoldedTransformEffect ? LRTB::Zero() : LRTB{ 0, 0, 0, 8 } });
		transformEffectNode->emplaceComponent<RectRenderer>(ColorF{ 0.3, 0.3 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0);

		transformEffectNode->addChild(CreateHeadingNode(U"TransformEffect", ColorF{ 0.3, 0.5, 0.3 }, m_isFoldedTransformEffect,
			[this](IsFoldedYN isFolded)
			{
				m_isFoldedTransformEffect = isFolded;
			}));

		const auto fnAddVec2Child =
			[this, &transformEffectNode](StringView name, SmoothProperty<Vec2>* pProperty, auto fnSetValue)
			{
				const auto propertyNode = transformEffectNode->addChild(createVec2PropertyNodeWithTooltip(U"TransformEffect", name, pProperty->propertyValue().defaultValue, fnSetValue, HasInteractivePropertyValueYN{ pProperty->hasInteractivePropertyValue() }));
				propertyNode->setActive(!m_isFoldedTransformEffect.getBool());
				propertyNode->template emplaceComponent<ContextMenuOpener>(m_contextMenu, Array<MenuElement>{ MenuItem{ U"ステート毎に値を変更..."_fmt(name), U"", KeyC, [this, pProperty] { m_dialogOpener->openDialog(std::make_shared<InteractivePropertyValueDialog>(pProperty, [this] { refreshInspector(); })); } } }, nullptr, RecursiveYN::Yes);
			};
		// Note: アクセサからポインタを取得しているので注意が必要
		fnAddVec2Child(U"position", &pTransformEffect->position(), [this, pTransformEffect](const Vec2& value) { pTransformEffect->setPosition(value); m_canvas->refreshLayout(); });
		fnAddVec2Child(U"scale", &pTransformEffect->scale(), [this, pTransformEffect](const Vec2& value) { pTransformEffect->setScale(value); m_canvas->refreshLayout(); });
		fnAddVec2Child(U"pivot", &pTransformEffect->pivot(), [this, pTransformEffect](const Vec2& value) { pTransformEffect->setPivot(value); m_canvas->refreshLayout(); });
		
		const auto fnAddBoolChild =
			[this, &transformEffectNode](StringView name, Property<bool>* pProperty, auto fnSetValue)
			{
				const auto propertyNode = transformEffectNode->addChild(createBoolPropertyNodeWithTooltip(U"TransformEffect", name, pProperty->propertyValue().defaultValue, fnSetValue, HasInteractivePropertyValueYN{ pProperty->hasInteractivePropertyValue() }));
				propertyNode->setActive(!m_isFoldedTransformEffect.getBool());
				propertyNode->template emplaceComponent<ContextMenuOpener>(m_contextMenu, Array<MenuElement>{ MenuItem{ U"ステート毎に値を変更..."_fmt(name), U"", KeyC, [this, pProperty] { m_dialogOpener->openDialog(std::make_shared<InteractivePropertyValueDialog>(pProperty, [this] { refreshInspector(); })); } } }, nullptr, RecursiveYN::Yes);
			};
		fnAddBoolChild(U"appliesToHitTest", &pTransformEffect->appliesToHitTest(), [this, pTransformEffect](bool value) { pTransformEffect->setAppliesToHitTest(value); });

		transformEffectNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly);

		return transformEffectNode;
	}

	[[nodiscard]]
	std::shared_ptr<Node> createComponentNode(const std::shared_ptr<Node>& node, const std::shared_ptr<SerializableComponentBase>& component, IsFoldedYN isFolded, std::function<void(IsFoldedYN)> onToggleFold)
	{
		auto componentNode = Node::Create(
			component->type(),
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.margin = LRTB{ 0, 0, 0, 8 },
			});
		componentNode->setBoxChildrenLayout(VerticalLayout{ .padding = isFolded ? LRTB::Zero() : LRTB{ 0, 0, 0, 8 } });
		componentNode->emplaceComponent<RectRenderer>(ColorF{ 0.3, 0.3 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0);

		const auto headingNode = componentNode->addChild(CreateHeadingNode(component->type(), ColorF{ 0.3, 0.3, 0.5 }, isFolded, std::move(onToggleFold)));
		Array<MenuElement> menuElements;
		menuElements.push_back(MenuItem{ U"{} を削除"_fmt(component->type()), U"", KeyR, [this, node, component] { node->removeComponent(component); refreshInspector(); } });
		menuElements.push_back(MenuSeparator{});
		menuElements.push_back(MenuItem{ U"{} を上へ移動"_fmt(component->type()), U"", KeyU, [this, node, component] { node->moveComponentUp(component); refreshInspector(); } });
		menuElements.push_back(MenuItem{ U"{} を下へ移動"_fmt(component->type()), U"", KeyD, [this, node, component] { node->moveComponentDown(component); refreshInspector(); } });
		menuElements.push_back(MenuSeparator{});
		menuElements.push_back(MenuItem{ U"{} の内容をコピー"_fmt(component->type()), U"", KeyC, [this, component] { onClickCopyComponent(component); } });
		
		// 同じタイプのコンポーネントがコピーされている場合のみ貼り付けメニューを表示
		if (m_copiedComponentType.has_value() && *m_copiedComponentType == component->type())
		{
			menuElements.push_back(MenuItem{ U"{} の内容を貼り付け"_fmt(component->type()), U"", KeyV, [this, component] { onClickPasteComponentTo(component); } });
		}
		
		headingNode->emplaceComponent<ContextMenuOpener>(m_contextMenu, menuElements);

		if (component->properties().empty())
		{
			const auto noPropertyLabelNode = componentNode->emplaceChild(
				U"NoProperty",
				BoxConstraint
				{
					.sizeRatio = { 1, 0 },
					.sizeDelta = { 0, 24 },
					.margin = { .top = 4 },
				});
			noPropertyLabelNode->emplaceComponent<Label>(
				U"(プロパティなし)",
				U"",
				14,
				Palette::White,
				HorizontalAlign::Center,
				VerticalAlign::Middle);
			if (isFolded)
			{
				noPropertyLabelNode->setActive(false);
			}
		}
		for (const auto& property : component->properties())
		{
			const PropertyEditType editType = property->editType();
			std::shared_ptr<Node> propertyNode;
			switch (editType)
			{
			case PropertyEditType::Text:
				{
					std::function<void(StringView)> onChange = [property](StringView value) { property->trySetPropertyValueString(value); };
					
					// メタデータに基づいて変更時の処理を設定
					const PropertyKey propertyKey{ String(component->type()), String(property->name()) };
					std::function<String()> fnGetValue = nullptr;
					if (const auto it = m_propertyMetadata.find(propertyKey); it != m_propertyMetadata.end())
					{
						const auto& metadata = it->second;
						if (metadata.refreshInspectorOnChange)
						{
							onChange = [this, property](StringView value)
							{
								property->trySetPropertyValueString(value);
								refreshInspector();
							};
						}
						// refreshesEveryFrameがtrueの場合のみfnGetValueを設定
						if (metadata.refreshesEveryFrame)
						{
							fnGetValue = [property]() -> String { return property->propertyValueStringOfDefault(); };
						}
					}
					
					propertyNode = componentNode->addChild(
						createPropertyNodeWithTooltip(
							component->type(),
							property->name(),
							property->propertyValueStringOfDefault(),
							onChange,
							HasInteractivePropertyValueYN{ property->hasInteractivePropertyValue() },
							fnGetValue));
				}
				break;
			case PropertyEditType::Bool:
				{
					std::function<void(bool)> onChange = [property](bool value) { property->trySetPropertyValueString(Format(value)); };
					
					// メタデータに基づいて変更時の処理を設定
					const PropertyKey propertyKey{ String(component->type()), String(property->name()) };
					if (const auto it = m_propertyMetadata.find(propertyKey); it != m_propertyMetadata.end())
					{
						const auto& metadata = it->second;
						if (metadata.refreshInspectorOnChange)
						{
							onChange = [this, property](bool value)
							{
								property->trySetPropertyValueString(Format(value));
								refreshInspector();
							};
						}
					}
					propertyNode = componentNode->addChild(
						createBoolPropertyNodeWithTooltip(
							component->type(),
							property->name(),
							ParseOr<bool>(property->propertyValueStringOfDefault(), false),
							onChange,
							HasInteractivePropertyValueYN{ property->hasInteractivePropertyValue() }));
				}
				break;
			case PropertyEditType::Vec2:
				{
					std::function<void(const Vec2&)> onChange = [property](const Vec2& value) { property->trySetPropertyValueString(Format(value)); };
					
					// メタデータに基づいて変更時の処理を設定
					const PropertyKey propertyKey{ String(component->type()), String(property->name()) };
					if (const auto it = m_propertyMetadata.find(propertyKey); it != m_propertyMetadata.end())
					{
						const auto& metadata = it->second;
						if (metadata.refreshInspectorOnChange)
						{
							onChange = [this, property](const Vec2& value)
							{
								property->trySetPropertyValueString(Format(value));
								refreshInspector();
							};
						}
					}
					
					propertyNode = componentNode->addChild(
						createVec2PropertyNodeWithTooltip(
							component->type(),
							property->name(),
							ParseOr<Vec2>(property->propertyValueStringOfDefault(), Vec2{ 0, 0 }),
							onChange,
							HasInteractivePropertyValueYN{ property->hasInteractivePropertyValue() }));
				}
				break;
			case PropertyEditType::Color:
				{
					std::function<void(const ColorF&)> onChange = [property](const ColorF& value) { property->trySetPropertyValueString(Format(value)); };
					
					// メタデータに基づいて変更時の処理を設定
					const PropertyKey propertyKey{ String(component->type()), String(property->name()) };
					if (const auto it = m_propertyMetadata.find(propertyKey); it != m_propertyMetadata.end())
					{
						const auto& metadata = it->second;
						if (metadata.refreshInspectorOnChange)
						{
							onChange = [this, property](const ColorF& value)
							{
								property->trySetPropertyValueString(Format(value));
								refreshInspector();
							};
						}
					}
					
					propertyNode = componentNode->addChild(
						createColorPropertyNodeWithTooltip(
							component->type(),
							property->name(),
							ParseOr<ColorF>(property->propertyValueStringOfDefault(), ColorF{ 0, 0, 0, 1 }),
							onChange,
							HasInteractivePropertyValueYN{ property->hasInteractivePropertyValue() }));
				}
				break;
			case PropertyEditType::LRTB:
				{
					std::function<void(const LRTB&)> onChange = [property](const LRTB& value) { property->trySetPropertyValueString(Format(value)); };
					
					// メタデータに基づいて変更時の処理を設定
					const PropertyKey propertyKey{ String(component->type()), String(property->name()) };
					if (const auto it = m_propertyMetadata.find(propertyKey); it != m_propertyMetadata.end())
					{
						const auto& metadata = it->second;
						if (metadata.refreshInspectorOnChange)
						{
							onChange = [this, property](const LRTB& value)
							{
								property->trySetPropertyValueString(Format(value));
								refreshInspector();
							};
						}
					}
					
					propertyNode = componentNode->addChild(
						createLRTBPropertyNodeWithTooltip(
							component->type(),
							property->name(),
							ParseOr<LRTB>(property->propertyValueStringOfDefault(), LRTB{ 0, 0, 0, 0 }),
							onChange,
							HasInteractivePropertyValueYN{ property->hasInteractivePropertyValue() }));
				}
				break;
			case PropertyEditType::Enum:
				{
					std::function<void(StringView)> onChange = [property](StringView value) { property->trySetPropertyValueString(value); };
					
					// メタデータに基づいて変更時の処理を設定
					const PropertyKey propertyKey{ String(component->type()), String(property->name()) };
					if (const auto it = m_propertyMetadata.find(propertyKey); it != m_propertyMetadata.end())
					{
						const auto& metadata = it->second;
						if (metadata.refreshInspectorOnChange)
						{
							onChange = [this, property](StringView value)
							{
								property->trySetPropertyValueString(value);
								refreshInspector();
							};
						}
					}
					
					propertyNode = componentNode->addChild(
						createEnumPropertyNodeWithTooltip(
							component->type(),
							property->name(),
							property->propertyValueStringOfDefault(),
							onChange,
							m_contextMenu,
							property->enumCandidates(),
							HasInteractivePropertyValueYN{ property->hasInteractivePropertyValue() }));
				}
				break;
			}
			if (!propertyNode)
			{
				throw Error{ U"Failed to create property node" };
			}

			// 表示条件のチェック
			const PropertyKey visibilityKey{ String(component->type()), String(property->name()) };
			if (const auto it = m_propertyMetadata.find(visibilityKey); it != m_propertyMetadata.end())
			{
				const auto& metadata = it->second;
				// 可視条件のチェック
				bool isVisible = true;
				if (metadata.visibilityCondition)
				{
					isVisible = metadata.visibilityCondition(*component);
				}
				
				// プロパティの可視情報を保存
				propertyNode->storeData<PropertyVisibilityData>({ .isVisibleByCondition = isVisible });
				
				// 可視条件を満たさない、または折り畳まれている場合は非表示
				if (!isVisible || isFolded)
				{
					propertyNode->setActive(false);
				}
			}
			else
			{
				// メタデータがない場合は、常に表示可能として扱う
				propertyNode->storeData<PropertyVisibilityData>({ .isVisibleByCondition = true });
				
				if (isFolded)
				{
					propertyNode->setActive(false);
				}
			}
			
			if (property->isInteractiveProperty())
			{
				propertyNode->emplaceComponent<ContextMenuOpener>(
					m_contextMenu,
					Array<MenuElement>
					{
						MenuItem{ U"ステート毎に値を変更..."_fmt(property->name()), U"", KeyC, [this, property] { m_dialogOpener->openDialog(std::make_shared<InteractivePropertyValueDialog>(property, [this] { refreshInspector(); })); } },
					},
					nullptr,
					RecursiveYN::Yes);
			}
		}

		componentNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly);

		return componentNode;
	}

	void clearTargetNode()
	{
		setTargetNode(nullptr);
	}

	void update()
	{
	}

	[[nodiscard]]
	const std::shared_ptr<Node>& inspectorFrameNode() const
	{
		return m_inspectorFrameNode;
	}
};

void InteractivePropertyValueDialog::createDialogContent(const std::shared_ptr<Node>& contentRootNode, const std::shared_ptr<ContextMenu>& dialogContextMenu)
{
	if (!m_pProperty)
	{
		throw Error{ U"Property is nullptr" };
	}

	const auto labelNode = contentRootNode->emplaceChild(
		U"Label",
		BoxConstraint
		{
			.sizeRatio = Vec2{ 1, 0 },
			.sizeDelta = SizeF{ 0, 36 },
			.margin = LRTB{ 0, 0, 0, 8 },
		});
	labelNode->emplaceComponent<Label>(
		m_pProperty->name(),
		U"",
		14,
		Palette::White,
		HorizontalAlign::Center,
		VerticalAlign::Middle);

	for (const auto selected : { SelectedYN::No, SelectedYN::Yes })
	{
		for (const auto interactionState : { InteractionState::Default, InteractionState::Hovered, InteractionState::Pressed, InteractionState::Disabled })
		{
			const String headingText = EnumToString(interactionState) + (selected.yesNo ? U" & Selected" : U"");

			const auto propertyNode = contentRootNode->emplaceChild(
				U"Property",
				BoxConstraint
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = SizeF{ -20, 0 },
					.margin = LRTB{ 0, 0, 0, 8 },
				});
			propertyNode->emplaceChild(
				U"Spacing",
				BoxConstraint
				{
					.sizeRatio = Vec2{ 0, 0 },
					.sizeDelta = SizeF{ 8, 0 },
				});
			propertyNode->setBoxChildrenLayout(HorizontalLayout{}, RefreshesLayoutYN::No);
			const auto currentValueString = std::make_shared<String>(m_pProperty->propertyValueStringOfFallback(interactionState, selected));
			std::shared_ptr<Node> propertyValueNode;
			switch (m_pProperty->editType())
			{
			case PropertyEditType::Text:
				propertyValueNode = propertyNode->addChild(
					Inspector::CreatePropertyNode(
						headingText,
						m_pProperty->propertyValueStringOfFallback(interactionState, selected),
						[this, interactionState, selected, currentValueString](StringView value)
						{
							if (m_pProperty->trySetPropertyValueStringOf(value, interactionState, selected))
							{
								*currentValueString = value;
								if (m_onChange)
								{
									m_onChange();
								}
							}
						}),
					RefreshesLayoutYN::No);
				break;
			case PropertyEditType::Bool:
				propertyValueNode = propertyNode->addChild(
					Inspector::CreateBoolPropertyNode(
						headingText,
						ParseOr<bool>(m_pProperty->propertyValueStringOfFallback(interactionState, selected), false),
						[this, interactionState, selected, currentValueString](bool value)
						{
							const String formattedValue = Format(value);
							if (m_pProperty->trySetPropertyValueStringOf(formattedValue, interactionState, selected))
							{
								*currentValueString = formattedValue;
								if (m_onChange)
								{
									m_onChange();
								}
							}
						}),
					RefreshesLayoutYN::No);
				break;
			case PropertyEditType::Vec2:
				propertyValueNode = propertyNode->addChild(
					Inspector::CreateVec2PropertyNode(
						headingText,
						ParseOr<Vec2>(m_pProperty->propertyValueStringOfFallback(interactionState, selected), Vec2{ 0, 0 }),
						[this, interactionState, selected, currentValueString](const Vec2& value)
						{
							const String formattedValue = Format(value);
							if (m_pProperty->trySetPropertyValueStringOf(formattedValue, interactionState, selected))
							{
								*currentValueString = formattedValue;
								if (m_onChange)
								{
									m_onChange();
								}
							}
						}),
					RefreshesLayoutYN::No);
				break;
			case PropertyEditType::Color:
				propertyValueNode = propertyNode->addChild(
					Inspector::CreateColorPropertyNode(
						headingText,
						ParseOr<ColorF>(m_pProperty->propertyValueStringOfFallback(interactionState, selected), ColorF{ 0, 0, 0, 1 }),
						[this, interactionState, selected, currentValueString](const ColorF& value)
						{
							const String formattedValue = Format(value);
							if (m_pProperty->trySetPropertyValueStringOf(formattedValue, interactionState, selected))
							{
								*currentValueString = formattedValue;
								if (m_onChange)
								{
									m_onChange();
								}
							}
						}),
					RefreshesLayoutYN::No);
				break;
			case PropertyEditType::LRTB:
				propertyValueNode = propertyNode->addChild(
					Inspector::CreateLRTBPropertyNode(
						headingText,
						ParseOr<LRTB>(m_pProperty->propertyValueStringOfFallback(interactionState, selected), LRTB{ 0, 0, 0, 0 }),
						[this, interactionState, selected, currentValueString](const LRTB& value)
						{
							const String formattedValue = Format(value);
							if (m_pProperty->trySetPropertyValueStringOf(formattedValue, interactionState, selected))
							{
								*currentValueString = formattedValue;
								if (m_onChange)
								{
									m_onChange();
								}
							}
						}),
					RefreshesLayoutYN::No);
				break;
			case PropertyEditType::Enum:
				propertyValueNode = propertyNode->addChild(
					Inspector::CreateEnumPropertyNode(
						headingText,
						m_pProperty->propertyValueStringOfFallback(interactionState, selected),
						[this, interactionState, selected, currentValueString](StringView value)
						{
							if (m_pProperty->trySetPropertyValueStringOf(value, interactionState, selected))
							{
								*currentValueString = value;
								if (m_onChange)
								{
									m_onChange();
								}
							}
						},
						dialogContextMenu,
						m_pProperty->enumCandidates()),
					RefreshesLayoutYN::No);
				break;
			}
			if (!propertyValueNode)
			{
				throw Error{ U"Property value node is nullptr" };
			}
			const auto checkboxNode = propertyNode->addChildAtIndex(Inspector::CreateCheckboxNode(
				m_pProperty->hasPropertyValueOf(interactionState, selected),
				[this, interactionState, selected, propertyValueNode, currentValueString](bool value)
				{
					if (value)
					{
						if (m_pProperty->trySetPropertyValueStringOf(*currentValueString, interactionState, selected))
						{
							propertyValueNode->setInteractable(true);
							if (m_onChange)
							{
								m_onChange();
							}
						}
					}
					else
					{
						if (m_pProperty->tryUnsetPropertyValueOf(interactionState, selected))
						{
							propertyValueNode->setInteractable(false);
							if (m_onChange)
							{
								m_onChange();
							}
						}
					}
				}),
				0,
				RefreshesLayoutYN::No);
			checkboxNode->setInteractable(interactionState != InteractionState::Default || selected.yesNo); // Defaultは必ず存在するのでチェックボックスは無効
			propertyValueNode->setInteractable(m_pProperty->hasPropertyValueOf(interactionState, selected));
			propertyNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly, RefreshesLayoutYN::No);
		}
	}

	// SmoothPropertyの場合はsmoothTimeの項目を追加
	if (m_pProperty->isSmoothProperty())
	{
		const auto propertyNode = contentRootNode->emplaceChild(
			U"Property",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = SizeF{ 0, 0 },
				.margin = LRTB{ 0, 0, 0, 8 },
			});
		propertyNode->addChild(
			Inspector::CreatePropertyNode(
				U"smoothTime [sec]",
				Format(m_pProperty->smoothTime()),
				[this](StringView value) { m_pProperty->trySetSmoothTime(ParseFloatOpt<double>(value).value_or(m_pProperty->smoothTime())); }),
			RefreshesLayoutYN::No);
		propertyNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly, RefreshesLayoutYN::No);
	}

	contentRootNode->refreshContainedCanvasLayout();
}

constexpr Vec2 InitialCanvasScrollOffset{ 0, -(MenuBarHeight + Toolbar::ToolbarHeight) / 2 };

class Editor
{
private:
	std::shared_ptr<Canvas> m_canvas;
	std::shared_ptr<Canvas> m_editorCanvas;
	std::shared_ptr<Canvas> m_editorOverlayCanvas;
	std::shared_ptr<ContextMenu> m_contextMenu;
	std::shared_ptr<Canvas> m_dialogCanvas;
	std::shared_ptr<Canvas> m_dialogOverlayCanvas;
	std::shared_ptr<ContextMenu> m_dialogContextMenu;
	std::shared_ptr<DialogOpener> m_dialogOpener;
	std::shared_ptr<Defaults> m_defaults = std::make_shared<Defaults>();
	bool m_isConfirmDialogShowing = false;
	Hierarchy m_hierarchy;
	Inspector m_inspector;
	MenuBar m_menuBar;
	Toolbar m_toolbar;
	Size m_prevSceneSize;
	Optional<String> m_filePath = none;
	uint64 m_savedHash = 0;
	Vec2 m_scrollOffset = InitialCanvasScrollOffset;
	double m_scrollScale = 1.0;
	bool m_isAltScrolling = false;
	HistorySystem m_historySystem;

public:
	Editor()
		: m_canvas(Canvas::Create())
		, m_editorCanvas(Canvas::Create())
		, m_editorOverlayCanvas(Canvas::Create())
		, m_contextMenu(std::make_shared<ContextMenu>(m_editorOverlayCanvas, U"EditorContextMenu"))
		, m_dialogCanvas(Canvas::Create())
		, m_dialogOverlayCanvas(Canvas::Create())
		, m_dialogContextMenu(std::make_shared<ContextMenu>(m_dialogOverlayCanvas, U"DialogContextMenu"))
		, m_dialogOpener(std::make_shared<DialogOpener>(m_dialogCanvas, m_dialogContextMenu))
		, m_hierarchy(m_canvas, m_editorCanvas, m_contextMenu, m_defaults)
		, m_inspector(m_canvas, m_editorCanvas, m_editorOverlayCanvas, m_contextMenu, m_defaults, m_dialogOpener, [this] { m_hierarchy.refreshNodeNames(); })
		, m_menuBar(m_editorCanvas, m_contextMenu)
		, m_toolbar(m_editorCanvas, m_editorOverlayCanvas)
		, m_prevSceneSize(Scene::Size())
	{
		m_menuBar.addMenuCategory(
			U"File",
			U"ファイル",
			KeyF,
			Array<MenuElement>
			{
				MenuItem{ U"新規作成", U"Ctrl+N", KeyN, [this] { onClickMenuFileNew(); } },
				MenuItem{ U"開く...", U"Ctrl+O", KeyO, [this] { onClickMenuFileOpen(); } },
				MenuItem{ U"保存", U"Ctrl+S", KeyS, [this] { onClickMenuFileSave(); } },
				MenuItem{ U"名前を付けて保存...", U"Ctrl+Shift+S", KeyA, [this] { onClickMenuFileSaveAs(); } },
				MenuSeparator{},
				MenuItem{ U"終了", U"Alt+F4", KeyQ, [this] { onClickMenuFileExit(); } },
			},
			100);
		m_menuBar.addMenuCategory(
			U"Edit",
			U"編集",
			KeyE,
			{
				MenuItem{ U"元に戻す", U"Ctrl+Z", KeyU, [this] { onClickMenuEditUndo(); }, [this] { return m_historySystem.canUndo(); } },
				MenuItem{ U"やり直し", U"Ctrl+Shift+Z", KeyR, [this] { onClickMenuEditRedo(); }, [this] { return m_historySystem.canRedo(); } },
				MenuSeparator{},
				MenuItem{ U"切り取り", U"Ctrl+X", KeyT, [this] { onClickMenuEditCut(); }, [this] { return m_hierarchy.hasSelection(); } },
				MenuItem{ U"コピー", U"Ctrl+C", KeyC, [this] { onClickMenuEditCopy(); }, [this] { return m_hierarchy.hasSelection(); } },
				MenuItem{ U"貼り付け", U"Ctrl+V", KeyP, [this] { onClickMenuEditPaste(); }, [this] { return m_hierarchy.canPaste(); } },
				MenuItem{ U"複製を作成", U"Ctrl+D", KeyL, [this] { onClickMenuEditDuplicate(); }, [this] { return m_hierarchy.hasSelection(); } },
				MenuItem{ U"削除", U"Delete", KeyD, [this] { onClickMenuEditDelete(); }, [this] { return m_hierarchy.hasSelection(); } },
				MenuSeparator{},
				MenuItem{ U"すべて選択", U"Ctrl+A", KeyA, [this] { m_hierarchy.selectAll(); } },
			});
		m_menuBar.addMenuCategory(
			U"View",
			U"表示",
			KeyV,
			{
				MenuItem{ U"表示位置をリセット", U"Ctrl+0", KeyR, [this] { onClickMenuViewResetPosition(); } },
			});
		m_menuBar.addMenuCategory(
			U"Tool",
			U"ツール",
			KeyT,
			{
				MenuItem{ U"アセットのルートディレクトリ(プレビュー用)を設定...", U"Ctrl+Alt+O", KeyA, [this] { onClickMenuToolChangeAssetDirectory(); } },
			},
			80,
			480);
		
		// ツールバーの初期化
		m_toolbar.addButton(U"New", U"\xF0224", U"新規作成 (Ctrl+N)", [this] { onClickMenuFileNew(); })->addClickHotKey(KeyN, CtrlYN::Yes, AltYN::No, ShiftYN::No, EnabledWhileTextEditingYN::Yes);
		m_toolbar.addButton(U"Open", U"\xF0256", U"開く (Ctrl+O)", [this] { onClickMenuFileOpen(); })->addClickHotKey(KeyO, CtrlYN::Yes, AltYN::No, ShiftYN::No, EnabledWhileTextEditingYN::Yes);
		m_toolbar.addButton(U"Save", U"\xF0818", U"保存 (Ctrl+S)", [this] { onClickMenuFileSave(); })->addClickHotKey(KeyS, CtrlYN::Yes, AltYN::No, ShiftYN::No, EnabledWhileTextEditingYN::Yes);
		m_toolbar.addButton(U"SaveAs", U"\xF0E28", U"名前を付けて保存 (Ctrl+Shift+S)", [this] { onClickMenuFileSaveAs(); })->addClickHotKey(KeyA, CtrlYN::Yes, AltYN::No, ShiftYN::Yes, EnabledWhileTextEditingYN::Yes);
		m_toolbar.addSeparator();
		m_toolbar.addButton(U"Undo", U"\xF054C", U"元に戻す (Ctrl+Z)", [this] { onClickMenuEditUndo(); }, [this] { return m_historySystem.canUndo(); })->addClickHotKey(KeyZ, CtrlYN::Yes, AltYN::No, ShiftYN::No, EnabledWhileTextEditingYN::Yes);
		m_toolbar.addButton(U"Redo", U"\xF054D", U"やり直し (Ctrl+Shift+Z)", [this] { onClickMenuEditRedo(); }, [this] { return m_historySystem.canRedo(); })
			->addClickHotKey(KeyY, CtrlYN::Yes, AltYN::No, ShiftYN::No, EnabledWhileTextEditingYN::Yes)
			->addClickHotKey(KeyZ, CtrlYN::Yes, AltYN::No, ShiftYN::Yes, EnabledWhileTextEditingYN::Yes);
		m_toolbar.addSeparator();
		m_toolbar.addButton(U"NewNode", U"\xF1200", U"新規ノード (Ctrl+Shift+N)", [this] { m_hierarchy.onClickNewNode(); })->addClickHotKey(KeyN, CtrlYN::Yes, AltYN::No, ShiftYN::Yes, EnabledWhileTextEditingYN::Yes);
		m_toolbar.addButton(U"NewNodeAsChild", U"\xF0F97", U"選択ノードの子として新規ノード (Ctrl+Alt+N)",
			[this]
			{
				if (const auto parent = m_hierarchy.selectedNode().lock())
				{
					m_hierarchy.onClickNewNode(parent);
				}
			},
			[this] { return m_hierarchy.hasSelection(); })
			->addClickHotKey(KeyN, CtrlYN::Yes, AltYN::Yes, ShiftYN::No, EnabledWhileTextEditingYN::Yes);
		m_toolbar.addSeparator();
		m_toolbar.addButton(U"CopyNode", U"\xF018F", U"選択ノードをコピー (Ctrl+C)", [this] { m_hierarchy.onClickCopy(); }, [this] { return m_hierarchy.hasSelection(); })->addClickHotKey(KeyC, CtrlYN::Yes, AltYN::No, ShiftYN::No);
		m_toolbar.addButton(U"PasteNode", U"\xF0192", U"ノードを貼り付け (Ctrl+V)", [this] { m_hierarchy.onClickPaste(); }, [this] { return m_hierarchy.canPaste(); })->addClickHotKey(KeyV, CtrlYN::Yes, AltYN::No, ShiftYN::No);
		m_toolbar.addButton(U"CutNode", U"\xF0190", U"選択ノードを切り取り (Ctrl+X)", [this] { m_hierarchy.onClickCut(); }, [this] { return m_hierarchy.hasSelection(); })->addClickHotKey(KeyX, CtrlYN::Yes, AltYN::No, ShiftYN::No);
		m_toolbar.addButton(U"DeleteNode", U"\xF0A7A", U"選択ノードを削除 (Delete)", [this] { m_hierarchy.onClickDelete(); }, [this] { return m_hierarchy.hasSelection(); });
		m_toolbar.addSeparator();

		// 初期位置を反映
		m_canvas->setOffsetScale(-m_scrollOffset, Vec2::All(m_scrollScale));
		
		// ツールバーの初期状態を更新
		m_toolbar.updateButtonStates();
	}

	void update()
	{
		m_dialogOverlayCanvas->update();
		m_dialogCanvas->update();
		m_editorOverlayCanvas->update();
		m_editorCanvas->update();
		const bool editorCanvasHovered = CurrentFrame::AnyNodeHovered();
		m_canvas->update();

		if (Cursor::OnClientRect() && !editorCanvasHovered && !CurrentFrame::AnyScrollableNodeHovered())
		{
			// マウス座標を中心に拡大縮小
			const Vec2 beforeOffset = m_scrollOffset;
			const double beforeScale = m_scrollScale;
			const double scaleFactor = std::exp(-0.2 * Mouse::Wheel());
			m_scrollScale = Clamp(beforeScale * scaleFactor, 0.1, 10.0);
			const Vec2 cursorPos = Cursor::PosF();
			const Vec2 cursorPosInWorldBefore = (cursorPos + m_scrollOffset) / beforeScale;
			const Vec2 cursorPosInWorldAfter = (cursorPos + m_scrollOffset) / m_scrollScale;
			m_scrollOffset += (cursorPosInWorldBefore - cursorPosInWorldAfter) * m_scrollScale;
			if (beforeOffset != m_scrollOffset || beforeScale != m_scrollScale)
			{
				m_canvas->setOffsetScale(-m_scrollOffset, Vec2::All(m_scrollScale));
			}
		}

		m_dialogContextMenu->update();
		m_contextMenu->update();
		m_menuBar.update();
		m_hierarchy.update();
		m_inspector.update();

		if (m_hierarchy.hasSelectionChanged())
		{
			m_inspector.setTargetNode(m_hierarchy.selectedNode().lock());
			m_toolbar.updateButtonStates();
		}
		
		// ツールバーの更新が必要かチェック
		if (m_hierarchy.toolbarRefreshRequested())
		{
			m_toolbar.updateButtonStates();
		}

		const auto sceneSize = Scene::Size();
		if (m_prevSceneSize != sceneSize)
		{
			refreshLayout();
			m_prevSceneSize = sceneSize;
		}

		// ショートカットキー
		const bool isWindowActive = Window::GetState().focused;
		if (isWindowActive && !CurrentFrame::HasInputBlocked() && !IsDraggingNode() && !m_dialogOpener->anyDialogOpened()) // ドラッグ中・ダイアログ表示中は無視
		{
			const bool ctrl = KeyControl.pressed();
			const bool alt = KeyAlt.pressed();
			const bool shift = KeyShift.pressed();

			// Ctrl + ○○
			if (ctrl && !alt && !shift)
			{
				if (KeyN.down())
				{
					onClickMenuFileNew();
				}
				else if (KeyO.down())
				{
					onClickMenuFileOpen();
				}
				else if (KeyS.down())
				{
					onClickMenuFileSave();
				}
			}

			// Ctrl + Shift + ○○
			if (ctrl && !alt && shift)
			{
				if (KeyS.down())
				{
					onClickMenuFileSaveAs();
				}
			}

			// テキストボックス編集中は実行しない操作
			if (!IsEditingTextBox())
			{
				// Ctrl + ○○
				if (ctrl && !alt && !shift)
				{
					// Ctrl+C, Ctrl+X, Ctrl+VはツールバーのHotKeyで別途処理している
					if (KeyA.down())
					{
						m_hierarchy.selectAll();
					}
					else if (KeyD.down())
					{
						m_hierarchy.onClickDuplicate();
					}
					else if (Key0.down())
					{
						onClickMenuViewResetPosition();
					}
				}

				// Alt + ○○
				if (!ctrl && alt && !shift)
				{
					if (KeyUp.down())
					{
						m_hierarchy.onClickMoveUp();
					}
					else if (KeyDown.down())
					{
						m_hierarchy.onClickMoveDown();
					}
					else
					{
						// Alt単体は手のひらツール
						if (!editorCanvasHovered && MouseL.down())
						{
							// ドラッグ開始
							m_isAltScrolling = true;
						}
						if (!MouseL.pressed())
						{
							// ドラッグ終了
							m_isAltScrolling = false;
						}
						if (m_isAltScrolling)
						{
							// 前回との差分を取るため、最初のフレーム(downがtrue)は無視
							if (!MouseL.down())
							{
								m_canvas->setOffset(m_canvas->offset() + Cursor::DeltaF());
								m_scrollOffset = -m_canvas->offset();
							}

							Cursor::RequestStyle(U"HandSmall");
						}
						else if (!editorCanvasHovered && Cursor::OnClientRect())
						{
							Cursor::RequestStyle(U"Hand");
						}
					}
				}
				else
				{
					m_isAltScrolling = false;
				}

				// Ctrl + Shift + ○○
				if (ctrl && !alt && shift)
				{
					// Ctrl+Shift+NはツールバーのHotKeyで別途処理している
				}

				// Ctrl + Alt + ○○
				if (ctrl && alt && !shift)
				{
					// Ctrl+Alt+NはツールバーのHotKeyで別途処理している

					if (KeyO.down())
					{
						onClickMenuToolChangeAssetDirectory();
					}
				}

				// 単体キー
				if (!ctrl && !alt && !shift)
				{
					if (KeyDelete.down())
					{
						m_hierarchy.onClickDelete();
					}
				}
			}
			else
			{
				m_isAltScrolling = false;
			}
		}
		else
		{
			m_isAltScrolling = false;
		}

		// ユーザー操作があった場合、状態を記録
		const bool hasUserInput = MouseL.down() || MouseR.down() || Keyboard::GetAllInputs().any([](const Input& input) { return input.down(); });
		if (hasUserInput)
		{
			m_historySystem.recordStateIfNeeded(m_canvas->toJSONImpl(IncludesInternalIdYN::Yes));
			m_toolbar.updateButtonStates();
		}
		
		// ウィンドウを閉じようとした場合
		if (!m_isConfirmDialogShowing && (System::GetUserActions() & UserAction::CloseButtonClicked))
		{
			showConfirmSaveIfDirty([] { System::Exit(); });
		}
	}

	void draw() const
	{
		m_canvas->draw();
		constexpr double Thickness = 2.0;
		m_canvas->rootNode()->rect().stretched(Thickness / 2).drawFrame(Thickness, ColorF{ 1.0 });
		m_hierarchy.drawSelectedNodesGizmo();
		m_editorCanvas->draw();
		m_editorOverlayCanvas->draw();
		m_dialogCanvas->draw();
		m_dialogOverlayCanvas->draw();
	}

	const std::shared_ptr<Canvas>& canvas() const
	{
		return m_canvas;
	}

	const Hierarchy& hierarchy() const
	{
		return m_hierarchy;
	}

	const std::shared_ptr<Node>& rootNode() const
	{
		return m_canvas->rootNode();
	}

	void refreshLayout()
	{
		m_editorCanvas->refreshLayout();
		m_editorOverlayCanvas->refreshLayout();
		m_canvas->refreshLayout();
		m_dialogCanvas->refreshLayout();
		m_dialogOverlayCanvas->refreshLayout();
	}

	void refresh()
	{
		m_hierarchy.refreshNodeList();
		refreshLayout();
	}

	// 選択中のノードのinternalIdを保存
	Array<uint64> saveSelectedNodeIds() const
	{
		Array<uint64> selectedIds;
		const auto selectedNodes = m_hierarchy.getSelectedNodesExcludingChildren();
		for (const auto& node : selectedNodes)
		{
			selectedIds.push_back(node->internalId());
		}
		return selectedIds;
	}

	// internalIdを使用してノードを検索（再帰的）
	std::shared_ptr<Node> findNodeByInternalId(const std::shared_ptr<Node>& node, uint64 targetId) const
	{
		if (!node)
		{
			return nullptr;
		}
		
		if (node->internalId() == targetId)
		{
			return node;
		}
		
		for (const auto& child : node->children())
		{
			if (auto found = findNodeByInternalId(child, targetId))
			{
				return found;
			}
		}
		
		return nullptr;
	}

	// internalIdのリストから選択を復元
	void restoreSelectedNodeIds(const Array<uint64>& selectedIds)
	{
		if (selectedIds.empty())
		{
			return;
		}
		
		Array<std::shared_ptr<Node>> nodesToSelect;
		for (const auto& id : selectedIds)
		{
			if (auto node = findNodeByInternalId(m_canvas->rootNode(), id))
			{
				nodesToSelect.push_back(node);
			}
		}
		
		if (!nodesToSelect.empty())
		{
			m_hierarchy.selectNodes(nodesToSelect);
		}
	}

	bool isDirty() const
	{
		return m_savedHash != m_canvas->toJSON().formatMinimum().hash();
	}

	void resetDirty()
	{
		m_savedHash = m_canvas->toJSON().formatMinimum().hash();
	}

	void showConfirmSaveIfDirty(std::function<void()> callback)
	{
		if (!isDirty())
		{
			if (callback)
			{
				callback();
			}
			return;
		}

		m_isConfirmDialogShowing = true;

		const String text = m_filePath.has_value()
			? U"'{}'には、保存されていない変更があります。\n上書き保存しますか？"_fmt(FileSystem::FileName(*m_filePath))
			: U"保存されていない変更があります。\n名前を付けて保存しますか？"_s;

		m_dialogOpener->openDialog(
			std::make_shared<SimpleDialog>(
				text,
				[this, callback = std::move(callback)](StringView buttonText)
				{
					m_isConfirmDialogShowing = false;

					if (buttonText == U"キャンセル")
					{
						return;
					}

					if (buttonText == U"はい")
					{
						const bool saved = onClickMenuFileSave();
						if (!saved)
						{
							return;
						}
					}

					if (callback)
					{
						callback();
					}
				},
				Array<DialogButtonDesc>
				{
					DialogButtonDesc
					{
						.text = U"はい",
						.mnemonicInput = KeyY,
						.isDefaultButton = IsDefaultButtonYN::Yes,
					},
					DialogButtonDesc
					{
						.text = U"いいえ",
						.mnemonicInput = KeyN,
					},
					DialogButtonDesc
					{
						.text = U"キャンセル",
						.mnemonicInput = KeyC,
						.isCancelButton = IsCancelButtonYN::Yes,
					}}));
	}

	void onClickMenuFileNew()
	{
		showConfirmSaveIfDirty(
			[this]
			{
				m_filePath = none;
				m_canvas->removeChildrenAll();
				refresh();
				createInitialNode();
				m_historySystem.clear();
				m_toolbar.updateButtonStates();
			});
	}

	void onClickMenuFileOpen()
	{
		showConfirmSaveIfDirty(
			[this]
			{
				if (const auto filePath = Dialog::OpenFile({ FileFilter{ U"NocoUI Canvas", { U"noco" } }, FileFilter::AllFiles() }))
				{
					JSON json;
					try
					{
						json = JSON::Load(*filePath, AllowExceptions::Yes);
					}
					catch (...)
					{
						System::MessageBoxOK(U"エラー", U"ファイルの読み込みに失敗しました", MessageBoxStyle::Error);
						return;
					}
					m_filePath = filePath;
					if (!m_canvas->tryReadFromJSON(json))
					{
						System::MessageBoxOK(U"エラー", U"データの読み取りに失敗しました", MessageBoxStyle::Error);
						return;
					}
					refresh();
					m_historySystem.clear();
					m_toolbar.updateButtonStates();
				}
			});
	}

	bool onClickMenuFileSave()
	{
		Optional<String> filePath = m_filePath;
		if (filePath == none)
		{
			filePath = Dialog::SaveFile({ FileFilter{ U"NocoUI Canvas", { U"noco" } }, FileFilter::AllFiles() });
			if (filePath == none)
			{
				return false;
			}
		}
		const auto json = m_canvas->toJSON();
		if (json.save(*filePath))
		{
			m_filePath = filePath;
			m_savedHash = json.formatMinimum().hash();
			return true;
		}
		else
		{
			System::MessageBoxOK(U"エラー", U"保存に失敗しました", MessageBoxStyle::Error);
			return false;
		}
	}

	void onClickMenuFileSaveAs()
	{
		if (const auto filePath = Dialog::SaveFile({ FileFilter{ U"NocoUI Canvas", { U"noco" } }, FileFilter::AllFiles() }))
		{
			const auto json = m_canvas->toJSON();
			if (json.save(*filePath))
			{
				m_filePath = filePath;
				m_savedHash = json.formatMinimum().hash();
			}
			else
			{
				System::MessageBoxOK(U"エラー", U"保存に失敗しました", MessageBoxStyle::Error);
			}
		}
	}

	void onClickMenuFileExit()
	{
		showConfirmSaveIfDirty(
			[]
			{
				System::Exit();
			});
	}

	void onClickMenuEditCut()
	{
		m_hierarchy.onClickCut();
	}

	void onClickMenuEditCopy()
	{
		m_hierarchy.onClickCopy();
	}

	void onClickMenuEditPaste()
	{
		m_hierarchy.onClickPaste();
	}

	void onClickMenuEditDuplicate()
	{
		m_hierarchy.onClickDuplicate();
	}

	void onClickMenuEditDelete()
	{
		m_hierarchy.onClickDelete();
	}

	void onClickMenuEditSelectAll()
	{
		m_hierarchy.selectAll();
	}
	
	void onClickMenuEditUndo()
	{
		if (const auto undoState = m_historySystem.undo(m_canvas->toJSONImpl(IncludesInternalIdYN::Yes)))
		{
			// 現在選択中のノードのinternalIdを保存
			const auto selectedNodeIds = saveSelectedNodeIds();
			
			m_canvas->tryReadFromJSONImpl(*undoState, IncludesInternalIdYN::Yes);
			refresh();
			
			// 選択を復元
			restoreSelectedNodeIds(selectedNodeIds);
			
			m_historySystem.endRestore();
			m_toolbar.updateButtonStates();
		}
	}
	
	void onClickMenuEditRedo()
	{
		if (const auto redoState = m_historySystem.redo(m_canvas->toJSONImpl(IncludesInternalIdYN::Yes)))
		{
			// 現在選択中のノードのinternalIdを保存
			const auto selectedNodeIds = saveSelectedNodeIds();
			
			m_canvas->tryReadFromJSONImpl(*redoState, IncludesInternalIdYN::Yes);
			refresh();
			
			// 選択を復元
			restoreSelectedNodeIds(selectedNodeIds);
			
			m_historySystem.endRestore();
			m_toolbar.updateButtonStates();
		}
	}

	void onClickMenuViewResetPosition()
	{
		m_scrollOffset = InitialCanvasScrollOffset;
		m_scrollScale = 1.0;
		m_canvas->setOffsetScale(-m_scrollOffset, Vec2::All(m_scrollScale));
	}

	void onClickMenuToolChangeAssetDirectory()
	{
		if (Optional<String> pathOpt = Dialog::SelectFolder(noco::Asset::GetBaseDirectoryPath(), U"アセットのルートディレクトリを選択"))
		{
			const String path = *pathOpt;
			noco::Asset::SetBaseDirectoryPath(path);
		}
	}

	void createInitialNode()
	{
		m_hierarchy.onClickNewNode();
	}
	
	void recordInitialState()
	{
		m_historySystem.recordStateIfNeeded(m_canvas->toJSONImpl(IncludesInternalIdYN::Yes));
	}
};

void Main()
{
	Window::SetStyle(WindowStyle::Sizable);
	Window::Resize(1280, 720);

	Cursor::RegisterCustomCursorStyle(U"Hand", Icon::CreateImage(0xF182DU, 40), Point{ 20, 20 });
	Cursor::RegisterCustomCursorStyle(U"HandSmall", Icon::CreateImage(0xF182DU, 32), Point{ 16, 16 });

	System::SetTerminationTriggers(UserAction::NoAction);

	Editor editor;
	editor.rootNode()->setConstraint(AnchorConstraint
	{
		.anchorMin = Anchor::MiddleCenter,
		.anchorMax = Anchor::MiddleCenter,
		.posDelta = Vec2{ 0, 0 },
		.sizeDelta = Vec2{ 800, 600 },
	});
	editor.refresh();
	editor.createInitialNode();
	editor.resetDirty();
	
	// 初期状態を記録
	editor.recordInitialState();

	Scene::SetBackground(ColorF{ 0.2, 0.2, 0.3 });

	while (System::Update())
	{
		editor.update();
		editor.draw();
	}
}
