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

	bool TextureFontLabel::CharacterCache::refreshIfDirty(
		StringView text,
		StringView characterSet,
		const Vec2& characterSize,
		const Vec2& characterSpacing,
		TextureFontLabelSizingMode newSizingMode,
		HorizontalOverflow horizontalOverflow,
		VerticalOverflow verticalOverflow,
		const SizeF& rectSize,
		const TextureFontCache& textureFontCache)
	{
		// isDirtyに渡すパラメータをメンバーから取得
		const bool hasPrevParams = prevParams.has_value();
		const auto& prev = hasPrevParams ? *prevParams : CacheParams{};

		if (hasPrevParams && !prev.isDirty(
			text, characterSet, characterSize, characterSpacing, newSizingMode,
			horizontalOverflow, verticalOverflow, rectSize,
			prev.textureCellSize, prev.textureOffset, prev.textureGridColumns, prev.textureGridRows, prev.preserveAspect))
		{
			return false;
		}

		prevParams = CacheParams{
			.text = String{ text },
			.characterSet = String{ characterSet },
			.characterSize = characterSize,
			.characterSpacing = characterSpacing,
			.sizingMode = newSizingMode,
			.horizontalOverflow = horizontalOverflow,
			.verticalOverflow = verticalOverflow,
			.rectSize = rectSize,
			// isDirtyでは使われないが、念のためTextureFontCacheのパラメータも更新
			.textureCellSize = textureFontCache.prevParams.has_value() ? textureFontCache.prevParams->textureCellSize : Vec2::Zero(),
			.textureOffset = textureFontCache.prevParams.has_value() ? textureFontCache.prevParams->textureOffset : Vec2::Zero(),
			.textureGridColumns = textureFontCache.prevParams.has_value() ? textureFontCache.prevParams->textureGridColumns : 0,
			.textureGridRows = textureFontCache.prevParams.has_value() ? textureFontCache.prevParams->textureGridRows : 0,
			.preserveAspect = prev.preserveAspect, // isDirtyにpreserveAspectがないため元の値を引き継ぐ
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

					const double charWidth = targetCharacterSize.x + targetCharacterSpacing.x;
					if (hov == HorizontalOverflow::Wrap && offset.x + charWidth > rectSize.x)
					{
						if (!fnPushLine())
						{
							break;
						}
					}

					if (auto uvRect = textureFontCache.getUV(ch))
					{
						lineChars.push_back(CharInfo{
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
			const double aspectRatio = (characterSize.y > 0) ? (characterSize.x / characterSize.y) : 1.0;
			const double minSize = 1.0;

			while (currentCharacterSize.x >= minSize && currentCharacterSize.y >= minSize)
			{
				Vec2 currentCharacterSpacing = characterSpacing;
				if (characterSize.x > 0)
				{
					currentCharacterSpacing.x *= (currentCharacterSize.x / characterSize.x);
				}
				if (characterSize.y > 0)
				{
					currentCharacterSpacing.y *= (currentCharacterSize.y / characterSize.y);
				}

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

			Vec2 finalCharacterSpacing = characterSpacing;
			if (characterSize.x > 0)
			{
				finalCharacterSpacing.x *= (this->effectiveCharacterSize.x / characterSize.x);
			}
			if (characterSize.y > 0)
			{
				finalCharacterSpacing.y *= (this->effectiveCharacterSize.y / characterSize.y);
			}
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
		else // Fixed
		{
			this->effectiveCharacterSize = characterSize;
			this->effectiveAutoShrinkWidthScale = 1.0;
			this->regionSize = refreshCacheAndGetRegionSize(characterSize, characterSpacing, horizontalOverflow, verticalOverflow);
		}

		return true;
	}

	void TextureFontLabel::draw(const Node& node) const
	{
		const String& text = m_text.value();
		if (text.empty())
		{
			return;
		}

		const Texture texture = GetTexture(
			m_textureFilePath.value(),
			m_textureAssetName.value()
		);

		if (!texture)
		{
			return;
		}

		const LRTB& padding = m_padding.value();

		const RectF rect = node.regionRect().stretched(
			-padding.top,
			-padding.right,
			-padding.bottom,
			-padding.left
		);

		m_textureFontCache.refreshIfDirty(
			m_characterSet.value(),
			m_textureCellSize.value(),
			m_textureOffset.value(),
			m_textureGridColumns.value(),
			m_textureGridRows.value()
		);

		m_cache.refreshIfDirty(
			text,
			m_characterSet.value(),
			m_characterSize.value(),
			m_characterSpacing.value(),
			m_sizingMode.value(),
			m_horizontalOverflow.value(),
			m_verticalOverflow.value(),
			rect.size,
			m_textureFontCache);

		const Color& color = m_color.value();
		const Color& addColorValue = m_addColor.value();
		const BlendMode blendModeValue = m_blendMode.value();

		const double autoShrinkWidthScale = m_sizingMode.value() == TextureFontLabelSizingMode::AutoShrinkWidth
			? m_cache.effectiveAutoShrinkWidthScale
			: 1.0;

		Vec2 characterSize = m_cache.effectiveCharacterSize;
		characterSize.x *= autoShrinkWidthScale;

		const bool preserveAspect = m_preserveAspect.value();

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
				const Vec2 drawPos = rect.pos + Vec2{
					lineOffsetX + charInfo.position.x * autoShrinkWidthScale,
					contentOffsetY + line.offsetY
				};

				if (preserveAspect && m_textureCellSize.value().y > 0 && characterSize.y > 0)
				{
					const double aspectRatio = m_textureCellSize.value().x / m_textureCellSize.value().y;
					const double targetAspectRatio = characterSize.x / characterSize.y;

					Vec2 adjustedSize = characterSize;
					if (aspectRatio > targetAspectRatio)
					{
						adjustedSize.y = characterSize.x / aspectRatio;
					}
					else
					{
						adjustedSize.x = characterSize.y * aspectRatio;
					}

					const Vec2 centerOffset = (characterSize - adjustedSize) * 0.5;
					texture(charInfo.sourceRect).resized(adjustedSize).draw(drawPos + centerOffset, color);
				}
				else
				{
					texture(charInfo.sourceRect).resized(characterSize).draw(drawPos, color);
				}
			}
		}
	}

	SizeF TextureFontLabel::getContentSizeForAutoResize() const
	{
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
			m_textureGridRows.value()
		);

		m_autoResizeCache.refreshIfDirty(
			m_text.value(),
			m_characterSet.value(),
			m_characterSize.value(),
			m_characterSpacing.value(),
			sizingMode,
			HorizontalOverflow::Overflow,
			VerticalOverflow::Overflow,
			m_autoResizeCache.prevParams.has_value() ? m_autoResizeCache.prevParams->rectSize : Vec2::Zero(),
			m_textureFontCache);

		const SizeF contentSize = m_autoResizeCache.regionSize;
		const SizeF ceiledContentSize = { Math::Ceil(contentSize.x), Math::Ceil(contentSize.y) };

		const LRTB& padding = m_padding.value();
		return ceiledContentSize + Vec2{ padding.totalWidth(), padding.totalHeight() };
	}

	void TextureFontLabel::update(const std::shared_ptr<Node>& node)
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
	}

	SizeF TextureFontLabel::getContentSize() const
	{
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
			m_textureGridRows.value()
		);

		m_cache.refreshIfDirty(
			m_text.value(),
			m_characterSet.value(),
			m_characterSize.value(),
			m_characterSpacing.value(),
			sizingMode,
			HorizontalOverflow::Overflow,
			VerticalOverflow::Overflow,
			m_cache.prevParams.has_value() ? m_cache.prevParams->rectSize : Vec2::Zero(),
			m_textureFontCache);

		return m_cache.regionSize;
	}

	SizeF TextureFontLabel::getContentSize(const SizeF& rectSize) const
	{
		m_textureFontCache.refreshIfDirty(
			m_characterSet.value(),
			m_textureCellSize.value(),
			m_textureOffset.value(),
			m_textureGridColumns.value(),
			m_textureGridRows.value()
		);

		m_cache.refreshIfDirty(
			m_text.value(),
			m_characterSet.value(),
			m_characterSize.value(),
			m_characterSpacing.value(),
			m_sizingMode.value(),
			m_horizontalOverflow.value(),
			m_verticalOverflow.value(),
			rectSize,
			m_textureFontCache);

		return m_cache.regionSize;
	}
}
