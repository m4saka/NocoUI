#include "NocoUI/Layout/FlowLayout.hpp"
#include "NocoUI/Node.hpp"

namespace noco
{
	JSON VerticalLayout::toJSON() const
	{
		return JSON
		{
			{ U"type", U"VerticalLayout" },
			{ U"padding", padding.toJSON() },
			{ U"horizontalAlign", EnumToString(horizontalAlign) },
		};
	}

	VerticalLayout VerticalLayout::FromJSON(const JSON& json)
	{
		VerticalLayout layout;
		layout.padding = json.contains(U"padding") ? LRTB::fromJSON(json[U"padding"]) : LRTB::Zero();
		layout.horizontalAlign = GetFromJSONOr(json, U"horizontalAlign", HorizontalAlign::Left);
		return layout;
	}

	void VerticalLayout::setBoxConstraintToFitToChildren(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children, Node& node, FitTarget fitTarget, RefreshesLayoutYN refreshesLayout) const
	{
		double totalHeight = padding.top + padding.bottom;
		double maxWidth = 0.0;

		for (const auto& child : children)
		{
			if (!child->activeSelf()) // 親の影響を受けないようactiveSelfを使う
			{
				continue;
			}

			if (const auto pBoxConstraint = child->boxConstraint())
			{
				const RectF measuredRect = pBoxConstraint->applyConstraint(
					RectF{ 0, 0, parentRect.w, parentRect.h }, // サイズが分かれば良いので親のサイズだけ渡す
					Vec2::Zero());
				const double childTotalWidth = measuredRect.w + pBoxConstraint->margin.left + pBoxConstraint->margin.right;
				const double childTotalHeight = measuredRect.h + pBoxConstraint->margin.top + pBoxConstraint->margin.bottom;
				totalHeight += childTotalHeight;
				maxWidth = Max(maxWidth, childTotalWidth);
			}
		}

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
				});
		}
		else
		{
			node.setConstraint(
				BoxConstraint
				{
					.sizeDelta = Vec2{ fitsWidth ? maxWidth : node.layoutAppliedRect().x, fitsHeight ? totalHeight : node.layoutAppliedRect().y },
				});
		}

		if (refreshesLayout)
		{
			node.refreshContainedCanvasLayout();
		}
	}
}
