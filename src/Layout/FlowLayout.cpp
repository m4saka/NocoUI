#include "NocoUI/Layout/FlowLayout.hpp"
#include "NocoUI/Node.hpp"

namespace noco
{
	JSON FlowLayout::toJSON() const
	{
		return JSON
		{
			{ U"type", U"FlowLayout" },
			{ U"padding", padding.toJSON() },
		};
	}

	FlowLayout FlowLayout::FromJSON(const JSON& json)
	{
		return FlowLayout
		{
			.padding = json.contains(U"padding") ? LRTB::fromJSON(json[U"padding"]) : LRTB::Zero(),
		};
	}

	FlowLayout::MeasureInfo FlowLayout::measure(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children) const
	{
		// TODO: 都度生成しないよう外部に持ちたい
		MeasureInfo measureInfo;
		measureInfo.lines.emplace_back(); // 最初の行
		measureInfo.measuredChildren.reserve(children.size());

		// 事前計測
		double currentX = 0.0;
		double currentLineMaxHeight = 0.0;
		const double availableWidth = parentRect.w - (padding.left + padding.right);
		for (size_t i = 0; i < children.size(); ++i)
		{
			const auto& child = children[i];
			if (!child->activeSelf()) // 親の影響を受けないようactiveSelfを使う
			{
				measureInfo.measuredChildren.emplace_back();
				continue;
			}

			if (const auto pBoxConstraint = std::get_if<BoxConstraint>(&child->constraint()))
			{
				const RectF measuredRect = pBoxConstraint->applyConstraint(
					RectF{ 0, 0, parentRect.w - (padding.left + padding.right), parentRect.h - (padding.top + padding.bottom) }, // 計測用に親サイズだけ渡す
					Vec2::Zero());
				measureInfo.measuredChildren.push_back(
					MeasureInfo::MeasuredChild
					{
						.size = measuredRect.size,
						.margin = pBoxConstraint->margin,
					});

				const double childW = measuredRect.w + pBoxConstraint->margin.left + pBoxConstraint->margin.right;
				const double childH = measuredRect.h + pBoxConstraint->margin.top + pBoxConstraint->margin.bottom;

				// 行がはみ出す場合は改行
				if (measureInfo.lines.back().boxConstraintChildExists &&
					currentX + childW > availableWidth)
				{
					// 行の最大の高さを記録
					measureInfo.lines.back().maxHeight = currentLineMaxHeight;

					// 新しい行へ
					measureInfo.lines.emplace_back();
					currentX = 0.0;
					currentLineMaxHeight = 0.0;
				}

				measureInfo.lines.back().childIndices.push_back(i);
				measureInfo.lines.back().boxConstraintChildExists = true;
				currentX += childW;

				if (childH > currentLineMaxHeight)
				{
					currentLineMaxHeight = childH;
				}
			}
			else
			{
				// BoxConstraint以外はオフセットに関与しないので、計測結果は空にする
				measureInfo.measuredChildren.emplace_back();
				measureInfo.lines.back().childIndices.push_back(i);
			}
		}
		if (!measureInfo.lines.empty())
		{
			measureInfo.lines.back().maxHeight = currentLineMaxHeight;
		}

		return measureInfo;
	}

	SizeF FlowLayout::fittingSizeToChildren(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children) const
	{
		const auto measureInfo = measure(parentRect, children);

		double maxWidth = 0.0;
		double totalHeight = padding.top + padding.bottom;
		for (const auto& line : measureInfo.lines)
		{
			double lineWidth = padding.left + padding.right;
			for (size_t index : line.childIndices)
			{
				const auto& measuredChild = measureInfo.measuredChildren[index];
				lineWidth += measuredChild.size.x + measuredChild.margin.left + measuredChild.margin.right;
			}
			if (lineWidth > maxWidth)
			{
				maxWidth = lineWidth;
			}
			totalHeight += line.maxHeight;
		}
		return { maxWidth, totalHeight };
	}

	void FlowLayout::setBoxConstraintToFitToChildren(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children, Node& node, FitTarget fitTarget, RefreshesLayoutYN refreshesLayout) const
	{
		const auto [maxWidth, totalHeight] = fittingSizeToChildren(parentRect, children);
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
					.sizeDelta = Vec2{ fitsWidth ? maxWidth : node.layoutAppliedRect().w, fitsHeight ? totalHeight : node.layoutAppliedRect().h },
				});
		}

		if (refreshesLayout)
		{
			node.refreshContainedCanvasLayout();
		}
	}
}
