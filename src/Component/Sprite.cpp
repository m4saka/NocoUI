#include "NocoUI/Component/Sprite.hpp"
#include "NocoUI/Node.hpp"
#include "NocoUI/Canvas.hpp"
#include "NocoUI/Asset.hpp"
#include "NocoUI/Enums.hpp"

namespace noco
{
	void Sprite::onActivated(const std::shared_ptr<Node>&)
	{
		restartAnimation();
	}
	
	void Sprite::update(const std::shared_ptr<Node>&)
	{
		const SpriteGridAnimationType gridAnimType = m_gridAnimationType.value();
		const SpriteOffsetAnimationType offsetAnimType = m_offsetAnimationType.value();
		
		if (!m_animationStopwatch.isRunning() && (gridAnimType != SpriteGridAnimationType::None || offsetAnimType != SpriteOffsetAnimationType::None))
		{
			m_animationStopwatch.start();
		}
		
		const double elapsedTime = m_animationStopwatch.sF();
		
		if (gridAnimType != SpriteGridAnimationType::None)
		{
			const double fps = m_gridAnimationFPS.value();
			if (fps > 0)
			{
				if (gridAnimType == SpriteGridAnimationType::OneShot)
				{
					if (!m_gridAnimationFinished)
					{
						const int32 startIndex = m_gridAnimationStartIndex.value();
						const int32 endIndex = m_gridAnimationEndIndex.value();

						const int32 frameCount = endIndex - startIndex + 1;

						if (frameCount <= 0)
						{
							m_currentGridAnimationIndex = startIndex;
							m_gridAnimationFinished = true;
						}
						else
						{
							const int32 frameIndex = static_cast<int32>(elapsedTime * fps);

							if (frameIndex >= frameCount)
							{
								m_currentGridAnimationIndex = endIndex;
								m_gridAnimationFinished = true;
							}
							else
							{
								m_currentGridAnimationIndex = startIndex + frameIndex;
							}
						}
					}
				}
				else if (gridAnimType == SpriteGridAnimationType::Loop)
				{
					const int32 startIndex = m_gridAnimationStartIndex.value();
					const int32 endIndex = m_gridAnimationEndIndex.value();

					const int32 frameCount = endIndex - startIndex + 1;

					if (frameCount <= 0)
					{
						m_currentGridAnimationIndex = startIndex;
					}
					else
					{
						const double totalDuration = frameCount / fps;
						const double loopTime = std::fmod(elapsedTime, totalDuration);
						m_currentGridAnimationIndex = startIndex + static_cast<int32>(loopTime * fps) % frameCount;
					}
				}
			}
		}
		
		if (offsetAnimType != SpriteOffsetAnimationType::None)
		{
			const Vec2 speed = m_offsetAnimationSpeed.value();
			const double deltaTime = Scene::DeltaTime();

			switch (offsetAnimType)
			{
			case SpriteOffsetAnimationType::Scroll:
				{
					m_currentOffsetAnimation += speed * deltaTime;

					const Vec2& textureSize = m_textureSize.value();
					if (textureSize.x > 0.0)
					{
						m_currentOffsetAnimation.x = std::fmod(m_currentOffsetAnimation.x, textureSize.x);
						if (m_currentOffsetAnimation.x < 0.0)
						{
							m_currentOffsetAnimation.x += textureSize.x;
						}
					}
					if (textureSize.y > 0.0)
					{
						m_currentOffsetAnimation.y = std::fmod(m_currentOffsetAnimation.y, textureSize.y);
						if (m_currentOffsetAnimation.y < 0.0)
						{
							m_currentOffsetAnimation.y += textureSize.y;
						}
					}
				}
				break;

			default:
				break;
			}
		}
		else
		{
			m_currentOffsetAnimation = Vec2::Zero();
		}
	}

	namespace
	{
		Texture GetTexture(const String& textureFilePath, const String& textureAssetName, MipmapEnabledYN mipmapEnabled)
		{
			if (detail::IsEditorMode())
			{
				// エディタモードではアセット名は無視してファイル名のみを使用
				if (!textureFilePath.empty())
				{
					return noco::Asset::GetOrLoadTexture(textureFilePath, mipmapEnabled);
				}
			}
			else
			{
				// 通常モードではアセット名を優先
				// 気付かないうちにファイルパスが使われるのを避けるため、TextureAssetに指定されたキーが存在しない時もファイルパスへのフォールバックはしない仕様とする
				if (!textureAssetName.empty())
				{
					return TextureAsset(textureAssetName);
				}
				if (!textureFilePath.empty())
				{
					return noco::Asset::GetOrLoadTexture(textureFilePath, mipmapEnabled);
				}
			}
			return Texture{};
		}

		// 描画先矩形をクリップ矩形で切り詰め、切り詰め後の描画先に対応するソース領域(テクスチャピクセル座標)を線形対応で切り出して描画
		void DrawTextureRegionClipped(const Texture& texture, const RectF& sourceRect, const RectF& destRect, const Optional<RectF>& clipRect, const Color& color)
		{
			if (!clipRect)
			{
				texture(sourceRect).resized(destRect.size).draw(destRect.pos, color);
				return;
			}
			if (destRect.w <= 0.0 || destRect.h <= 0.0)
			{
				return;
			}
			const RectF clippedDestRect = destRect.getOverlap(*clipRect);
			if (clippedDestRect.isEmpty())
			{
				return;
			}
			const double scaleX = sourceRect.w / destRect.w;
			const double scaleY = sourceRect.h / destRect.h;
			const RectF clippedSourceRect{
				sourceRect.x + (clippedDestRect.x - destRect.x) * scaleX,
				sourceRect.y + (clippedDestRect.y - destRect.y) * scaleY,
				clippedDestRect.w * scaleX,
				clippedDestRect.h * scaleY };
			texture(clippedSourceRect).resized(clippedDestRect.size).draw(clippedDestRect.pos, color);
		}

		// preserveAspect有効時のfittedと同等の描画先矩形を計算
		RectF CalculateFittedRect(const RectF& rect, const SizeF& sourceSize)
		{
			if (sourceSize.x <= 0.0 || sourceSize.y <= 0.0)
			{
				return rect;
			}
			const double scale = Min(rect.w / sourceSize.x, rect.h / sourceSize.y);
			const SizeF fittedSize = sourceSize * scale;
			return RectF{ rect.center() - fittedSize / 2, fittedSize };
		}
	}

	Optional<RectF> Sprite::fillRectOpt(const RectF& rect) const
	{
		const double fillAmount = Clamp(m_fillAmount.value(), 0.0, 1.0);
		if (fillAmount >= 1.0)
		{
			return none;
		}
		switch (m_fillDirection.value())
		{
		case SpriteFillDirection::BottomToTop:
			return RectF{ rect.x, rect.y + rect.h * (1.0 - fillAmount), rect.w, rect.h * fillAmount };
		case SpriteFillDirection::TopToBottom:
			return RectF{ rect.x, rect.y, rect.w, rect.h * fillAmount };
		case SpriteFillDirection::LeftToRight:
			return RectF{ rect.x, rect.y, rect.w * fillAmount, rect.h };
		case SpriteFillDirection::RightToLeft:
			return RectF{ rect.x + rect.w * (1.0 - fillAmount), rect.y, rect.w * fillAmount, rect.h };
		default:
			return none;
		}
	}

	void Sprite::drawNineSlice(const Texture& texture, const RectF& rect, const Optional<RectF>& clipRect, const Color& color) const
	{
		const LRTB& margin = m_nineSliceMargin.value();
		const Size textureSize = texture.size();
		
		// マージンを素材のピクセル単位で適用
		const double leftMargin = margin.left;
		const double rightMargin = margin.right;
		const double topMargin = margin.top;
		const double bottomMargin = margin.bottom;
		
		// 中央領域のサイズを計算
		const double centerWidth = textureSize.x - leftMargin - rightMargin;
		const double centerHeight = textureSize.y - topMargin - bottomMargin;
		
		// 中央領域が無効な場合は通常描画にフォールバック
		if (centerWidth <= 0 || centerHeight <= 0)
		{
			const RectF sourceRect{ Vec2::Zero(), Vec2{ textureSize } };
			if (m_preserveAspect.value())
			{
				DrawTextureRegionClipped(texture, sourceRect, CalculateFittedRect(rect, Vec2{ textureSize }), clipRect, color);
			}
			else
			{
				DrawTextureRegionClipped(texture, sourceRect, rect, clipRect, color);
			}
			return;
		}

		const Vec2& nineSliceScale = m_nineSliceScale.value();

		const double drawLeftMargin = leftMargin * nineSliceScale.x;
		const double drawRightMargin = rightMargin * nineSliceScale.x;
		const double drawTopMargin = topMargin * nineSliceScale.y;
		const double drawBottomMargin = bottomMargin * nineSliceScale.y;

		if (m_nineSliceFallback.value())
		{
			if (rect.w < drawLeftMargin + drawRightMargin || rect.h < drawTopMargin + drawBottomMargin)
			{
				// サイズが小さすぎる場合は通常描画
				DrawTextureRegionClipped(texture, RectF{ Vec2::Zero(), Vec2{ textureSize } }, rect, clipRect, color);
				return;
			}
		}
		
		// 9つの領域を定義(テクスチャ上の座標)
		const Rect srcTopLeft{ 0, 0, static_cast<int32>(leftMargin), static_cast<int32>(topMargin) };
		const Rect srcTop{ static_cast<int32>(leftMargin), 0, static_cast<int32>(centerWidth), static_cast<int32>(topMargin) };
		const Rect srcTopRight{ static_cast<int32>(textureSize.x - rightMargin), 0, static_cast<int32>(rightMargin), static_cast<int32>(topMargin) };
		
		const Rect srcLeft{ 0, static_cast<int32>(topMargin), static_cast<int32>(leftMargin), static_cast<int32>(centerHeight) };
		const Rect srcCenter{ static_cast<int32>(leftMargin), static_cast<int32>(topMargin), static_cast<int32>(centerWidth), static_cast<int32>(centerHeight) };
		const Rect srcRight{ static_cast<int32>(textureSize.x - rightMargin), static_cast<int32>(topMargin), static_cast<int32>(rightMargin), static_cast<int32>(centerHeight) };
		
		const Rect srcBottomLeft{ 0, static_cast<int32>(textureSize.y - bottomMargin), static_cast<int32>(leftMargin), static_cast<int32>(bottomMargin) };
		const Rect srcBottom{ static_cast<int32>(leftMargin), static_cast<int32>(textureSize.y - bottomMargin), static_cast<int32>(centerWidth), static_cast<int32>(bottomMargin) };
		const Rect srcBottomRight{ static_cast<int32>(textureSize.x - rightMargin), static_cast<int32>(textureSize.y - bottomMargin), static_cast<int32>(rightMargin), static_cast<int32>(bottomMargin) };
		
		const double centerDrawWidth = rect.w - drawLeftMargin - drawRightMargin;
		const double centerDrawHeight = rect.h - drawTopMargin - drawBottomMargin;
		
		const bool centerTiled = m_nineSliceCenterTiled.value();
		const bool leftTiled = m_nineSliceLeftTiled.value();
		const bool rightTiled = m_nineSliceRightTiled.value();
		const bool topTiled = m_nineSliceTopTiled.value();
		const bool bottomTiled = m_nineSliceBottomTiled.value();
		
		// 四隅を描画(常に伸縮)
		DrawTextureRegionClipped(texture, srcTopLeft, RectF{ rect.pos, drawLeftMargin, drawTopMargin }, clipRect, color);
		DrawTextureRegionClipped(texture, srcTopRight, RectF{ rect.x + rect.w - drawRightMargin, rect.y, drawRightMargin, drawTopMargin }, clipRect, color);
		DrawTextureRegionClipped(texture, srcBottomLeft, RectF{ rect.x, rect.y + rect.h - drawBottomMargin, drawLeftMargin, drawBottomMargin }, clipRect, color);
		DrawTextureRegionClipped(texture, srcBottomRight, RectF{ rect.x + rect.w - drawRightMargin, rect.y + rect.h - drawBottomMargin, drawRightMargin, drawBottomMargin }, clipRect, color);

		// 上辺を描画
		if (topTiled)
		{
			// タイル描画
			const double tileWidth = centerWidth * nineSliceScale.x;

			// tileWidthが0以下の場合はスキップ
			if (tileWidth > 0)
			{
				for (double x = rect.x + drawLeftMargin; x < rect.x + rect.w - drawRightMargin; x += tileWidth)
				{
					const double width = Min(tileWidth, rect.x + rect.w - drawRightMargin - x);
					const double srcWidth = width / nineSliceScale.x;
					const RectF partialSrc{ srcTop.x, srcTop.y, srcWidth, topMargin };
					DrawTextureRegionClipped(texture, partialSrc, RectF{ x, rect.y, width, drawTopMargin }, clipRect, color);
				}
			}
		}
		else
		{
			DrawTextureRegionClipped(texture, srcTop, RectF{ rect.x + drawLeftMargin, rect.y, centerDrawWidth, drawTopMargin }, clipRect, color);
		}

		// 下辺を描画
		if (bottomTiled)
		{
			// タイル描画
			const double tileWidth = centerWidth * nineSliceScale.x;

			// tileWidthが0以下の場合はスキップ
			if (tileWidth > 0)
			{
				for (double x = rect.x + drawLeftMargin; x < rect.x + rect.w - drawRightMargin; x += tileWidth)
				{
					const double width = Min(tileWidth, rect.x + rect.w - drawRightMargin - x);
					const double srcWidth = width / nineSliceScale.x;
					const RectF partialSrc{ srcBottom.x, srcBottom.y, srcWidth, bottomMargin };
					DrawTextureRegionClipped(texture, partialSrc, RectF{ x, rect.y + rect.h - drawBottomMargin, width, drawBottomMargin }, clipRect, color);
				}
			}
		}
		else
		{
			DrawTextureRegionClipped(texture, srcBottom, RectF{ rect.x + drawLeftMargin, rect.y + rect.h - drawBottomMargin, centerDrawWidth, drawBottomMargin }, clipRect, color);
		}

		// 左辺を描画
		if (leftTiled)
		{
			// タイル描画
			const double tileHeight = centerHeight * nineSliceScale.y;

			// tileHeightが0以下の場合はスキップ
			if (tileHeight > 0)
			{
				for (double y = rect.y + drawTopMargin; y < rect.y + rect.h - drawBottomMargin; y += tileHeight)
				{
					const double height = Min(tileHeight, rect.y + rect.h - drawBottomMargin - y);
					const double srcHeight = height / nineSliceScale.y;
					const RectF partialSrc{ srcLeft.x, srcLeft.y, leftMargin, srcHeight };
					DrawTextureRegionClipped(texture, partialSrc, RectF{ rect.x, y, drawLeftMargin, height }, clipRect, color);
				}
			}
		}
		else
		{
			DrawTextureRegionClipped(texture, srcLeft, RectF{ rect.x, rect.y + drawTopMargin, drawLeftMargin, centerDrawHeight }, clipRect, color);
		}

		// 右辺を描画
		if (rightTiled)
		{
			// タイル描画
			const double tileHeight = centerHeight * nineSliceScale.y;

			// tileHeightが0以下の場合はスキップ
			if (tileHeight > 0)
			{
				for (double y = rect.y + drawTopMargin; y < rect.y + rect.h - drawBottomMargin; y += tileHeight)
				{
					const double height = Min(tileHeight, rect.y + rect.h - drawBottomMargin - y);
					const double srcHeight = height / nineSliceScale.y;
					const RectF partialSrc{ srcRight.x, srcRight.y, rightMargin, srcHeight };
					DrawTextureRegionClipped(texture, partialSrc, RectF{ rect.x + rect.w - drawRightMargin, y, drawRightMargin, height }, clipRect, color);
				}
			}
		}
		else
		{
			DrawTextureRegionClipped(texture, srcRight, RectF{ rect.x + rect.w - drawRightMargin, rect.y + drawTopMargin, drawRightMargin, centerDrawHeight }, clipRect, color);
		}

		// 中央を描画
		if (centerTiled)
		{
			// タイル描画
			const double tileWidth = centerWidth * nineSliceScale.x;
			const double tileHeight = centerHeight * nineSliceScale.y;

			// tileWidthまたはtileHeightが0以下の場合はスキップ
			if (tileWidth > 0 && tileHeight > 0)
			{
				for (double y = rect.y + drawTopMargin; y < rect.y + rect.h - drawBottomMargin; y += tileHeight)
				{
					const double height = Min(tileHeight, rect.y + rect.h - drawBottomMargin - y);
					const double srcHeight = height / nineSliceScale.y;

					for (double x = rect.x + drawLeftMargin; x < rect.x + rect.w - drawRightMargin; x += tileWidth)
					{
						const double width = Min(tileWidth, rect.x + rect.w - drawRightMargin - x);
						const double srcWidth = width / nineSliceScale.x;
						const RectF partialSrc{ srcCenter.x, srcCenter.y, srcWidth, srcHeight };
						DrawTextureRegionClipped(texture, partialSrc, RectF{ x, y, width, height }, clipRect, color);
					}
				}
			}
		}
		else
		{
			DrawTextureRegionClipped(texture, srcCenter, RectF{ rect.x + drawLeftMargin, rect.y + drawTopMargin, centerDrawWidth, centerDrawHeight }, clipRect, color);
		}
	}

	void Sprite::draw(const Node& node) const
	{
		Texture texture;
		
		// m_textureOptが設定されている場合は優先的に使用
		if (m_textureOpt)
		{
			texture = *m_textureOpt;
		}
		else
		{
			const String& textureFilePath = m_textureFilePath.value();
			const String& textureAssetName = m_textureAssetName.value();
			texture = GetTexture(textureFilePath, textureAssetName, MipmapEnabledYN{ m_mipmapEnabled.value() });
		}
		
		const RectF rect = node.regionRect();

		// fillAmountによるクリップ矩形(1未満の場合のみ値あり)
		const Optional<RectF> clipRect = fillRectOpt(rect);
		if (clipRect && clipRect->isEmpty())
		{
			// 完全にクリップされている場合は描画しない
			return;
		}

		const Color& color = m_color.value();
		const Color& addColorValue = m_addColor.value();
		const BlendMode blendModeValue = m_blendMode.value();
		const TextureRegionMode textureRegionModeValue = m_textureRegionMode.value();
		
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
			// 乗算ブレンド用のカスタムブレンドステート
			// Premultiplied Alphaを考慮し、透明部分が黒くならないようにする
			blendState.emplace(
				BlendState
				{
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
			// Normal の場合は何もしない
			break;
		}
		
		// 加算カラーの設定（完全に黒の場合はnone）
		Optional<ScopedColorAdd2D> colorAdd;
		if (addColorValue.r > 0.0 || addColorValue.g > 0.0 || addColorValue.b > 0.0 || addColorValue.a > 0.0)
		{
			colorAdd.emplace(addColorValue);
		}

		// SamplerStateの設定
		Optional<ScopedRenderStates2D> samplerState;
		const SpriteTextureFilter filterMode = m_textureFilter.value();
		const SpriteTextureAddressMode addressMode = m_textureAddressMode.value();
		const bool mipmapEnabled = m_mipmapEnabled.value();
		const double mipmapLodBias = m_mipmapLodBias.value();
		// LODバイアスはミップマップ使用時のみ意味を持ち、0のときはSiv3D標準のサンプラーをそのまま使う
		const bool applyLodBias = mipmapEnabled && mipmapLodBias != 0.0;

		if (filterMode != SpriteTextureFilter::Default || addressMode != SpriteTextureAddressMode::Default || applyLodBias)
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

			// LODバイアスの設定
			if (applyLodBias)
			{
				// 負の値でLODを鮮鋭側へバイアスする(軽度の縮小ではmip0を維持しつつ、大きな縮小でのみミップを使う)
				currentState.lodBias = static_cast<float>(mipmapLodBias);
			}

			samplerState.emplace(currentState);
		}

		Optional<ScopedCustomShader2D> customShader;
		{
			const String& psAssetName = m_pixelShaderAssetName.value();
			if (!psAssetName.empty())
			{
				customShader.emplace(PixelShaderAsset(psAssetName));
			}
		}

		if (!texture)
		{
			if (m_hideIfTextureEmpty.value())
			{
				return;
			}
			// 空のテクスチャ(黄色になる)をそのまま描画
			texture.resized(rect.size).draw(rect.pos, color);
			return;
		}
		
		// テクスチャ領域の指定がある場合
		if (textureRegionModeValue == TextureRegionMode::OffsetSize)
		{
			const Vec2& baseOffset = m_textureOffset.value();
			const Vec2& size = m_textureSize.value();
			
			const Vec2 offset = baseOffset + m_currentOffsetAnimation;
			
			// サイズが0の場合はテクスチャ全体のサイズを使用
			const Vec2 actualSize = (size.x > 0 && size.y > 0) ? size : Vec2{ texture.size() };
			
			const RectF textureRect{ offset.x, offset.y, actualSize.x, actualSize.y };

			if (m_nineSliceEnabled.value())
			{
				drawNineSliceFromRegion(texture, textureRect, rect, clipRect, color);
			}
			else if (m_preserveAspect.value())
			{
				DrawTextureRegionClipped(texture, textureRect, CalculateFittedRect(rect, actualSize), clipRect, color);
			}
			else
			{
				DrawTextureRegionClipped(texture, textureRect, rect, clipRect, color);
			}
		}
		else if (textureRegionModeValue == TextureRegionMode::Grid)
		{
			const int32 index = (m_gridAnimationType.value() == SpriteGridAnimationType::None)
				? m_textureGridIndex.value()
				: m_currentGridAnimationIndex;
			const int32 columns = m_textureGridColumns.value();
			const int32 rows = m_textureGridRows.value();
			
			// columnsまたはrowsが0以下の場合は描画しない
			if (columns <= 0 || rows <= 0)
			{
				return;
			}
			
			// 範囲チェック
			const int32 maxIndex = columns * rows;
			if (index < 0 || index >= maxIndex)
			{
				// インデックスが範囲外の場合は描画しない
				return;
			}
			
			const int32 gridX = index % columns;
			const int32 gridY = index / columns;

			const Vec2& offset = m_textureOffset.value();
			const Vec2& cellSize = m_textureGridCellSize.value();
			const LRTB& trim = m_textureGridCellTrim.value();

			// トリミングを適用したテクスチャ領域
			const RectF textureRect{
				offset.x + gridX * cellSize.x + trim.left,
				offset.y + gridY * cellSize.y + trim.top,
				cellSize.x - trim.left - trim.right,
				cellSize.y - trim.top - trim.bottom
			};

			// トリミング後の有効サイズを計算
			const Vec2 effectiveSize{
				cellSize.x - trim.left - trim.right,
				cellSize.y - trim.top - trim.bottom
			};

			if (m_nineSliceEnabled.value())
			{
				drawNineSliceFromRegion(texture, textureRect, rect, clipRect, color);
			}
			else if (m_preserveAspect.value())
			{
				// トリミング前のセルサイズでアスペクト比固定のスケールを計算
				const double cellAspect = cellSize.y != 0.0 ? cellSize.x / cellSize.y : 1.0;
				const double rectAspect = rect.h != 0.0 ? rect.w / rect.h : 1.0;

				Vec2 fittedSize = rect.size;
				if (cellAspect > rectAspect)
				{
					fittedSize.y = rect.w / cellAspect;
				}
				else
				{
					fittedSize.x = rect.h * cellAspect;
				}

				// トリミング後のサイズに変換
				Vec2 drawSize = fittedSize;
				if (cellSize.x != 0.0 && cellSize.y != 0.0)
				{
					drawSize.x = fittedSize.x * effectiveSize.x / cellSize.x;
					drawSize.y = fittedSize.y * effectiveSize.y / cellSize.y;
				}

				const Vec2 drawPos = rect.pos + (rect.size - drawSize) * 0.5;
				DrawTextureRegionClipped(texture, textureRect, RectF{ drawPos, drawSize }, clipRect, color);
			}
			else
			{
				// トリミング後のサイズに基づいて描画サイズを計算
				Vec2 drawSize = rect.size;
				if (cellSize.x != 0.0 && cellSize.y != 0.0)
				{
					drawSize.x = rect.w * effectiveSize.x / cellSize.x;
					drawSize.y = rect.h * effectiveSize.y / cellSize.y;
				}

				// 中央揃え
				const Vec2 drawPos = rect.pos + (rect.size - drawSize) * 0.5;

				DrawTextureRegionClipped(texture, textureRect, RectF{ drawPos, drawSize }, clipRect, color);
			}
		}
		else
		{
			if (m_nineSliceEnabled.value())
			{
				drawNineSlice(texture, rect, clipRect, color);
			}
			else if (m_preserveAspect.value())
			{
				const Vec2 textureSize{ texture.size() };
				DrawTextureRegionClipped(texture, RectF{ Vec2::Zero(), textureSize }, CalculateFittedRect(rect, textureSize), clipRect, color);
			}
			else
			{
				DrawTextureRegionClipped(texture, RectF{ Vec2::Zero(), Vec2{ texture.size() } }, rect, clipRect, color);
			}
		}
	}

	void Sprite::drawNineSliceFromRegion(const Texture& texture, const RectF& sourceRect, const RectF& rect, const Optional<RectF>& clipRect, const Color& color) const
	{
		const LRTB& margin = m_nineSliceMargin.value();

		const double leftMargin = margin.left;
		const double rightMargin = margin.right;
		const double topMargin = margin.top;
		const double bottomMargin = margin.bottom;

		const double centerWidth = sourceRect.w - leftMargin - rightMargin;
		const double centerHeight = sourceRect.h - topMargin - bottomMargin;

		// 中央領域が無効な場合は通常描画にフォールバック
		if (centerWidth <= 0 || centerHeight <= 0)
		{
			if (m_preserveAspect.value())
			{
				DrawTextureRegionClipped(texture, sourceRect, CalculateFittedRect(rect, sourceRect.size), clipRect, color);
			}
			else
			{
				DrawTextureRegionClipped(texture, sourceRect, rect, clipRect, color);
			}
			return;
		}

		const Vec2& nineSliceScale = m_nineSliceScale.value();

		const double drawLeftMargin = leftMargin * nineSliceScale.x;
		const double drawRightMargin = rightMargin * nineSliceScale.x;
		const double drawTopMargin = topMargin * nineSliceScale.y;
		const double drawBottomMargin = bottomMargin * nineSliceScale.y;

		if (m_nineSliceFallback.value())
		{
			if (rect.w < drawLeftMargin + drawRightMargin || rect.h < drawTopMargin + drawBottomMargin)
			{
				// サイズが小さすぎる場合は通常描画
				DrawTextureRegionClipped(texture, sourceRect, rect, clipRect, color);
				return;
			}
		}
		
		const double srcX = sourceRect.x;
		const double srcY = sourceRect.y;
		
		const RectF srcTopLeft{ srcX, srcY, leftMargin, topMargin };
		const RectF srcTop{ srcX + leftMargin, srcY, centerWidth, topMargin };
		const RectF srcTopRight{ srcX + sourceRect.w - rightMargin, srcY, rightMargin, topMargin };
		
		const RectF srcLeft{ srcX, srcY + topMargin, leftMargin, centerHeight };
		const RectF srcCenter{ srcX + leftMargin, srcY + topMargin, centerWidth, centerHeight };
		const RectF srcRight{ srcX + sourceRect.w - rightMargin, srcY + topMargin, rightMargin, centerHeight };
		
		const RectF srcBottomLeft{ srcX, srcY + sourceRect.h - bottomMargin, leftMargin, bottomMargin };
		const RectF srcBottom{ srcX + leftMargin, srcY + sourceRect.h - bottomMargin, centerWidth, bottomMargin };
		const RectF srcBottomRight{ srcX + sourceRect.w - rightMargin, srcY + sourceRect.h - bottomMargin, rightMargin, bottomMargin };
		
		const double centerDrawWidth = rect.w - drawLeftMargin - drawRightMargin;
		const double centerDrawHeight = rect.h - drawTopMargin - drawBottomMargin;
		
		const bool centerTiled = m_nineSliceCenterTiled.value();
		const bool leftTiled = m_nineSliceLeftTiled.value();
		const bool rightTiled = m_nineSliceRightTiled.value();
		const bool topTiled = m_nineSliceTopTiled.value();
		const bool bottomTiled = m_nineSliceBottomTiled.value();
		
		// 四隅を描画(常に伸縮)
		DrawTextureRegionClipped(texture, srcTopLeft, RectF{ rect.pos, drawLeftMargin, drawTopMargin }, clipRect, color);
		DrawTextureRegionClipped(texture, srcTopRight, RectF{ rect.x + rect.w - drawRightMargin, rect.y, drawRightMargin, drawTopMargin }, clipRect, color);
		DrawTextureRegionClipped(texture, srcBottomLeft, RectF{ rect.x, rect.y + rect.h - drawBottomMargin, drawLeftMargin, drawBottomMargin }, clipRect, color);
		DrawTextureRegionClipped(texture, srcBottomRight, RectF{ rect.x + rect.w - drawRightMargin, rect.y + rect.h - drawBottomMargin, drawRightMargin, drawBottomMargin }, clipRect, color);

		// 上辺を描画
		if (topTiled)
		{
			const double tileWidth = centerWidth * nineSliceScale.x;

			if (tileWidth > 0)
			{
				for (double x = rect.x + drawLeftMargin; x < rect.x + rect.w - drawRightMargin; x += tileWidth)
				{
					const double width = Min(tileWidth, rect.x + rect.w - drawRightMargin - x);
					const double srcWidth = width / nineSliceScale.x;
					const RectF partialSrc{ srcTop.x, srcTop.y, srcWidth, topMargin };
					DrawTextureRegionClipped(texture, partialSrc, RectF{ x, rect.y, width, drawTopMargin }, clipRect, color);
				}
			}
		}
		else
		{
			DrawTextureRegionClipped(texture, srcTop, RectF{ rect.x + drawLeftMargin, rect.y, centerDrawWidth, drawTopMargin }, clipRect, color);
		}

		// 下辺を描画
		if (bottomTiled)
		{
			const double tileWidth = centerWidth * nineSliceScale.x;

			if (tileWidth > 0)
			{
				for (double x = rect.x + drawLeftMargin; x < rect.x + rect.w - drawRightMargin; x += tileWidth)
				{
					const double width = Min(tileWidth, rect.x + rect.w - drawRightMargin - x);
					const double srcWidth = width / nineSliceScale.x;
					const RectF partialSrc{ srcBottom.x, srcBottom.y, srcWidth, bottomMargin };
					DrawTextureRegionClipped(texture, partialSrc, RectF{ x, rect.y + rect.h - drawBottomMargin, width, drawBottomMargin }, clipRect, color);
				}
			}
		}
		else
		{
			DrawTextureRegionClipped(texture, srcBottom, RectF{ rect.x + drawLeftMargin, rect.y + rect.h - drawBottomMargin, centerDrawWidth, drawBottomMargin }, clipRect, color);
		}

		// 左辺を描画
		if (leftTiled)
		{
			const double tileHeight = centerHeight * nineSliceScale.y;

			if (tileHeight > 0)
			{
				for (double y = rect.y + drawTopMargin; y < rect.y + rect.h - drawBottomMargin; y += tileHeight)
				{
					const double height = Min(tileHeight, rect.y + rect.h - drawBottomMargin - y);
					const double srcHeight = height / nineSliceScale.y;
					const RectF partialSrc{ srcLeft.x, srcLeft.y, leftMargin, srcHeight };
					DrawTextureRegionClipped(texture, partialSrc, RectF{ rect.x, y, drawLeftMargin, height }, clipRect, color);
				}
			}
		}
		else
		{
			DrawTextureRegionClipped(texture, srcLeft, RectF{ rect.x, rect.y + drawTopMargin, drawLeftMargin, centerDrawHeight }, clipRect, color);
		}

		// 右辺を描画
		if (rightTiled)
		{
			const double tileHeight = centerHeight * nineSliceScale.y;

			if (tileHeight > 0)
			{
				for (double y = rect.y + drawTopMargin; y < rect.y + rect.h - drawBottomMargin; y += tileHeight)
				{
					const double height = Min(tileHeight, rect.y + rect.h - drawBottomMargin - y);
					const double srcHeight = height / nineSliceScale.y;
					const RectF partialSrc{ srcRight.x, srcRight.y, rightMargin, srcHeight };
					DrawTextureRegionClipped(texture, partialSrc, RectF{ rect.x + rect.w - drawRightMargin, y, drawRightMargin, height }, clipRect, color);
				}
			}
		}
		else
		{
			DrawTextureRegionClipped(texture, srcRight, RectF{ rect.x + rect.w - drawRightMargin, rect.y + drawTopMargin, drawRightMargin, centerDrawHeight }, clipRect, color);
		}

		// 中央を描画
		if (centerTiled)
		{
			const double tileWidth = centerWidth * nineSliceScale.x;
			const double tileHeight = centerHeight * nineSliceScale.y;

			if (tileWidth > 0 && tileHeight > 0)
			{
				for (double y = rect.y + drawTopMargin; y < rect.y + rect.h - drawBottomMargin; y += tileHeight)
				{
					const double height = Min(tileHeight, rect.y + rect.h - drawBottomMargin - y);
					const double srcHeight = height / nineSliceScale.y;

					for (double x = rect.x + drawLeftMargin; x < rect.x + rect.w - drawRightMargin; x += tileWidth)
					{
						const double width = Min(tileWidth, rect.x + rect.w - drawRightMargin - x);
						const double srcWidth = width / nineSliceScale.x;
						const RectF partialSrc{ srcCenter.x, srcCenter.y, srcWidth, srcHeight };
						DrawTextureRegionClipped(texture, partialSrc, RectF{ x, y, width, height }, clipRect, color);
					}
				}
			}
		}
		else
		{
			DrawTextureRegionClipped(texture, srcCenter, RectF{ rect.x + drawLeftMargin, rect.y + drawTopMargin, centerDrawWidth, centerDrawHeight }, clipRect, color);
		}
	}
}
