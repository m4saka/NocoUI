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
		TextureFontLabelSizingMode textureFontSizingMode,
		HorizontalOverflow horizontalOverflow,
		VerticalOverflow verticalOverflow,
		const SizeF& rectSize,
		const Vec2& textureCellSize,
		const Vec2& textureOffset,
		int32 textureGridColumns,
		int32 textureGridRows,
		bool preserveAspect,
		const TextureFontCache& textureFontCache)
	{
		if (prevParams.has_value() && !prevParams->isDirty(
			text, characterSet, characterSize, characterSpacing, textureFontSizingMode,
			horizontalOverflow, verticalOverflow, rectSize,
			textureCellSize, textureOffset, textureGridColumns, textureGridRows, preserveAspect))
		{
			return false;
		}

		prevParams = CacheParams{
			.text = String{ text },
			.characterSet = String{ characterSet },
			.characterSize = characterSize,
			.characterSpacing = characterSpacing,
			.sizingMode = sizingMode,
			.horizontalOverflow = horizontalOverflow,
			.verticalOverflow = verticalOverflow,
			.rectSize = rectSize,
			.textureCellSize = textureCellSize,
			.textureOffset = textureOffset,
			.textureGridColumns = textureGridColumns,
			.textureGridRows = textureGridRows,
			.preserveAspect = preserveAspect,
		};

		lineCaches.clear();

		double maxWidth = 0.0;
		Vec2 offset = Vec2::Zero();
		Array<CharInfo> lineChars;
		const double lineHeight = characterSize.y;

		const auto fnPushLine = [&]() -> bool
		{
			const double currentLineBottom = offset.y + lineHeight;

			if (verticalOverflow == VerticalOverflow::Clip && currentLineBottom > rectSize.y)
			{
				return false;
			}

			if (!lineChars.empty())
			{
				offset.x -= characterSpacing.x;
			}

			lineCaches.push_back({ lineChars, offset.x, offset.y });
			lineChars.clear();
			maxWidth = Max(maxWidth, offset.x);
			offset.x = 0;

			offset.y = currentLineBottom + characterSpacing.y;
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

			const double charWidth = characterSize.x + characterSpacing.x;
			if (horizontalOverflow == HorizontalOverflow::Wrap && offset.x + charWidth > rectSize.x)
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

		if (!lineChars.empty())
		{
			fnPushLine();
		}

		contentSize = { maxWidth, offset.y - characterSpacing.y };
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

		const Vec2& characterSpacing = m_characterSpacing.value();
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

		const bool wasUpdated = m_cache.refreshIfDirty(
			text,
			m_characterSet.value(),
			m_characterSize.value(),
			characterSpacing,
			m_sizingMode.value(),
			m_horizontalOverflow.value(),
			m_verticalOverflow.value(),
			rect.size,
			m_textureCellSize.value(),
			m_textureOffset.value(),
			m_textureGridColumns.value(),
			m_textureGridRows.value(),
			m_preserveAspect.value(),
			m_textureFontCache);

		Vec2 effectiveCharacterSize = m_characterSize.value();
		
		if (m_sizingMode.value() == TextureFontLabelSizingMode::AutoShrink)
		{
			const SizeF availableSize = rect.size;
			
			constexpr double Epsilon = 1e-2;
			const bool cacheValid = !wasUpdated &&
			                       (m_cache.sizingMode == TextureFontLabelSizingMode::AutoShrink) &&
			                       (Abs(m_cache.availableSize.x - availableSize.x) < Epsilon) &&
			                       (Abs(m_cache.availableSize.y - availableSize.y) < Epsilon) &&
			                       (Abs(m_cache.originalCharacterSize.x - m_characterSize.value().x) < Epsilon) &&
			                       (Abs(m_cache.originalCharacterSize.y - m_characterSize.value().y) < Epsilon);
			
			if (!cacheValid)
			{
				m_cache.sizingMode = TextureFontLabelSizingMode::AutoShrink;
				m_cache.availableSize = availableSize;
				m_cache.originalCharacterSize = m_characterSize.value();
				
				effectiveCharacterSize = m_characterSize.value();
				const double aspectRatio = effectiveCharacterSize.x / effectiveCharacterSize.y;
				const double minSize = 1.0;
				
				while (effectiveCharacterSize.x >= minSize && effectiveCharacterSize.y >= minSize)
				{
					m_cache.prevParams.reset();
					m_cache.refreshIfDirty(
						text,
						m_characterSet.value(),
						effectiveCharacterSize,
						characterSpacing,
						m_sizingMode.value(),
						m_horizontalOverflow.value(),
						VerticalOverflow::Overflow,
						availableSize,
						m_textureCellSize.value(),
						m_textureOffset.value(),
						m_textureGridColumns.value(),
						m_textureGridRows.value(),
						m_preserveAspect.value(),
						m_textureFontCache);
					
					if (m_cache.contentSize.x <= availableSize.x && 
					    m_cache.contentSize.y <= availableSize.y)
					{
						break;
					}
					
					effectiveCharacterSize *= ShrinkScaleFactor;
					
					// 縦横比を維持しながら、いずれかが最小値に達したら停止
					if (effectiveCharacterSize.x < minSize)
					{
						effectiveCharacterSize.x = minSize;
						effectiveCharacterSize.y = minSize / aspectRatio;
						m_cache.prevParams.reset();
						m_cache.refreshIfDirty(
							text,
							m_characterSet.value(),
							effectiveCharacterSize,
							characterSpacing,
							m_sizingMode.value(),
							m_horizontalOverflow.value(),
							VerticalOverflow::Overflow,
							availableSize,
							m_textureCellSize.value(),
							m_textureOffset.value(),
							m_textureGridColumns.value(),
							m_textureGridRows.value(),
							m_preserveAspect.value(),
							m_textureFontCache);
						break;
					}
					else if (effectiveCharacterSize.y < minSize)
					{
						effectiveCharacterSize.y = minSize;
						effectiveCharacterSize.x = minSize * aspectRatio;
						m_cache.prevParams.reset();
						m_cache.refreshIfDirty(
							text,
							m_characterSet.value(),
							effectiveCharacterSize,
							characterSpacing,
							m_sizingMode.value(),
							m_horizontalOverflow.value(),
							VerticalOverflow::Overflow,
							availableSize,
							m_textureCellSize.value(),
							m_textureOffset.value(),
							m_textureGridColumns.value(),
							m_textureGridRows.value(),
							m_preserveAspect.value(),
							m_textureFontCache);
						break;
					}
				}
				
				m_cache.effectiveCharacterSize = effectiveCharacterSize;
				
				if (m_verticalOverflow.value() == VerticalOverflow::Clip)
				{
					m_cache.prevParams.reset();
					m_cache.refreshIfDirty(
						text,
						m_characterSet.value(),
						effectiveCharacterSize,
						characterSpacing,
						m_sizingMode.value(),
						m_horizontalOverflow.value(),
						m_verticalOverflow.value(),
						availableSize,
						m_textureCellSize.value(),
						m_textureOffset.value(),
						m_textureGridColumns.value(),
						m_textureGridRows.value(),
						m_preserveAspect.value(),
						m_textureFontCache);
				}
			}
			else
			{
				effectiveCharacterSize = m_cache.effectiveCharacterSize;
			}
		}
		else if (m_sizingMode.value() == TextureFontLabelSizingMode::AutoShrinkWidth)
		{
			const SizeF availableSize = rect.size;
			
			constexpr double Epsilon = 1e-2;
			const bool cacheValid = !wasUpdated &&
			                       (m_cache.sizingMode == TextureFontLabelSizingMode::AutoShrinkWidth) &&
			                       (Abs(m_cache.availableSize.x - availableSize.x) < Epsilon) &&
			                       (Abs(m_cache.availableSize.y - availableSize.y) < Epsilon) &&
			                       (Abs(m_cache.originalCharacterSize.x - m_characterSize.value().x) < Epsilon) &&
			                       (Abs(m_cache.originalCharacterSize.y - m_characterSize.value().y) < Epsilon);
			
			if (!cacheValid)
			{
				m_cache.sizingMode = TextureFontLabelSizingMode::AutoShrinkWidth;
				m_cache.availableSize = availableSize;
				m_cache.originalCharacterSize = m_characterSize.value();
				
				effectiveCharacterSize = m_characterSize.value();
				const double minCharWidth = 1.0;
				
				while (effectiveCharacterSize.x >= minCharWidth)
				{
					m_cache.prevParams.reset();
					m_cache.refreshIfDirty(
						text,
						m_characterSet.value(),
						effectiveCharacterSize,
						characterSpacing,
						m_sizingMode.value(),
						m_horizontalOverflow.value(),
						VerticalOverflow::Overflow,
						availableSize,
						m_textureCellSize.value(),
						m_textureOffset.value(),
						m_textureGridColumns.value(),
						m_textureGridRows.value(),
						m_preserveAspect.value(),
						m_textureFontCache);
					
					if (m_cache.contentSize.x <= availableSize.x && 
					    m_cache.contentSize.y <= availableSize.y)
					{
						break;
					}
					
					effectiveCharacterSize.x *= ShrinkScaleFactor;
					// AutoShrinkWidthでは高さは固定のまま、幅のみ縮小
					
					if (effectiveCharacterSize.x < minCharWidth)
					{
						effectiveCharacterSize.x = minCharWidth;
						m_cache.prevParams.reset();
						m_cache.refreshIfDirty(
							text,
							m_characterSet.value(),
							effectiveCharacterSize,
							characterSpacing,
							m_sizingMode.value(),
							m_horizontalOverflow.value(),
							VerticalOverflow::Overflow,
							availableSize,
							m_textureCellSize.value(),
							m_textureOffset.value(),
							m_textureGridColumns.value(),
							m_textureGridRows.value(),
							m_preserveAspect.value(),
							m_textureFontCache);
						break;
					}
				}
				
				m_cache.effectiveCharacterSize = effectiveCharacterSize;
				
				if (m_verticalOverflow.value() == VerticalOverflow::Clip)
				{
					m_cache.prevParams.reset();
					m_cache.refreshIfDirty(
						text,
						m_characterSet.value(),
						effectiveCharacterSize,
						characterSpacing,
						m_sizingMode.value(),
						m_horizontalOverflow.value(),
						m_verticalOverflow.value(),
						availableSize,
						m_textureCellSize.value(),
						m_textureOffset.value(),
						m_textureGridColumns.value(),
						m_textureGridRows.value(),
						m_preserveAspect.value(),
						m_textureFontCache);
				}
			}
			else
			{
				effectiveCharacterSize = m_cache.effectiveCharacterSize;
			}
		}
		else
		{
			if (m_cache.sizingMode == TextureFontLabelSizingMode::AutoShrink || 
			    m_cache.sizingMode == TextureFontLabelSizingMode::AutoShrinkWidth)
			{
				effectiveCharacterSize = m_characterSize.value();
			}
			m_cache.sizingMode = TextureFontLabelSizingMode::Fixed;
		}
		
		// AutoShrink/AutoShrinkWidthモードでは、サイズ探索時にrefreshIfDirtyを呼ぶため、
		// prevParams->characterSizeが縮小後のサイズで保存される。
		// 次フレームで元のcharacterSizeと比較されると常に変更ありと判定され、
		// 毎フレーム再計算が発生してしまう。
		// これを防ぐため、prevParams->characterSizeを元のサイズに戻す。
		if (m_cache.prevParams.has_value() && 
		    (m_sizingMode.value() == TextureFontLabelSizingMode::AutoShrink || 
		     m_sizingMode.value() == TextureFontLabelSizingMode::AutoShrinkWidth))
		{
			m_cache.prevParams->characterSize = m_characterSize.value();
		}

		const ColorF& color = m_color.value();
		const ColorF& addColorValue = m_addColor.value();
		const BlendMode blendModeValue = m_blendMode.value();
		const Vec2& characterSize = effectiveCharacterSize;
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

		// SamplerStateの設定
		Optional<ScopedRenderStates2D> samplerState;
		const SpriteTextureFilter filterMode = m_textureFilter.value();
		const SpriteTextureAddressMode addressMode = m_textureAddressMode.value();

		if (filterMode != SpriteTextureFilter::Default || addressMode != SpriteTextureAddressMode::Default)
		{
			// 現在のSamplerStateを取得してベースにする
			SamplerState currentState = Graphics2D::GetSamplerState(ShaderStage::Pixel, 0);

			// TextureAddressModeの設定
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

			// TextureFilterの設定
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
			contentOffsetY = (rect.h - m_cache.contentSize.y) * 0.5;
		}
		else if (vAlign == VerticalAlign::Bottom)
		{
			contentOffsetY = rect.h - m_cache.contentSize.y;
		}

		for (const auto& line : m_cache.lineCaches)
		{
			double lineOffsetX = 0.0;
			if (hAlign == HorizontalAlign::Center)
			{
				lineOffsetX = (rect.w - line.width) * 0.5;
			}
			else if (hAlign == HorizontalAlign::Right)
			{
				lineOffsetX = rect.w - line.width;
			}

			for (const auto& charInfo : line.characters)
			{
				const Vec2 drawPos = rect.pos + Vec2{
					lineOffsetX + charInfo.position.x,
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
}
