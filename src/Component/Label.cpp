#include "NocoUI/Component/Label.hpp"
#include "NocoUI/Node.hpp"

namespace noco
{
	bool Label::Cache::refreshIfDirty(StringView text, const Optional<Font>& fontOpt, StringView fontAssetName, double fontSize, const Vec2& spacing, HorizontalOverflow horizontalOverflow, VerticalOverflow verticalOverflow, const SizeF& rectSize, LabelSizingMode newSizingMode)
	{
		const bool hasCustomFont = fontOpt.has_value();
		const Font newFont = hasCustomFont ? *fontOpt : ((!fontAssetName.empty() && FontAsset::IsRegistered(fontAssetName)) ? FontAsset(fontAssetName) : SimpleGUI::GetFont());
		
		if (prevParams.has_value() && !prevParams->isDirty(text, fontAssetName, fontSize, horizontalOverflow, verticalOverflow, spacing, rectSize, hasCustomFont, newFont, newSizingMode))
		{
			return false;
		}
		prevParams = CacheParams
		{
			.text = String{ text },
			.fontAssetName = String{ fontAssetName },
			.fontSize = fontSize,
			.horizontalOverflow = horizontalOverflow,
			.verticalOverflow = verticalOverflow,
			.spacing = spacing,
			.rectSize = rectSize,
			.hasCustomFont = hasCustomFont,
			.customFont = newFont,
			.sizingMode = newSizingMode,
		};

		currentFont = newFont;
		fontMethod = newFont.method();
		const Array<Glyph> glyphs = newFont.getGlyphs(text);
		baseFontSize = newFont.fontSize();
		if (baseFontSize == 0)
		{
			scale = 1.0;
		}
		else
		{
			scale = fontSize / baseFontSize;
		}
		lineHeight = newFont.height(fontSize);

		lineCaches.clear();

		double maxWidth = 0.0;
		Vec2 offset = Vec2::Zero();
		Array<Glyph> lineGlyphs;

		const auto fnPushLine =
			[&]() -> bool
			{
				// 現在の行の下端位置を計算
				const double currentLineBottom = offset.y + lineHeight;
				
				// Clipモードで現在の行が矩形を超える場合は追加しない
				if (verticalOverflow == VerticalOverflow::Clip && currentLineBottom > rectSize.y)
				{
					return false;
				}
				
				if (!lineGlyphs.empty())
				{
					// 行末文字の右側に余白を入れないため、その分を引く
					offset.x -= spacing.x;
				}
				
				// 矩形内に収まる行のみキャッシュに追加
				lineCaches.push_back({ lineGlyphs, offset.x, offset.y });
				lineGlyphs.clear();
				maxWidth = Max(maxWidth, offset.x);
				offset.x = 0;
				
				// 次の行の開始位置を計算
				offset.y = currentLineBottom + spacing.y;
				return true;
			};

		for (const auto& glyph : glyphs)
		{
			if (glyph.codePoint == U'\n')
			{
				if (!fnPushLine())
				{
					break;
				}
				continue;
			}

			const double xAdvance = glyph.xAdvance * scale + spacing.x;
			if (horizontalOverflow == HorizontalOverflow::Wrap && offset.x + xAdvance > rectSize.x)
			{
				if (!fnPushLine())
				{
					break;
				}
			}

			offset.x += xAdvance;
			lineGlyphs.push_back(glyph);
		}
		// 残っている文字がある場合のみ最後の行を追加
		if (!lineGlyphs.empty())
		{
			fnPushLine();
		}
		regionSize = { maxWidth, offset.y - spacing.y };
		return true;
	}

	void Label::draw(const Node& node) const
	{
		const auto& text = m_text.value();

		if (text.empty())
		{
			return;
		}

		const Vec2& effectScale = node.effectScale();
		const Vec2& characterSpacing = m_characterSpacing.value();
		const LRTB& padding = m_padding.value();

		// node.rect()は既にeffectScaleが適用されているので、paddingもeffectScaleを適用
		const RectF rect = node.rect().stretched(
			-padding.top * effectScale.y,
			-padding.right * effectScale.x,
			-padding.bottom * effectScale.y,
			-padding.left * effectScale.x
		);

		const bool wasUpdated = m_cache.refreshIfDirty(
			text,
			m_fontOpt,
			m_fontAssetName.value(),
			m_fontSize.value(),
			characterSpacing,
			m_horizontalOverflow.value(),
			m_verticalOverflow.value(),
			rect.size / effectScale,
			m_sizingMode.value());

		if (m_sizingMode.value() == LabelSizingMode::ShrinkToFit)
		{
			const SizeF availableSize = rect.size / effectScale;
			
			// フォントサイズはSmoothPropertyなので許容誤差を大きめに取る
			constexpr double Epsilon = 1e-2;
			const bool cacheValid = !wasUpdated &&
			                       (m_cache.sizingMode == LabelSizingMode::ShrinkToFit) &&
			                       (Abs(m_cache.availableSize.x - availableSize.x) < Epsilon) &&
			                       (Abs(m_cache.availableSize.y - availableSize.y) < Epsilon) &&
			                       (Abs(m_cache.originalFontSize - m_fontSize.value()) < Epsilon) &&
			                       (Abs(m_cache.minFontSize - m_minFontSize.value()) < Epsilon);
			
			if (!cacheValid)
			{
				m_cache.sizingMode = LabelSizingMode::ShrinkToFit;
				m_cache.availableSize = availableSize;
				m_cache.originalFontSize = m_fontSize.value();
				m_cache.minFontSize = m_minFontSize.value();
				
				double effectiveFontSize = m_fontSize.value();
				const double minFontSize = m_minFontSize.value();
				
				while (effectiveFontSize >= minFontSize)
				{
					// 現在のフォントサイズで収まるかチェック
					m_cache.prevParams.reset();
					m_cache.refreshIfDirty(
						text,
						m_fontOpt,
						m_fontAssetName.value(),
						effectiveFontSize,
						characterSpacing,
						m_horizontalOverflow.value(),
						VerticalOverflow::Overflow,  // ShrinkToFitでは常にOverflowとして計算
						availableSize,
						m_sizingMode.value());
					
					// 収まっていれば、このサイズで確定
					if (m_cache.regionSize.x <= availableSize.x && 
					    m_cache.regionSize.y <= availableSize.y)
					{
						break;
					}
					
					// 収まらない場合、フォントサイズを1下げる
					effectiveFontSize = effectiveFontSize - 1.0;
					
					// 最小フォントサイズを下回らないようにする
					if (effectiveFontSize < minFontSize)
					{
						effectiveFontSize = minFontSize;
						// 最小フォントサイズで再計算して終了
						m_cache.prevParams.reset();
						m_cache.refreshIfDirty(
							text,
							m_fontOpt,
							m_fontAssetName.value(),
							effectiveFontSize,
							characterSpacing,
							m_horizontalOverflow.value(),
							VerticalOverflow::Overflow,  // ShrinkToFitでは常にOverflowとして計算
							availableSize,
							m_sizingMode.value());
						break;
					}
				}
				
				m_cache.effectiveFontSize = effectiveFontSize;
				
				// ShrinkToFitの探索ではVerticalOverflow::Overflowで計算したため、
				// 実際のverticalOverflowプロパティ値で再計算する必要がある
				if (m_verticalOverflow.value() == VerticalOverflow::Clip)
				{
					m_cache.prevParams.reset();
					m_cache.refreshIfDirty(
						text,
						m_fontOpt,
						m_fontAssetName.value(),
						effectiveFontSize,
						characterSpacing,
						m_horizontalOverflow.value(),
						m_verticalOverflow.value(),  // 実際のプロパティ値を使用
						availableSize,
						m_sizingMode.value());
				}
			}
			else if (m_cache.effectiveFontSize != m_fontSize.value())
			{
				// キャッシュされた縮小フォントサイズで再適用（キャッシュはリセットしない）
				if (m_cache.baseFontSize != 0)
				{
					m_cache.scale = m_cache.effectiveFontSize / m_cache.baseFontSize;
				}
			}
		}
		else
		{
			// Fixedモードに切り替わった場合、スケールを元に戻す
			if (m_cache.sizingMode == LabelSizingMode::ShrinkToFit && m_cache.baseFontSize != 0)
			{
				m_cache.scale = m_fontSize.value() / m_cache.baseFontSize;
			}
			m_cache.sizingMode = LabelSizingMode::Fixed;
		}

		// ShrinkToFitモードでは、フォントサイズ探索時にrefreshIfDirtyを呼ぶため、
		// prevParams->fontSizeが縮小後のフォントサイズで保存される。
		// 次フレームで元のフォントサイズと比較されると常に変更ありと判定され、
		// 毎フレーム再計算が発生してしまう。
		// これを防ぐため、prevParams->fontSizeを元のフォントサイズに戻す。
		if (m_cache.prevParams.has_value() && m_sizingMode.value() == LabelSizingMode::ShrinkToFit)
		{
			m_cache.prevParams->fontSize = m_fontSize.value();
		}

		const double startY = [this, &rect, &effectScale]()
			{
				const VerticalAlign& verticalAlign = m_verticalAlign.value();
				switch (verticalAlign)
				{
				case VerticalAlign::Top:
					return rect.y;
				case VerticalAlign::Middle:
					return rect.y + (rect.h - m_cache.regionSize.y * effectScale.y) / 2;
				case VerticalAlign::Bottom:
					return rect.y + rect.h - m_cache.regionSize.y * effectScale.y;
				default:
					throw Error{ U"Invalid VerticalAlign: {}"_fmt(static_cast<std::underlying_type_t<VerticalAlign>>(verticalAlign)) };
				}
			}();

		const HorizontalAlign& horizontalAlign = m_horizontalAlign.value();

		{
			const ScopedCustomShader2D shader{ Font::GetPixelShader(m_cache.fontMethod) };
			for (const auto& lineCache : m_cache.lineCaches)
			{
				const double startX = [this, &rect, &effectScale, &lineCache, horizontalAlign]()
					{
						switch (horizontalAlign)
						{
						case HorizontalAlign::Left:
							return rect.x;
						case HorizontalAlign::Center:
							return rect.x + (rect.w - lineCache.width * effectScale.x) / 2;
						case HorizontalAlign::Right:
							return rect.x + rect.w - lineCache.width * effectScale.x;
						default:
							throw Error{ U"Invalid HorizontalAlign: {}"_fmt(static_cast<std::underlying_type_t<HorizontalAlign>>(horizontalAlign)) };
						}
					}();

				double x = 0;
				for (const auto& glyph : lineCache.glyphs)
				{
					if (glyph.codePoint == U'\n')
					{
						continue;
					}
					const Vec2 pos{ startX + x, startY + lineCache.offsetY * effectScale.y };
					const ColorF& color = m_color.value();
					glyph.texture.scaled(m_cache.scale * effectScale).draw(pos + glyph.getOffset(m_cache.scale) * effectScale, color);
					x += (glyph.xAdvance * m_cache.scale + characterSpacing.x) * effectScale.x;
				}
			}
		}

		if (m_underlineStyle.value() == LabelUnderlineStyle::Solid)
		{
			for (const auto& lineCache : m_cache.lineCaches)
			{
				const double startX = [this, &rect, &effectScale, &lineCache, horizontalAlign]()
					{
						switch (horizontalAlign)
						{
						case HorizontalAlign::Left:
							return rect.x;
						case HorizontalAlign::Center:
							return rect.x + (rect.w - lineCache.width * effectScale.x) / 2;
						case HorizontalAlign::Right:
							return rect.x + rect.w - lineCache.width * effectScale.x;
						default:
							throw Error{ U"Invalid HorizontalAlign: {}"_fmt(static_cast<std::underlying_type_t<HorizontalAlign>>(horizontalAlign)) };
						}
					}();

				const double thickness = m_underlineThickness.value() * effectScale.y;
				const double y = startY + (lineCache.offsetY + m_cache.lineHeight) * effectScale.y;
				Line{ startX, y, startX + lineCache.width * effectScale.x, y }.draw(thickness, m_underlineColor.value());
			}
		}
	}

	SizeF Label::contentSize() const
	{
		m_cache.refreshIfDirty(
			m_text.value(),
			m_fontOpt,
			m_fontAssetName.value(),
			m_fontSize.value(),
			m_characterSpacing.value(),
			HorizontalOverflow::Overflow, // rectSize指定なしでのサイズ計算は折り返さないようOverflowで固定
			VerticalOverflow::Overflow, // rectSize指定なしでのサイズ計算はクリップされないようOverflowで固定
			Vec2::Zero(),
			m_sizingMode.value());

		return m_cache.regionSize;
	}

	SizeF Label::contentSize(const SizeF& rectSize) const
	{
		m_cache.refreshIfDirty(
			m_text.value(),
			m_fontOpt,
			m_fontAssetName.value(),
			m_fontSize.value(),
			m_characterSpacing.value(),
			m_horizontalOverflow.value(),
			m_verticalOverflow.value(),
			rectSize,
			m_sizingMode.value());

		return m_cache.regionSize;
	}
}
