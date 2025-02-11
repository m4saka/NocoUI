#include "NocoUI/Component/Label.hpp"
#include "NocoUI/Node.hpp"

namespace noco
{
	void Label::Cache::refreshIfDirty(StringView text, StringView fontAssetName, double fontSize, const Vec2& spacing, HorizontalOverflow horizontalOverflow, VerticalOverflow verticalOverflow, const SizeF& rectSize)
	{
		if (prevParams.has_value() && !prevParams->isDirty(text, fontAssetName, fontSize, horizontalOverflow, verticalOverflow, spacing, rectSize))
		{
			return;
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
		};

		const Font font = FontAsset(fontAssetName);
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
		const Vec2& spacing = m_spacing.value();
		const LRTB& padding = m_padding.value();
		const double leftPadding = padding.left * effectScale.x;
		const double rightPadding = padding.right * effectScale.x;
		const double topPadding = padding.top * effectScale.y;
		const double bottomPadding = padding.bottom * effectScale.y;

		// stretchedはtop,right,bottom,leftの順
		const RectF rect = node.rect().stretched(-topPadding, -rightPadding, -bottomPadding, -leftPadding);

		m_cache.refreshIfDirty(
			text,
			m_fontAssetName.value(),
			m_fontSize.value(),
			spacing,
			m_horizontalOverflow.value(),
			m_verticalOverflow.value(),
			rect.size / effectScale);

		double posY;
		const VerticalAlign& verticalAlign = m_verticalAlign.value();
		switch (verticalAlign)
		{
		case VerticalAlign::Top:
			posY = rect.y;
			break;
		case VerticalAlign::Middle:
			posY = rect.y + (rect.h - m_cache.regionSize.y * effectScale.y) / 2;
			break;
		case VerticalAlign::Bottom:
			posY = rect.y + rect.h - m_cache.regionSize.y * effectScale.y;
			break;
		default:
			throw Error{ U"Invalid VerticalAlign: {}"_fmt(static_cast<std::underlying_type_t<VerticalAlign>>(verticalAlign)) };
		}

		const ScopedCustomShader2D shader{ Font::GetPixelShader(m_cache.fontMethod) };
		const HorizontalAlign& horizontalAlign = m_horizontalAlign.value();
		for (const auto& lineCache : m_cache.lineCaches)
		{
			double posX;
			switch (horizontalAlign)
			{
			case HorizontalAlign::Left:
				posX = rect.x;
				break;
			case HorizontalAlign::Center:
				posX = rect.x + (rect.w - lineCache.width * effectScale.x) / 2;
				break;
			case HorizontalAlign::Right:
				posX = rect.x + rect.w - lineCache.width * effectScale.x;
				break;
			default:
				throw Error{ U"Invalid HorizontalAlign: {}"_fmt(static_cast<std::underlying_type_t<HorizontalAlign>>(horizontalAlign)) };
			}

			for (const auto& glyph : lineCache.glyphs)
			{
				if (glyph.codePoint == U'\n')
				{
					continue;
				}
				const Vec2 pos{ posX, posY + lineCache.offsetY * effectScale.y };
				const ColorF& color = m_color.value();
				glyph.texture.scaled(m_cache.scale * effectScale).draw(pos + glyph.getOffset(m_cache.scale) * effectScale, color);
				posX += (glyph.xAdvance * m_cache.scale + spacing.x) * effectScale.x;
			}
		}
	}
}
