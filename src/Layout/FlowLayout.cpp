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
			{ U"spacing", spacing },
			{ U"horizontalAlign", EnumToString(horizontalAlign) },
			{ U"verticalAlign", EnumToString(verticalAlign) },
		};
	}

	FlowLayout FlowLayout::FromJSON(const JSON& json)
	{
		return FlowLayout
		{
			.padding = GetFromJSONOr(json, U"padding", LRTB::Zero()),
			.spacing = GetFromJSONOr(json, U"spacing", Vec2::Zero()),
			.horizontalAlign = GetFromJSONOr(json, U"horizontalAlign", HorizontalAlign::Left),
			.verticalAlign = GetFromJSONOr(json, U"verticalAlign", VerticalAlign::Top),
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
		double currentLineTotalFlexibleWeight = 0.0;
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

				// 行の最初の要素でない場合はspacing.xの余白を追加
				double childWWithSpacing = childW;
				if (measureInfo.lines.back().boxConstraintChildExists && !measureInfo.lines.back().childIndices.empty())
				{
					childWWithSpacing += spacing.x;
				}

				// 行がはみ出す場合は改行
				if (measureInfo.lines.back().boxConstraintChildExists &&
					currentX + childWWithSpacing > availableWidth)
				{
					// 行ごとの幅・高さを記録
					auto& lastLine = measureInfo.lines.back();
					lastLine.totalWidth = currentX;
					lastLine.maxHeight = currentLineMaxHeight;
					lastLine.totalFlexibleWeight = currentLineTotalFlexibleWeight;

					// 新しい行へ
					measureInfo.lines.emplace_back();
					currentX = 0.0;
					currentLineMaxHeight = 0.0;
					currentLineTotalFlexibleWeight = 0.0;
				}

				// 行の最初の要素でない場合はspacing.xの余白を追加
				if (measureInfo.lines.back().boxConstraintChildExists && !measureInfo.lines.back().childIndices.empty())
				{
					currentX += spacing.x;
				}

				measureInfo.lines.back().childIndices.push_back(i);
				measureInfo.lines.back().boxConstraintChildExists = true;
				currentX += childW;
				currentLineTotalFlexibleWeight += Max(pBoxConstraint->flexibleWeight, 0.0);

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
			// 最後の行の幅・高さを記録
			auto& lastLine = measureInfo.lines.back();
			lastLine.totalWidth = currentX;
			lastLine.maxHeight = currentLineMaxHeight;
			lastLine.totalFlexibleWeight = currentLineTotalFlexibleWeight;
		}

		// flexibleWeightが設定されている場合は残りの幅を分配
		for (auto& line : measureInfo.lines)
		{
			if (!line.boxConstraintChildExists || line.totalFlexibleWeight <= 0.0)
			{
				// flexibleWeight不使用の行はスキップ
				continue;
			}

			const double remainingWidth = availableWidth - line.totalWidth;
			if (remainingWidth <= 0.0)
			{
				continue;
			}

			for (const size_t index : line.childIndices)
			{
				if (index >= children.size() || index >= measureInfo.measuredChildren.size())
				{
					continue;
				}
				const auto& child = children[index];
				if (!child->activeSelf()) // 親の影響を受けないようactiveSelfを使う
				{
					continue;
				}
				if (const auto pBoxConstraint = std::get_if<BoxConstraint>(&child->constraint()))
				{
					if (pBoxConstraint->flexibleWeight <= 0.0)
					{
						continue;
					}
					measureInfo.measuredChildren[index].size.x += remainingWidth * pBoxConstraint->flexibleWeight / line.totalFlexibleWeight;
				}
			}
			line.totalWidth = availableWidth;
		}

		return measureInfo;
	}

	SizeF FlowLayout::getFittingSizeToChildren(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children) const
	{
		const auto measureInfo = measure(parentRect, children);

		double maxWidth = 0.0;
		double totalHeight = padding.top + padding.bottom;
		for (size_t lineIndex = 0; lineIndex < measureInfo.lines.size(); ++lineIndex)
		{
			const auto& line = measureInfo.lines[lineIndex];
			double lineWidth = padding.left + padding.right;
			for (size_t i = 0; i < line.childIndices.size(); ++i)
			{
				const size_t index = line.childIndices[i];
				if (index >= measureInfo.measuredChildren.size())
				{
					continue;
				}
				const auto& measuredChild = measureInfo.measuredChildren[index];
				if (i > 0)
				{
					lineWidth += spacing.x;
				}
				lineWidth += measuredChild.size.x + measuredChild.margin.left + measuredChild.margin.right;
			}
			if (lineWidth > maxWidth)
			{
				maxWidth = lineWidth;
			}
			totalHeight += line.maxHeight;
			if (lineIndex + 1 < measureInfo.lines.size())
			{
				totalHeight += spacing.y;
			}
		}
		return { maxWidth, totalHeight };
	}

	void FlowLayout::setBoxConstraintToFitToChildren(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children, Node& node, FitTarget fitTarget, RefreshesLayoutYN refreshesLayout) const
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

	RectF FlowLayout::executeChild(const RectF& parentRect, const std::shared_ptr<Node>& child, const MeasureInfo::MeasuredChild& measuredChild, double offsetY, double lineHeight, double* pOffsetX) const
	{
		if (const auto pBoxConstraint = child->boxConstraint())
		{
			const double w = measuredChild.size.x;
			const double h = measuredChild.size.y;
			const double marginLeft = measuredChild.margin.left;
			const double marginRight = measuredChild.margin.right;
			const double marginTop = measuredChild.margin.top;
			const double marginBottom = measuredChild.margin.bottom;

			const double shiftY = lineHeight - (h + marginTop + marginBottom);
			const Vec2 pos = parentRect.pos + Vec2{ pOffsetX ? *pOffsetX + marginLeft : marginLeft, offsetY + marginTop + shiftY };
			if (pOffsetX)
			{
				*pOffsetX += w + marginLeft + marginRight;
			}
			return RectF{ pos, measuredChild.size };
		}
		else if (const auto pAnchorConstraint = child->anchorConstraint())
		{
			return pAnchorConstraint->applyConstraint(parentRect, Vec2::Zero());
		}
		else
		{
			// TODO: コンパイルエラーにする
			throw Error{ U"FlowLayout::executeChild: Invalid constraint" };
		}
	}
}
