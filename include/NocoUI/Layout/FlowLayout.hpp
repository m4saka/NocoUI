﻿#pragma once
#include <Siv3D.hpp>
#include "../Serialization.hpp"
#include "../LRTB.hpp"
#include "../YN.hpp"
#include "../Anchor.hpp"
#include "../Enums.hpp"

namespace noco
{
	class Node;

	struct FlowLayout
	{
		struct MeasureInfo
		{
			struct Line
			{
				Array<size_t> childIndices;
				double totalWidth = 0.0;
				double maxHeight = 0.0;
				double totalFlexibleWeight = 0.0;
				bool boxConstraintChildExists = false;
			};
			Array<Line> lines;

			struct MeasuredChild
			{
				SizeF size;
				LRTB margin;
			};
			Array<MeasuredChild> measuredChildren;
		};

		LRTB padding = LRTB::Zero();

		HorizontalAlign horizontalAlign = HorizontalAlign::Left;

		VerticalAlign verticalAlign = VerticalAlign::Top;

		[[nodiscard]]
		JSON toJSON() const;

		[[nodiscard]]
		static FlowLayout FromJSON(const JSON& json);

		MeasureInfo measure(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children) const;

		template <class Fty>
		void execute(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children, Fty fnSetRect) const
			requires std::invocable<Fty, const std::shared_ptr<Node>&, const RectF&>;

		[[nodiscard]]
		SizeF getFittingSizeToChildren(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children) const;

		void setBoxConstraintToFitToChildren(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children, Node& node, FitTarget fitTarget, RefreshesLayoutYN refreshesLayout) const;
		
		[[nodiscard]]
		Vec2 scrollOffsetAnchor() const
		{
			return Anchor::FromAlign(horizontalAlign, verticalAlign);
		}
	};

	template <class Fty>
	void FlowLayout::execute(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children, Fty fnSetRect) const
		requires std::invocable<Fty, const std::shared_ptr<Node>&, const RectF&>
	{
		const auto measureInfo = measure(parentRect, children);
		const double availableWidth = parentRect.w - (padding.left + padding.right);

		// 実際に配置していく
		double offsetY = padding.top;
		if (verticalAlign == VerticalAlign::Middle || verticalAlign == VerticalAlign::Bottom)
		{
			const double totalHeight = std::accumulate(
				measureInfo.lines.begin(),
				measureInfo.lines.end(),
				0.0,
				[](double sum, const auto& line) { return sum + line.maxHeight; });
			const double availableHeight = parentRect.h - (padding.top + padding.bottom);
			if (verticalAlign == VerticalAlign::Middle)
			{
				offsetY += (availableHeight - totalHeight) / 2;
			}
			else if (verticalAlign == VerticalAlign::Bottom)
			{
				offsetY += availableHeight - totalHeight;
			}
		}
		for (auto& line : measureInfo.lines)
		{
			double offsetX = padding.left;
			if (horizontalAlign == HorizontalAlign::Center)
			{
				offsetX += (availableWidth - line.totalWidth) / 2;
			}
			else if (horizontalAlign == HorizontalAlign::Right)
			{
				offsetX += availableWidth - line.totalWidth;
			}
			const double lineHeight = line.maxHeight;
			for (size_t index : line.childIndices)
			{
				const auto& child = children[index];
				if (const auto pBoxConstraint = child->boxConstraint())
				{
					const auto& measuredChild = measureInfo.measuredChildren[index];

					const double w = measuredChild.size.x;
					const double h = measuredChild.size.y;
					const double marginLeft = measuredChild.margin.left;
					const double marginRight = measuredChild.margin.right;
					const double marginTop = measuredChild.margin.top;
					const double marginBottom = measuredChild.margin.bottom;

					const double shiftY = lineHeight - (h + marginTop + marginBottom);
					const Vec2 pos = parentRect.pos + Vec2{ offsetX + marginLeft, offsetY + marginTop + shiftY };
					const RectF finalRect{ pos, measuredChild.size };
					fnSetRect(child, finalRect);

					offsetX += (w + marginLeft + marginRight);
				}
				else if (const auto pAnchorConstraint = child->anchorConstraint())
				{
					const RectF finalRect = pAnchorConstraint->applyConstraint(parentRect, Vec2::Zero());
					fnSetRect(child, finalRect);
				}
			}

			offsetY += lineHeight;
		}
	}
}
