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

		const Font font = FontAsset(fontAssetName);
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

	size_t TextBox::moveCursorToMousePos(const RectF& rect, const Vec2& effectScale)
	{
		m_cache.refreshIfDirty(
			m_text,
			m_fontAssetName.value(),
			m_fontSize.value(),
			rect.size);

		const double posX = (Cursor::PosF().x - rect.x) / effectScale.x;
		const double drawOffsetX = getDrawOffsetX();
		return m_cache.getCursorIndex(drawOffsetX, m_scrollOffset, posX);
	}

	void TextBox::onDeactivated(CanvasUpdateContext* pContext, const std::shared_ptr<Node>& node)
	{
		deselect(node);
		if (pContext)
		{
			pContext->editingTextBox.reset();
		}
		m_isDragging = false;
	}

	void TextBox::update(CanvasUpdateContext* pContext, const std::shared_ptr<Node>& node)
	{
		m_isChanged = false;

		// Interactableがfalseの場合、または他のテキストボックスが編集中の場合は選択解除
		if (m_isEditing && (!node->interactable() || (pContext && !pContext->editingTextBox.expired() && pContext->editingTextBox.lock().get() != this)))
		{
			deselect(node);
			if (pContext && pContext->editingTextBox.lock().get() == this)
			{
				pContext->editingTextBox.reset();
			}
			return;
		}

		const Vec2& effectScale = node->effectScale();
		const Vec2& horizontalPadding = m_horizontalPadding.value() * effectScale.x;
		const Vec2& verticalPadding = m_verticalPadding.value() * effectScale.y;

		// stretchedはtop,rigght,bottom,leftの順
		const RectF rect = node->rect().stretched(-verticalPadding.x, -horizontalPadding.y, -verticalPadding.y, -horizontalPadding.x);

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
				const double mouseCursorPosX = Cursor::PosF().x;
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
					if (m_cursorIndex < m_text.size() && (!m_dragScrollStopwatch.isStarted() || m_dragScrollStopwatch.elapsed() > dragScrollInterval))
					{
						++m_cursorIndex;

						m_cache.refreshIfDirty(
							m_text,
							m_fontAssetName.value(),
							m_fontSize.value(),
							rect.size);
						const size_t rightMostCursorIndex = m_cache.getCursorIndex(getDrawOffsetX(), m_scrollOffset, rect.w / effectScale.x);
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
					m_cursorIndex = moveCursorToMousePos(rect, effectScale);
				}

				updateScrollOffset(rect, effectScale);
			}
			else
			{
				// 離したらドラッグ終了
				m_isDragging = false;
			}
		}
		else
		{
			if (node->isMouseDown())
			{
				// 領域内で左クリックし始めた場合
				m_cursorBlinkTime = 0.0;
				node->setSelected(SelectedYN::Yes);
				if (!m_isEditing)
				{
					// 初回クリック時は全て選択
					m_selectionAnchor = 0;
					m_cursorIndex = m_text.size();
					m_isDragging = false;
				}
				else if (!KeyShift.pressed())
				{
					// Shiftを押していなければ選択をリセットして選択起点を新たに設定し直す
					m_selectionAnchor = moveCursorToMousePos(rect, effectScale);
					m_cursorIndex = m_selectionAnchor;
					m_isDragging = true;
				}
				else
				{
					// Shiftを押しながらクリックした場合、既存の起点を維持しつつカーソルのみクリック位置へ移動
					m_cursorIndex = moveCursorToMousePos(rect, effectScale);
					m_isDragging = true;
				}
				m_isEditing = true;
				updateScrollOffset(rect, effectScale);
			}
			else if (node->isRightMouseDown())
			{
				// 右クリック時は何もしない
			}
			else if (MouseL.down() || MouseM.down() || MouseR.down())
			{
				// 領域外をクリックした場合は選択解除
				deselect(node);
				if (pContext && pContext->editingTextBox.lock().get() == this)
				{
					pContext->editingTextBox.reset();
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
			if (pContext)
			{
				pContext->editingTextBox = shared_from_this();
			}

			const size_t prevCursorIndex = m_cursorIndex;

			const bool shift = KeyShift.pressed();
			const bool ctrl = KeyControl.pressed();
			bool keyMoveTried = false;

			if (KeyLeft.down() || (KeyLeft.pressedDuration() > 0.4s && m_leftPressStopwatch.elapsed() > 0.03s))
			{
				if (m_selectionAnchor != m_cursorIndex && !shift)
				{
					m_cursorIndex = Min(m_cursorIndex, m_selectionAnchor);
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
				else if (m_cursorIndex < m_text.size())
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
				m_cursorIndex = m_text.size();
				keyMoveTried = true;
			}

			if (KeyBackspace.down() || (KeyBackspace.pressedDuration() > 0.4s && m_backspacePressStopwatch.elapsed() > 0.03s))
			{
				if (m_cursorIndex != m_selectionAnchor)
				{
					m_text.erase(Min(m_cursorIndex, m_selectionAnchor), Abs(static_cast<int64>(m_cursorIndex) - static_cast<int64>(m_selectionAnchor)));
					m_cursorIndex = Min(m_cursorIndex, m_selectionAnchor);
					m_selectionAnchor = m_cursorIndex;
					m_isChanged = true;
				}
				else if (m_cursorIndex > 0)
				{
					m_text.erase(m_cursorIndex - 1, 1);
					--m_cursorIndex;
					m_selectionAnchor = m_cursorIndex;
					m_isChanged = true;
				}
				keyMoveTried = true;
				m_backspacePressStopwatch.restart();
			}

			if (KeyDelete.down() || (KeyDelete.pressedDuration() > 0.4s && m_deletePressStopwatch.elapsed() > 0.03s))
			{
				if (m_cursorIndex != m_selectionAnchor)
				{
					m_text.erase(Min(m_cursorIndex, m_selectionAnchor), Abs(static_cast<int64>(m_cursorIndex) - static_cast<int64>(m_selectionAnchor)));
					m_cursorIndex = Min(m_cursorIndex, m_selectionAnchor);
					m_selectionAnchor = m_cursorIndex;
					m_isChanged = true;
				}
				else if (m_cursorIndex < m_text.size())
				{
					m_text.erase(m_cursorIndex, 1);
					m_isChanged = true;
				}
				keyMoveTried = true;
				m_deletePressStopwatch.restart();
			}

			for (const auto c : TextInput::GetRawInput())
			{
				if (IsControl(c))
				{
					// 制御文字は無視
					continue;
				}

				if (m_cursorIndex != m_selectionAnchor)
				{
					m_text.erase(Min(m_cursorIndex, m_selectionAnchor), Abs(static_cast<int64>(m_cursorIndex) - static_cast<int64>(m_selectionAnchor)));
					m_cursorIndex = Min(m_cursorIndex, m_selectionAnchor);
				}
				m_text = m_text.substrView(0, m_cursorIndex) + c + m_text.substrView(m_cursorIndex);
				++m_cursorIndex;
				m_selectionAnchor = m_cursorIndex;
				m_isChanged = true;
			}

			if (ctrl && KeyA.down())
			{
				m_cursorIndex = m_text.size();
				m_selectionAnchor = 0;
			}
			else if (m_cursorIndex != prevCursorIndex || keyMoveTried)
			{
				m_cursorBlinkTime = 0.0;
				if (!shift)
				{
					// Shiftを離した状態で矢印キーを押した場合は選択解除
					m_selectionAnchor = m_cursorIndex;
				}
				updateScrollOffset(rect, node->effectScale());
			}

			// カーソル点滅
			m_cursorBlinkTime += Scene::DeltaTime();
			if (m_cursorBlinkTime >= 1.0)
			{
				m_cursorBlinkTime -= 1.0;
			}
		}

		if (m_text != m_prevText)
		{
			m_isChanged = true;
			m_prevText = m_text;
		}
	}

	void TextBox::updateScrollOffset(const RectF& rect, const Vec2& effectScale)
	{
		m_cache.refreshIfDirty(
			m_text,
			m_fontAssetName.value(),
			m_fontSize.value(),
			rect.size);

		const double cursorWidth = CursorWidth * effectScale.x;
		for (size_t i = 0; i < 1000; ++i) // 無限ループ回避
		{
			double drawOffsetX = getDrawOffsetX();
			double cursorPosX = m_cache.getCursorPosX(drawOffsetX, m_scrollOffset, m_cursorIndex) * effectScale.x;

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
			cursorPosX = m_cache.getCursorPosX(drawOffsetX, m_scrollOffset, m_cursorIndex) * effectScale.x;
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
				++m_scrollOffset;
			}
			else
			{
				break;
			}
		}

		const double totalTextWidth = m_cache.regionSize.x * effectScale.x;
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
				const double cursorPosX = m_cache.getCursorPosX(drawOffsetX, m_scrollOffset, m_cache.glyphs.size()) * effectScale.x;
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
		const Vec2& effectScale = node.effectScale();
		const Vec2& horizontalPadding = m_horizontalPadding.value() * effectScale.x;
		const Vec2& verticalPadding = m_verticalPadding.value() * effectScale.y;

		// stretchedはtop,rigght,bottom,leftの順
		const RectF rect = node.rect().stretched(-verticalPadding.x, -horizontalPadding.y, -verticalPadding.y, -horizontalPadding.x);

		m_cache.refreshIfDirty(
			m_text,
			m_fontAssetName.value(),
			m_fontSize.value(),
			rect.size);

		const double drawOffsetX = getDrawOffsetX();
		const Vec2 offset = rect.pos + Vec2{ drawOffsetX * effectScale.x, 0.0 };

		{
			detail::ScopedScissorRect scissorRect{ rect.asRect() };

			// 選択範囲を描画
			if (m_selectionAnchor != m_cursorIndex)
			{
				const auto [selectionBegin, selectionEnd] = std::minmax(m_selectionAnchor, m_cursorIndex);
				const double xBegin = rect.pos.x + m_cache.getCursorPosX(drawOffsetX, m_scrollOffset, selectionBegin) * effectScale.x;
				const double xEnd = rect.pos.x + m_cache.getCursorPosX(drawOffsetX, m_scrollOffset, selectionEnd) * effectScale.x;

				const double selectionLeft = Min(xBegin, xEnd);
				const double selectionWidth = Abs(xEnd - xBegin);

				const RectF selectionRect{
					Vec2{ selectionLeft, offset.y },
					Vec2{ selectionWidth, m_cache.regionSize.y * effectScale.y } };
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
					glyph.texture.scaled(m_cache.scale * effectScale).draw(pos + glyph.getOffset(m_cache.scale) * effectScale, color);
					pos.x += glyph.xAdvance * m_cache.scale * effectScale.x;
				}
			}

			// カーソルを描画
			if (m_isEditing && m_cursorBlinkTime < 0.5)
			{
				const double cursorWidth = CursorWidth * effectScale.x;
				const double cursorHeight = m_cache.regionSize.y * effectScale.y;
				const RectF cursorRect{ rect.pos + Vec2{ m_cache.getCursorPosX(drawOffsetX, m_scrollOffset, m_cursorIndex) * effectScale.x, 0.0 }, Vec2{ cursorWidth, cursorHeight } };
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

				const Vec2 editingOffset = offset + Vec2{ m_cache.getCursorPosX(drawOffsetX, m_scrollOffset, m_cursorIndex) * effectScale.x, 0.0 };

				// 領域を塗りつぶし
				{
					const RectF editingRect = RectF{ editingOffset, m_editingCache.regionSize * effectScale };
					editingRect.draw(ColorF{ 0.0, 0.6 });
				}

				// 各文字を描画
				Vec2 editingPos = editingOffset;
				{
					const ScopedCustomShader2D shader{ Font::GetPixelShader(m_editingCache.fontMethod) };
					for (const auto& glyph : m_editingCache.glyphs)
					{
						glyph.texture.scaled(m_editingCache.scale * effectScale).draw(editingPos + glyph.getOffset(m_editingCache.scale) * effectScale);
						editingPos.x += glyph.xAdvance * m_editingCache.scale * effectScale.x;
					}
				}
			}
		}
	}

	void TextBox::deselect(const std::shared_ptr<Node>& node)
	{
		m_isEditing = false;
		m_isDragging = false;
		node->setSelected(SelectedYN::No);
		m_selectionAnchor = m_cursorIndex;
	}

	void TextBox::setText(StringView text, IgnoreIsChangedYN ignoreIsChanged)
	{
		m_text = text;
		m_cursorIndex = 0;
		m_selectionAnchor = m_cursorIndex;
		m_scrollOffset = 0;
		if (ignoreIsChanged)
		{
			m_prevText = text;
		}
	}
}
