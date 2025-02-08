#pragma once
#include <Siv3D.hpp>

namespace noco
{
	class Label : public ComponentBase
	{
	private:
		Property<String> m_text;
		Property<String> m_fontAssetName;
		SmoothProperty<double> m_fontSize;
		SmoothProperty<ColorF> m_color;
		Property<HorizontalAlign> m_horizontalAlign;
		Property<VerticalAlign> m_verticalAlign;
		SmoothProperty<LRTB> m_padding;
		Property<HorizontalOverflow> m_horizontalOverflow;
		Property<VerticalOverflow> m_verticalOverflow;
		SmoothProperty<Vec2> m_spacing;

		struct CacheParams
		{
			String text;
			String fontAssetName;
			double fontSize;
			HorizontalOverflow horizontalOverflow;
			VerticalOverflow verticalOverflow;
			Vec2 spacing;
			SizeF rectSize;

			[[nodiscard]]
			bool isDirty(
				StringView newText,
				StringView newFontAssetName,
				double newFontSize,
				HorizontalOverflow newHorizontalOverflow,
				VerticalOverflow newVerticalOverflow,
				const Vec2& newSpacing,
				const SizeF& newRectSize) const
			{
				return text != newText
					|| fontAssetName != newFontAssetName
					|| fontSize != newFontSize
					|| horizontalOverflow != newHorizontalOverflow
					|| verticalOverflow != newVerticalOverflow
					|| spacing != newSpacing
					|| rectSize != newRectSize;
			}
		};

		struct Cache
		{
			struct LineCache
			{
				Array<Glyph> glyphs;
				double width = 0.0;
				double offsetY = 0.0;
			};

			Array<LineCache> lineCaches;
			double scale = 1.0;
			double lineHeight = 0.0;
			SizeF regionSize = SizeF::Zero();
			Optional<CacheParams> prevParams = std::nullopt;
			FontMethod fontMethod = FontMethod::Bitmap;

			Cache() = default;

			void refreshIfDirty(StringView text, StringView fontAssetName, double fontSize, const Vec2& spacing, HorizontalOverflow horizontalOverflow, VerticalOverflow verticalOverflow, const SizeF& rectSize)
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
		};

		/* NonSerialized */ mutable Cache m_cache;

	public:
		explicit Label(
			const PropertyValue<String>& text = U"",
			const PropertyValue<String>& fontAssetName = U"",
			const PropertyValue<double>& fontSize = 12.0,
			const PropertyValue<ColorF>& color = Palette::White,
			const PropertyValue<HorizontalAlign>& horizontalAlign = HorizontalAlign::Left,
			const PropertyValue<VerticalAlign>& verticalAlign = VerticalAlign::Top,
			const PropertyValue<LRTB>& padding = LRTB::Zero(),
			const PropertyValue<HorizontalOverflow>& horizontalOverflow = HorizontalOverflow::Wrap,
			const PropertyValue<VerticalOverflow>& verticalOverflow = VerticalOverflow::Overflow,
			const PropertyValue<Vec2>& spacing = Vec2::Zero())
			: ComponentBase{ U"Label", { &m_text, &m_fontAssetName, &m_fontSize, &m_color, &m_horizontalAlign, &m_verticalAlign, &m_padding, &m_horizontalOverflow, &m_verticalOverflow, &m_spacing } }
			, m_text{ U"text", text }
			, m_fontAssetName{ U"fontAssetName", fontAssetName }
			, m_fontSize{ U"fontSize", fontSize }
			, m_color{ U"color", color }
			, m_horizontalAlign{ U"horizontalAlign", horizontalAlign }
			, m_verticalAlign{ U"verticalAlign", verticalAlign }
			, m_padding{ U"padding", padding }
			, m_horizontalOverflow{ U"horizontalOverflow", horizontalOverflow }
			, m_verticalOverflow{ U"verticalOverflow", verticalOverflow }
			, m_spacing{ U"spacing", spacing }
		{
		}

		void update(CanvasUpdateContext*, const std::shared_ptr<Node>&) override
		{
		}

		void draw(const Node& node) const override;

		[[nodiscard]]
		const PropertyValue<String>& text() const
		{
			return m_text.propertyValue();
		}

		void setText(const PropertyValue<String>& text)
		{
			m_text.setPropertyValue(text);
		}

		[[nodiscard]]
		const PropertyValue<String>& fontAssetName() const
		{
			return m_fontAssetName.propertyValue();
		}

		void setFontAssetName(const PropertyValue<String>& fontAssetName)
		{
			m_fontAssetName.setPropertyValue(fontAssetName);
		}

		[[nodiscard]]
		const PropertyValue<double>& fontSize() const
		{
			return m_fontSize.propertyValue();
		}

		void setFontSize(const PropertyValue<double>& fontSize)
		{
			m_fontSize.setPropertyValue(fontSize);
		}

		[[nodiscard]]
		const PropertyValue<ColorF>& color() const
		{
			return m_color.propertyValue();
		}

		void setColor(const PropertyValue<ColorF>& color)
		{
			m_color.setPropertyValue(color);
		}

		[[nodiscard]]
		const PropertyValue<HorizontalAlign>& horizontalAlign() const
		{
			return m_horizontalAlign.propertyValue();
		}

		void setHorizontalAlign(const PropertyValue<HorizontalAlign>& horizontalAlign)
		{
			m_horizontalAlign.setPropertyValue(horizontalAlign);
		}

		[[nodiscard]]
		const PropertyValue<VerticalAlign>& verticalAlign() const
		{
			return m_verticalAlign.propertyValue();
		}

		void setVerticalAlign(const PropertyValue<VerticalAlign>& verticalAlign)
		{
			m_verticalAlign.setPropertyValue(verticalAlign);
		}

		[[nodiscard]]
		const PropertyValue<LRTB>& padding() const
		{
			return m_padding.propertyValue();
		}

		void setPadding(const PropertyValue<LRTB>& padding)
		{
			m_padding.setPropertyValue(padding);
		}

		[[nodiscard]]
		const PropertyValue<HorizontalOverflow>& horizontalOverflow() const
		{
			return m_horizontalOverflow.propertyValue();
		}

		void setHorizontalOverflow(const PropertyValue<HorizontalOverflow>& horizontalOverflow)
		{
			m_horizontalOverflow.setPropertyValue(horizontalOverflow);
		}

		[[nodiscard]]
		const PropertyValue<VerticalOverflow>& verticalOverflow() const
		{
			return m_verticalOverflow.propertyValue();
		}

		void setVerticalOverflow(const PropertyValue<VerticalOverflow>& verticalOverflow)
		{
			m_verticalOverflow.setPropertyValue(verticalOverflow);
		}

		[[nodiscard]]
		const PropertyValue<Vec2>& spacing() const
		{
			return m_spacing.propertyValue();
		}

		void setSpacing(const PropertyValue<Vec2>& spacing)
		{
			m_spacing.setPropertyValue(spacing);
		}
	};
}
