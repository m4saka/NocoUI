#include "NocoUI/Component/Label.hpp"
#include "NocoUI/Node.hpp"

namespace noco
{
	void Label::Cache::refreshIfDirty(StringView text, const Optional<Font>& fontOpt, StringView fontAssetName, double fontSize, const Vec2& spacing, HorizontalOverflow horizontalOverflow, VerticalOverflow verticalOverflow, const SizeF& rectSize)
	{
		if (prevParams.has_value() && !prevParams->isDirty(text, fontAssetName, fontSize, horizontalOverflow, verticalOverflow, spacing, rectSize))
		{
			return;
		}
		prevParams = CacheParams
		{
			.text = String{ text },
			// fontOptの変更はsetFont/clearFontで手動でキャッシュ削除して反映するため、ここには入れない
			.fontAssetName = String{ fontAssetName },
			.fontSize = fontSize,
			.horizontalOverflow = horizontalOverflow,
			.verticalOverflow = verticalOverflow,
			.spacing = spacing,
			.rectSize = rectSize,
		};

		const Font font = fontOpt.has_value() ? *fontOpt : ((!fontAssetName.empty() && FontAsset::IsRegistered(fontAssetName)) ? FontAsset(fontAssetName) : SimpleGUI::GetFont());
		fontMethod = font.method();
		const Array<Glyph> glyphs = font.getGlyphs(text);
		const int32 baseFontSize = font.fontSize();
		if (baseFontSize == 0)
		{
			scale = 1.0;
		}
		else
		{
			scale = fontSize / baseFontSize;
		}
		lineHeight = font.height(fontSize);

		lineCaches.clear();

		double maxWidth = 0.0;
		Vec2 offset = Vec2::Zero();
		Array<Glyph> lineGlyphs;

		const auto fnPushLine =
			[&]() -> bool
			{
				if (verticalOverflow == VerticalOverflow::Clip && offset.y + lineHeight > rectSize.y)
				{
					// verticalOverflowがClipの場合、矩形の高さを超えたら終了
					return false;
				}
				if (!lineGlyphs.empty())
				{
					// 行末文字の右側に余白を入れないため、その分を引く
					offset.x -= spacing.x;
				}
				lineCaches.push_back({ lineGlyphs, offset.x, offset.y });
				lineGlyphs.clear();
				maxWidth = Max(maxWidth, offset.x);
				offset.x = 0;
				offset.y += lineHeight + spacing.y;
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
		fnPushLine(); // 最後の行を追加
		regionSize = { maxWidth, offset.y - spacing.y };
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
		const double leftPadding = padding.left * effectScale.x;
		const double rightPadding = padding.right * effectScale.x;
		const double topPadding = padding.top * effectScale.y;
		const double bottomPadding = padding.bottom * effectScale.y;

		// stretchedはtop,right,bottom,leftの順
		const RectF rect = node.rect().stretched(-topPadding, -rightPadding, -bottomPadding, -leftPadding);

		m_cache.refreshIfDirty(
			text,
			m_fontOpt,
			m_fontAssetName.value(),
			m_fontSize.value(),
			characterSpacing,
			m_horizontalOverflow.value(),
			m_verticalOverflow.value(),
			rect.size / effectScale);

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
			Vec2::Zero());

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
			rectSize);

		return m_cache.regionSize;
	}
}
