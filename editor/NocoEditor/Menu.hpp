#pragma once
#include <Siv3D.hpp>
#include "NocoUI.hpp"
#include "EditorTypes.hpp"

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
noco::PropertyValue<ColorF> MenuItemRectFillColor();

class ContextMenu
{
public:
	static constexpr int32 DefaultMenuItemWidth = 300;
	static constexpr int32 MenuItemHeight = 30;

private:
	std::shared_ptr<noco::Canvas> m_editorOverlayCanvas;
	std::shared_ptr<noco::Node> m_screenMaskNode;
	std::shared_ptr<noco::Node> m_rootNode;

	Array<MenuElement> m_elements;
	Array<std::shared_ptr<noco::Node>> m_elementNodes;

	std::function<void()> m_fnOnHide = nullptr;

	bool m_isFirstUpdateSinceShown = false;

public:
	explicit ContextMenu(const std::shared_ptr<noco::Canvas>& editorOverlayCanvas, StringView name = U"ContextMenu");

	void show(const Vec2& pos, const Array<MenuElement>& elements, int32 menuItemWidth = DefaultMenuItemWidth, ScreenMaskEnabledYN screenMaskEnabled = ScreenMaskEnabledYN::Yes, std::function<void()> fnOnHide = nullptr);

	void hide(noco::RefreshesLayoutYN refreshesLayout = noco::RefreshesLayoutYN::Yes);

	void update();

	[[nodiscard]]
	bool isHoveredRecursive() const;
};

class ContextMenuOpener : public noco::ComponentBase
{
private:
	std::shared_ptr<ContextMenu> m_contextMenu;
	Array<MenuElement> m_menuElements;
	std::function<void()> m_fnBeforeOpen;
	noco::RecursiveYN m_recursive;

public:
	explicit ContextMenuOpener(const std::shared_ptr<ContextMenu>& contextMenu, Array<MenuElement> menuElements, std::function<void()> fnBeforeOpen = nullptr, noco::RecursiveYN recursive = noco::RecursiveYN::No);

	void update(const std::shared_ptr<noco::Node>& node) override;

	void draw(const noco::Node&) const override;

	void openManually(const Vec2& pos = Cursor::PosF()) const;
};

struct MenuCategory
{
	Array<MenuElement> elements;
	std::shared_ptr<noco::Node> node;
	int32 subMenuWidth;
};

class MenuBar
{
private:
	static constexpr int32 DefaultSubMenuWidth = 300;

	std::shared_ptr<noco::Canvas> m_editorCanvas;
	std::shared_ptr<noco::Node> m_menuBarRootNode;
	Array<MenuCategory> m_menuCategories;
	std::shared_ptr<ContextMenu> m_contextMenu;
	std::shared_ptr<noco::Node> m_activeMenuCategoryNode;
	bool m_hasMenuClosed = false;

public:
	explicit MenuBar(const std::shared_ptr<noco::Canvas>& editorCanvas, const std::shared_ptr<ContextMenu>& contextMenu);

	void addMenuCategory(StringView name, StringView text, const Input& mnemonicInput, const Array<MenuElement>& elements, int32 width = 80, int32 subMenuWidth = DefaultSubMenuWidth);

	void update();
};