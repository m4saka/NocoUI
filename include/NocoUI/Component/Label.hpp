#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"
#include "../Enums.hpp"

namespace noco
{
	enum class LabelUnderlineStyle : uint8
	{
		None,
		Solid,
	};

	enum class LabelSizingMode : uint8
	{
		Fixed,
		ShrinkToFit,
		// 将来的にテキストに応じてノード側を自動リサイズするモードが追加されることを想定
	};

	class Label : public SerializableComponentBase
	{
	private:
		Property<String> m_text;
		Property<String> m_fontAssetName;
		SmoothProperty<double> m_fontSize;
		Property<LabelSizingMode> m_sizingMode;
		SmoothProperty<double> m_minFontSize;
		SmoothProperty<ColorF> m_color;
		Property<HorizontalAlign> m_horizontalAlign;
		Property<VerticalAlign> m_verticalAlign;
		SmoothProperty<LRTB> m_padding;
		Property<HorizontalOverflow> m_horizontalOverflow;
		Property<VerticalOverflow> m_verticalOverflow;
		SmoothProperty<Vec2> m_characterSpacing;
		Property<LabelUnderlineStyle> m_underlineStyle;
		SmoothProperty<ColorF> m_underlineColor;
		SmoothProperty<double> m_underlineThickness;

		/* NonSerialized */ Optional<Font> m_fontOpt;
		
		struct CacheParams
		{
			String text;
			String fontAssetName;
			double fontSize;
			HorizontalOverflow horizontalOverflow;
			VerticalOverflow verticalOverflow;
			Vec2 spacing;
			SizeF rectSize;
			bool hasCustomFont;
			Font customFont;
			LabelSizingMode sizingMode;

			[[nodiscard]]
			bool isDirty(
				StringView newText,
				StringView newFontAssetName,
				double newFontSize,
				HorizontalOverflow newHorizontalOverflow,
				VerticalOverflow newVerticalOverflow,
				const Vec2& newSpacing,
				const SizeF& newRectSize,
				bool newHasCustomFont,
				const Font& newCustomFont,
				LabelSizingMode newSizingMode) const
			{
				return text != newText
					|| fontAssetName != newFontAssetName
					|| fontSize != newFontSize
					|| horizontalOverflow != newHorizontalOverflow
					|| verticalOverflow != newVerticalOverflow
					|| spacing != newSpacing
					|| rectSize != newRectSize
					|| hasCustomFont != newHasCustomFont
					|| (hasCustomFont && customFont != newCustomFont)
					|| sizingMode != newSizingMode;
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
			Font currentFont;  // 現在使用中のフォント
			int32 baseFontSize = 0;  // ベースフォントサイズ
			
			// ShrinkToFit用キャッシュ
			double effectiveFontSize = 0.0;
			SizeF availableSize = SizeF::Zero();
			double originalFontSize = 0.0;
			double minFontSize = 0.0;
			LabelSizingMode sizingMode = LabelSizingMode::Fixed;

			Cache() = default;

			bool refreshIfDirty(StringView text, const Optional<Font>& fontOpt, StringView fontAssetName, double fontSize, const Vec2& spacing, HorizontalOverflow horizontalOverflow, VerticalOverflow verticalOverflow, const SizeF& rectSize, LabelSizingMode newSizingMode);
		};

		/* NonSerialized */ mutable Cache m_cache;

	public:
		explicit Label(
			const PropertyValue<String>& text = U"",
			const PropertyValue<String>& fontAssetName = U"",
			const PropertyValue<double>& fontSize = 24.0,
			const PropertyValue<ColorF>& color = Palette::White,
			const PropertyValue<HorizontalAlign>& horizontalAlign = HorizontalAlign::Left,
			const PropertyValue<VerticalAlign>& verticalAlign = VerticalAlign::Top,
			const PropertyValue<LRTB>& padding = LRTB::Zero(),
			const PropertyValue<HorizontalOverflow>& horizontalOverflow = HorizontalOverflow::Wrap,
			const PropertyValue<VerticalOverflow>& verticalOverflow = VerticalOverflow::Overflow,
			const PropertyValue<Vec2>& characterSpacing = Vec2::Zero(),
			const PropertyValue<LabelUnderlineStyle>& underlineStyle = LabelUnderlineStyle::None,
			const PropertyValue<ColorF>& underlineColor = Palette::White,
			const PropertyValue<double>& underlineThickness = 1.0,
			const PropertyValue<LabelSizingMode>& sizingMode = LabelSizingMode::Fixed,
			const PropertyValue<double>& minFontSize = 8.0)
			: SerializableComponentBase{ U"Label", { &m_text, &m_fontAssetName, &m_fontSize, &m_sizingMode, &m_minFontSize, &m_color, &m_horizontalAlign, &m_verticalAlign, &m_padding, &m_horizontalOverflow, &m_verticalOverflow, &m_characterSpacing, &m_underlineStyle, &m_underlineColor, &m_underlineThickness } }
			, m_text{ U"text", text }
			, m_fontAssetName{ U"fontAssetName", fontAssetName }
			, m_fontSize{ U"fontSize", fontSize }
			, m_sizingMode{ U"sizingMode", sizingMode }
			, m_minFontSize{ U"minFontSize", minFontSize }
			, m_color{ U"color", color }
			, m_horizontalAlign{ U"horizontalAlign", horizontalAlign }
			, m_verticalAlign{ U"verticalAlign", verticalAlign }
			, m_padding{ U"padding", padding }
			, m_horizontalOverflow{ U"horizontalOverflow", horizontalOverflow }
			, m_verticalOverflow{ U"verticalOverflow", verticalOverflow }
			, m_characterSpacing{ U"characterSpacing", characterSpacing }
			, m_underlineStyle{ U"underlineStyle", underlineStyle }
			, m_underlineColor{ U"underlineColor", underlineColor }
			, m_underlineThickness{ U"underlineThickness", underlineThickness }
		{
		}

		void update(const std::shared_ptr<Node>&) override
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
		const PropertyValue<Vec2>& characterSpacing() const
		{
			return m_characterSpacing.propertyValue();
		}

		void setCharacterSpacing(const PropertyValue<Vec2>& characterSpacing)
		{
			m_characterSpacing.setPropertyValue(characterSpacing);
		}

		[[nodiscard]]
		const PropertyValue<LabelUnderlineStyle>& underlineStyle() const
		{
			return m_underlineStyle.propertyValue();
		}

		void setUnderlineStyle(const PropertyValue<LabelUnderlineStyle>& underlineStyle)
		{
			m_underlineStyle.setPropertyValue(underlineStyle);
		}

		[[nodiscard]]
		const PropertyValue<ColorF>& underlineColor() const
		{
			return m_underlineColor.propertyValue();
		}

		void setUnderlineColor(const PropertyValue<ColorF>& underlineColor)
		{
			m_underlineColor.setPropertyValue(underlineColor);
		}

		[[nodiscard]]
		const PropertyValue<LabelSizingMode>& sizingMode() const
		{
			return m_sizingMode.propertyValue();
		}

		void setSizingMode(const PropertyValue<LabelSizingMode>& sizingMode)
		{
			m_sizingMode.setPropertyValue(sizingMode);
		}

		[[nodiscard]]
		const PropertyValue<double>& minFontSize() const
		{
			return m_minFontSize.propertyValue();
		}

		void setMinFontSize(const PropertyValue<double>& minFontSize)
		{
			m_minFontSize.setPropertyValue(minFontSize);
		}

		void setFont(const Font& font)
		{
			m_fontOpt = font;
			m_cache.prevParams.reset();
		}

		void clearFont()
		{
			m_fontOpt.reset();
			m_cache.prevParams.reset();
		}

		SizeF contentSize() const;

		SizeF contentSize(const SizeF& rectSize) const;
	};
}
