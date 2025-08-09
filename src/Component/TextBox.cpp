#include "NocoUI/Component/TextBox.hpp"
#include "NocoUI/Canvas.hpp"
#include "NocoUI/detail/ScopedScissorRect.hpp"

namespace noco
{
	void TextBox::Cache::refreshIfDirty(StringView text, StringView fontAssetName, double fontSize, const SizeF& rectSize)
	{
		if (prevParams.has_value() && !prevParams->isDirty(text, fontAssetName, fontSize, rectSize))
		{
			return;
		}
		prevParams = CacheParams
		{
			.text = String{ text },
			.fontAssetName = String{ fontAssetName },
			.fontSize = fontSize,
			.rectSize = rectSize,
		};

		const Font font = (!fontAssetName.empty() && FontAsset::IsRegistered(fontAssetName)) ? FontAsset(fontAssetName) : SimpleGUI::GetFont();
		fontMethod = font.method();
		glyphs = font.getGlyphs(text);
		const int32 baseFontSize = font.fontSize();
		scale = (baseFontSize == 0) ? 1.0 : (fontSize / baseFontSize);
		lineHeight = font.height(fontSize);

		double posX = 0.0;
		for (auto& glyph : glyphs)
		{
			posX += glyph.xAdvance * scale;
		}
		regionSize = { posX, lineHeight };
	}

	double TextBox::Cache::getCursorPosX(double drawOffsetX, size_t scrollOffset, size_t cursorIndex) const
	{
		double posX = 0.0;
		Optional<double> offsetPosX;
		Optional<double> cursorPosX;
		for (size_t i = 0; i < glyphs.size(); ++i)
		{
			if (i == scrollOffset)
			{
				offsetPosX = posX;
			}
			if (i == cursorIndex)
			{
				cursorPosX = posX;
			}
			if (offsetPosX.has_value() && cursorPosX.has_value())
			{
				break;
			}
			posX += glyphs[i].xAdvance * scale;
		}
		if (!cursorPosX.has_value())
		{
			cursorPosX = posX;
		}
		if (!offsetPosX.has_value())
		{
			offsetPosX = posX;
		}
		return drawOffsetX + cursorPosX.value() - offsetPosX.value();
	}

	size_t TextBox::Cache::getCursorIndex(double drawOffsetX, size_t scrollOffset, double cursorPosX) const
	{
		cursorPosX = Max(cursorPosX, 0.0);

		// scrollOffsetの範囲チェック
		if (scrollOffset > glyphs.size())
		{
			return glyphs.size();
		}

		double posX = drawOffsetX;
		for (size_t i = scrollOffset; i < glyphs.size(); ++i)
		{
			const double nextPosX = posX + glyphs[i].xAdvance * scale;
			const double halfPosX = (posX + nextPosX) / 2;
			if (posX <= cursorPosX && cursorPosX < halfPosX)
			{
				return i;
			}
			else if (halfPosX <= cursorPosX && cursorPosX < nextPosX)
			{
				return i + 1;
			}
			posX = nextPosX;
		}
		return glyphs.size();
	}

	double TextBox::getDrawOffsetX() const
	{
		if (m_fitDirection == FitDirection::Right)
		{
			// カーソルがはみ出さないようずらす
			return -4.0;
		}
		return 0.0;
	}

	Vec2 TextBox::getAlignOffset(const RectF& rect) const
	{
		double offsetX = 0.0;
		double offsetY = 0.0;
		
		// 水平揃え
		const double contentWidth = m_cache.regionSize.x;
		const double availableWidth = rect.w;
		
		// テキストが収まる場合のみ水平揃えを適用
		if (contentWidth <= availableWidth)
		{
			switch (m_horizontalAlign.value())
			{
			case HorizontalAlign::Left:
				offsetX = 0.0;
				break;
			case HorizontalAlign::Center:
				offsetX = (availableWidth - contentWidth) / 2.0;
				break;
			case HorizontalAlign::Right:
				offsetX = availableWidth - contentWidth;
				break;
			}
		}
		else
		{
			// スクロールが必要な場合は水平揃えしない
			offsetX = 0.0;
		}
		
		// 垂直揃え
		const double contentHeight = m_cache.lineHeight;
		const double availableHeight = rect.h;
		
		if (contentHeight <= availableHeight)
		{
			switch (m_verticalAlign.value())
			{
			case VerticalAlign::Top:
				offsetY = 0.0;
				break;
			case VerticalAlign::Middle:
				offsetY = (availableHeight - contentHeight) / 2.0;
				break;
			case VerticalAlign::Bottom:
				offsetY = availableHeight - contentHeight;
				break;
			}
		}
		
		return Vec2{ offsetX, offsetY };
	}

	size_t TextBox::moveCursorToMousePos(const RectF& rect, const std::shared_ptr<Node>& node)
	{
		m_cache.refreshIfDirty(
			m_text.value(),
			m_fontAssetName.value(),
			m_fontSize.value(),
			rect.size);

		const Vec2 alignOffset = getAlignOffset(rect);
		const double drawOffsetX = getDrawOffsetX();
		
		// ヒットテスト用逆変換でマウス座標を変換
		const Vec2 nodeSpaceMousePos = node ? node->inverseTransformHitTestPoint(Cursor::PosF()) : Cursor::PosF();
		
		const double posX = (nodeSpaceMousePos.x - rect.x) - alignOffset.x;
		
		return m_cache.getCursorIndex(drawOffsetX, m_scrollOffset, posX);
	}


	bool TextBox::hasSelection() const
	{
		return (m_cursorIndex != m_selectionAnchor);
	}

	std::pair<size_t, size_t> TextBox::getSelectionRange() const
	{
		const size_t begin = Min(m_cursorIndex, m_selectionAnchor);
		const size_t end = Max(m_cursorIndex, m_selectionAnchor);
		return { begin, end };
	}

	String TextBox::getSelectedText() const
	{
		if (!hasSelection())
		{
			return U"";
		}
		const auto [b, e] = getSelectionRange();
		return m_text.value().substr(b, e - b);
	}

	void TextBox::deleteSelection()
	{
		if (!hasSelection())
		{
			return;
		}
		const auto [b, e] = getSelectionRange();
		String text = m_text.value();
		text.erase(b, e - b);
		m_text.setValue(text);
		m_cursorIndex = m_selectionAnchor = b;
	}

	void TextBox::insertTextAtCursor(StringView str)
	{
		String text = m_text.value();
		text.insert(m_cursorIndex, str);
		m_text.setValue(text);
		m_cursorIndex += str.size();
		m_selectionAnchor = m_cursorIndex;
	}

	void TextBox::handleClipboardShortcut()
	{
		if (!m_isEditing || m_isDragging)
		{
			return;
		}

		const bool ctrl = KeyControl.pressed();
		const bool alt = KeyAlt.pressed();
		const bool shift = KeyShift.pressed();

		const bool ctrlOnly = ctrl && !alt && !shift;
		if (!ctrlOnly)
		{
			return;
		}

		// Ctrl+C
		if (KeyC.down())
		{
			if (hasSelection())
			{
				Clipboard::SetText(getSelectedText());
			}
			return;
		}
		
		// Ctrl+A
		if (KeyA.down())
		{
			m_cursorIndex = m_text.value().size();
			m_selectionAnchor = 0;
			return;
		}

		// readOnly時はカット・ペーストを無効化
		if (m_readOnly.value())
		{
			return;
		}

		// Ctrl + X
		if (KeyX.down())
		{
			if (hasSelection())
			{
				Clipboard::SetText(getSelectedText());
				deleteSelection();
				m_isChanged = true;
			}
			return;
		}

		// Ctrl + V
		if (KeyV.down())
		{
			String clip;
			Clipboard::GetText(clip);
			if (clip.isEmpty())
			{
				return;
			}

			if (hasSelection())
			{
				deleteSelection();
			}
			insertTextAtCursor(clip);
			m_isChanged = true;
			return;
		}
	}

	void TextBox::onDeactivated(const std::shared_ptr<Node>& node)
	{
		CurrentFrame::UnfocusNodeIfFocused(node);
	}

	void TextBox::focus(const std::shared_ptr<Node>& node)
	{
		// readOnly時もフォーカスは受け取るが、編集状態にはしない
		if (!m_readOnly.value())
		{
			m_isEditing = true;
			m_cursorBlinkTime = 0.0;
			// 全選択状態にする
			m_selectionAnchor = 0;
			m_cursorIndex = m_text.value().size();
			node->setStyleState(U"selected");
			detail::s_canvasUpdateContext.editingTextBox = shared_from_this();
		}
	}

	void TextBox::blur(const std::shared_ptr<Node>& node)
	{
		m_isEditing = false;
		m_isDragging = false;
		node->setStyleState(U"");
		m_selectionAnchor = m_cursorIndex;
		if (auto editingTextBox = detail::s_canvasUpdateContext.editingTextBox.lock(); editingTextBox && editingTextBox.get() == static_cast<ITextBox*>(this))
		{
			detail::s_canvasUpdateContext.editingTextBox.reset();
		}
	}

	void TextBox::updateKeyInput(const std::shared_ptr<Node>& node)
	{
		m_prevActiveInHierarchy = true;
		m_isChanged = false;

		// Interactableがfalseの場合、または他のテキストボックスが編集中の場合は選択解除
		if (m_isEditing && (!node->interactable() || GetEditingTextBox().get() != static_cast<ITextBox*>(this)))
		{
			CurrentFrame::UnfocusNodeIfFocused(node);
			return;
		}

		const Vec2 horizontalPadding = m_horizontalPadding.value();
		const Vec2 verticalPadding = m_verticalPadding.value();

		// stretchedはtop,rigght,bottom,leftの順
		const RectF rect = node->layoutAppliedRect().stretched(-verticalPadding.x, -horizontalPadding.y, -verticalPadding.y, -horizontalPadding.x);

		if (m_isDragging)
		{
			// ドラッグ中
			if (MouseL.pressed())
			{
				// 押下中はマウス座標にあわせてカーソル移動
				auto fnGetDragScrollInterval = [](double distance) -> Duration
					{
						return Max(0.2s / Max(distance / 5, 1.0), 0.01s);
					};
				
				// ヒットテスト用逆変換でマウス座標を変換
				const Vec2 nodeSpaceMousePos = node->inverseTransformHitTestPoint(Cursor::PosF());
				
				const double mouseCursorPosX = nodeSpaceMousePos.x;
				if (mouseCursorPosX < rect.x)
				{
					// テキストボックス左側の領域外にマウスカーソルがある場合のカーソル移動
					const Duration dragScrollInterval = fnGetDragScrollInterval(rect.x - mouseCursorPosX);
					if (m_cursorIndex > 0 && (!m_dragScrollStopwatch.isStarted() || m_dragScrollStopwatch.elapsed() > dragScrollInterval))
					{
						--m_cursorIndex;
						if (m_scrollOffset < m_cursorIndex)
						{
							// 遅れて見えないよう見えている範囲の左端までは即座に移動
							m_cursorIndex = m_scrollOffset;
						}
						m_dragScrollStopwatch.restart();
					}
				}
				else if (mouseCursorPosX > rect.br().x)
				{
					// テキストボックス右側の領域外にマウスカーソルがある場合のカーソル移動
					const Duration dragScrollInterval = fnGetDragScrollInterval(mouseCursorPosX - rect.br().x);
					if (m_cursorIndex < m_text.value().size() && (!m_dragScrollStopwatch.isStarted() || m_dragScrollStopwatch.elapsed() > dragScrollInterval))
					{
						++m_cursorIndex;

						m_cache.refreshIfDirty(
							m_text.value(),
							m_fontAssetName.value(),
							m_fontSize.value(),
							rect.size);
						const size_t rightMostCursorIndex = m_cache.getCursorIndex(getDrawOffsetX(), m_scrollOffset, rect.w);
						if (m_cursorIndex < rightMostCursorIndex)
						{
							// 遅れて見えないよう見えている範囲の右端までは即座に移動
							m_cursorIndex = rightMostCursorIndex;
						}
						m_dragScrollStopwatch.restart();
					}
				}
				else
				{
					// テキストボックス内にマウスカーソルがある場合のカーソル移動
					m_cursorIndex = moveCursorToMousePos(rect, node);
				}

				updateScrollOffset(rect);
			}
			else
			{
				// 離したらドラッグ終了
				m_isDragging = false;
			}
		}
		else
		{
			if (node->isMouseDown() || node->isClickRequested())
			{
				// 領域内で左クリックし始めた場合
				m_cursorBlinkTime = 0.0;
				node->setStyleState(U"selected");
				if (!m_isEditing)
				{
					// 初回クリック時の処理
					if (m_readOnly.value())
					{
						// readOnly時はクリック位置にカーソルを移動
						m_selectionAnchor = moveCursorToMousePos(rect, node);
						m_cursorIndex = m_selectionAnchor;
						m_isDragging = true;
						node->preventDragScroll();
					}
					else
					{
						// 通常時は全て選択
						m_selectionAnchor = 0;
						m_cursorIndex = m_text.value().size();
						m_isDragging = false;
						node->preventDragScroll();
					}
				}
				else if (!KeyShift.pressed())
				{
					// Shiftを押していなければ選択をリセットして選択起点を新たに設定し直す
					m_selectionAnchor = moveCursorToMousePos(rect, node);
					m_cursorIndex = m_selectionAnchor;
					m_isDragging = true;
					node->preventDragScroll();
				}
				else
				{
					// Shiftを押しながらクリックした場合、既存の起点を維持しつつカーソルのみクリック位置へ移動
					m_cursorIndex = moveCursorToMousePos(rect, node);
					m_isDragging = true;
					node->preventDragScroll();
				}
				m_isEditing = true;
				CurrentFrame::SetFocusedNode(node);
				updateScrollOffset(rect);
			}
			else if (node->isRightMouseDown())
			{
				// 右クリック時は何もしない
			}
			else if (MouseL.down() || MouseM.down() || MouseR.down())
			{
				// 領域外をクリックした場合は選択解除
				CurrentFrame::UnfocusNodeIfFocused(node);
				if (auto editingTextBox = detail::s_canvasUpdateContext.editingTextBox.lock(); editingTextBox && editingTextBox.get() == static_cast<ITextBox*>(this))
				{
					detail::s_canvasUpdateContext.editingTextBox.reset();
				}
			}
		}

		if (node->isHovered())
		{
			// カーソルを文字選択カーソルに変更
			Cursor::RequestStyle(CursorStyle::IBeam);
		}

		if (m_isEditing)
		{
			detail::s_canvasUpdateContext.editingTextBox = shared_from_this();

			const size_t prevCursorIndex = m_cursorIndex;

			const bool ctrl = KeyControl.pressed();
			const bool alt = KeyAlt.pressed();
			const bool shift = KeyShift.pressed();
			bool keyMoveTried = false;

			const bool editingTextExists = !TextInput::GetEditingText().empty();
			if (!m_prevEditingTextExists && !editingTextExists && !m_isDragging) // 未変換テキストがある場合はテキスト編集・カーソル移動しない(Windows環境だとEnterによる確定時は空なので、前フレームも見る)、ドラッグ中もキー操作を無効化
			{
				if (KeyLeft.down() || (KeyLeft.pressedDuration() > 0.4s && m_leftPressStopwatch.elapsed() > 0.03s))
				{
					if (m_selectionAnchor != m_cursorIndex && !shift)
					{
						m_cursorIndex = Min(m_selectionAnchor, m_cursorIndex);
						m_selectionAnchor = m_cursorIndex;
					}
					else if (m_cursorIndex > 0)
					{
						--m_cursorIndex;
					}
					keyMoveTried = true;
					m_leftPressStopwatch.restart();
				}

				if (KeyRight.down() || (KeyRight.pressedDuration() > 0.4s && m_rightPressStopwatch.elapsed() > 0.03s))
				{
					if (m_selectionAnchor != m_cursorIndex && !shift)
					{
						m_cursorIndex = Max(m_cursorIndex, m_selectionAnchor);
						m_selectionAnchor = m_cursorIndex;
					}
					else if (m_cursorIndex < m_text.value().size())
					{
						++m_cursorIndex;
					}
					keyMoveTried = true;
					m_rightPressStopwatch.restart();
				}

				if (KeyHome.down())
				{
					m_cursorIndex = 0;
					keyMoveTried = true;
				}

				if (KeyEnd.down())
				{
					m_cursorIndex = m_text.value().size();
					keyMoveTried = true;
				}

				if (!m_readOnly.value() && (KeyBackspace.down() || (KeyBackspace.pressedDuration() > 0.4s && m_backspacePressStopwatch.elapsed() > 0.03s)))
				{
					if (m_cursorIndex != m_selectionAnchor)
					{
						const size_t erasePos = Min(m_cursorIndex, m_selectionAnchor);
						const size_t eraseLen = (m_cursorIndex > m_selectionAnchor) ? (m_cursorIndex - m_selectionAnchor) : (m_selectionAnchor - m_cursorIndex);
						String text = m_text.value();
						text.erase(erasePos, eraseLen);
						m_text.setValue(text);
						m_cursorIndex = erasePos;
						m_selectionAnchor = m_cursorIndex;
						m_isChanged = true;
					}
					else if (m_cursorIndex > 0)
					{
						String text = m_text.value();
						text.erase(m_cursorIndex - 1, 1);
						m_text.setValue(text);
						--m_cursorIndex;
						m_selectionAnchor = m_cursorIndex;
						m_isChanged = true;
					}
					keyMoveTried = true;
					m_backspacePressStopwatch.restart();
				}

				if (!m_readOnly.value() && (KeyDelete.down() || (KeyDelete.pressedDuration() > 0.4s && m_deletePressStopwatch.elapsed() > 0.03s)))
				{
					if (m_cursorIndex != m_selectionAnchor)
					{
						const size_t erasePos = Min(m_cursorIndex, m_selectionAnchor);
						const size_t eraseLen = (m_cursorIndex > m_selectionAnchor) ? (m_cursorIndex - m_selectionAnchor) : (m_selectionAnchor - m_cursorIndex);
						String text = m_text.value();
						text.erase(erasePos, eraseLen);
						m_text.setValue(text);
						m_cursorIndex = erasePos;
						m_selectionAnchor = m_cursorIndex;
						m_isChanged = true;
					}
					else if (m_cursorIndex < m_text.value().size())
					{
						String text = m_text.value();
						text.erase(m_cursorIndex, 1);
						m_text.setValue(text);
						m_isChanged = true;
					}
					keyMoveTried = true;
					m_deletePressStopwatch.restart();
				}

				handleClipboardShortcut();
			}

			// CtrlキーまたはAltキーが押されている場合は通常の文字入力を無視
			if (!ctrl && !alt && !m_readOnly.value())
			{
				for (const auto c : TextInput::GetRawInput())
				{
					if (IsControl(c))
					{
						// 制御文字は無視
						continue;
					}

					// 文字入力があった場合はドラッグ状態を解除
					m_isDragging = false;

					String text = m_text.value();
					if (m_cursorIndex != m_selectionAnchor)
					{
						const size_t erasePos = Min(m_cursorIndex, m_selectionAnchor);
						const size_t eraseLen = (m_cursorIndex > m_selectionAnchor) ? (m_cursorIndex - m_selectionAnchor) : (m_selectionAnchor - m_cursorIndex);
						text.erase(erasePos, eraseLen);
						m_cursorIndex = erasePos;
					}
					text = text.substrView(0, m_cursorIndex) + c + text.substrView(m_cursorIndex);
					m_text.setValue(text);
					++m_cursorIndex;
					m_selectionAnchor = m_cursorIndex;
					m_isChanged = true;
				}
			}

			if (m_cursorIndex != prevCursorIndex || keyMoveTried)
			{
				m_cursorBlinkTime = 0.0;
				if (!shift)
				{
					// Shiftを離した状態で矢印キーを押した場合は選択解除
					m_selectionAnchor = m_cursorIndex;
				}
				updateScrollOffset(rect);
			}

			// カーソル点滅
			m_cursorBlinkTime += Scene::DeltaTime();
			if (m_cursorBlinkTime >= 1.0)
			{
				m_cursorBlinkTime -= 1.0;
			}

			m_prevEditingTextExists = editingTextExists;
		}
		else
		{
			m_prevEditingTextExists = false;
		}

		if (m_text.value() != m_prevText)
		{
			m_isChanged = true;
			m_prevText = m_text.value();
		}
	}

	void TextBox::updateKeyInputInactive(const std::shared_ptr<Node>& node)
	{
		if (m_prevActiveInHierarchy)
		{
			// 前回はアクティブだったが今回は非アクティブになった場合
			onDeactivated(node);
			m_prevActiveInHierarchy = false;
		}
		m_prevEditingTextExists = false;
	}

	void TextBox::updateScrollOffset(const RectF& rect)
	{
		m_cache.refreshIfDirty(
			m_text.value(),
			m_fontAssetName.value(),
			m_fontSize.value(),
			rect.size);

		const double contentWidth = m_cache.regionSize.x;
		const double availableWidth = rect.w;
		if (contentWidth <= availableWidth)
		{
			m_scrollOffset = 0;
			m_fitDirection = FitDirection::Left;
			return;
		}

		const double cursorWidth = CursorWidth;
		for (size_t i = 0; i < 1000; ++i) // 無限ループ回避
		{
			double drawOffsetX = getDrawOffsetX();
			double cursorPosX = m_cache.getCursorPosX(drawOffsetX, m_scrollOffset, m_cursorIndex);

			// まずはフィット方向のみを更新して解決するか調べる
			if (cursorPosX < 0)
			{
				m_fitDirection = FitDirection::Left;
			}
			else if (cursorPosX >= Max(rect.w - cursorWidth, 0.0))
			{
				m_fitDirection = FitDirection::Right;
			}
			else
			{
				break;
			}

			// フィット方向を更新しても解決しない場合はスクロールオフセットを更新
			drawOffsetX = getDrawOffsetX();
			cursorPosX = m_cache.getCursorPosX(drawOffsetX, m_scrollOffset, m_cursorIndex);
			if (cursorPosX < 0)
			{
				if (m_scrollOffset > 0)
				{
					--m_scrollOffset;
				}
				else
				{
					break;
				}
			}
			else if (cursorPosX > Max(rect.w - cursorWidth, 0.0))
			{
				if (m_scrollOffset < m_cache.glyphs.size())
				{
					++m_scrollOffset;
				}
			}
			else
			{
				break;
			}
		}

		const double totalTextWidth = m_cache.regionSize.x;
		if (totalTextWidth + cursorWidth <= rect.w)
		{
			// 領域内に全て収まるのにスクロールしている場合はスクロールをリセット
			m_scrollOffset = 0;
			m_fitDirection = FitDirection::Left;
		}
		else if (m_scrollOffset > 0 && m_scrollOffset == m_cache.glyphs.size())
		{
			// 右端にスクロールしていて1文字も見えない状況を回避するために、最低限1文字は見えるようにする
			for (size_t i = 0; i < 1000; ++i) // 無限ループ回避
			{
				if (m_scrollOffset == 0)
				{
					break;
				}
				const double drawOffsetX = getDrawOffsetX();
				const double cursorPosX = m_cache.getCursorPosX(drawOffsetX, m_scrollOffset, m_cache.glyphs.size());
				if (cursorPosX > rect.w / 2)
				{
					break;
				}
				--m_scrollOffset;
				m_fitDirection = FitDirection::Left;
			}
		}
	}

	void TextBox::draw(const Node& node) const
	{
		const Vec2& horizontalPadding = m_horizontalPadding.value();
		const Vec2& verticalPadding = m_verticalPadding.value();

		// stretchedはtop,right,bottom,leftの順
		const RectF rect = node.layoutAppliedRect().stretched(-verticalPadding.x, -horizontalPadding.y, -verticalPadding.y, -horizontalPadding.x);
		
		// クリッピング用の矩形（Transformer2DがScissorRectに効かないため）
		const RectF clipRect = node.unrotatedRect().stretched(-verticalPadding.x, -horizontalPadding.y, -verticalPadding.y, -horizontalPadding.x);

		m_cache.refreshIfDirty(
			m_text.value(),
			m_fontAssetName.value(),
			m_fontSize.value(),
			rect.size);

		// Alignオフセットを計算
		const Vec2 alignOffset = getAlignOffset(rect);
		const double drawOffsetX = getDrawOffsetX();
		const Vec2 offset = rect.pos + Vec2{ drawOffsetX + alignOffset.x, alignOffset.y };

		{
			detail::ScopedScissorRect scissorRect{ clipRect.asRect() };

			// 選択範囲を描画
			if (m_selectionAnchor != m_cursorIndex)
			{
				const auto [selectionBegin, selectionEnd] = std::minmax(m_selectionAnchor, m_cursorIndex);
				const double xBegin = rect.pos.x + alignOffset.x + m_cache.getCursorPosX(drawOffsetX, m_scrollOffset, selectionBegin);
				const double xEnd = rect.pos.x + alignOffset.x + m_cache.getCursorPosX(drawOffsetX, m_scrollOffset, selectionEnd);

				const double selectionLeft = Min(xBegin, xEnd);
				const double selectionWidth = Abs(xEnd - xBegin);

				const RectF selectionRect{
					Vec2{ selectionLeft, offset.y },
					Vec2{ selectionWidth, m_cache.lineHeight } };
				selectionRect.draw(m_selectionColor.value());
			}

			// 各文字を描画
			Vec2 pos = offset;
			{
				const ScopedCustomShader2D shader{ Font::GetPixelShader(m_cache.fontMethod) };
				for (size_t index = m_scrollOffset; index < m_cache.glyphs.size(); ++index)
				{
					const auto& glyph = m_cache.glyphs[index];
					const ColorF& color = m_color.value();
					glyph.texture.scaled(m_cache.scale).draw(pos + glyph.getOffset(m_cache.scale), color);
					pos.x += glyph.xAdvance * m_cache.scale;
				}
			}

			// カーソルを描画
			if (m_isEditing && m_cursorBlinkTime < 0.5)
			{
				const double cursorWidth = CursorWidth;
				const double cursorHeight = m_cache.lineHeight;
				const double cursorX = rect.pos.x + alignOffset.x + m_cache.getCursorPosX(drawOffsetX, m_scrollOffset, m_cursorIndex);
				const RectF cursorRect{ Vec2{ cursorX, offset.y }, Vec2{ cursorWidth, cursorHeight } };
				cursorRect.draw(m_cursorColor.value());
			}
		}

		// 未変換テキストを描画
		if (m_isEditing)
		{
			if (const String editingText = TextInput::GetEditingText(); !editingText.empty())
			{
				m_editingCache.refreshIfDirty(
					editingText,
					m_fontAssetName.value(),
					m_fontSize.value(),
					rect.size);

				const double editingX = rect.pos.x + alignOffset.x + m_cache.getCursorPosX(drawOffsetX, m_scrollOffset, m_cursorIndex);
				const Vec2 editingOffset = Vec2{ editingX, offset.y };

				// 領域を塗りつぶし
				{
					const RectF editingRect = RectF{ editingOffset, m_editingCache.regionSize };
					editingRect.draw(ColorF{ 0.0, 0.6 });
				}

				// 各文字を描画
				Vec2 editingPos = editingOffset;
				{
					const ScopedCustomShader2D shader{ Font::GetPixelShader(m_editingCache.fontMethod) };
					for (const auto& glyph : m_editingCache.glyphs)
					{
						glyph.texture.scaled(m_editingCache.scale).draw(editingPos + glyph.getOffset(m_editingCache.scale));
						editingPos.x += glyph.xAdvance * m_editingCache.scale;
					}
				}
			}
		}
	}

	std::shared_ptr<TextBox> TextBox::setText(StringView text, IgnoreIsChangedYN ignoreIsChanged)
	{
		m_text.setValue(text);
		m_cursorIndex = 0;
		m_selectionAnchor = m_cursorIndex;
		m_scrollOffset = 0;
		if (ignoreIsChanged)
		{
			m_prevText = text;
		}
		return shared_from_this();
	}
}
