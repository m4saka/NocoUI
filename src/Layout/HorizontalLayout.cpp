#include "NocoUI/Layout/HorizontalLayout.hpp"
#include "NocoUI/Node.hpp"

namespace noco
{
	thread_local Array<SizeF> HorizontalLayout::t_tempSizes;
	thread_local Array<LRTB> HorizontalLayout::t_tempMargins;

	JSON HorizontalLayout::toJSON() const
	{
		return JSON
		{
			{ U"type", U"HorizontalLayout" },
			{ U"padding", padding.toJSON() },
			{ U"spacing", spacing },
			{ U"horizontalAlign", EnumToString(horizontalAlign) },
			{ U"verticalAlign", EnumToString(verticalAlign) },
		};
	}

	HorizontalLayout HorizontalLayout::FromJSON(const JSON& json)
	{
		HorizontalLayout layout;
		layout.padding = GetFromJSONOr(json, U"padding", LRTB::Zero());
		layout.spacing = GetFromJSONOr(json, U"spacing", 0.0);
		layout.horizontalAlign = GetFromJSONOr(json, U"horizontalAlign", HorizontalAlign::Left);
		layout.verticalAlign = GetFromJSONOr(json, U"verticalAlign", VerticalAlign::Middle);
		return layout;
	}

	SizeF HorizontalLayout::getFittingSizeToChildren(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children) const
	{
		double totalWidth = padding.totalWidth();
		double maxHeight = 0.0;
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
					RectF{ 0, 0, parentRect.w, parentRect.h }, // サイズが分かれば良いので親のサイズだけ渡す
					Vec2::Zero());
				const double childW = measuredRect.w + pInlineRegion->margin.totalWidth();
				const double childH = measuredRect.h + pInlineRegion->margin.totalHeight();
				if (!isFirstInlineRegionChild)
				{
					totalWidth += spacing;
				}
				totalWidth += childW;
				maxHeight = Max(maxHeight, childH);
				isFirstInlineRegionChild = false;
			}
		}
		return { totalWidth, maxHeight + padding.totalHeight() };
	}

	void HorizontalLayout::setInlineRegionToFitToChildren(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children, Node& node, FitTarget fitTarget) const
	{
		const auto [totalWidth, maxHeight] = getFittingSizeToChildren(parentRect, children);
		const bool fitsWidth = fitTarget == FitTarget::WidthOnly || fitTarget == FitTarget::Both;
		const bool fitsHeight = fitTarget == FitTarget::HeightOnly || fitTarget == FitTarget::Both;
		if (const auto pInlineRegion = node.inlineRegion())
		{
			node.setRegion(
				InlineRegion
				{
					.sizeRatio = Vec2{ fitsWidth ? 0.0 : pInlineRegion->sizeRatio.x, fitsHeight ? 0.0 : pInlineRegion->sizeRatio.y },
					.sizeDelta = Vec2{ fitsWidth ? totalWidth : pInlineRegion->sizeDelta.x, fitsHeight ? maxHeight : pInlineRegion->sizeDelta.y },
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
					.sizeDelta = Vec2{ fitsWidth ? totalWidth : node.regionRect().w, fitsHeight ? maxHeight : node.regionRect().h },
					.maxWidth = none,
					.maxHeight = none,
				});
		}

		node.markLayoutAsDirty();
	}
}
