#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"
#include "IFontCachedComponent.hpp"
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
		AutoShrink,
		AutoShrinkWidth,
		AutoResize,
		AutoResizeHeight,
	};

	enum class LabelGradationType : uint8
	{
		None,
		TopBottom,
		LeftRight,
	};

	class Label : public SerializableComponentBase, public detail::IFontCachedComponent, public std::enable_shared_from_this<Label>
	{
	private:
		Property<String> m_text;
		Property<String> m_fontAssetName;
		SmoothProperty<double> m_fontSize;
		Property<LabelGradationType> m_gradationType;
		SmoothProperty<Color> m_color;
		SmoothProperty<Color> m_gradationColor1;
		SmoothProperty<Color> m_gradationColor2;
		Property<LabelSizingMode> m_sizingMode;
		SmoothProperty<double> m_minFontSize;
		Property<HorizontalAlign> m_horizontalAlign;
		Property<VerticalAlign> m_verticalAlign;
		SmoothProperty<LRTB> m_padding;
		Property<HorizontalOverflow> m_horizontalOverflow;
		Property<VerticalOverflow> m_verticalOverflow;
		SmoothProperty<Vec2> m_characterSpacing;
		Property<LabelUnderlineStyle> m_underlineStyle;
		SmoothProperty<Color> m_underlineColor;
		SmoothProperty<double> m_underlineThickness;
		SmoothProperty<Color> m_outlineColor;
		SmoothProperty<double> m_outlineFactorInner;
		SmoothProperty<double> m_outlineFactorOuter;
		SmoothProperty<Color> m_shadowColor;
		SmoothProperty<Vec2> m_shadowOffset;

		/* NonSerialized */ Optional<Font> m_fontOpt;

		struct CacheParams
		{
			String text;
			String fontAssetName;
			double fontSize;
			double minFontSize;
			HorizontalOverflow horizontalOverflow;
			VerticalOverflow verticalOverflow;
			Vec2 spacing;
			SizeF rectSize;
			bool hasCustomFont;
			Font customFont;
			LabelSizingMode sizingMode;

			[[nodiscard]]
			bool isDirty(
				const String& newText,
				const String& newFontAssetName,
				double newFontSize,
				double newMinFontSize,
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
					|| minFontSize != newMinFontSize
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
				double minTopT = 0.0;
				double maxBottomT = 1.0;
			};

			Array<LineCache> lineCaches;
			double assetFontSizeScale = 1.0;
			double lineHeight = 0.0;
			SizeF regionSize = SizeF::Zero();
			Optional<CacheParams> prevParams = std::nullopt;
			FontMethod fontMethod = FontMethod::Bitmap;
			Font currentFont;
			int32 assetFontSize = 0;

			// AutoShrink用キャッシュ
			double effectiveFontSize = 0.0;

			// AutoShrinkWidth用キャッシュ
			double effectiveAutoShrinkWidthScale = 1.0;

			Cache() = default;

			bool refreshIfDirty(const String& text, const Optional<Font>& fontOpt, const String& fontAssetName, const String& canvasDefaultFontAssetName, double fontSize, double minFontSize, const Vec2& spacing, HorizontalOverflow horizontalOverflow, VerticalOverflow verticalOverflow, const SizeF& rectSize, LabelSizingMode newSizingMode);
		};

		/* NonSerialized */ mutable Cache m_cache;
		/* NonSerialized */ mutable Cache m_autoResizeCache;

		SizeF getContentSizeForAutoResize(const String& canvasDefaultFontAssetName = U"") const;

		void clearFontCache() override
		{
			m_cache.prevParams.reset();
			m_autoResizeCache.prevParams.reset();
		}

	public:
		explicit Label(
			const PropertyValue<String>& text = U"",
			const PropertyValue<String>& fontAssetName = U"",
			const PropertyValue<double>& fontSize = 24.0,
			const PropertyValue<Color>& color = Palette::White,
			const PropertyValue<HorizontalAlign>& horizontalAlign = HorizontalAlign::Center,
			const PropertyValue<VerticalAlign>& verticalAlign = VerticalAlign::Middle,
			const PropertyValue<LRTB>& padding = LRTB::Zero(),
			const PropertyValue<HorizontalOverflow>& horizontalOverflow = HorizontalOverflow::Wrap,
			const PropertyValue<VerticalOverflow>& verticalOverflow = VerticalOverflow::Overflow,
			const PropertyValue<Vec2>& characterSpacing = Vec2::Zero(),
			const PropertyValue<LabelUnderlineStyle>& underlineStyle = LabelUnderlineStyle::None,
			const PropertyValue<Color>& underlineColor = Palette::White,
			const PropertyValue<double>& underlineThickness = 1.0,
			const PropertyValue<LabelSizingMode>& sizingMode = LabelSizingMode::Fixed)
			: SerializableComponentBase{ U"Label", { &m_text, &m_fontAssetName, &m_fontSize, &m_gradationType, &m_color, &m_gradationColor1, &m_gradationColor2, &m_sizingMode, &m_minFontSize, &m_horizontalAlign, &m_verticalAlign, &m_padding, &m_horizontalOverflow, &m_verticalOverflow, &m_characterSpacing, &m_underlineStyle, &m_underlineColor, &m_underlineThickness, &m_outlineColor, &m_outlineFactorInner, &m_outlineFactorOuter, &m_shadowColor, &m_shadowOffset } }
			, m_text{ U"text", text }
			, m_fontAssetName{ U"fontAssetName", fontAssetName }
			, m_fontSize{ U"fontSize", fontSize }
			, m_gradationType{ U"gradationType", LabelGradationType::None }
			, m_color{ U"color", color }
			, m_gradationColor1{ U"gradationColor1", color }
			, m_gradationColor2{ U"gradationColor2", color }
			, m_sizingMode{ U"sizingMode", sizingMode }
			, m_minFontSize{ U"minFontSize", 1.0 }
			, m_horizontalAlign{ U"horizontalAlign", horizontalAlign }
			, m_verticalAlign{ U"verticalAlign", verticalAlign }
			, m_padding{ U"padding", padding }
			, m_horizontalOverflow{ U"horizontalOverflow", horizontalOverflow }
			, m_verticalOverflow{ U"verticalOverflow", verticalOverflow }
			, m_characterSpacing{ U"characterSpacing", characterSpacing }
			, m_underlineStyle{ U"underlineStyle", underlineStyle }
			, m_underlineColor{ U"underlineColor", underlineColor }
			, m_underlineThickness{ U"underlineThickness", underlineThickness }
			, m_outlineColor{ U"outlineColor", Palette::Black }
			, m_outlineFactorInner{ U"outlineFactorInner", 0.0 }
			, m_outlineFactorOuter{ U"outlineFactorOuter", 0.0 }
			, m_shadowColor{ U"shadowColor", Color{ 0, 0, 0, 0 } }
			, m_shadowOffset{ U"shadowOffset", Vec2{ 1.0, 1.0 } }
		{
		}

		void update(const std::shared_ptr<Node>& node) override;

		void draw(const Node& node) const override;

		[[nodiscard]]
		const PropertyValue<String>& text() const
		{
			return m_text.propertyValue();
		}

		std::shared_ptr<Label> setText(const PropertyValue<String>& text)
		{
			m_text.setPropertyValue(text);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<String>& fontAssetName() const
		{
			return m_fontAssetName.propertyValue();
		}

		std::shared_ptr<Label> setFontAssetName(const PropertyValue<String>& fontAssetName)
		{
			m_fontAssetName.setPropertyValue(fontAssetName);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<double>& fontSize() const
		{
			return m_fontSize.propertyValue();
		}

		std::shared_ptr<Label> setFontSize(const PropertyValue<double>& fontSize)
		{
			m_fontSize.setPropertyValue(fontSize);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<LabelGradationType>& gradationType() const
		{
			return m_gradationType.propertyValue();
		}

		std::shared_ptr<Label> setGradationType(const PropertyValue<LabelGradationType>& gradationType)
		{
			m_gradationType.setPropertyValue(gradationType);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<Color>& color() const
		{
			return m_color.propertyValue();
		}

		std::shared_ptr<Label> setColor(const PropertyValue<Color>& color)
		{
			m_color.setPropertyValue(color);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<Color>& gradationColor1() const
		{
			return m_gradationColor1.propertyValue();
		}

		std::shared_ptr<Label> setGradationColor1(const PropertyValue<Color>& gradationColor1)
		{
			m_gradationColor1.setPropertyValue(gradationColor1);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<Color>& gradationColor2() const
		{
			return m_gradationColor2.propertyValue();
		}

		std::shared_ptr<Label> setGradationColor2(const PropertyValue<Color>& gradationColor2)
		{
			m_gradationColor2.setPropertyValue(gradationColor2);
			return shared_from_this();
		}

		std::shared_ptr<Label> setGradationColors(const PropertyValue<Color>& color1, const PropertyValue<Color>& color2)
		{
			m_gradationColor1.setPropertyValue(color1);
			m_gradationColor2.setPropertyValue(color2);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<LabelSizingMode>& sizingMode() const
		{
			return m_sizingMode.propertyValue();
		}

		std::shared_ptr<Label> setSizingMode(const PropertyValue<LabelSizingMode>& sizingMode)
		{
			m_sizingMode.setPropertyValue(sizingMode);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<double>& minFontSize() const
		{
			return m_minFontSize.propertyValue();
		}

		std::shared_ptr<Label> setMinFontSize(const PropertyValue<double>& minFontSize)
		{
			m_minFontSize.setPropertyValue(minFontSize);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<HorizontalAlign>& horizontalAlign() const
		{
			return m_horizontalAlign.propertyValue();
		}

		std::shared_ptr<Label> setHorizontalAlign(const PropertyValue<HorizontalAlign>& horizontalAlign)
		{
			m_horizontalAlign.setPropertyValue(horizontalAlign);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<VerticalAlign>& verticalAlign() const
		{
			return m_verticalAlign.propertyValue();
		}

		std::shared_ptr<Label> setVerticalAlign(const PropertyValue<VerticalAlign>& verticalAlign)
		{
			m_verticalAlign.setPropertyValue(verticalAlign);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<LRTB>& padding() const
		{
			return m_padding.propertyValue();
		}

		std::shared_ptr<Label> setPadding(const PropertyValue<LRTB>& padding)
		{
			m_padding.setPropertyValue(padding);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<HorizontalOverflow>& horizontalOverflow() const
		{
			return m_horizontalOverflow.propertyValue();
		}

		std::shared_ptr<Label> setHorizontalOverflow(const PropertyValue<HorizontalOverflow>& horizontalOverflow)
		{
			m_horizontalOverflow.setPropertyValue(horizontalOverflow);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<VerticalOverflow>& verticalOverflow() const
		{
			return m_verticalOverflow.propertyValue();
		}

		std::shared_ptr<Label> setVerticalOverflow(const PropertyValue<VerticalOverflow>& verticalOverflow)
		{
			m_verticalOverflow.setPropertyValue(verticalOverflow);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<Vec2>& characterSpacing() const
		{
			return m_characterSpacing.propertyValue();
		}

		std::shared_ptr<Label> setCharacterSpacing(const PropertyValue<Vec2>& characterSpacing)
		{
			m_characterSpacing.setPropertyValue(characterSpacing);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<LabelUnderlineStyle>& underlineStyle() const
		{
			return m_underlineStyle.propertyValue();
		}

		std::shared_ptr<Label> setUnderlineStyle(const PropertyValue<LabelUnderlineStyle>& underlineStyle)
		{
			m_underlineStyle.setPropertyValue(underlineStyle);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<Color>& underlineColor() const
		{
			return m_underlineColor.propertyValue();
		}

		std::shared_ptr<Label> setUnderlineColor(const PropertyValue<Color>& underlineColor)
		{
			m_underlineColor.setPropertyValue(underlineColor);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<double>& underlineThickness() const
		{
			return m_underlineThickness.propertyValue();
		}

		std::shared_ptr<Label> setUnderlineThickness(const PropertyValue<double>& underlineThickness)
		{
			m_underlineThickness.setPropertyValue(underlineThickness);
			return shared_from_this();
		}

		std::shared_ptr<Label> setFont(const Font& font)
		{
			m_fontOpt = font;
			m_cache.prevParams.reset();
			return shared_from_this();
		}

		std::shared_ptr<Label> clearFont()
		{
			m_fontOpt.reset();
			m_cache.prevParams.reset();
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<Color>& outlineColor() const
		{
			return m_outlineColor.propertyValue();
		}

		std::shared_ptr<Label> setOutlineColor(const PropertyValue<Color>& outlineColor)
		{
			m_outlineColor.setPropertyValue(outlineColor);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<double>& outlineFactorInner() const
		{
			return m_outlineFactorInner.propertyValue();
		}

		std::shared_ptr<Label> setOutlineFactorInner(const PropertyValue<double>& outlineFactorInner)
		{
			m_outlineFactorInner.setPropertyValue(outlineFactorInner);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<double>& outlineFactorOuter() const
		{
			return m_outlineFactorOuter.propertyValue();
		}

		std::shared_ptr<Label> setOutlineFactorOuter(const PropertyValue<double>& outlineFactorOuter)
		{
			m_outlineFactorOuter.setPropertyValue(outlineFactorOuter);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<Color>& shadowColor() const
		{
			return m_shadowColor.propertyValue();
		}

		std::shared_ptr<Label> setShadowColor(const PropertyValue<Color>& shadowColor)
		{
			m_shadowColor.setPropertyValue(shadowColor);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<Vec2>& shadowOffset() const
		{
			return m_shadowOffset.propertyValue();
		}

		std::shared_ptr<Label> setShadowOffset(const PropertyValue<Vec2>& shadowOffset)
		{
			m_shadowOffset.setPropertyValue(shadowOffset);
			return shared_from_this();
		}

		SizeF getContentSize(const String& canvasDefaultFontAssetName = U"") const;

		SizeF getContentSize(const SizeF& rectSize, const String& canvasDefaultFontAssetName = U"") const;
	};
}
