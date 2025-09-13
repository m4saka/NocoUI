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
			{ U"spacing", ToArrayJSON<Vec2>(spacing) },
			{ U"horizontalAlign", EnumToString(horizontalAlign) },
			{ U"verticalAlign", EnumToString(verticalAlign) },
		};
	}

	FlowLayout FlowLayout::FromJSON(const JSON& json)
	{
		return FlowLayout
		{
			.padding = GetFromJSONOr(json, U"padding", LRTB::Zero()),
			.spacing = json.contains(U"spacing") ? FromArrayJSON<Vec2>(json[U"spacing"]) : Vec2::Zero(),
			.horizontalAlign = GetFromJSONOr(json, U"horizontalAlign", HorizontalAlign::Left),
			.verticalAlign = GetFromJSONOr(json, U"verticalAlign", VerticalAlign::Top),
		};
	}

	void FlowLayout::measure(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children, MeasureInfo* pMeasureInfo) const
	{
		auto& measureInfo = *pMeasureInfo;
		measureInfo.clear();
		measureInfo.reserve(children.size());
		measureInfo.lines.emplace_back(); // 最初の行

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

			if (const auto pInlineRegion = std::get_if<InlineRegion>(&child->region()))
			{
				const RectF measuredRect = pInlineRegion->applyRegion(
					RectF{ 0, 0, parentRect.w - (padding.left + padding.right), parentRect.h - (padding.top + padding.bottom) }, // 計測用に親サイズだけ渡す
					Vec2::Zero());
				measureInfo.measuredChildren.push_back(
					MeasureInfo::MeasuredChild
					{
						.size = measuredRect.size,
						.margin = pInlineRegion->margin,
					});

				const double childW = measuredRect.w + pInlineRegion->margin.left + pInlineRegion->margin.right;
				const double childH = measuredRect.h + pInlineRegion->margin.top + pInlineRegion->margin.bottom;

				// 行の最初の要素でない場合はspacing.xの余白を追加
				double childWWithSpacing = childW;
				if (measureInfo.lines.back().inlineRegionChildExists && !measureInfo.lines.back().childIndices.empty())
				{
					childWWithSpacing += spacing.x;
				}

				// 行がはみ出す場合は改行
				if (measureInfo.lines.back().inlineRegionChildExists &&
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
				if (measureInfo.lines.back().inlineRegionChildExists && !measureInfo.lines.back().childIndices.empty())
				{
					currentX += spacing.x;
				}

				measureInfo.lines.back().childIndices.push_back(i);
				measureInfo.lines.back().inlineRegionChildExists = true;
				currentX += childW;
				currentLineTotalFlexibleWeight += Max(pInlineRegion->flexibleWeight, 0.0);

				if (childH > currentLineMaxHeight)
				{
					currentLineMaxHeight = childH;
				}
			}
			else
			{
				// InlineRegion以外はオフセットに関与しないので、計測結果は空にする
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
			if (!line.inlineRegionChildExists || line.totalFlexibleWeight <= 0.0)
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
				if (const auto pInlineRegion = std::get_if<InlineRegion>(&child->region()))
				{
					if (pInlineRegion->flexibleWeight <= 0.0)
					{
						continue;
					}
					measureInfo.measuredChildren[index].size.x += remainingWidth * pInlineRegion->flexibleWeight / line.totalFlexibleWeight;
				}
			}
			line.totalWidth = availableWidth;
		}

	}

	SizeF FlowLayout::getFittingSizeToChildren(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children) const
	{
		static thread_local MeasureInfo t_measureInfoBuffer;
		measure(parentRect, children, &t_measureInfoBuffer);
		const auto& measureInfo = t_measureInfoBuffer;

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

	void FlowLayout::setInlineRegionToFitToChildren(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children, Node& node, FitTarget fitTarget) const
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

		node.markLayoutAsDirty();
	}

	RectF FlowLayout::executeChild(const RectF& parentRect, const std::shared_ptr<Node>& child, const MeasureInfo::MeasuredChild& measuredChild, double offsetY, double lineHeight, double* pOffsetX) const
	{
		if (const auto pInlineRegion = child->inlineRegion())
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
		else if (const auto pAnchorRegion = child->anchorRegion())
		{
			return pAnchorRegion->applyRegion(parentRect, Vec2::Zero());
		}
		else
		{
			// TODO: コンパイルエラーにする
			throw Error{ U"FlowLayout::executeChild: Invalid region" };
		}
	}
}
