#include <Siv3D.hpp>
#include <NocoUI.hpp>
#include "HistorySystem.hpp"
#include "ResizableHandle.hpp"
#include "Tooltip.hpp"
#include "TabStop.hpp"
#include "ContextMenu.hpp"
#include "EnumPropertyComboBox.hpp"
#include "CheckboxToggler.hpp"
#include "Vec2PropertyTextBox.hpp"
#include "Vec4PropertyTextBox.hpp"
#include "ColorPropertyTextBox.hpp"
#include "LRTBPropertyTextBox.hpp"
#include "Defaults.hpp"
#include "EditorYN.hpp"
#include "EditorEnums.hpp"
#include "EditorButton.hpp"
#include "EditorDialog.hpp"
#include "MenuBar.hpp"
#include "Toolbar.hpp"
#include "Hierarchy.hpp"
#include "PropertyMetaData.hpp"
#include "Inspector.hpp"
#include "ComponentSchemaLoader.hpp"
#include "PlaceholderComponent.hpp"
#include <NocoUI/ComponentFactory.hpp>

using namespace noco;
using namespace noco::editor;
using noco::detail::WithInstanceIdYN;

Vec2 calculateCanvasCenterOffset(const Size& sceneSize, const std::shared_ptr<Canvas>& canvas)
{
	// Canvas表示エリア（メニューバーとツールバーを除く）の中央を計算
	const double contentAreaTop = MenuBarHeight + Toolbar::ToolbarHeight;
	const double contentAreaHeight = sceneSize.y - contentAreaTop;
	const Vec2 contentAreaCenter{ 
		sceneSize.x / 2.0, 
		contentAreaTop + contentAreaHeight / 2.0 
	};
	
	const Vec2 canvasSize{ canvas->width(), canvas->height() };
	const Vec2 targetPosition = contentAreaCenter - canvasSize / 2.0;
	return -targetPosition;
}

class Editor
{
private:
	bool m_exitRequested = false;
	
	std::shared_ptr<Canvas> m_canvas;
	std::shared_ptr<Canvas> m_editorCanvas;
	std::shared_ptr<Canvas> m_editorOverlayCanvas;
	std::shared_ptr<ContextMenu> m_contextMenu;
	std::shared_ptr<Canvas> m_dialogCanvas;
	std::shared_ptr<Canvas> m_dialogOverlayCanvas;
	std::shared_ptr<ContextMenu> m_dialogContextMenu;
	std::shared_ptr<DialogOpener> m_dialogOpener;
	std::shared_ptr<Defaults> m_defaults = std::make_shared<Defaults>();
	std::shared_ptr<ComponentFactory> m_componentFactory;
	bool m_isConfirmDialogShowing = false;
	Hierarchy m_hierarchy;
	Inspector m_inspector;
	MenuBar m_menuBar;
	Toolbar m_toolbar;
	Size m_prevSceneSize;
	Optional<String> m_filePath = none;
	uint64 m_savedHash = 0;
	Vec2 m_scrollOffset{ 0, 0 };
	double m_scrollScale = 1.0;
	bool m_isAltScrolling = false;
	HistorySystem m_historySystem;
	
	// リサイズ機能
	double m_hierarchyWidth = 300.0;
	double m_inspectorWidth = 400.0;
	std::unique_ptr<ResizableHandle> m_hierarchyResizeHandle;
	std::unique_ptr<ResizableHandle> m_inspectorResizeHandle;

	void updateZoom()
	{
		if (!Cursor::OnClientRect())
		{
			// カーソルがウィンドウ外にある
			return;
		}

		if (!Window::GetState().focused)
		{
			// ウィンドウが非アクティブ
			// TODO: ウィンドウが非アクティブの場合はエディタ系Canvasがホバーしないため。もし今後Canvas::updateでTODOに書いた非アクティブ時もホバー可能にする対応が入った場合、この条件は不要になる
			return;
		}

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
			m_canvas->setPositionScale(-m_scrollOffset, Vec2::All(m_scrollScale));
		}
	}

public:
	Editor()
		: m_canvas(Canvas::Create()->setEditorPreviewInternal(true))
		, m_editorCanvas(Canvas::Create(Scene::Size())->setAutoResizeMode(AutoResizeMode::MatchSceneSize))
		, m_editorOverlayCanvas(Canvas::Create(Scene::Size())->setAutoResizeMode(AutoResizeMode::MatchSceneSize))
		, m_contextMenu(std::make_shared<ContextMenu>(m_editorOverlayCanvas, U"EditorContextMenu"))
		, m_dialogCanvas(Canvas::Create(Scene::Size())->setAutoResizeMode(AutoResizeMode::MatchSceneSize))
		, m_dialogOverlayCanvas(Canvas::Create(Scene::Size())->setAutoResizeMode(AutoResizeMode::MatchSceneSize))
		, m_dialogContextMenu(std::make_shared<ContextMenu>(m_dialogOverlayCanvas, U"DialogContextMenu"))
		, m_dialogOpener(std::make_shared<DialogOpener>(m_dialogCanvas, m_dialogContextMenu))
		, m_componentFactory(std::make_shared<ComponentFactory>(ComponentFactory::GetBuiltinFactory()))
		, m_hierarchy(m_canvas, m_editorCanvas, m_contextMenu, m_defaults, m_dialogOpener)
		, m_inspector(m_canvas, m_editorCanvas, m_editorOverlayCanvas, m_contextMenu, m_defaults, m_dialogOpener, m_componentFactory, [this] { m_hierarchy.refreshNodeNames(); })
		, m_menuBar(m_editorCanvas, m_contextMenu)
		, m_toolbar(m_editorCanvas, m_editorOverlayCanvas)
		, m_prevSceneSize(Scene::Size())
	{
		m_componentFactory->setUnknownComponentHandler(
			[](const String& type, const JSON& json, noco::detail::WithInstanceIdYN withInstanceId) -> std::shared_ptr<ComponentBase>
			{
				return PlaceholderComponent::Create(type, json, withInstanceId);
			});
		
		const FilePath customComponentsDir = FileSystem::PathAppend(FileSystem::PathAppend(FileSystem::ParentPath(FileSystem::ModulePath()), U"Custom"), U"Components");
		if (FileSystem::Exists(customComponentsDir))
		{
			ComponentSchemaLoader::LoadFromDirectory(customComponentsDir);
		}
		
		const FilePath customFontAssetsDir = FileSystem::PathAppend(FileSystem::PathAppend(FileSystem::ParentPath(FileSystem::ModulePath()), U"Custom"), U"FontAssets");
		if (FileSystem::Exists(customFontAssetsDir))
		{
			loadFontAssetsFromDirectory(customFontAssetsDir);
		}
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

		// Canvas初期位置をウィンドウ中央に設定
		m_scrollOffset = calculateCanvasCenterOffset(Scene::Size(), m_canvas);
		m_canvas->setPositionScale(-m_scrollOffset, Vec2::All(m_scrollScale));
		
		// ツールバーの初期状態を更新
		m_toolbar.updateButtonStates();
		
		initializeResizeHandles();
	}

	void update()
	{
		m_dialogOverlayCanvas->update();
		m_dialogCanvas->update();
		m_editorOverlayCanvas->update();
		m_editorCanvas->update();

		// エディタ系Canvasの更新終了時点でノードがホバーされているかどうか
		// (m_canvas->update()より手前で取得する必要がある点に注意)
		const bool editorCanvasHovered = CurrentFrame::AnyNodeHovered();

		m_canvas->update();

		// エディタ系Canvasやスクロール可能ノードにカーソルがなければズーム操作を更新
		if (!editorCanvasHovered && !CurrentFrame::AnyScrollableNodeHovered())
		{
			updateZoom();
		}

		m_dialogContextMenu->update();
		m_contextMenu->update();
		m_menuBar.update();
		m_hierarchy.update();
		m_inspector.update();
		
		if (m_hierarchyResizeHandle)
		{
			m_hierarchyResizeHandle->update();
		}
		if (m_inspectorResizeHandle)
		{
			m_inspectorResizeHandle->update();
		}

		if (m_hierarchy.checkSelectionChanged())
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
		if (isWindowActive && !CurrentFrame::HasKeyInputBlocked() && !IsDraggingNode() && !m_dialogOpener->anyDialogOpened()) // ドラッグ中・ダイアログ表示中は無視
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
								m_canvas->setPosition(m_canvas->position() + Cursor::DeltaF());
								m_scrollOffset = -m_canvas->position();
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
		const auto userActionFlags = System::GetUserActions();
		const bool hasUserInput = userActionFlags & UserAction::AnyKeyOrMouseDown;
		if (hasUserInput)
		{
			m_historySystem.recordStateIfNeeded(m_canvas->toJSON(WithInstanceIdYN::Yes));
			m_toolbar.updateButtonStates();
		}
		
		// ウィンドウを閉じようとした場合
		if (!m_isConfirmDialogShowing && (userActionFlags & UserAction::CloseButtonClicked))
		{
			showConfirmSaveIfDirty([this] { requestExit(); });
		}
	}

	void draw() const
	{
		m_canvas->draw();
		constexpr double Thickness = 2.0;
		m_canvas->quad().drawFrame(0.0, Thickness, ColorF{ 1.0 });
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

	void initializeResizeHandles()
	{
		// Hierarchyのリサイズハンドル
		m_hierarchyResizeHandle = std::make_unique<ResizableHandle>(
			m_editorCanvas, ResizeDirection::Horizontal, 8.0);
		m_hierarchyResizeHandle->setOnResize(
			[this](double newWidth)
			{
				onHierarchyResize(newWidth);
			});
		
		// Inspectorのリサイズハンドル  
		m_inspectorResizeHandle = std::make_unique<ResizableHandle>(
			m_editorCanvas, ResizeDirection::Horizontal, 8.0);
		m_inspectorResizeHandle->setOnResize(
			[this](double newXPosition)
			{
				onInspectorResize(newXPosition);
			});
		
		updateResizeHandlePositions();
	}
	
	void updateResizeHandlePositions()
	{
		const auto sceneSize = Scene::Size();
		const double topOffset = MenuBarHeight + Toolbar::ToolbarHeight;
		
		// Hierarchyリサイズハンドル(右端)
		if (m_hierarchyResizeHandle)
		{
			m_hierarchyResizeHandle->setPosition(Vec2{ m_hierarchyWidth - 4, topOffset });
			m_hierarchyResizeHandle->setSize(Vec2{ 8, sceneSize.y - topOffset });
		}
		
		// Inspectorリサイズハンドル(左端)
		if (m_inspectorResizeHandle)
		{
			m_inspectorResizeHandle->setPosition(Vec2{ sceneSize.x - m_inspectorWidth - 4, topOffset });
			m_inspectorResizeHandle->setSize(Vec2{ 8, sceneSize.y - topOffset });
		}
	}
	
	void onHierarchyResize(double newWidth)
	{
		m_hierarchyWidth = Math::Clamp(newWidth, 150.0, Scene::Width() * 0.4);
		updatePanelLayout();
		updateResizeHandlePositions();
	}
	
	void onInspectorResize(double newXPosition)
	{
		// Inspectorの左端の位置から幅を計算
		const double newWidth = Scene::Width() - newXPosition;
		m_inspectorWidth = Math::Clamp(newWidth, 150.0, Scene::Width() * 0.4);
		updatePanelLayout();
		updateResizeHandlePositions();
	}
	
	void updatePanelLayout()
	{
		m_hierarchy.setWidth(m_hierarchyWidth);
		m_inspector.setWidth(m_inspectorWidth);
		
		refreshLayout();
	}

	void refreshLayout()
	{
		updateResizeHandlePositions();
		const auto sceneSize = Scene::Size();
		
		// ウィンドウリサイズ時にCanvas中央位置を保持
		m_scrollOffset = calculateCanvasCenterOffset(sceneSize, m_canvas);
		m_canvas->setPositionScale(-m_scrollOffset, Vec2::All(m_scrollScale));
	}

	void refresh()
	{
		m_hierarchy.refreshNodeList();
		refreshLayout();
	}

	bool isExitRequested() const
	{
		return m_exitRequested;
	}

	void requestExit()
	{
		m_exitRequested = true;
	}

	// 選択中のノードのinstanceIdを保存
	Array<uint64> saveSelectedNodeIds() const
	{
		const auto selectedNodes = m_hierarchy.getSelectedNodesExcludingChildren();
		return selectedNodes.map([](const auto& node) { return node->instanceId(); });
	}

	// instanceIdのリストから選択を復元
	void restoreSelectedNodeIds(const Array<uint64>& selectedIds)
	{
		if (selectedIds.empty())
		{
			return;
		}
		
		Array<std::shared_ptr<Node>> nodesToSelect;
		nodesToSelect.reserve(selectedIds.size());
		for (const auto& id : selectedIds)
		{
			if (auto node = m_canvas->findNodeByInstanceId(id))
			{
				nodesToSelect.push_back(std::move(node));
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

	void resetDirtyState()
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
	
	void loadFontAssetsFromDirectory(const FilePath& directory)
	{
		for (const FilePath& path : FileSystem::DirectoryContents(directory, Recursive::Yes))
		{
			if (FileSystem::Extension(path) == U"json")
			{
				loadFontAssetFromFile(path);
			}
		}
	}
	
	enum class FontSourceType : uint8
	{
		File,
		Typeface,
	};
	
	// Siv3DのTypefaceは列挙子にエイリアスを含みmagic_enumで使用できないため別途定義
	enum class TypefaceType : uint8
	{
		CJK_Regular_JP,
		CJK_Regular_KR,
		CJK_Regular_SC,
		CJK_Regular_TC,
		CJK_Regular_HK,
		MonochromeEmoji,
		ColorEmoji,
		Mplus_Thin,
		Mplus_Light,
		Mplus_Regular,
		Mplus_Medium,
		Mplus_Bold,
		Mplus_Heavy,
		Mplus_Black,
		Icon_Awesome_Solid,
		Icon_Awesome_Brand,
		Icon_MaterialDesign,
		Thin,
		Light,
		Regular,
		Medium,
		Bold,
		Heavy,
		Black,
	};
	
	[[nodiscard]]
	Typeface ConvertToS3dTypeface(TypefaceType type)
	{
		switch (type)
		{
		case TypefaceType::CJK_Regular_JP:
			return Typeface::CJK_Regular_JP;
		case TypefaceType::CJK_Regular_KR:
			return Typeface::CJK_Regular_KR;
		case TypefaceType::CJK_Regular_SC:
			return Typeface::CJK_Regular_SC;
		case TypefaceType::CJK_Regular_TC:
			return Typeface::CJK_Regular_TC;
		case TypefaceType::CJK_Regular_HK:
			return Typeface::CJK_Regular_HK;
		case TypefaceType::MonochromeEmoji:
			return Typeface::MonochromeEmoji;
		case TypefaceType::ColorEmoji:
			return Typeface::ColorEmoji;
		case TypefaceType::Mplus_Thin:
			return Typeface::Mplus_Thin;
		case TypefaceType::Mplus_Light:
			return Typeface::Mplus_Light;
		case TypefaceType::Mplus_Regular:
			return Typeface::Mplus_Regular;
		case TypefaceType::Mplus_Medium:
			return Typeface::Mplus_Medium;
		case TypefaceType::Mplus_Bold:
			return Typeface::Mplus_Bold;
		case TypefaceType::Mplus_Heavy:
			return Typeface::Mplus_Heavy;
		case TypefaceType::Mplus_Black:
			return Typeface::Mplus_Black;
		case TypefaceType::Icon_Awesome_Solid:
			return Typeface::Icon_Awesome_Solid;
		case TypefaceType::Icon_Awesome_Brand:
			return Typeface::Icon_Awesome_Brand;
		case TypefaceType::Icon_MaterialDesign:
			return Typeface::Icon_MaterialDesign;
		case TypefaceType::Thin:
			return Typeface::Thin;
		case TypefaceType::Light:
			return Typeface::Light;
		case TypefaceType::Regular:
			return Typeface::Regular;
		case TypefaceType::Medium:
			return Typeface::Medium;
		case TypefaceType::Bold:
			return Typeface::Bold;
		case TypefaceType::Heavy:
			return Typeface::Heavy;
		case TypefaceType::Black:
			return Typeface::Black;
		default:
			return Typeface::Mplus_Regular;
		}
	}
	
	void loadFontAssetFromFile(const FilePath& path)
	{
		const JSON json = JSON::Load(path);
		if (not json)
		{
			Logger << U"[NocoEditor warning] Failed to load font asset JSON: " << path;
			return;
		}
		
		const String fontAssetName = json[U"fontAssetName"].getString();
		if (fontAssetName.isEmpty())
		{
			Logger << U"[NocoEditor warning] fontAssetName is empty in: " << path;
			return;
		}
		
		const int32 fontSize = json[U"fontSize"].get<int32>();
		
		const FontMethod method = json.contains(U"method")
			? StringToEnumOpt<FontMethod>(json[U"method"].getString()).value_or(FontMethod::Bitmap)
			: FontMethod::Bitmap;
		
		const FontStyle style = json.contains(U"style")
			? StringToEnumOpt<FontStyle>(json[U"style"].getString()).value_or(FontStyle::Default)
			: FontStyle::Default;
		
		const JSON sourceJson = json[U"source"];
		const auto sourceType = StringToEnumOpt<FontSourceType>(sourceJson[U"type"].getString());
		if (not sourceType)
		{
			Logger << U"[NocoEditor warning] Invalid source type in: " << path;
			return;
		}
		
		if (*sourceType == FontSourceType::File)
		{
			// JSONファイルと同じディレクトリからの相対パスとして扱う
			const String fontPath = FileSystem::PathAppend(FileSystem::ParentPath(path), sourceJson[U"path"].getString());
			
			if (sourceJson.contains(U"faceIndex"))
			{
				const size_t faceIndex = sourceJson[U"faceIndex"].get<size_t>();
				FontAsset::Register(fontAssetName, method, fontSize, fontPath, faceIndex, style);
			}
			else
			{
				FontAsset::Register(fontAssetName, method, fontSize, fontPath, style);
			}
		}
		else if (*sourceType == FontSourceType::Typeface)
		{
			const String typefaceStr = sourceJson[U"typeface"].getString();
			const TypefaceType typefaceType = StringToEnumOpt<TypefaceType>(typefaceStr).value_or(TypefaceType::Regular);
			const Typeface typeface = ConvertToS3dTypeface(typefaceType);
			FontAsset::Register(fontAssetName, method, fontSize, typeface, style);
		}
	}

	void onClickMenuFileNew()
	{
		showConfirmSaveIfDirty(
			[this]
			{
				m_filePath = none;
				m_canvas->clearParams();
				m_canvas->clearAll();
				refresh();
				m_inspector.refreshInspector(PreserveScrollYN::No);
				createInitialNode();
				recordInitialHistoryState();
				m_toolbar.updateButtonStates();

				// アセットのルートディレクトリを初期化
				noco::Asset::SetBaseDirectoryPath(U"");

				resetDirtyState();
			});
	}

	void appendClearedParamRefsWarning(Array<String>& warningMessages, const Array<String>& clearedParams)
	{
		if (clearedParams.empty())
		{
			return;
		}
		
		// アルファベット順でソート
		Array<String> sortedParams = clearedParams;
		sortedParams.sort();
		
		// 最大10件まで表示
		String paramList;
		const size_t displayCount = Min(sortedParams.size(), size_t(10));
		for (size_t i = 0; i < displayCount; ++i)
		{
			if (i > 0)
			{
				paramList += U"\n";
			}
			paramList += U"・" + sortedParams[i];
		}
		
		// 10件を超える場合は合計数を表示
		if (sortedParams.size() > 10)
		{
			paramList += U"\n... (全{}件)"_fmt(sortedParams.size());
		}
		
		warningMessages.push_back(U"以下のパラメータ参照は利用できないため解除されました。\n\n" + paramList);
	}

	bool loadFromFile(const FilePath& filePath, bool showMessageBoxOnError = true)
	{
		JSON json;
		try
		{
			json = JSON::Load(filePath, AllowExceptions::Yes);
		}
		catch (...)
		{
			if (showMessageBoxOnError)
			{
				System::MessageBoxOK(U"エラー", U"ファイルの読み込みに失敗しました", MessageBoxStyle::Error);
			}
			return false;
		}
		
		Array<String> warningMessages;
		
		// serializedVersionのチェック
		const int32 fileSerializedVersion = json.contains(U"serializedVersion") ? json[U"serializedVersion"].get<int32>() : 0;
		if (fileSerializedVersion > noco::CurrentSerializedVersion)
		{
			const String fileVersion = json.contains(U"version") ? json[U"version"].get<String>() : U"unknown";
			warningMessages.push_back(U"新しいバージョンのNocoEditor({})で作成されたファイルを開きました。\n上書き保存すると、ファイル内容が一部欠損する可能性があります。"_fmt(fileVersion));
		}
		
		m_filePath = filePath;
		if (!m_canvas->tryReadFromJSON(json, *m_componentFactory, RefreshesLayoutYN::Yes, RefreshesLayoutYN::Yes, WithInstanceIdYN::No))
		{
			if (showMessageBoxOnError)
			{
				System::MessageBoxOK(U"エラー", U"データの読み取りに失敗しました", MessageBoxStyle::Error);
			}
			return false;
		}
		const auto clearedParams = m_canvas->removeInvalidParamRefs();
		refresh();
		m_inspector.refreshInspector(PreserveScrollYN::No);
		m_historySystem.clear();
		m_toolbar.updateButtonStates();

		// ファイルと同じディレクトリをアセットのルートディレクトリに設定
		const String folderPath = FileSystem::ParentPath(filePath);
		noco::Asset::SetBaseDirectoryPath(folderPath);

		resetDirtyState();
		
		// パラメータ参照解除の警告を追加
		appendClearedParamRefsWarning(warningMessages, clearedParams);
		
		// 警告ダイアログを順番に表示
		if (showMessageBoxOnError && !warningMessages.empty())
		{
			m_dialogOpener->openDialogOKMultiple(warningMessages);
		}
		
		return true;
	}

	void onClickMenuFileOpen()
	{
		showConfirmSaveIfDirty(
			[this]
			{
				if (const auto filePath = Dialog::OpenFile({ FileFilter{ U"NocoUI Canvas", { U"noco" } }, FileFilter::AllFiles() }))
				{
					loadFromFile(*filePath);
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

				if (noco::Asset::GetBaseDirectoryPath().empty())
				{
					// 明示的にアセットパスが指定されていない場合、ファイルと同じディレクトリをアセットのルートディレクトリに設定
					const String folderPath = FileSystem::ParentPath(*filePath);
					noco::Asset::SetBaseDirectoryPath(folderPath);
				}
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
			[this]
			{
				requestExit();
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
		if (const auto undoState = m_historySystem.undo(m_canvas->toJSON(WithInstanceIdYN::Yes)))
		{
			// 現在選択中のノードのinstanceIdを保存
			const auto selectedNodeIds = saveSelectedNodeIds();
			
			m_canvas->tryReadFromJSON(*undoState, *m_componentFactory, RefreshesLayoutYN::Yes, RefreshesLayoutYN::Yes, WithInstanceIdYN::Yes);
			refresh();
			
			// 選択を復元
			restoreSelectedNodeIds(selectedNodeIds);
			
			// ノード未選択時は選択復元によるInspector再構築が走らないので再構築を呼ぶ
			if (selectedNodeIds.empty())
			{
				m_inspector.refreshInspector();
			}
			
			m_historySystem.endRestore();
			m_toolbar.updateButtonStates();
		}
	}
	
	void onClickMenuEditRedo()
	{
		if (const auto redoState = m_historySystem.redo(m_canvas->toJSON(WithInstanceIdYN::Yes)))
		{
			// 現在選択中のノードのinstanceIdを保存
			const auto selectedNodeIds = saveSelectedNodeIds();
			
			m_canvas->tryReadFromJSON(*redoState, *m_componentFactory, RefreshesLayoutYN::Yes, RefreshesLayoutYN::Yes, WithInstanceIdYN::Yes);
			refresh();
			
			// 選択を復元
			restoreSelectedNodeIds(selectedNodeIds);
			
			// ノード未選択時は選択復元によるInspector再構築が走らないためここで再構築を呼ぶ
			if (selectedNodeIds.empty())
			{
				m_inspector.refreshInspector();
			}
			
			m_historySystem.endRestore();
			m_toolbar.updateButtonStates();
		}
	}

	void onClickMenuViewResetPosition()
	{
		m_scrollOffset = calculateCanvasCenterOffset(Scene::Size(), m_canvas);
		m_scrollScale = 1.0;
		m_canvas->setPositionScale(-m_scrollOffset, Vec2::All(m_scrollScale));
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
	
	void recordInitialHistoryState()
	{
		m_historySystem.recordInitialState(m_canvas->toJSON(WithInstanceIdYN::Yes));
	}
};

void Main()
{
	noco::detail::SetEditorMode(true);

	Window::SetStyle(WindowStyle::Sizable);
	Window::Resize(1280, 720);

	Cursor::RegisterCustomCursorStyle(U"Hand", Icon::CreateImage(0xF182DU, 40), Point{ 20, 20 });
	Cursor::RegisterCustomCursorStyle(U"HandSmall", Icon::CreateImage(0xF182DU, 32), Point{ 16, 16 });

	System::SetTerminationTriggers(UserAction::NoAction);

	Editor editor;
	
	const Array<String> commandLineArgs = System::GetCommandLineArgs();
	bool fileLoaded = false;
	
	for (size_t i = 1; i < commandLineArgs.size(); ++i)
	{
		const String& arg = commandLineArgs[i];
		// .nocoファイルの場合は開く
		if (FileSystem::Extension(arg) == U"noco" && FileSystem::Exists(arg))
		{
			if (editor.loadFromFile(arg, false))
			{
				fileLoaded = true;
				break;
			}
		}
	}
	
	// ファイルが読み込まれなかった場合は新規作成
	if (!fileLoaded)
	{
		editor.createInitialNode();
		editor.resetDirtyState();
	}
	
	editor.recordInitialHistoryState();

	Scene::SetBackground(ColorF{ 0.2, 0.2, 0.3 });

	while (System::Update())
	{
		editor.update();
		if (editor.isExitRequested())
		{
			break;
		}
		editor.draw();
	}
}
