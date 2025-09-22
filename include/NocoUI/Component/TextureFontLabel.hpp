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
		AutoResize,
	};

	class TextureFontLabel : public SerializableComponentBase, public std::enable_shared_from_this<TextureFontLabel>
	{
	private:
		Property<String> m_text;
		SmoothProperty<Vec2> m_characterSize;
		Property<TextureFontLabelSizingMode> m_sizingMode;
		SmoothProperty<Color> m_color;
		Property<HorizontalAlign> m_horizontalAlign;
		Property<VerticalAlign> m_verticalAlign;
		SmoothProperty<Vec2> m_characterSpacing;
		SmoothProperty<LRTB> m_padding;
		Property<HorizontalOverflow> m_horizontalOverflow;
		Property<VerticalOverflow> m_verticalOverflow;
		SmoothProperty<Color> m_addColor;
		Property<BlendMode> m_blendMode;
		Property<bool> m_preserveAspect;
		Property<String> m_textureFilePath;
		Property<String> m_textureAssetName;
		Property<String> m_characterSet;
		Property<Vec2> m_textureCellSize;
		Property<Vec2> m_textureOffset;
		Property<int32> m_textureGridColumns;
		Property<int32> m_textureGridRows;
		Property<SpriteTextureFilter> m_textureFilter;
		Property<SpriteTextureAddressMode> m_textureAddressMode;

		struct TextureFontCache
		{
			struct CacheParams
			{
				String characterSet;
				Vec2 textureCellSize;
				Vec2 textureOffset;
				int32 textureGridColumns;
				int32 textureGridRows;

				[[nodiscard]]
				bool isDirty(
					StringView newCharacterSet,
					const Vec2& newTextureCellSize,
					const Vec2& newTextureOffset,
					int32 newTextureGridColumns,
					int32 newTextureGridRows) const;
			};

			HashTable<char32, RectF> uvMap;
			Optional<CacheParams> prevParams;

			bool refreshIfDirty(
				StringView characterSet,
				const Vec2& textureCellSize,
				const Vec2& textureOffset,
				int32 textureGridColumns,
				int32 textureGridRows);

			[[nodiscard]]
			Optional<RectF> getUV(char32 character) const;
		};

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
			SizeF regionSize = SizeF::Zero();

			Vec2 effectiveCharacterSize = Vec2::Zero();
			double effectiveAutoShrinkWidthScale = 1.0;

			struct CacheParams
			{
				String text;
				Vec2 characterSize;
				Vec2 characterSpacing;
				TextureFontLabelSizingMode sizingMode;
				HorizontalOverflow horizontalOverflow;
				VerticalOverflow verticalOverflow;
				SizeF rectSize;
				String characterSet;
				Vec2 textureCellSize;
				Vec2 textureOffset;
				int32 textureGridColumns;
				int32 textureGridRows;

				[[nodiscard]]
				bool isDirty(
					StringView newText,
					const Vec2& newCharacterSize,
					const Vec2& newCharacterSpacing,
					TextureFontLabelSizingMode newSizingMode,
					HorizontalOverflow newHorizontalOverflow,
					VerticalOverflow newVerticalOverflow,
					const SizeF& newRectSize,
					StringView newCharacterSet,
					const Vec2& newTextureCellSize,
					const Vec2& newTextureOffset,
					int32 newTextureGridColumns,
					int32 newTextureGridRows) const;
			};

			Optional<CacheParams> prevParams;

			bool refreshIfDirty(
				StringView text,
				const Vec2& characterSize,
				const Vec2& characterSpacing,
				TextureFontLabelSizingMode newSizingMode,
				HorizontalOverflow horizontalOverflow,
				VerticalOverflow verticalOverflow,
				const SizeF& rectSize,
				const TextureFontCache& textureFontCache,
				StringView newCharacterSet,
				const Vec2& newTextureCellSize,
				const Vec2& newTextureOffset,
				int32 newTextureGridColumns,
				int32 newTextureGridRows);
		};

		/* NonSerialized */ mutable TextureFontCache m_textureFontCache;
		/* NonSerialized */ mutable CharacterCache m_cache;
		/* NonSerialized */ mutable CharacterCache m_autoResizeCache;

		SizeF getContentSizeForAutoResize() const;

	public:
		explicit TextureFontLabel(
			const PropertyValue<String>& text = U"",
			const PropertyValue<Vec2>& characterSize = Vec2{ 24, 24 },
			const PropertyValue<String>& textureFilePath = String{},
			const PropertyValue<String>& textureAssetName = String{},
			const PropertyValue<String>& characterSet = U"0123456789",
			const PropertyValue<Vec2>& textureCellSize = Vec2{ 32, 32 },
			const PropertyValue<Vec2>& textureOffset = Vec2::Zero(),
			const PropertyValue<int32>& textureGridColumns = 1,
			const PropertyValue<int32>& textureGridRows = 10,
			const PropertyValue<TextureFontLabelSizingMode>& sizingMode = TextureFontLabelSizingMode::Fixed,
			const PropertyValue<HorizontalAlign>& horizontalAlign = HorizontalAlign::Left,
			const PropertyValue<VerticalAlign>& verticalAlign = VerticalAlign::Top,
			const PropertyValue<Vec2>& characterSpacing = Vec2::Zero(),
			const PropertyValue<LRTB>& padding = LRTB::Zero(),
			const PropertyValue<HorizontalOverflow>& horizontalOverflow = HorizontalOverflow::Wrap,
			const PropertyValue<VerticalOverflow>& verticalOverflow = VerticalOverflow::Overflow);

		void update(const std::shared_ptr<Node>& node) override;

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
		const PropertyValue<Color>& color() const
		{
			return m_color.propertyValue();
		}

		std::shared_ptr<TextureFontLabel> setColor(const PropertyValue<Color>& color)
		{
			m_color.setPropertyValue(color);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<Color>& addColor() const
		{
			return m_addColor.propertyValue();
		}

		std::shared_ptr<TextureFontLabel> setAddColor(const PropertyValue<Color>& addColor)
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

		[[nodiscard]]
		const PropertyValue<SpriteTextureFilter>& textureFilter() const
		{
			return m_textureFilter.propertyValue();
		}

		std::shared_ptr<TextureFontLabel> setTextureFilter(const PropertyValue<SpriteTextureFilter>& textureFilter)
		{
			m_textureFilter.setPropertyValue(textureFilter);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<SpriteTextureAddressMode>& textureAddressMode() const
		{
			return m_textureAddressMode.propertyValue();
		}

		std::shared_ptr<TextureFontLabel> setTextureAddressMode(const PropertyValue<SpriteTextureAddressMode>& textureAddressMode)
		{
			m_textureAddressMode.setPropertyValue(textureAddressMode);
			return shared_from_this();
		}

		SizeF getContentSize() const;

		SizeF getContentSize(const SizeF& rectSize) const;
	};
}
