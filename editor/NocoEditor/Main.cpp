#include <Siv3D.hpp>
#include "NocoUI.hpp"
#include "EditorTypes.hpp"
#include "Menu.hpp"
#include "Dialog.hpp"
#include "Hierarchy.hpp"
#include "Inspector.hpp"
#include "EditorUtility.hpp"
#include "EditorDefaults.hpp"

using namespace noco;

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
	Size m_prevSceneSize;
	std::weak_ptr<Node> m_prevSelectedNode;
	bool m_prevSelectedNodeExists = false;
	Optional<String> m_filePath = none;
	uint64 m_savedHash = 0;
	Vec2 m_scrollOffset = Vec2::Zero();
	double m_scrollScale = 1.0;
	bool m_isAltScrolling = false;

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
		, m_inspector(m_canvas, m_editorCanvas, m_contextMenu, m_dialogOpener, m_defaults, [this] { m_hierarchy.refreshNodeNames(); })
		, m_menuBar(m_editorCanvas, m_contextMenu)
		, m_prevSceneSize(Scene::Size())
	{
		m_menuBar.addMenuCategory(
			U"File",
			U"ファイル",
			KeyF,
			Array<MenuElement>
			{
				MenuItem{ U"新規作成", U"Ctrl+N", KeyN, [this] { onClickMenuFileNew(); } },
				MenuSeparator{},
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
				MenuItem{ U"切り取り", U"Ctrl+X", KeyT, [this] { onClickMenuEditCut(); }, [this] { return m_hierarchy.hasSelection(); } },
				MenuItem{ U"コピー", U"Ctrl+C", KeyC, [this] { onClickMenuEditCopy(); }, [this] { return m_hierarchy.hasSelection(); } },
				MenuItem{ U"貼り付け", U"Ctrl+V", KeyP, [this] { onClickMenuEditPaste(); }, [this] { return m_hierarchy.canPaste(); } },
				MenuItem{ U"複製を作成", U"Ctrl+D", KeyL, [this] { onClickMenuEditDuplicate(); }, [this] { return m_hierarchy.hasSelection(); } },
				MenuItem{ U"削除", U"Delete", KeyR, [this] { onClickMenuEditDelete(); }, [this] { return m_hierarchy.hasSelection(); } },
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

		const auto selectedNode = m_hierarchy.selectedNode().lock();
		if (selectedNode != m_prevSelectedNode.lock() ||
			(!selectedNode && m_prevSelectedNodeExists))
		{
			m_inspector.setTargetNode(selectedNode);
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
					if (KeyA.down())
					{
						m_hierarchy.selectAll();
					}
					else if (KeyC.down())
					{
						m_hierarchy.onClickCopy();
					}
					else if (KeyV.down())
					{
						m_hierarchy.onClickPaste();
					}
					else if (KeyX.down())
					{
						m_hierarchy.onClickCut();
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

				// Ctrl + Alt + ○○
				if (ctrl && alt && !shift)
				{
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

		// ウィンドウを閉じようとした場合
		if (!m_isConfirmDialogShowing && (System::GetUserActions() & UserAction::CloseButtonClicked))
		{
			showConfirmSaveIfDirty([] { System::Exit(); });
		}

		m_prevSelectedNode = selectedNode;
		m_prevSelectedNodeExists = selectedNode != nullptr;
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
		m_filePath = none;
		m_canvas->removeChildrenAll();
		refresh();
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

	void onClickMenuViewResetPosition()
	{
		m_scrollOffset = Vec2::Zero();
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
	editor.resetDirty();

	Scene::SetBackground(ColorF{ 0.2, 0.2, 0.3 });

	while (System::Update())
	{
		editor.update();
		editor.draw();
	}
}
