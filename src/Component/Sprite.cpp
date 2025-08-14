#include "NocoUI/Component/Sprite.hpp"
#include "NocoUI/Node.hpp"
#include "NocoUI/Asset.hpp"
#include "NocoUI/Enums.hpp"

namespace noco
{
	namespace
	{
		String FormatTextureAssetName(const String& textureFilePath)
		{
			return U"noco::{}"_fmt(textureFilePath);
		}

		Texture GetTexture(const String& textureFilePath, const String& textureAssetName)
		{
#ifdef NOCO_EDITOR
			// エディタでは除外
			(void)textureAssetName;
#else
			// 気付かないうちにファイルパスが使われるのを避けるため、TextureAssetに指定されたキーが存在しない時もファイルパスへのフォールバックはしない仕様とする
			if (!textureAssetName.empty())
			{
				return TextureAsset(textureAssetName);
			}
#endif // NOCO_EDITOR
			if (!textureFilePath.empty())
			{
				return noco::Asset::GetOrLoadTexture(textureFilePath);
			}
			return Texture{};
		}
	}

	void Sprite::drawNineSlice(const Texture& texture, const RectF& rect, const ColorF& color) const
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
			if (m_preserveAspect.value())
			{
				texture.fitted(rect.size).drawAt(rect.center(), color);
			}
			else
			{
				texture.resized(rect.size).draw(rect.pos, color);
			}
			return;
		}
		
		// nineSliceScaleプロパティを取得
		const Vec2& nineSliceScale = m_nineSliceScale.value();
		
		// nineSliceScaleを考慮した描画サイズのマージン
		const double drawLeftMargin = leftMargin * nineSliceScale.x;
		const double drawRightMargin = rightMargin * nineSliceScale.x;
		const double drawTopMargin = topMargin * nineSliceScale.y;
		const double drawBottomMargin = bottomMargin * nineSliceScale.y;
		
		// フォールバック判定
		if (m_nineSliceFallback.value())
		{
			if (rect.w < drawLeftMargin + drawRightMargin || rect.h < drawTopMargin + drawBottomMargin)
			{
				// サイズが小さすぎる場合は通常描画
				texture.resized(rect.size).draw(rect.pos, color);
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
		
		// 描画先の領域を定義
		const double centerDrawWidth = rect.w - drawLeftMargin - drawRightMargin;
		const double centerDrawHeight = rect.h - drawTopMargin - drawBottomMargin;
		
		// タイル設定を取得
		const bool centerTiled = m_nineSliceCenterTiled.value();
		const bool leftTiled = m_nineSliceLeftTiled.value();
		const bool rightTiled = m_nineSliceRightTiled.value();
		const bool topTiled = m_nineSliceTopTiled.value();
		const bool bottomTiled = m_nineSliceBottomTiled.value();
		
		// 四隅を描画(常に伸縮)
		texture(srcTopLeft).resized(drawLeftMargin, drawTopMargin).draw(rect.pos, color);
		texture(srcTopRight).resized(drawRightMargin, drawTopMargin).draw(rect.x + rect.w - drawRightMargin, rect.y, color);
		texture(srcBottomLeft).resized(drawLeftMargin, drawBottomMargin).draw(rect.x, rect.y + rect.h - drawBottomMargin, color);
		texture(srcBottomRight).resized(drawRightMargin, drawBottomMargin).draw(rect.x + rect.w - drawRightMargin, rect.y + rect.h - drawBottomMargin, color);
		
		// 上辺を描画
		if (topTiled)
		{
			// タイル描画
			const TextureRegion topRegion = texture(srcTop);
			const double tileWidth = centerWidth * nineSliceScale.x;
			
			// tileWidthが0以下の場合はスキップ
			if (tileWidth > 0)
			{
				for (double x = rect.x + drawLeftMargin; x < rect.x + rect.w - drawRightMargin; x += tileWidth)
				{
					const double width = Min(tileWidth, rect.x + rect.w - drawRightMargin - x);
					const double uvWidth = width / nineSliceScale.x;
					topRegion.resized(uvWidth, topMargin).resized(width, drawTopMargin).draw(x, rect.y, color);
				}
			}
		}
		else
		{
			// 伸縮描画
			texture(srcTop).resized(centerDrawWidth, drawTopMargin).draw(rect.x + drawLeftMargin, rect.y, color);
		}
		
		// 下辺を描画
		if (bottomTiled)
		{
			// タイル描画
			const TextureRegion bottomRegion = texture(srcBottom);
			const double tileWidth = centerWidth * nineSliceScale.x;
			
			// tileWidthが0以下の場合はスキップ
			if (tileWidth > 0)
			{
				for (double x = rect.x + drawLeftMargin; x < rect.x + rect.w - drawRightMargin; x += tileWidth)
				{
					const double width = Min(tileWidth, rect.x + rect.w - drawRightMargin - x);
					const double uvWidth = width / nineSliceScale.x;
					bottomRegion.resized(uvWidth, bottomMargin).resized(width, drawBottomMargin).draw(x, rect.y + rect.h - drawBottomMargin, color);
				}
			}
		}
		else
		{
			// 伸縮描画
			texture(srcBottom).resized(centerDrawWidth, drawBottomMargin).draw(rect.x + drawLeftMargin, rect.y + rect.h - drawBottomMargin, color);
		}
		
		// 左辺を描画
		if (leftTiled)
		{
			// タイル描画
			const TextureRegion leftRegion = texture(srcLeft);
			const double tileHeight = centerHeight * nineSliceScale.y;
			
			// tileHeightが0以下の場合はスキップ
			if (tileHeight > 0)
			{
				for (double y = rect.y + drawTopMargin; y < rect.y + rect.h - drawBottomMargin; y += tileHeight)
				{
					const double height = Min(tileHeight, rect.y + rect.h - drawBottomMargin - y);
					const double uvHeight = height / nineSliceScale.y;
					leftRegion.resized(leftMargin, uvHeight).resized(drawLeftMargin, height).draw(rect.x, y, color);
				}
			}
		}
		else
		{
			// 伸縮描画
			texture(srcLeft).resized(drawLeftMargin, centerDrawHeight).draw(rect.x, rect.y + drawTopMargin, color);
		}
		
		// 右辺を描画
		if (rightTiled)
		{
			// タイル描画
			const TextureRegion rightRegion = texture(srcRight);
			const double tileHeight = centerHeight * nineSliceScale.y;
			
			// tileHeightが0以下の場合はスキップ
			if (tileHeight > 0)
			{
				for (double y = rect.y + drawTopMargin; y < rect.y + rect.h - drawBottomMargin; y += tileHeight)
				{
					const double height = Min(tileHeight, rect.y + rect.h - drawBottomMargin - y);
					const double uvHeight = height / nineSliceScale.y;
					rightRegion.resized(rightMargin, uvHeight).resized(drawRightMargin, height).draw(rect.x + rect.w - drawRightMargin, y, color);
				}
			}
		}
		else
		{
			// 伸縮描画
			texture(srcRight).resized(drawRightMargin, centerDrawHeight).draw(rect.x + rect.w - drawRightMargin, rect.y + drawTopMargin, color);
		}
		
		// 中央を描画
		if (centerTiled)
		{
			// タイル描画
			const TextureRegion centerRegion = texture(srcCenter);
			const double tileWidth = centerWidth * nineSliceScale.x;
			const double tileHeight = centerHeight * nineSliceScale.y;
			
			// tileWidthまたはtileHeightが0以下の場合はスキップ
			if (tileWidth > 0 && tileHeight > 0)
			{
				for (double y = rect.y + drawTopMargin; y < rect.y + rect.h - drawBottomMargin; y += tileHeight)
				{
					const double height = Min(tileHeight, rect.y + rect.h - drawBottomMargin - y);
					const double uvHeight = height / nineSliceScale.y;
					
					for (double x = rect.x + drawLeftMargin; x < rect.x + rect.w - drawRightMargin; x += tileWidth)
					{
						const double width = Min(tileWidth, rect.x + rect.w - drawRightMargin - x);
						const double uvWidth = width / nineSliceScale.x;
						centerRegion.resized(uvWidth, uvHeight).resized(width, height).draw(x, y, color);
					}
				}
			}
		}
		else
		{
			// 伸縮描画
			texture(srcCenter).resized(centerDrawWidth, centerDrawHeight).draw(rect.x + drawLeftMargin, rect.y + drawTopMargin, color);
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
			texture = GetTexture(textureFilePath, textureAssetName);
		}
		
		const RectF rect = node.regionRect();
		const ColorF& color = m_color.value();
		const ColorF& addColorValue = m_addColor.value();
		const BlendMode blendModeValue = m_blendMode.value();
		
		// ブレンドモードの設定
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
		
		if (m_nineSliceEnabled.value())
		{
			drawNineSlice(texture, rect, color);
		}
		else if (m_preserveAspect.value())
		{
			texture.fitted(rect.size).drawAt(rect.center(), color);
		}
		else
		{
			texture.resized(rect.size).draw(rect.pos, color);
		}
	}
}
