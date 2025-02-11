#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"
#include "../Enums.hpp"

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

			void refreshIfDirty(StringView text, StringView fontAssetName, double fontSize, const Vec2& spacing, HorizontalOverflow horizontalOverflow, VerticalOverflow verticalOverflow, const SizeF& rectSize);
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
