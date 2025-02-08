#include "NocoUI/Component/RectRenderer.hpp"
#include "NocoUI/Node.hpp"

namespace noco
{
	void RectRenderer::draw(const Node& node) const
	{
		const double effectScaleAvg = (node.effectScale().x + node.effectScale().y) / 2;
		const ColorF& fillColor = m_fillColor.value();
		const ColorF& outlineColor = m_outlineColor.value();
		const double outlineThickness = m_outlineThickness.value() * effectScaleAvg;
		const double cornerRadius = m_cornerRadius.value() * effectScaleAvg;
		const ColorF& shadowColor = m_shadowColor.value();
		const Vec2& shadowOffset = m_shadowOffset.value() * effectScaleAvg;
		const double shadowBlur = m_shadowBlur.value() * effectScaleAvg;
		const double shadowSpread = m_shadowSpread.value() * effectScaleAvg;

		if (cornerRadius == 0.0)
		{
			const RectF rect = node.rect().stretched(-outlineThickness / 2);
			if (shadowColor.a > 0.0)
			{
				rect.drawShadow(shadowOffset, shadowBlur, shadowSpread, shadowColor);
			}
			if (outlineThickness == 0.0)
			{
				rect.draw(fillColor);
			}
			else
			{
				rect.draw(fillColor).drawFrame(outlineThickness, outlineColor);
			}
		}
		else
		{
			const RectF rect = node.rect().stretched(-outlineThickness / 2);
			const RoundRect roundRect = rect.rounded(cornerRadius);
			if (shadowColor.a > 0.0)
			{
				roundRect.drawShadow(shadowOffset, shadowBlur, shadowSpread, shadowColor);
			}
			if (outlineThickness == 0.0)
			{
				roundRect.draw(fillColor);
			}
			else
			{
				roundRect.draw(fillColor).drawFrame(outlineThickness, outlineColor);
			}
		}
	}
}
