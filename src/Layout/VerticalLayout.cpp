#include "NocoUI/Layout/VerticalLayout.hpp"
#include "NocoUI/Node.hpp"

namespace noco
{
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
		bool isFirstBoxConstraintChild = true;
		for (const auto& child : children)
		{
			if (!child->activeSelf()) // 親の影響を受けないようactiveSelfを使う
			{
				continue;
			}
			if (const auto pBoxConstraint = child->boxConstraint())
			{
				const RectF measuredRect = pBoxConstraint->applyConstraint(
					RectF{ 0, 0, parentRect.w - (padding.left + padding.right), parentRect.h - (padding.top + padding.bottom) }, // 計測用に親サイズだけ渡す
					Vec2::Zero());
				const double childW = measuredRect.w + pBoxConstraint->margin.left + pBoxConstraint->margin.right;
				const double childH = measuredRect.h + pBoxConstraint->margin.top + pBoxConstraint->margin.bottom;
				if (!isFirstBoxConstraintChild)
				{
					totalHeight += spacing;
				}
				totalHeight += childH;
				maxWidth = Max(maxWidth, childW);
				isFirstBoxConstraintChild = false;
			}
		}
		return { maxWidth + padding.left + padding.right, totalHeight };
	}

	void VerticalLayout::setBoxConstraintToFitToChildren(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children, Node& node, FitTarget fitTarget, RefreshesLayoutYN refreshesLayout) const
	{
		const auto [maxWidth, totalHeight] = getFittingSizeToChildren(parentRect, children);
		const bool fitsWidth = fitTarget == FitTarget::WidthOnly || fitTarget == FitTarget::Both;
		const bool fitsHeight = fitTarget == FitTarget::HeightOnly || fitTarget == FitTarget::Both;
		if (const auto pBoxConstraint = node.boxConstraint())
		{
			node.setConstraint(
				BoxConstraint
				{
					.sizeRatio = Vec2{ fitsWidth ? 0.0 : pBoxConstraint->sizeRatio.x, fitsHeight ? 0.0 : pBoxConstraint->sizeRatio.y },
					.sizeDelta = Vec2{ fitsWidth ? maxWidth : pBoxConstraint->sizeDelta.x, fitsHeight ? totalHeight : pBoxConstraint->sizeDelta.y },
					.margin = pBoxConstraint->margin,
					.maxWidth = pBoxConstraint->maxWidth,
					.maxHeight = pBoxConstraint->maxHeight,
				});
		}
		else
		{
			node.setConstraint(
				BoxConstraint
				{
					.sizeDelta = Vec2{ fitsWidth ? maxWidth : node.layoutAppliedRect().w, fitsHeight ? totalHeight : node.layoutAppliedRect().h },
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
