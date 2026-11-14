#include "NocoUI/Component/RectRenderer.hpp"
#include "NocoUI/Node.hpp"

namespace noco
{
	RectRenderer::RectRenderer(
		const PropertyValue<Color>& fillColor,
		const PropertyValue<Color>& outlineColor,
		const PropertyValue<double>& outlineThicknessInner,
		const PropertyValue<double>& outlineThicknessOuter,
		const PropertyValue<double>& cornerRadius,
		const PropertyValue<Color>& shadowColor,
		const PropertyValue<Vec2>& shadowOffset,
		const PropertyValue<double>& shadowBlur,
		const PropertyValue<double>& shadowSpread)
		: SerializableComponentBase{ U"RectRenderer", { &m_fillGradationType, &m_fillColor, &m_fillGradationColor1, &m_fillGradationColor2, &m_blendMode, &m_outlineColor, &m_outlineThicknessInner, &m_outlineThicknessOuter, &m_cornerRadius, &m_shadowColor, &m_shadowOffset, &m_shadowBlur, &m_shadowSpread } }
		, m_fillGradationType{ U"fillGradationType", RectFillGradationType::None }
		, m_fillColor{ U"fillColor", fillColor }
		, m_fillGradationColor1{ U"fillGradationColor1", fillColor }
		, m_fillGradationColor2{ U"fillGradationColor2", fillColor }
		, m_blendMode{ U"blendMode", BlendMode::Normal }
		, m_outlineColor{ U"outlineColor", outlineColor }
		, m_outlineThicknessInner{ U"outlineThicknessInner", outlineThicknessInner }
		, m_outlineThicknessOuter{ U"outlineThicknessOuter", outlineThicknessOuter }
		, m_cornerRadius{ U"cornerRadius", cornerRadius }
		, m_shadowColor{ U"shadowColor", shadowColor }
		, m_shadowOffset{ U"shadowOffset", shadowOffset }
		, m_shadowBlur{ U"shadowBlur", shadowBlur }
		, m_shadowSpread{ U"shadowSpread", shadowSpread }
	{
	}

	void RectRenderer::draw(const Node& node) const
	{
		const RectFillGradationType fillGradationType = m_fillGradationType.value();
		const Color& fillColor = m_fillColor.value();
		const Color& fillGradationColor1 = m_fillGradationColor1.value();
		const Color& fillGradationColor2 = m_fillGradationColor2.value();
		const BlendMode blendModeValue = m_blendMode.value();
		const Color& outlineColor = m_outlineColor.value();
		const double outlineThicknessInner = m_outlineThicknessInner.value();
		const double outlineThicknessOuter = m_outlineThicknessOuter.value();
		const double cornerRadius = m_cornerRadius.value();
		const Color& shadowColor = m_shadowColor.value();
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
			const RectF rect = node.regionRect();
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
			
			if (outlineThicknessInner > 0.0 || outlineThicknessOuter > 0.0)
			{
				rect.drawFrame(outlineThicknessInner, outlineThicknessOuter, outlineColor);
			}
		}
		else
		{
			const RectF rect = node.regionRect();
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
			
			if (outlineThicknessInner > 0.0 || outlineThicknessOuter > 0.0)
			{
				roundRect.drawFrame(outlineThicknessInner, outlineThicknessOuter, outlineColor);
			}
		}
	}
}
