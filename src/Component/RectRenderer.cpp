#include "NocoUI/Component/RectRenderer.hpp"
#include "NocoUI/Node.hpp"

namespace noco
{
	void RectRenderer::draw(const Node& node) const
	{
		const RectFillGradationType fillGradationType = m_fillGradationType.value();
		const ColorF& fillColor = m_fillColor.value();
		const ColorF& fillGradationColor1 = m_fillGradationColor1.value();
		const ColorF& fillGradationColor2 = m_fillGradationColor2.value();
		const BlendMode blendModeValue = m_blendMode.value();
		const ColorF& outlineColor = m_outlineColor.value();
		const double outlineThickness = m_outlineThickness.value();
		const double cornerRadius = m_cornerRadius.value();
		const ColorF& shadowColor = m_shadowColor.value();
		const Vec2& shadowOffset = m_shadowOffset.value();
		const double shadowBlur = m_shadowBlur.value();
		const double shadowSpread = m_shadowSpread.value();

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
			blendState.emplace(BlendState::Multiplicative);
			break;
		default:
			// Normal の場合は何もしない
			break;
		}

		if (cornerRadius == 0.0)
		{
			const RectF rect = node.regionRect().stretched(-outlineThickness / 2);
			if (shadowColor.a > 0.0)
			{
				rect.drawShadow(shadowOffset, shadowBlur, shadowSpread, shadowColor);
			}
			
			if (fillGradationType == RectFillGradationType::TopBottom)
			{
				// 上下グラデーション描画
				rect.draw(Arg::top = fillGradationColor1, Arg::bottom = fillGradationColor2);
			}
			else if (fillGradationType == RectFillGradationType::LeftRight)
			{
				// 左右グラデーション描画
				rect.draw(Arg::left = fillGradationColor1, Arg::right = fillGradationColor2);
			}
			else
			{
				// 単色塗りつぶし
				rect.draw(fillColor);
			}
			
			if (outlineThickness > 0.0)
			{
				rect.drawFrame(outlineThickness, outlineColor);
			}
		}
		else
		{
			const RectF rect = node.regionRect().stretched(-outlineThickness / 2);
			const RoundRect roundRect = rect.rounded(cornerRadius);
			if (shadowColor.a > 0.0)
			{
				roundRect.drawShadow(shadowOffset, shadowBlur, shadowSpread, shadowColor);
			}
			
			if (fillGradationType == RectFillGradationType::TopBottom)
			{
				// 角丸上下グラデーション描画
				roundRect.draw(Arg::top = fillGradationColor1, Arg::bottom = fillGradationColor2);
			}
			else if (fillGradationType == RectFillGradationType::LeftRight)
			{
				// 角丸左右グラデーション描画
				// RoundRectは左右グラデーションをサポートしていないため、回転を使用
				const Vec2 center = rect.center();
				const Transformer2D transformer{ Mat3x2::Rotate(-90_deg, center) };
				const RoundRect rotatedRoundRect = RoundRect{ center.x - rect.h / 2, center.y - rect.w / 2, rect.h, rect.w, cornerRadius };
				rotatedRoundRect.draw(Arg::top = fillGradationColor1, Arg::bottom = fillGradationColor2);
			}
			else
			{
				// 単色塗りつぶし
				roundRect.draw(fillColor);
			}
			
			if (outlineThickness > 0.0)
			{
				roundRect.drawFrame(outlineThickness, outlineColor);
			}
		}
	}
}
