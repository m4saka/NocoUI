#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"
#include "../LRTB.hpp"
#include "../Enums.hpp"

namespace noco
{
	enum class TextureFontLabelSizingMode : uint8
	{
		Fixed,
		AutoShrink,
		AutoShrinkWidth,
	};

	class TextureFontLabel : public SerializableComponentBase, public std::enable_shared_from_this<TextureFontLabel>
	{
	private:
		Property<String> m_textureFilePath;
		Property<String> m_textureAssetName;
		Property<String> m_characterSet;
		Property<Vec2> m_textureCellSize;
		Property<Vec2> m_textureOffset;
		Property<int32> m_textureGridColumns;
		Property<int32> m_textureGridRows;
		Property<String> m_text;
		Property<TextureFontLabelSizingMode> m_sizingMode;
		SmoothProperty<Vec2> m_characterSize;
		SmoothProperty<Vec2> m_characterSpacing;
		Property<HorizontalAlign> m_horizontalAlign;
		Property<VerticalAlign> m_verticalAlign;
		SmoothProperty<LRTB> m_padding;
		Property<HorizontalOverflow> m_horizontalOverflow;
		Property<VerticalOverflow> m_verticalOverflow;
		Property<bool> m_preserveAspect;
		SmoothProperty<ColorF> m_color;
		SmoothProperty<ColorF> m_addColor;
		Property<BlendMode> m_blendMode;

		struct CharacterCache
		{
			struct CharInfo
			{
				char32 character;
				RectF sourceRect;
				Vec2 position;
			};

			struct LineCache
			{
				Array<CharInfo> characters;
				double width = 0.0;
				double offsetY = 0.0;
			};

			Array<LineCache> lineCaches;
			SizeF contentSize = SizeF::Zero();
			
			// AutoShrink用キャッシュ
			Vec2 effectiveCharacterSize = Vec2::Zero();
			SizeF availableSize = SizeF::Zero();
			Vec2 originalCharacterSize = Vec2::Zero();
			TextureFontLabelSizingMode sizingMode = TextureFontLabelSizingMode::Fixed;

			struct CacheParams
			{
				String text;
				String characterSet;
				Vec2 characterSize;
				Vec2 characterSpacing;
				TextureFontLabelSizingMode sizingMode;
				HorizontalOverflow horizontalOverflow;
				VerticalOverflow verticalOverflow;
				SizeF rectSize;
				Vec2 textureCellSize;
				Vec2 textureOffset;
				int32 textureGridColumns;
				int32 textureGridRows;
				bool preserveAspect;

				[[nodiscard]]
				bool isDirty(
					StringView newText,
					StringView newCharacterSet,
					const Vec2& newCharacterSize,
					const Vec2& newCharacterSpacing,
					TextureFontLabelSizingMode newSizingMode,
					HorizontalOverflow newHorizontalOverflow,
					VerticalOverflow newVerticalOverflow,
					const SizeF& newRectSize,
					const Vec2& newTextureCellSize,
					const Vec2& newTextureOffset,
					int32 newTextureGridColumns,
					int32 newTextureGridRows,
					bool newPreserveAspect) const
				{
					return text != newText
						|| characterSet != newCharacterSet
						|| characterSize != newCharacterSize
						|| characterSpacing != newCharacterSpacing
						|| sizingMode != newSizingMode
						|| horizontalOverflow != newHorizontalOverflow
						|| verticalOverflow != newVerticalOverflow
						|| rectSize != newRectSize
						|| textureCellSize != newTextureCellSize
						|| textureOffset != newTextureOffset
						|| textureGridColumns != newTextureGridColumns
						|| textureGridRows != newTextureGridRows
						|| preserveAspect != newPreserveAspect;
				}
			};

			Optional<CacheParams> prevParams;

			bool refreshIfDirty(
				StringView text,
				StringView characterSet,
				const Vec2& characterSize,
				const Vec2& characterSpacing,
				TextureFontLabelSizingMode sizingMode,
				HorizontalOverflow horizontalOverflow,
				VerticalOverflow verticalOverflow,
				const SizeF& rectSize,
				const Vec2& textureCellSize,
				const Vec2& textureOffset,
				int32 textureGridColumns,
				int32 textureGridRows,
				bool preserveAspect);
		};

		/* NonSerialized */ mutable CharacterCache m_cache;

		[[nodiscard]]
		Optional<RectF> getCharacterUV(char32 character) const;

	public:
		explicit TextureFontLabel(
			const PropertyValue<String>& textureFilePath = String{},
			const PropertyValue<String>& textureAssetName = String{},
			const PropertyValue<String>& characterSet = U"0123456789",
			const PropertyValue<Vec2>& textureCellSize = Vec2{ 32, 32 },
			const PropertyValue<Vec2>& textureOffset = Vec2::Zero(),
			const PropertyValue<int32>& textureGridColumns = 1,
			const PropertyValue<int32>& textureGridRows = 10,
			const PropertyValue<String>& text = U"",
			const PropertyValue<TextureFontLabelSizingMode>& sizingMode = TextureFontLabelSizingMode::Fixed,
			const PropertyValue<Vec2>& characterSize = Vec2{ 24, 24 },
			const PropertyValue<Vec2>& characterSpacing = Vec2::Zero(),
			const PropertyValue<HorizontalAlign>& horizontalAlign = HorizontalAlign::Left,
			const PropertyValue<VerticalAlign>& verticalAlign = VerticalAlign::Top,
			const PropertyValue<LRTB>& padding = LRTB::Zero(),
			const PropertyValue<HorizontalOverflow>& horizontalOverflow = HorizontalOverflow::Wrap,
			const PropertyValue<VerticalOverflow>& verticalOverflow = VerticalOverflow::Overflow,
			const PropertyValue<bool>& preserveAspect = true,
			const PropertyValue<ColorF>& color = Palette::White,
			const PropertyValue<ColorF>& addColor = ColorF{ 0.0, 0.0, 0.0, 0.0 },
			const PropertyValue<BlendMode>& blendMode = BlendMode::Normal)
			: SerializableComponentBase{ U"TextureFontLabel", { &m_textureFilePath, &m_textureAssetName, &m_characterSet, &m_textureCellSize, &m_textureOffset, &m_textureGridColumns, &m_textureGridRows, &m_text, &m_sizingMode, &m_characterSize, &m_characterSpacing, &m_horizontalAlign, &m_verticalAlign, &m_padding, &m_horizontalOverflow, &m_verticalOverflow, &m_preserveAspect, &m_color, &m_addColor, &m_blendMode } }
			, m_textureFilePath{ U"textureFilePath", textureFilePath }
			, m_textureAssetName{ U"textureAssetName", textureAssetName }
			, m_characterSet{ U"characterSet", characterSet }
			, m_textureCellSize{ U"textureCellSize", textureCellSize }
			, m_textureOffset{ U"textureOffset", textureOffset }
			, m_textureGridColumns{ U"textureGridColumns", textureGridColumns }
			, m_textureGridRows{ U"textureGridRows", textureGridRows }
			, m_text{ U"text", text }
			, m_sizingMode{ U"sizingMode", sizingMode }
			, m_characterSize{ U"characterSize", characterSize }
			, m_characterSpacing{ U"characterSpacing", characterSpacing }
			, m_horizontalAlign{ U"horizontalAlign", horizontalAlign }
			, m_verticalAlign{ U"verticalAlign", verticalAlign }
			, m_padding{ U"padding", padding }
			, m_horizontalOverflow{ U"horizontalOverflow", horizontalOverflow }
			, m_verticalOverflow{ U"verticalOverflow", verticalOverflow }
			, m_preserveAspect{ U"preserveAspect", preserveAspect }
			, m_color{ U"color", color }
			, m_addColor{ U"addColor", addColor }
			, m_blendMode{ U"blendMode", blendMode }
		{
		}

		void draw(const Node& node) const override;

		[[nodiscard]]
		const PropertyValue<String>& textureFilePath() const
		{
			return m_textureFilePath.propertyValue();
		}

		std::shared_ptr<TextureFontLabel> setTextureFilePath(const PropertyValue<String>& textureFilePath)
		{
			m_textureFilePath.setPropertyValue(textureFilePath);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<String>& textureAssetName() const
		{
			return m_textureAssetName.propertyValue();
		}

		std::shared_ptr<TextureFontLabel> setTextureAssetName(const PropertyValue<String>& textureAssetName)
		{
			m_textureAssetName.setPropertyValue(textureAssetName);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<String>& characterSet() const
		{
			return m_characterSet.propertyValue();
		}

		std::shared_ptr<TextureFontLabel> setCharacterSet(const PropertyValue<String>& characterSet)
		{
			m_characterSet.setPropertyValue(characterSet);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<Vec2>& textureCellSize() const
		{
			return m_textureCellSize.propertyValue();
		}

		std::shared_ptr<TextureFontLabel> setTextureCellSize(const PropertyValue<Vec2>& textureCellSize)
		{
			m_textureCellSize.setPropertyValue(textureCellSize);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<Vec2>& textureOffset() const
		{
			return m_textureOffset.propertyValue();
		}

		std::shared_ptr<TextureFontLabel> setTextureOffset(const PropertyValue<Vec2>& textureOffset)
		{
			m_textureOffset.setPropertyValue(textureOffset);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<int32>& textureGridColumns() const
		{
			return m_textureGridColumns.propertyValue();
		}

		std::shared_ptr<TextureFontLabel> setTextureGridColumns(const PropertyValue<int32>& textureGridColumns)
		{
			m_textureGridColumns.setPropertyValue(textureGridColumns);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<int32>& textureGridRows() const
		{
			return m_textureGridRows.propertyValue();
		}

		std::shared_ptr<TextureFontLabel> setTextureGridRows(const PropertyValue<int32>& textureGridRows)
		{
			m_textureGridRows.setPropertyValue(textureGridRows);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<String>& text() const
		{
			return m_text.propertyValue();
		}

		std::shared_ptr<TextureFontLabel> setText(const PropertyValue<String>& text)
		{
			m_text.setPropertyValue(text);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<TextureFontLabelSizingMode>& sizingMode() const
		{
			return m_sizingMode.propertyValue();
		}

		std::shared_ptr<TextureFontLabel> setSizingMode(const PropertyValue<TextureFontLabelSizingMode>& sizingMode)
		{
			m_sizingMode.setPropertyValue(sizingMode);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<Vec2>& characterSize() const
		{
			return m_characterSize.propertyValue();
		}

		std::shared_ptr<TextureFontLabel> setCharacterSize(const PropertyValue<Vec2>& characterSize)
		{
			m_characterSize.setPropertyValue(characterSize);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<Vec2>& characterSpacing() const
		{
			return m_characterSpacing.propertyValue();
		}

		std::shared_ptr<TextureFontLabel> setCharacterSpacing(const PropertyValue<Vec2>& characterSpacing)
		{
			m_characterSpacing.setPropertyValue(characterSpacing);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<HorizontalAlign>& horizontalAlign() const
		{
			return m_horizontalAlign.propertyValue();
		}

		std::shared_ptr<TextureFontLabel> setHorizontalAlign(const PropertyValue<HorizontalAlign>& horizontalAlign)
		{
			m_horizontalAlign.setPropertyValue(horizontalAlign);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<VerticalAlign>& verticalAlign() const
		{
			return m_verticalAlign.propertyValue();
		}

		std::shared_ptr<TextureFontLabel> setVerticalAlign(const PropertyValue<VerticalAlign>& verticalAlign)
		{
			m_verticalAlign.setPropertyValue(verticalAlign);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<LRTB>& padding() const
		{
			return m_padding.propertyValue();
		}

		std::shared_ptr<TextureFontLabel> setPadding(const PropertyValue<LRTB>& padding)
		{
			m_padding.setPropertyValue(padding);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<HorizontalOverflow>& horizontalOverflow() const
		{
			return m_horizontalOverflow.propertyValue();
		}

		std::shared_ptr<TextureFontLabel> setHorizontalOverflow(const PropertyValue<HorizontalOverflow>& horizontalOverflow)
		{
			m_horizontalOverflow.setPropertyValue(horizontalOverflow);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<VerticalOverflow>& verticalOverflow() const
		{
			return m_verticalOverflow.propertyValue();
		}

		std::shared_ptr<TextureFontLabel> setVerticalOverflow(const PropertyValue<VerticalOverflow>& verticalOverflow)
		{
			m_verticalOverflow.setPropertyValue(verticalOverflow);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<bool>& preserveAspect() const
		{
			return m_preserveAspect.propertyValue();
		}

		std::shared_ptr<TextureFontLabel> setPreserveAspect(const PropertyValue<bool>& preserveAspect)
		{
			m_preserveAspect.setPropertyValue(preserveAspect);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<ColorF>& color() const
		{
			return m_color.propertyValue();
		}

		std::shared_ptr<TextureFontLabel> setColor(const PropertyValue<ColorF>& color)
		{
			m_color.setPropertyValue(color);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<ColorF>& addColor() const
		{
			return m_addColor.propertyValue();
		}

		std::shared_ptr<TextureFontLabel> setAddColor(const PropertyValue<ColorF>& addColor)
		{
			m_addColor.setPropertyValue(addColor);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<BlendMode>& blendMode() const
		{
			return m_blendMode.propertyValue();
		}

		std::shared_ptr<TextureFontLabel> setBlendMode(const PropertyValue<BlendMode>& blendMode)
		{
			m_blendMode.setPropertyValue(blendMode);
			return shared_from_this();
		}
	};
}