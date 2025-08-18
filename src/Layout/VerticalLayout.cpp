#include "NocoUI/Layout/VerticalLayout.hpp"
#include "NocoUI/Node.hpp"

namespace noco
{
	thread_local Array<SizeF> VerticalLayout::t_tempSizes;
	thread_local Array<LRTB> VerticalLayout::t_tempMargins;

	JSON VerticalLayout::toJSON() const
	{
		return JSON
		{
			{ U"type", U"VerticalLayout" },
			{ U"padding", padding.toJSON() },
			{ U"spacing", spacing },
			{ U"horizontalAlign", EnumToString(horizontalAlign) },
			{ U"verticalAlign", EnumToString(verticalAlign) },
		};
	}

	VerticalLayout VerticalLayout::FromJSON(const JSON& json)
	{
		VerticalLayout layout;
		layout.padding = GetFromJSONOr(json, U"padding", LRTB::Zero());
		layout.spacing = GetFromJSONOr(json, U"spacing", 0.0);
		layout.horizontalAlign = GetFromJSONOr(json, U"horizontalAlign", HorizontalAlign::Center);
		layout.verticalAlign = GetFromJSONOr(json, U"verticalAlign", VerticalAlign::Top);
		return layout;
	}

	SizeF VerticalLayout::getFittingSizeToChildren(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children) const
	{
		double totalHeight = padding.top + padding.bottom;
		double maxWidth = 0.0;
		bool isFirstInlineRegionChild = true;
		for (const auto& child : children)
		{
			if (!child->activeSelf()) // 親の影響を受けないようactiveSelfを使う
			{
				continue;
			}
			if (const auto pInlineRegion = child->inlineRegion())
			{
				const RectF measuredRect = pInlineRegion->applyRegion(
					RectF{ 0, 0, parentRect.w - (padding.left + padding.right), parentRect.h - (padding.top + padding.bottom) }, // 計測用に親サイズだけ渡す
					Vec2::Zero());
				const double childW = measuredRect.w + pInlineRegion->margin.left + pInlineRegion->margin.right;
				const double childH = measuredRect.h + pInlineRegion->margin.top + pInlineRegion->margin.bottom;
				if (!isFirstInlineRegionChild)
				{
					totalHeight += spacing;
				}
				totalHeight += childH;
				maxWidth = Max(maxWidth, childW);
				isFirstInlineRegionChild = false;
			}
		}
		return { maxWidth + padding.left + padding.right, totalHeight };
	}

	void VerticalLayout::setInlineRegionToFitToChildren(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children, Node& node, FitTarget fitTarget, RefreshesLayoutYN refreshesLayout) const
	{
		const auto [maxWidth, totalHeight] = getFittingSizeToChildren(parentRect, children);
		const bool fitsWidth = fitTarget == FitTarget::WidthOnly || fitTarget == FitTarget::Both;
		const bool fitsHeight = fitTarget == FitTarget::HeightOnly || fitTarget == FitTarget::Both;
		if (const auto pInlineRegion = node.inlineRegion())
		{
			node.setRegion(
				InlineRegion
				{
					.sizeRatio = Vec2{ fitsWidth ? 0.0 : pInlineRegion->sizeRatio.x, fitsHeight ? 0.0 : pInlineRegion->sizeRatio.y },
					.sizeDelta = Vec2{ fitsWidth ? maxWidth : pInlineRegion->sizeDelta.x, fitsHeight ? totalHeight : pInlineRegion->sizeDelta.y },
					.margin = pInlineRegion->margin,
					.maxWidth = pInlineRegion->maxWidth,
					.maxHeight = pInlineRegion->maxHeight,
				});
		}
		else
		{
			node.setRegion(
				InlineRegion
				{
					.sizeDelta = Vec2{ fitsWidth ? maxWidth : node.regionRect().w, fitsHeight ? totalHeight : node.regionRect().h },
					.maxWidth = none,
					.maxHeight = none,
				});
		}

		if (refreshesLayout)
		{
			node.refreshContainedCanvasLayout();
		}
	}
}
