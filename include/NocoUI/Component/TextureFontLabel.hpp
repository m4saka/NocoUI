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
		Property<String> m_text;
		SmoothProperty<Vec2> m_characterSize;
		Property<TextureFontLabelSizingMode> m_sizingMode;
		SmoothProperty<ColorF> m_color;
		Property<HorizontalAlign> m_horizontalAlign;
		Property<VerticalAlign> m_verticalAlign;
		SmoothProperty<Vec2> m_characterSpacing;
		SmoothProperty<LRTB> m_padding;
		Property<HorizontalOverflow> m_horizontalOverflow;
		Property<VerticalOverflow> m_verticalOverflow;
		SmoothProperty<ColorF> m_addColor;
		Property<BlendMode> m_blendMode;
		Property<bool> m_preserveAspect;
		Property<String> m_textureFilePath;
		Property<String> m_textureAssetName;
		Property<String> m_characterSet;
		Property<Vec2> m_textureCellSize;
		Property<Vec2> m_textureOffset;
		Property<int32> m_textureGridColumns;
		Property<int32> m_textureGridRows;

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
					int32 newTextureGridRows) const
				{
					return characterSet != newCharacterSet
						|| textureCellSize != newTextureCellSize
						|| textureOffset != newTextureOffset
						|| textureGridColumns != newTextureGridColumns
						|| textureGridRows != newTextureGridRows;
				}
			};

			HashTable<char32, RectF> uvMap;
			Optional<CacheParams> prevParams;

			bool refreshIfDirty(
				StringView characterSet,
				const Vec2& textureCellSize,
				const Vec2& textureOffset,
				int32 textureGridColumns,
				int32 textureGridRows)
			{
				if (prevParams.has_value() && !prevParams->isDirty(
					characterSet, textureCellSize, textureOffset,
					textureGridColumns, textureGridRows))
				{
					return false;
				}

				uvMap.clear();

				// columnsまたはrowsが0以下の場合は空のuvMapを返す（描画されない）
				if (textureGridColumns <= 0 || textureGridRows <= 0)
				{
					prevParams = CacheParams{
						.characterSet = String{ characterSet },
						.textureCellSize = textureCellSize,
						.textureOffset = textureOffset,
						.textureGridColumns = textureGridColumns,
						.textureGridRows = textureGridRows,
					};
					return true;
				}

				const int32 maxIndex = textureGridColumns * textureGridRows;
				size_t normalizedIndex = 0;

				for (char32 ch : characterSet)
				{
					if (ch == U'\n' || ch == U'\r')
					{
						continue;
					}

					// インデックスが範囲外の場合はスキップ
					if (static_cast<int32>(normalizedIndex) >= maxIndex)
					{
						break;
					}

					const double gridX = static_cast<double>(normalizedIndex % textureGridColumns);
					const double gridY = static_cast<double>(normalizedIndex / textureGridColumns);

					uvMap[ch] = RectF{
						textureOffset.x + gridX * textureCellSize.x,
						textureOffset.y + gridY * textureCellSize.y,
						textureCellSize.x,
						textureCellSize.y
					};

					++normalizedIndex;
				}

				prevParams = CacheParams{
					.characterSet = String{ characterSet },
					.textureCellSize = textureCellSize,
					.textureOffset = textureOffset,
					.textureGridColumns = textureGridColumns,
					.textureGridRows = textureGridRows,
				};

				return true;
			}

			[[nodiscard]]
			Optional<RectF> getUV(char32 character) const
			{
				if (auto it = uvMap.find(character); it != uvMap.end())
				{
					return it->second;
				}
				return none;
			}
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
				bool preserveAspect,
				const TextureFontCache& textureFontCache);
		};

		/* NonSerialized */ mutable TextureFontCache m_textureFontCache;
		/* NonSerialized */ mutable CharacterCache m_cache;

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
			const PropertyValue<VerticalOverflow>& verticalOverflow = VerticalOverflow::Overflow)
			: SerializableComponentBase{ U"TextureFontLabel", { &m_text, &m_characterSize, &m_sizingMode, &m_color, &m_horizontalAlign, &m_verticalAlign, &m_characterSpacing, &m_padding, &m_horizontalOverflow, &m_verticalOverflow, &m_addColor, &m_blendMode, &m_preserveAspect, &m_textureFilePath, &m_textureAssetName, &m_characterSet, &m_textureCellSize, &m_textureOffset, &m_textureGridColumns, &m_textureGridRows } }
			, m_text{ U"text", text }
			, m_characterSize{ U"characterSize", characterSize }
			, m_sizingMode{ U"sizingMode", sizingMode }
			, m_color{ U"color", Palette::White }
			, m_horizontalAlign{ U"horizontalAlign", horizontalAlign }
			, m_verticalAlign{ U"verticalAlign", verticalAlign }
			, m_characterSpacing{ U"characterSpacing", characterSpacing }
			, m_padding{ U"padding", padding }
			, m_horizontalOverflow{ U"horizontalOverflow", horizontalOverflow }
			, m_verticalOverflow{ U"verticalOverflow", verticalOverflow }
			, m_addColor{ U"addColor", ColorF{ 0.0, 0.0, 0.0, 0.0 } }
			, m_blendMode{ U"blendMode", BlendMode::Normal }
			, m_preserveAspect{ U"preserveAspect", true }
			, m_textureFilePath{ U"textureFilePath", textureFilePath }
			, m_textureAssetName{ U"textureAssetName", textureAssetName }
			, m_characterSet{ U"characterSet", characterSet }
			, m_textureCellSize{ U"textureCellSize", textureCellSize }
			, m_textureOffset{ U"textureOffset", textureOffset }
			, m_textureGridColumns{ U"textureGridColumns", textureGridColumns }
			, m_textureGridRows{ U"textureGridRows", textureGridRows }
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