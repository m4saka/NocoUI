#include "NocoUI/Component/Sprite.hpp"
#include "NocoUI/Node.hpp"
#include "NocoUI/Asset.hpp"

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
			if (!textureAssetName.empty() && TextureAsset::IsRegistered(textureAssetName))
			{
				return TextureAsset(textureAssetName);
			}
			else if (!textureFilePath.empty())
			{
				return noco::Asset::GetOrLoadTexture(textureFilePath);
			}
			else
			{
				return Texture{};
			}
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
		
		const RectF& rect = node.rect();
		const ColorF& color = m_color.value();
		
		// 9スライスが有効な場合
		if (m_nineSliceEnabled.value())
		{
			const LRTB& margin = m_nineSliceMargin.value();
			// LRTB型からRect型に変換（素材側の座標スケールでの指定）
			// NinePatchのRectは中央の伸縮可能領域を指定するため、マージンから計算
			const Size textureSize = texture.size();
			
			// マージンの妥当性チェック
			const int32 centerWidth = textureSize.x - static_cast<int32>(margin.left + margin.right);
			const int32 centerHeight = textureSize.y - static_cast<int32>(margin.top + margin.bottom);
			
			// 中央領域のサイズが0以下の場合は通常描画にフォールバック
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
			
			const Rect centerRect{
				static_cast<int32>(margin.left),
				static_cast<int32>(margin.top),
				centerWidth,
				centerHeight
			};
			const NinePatch ninePatch{ texture, centerRect };
			
			// タイル表示の設定に基づいてスタイルを決定
			const bool centerTiled = m_nineSliceCenterTiled.value();
			const bool topTiled = m_nineSliceTopTiled.value();
			const bool bottomTiled = m_nineSliceBottomTiled.value();
			const bool leftTiled = m_nineSliceLeftTiled.value();
			const bool rightTiled = m_nineSliceRightTiled.value();
			
			// Siv3DのNinePatch::Styleに対応するパターンを判定
			if (centerTiled && topTiled && bottomTiled && leftTiled && rightTiled)
			{
				// すべての領域をタイル表示
				ninePatch.draw(rect, NinePatch::Style::TileAll(), color);
			}
			else if (!centerTiled && topTiled && bottomTiled && leftTiled && rightTiled)
			{
				// 4辺すべてタイル表示（中央は伸縮）
				ninePatch.draw(rect, NinePatch::Style::Tile4(), color);
			}
			else if (centerTiled && !topTiled && !bottomTiled && !leftTiled && !rightTiled)
			{
				// 中央のみタイル表示
				ninePatch.draw(rect, NinePatch::Style::TileCenter(), color);
			}
			else if (!centerTiled && topTiled && bottomTiled && !leftTiled && !rightTiled)
			{
				// 上下のみタイル表示
				ninePatch.draw(rect, NinePatch::Style::TileTopBottom(), color);
			}
			else if (!centerTiled && !topTiled && !bottomTiled && leftTiled && rightTiled)
			{
				// 左右のみタイル表示
				ninePatch.draw(rect, NinePatch::Style::TileLeftRight(), color);
			}
			else
			{
				// カスタムスタイルを作成
				NinePatch::Style customStyle;
				customStyle.tileCenter = centerTiled;
				customStyle.tileTopBottom = topTiled || bottomTiled;
				customStyle.tileLeftRight = leftTiled || rightTiled;
				customStyle.fallbackToSimple = true;
				ninePatch.draw(rect, customStyle, color);
			}
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
