#include "NocoUI/Component/TextArea.hpp"
#include "NocoUI/Canvas.hpp"
#include "NocoUI/detail/ScopedScissorRect.hpp"
namespace noco
{
	void TextArea::Cache::refreshIfDirty(StringView text, StringView fontAssetName, double fontSize, const SizeF& rectSize)
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
		const Array<Glyph> allGlyphs = font.getGlyphs(text);
		const int32 baseFontSize = font.fontSize();
		scale = (baseFontSize == 0) ? 1.0 : (fontSize / baseFontSize);
		lineHeight = font.height(fontSize);
		lines.clear();

		Array<Glyph> lineGlyphs;
		size_t lineBeginIndex = 0;
		size_t currentIndex = 0;
		double lineWidth = 0.0;
		double maxWidth = 0.0;
		const auto fnPushLine = [&]()
		{
			lines.push_back({
				lineGlyphs,
				lineBeginIndex,
				currentIndex,
				lineWidth
			});
			lineGlyphs.clear();
			lineBeginIndex = currentIndex;
			maxWidth = Max(maxWidth, lineWidth);
			lineWidth = 0.0;
		};

		for (size_t i = 0; i < text.length(); ++i, ++currentIndex)
		{
			if (text[i] == U'\n')
			{
				// 改行文字のglyphは描画しないので、lineGlyphsには追加しない
				fnPushLine();
				lineBeginIndex = currentIndex + 1;
				continue;
			}
			// 改行文字以外の場合、対応するglyphを追加
			if (i < allGlyphs.size())
			{
				const auto& glyph = allGlyphs[i];
				const double glyphWidth = glyph.xAdvance * scale;
				if (lineWidth + glyphWidth > rectSize.x && !lineGlyphs.empty())
				{
					fnPushLine();
				}
				lineGlyphs.push_back(glyph);
				lineWidth += glyphWidth;
			}
		}
		// 最後の行を追加
		// テキストが空でない場合、または改行で終わっている場合は必ず最後の行を追加
		if (!text.empty())
		{
			fnPushLine();
		}
		regionSize = { maxWidth, lines.size() * lineHeight };
	}

	Vec2 TextArea::Cache::getCursorPos(size_t line, size_t column, size_t scrollOffsetX, size_t scrollOffsetY) const
	{
		if (lines.empty())
		{
			return Vec2::Zero();
		}
		if (line >= lines.size())
		{
			line = lines.empty() ? 0 : lines.size() - 1;
		}
		double y = (static_cast<double>(line) - static_cast<double>(scrollOffsetY)) * lineHeight;
		const auto& lineCache = lines[line];
		double x = 0.0;
		// スクロールオフセットまでの文字幅を計算して減算
		for (size_t i = 0; i < Min(scrollOffsetX, lineCache.glyphs.size()); ++i)
		{
			x -= lineCache.glyphs[i].xAdvance * scale;
		}
		// カーソル位置までの文字幅を加算
		const size_t glyphCount = Min(column, lineCache.glyphs.size());
		const size_t startIndex = Min(scrollOffsetX, lineCache.glyphs.size());
		for (size_t i = startIndex; i < glyphCount; ++i)
		{
			x += lineCache.glyphs[i].xAdvance * scale;
		}
		return { x, y };
	}

	std::pair<size_t, size_t> TextArea::Cache::getCursorIndex(const Vec2& pos, size_t scrollOffsetX, size_t scrollOffsetY) const
	{
		if (lines.empty())
		{
			return { 0, 0 };
		}
		size_t line = static_cast<size_t>(Max(0.0, pos.y / lineHeight + scrollOffsetY));
		line = Min(line, lines.size() - 1);
		if (line >= lines.size())
		{
			return { lines.size() - 1, 0 };
		}
		const auto& lineCache = lines[line];
		if (lineCache.glyphs.empty())
		{
			return { line, 0 };
		}
		double x = 0.0;
		// スクロールオフセットまでの文字幅を計算して減算
		for (size_t i = 0; i < Min(scrollOffsetX, lineCache.glyphs.size()); ++i)
		{
			x -= lineCache.glyphs[i].xAdvance * scale;
		}
		// マウス位置に対応する文字インデックスを検索
		for (size_t i = 0; i < lineCache.glyphs.size(); ++i)
		{
			const double glyphWidth = lineCache.glyphs[i].xAdvance * scale;
			const double nextX = x + glyphWidth;
			const double halfX = (x + nextX) / 2;
			if (pos.x < halfX)
			{
				return { line, i };
			}
			x = nextX;
		}
		return { line, lineCache.glyphs.size() };
	}

	size_t TextArea::Cache::getLineColumnToIndex(size_t line, size_t column) const
	{
		if (lines.empty())
		{
			return 0;
		}
		if (line >= lines.size())
		{
			return lines.back().textEndIndex;
		}
		const auto& lineCache = lines[line];
		return lineCache.textBeginIndex + Min(column, lineCache.textEndIndex - lineCache.textBeginIndex);
	}

	std::pair<size_t, size_t> TextArea::Cache::getIndexToLineColumn(size_t index) const
	{
		for (size_t i = 0; i < lines.size(); ++i)
		{
			const auto& lineCache = lines[i];
			if (index >= lineCache.textBeginIndex && index <= lineCache.textEndIndex)
			{
				return { i, index - lineCache.textBeginIndex };
			}
		}
		// 見つからない場合は最後の行の最後にカーソルを配置
		if (!lines.empty())
		{
			const auto& lastLine = lines.back();
			return { lines.size() - 1, lastLine.textEndIndex - lastLine.textBeginIndex };
		}
		return { 0, 0 };
	}

	std::pair<size_t, size_t> TextArea::moveCursorToMousePos(const RectF& rect, const std::shared_ptr<Node>& node)
	{
		m_cache.refreshIfDirty(
			m_text.value(),
			m_fontAssetName.value(),
			m_fontSize.value(),
			rect.size);
		
		// ヒットテスト用逆変換でマウス座標を変換
		const Vec2 nodeSpaceMousePos = node ? node->inverseTransformHitTestPoint(Cursor::PosF()) : Cursor::PosF();
		
		const Vec2 pos = nodeSpaceMousePos - rect.pos;
		return m_cache.getCursorIndex(pos, m_scrollOffsetX, m_scrollOffsetY);
	}

	bool TextArea::hasSelection() const
	{
		return (m_cursorLine != m_selectionAnchorLine || m_cursorColumn != m_selectionAnchorColumn);
	}

	std::pair<size_t, size_t> TextArea::getSelectionRange() const
	{
		const size_t cursorIndex = m_cache.getLineColumnToIndex(m_cursorLine, m_cursorColumn);
		const size_t anchorIndex = m_cache.getLineColumnToIndex(m_selectionAnchorLine, m_selectionAnchorColumn);
		return { Min(cursorIndex, anchorIndex), Max(cursorIndex, anchorIndex) };
	}

	String TextArea::getSelectedText() const
	{
		if (!hasSelection())
		{
			return U"";
		}
		const auto [b, e] = getSelectionRange();
		return m_text.value().substr(b, e - b);
	}

	void TextArea::deleteSelection()
	{
		if (!hasSelection())
		{
			return;
		}
		const auto [b, e] = getSelectionRange();
		String text = m_text.value();
		text.erase(b, e - b);
		m_text.setValue(text);
		const auto [line, column] = m_cache.getIndexToLineColumn(b);
		m_cursorLine = m_selectionAnchorLine = line;
		m_cursorColumn = m_selectionAnchorColumn = column;
	}

	std::pair<size_t, size_t> TextArea::insertTextAtCursor(StringView str)
	{
		const size_t cursorIndex = m_cache.getLineColumnToIndex(m_cursorLine, m_cursorColumn);
		String text = m_text.value();
		text.insert(cursorIndex, str);
		m_text.setValue(text);
		// 挿入位置と挿入サイズを返す
		return { cursorIndex, str.size() };
	}

	std::tuple<bool, size_t, size_t> TextArea::handleShortcut()
	{
		if (!m_isEditing || m_isDragging)
		{
			return { false, 0, 0 };
		}

		const bool ctrl = KeyControl.pressed();
		const bool alt = KeyAlt.pressed();
		const bool shift = KeyShift.pressed();
		const bool ctrlOnly = ctrl && !alt && !shift;
		if (!ctrlOnly)
		{
			return { false, 0, 0 };
		}

		if (KeyC.down())
		{
			if (hasSelection())
			{
				Clipboard::SetText(getSelectedText());
			}
			return { true, 0, 0 };
		}

		if (KeyA.down())
		{
			m_selectionAnchorLine = 0;
			m_selectionAnchorColumn = 0;
			if (getLineCount() > 0)
			{
				m_cursorLine = getLineCount() - 1;
				m_cursorColumn = getColumnCount(m_cursorLine);
			}
			else
			{
				m_cursorLine = 0;
				m_cursorColumn = 0;
			}
			return { true, 0, 0 };
		}

		// readOnly時はカット・ペーストを無効化
		if (m_readOnly.value())
		{
			return { false, 0, 0 };
		}

		if (KeyX.down())
		{
			if (hasSelection())
			{
				Clipboard::SetText(getSelectedText());
				deleteSelection();
				m_isChanged = true;
			}
			return { true, 0, 0 };
		}

		if (KeyV.down())
		{
			String clip;
			Clipboard::GetText(clip);
			if (clip.isEmpty())
			{
				return { false, 0, 0 };
			}
			if (hasSelection())
			{
				deleteSelection();
			}
			const auto [index, size] = insertTextAtCursor(clip);
			m_isChanged = true;
			return { true, index, size };
		}

		return { false, 0, 0 };
	}

	void TextArea::onDeactivated(const std::shared_ptr<Node>& node)
	{
		CurrentFrame::UnfocusNodeIfFocused(node);
	}

	void TextArea::updateScrollOffset(const RectF& rect)
	{
		m_cache.refreshIfDirty(
			m_text.value(),
			m_fontAssetName.value(),
			m_fontSize.value(),
			rect.size);

		const Vec2 cursorPos = m_cache.getCursorPos(m_cursorLine, m_cursorColumn, m_scrollOffsetX, m_scrollOffsetY);

		// 現状は水平スクロールは行わない
		// (将来的に折返し無効オプションを追加した場合に必要になる)
		m_scrollOffsetX = 0;

		// 垂直スクロール調整
		if (m_cache.lines.empty())
		{
			m_scrollOffsetY = 0;
			return;
		}

		// カーソル位置を可視領域内に収めるための計算
		const double lineHeight = m_cache.lineHeight;
		const size_t visibleLines = static_cast<size_t>(Max(1.0, rect.h / lineHeight));
		const double cursorTop = m_cursorLine * lineHeight;
		const double cursorBottom = cursorTop + lineHeight;
		const double viewTop = m_scrollOffsetY * lineHeight;
		const double viewBottom = viewTop + rect.h;

		// カーソルが上側にはみ出している場合
		if (cursorTop < viewTop)
		{
			m_scrollOffsetY = m_cursorLine;
		}
		// カーソルが下側にはみ出している場合
		else if (cursorBottom > viewBottom)
		{
			// 可視領域の高さに収まる行数を計算
			if (m_cursorLine >= visibleLines - 1)
			{
				m_scrollOffsetY = m_cursorLine - visibleLines + 1;
			}
			else
			{
				m_scrollOffsetY = 0;
			}
		}

		// スクロールオフセットの範囲制限
		const size_t maxScrollY = (m_cache.lines.size() > visibleLines) ? m_cache.lines.size() - visibleLines : 0;
		m_scrollOffsetY = Min(m_scrollOffsetY, maxScrollY);
	}

	size_t TextArea::getLineCount() const
	{
		return m_cache.lines.size();
	}

	size_t TextArea::getColumnCount(size_t line) const
	{
		if (line >= m_cache.lines.size())
		{
			return 0;
		}
		const auto& lineCache = m_cache.lines[line];
		return lineCache.textEndIndex - lineCache.textBeginIndex;
	}

	void TextArea::focus(const std::shared_ptr<Node>& node)
	{
		// readOnly時もフォーカスは受け取るが、編集状態にはしない
		if (!m_readOnly.value())
		{
			m_isEditing = true;
			m_cursorBlinkTime = 0.0;
			node->setStyleState(U"selected");
			detail::s_canvasUpdateContext.editingTextBox = shared_from_this();
			CurrentFrame::SetFocusedNode(node);
		}
	}

	void TextArea::blur(const std::shared_ptr<Node>& node)
	{
		m_isEditing = false;
		m_isDragging = false;
		node->setStyleState(U"");
		m_selectionAnchorLine = m_cursorLine;
		m_selectionAnchorColumn = m_cursorColumn;
		if (auto editingTextBox = detail::s_canvasUpdateContext.editingTextBox.lock(); editingTextBox && editingTextBox.get() == static_cast<ITextBox*>(this))
		{
			detail::s_canvasUpdateContext.editingTextBox.reset();
		}
	}

	void TextArea::updateInput(const std::shared_ptr<Node>& node)
	{
		m_prevActiveInHierarchy = true;
		m_isChanged = false;

		if (m_isEditing && (!node->interactable() || GetEditingTextBox().get() != static_cast<ITextBox*>(this)))
		{
			CurrentFrame::UnfocusNodeIfFocused(node);
			return;
		}

		const Vec2 horizontalPadding = m_horizontalPadding.value();
		const Vec2 verticalPadding = m_verticalPadding.value();

		// stretchedはtop,right,bottom,leftの順
		const RectF rect = node->layoutAppliedRect().stretched(-verticalPadding.x, -horizontalPadding.y, -verticalPadding.y, -horizontalPadding.x);

		m_cache.refreshIfDirty(
			m_text.value(),
			m_fontAssetName.value(),
			m_fontSize.value(),
			rect.size);

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
				
				const Vec2 mousePos = nodeSpaceMousePos;
				bool needScroll = false;
				if (mousePos.x < rect.x)
				{
					// テキストボックス左側の領域外にマウスカーソルがある場合のカーソル移動
					const Duration dragScrollInterval = fnGetDragScrollInterval(rect.x - mousePos.x);
					if (m_cursorColumn > 0 && (!m_dragScrollStopwatch.isStarted() || m_dragScrollStopwatch.elapsed() > dragScrollInterval))
					{
						--m_cursorColumn;
						if (m_scrollOffsetX < m_cursorColumn)
						{
							// 遅れて見えないよう見えている範囲の左端までは即座に移動
							m_cursorColumn = m_scrollOffsetX;
						}
						needScroll = true;
						m_dragScrollStopwatch.restart();
					}
				}
				else if (mousePos.x > rect.br().x)
				{
					// テキストボックス右側の領域外にマウスカーソルがある場合のカーソル移動
					const Duration dragScrollInterval = fnGetDragScrollInterval(mousePos.x - rect.br().x);
					const size_t maxColumn = getColumnCount(m_cursorLine);
					if (m_cursorColumn < maxColumn && (!m_dragScrollStopwatch.isStarted() || m_dragScrollStopwatch.elapsed() > dragScrollInterval))
					{
						++m_cursorColumn;
						
						// 遅れて見えないよう見えている範囲の右端までは即座に移動
						const auto [rightMostLine, rightMostColumn] = m_cache.getCursorIndex(Vec2{ rect.w, (m_cursorLine - m_scrollOffsetY) * m_cache.lineHeight }, m_scrollOffsetX, m_scrollOffsetY);
						if (rightMostLine == m_cursorLine && m_cursorColumn < rightMostColumn)
						{
							m_cursorColumn = rightMostColumn;
						}
						
						needScroll = true;
						m_dragScrollStopwatch.restart();
					}
				}

				if (mousePos.y < rect.y)
				{
					// テキストボックス上側の領域外にマウスカーソルがある場合のカーソル移動
					const Duration dragScrollInterval = fnGetDragScrollInterval(rect.y - mousePos.y);
					if (m_cursorLine > 0 && (!m_dragScrollStopwatch.isStarted() || m_dragScrollStopwatch.elapsed() > dragScrollInterval))
					{
						--m_cursorLine;
						if (m_scrollOffsetY < m_cursorLine)
						{
							// 遅れて見えないよう見えている範囲の上端までは即座に移動
							m_cursorLine = m_scrollOffsetY;
						}
						m_cursorColumn = Min(m_cursorColumn, getColumnCount(m_cursorLine));
						needScroll = true;
						m_dragScrollStopwatch.restart();
					}
				}
				else if (mousePos.y > rect.br().y)
				{
					// テキストボックス下側の領域外にマウスカーソルがある場合のカーソル移動
					const Duration dragScrollInterval = fnGetDragScrollInterval(mousePos.y - rect.br().y);
					const size_t maxLine = getLineCount();
					if (m_cursorLine < maxLine - 1 && (!m_dragScrollStopwatch.isStarted() || m_dragScrollStopwatch.elapsed() > dragScrollInterval))
					{
						++m_cursorLine;
						
						// 遅れて見えないよう見えている範囲の下端までは即座に移動
						const double lineHeight = m_cache.lineHeight;
						const size_t visibleLines = static_cast<size_t>(Max(1.0, rect.h / lineHeight));
						const size_t bottomMostLine = Min(m_scrollOffsetY + visibleLines - 1, maxLine - 1);
						if (m_cursorLine < bottomMostLine)
						{
							m_cursorLine = bottomMostLine;
						}
						
						m_cursorColumn = Min(m_cursorColumn, getColumnCount(m_cursorLine));
						needScroll = true;
						m_dragScrollStopwatch.restart();
					}
				}

				if (!needScroll && rect.contains(mousePos))
				{
					// テキストボックス内にマウスカーソルがある場合のカーソル移動
					const auto [line, column] = moveCursorToMousePos(rect, node);
					m_cursorLine = line;
					m_cursorColumn = column;
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
					// 初回クリック時
					const auto [line, column] = moveCursorToMousePos(rect, node);
					m_selectionAnchorLine = line;
					m_selectionAnchorColumn = column;
					m_cursorLine = line;
					m_cursorColumn = column;
					m_isDragging = true;
					node->preventDragScroll();
				}
				else if (!KeyShift.pressed())
				{
					// Shiftを押していなければ選択をリセットして選択起点を新たに設定し直す
					const auto [line, column] = moveCursorToMousePos(rect, node);
					m_selectionAnchorLine = line;
					m_selectionAnchorColumn = column;
					m_cursorLine = line;
					m_cursorColumn = column;
					m_isDragging = true;
					node->preventDragScroll();
				}
				else
				{
					// Shiftを押しながらクリックした場合、既存の起点を維持しつつカーソルのみクリック位置へ移動
					const auto [line, column] = moveCursorToMousePos(rect, node);
					m_cursorLine = line;
					m_cursorColumn = column;
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

			const size_t prevCursorLine = m_cursorLine;
			const size_t prevCursorColumn = m_cursorColumn;

			const bool ctrl = KeyControl.pressed();
			const bool alt = KeyAlt.pressed();
			const bool shift = KeyShift.pressed();
			bool keyMoveTried = false;
			bool shortcutActionTried = false;

			const bool editingTextExists = !TextInput::GetEditingText().empty();
			if (!m_prevEditingTextExists && !editingTextExists && !m_isDragging) // 未変換テキストがある場合はテキスト編集・カーソル移動しない(Windows環境だとEnterによる確定時は空なので、前フレームも見る)、ドラッグ中もキー操作を無効化
			{
				if (KeyLeft.down() || (KeyLeft.pressedDuration() > 0.4s && m_leftPressStopwatch.elapsed() > 0.03s))
				{
					if (hasSelection() && !shift)
					{
						const auto [b, e] = getSelectionRange();
						const auto [line, column] = m_cache.getIndexToLineColumn(b);
						m_cursorLine = m_selectionAnchorLine = line;
						m_cursorColumn = m_selectionAnchorColumn = column;
					}
					else if (m_cursorColumn > 0)
					{
						--m_cursorColumn;
					}
					else if (m_cursorLine > 0)
					{
						--m_cursorLine;
						m_cursorColumn = getColumnCount(m_cursorLine);
					}
					keyMoveTried = true;
					m_leftPressStopwatch.restart();
				}

				if (KeyRight.down() || (KeyRight.pressedDuration() > 0.4s && m_rightPressStopwatch.elapsed() > 0.03s))
				{
					if (hasSelection() && !shift)
					{
						const auto [b, e] = getSelectionRange();
						const auto [line, column] = m_cache.getIndexToLineColumn(e);
						m_cursorLine = m_selectionAnchorLine = line;
						m_cursorColumn = m_selectionAnchorColumn = column;
					}
					else if (m_cursorColumn < getColumnCount(m_cursorLine))
					{
						++m_cursorColumn;
					}
					else if (getLineCount() > 0 && m_cursorLine < getLineCount() - 1)
					{
						++m_cursorLine;
						m_cursorColumn = 0;
					}
					keyMoveTried = true;
					m_rightPressStopwatch.restart();
				}

				if (KeyUp.down() || (KeyUp.pressedDuration() > 0.4s && m_upPressStopwatch.elapsed() > 0.03s))
				{
					if (m_cursorLine > 0)
					{
						--m_cursorLine;
						m_cursorColumn = Min(m_cursorColumn, getColumnCount(m_cursorLine));
					}
					keyMoveTried = true;
					m_upPressStopwatch.restart();
				}

				if (KeyDown.down() || (KeyDown.pressedDuration() > 0.4s && m_downPressStopwatch.elapsed() > 0.03s))
				{
					if (getLineCount() > 0 && m_cursorLine < getLineCount() - 1)
					{
						++m_cursorLine;
						m_cursorColumn = Min(m_cursorColumn, getColumnCount(m_cursorLine));
					}
					keyMoveTried = true;
					m_downPressStopwatch.restart();
				}

				if (KeyHome.down())
				{
					if (ctrl)
					{
						m_cursorLine = 0;
						m_cursorColumn = 0;
					}
					else
					{
						m_cursorColumn = 0;
					}
					keyMoveTried = true;
				}

				if (KeyEnd.down())
				{
					if (ctrl)
					{
						if (getLineCount() > 0)
						{
							m_cursorLine = getLineCount() - 1;
							m_cursorColumn = getColumnCount(m_cursorLine);
						}
						else
						{
							m_cursorLine = 0;
							m_cursorColumn = 0;
						}
					}
					else
					{
						m_cursorColumn = getColumnCount(m_cursorLine);
					}
					keyMoveTried = true;
				}

				if (KeyPageUp.down() || (KeyPageUp.pressedDuration() > 0.4s && m_pageUpPressStopwatch.elapsed() > 0.03s))
				{
					if (m_cursorLine > 0 && m_cache.lineHeight > 0.0)
					{
						const size_t pageLines = static_cast<size_t>(Max(1.0, rect.h / m_cache.lineHeight));
						m_cursorLine = m_cursorLine > pageLines ? (m_cursorLine - pageLines) : 0;
						m_cursorColumn = Min(m_cursorColumn, getColumnCount(m_cursorLine));
					}
					keyMoveTried = true;
					m_pageUpPressStopwatch.restart();
				}

				if (KeyPageDown.down() || (KeyPageDown.pressedDuration() > 0.4s && m_pageDownPressStopwatch.elapsed() > 0.03s))
				{
					if (getLineCount() > 0 && m_cache.lineHeight > 0.0)
					{
						const size_t pageLines = static_cast<size_t>(Max(1.0, rect.h / m_cache.lineHeight));
						m_cursorLine = Min(m_cursorLine + pageLines, getLineCount() - 1);
						m_cursorColumn = Min(m_cursorColumn, getColumnCount(m_cursorLine));
					}
					keyMoveTried = true;
					m_pageDownPressStopwatch.restart();
				}

				if (!m_readOnly.value() && (KeyBackspace.down() || (KeyBackspace.pressedDuration() > 0.4s && m_backspacePressStopwatch.elapsed() > 0.03s)))
				{
					if (hasSelection())
					{
						deleteSelection();
						m_isChanged = true;
					}
					else
					{
						const size_t cursorIndex = m_cache.getLineColumnToIndex(m_cursorLine, m_cursorColumn);
						if (cursorIndex > 0)
						{
							String text = m_text.value();
							text.erase(cursorIndex - 1, 1);
							m_text.setValue(text);
							const auto [line, column] = m_cache.getIndexToLineColumn(cursorIndex - 1);
							m_cursorLine = m_selectionAnchorLine = line;
							m_cursorColumn = m_selectionAnchorColumn = column;
							m_isChanged = true;
						}
					}
					keyMoveTried = true;
					m_backspacePressStopwatch.restart();
				}

				if (!m_readOnly.value() && (KeyDelete.down() || (KeyDelete.pressedDuration() > 0.4s && m_deletePressStopwatch.elapsed() > 0.03s)))
				{
					if (hasSelection())
					{
						deleteSelection();
						m_isChanged = true;
					}
					else
					{
						const size_t cursorIndex = m_cache.getLineColumnToIndex(m_cursorLine, m_cursorColumn);
						if (cursorIndex < m_text.value().size())
						{
							String text = m_text.value();
							text.erase(cursorIndex, 1);
							m_text.setValue(text);
							m_isChanged = true;
						}
					}
					keyMoveTried = true;
					m_deletePressStopwatch.restart();
				}

				if (!m_readOnly.value() && (KeyEnter.down() || (KeyEnter.pressedDuration() > 0.4s && m_enterPressStopwatch.elapsed() > 0.03s)))
				{
					if (hasSelection())
					{
						deleteSelection();
					}
					const size_t beforeIndex = m_cache.getLineColumnToIndex(m_cursorLine, m_cursorColumn);
					insertTextAtCursor(U"\n");

					m_cache.refreshIfDirty(
						m_text.value(),
						m_fontAssetName.value(),
						m_fontSize.value(),
						rect.size);

					// カーソルを次の行の先頭に移動
					const auto [line, column] = m_cache.getIndexToLineColumn(beforeIndex + 1);
					m_cursorLine = m_selectionAnchorLine = line;
					m_cursorColumn = m_selectionAnchorColumn = column;
					m_isChanged = true;
					m_enterPressStopwatch.restart();
				}

				const auto [shortcutActionTriedResult, shortcutInsertIndex, shortcutInsertSize] = handleShortcut();
				shortcutActionTried = shortcutActionTriedResult;
				if (shortcutActionTried && m_isChanged)
				{
					// テキストが変更された場合、キャッシュを更新
					m_cache.refreshIfDirty(
						m_text.value(),
						m_fontAssetName.value(),
						m_fontSize.value(),
						rect.size);

					// Ctrl+Vで挿入された場合、カーソル位置を挿入したテキストの末尾に更新
					if (shortcutInsertSize > 0)
					{
						const auto [line, column] = m_cache.getIndexToLineColumn(shortcutInsertIndex + shortcutInsertSize);
						m_cursorLine = m_selectionAnchorLine = line;
						m_cursorColumn = m_selectionAnchorColumn = column;
					}
				}
			}

			const String rawInput = TextInput::GetRawInput();
			// CtrlキーまたはAltキーが押されている場合は通常の文字入力を無視
			if (!rawInput.empty() && !ctrl && !alt && !m_readOnly.value())
			{
				// 文字入力があった場合はドラッグ状態を解除
				m_isDragging = false;
				
				if (hasSelection())
				{
					deleteSelection();
				}
				// 制御文字を除外
				String validInput;
				for (const auto c : rawInput)
				{
					if (!IsControl(c))
					{
						validInput += c;
					}
				}
				if (!validInput.empty())
				{
					const size_t cursorIndex = m_cache.getLineColumnToIndex(m_cursorLine, m_cursorColumn);
					String text = m_text.value();
					text = text.substrView(0, cursorIndex) + validInput + text.substrView(cursorIndex);
					m_text.setValue(text);

					m_cache.refreshIfDirty(
						m_text.value(),
						m_fontAssetName.value(),
						m_fontSize.value(),
						rect.size);

					const auto [line, column] = m_cache.getIndexToLineColumn(cursorIndex + validInput.size());
					m_cursorLine = m_selectionAnchorLine = line;
					m_cursorColumn = m_selectionAnchorColumn = column;
					m_isChanged = true;
				}
			}

			if ((m_cursorLine != prevCursorLine || m_cursorColumn != prevCursorColumn) || keyMoveTried)
			{
				m_cursorBlinkTime = 0.0;
				if (!shift && !shortcutActionTried)
				{
					m_selectionAnchorLine = m_cursorLine;
					m_selectionAnchorColumn = m_cursorColumn;
				}
				updateScrollOffset(rect);
			}

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

	void TextArea::updateInputInactive(const std::shared_ptr<Node>& node)
	{
		if (m_prevActiveInHierarchy)
		{
			onDeactivated(node);
			m_prevActiveInHierarchy = false;
		}
		m_prevEditingTextExists = false;
	}

	void TextArea::draw(const Node& node) const
	{
		const Vec2 horizontalPadding = m_horizontalPadding.value();
		const Vec2 verticalPadding = m_verticalPadding.value();

		// stretchedはtop,right,bottom,leftの順
		const RectF rect = node.layoutAppliedRect().stretched(-verticalPadding.x, -horizontalPadding.y, -verticalPadding.y, -horizontalPadding.x);
		
		// クリッピング用の矩形（Transformer2DがScissorRectに効かないため）
		const RectF clipRect = node.unrotatedRect().stretched(-verticalPadding.x, -horizontalPadding.y, -verticalPadding.y, -horizontalPadding.x);

		m_cache.refreshIfDirty(
			m_text.value(),
			m_fontAssetName.value(),
			m_fontSize.value(),
			rect.size);

		{
			detail::ScopedScissorRect scissorRect{ clipRect.asRect() };

			// 選択範囲を描画
			if (hasSelection())
			{
				const auto [selectionBegin, selectionEnd] = getSelectionRange();
				const auto [beginLine, beginColumn] = m_cache.getIndexToLineColumn(selectionBegin);
				const auto [endLine, endColumn] = m_cache.getIndexToLineColumn(selectionEnd);
				for (size_t line = beginLine; line <= endLine; ++line)
				{
					if (line >= m_scrollOffsetY && line < m_scrollOffsetY + static_cast<size_t>(rect.h / m_cache.lineHeight + 1))
					{
						size_t startColumn = (line == beginLine) ? beginColumn : 0;
						size_t endCol = (line == endLine) ? endColumn : getColumnCount(line);
						const Vec2 startPos = rect.pos + m_cache.getCursorPos(line, startColumn, m_scrollOffsetX, m_scrollOffsetY);
						const Vec2 endPos = rect.pos + m_cache.getCursorPos(line, endCol, m_scrollOffsetX, m_scrollOffsetY);
						// 表示領域内でクリップ
						const double selectionLeft = Max(startPos.x, rect.x);
						const double selectionRight = Min(endPos.x, rect.br().x);
						if (selectionLeft < selectionRight)
						{
							const RectF selectionRect{
								Vec2{ selectionLeft, startPos.y },
								Vec2{ selectionRight - selectionLeft, m_cache.lineHeight }
							};
							selectionRect.draw(m_selectionColor.value());
						}
					}
				}
			}

			// 各文字を描画
			{
				const ScopedCustomShader2D shader{ Font::GetPixelShader(m_cache.fontMethod) };
				for (size_t lineIndex = m_scrollOffsetY; lineIndex < m_cache.lines.size(); ++lineIndex)
				{
					const auto& line = m_cache.lines[lineIndex];
					const double y = rect.y + (lineIndex - m_scrollOffsetY) * m_cache.lineHeight;
					if (y > rect.br().y)
					{
						break;
					}
					double x = rect.x;
					// スクロールオフセットまでの文字幅を計算して減算
					for (size_t i = 0; i < Min(m_scrollOffsetX, line.glyphs.size()); ++i)
					{
						x -= line.glyphs[i].xAdvance * m_cache.scale;
					}
					// 表示する文字を描画
					for (size_t i = 0; i < line.glyphs.size(); ++i)
					{
						const auto& glyph = line.glyphs[i];
						// 改行文字はスキップ
						if (glyph.codePoint == U'\n')
						{
							continue;
						}
						if (x + glyph.xAdvance * m_cache.scale > rect.x && x < rect.br().x)
						{
							glyph.texture.scaled(m_cache.scale).draw(Vec2{ x, y } + glyph.getOffset(m_cache.scale), m_color.value());
						}
						x += glyph.xAdvance * m_cache.scale;
						if (x > rect.br().x)
						{
							break;
						}
					}
				}
			}

			// カーソルを描画
			if (m_isEditing && m_cursorBlinkTime < 0.5)
			{
				const double cursorWidth = CursorWidth;
				const Vec2 cursorPos = rect.pos + m_cache.getCursorPos(m_cursorLine, m_cursorColumn, m_scrollOffsetX, m_scrollOffsetY);
				const RectF cursorRect{ cursorPos, Vec2{ cursorWidth, m_cache.lineHeight } };
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
					SizeF{ std::numeric_limits<double>::max(), std::numeric_limits<double>::max() }); // 折り返しが必要ないため、適当な大きい値を指定

				const Vec2 editingOffset = rect.pos + m_cache.getCursorPos(m_cursorLine, m_cursorColumn, m_scrollOffsetX, m_scrollOffsetY);

				// 領域を塗りつぶし
				{
					const RectF editingRect = RectF{ editingOffset, m_editingCache.regionSize };
					editingRect.draw(ColorF{ 0.0, 0.6 });
				}

				// 各文字を描画
				Vec2 editingPos = editingOffset;
				{
					const ScopedCustomShader2D shader{ Font::GetPixelShader(m_editingCache.fontMethod) };
					for (const auto& glyph : m_editingCache.lines[0].glyphs)
					{
						glyph.texture.scaled(m_editingCache.scale).draw(editingPos + glyph.getOffset(m_editingCache.scale));
						editingPos.x += glyph.xAdvance * m_editingCache.scale;
					}
				}
			}
		}
	}

	std::shared_ptr<TextArea> TextArea::setText(StringView text, IgnoreIsChangedYN ignoreIsChanged)
	{
		m_text.setValue(text);
		m_cursorLine = 0;
		m_cursorColumn = 0;
		m_selectionAnchorLine = 0;
		m_selectionAnchorColumn = 0;
		m_scrollOffsetX = 0;
		m_scrollOffsetY = 0;
		if (ignoreIsChanged)
		{
			m_prevText = text;
		}
		return shared_from_this();
	}
}
