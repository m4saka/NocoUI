#include "NocoUI/Component/TextureFontLabel.hpp"
#include "NocoUI/Node.hpp"
#include "NocoUI/Canvas.hpp"
#include "NocoUI/Asset.hpp"

namespace noco
{
	namespace
	{
		constexpr double ShrinkScaleFactor = 0.95;

		Texture GetTexture(const String& textureFilePath, const String& textureAssetName)
		{
			if (detail::IsEditorMode())
			{
				if (!textureFilePath.empty())
				{
					return noco::Asset::GetOrLoadTexture(textureFilePath);
				}
			}
			else
			{
				if (!textureAssetName.empty())
				{
					return TextureAsset(textureAssetName);
				}
				if (!textureFilePath.empty())
				{
					return noco::Asset::GetOrLoadTexture(textureFilePath);
				}
			}
			return Texture{};
		}
	}

	bool TextureFontLabel::TextureFontCache::CacheParams::isDirty(
		const String& newCharacterSet, const Vec2& newTextureCellSize, const Vec2& newTextureOffset, int32 newTextureGridColumns, int32 newTextureGridRows, const LRTB& newTextureCellTrim, const String& newCharTextureCellTrimJSON) const
	{
		return characterSet != newCharacterSet
			|| textureCellSize != newTextureCellSize
			|| textureOffset != newTextureOffset
			|| textureGridColumns != newTextureGridColumns
			|| textureGridRows != newTextureGridRows
			|| textureCellTrim != newTextureCellTrim
			|| textureCellTrimByCharacterJSON != newCharTextureCellTrimJSON;
	}

	bool TextureFontLabel::TextureFontCache::refreshIfDirty(
		const String& characterSet, const Vec2& textureCellSize, const Vec2& textureOffset, int32 textureGridColumns, int32 textureGridRows, const LRTB& textureCellTrim, const String& textureCellTrimByCharacterJSON)
	{
		if (prevParams.has_value() && !prevParams->isDirty(
			characterSet, textureCellSize, textureOffset,
			textureGridColumns, textureGridRows, textureCellTrim, textureCellTrimByCharacterJSON))
		{
			return false;
		}

		uvMap.clear();
		metricsMap.clear();

		if (textureGridColumns <= 0 || textureGridRows <= 0)
		{
			prevParams = CacheParams{
				.characterSet = String{ characterSet },
				.textureCellSize = textureCellSize,
				.textureOffset = textureOffset,
				.textureGridColumns = textureGridColumns,
				.textureGridRows = textureGridRows,
				.textureCellTrim = textureCellTrim,
				.textureCellTrimByCharacterJSON = String{ textureCellTrimByCharacterJSON },
			};
			return true;
		}

		// JSON���� Parse
		HashTable<char32, LRTB> charTrimMap;
		if (!textureCellTrimByCharacterJSON.isEmpty())
		{
			try
			{
				const JSON json = JSON::Parse(textureCellTrimByCharacterJSON);
				if (json.isObject())
				{
					// JSONオブジェクトの各キーをイテレート
					for (const auto& ch : characterSet)
					{
						const String key{ ch };
						if (json.hasElement(key))
						{
							const auto& value = json[key];
							if (value.isArray() && value.size() == 4)
							{
								charTrimMap[ch] = LRTB{
									value[0].get<double>(),
									value[1].get<double>(),
									value[2].get<double>(),
									value[3].get<double>(),
								};
							}
						}
					}
				}
			}
			catch (...)
			{
				// JSON parsing failed, ignore
			}
		}

		const int32 maxIndex = textureGridColumns * textureGridRows;
		size_t normalizedIndex = 0;

		for (char32 ch : characterSet)
		{
			if (ch == U'\n' || ch == U'\r')
			{
				continue;
			}

			if (static_cast<int32>(normalizedIndex) >= maxIndex)
			{
				break;
			}

			const double gridX = static_cast<double>(normalizedIndex % textureGridColumns);
			const double gridY = static_cast<double>(normalizedIndex / textureGridColumns);

			// このキャラクタのトリム量を決定(JSON指定があればそれを、なければ共通値を使用)
			const LRTB trim = charTrimMap.contains(ch) ? charTrimMap[ch] : textureCellTrim;

			// UV座標とサイズを計算(トリムを考慮)
			const RectF baseRect{
				textureOffset.x + gridX * textureCellSize.x,
				textureOffset.y + gridY * textureCellSize.y,
				textureCellSize.x,
				textureCellSize.y
			};

			uvMap[ch] = RectF{
				baseRect.x + trim.left,
				baseRect.y + trim.top,
				baseRect.w - trim.left - trim.right,
				baseRect.h - trim.top - trim.bottom
			};

			metricsMap[ch] = CharacterMetrics{
				.trim = trim,
				.effectiveSize = Vec2{
					textureCellSize.x - trim.left - trim.right,
					textureCellSize.y - trim.top - trim.bottom
				},
			};

			++normalizedIndex;
		}

		prevParams = CacheParams{
			.characterSet = String{ characterSet },
			.textureCellSize = textureCellSize,
			.textureOffset = textureOffset,
			.textureGridColumns = textureGridColumns,
			.textureGridRows = textureGridRows,
			.textureCellTrim = textureCellTrim,
			.textureCellTrimByCharacterJSON = String{ textureCellTrimByCharacterJSON },
		};

		return true;
	}

	Optional<RectF> TextureFontLabel::TextureFontCache::getUV(char32 character) const
	{
		if (auto it = uvMap.find(character); it != uvMap.end())
		{
			return it->second;
		}
		return none;
	}

	bool TextureFontLabel::CharacterCache::CacheParams::isDirty(
		const String& newText,
		const Vec2& newCharacterSize,
		const Vec2& newCharacterSpacing,
		TextureFontLabelSizingMode newSizingMode,
		HorizontalOverflow newHorizontalOverflow,
		VerticalOverflow newVerticalOverflow,
		const SizeF& newRectSize,
		const String& newCharacterSet,
		const Vec2& newTextureCellSize,
		const Vec2& newTextureOffset,
		int32 newTextureGridColumns,
		int32 newTextureGridRows,
		const LRTB& newTextureCellTrim,
		const String& newCharTextureCellTrimJSON) const
	{
		return text != newText
			|| characterSize != newCharacterSize
			|| characterSpacing != newCharacterSpacing
			|| sizingMode != newSizingMode
			|| horizontalOverflow != newHorizontalOverflow
			|| verticalOverflow != newVerticalOverflow
			|| rectSize != newRectSize
			|| characterSet != newCharacterSet
			|| textureCellSize != newTextureCellSize
			|| textureOffset != newTextureOffset
			|| textureGridColumns != newTextureGridColumns
			|| textureGridRows != newTextureGridRows
			|| textureCellTrim != newTextureCellTrim
			|| textureCellTrimByCharacterJSON != newCharTextureCellTrimJSON;
	}

	bool TextureFontLabel::CharacterCache::refreshIfDirty(
		const String& text,
		const Vec2& characterSize,
		const Vec2& characterSpacing,
		TextureFontLabelSizingMode newSizingMode,
		HorizontalOverflow horizontalOverflow,
		VerticalOverflow verticalOverflow,
		const SizeF& rectSize,
		const TextureFontCache& textureFontCache,
		const String& newCharacterSet,
		const Vec2& newTextureCellSize,
		const Vec2& newTextureOffset,
		int32 newTextureGridColumns,
		int32 newTextureGridRows,
		const LRTB& newTextureCellTrim,
		const String& newCharTextureCellTrimJSON)
	{
		if (prevParams.has_value() &&
			!prevParams->isDirty(text, characterSize, characterSpacing, newSizingMode, horizontalOverflow, verticalOverflow, rectSize, newCharacterSet, newTextureCellSize, newTextureOffset, newTextureGridColumns, newTextureGridRows, newTextureCellTrim, newCharTextureCellTrimJSON))
		{
			return false;
		}

		prevParams = CacheParams
		{
			.text = text,
			.characterSize = characterSize,
			.characterSpacing = characterSpacing,
			.sizingMode = newSizingMode,
			.horizontalOverflow = horizontalOverflow,
			.verticalOverflow = verticalOverflow,
			.rectSize = rectSize,
			.characterSet = newCharacterSet,
			.textureCellSize = newTextureCellSize,
			.textureOffset = newTextureOffset,
			.textureGridColumns = newTextureGridColumns,
			.textureGridRows = newTextureGridRows,
			.textureCellTrim = newTextureCellTrim,
			.textureCellTrimByCharacterJSON = newCharTextureCellTrimJSON,
		};

		auto refreshCacheAndGetRegionSize = [&](const Vec2& targetCharacterSize, const Vec2& targetCharacterSpacing, HorizontalOverflow hov, VerticalOverflow vov) -> SizeF
			{
				lineCaches.clear();

				double maxWidth = 0.0;
				Vec2 offset = Vec2::Zero();
				Array<CharInfo> lineChars;
				const double lineHeight = targetCharacterSize.y;

				const auto fnPushLine = [&]() -> bool
					{
						const double currentLineBottom = offset.y + lineHeight;

						if (vov == VerticalOverflow::Clip && currentLineBottom > rectSize.y)
						{
							return false;
						}

						if (!lineChars.empty())
						{
							offset.x -= targetCharacterSpacing.x;
						}

						lineCaches.push_back({ lineChars, offset.x, offset.y });
						lineChars.clear();
						maxWidth = Max(maxWidth, offset.x);

						offset.x = 0;
						offset.y = currentLineBottom + targetCharacterSpacing.y;

						return true;
					};

				for (const char32 ch : text)
				{
					if (ch == U'\n')
					{
						if (!fnPushLine())
						{
							break;
						}
						continue;
					}

					// 個別のトリミング指定があれば適用、なければ共通の幅を使用
					double charWidth = targetCharacterSize.x + targetCharacterSpacing.x;
					if (auto it = textureFontCache.metricsMap.find(ch); it != textureFontCache.metricsMap.end())
					{
						const double widthRatio = it->second.effectiveSize.x / newTextureCellSize.x;
						charWidth = targetCharacterSize.x * widthRatio + targetCharacterSpacing.x;
					}

					if (hov == HorizontalOverflow::Wrap && offset.x + charWidth > rectSize.x)
					{
						if (!fnPushLine())
						{
							break;
						}
					}

					if (auto uvRect = textureFontCache.getUV(ch))
					{
						lineChars.push_back(
							CharInfo
							{
								ch,
								*uvRect,
								Vec2{ offset.x, 0 },
							});
					}
					offset.x += charWidth;
				}
				fnPushLine();
				return { maxWidth, offset.y - targetCharacterSpacing.y };
			};

		if (newSizingMode == TextureFontLabelSizingMode::AutoShrink)
		{
			Vec2 currentCharacterSize = characterSize;
			const double aspectRatio = characterSize.y > 0 ? characterSize.x / characterSize.y : 1.0;
			const double minSize = 1.0;

			while (currentCharacterSize.x >= minSize && currentCharacterSize.y >= minSize)
			{
				const Vec2 currentCharacterSpacing
				{
					characterSize.x > 0.0 ? characterSpacing.x * currentCharacterSize.x / characterSize.x : characterSpacing.x,
					characterSize.y > 0.0 ? characterSpacing.y * currentCharacterSize.y / characterSize.y : characterSpacing.y
				};

				const SizeF requiredSize = refreshCacheAndGetRegionSize(currentCharacterSize, currentCharacterSpacing, horizontalOverflow, VerticalOverflow::Overflow);

				if (requiredSize.x <= rectSize.x && requiredSize.y <= rectSize.y)
				{
					break;
				}

				currentCharacterSize *= ShrinkScaleFactor;

				if (currentCharacterSize.x < minSize || currentCharacterSize.y < minSize)
				{
					currentCharacterSize.x = Max(currentCharacterSize.x, minSize);
					currentCharacterSize.y = Max(currentCharacterSize.y, minSize);
					if (aspectRatio > 0)
					{
						if (currentCharacterSize.x / currentCharacterSize.y > aspectRatio)
						{
							currentCharacterSize.x = currentCharacterSize.y * aspectRatio;
						}
						else
						{
							currentCharacterSize.y = currentCharacterSize.x / aspectRatio;
						}
					}
					break;
				}
			}
			this->effectiveCharacterSize = currentCharacterSize;
			this->effectiveAutoShrinkWidthScale = 1.0;

			const Vec2 finalCharacterSpacing
			{
				characterSize.x > 0.0 ? characterSpacing.x * this->effectiveCharacterSize.x / characterSize.x : characterSpacing.x,
				characterSize.y > 0.0 ? characterSpacing.y * this->effectiveCharacterSize.y / characterSize.y : characterSpacing.y
			};
			this->regionSize = refreshCacheAndGetRegionSize(this->effectiveCharacterSize, finalCharacterSpacing, horizontalOverflow, verticalOverflow);
		}
		else if (newSizingMode == TextureFontLabelSizingMode::AutoShrinkWidth)
		{
			this->effectiveCharacterSize = characterSize;

			// AutoShrinkWidthでは折り返さないため常にHorizontalOverflow::Overflowとする
			this->regionSize = refreshCacheAndGetRegionSize(characterSize, characterSpacing, HorizontalOverflow::Overflow, verticalOverflow);

			if (this->regionSize.x > rectSize.x && this->regionSize.x > 0)
			{
				this->effectiveAutoShrinkWidthScale = rectSize.x / this->regionSize.x;
			}
			else
			{
				this->effectiveAutoShrinkWidthScale = 1.0;
			}
		}
		else if (newSizingMode == TextureFontLabelSizingMode::AutoResize)
		{
			// AutoResizeではノードサイズの誤差による折り返しやクリップが発生しないよう、両方Overflowとする
			this->effectiveCharacterSize = characterSize;
			this->effectiveAutoShrinkWidthScale = 1.0;
			this->regionSize = refreshCacheAndGetRegionSize(characterSize, characterSpacing, HorizontalOverflow::Overflow, VerticalOverflow::Overflow);
		}
		else if (newSizingMode == TextureFontLabelSizingMode::AutoResizeHeight)
		{
			// AutoResizeHeightでは幅は固定し、HorizontalOverflowの設定に従って折り返し、高さのみOverflowとする
			this->effectiveCharacterSize = characterSize;
			this->effectiveAutoShrinkWidthScale = 1.0;
			this->regionSize = refreshCacheAndGetRegionSize(characterSize, characterSpacing, horizontalOverflow, VerticalOverflow::Overflow);
		}
		else // TextureFontLabelSizingMode::Fixed
		{
			this->effectiveCharacterSize = characterSize;
			this->effectiveAutoShrinkWidthScale = 1.0;
			this->regionSize = refreshCacheAndGetRegionSize(characterSize, characterSpacing, horizontalOverflow, verticalOverflow);
		}

		return true;
	}

	TextureFontLabel::TextureFontLabel(
		const PropertyValue<String>& text,
		const PropertyValue<Vec2>& characterSize,
		const PropertyValue<String>& textureFilePath,
		const PropertyValue<String>& textureAssetName,
		const PropertyValue<String>& characterSet,
		const PropertyValue<Vec2>& textureCellSize,
		const PropertyValue<Vec2>& textureOffset,
		const PropertyValue<int32>& textureGridColumns,
		const PropertyValue<int32>& textureGridRows,
		const PropertyValue<TextureFontLabelSizingMode>& sizingMode,
		const PropertyValue<HorizontalAlign>& horizontalAlign,
		const PropertyValue<VerticalAlign>& verticalAlign,
		const PropertyValue<Vec2>& characterSpacing,
		const PropertyValue<LRTB>& padding,
		const PropertyValue<HorizontalOverflow>& horizontalOverflow,
		const PropertyValue<VerticalOverflow>& verticalOverflow,
		const PropertyValue<LRTB>& textureCellTrim)
		: SerializableComponentBase{
			U"TextureFontLabel",
			{
				&m_text,
				&m_characterSize,
				&m_sizingMode,
				&m_color,
				&m_horizontalAlign,
				&m_verticalAlign,
				&m_characterSpacing,
				&m_padding,
				&m_horizontalOverflow,
				&m_verticalOverflow,
				&m_addColor,
				&m_blendMode,
				&m_preserveAspect,
				&m_textureFilePath,
				&m_textureAssetName,
				&m_characterSet,
				&m_textureCellSize,
				&m_textureOffset,
				&m_textureGridColumns,
				&m_textureGridRows,
				&m_textureFilter,
				&m_textureAddressMode,
				&m_textureCellTrim,
				&m_textureCellTrimByCharacterJSON,
			} }
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
		, m_addColor{ U"addColor", Color{ 0, 0, 0, 0 } }
		, m_blendMode{ U"blendMode", BlendMode::Normal }
		, m_preserveAspect{ U"preserveAspect", true }
		, m_textureFilePath{ U"textureFilePath", textureFilePath }
		, m_textureAssetName{ U"textureAssetName", textureAssetName }
		, m_characterSet{ U"characterSet", characterSet }
		, m_textureCellSize{ U"textureCellSize", textureCellSize }
		, m_textureOffset{ U"textureOffset", textureOffset }
		, m_textureGridColumns{ U"textureGridColumns", textureGridColumns }
		, m_textureGridRows{ U"textureGridRows", textureGridRows }
		, m_textureFilter{ U"textureFilter", SpriteTextureFilter::Default }
		, m_textureAddressMode{ U"textureAddressMode", SpriteTextureAddressMode::Default }
		, m_textureCellTrim{ U"textureCellTrim", textureCellTrim }
		, m_textureCellTrimByCharacterJSON{ U"textureCellTrimByCharacterJSON", U"{}" }
	{
	}

	std::shared_ptr<TextureFontLabel> TextureFontLabel::setTextureCellTrimByCharacter(const HashTable<char32, LRTB>& trimMap)
	{
		// Convert HashTable to JSON format
		JSON json;
		for (const auto& [ch, trim] : trimMap)
		{
			const String key{ ch };
			json[key] = Array<double>{ trim.left, trim.right, trim.top, trim.bottom };
		}

		m_textureCellTrimByCharacterJSON.setValue(json.formatMinimum());
		return shared_from_this();
	}

	void TextureFontLabel::draw(const Node& node) const
	{
		const String& text = m_text.value();
		if (text.empty())
		{
			return;
		}

		const Texture texture = GetTexture(m_textureFilePath.value(), m_textureAssetName.value());
		if (!texture)
		{
			return;
		}

		const LRTB& padding = m_padding.value();
		const RectF rect = node.regionRect().stretched(-padding.top, -padding.right, -padding.bottom, -padding.left);

		m_textureFontCache.refreshIfDirty(
			m_characterSet.value(),
			m_textureCellSize.value(),
			m_textureOffset.value(),
			m_textureGridColumns.value(),
			m_textureGridRows.value(),
			m_textureCellTrim.value(),
			m_textureCellTrimByCharacterJSON.value());

		m_cache.refreshIfDirty(
			text,
			m_characterSize.value(),
			m_characterSpacing.value(),
			m_sizingMode.value(),
			m_horizontalOverflow.value(),
			m_verticalOverflow.value(),
			rect.size,
			m_textureFontCache,
			m_characterSet.value(),
			m_textureCellSize.value(),
			m_textureOffset.value(),
			m_textureGridColumns.value(),
			m_textureGridRows.value(),
			m_textureCellTrim.value(),
			m_textureCellTrimByCharacterJSON.value());

		const Color& color = m_color.value();
		const Color& addColorValue = m_addColor.value();
		const BlendMode blendModeValue = m_blendMode.value();

		const double autoShrinkWidthScale = m_sizingMode.value() == TextureFontLabelSizingMode::AutoShrinkWidth
			? m_cache.effectiveAutoShrinkWidthScale
			: 1.0;

		const Vec2& characterSize = m_cache.effectiveCharacterSize;

		Optional<ScopedRenderStates2D> blendState;
		switch (blendModeValue)
		{
		case BlendMode::Additive:
			blendState.emplace(BlendState::Additive);
			break;
		case BlendMode::Subtractive:
			blendState.emplace(BlendState::Subtractive);
			break;
		case BlendMode::Multiply:
			blendState.emplace(
				BlendState{
					true,
					Blend::DestColor,
					Blend::InvSrcAlpha,
					BlendOp::Add,
					Blend::DestAlpha,
					Blend::InvSrcAlpha,
					BlendOp::Add
				});
			break;
		default:
			break;
		}

		Optional<ScopedColorAdd2D> colorAdd;
		if (addColorValue.r > 0.0 || addColorValue.g > 0.0 || addColorValue.b > 0.0 || addColorValue.a > 0.0)
		{
			colorAdd.emplace(addColorValue);
		}

		Optional<ScopedRenderStates2D> samplerState;
		const SpriteTextureFilter filterMode = m_textureFilter.value();
		const SpriteTextureAddressMode addressMode = m_textureAddressMode.value();

		if (filterMode != SpriteTextureFilter::Default || addressMode != SpriteTextureAddressMode::Default)
		{
			SamplerState currentState = Graphics2D::GetSamplerState(ShaderStage::Pixel, 0);

			if (addressMode != SpriteTextureAddressMode::Default)
			{
				TextureAddressMode textureAddressMode;
				switch (addressMode)
				{
				case SpriteTextureAddressMode::Repeat:
					textureAddressMode = TextureAddressMode::Repeat;
					break;
				case SpriteTextureAddressMode::Mirror:
					textureAddressMode = TextureAddressMode::Mirror;
					break;
				case SpriteTextureAddressMode::Clamp:
					textureAddressMode = TextureAddressMode::Clamp;
					break;
				case SpriteTextureAddressMode::BorderColor:
					textureAddressMode = TextureAddressMode::Border;
					break;
				default:
					textureAddressMode = currentState.addressU;
					break;
				}
				currentState.addressU = textureAddressMode;
				currentState.addressV = textureAddressMode;
				currentState.addressW = textureAddressMode;
			}

			if (filterMode != SpriteTextureFilter::Default)
			{
				switch (filterMode)
				{
				case SpriteTextureFilter::Nearest:
					currentState.min = TextureFilter::Nearest;
					currentState.mag = TextureFilter::Nearest;
					currentState.mip = TextureFilter::Nearest;
					currentState.maxAnisotropy = 1;
					break;
				case SpriteTextureFilter::Linear:
					currentState.min = TextureFilter::Linear;
					currentState.mag = TextureFilter::Linear;
					currentState.mip = TextureFilter::Linear;
					currentState.maxAnisotropy = 1;
					break;
				case SpriteTextureFilter::Aniso:
					currentState.min = TextureFilter::Linear;
					currentState.mag = TextureFilter::Linear;
					currentState.mip = TextureFilter::Linear;
					currentState.maxAnisotropy = SamplerState::DefaultMaxAnisotropy;
					break;
				default:
					break;
				}
			}

			samplerState.emplace(currentState);
		}

		const HorizontalAlign hAlign = m_horizontalAlign.value();
		const VerticalAlign vAlign = m_verticalAlign.value();

		double contentOffsetY = 0.0;
		if (vAlign == VerticalAlign::Middle)
		{
			contentOffsetY = (rect.h - m_cache.regionSize.y) * 0.5;
		}
		else if (vAlign == VerticalAlign::Bottom)
		{
			contentOffsetY = rect.h - m_cache.regionSize.y;
		}

		const bool preserveAspect = m_preserveAspect.value();
		for (const auto& line : m_cache.lineCaches)
		{
			const double effectiveLineWidth = line.width * autoShrinkWidthScale;
			double lineOffsetX = 0.0;
			if (hAlign == HorizontalAlign::Center)
			{
				lineOffsetX = (rect.w - effectiveLineWidth) * 0.5;
			}
			else if (hAlign == HorizontalAlign::Right)
			{
				lineOffsetX = rect.w - effectiveLineWidth;
			}
			
			for (const auto& charInfo : line.characters)
			{
				const Vec2 drawPos = rect.pos + Vec2{ lineOffsetX + charInfo.position.x * autoShrinkWidthScale, contentOffsetY + line.offsetY };

				// 個別のトリミング指定があれば適用、なければ通常のサイズを使用
				Vec2 finalSize = characterSize;
				if (auto it = m_textureFontCache.metricsMap.find(charInfo.character); it != m_textureFontCache.metricsMap.end())
				{
					const Vec2& effectiveSize = it->second.effectiveSize;
					const Vec2& cellSize = m_textureCellSize.value();
					if (cellSize.x != 0.0 && cellSize.y != 0.0)
					{
						finalSize.x = characterSize.x * effectiveSize.x / cellSize.x;
						finalSize.y = characterSize.y * effectiveSize.y / cellSize.y;
					}
				}

				// 行の固定高さ内で垂直方向に中央揃え
				const double verticalCenterOffset = (characterSize.y - finalSize.y) * 0.5;
				Vec2 centerOffset{ 0.0, verticalCenterOffset };

				if (preserveAspect && m_textureCellSize.value().y > 0.0 && finalSize.y > 0.0)
				{
					// トリミング前のセルサイズのアスペクト比を使用
					const double aspectRatio = m_textureCellSize.value().x / m_textureCellSize.value().y;
					if (aspectRatio > 0.0)
					{
						const double targetAspectRatio = finalSize.x / finalSize.y;

						Vec2 adjustedSize = finalSize;
						if (aspectRatio > targetAspectRatio)
						{
							adjustedSize.y = finalSize.x / aspectRatio;
						}
						else
						{
							adjustedSize.x = finalSize.y * aspectRatio;
						}

						centerOffset = (finalSize - adjustedSize) * 0.5;
						centerOffset.y += verticalCenterOffset;
						finalSize = adjustedSize;
					}
				}

				finalSize.x *= autoShrinkWidthScale;
				centerOffset.x *= autoShrinkWidthScale;

				texture(charInfo.sourceRect).resized(finalSize).draw(drawPos + centerOffset, color);
			}
		}
	}

	void TextureFontLabel::update(const std::shared_ptr<Node>& node)
	{
		refreshAutoResizeImmediately(node);
	}

	SizeF TextureFontLabel::getContentSizeForAutoResize() const
	{
		// rectSize指定なしでのサイズ計算は縮小されないようAutoShrinkはFixedとして扱う
		auto sizingMode = m_sizingMode.value();
		if (sizingMode == TextureFontLabelSizingMode::AutoShrink || sizingMode == TextureFontLabelSizingMode::AutoShrinkWidth)
		{
			sizingMode = TextureFontLabelSizingMode::Fixed;
		}

		m_textureFontCache.refreshIfDirty(
			m_characterSet.value(),
			m_textureCellSize.value(),
			m_textureOffset.value(),
			m_textureGridColumns.value(),
			m_textureGridRows.value(),
			m_textureCellTrim.value(),
			m_textureCellTrimByCharacterJSON.value());

		m_autoResizeCache.refreshIfDirty(
			m_text.value(),
			m_characterSize.value(),
			m_characterSpacing.value(),
			sizingMode,
			HorizontalOverflow::Overflow,
			VerticalOverflow::Overflow,
			m_autoResizeCache.prevParams.has_value() ? m_autoResizeCache.prevParams->rectSize : Vec2::Zero(),
			m_textureFontCache,
			m_characterSet.value(),
			m_textureCellSize.value(),
			m_textureOffset.value(),
			m_textureGridColumns.value(),
			m_textureGridRows.value(),
			m_textureCellTrim.value(),
			m_textureCellTrimByCharacterJSON.value());

		// AutoResizeでは小数点以下を切り上げたサイズをノードサイズとして使用
		const SizeF& contentSize = m_autoResizeCache.regionSize;
		const SizeF ceiledContentSize{ Math::Ceil(contentSize.x), Math::Ceil(contentSize.y) };

		// AutoResizeでは余白を加えたサイズを使用
		const LRTB& padding = m_padding.value();
		return ceiledContentSize + Vec2{ padding.totalWidth(), padding.totalHeight() };
	}

	SizeF TextureFontLabel::getContentSize() const
	{
		// rectSize指定なしでのサイズ計算は縮小されないようAutoShrink/AutoShrinkWidth/AutoResizeHeightはFixedとして扱う
		auto sizingMode = m_sizingMode.value();
		if (sizingMode == TextureFontLabelSizingMode::AutoShrink || sizingMode == TextureFontLabelSizingMode::AutoShrinkWidth || sizingMode == TextureFontLabelSizingMode::AutoResizeHeight)
		{
			sizingMode = TextureFontLabelSizingMode::Fixed;
		}

		m_textureFontCache.refreshIfDirty(
			m_characterSet.value(),
			m_textureCellSize.value(),
			m_textureOffset.value(),
			m_textureGridColumns.value(),
			m_textureGridRows.value(),
			m_textureCellTrim.value(),
			m_textureCellTrimByCharacterJSON.value());

		m_cache.refreshIfDirty(
			m_text.value(),
			m_characterSize.value(),
			m_characterSpacing.value(),
			sizingMode,
			HorizontalOverflow::Overflow,
			VerticalOverflow::Overflow,
			m_cache.prevParams.has_value() ? m_cache.prevParams->rectSize : Vec2::Zero(),
			m_textureFontCache,
			m_characterSet.value(),
			m_textureCellSize.value(),
			m_textureOffset.value(),
			m_textureGridColumns.value(),
			m_textureGridRows.value(),
			m_textureCellTrim.value(),
			m_textureCellTrimByCharacterJSON.value());

		return m_cache.regionSize;
	}

	SizeF TextureFontLabel::getContentSize(const SizeF& rectSize) const
	{
		m_textureFontCache.refreshIfDirty(
			m_characterSet.value(),
			m_textureCellSize.value(),
			m_textureOffset.value(),
			m_textureGridColumns.value(),
			m_textureGridRows.value(),
			m_textureCellTrim.value(),
			m_textureCellTrimByCharacterJSON.value());

		m_cache.refreshIfDirty(
			m_text.value(),
			m_characterSize.value(),
			m_characterSpacing.value(),
			m_sizingMode.value(),
			m_horizontalOverflow.value(),
			m_verticalOverflow.value(),
			rectSize,
			m_textureFontCache,
			m_characterSet.value(),
			m_textureCellSize.value(),
			m_textureOffset.value(),
			m_textureGridColumns.value(),
			m_textureGridRows.value(),
			m_textureCellTrim.value(),
			m_textureCellTrimByCharacterJSON.value());

		return m_cache.regionSize;
	}

	void TextureFontLabel::refreshAutoResizeImmediately(const std::shared_ptr<Node>& node)
	{
		if (m_sizingMode.value() == TextureFontLabelSizingMode::AutoResize)
		{
			const SizeF size = getContentSizeForAutoResize();
			if (node->regionRect().size != size)
			{
				if (const AnchorRegion* pAnchorRegion = node->anchorRegion())
				{
					AnchorRegion newRegion = *pAnchorRegion;
					newRegion.sizeDelta = size;
					newRegion.anchorMax = newRegion.anchorMin;
					node->setRegion(newRegion);
				}
				else if (const InlineRegion* pInlineRegion = node->inlineRegion())
				{
					InlineRegion newRegion = *pInlineRegion;
					newRegion.sizeDelta = size;
					newRegion.sizeRatio = Vec2::Zero();
					newRegion.flexibleWeight = 0.0;
					node->setRegion(newRegion);
				}
			}
		}
		else if (m_sizingMode.value() == TextureFontLabelSizingMode::AutoResizeHeight)
		{
			// AutoResizeHeightはAnchorやsizeRatioによる幅をあらかじめ確定させておく必要があるため、事前にレイアウト更新
			node->refreshContainedCanvasLayoutImmediately();

			const LRTB& padding = m_padding.value();
			const RectF paddedRect = node->regionRect().stretched(
				-padding.top,
				-padding.right,
				-padding.bottom,
				-padding.left
			);

			// 現在のノード幅を使ってコンテンツサイズを計算
			const SizeF contentSize = getContentSize(paddedRect.size);

			// 高さのみを切り上げてpaddingを加算
			const double newHeight = Math::Ceil(contentSize.y) + padding.totalHeight();

			if (node->regionRect().h != newHeight)
			{
				if (const AnchorRegion* pAnchorRegion = node->anchorRegion())
				{
					AnchorRegion newRegion = *pAnchorRegion;
					// 高さのみを更新(幅のsizeDeltaは維持)
					newRegion.sizeDelta.y = newHeight;
					newRegion.anchorMax.y = newRegion.anchorMin.y;
					node->setRegion(newRegion);
				}
				else if (const InlineRegion* pInlineRegion = node->inlineRegion())
				{
					InlineRegion newRegion = *pInlineRegion;
					newRegion.sizeDelta.y = newHeight;
					newRegion.sizeRatio.y = 0.0;
					// 幅に関する設定は維持
					node->setRegion(newRegion);
				}
			}
		}
	}
}
