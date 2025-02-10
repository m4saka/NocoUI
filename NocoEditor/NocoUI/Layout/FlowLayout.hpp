#pragma once
#include <Siv3D.hpp>
#include "../Serialization.hpp"
#include "../LRTB.hpp"
#include "../YN.hpp"
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
				double maxHeight = 0.0;
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

		[[nodiscard]]
		JSON toJSON() const;

		[[nodiscard]]
		static FlowLayout FromJSON(const JSON& json);

		MeasureInfo measure(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children) const;

		template <class Fty>
		void execute(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children, Fty fnSetRect) const
			requires std::invocable<Fty, const std::shared_ptr<Node>&, const RectF&>;

		[[nodiscard]]
		SizeF fittingSizeToChildren(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children) const;

		void setBoxConstraintToFitToChildren(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children, Node& node, FitTarget fitTarget, RefreshesLayoutYN refreshesLayout) const;
	};

	template <class Fty>
	void FlowLayout::execute(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children, Fty fnSetRect) const
		requires std::invocable<Fty, const std::shared_ptr<Node>&, const RectF&>
	{
		const auto measureInfo = measure(parentRect, children);

		// 実際に配置していく
		double offsetY = 0.0;
		for (auto& line : measureInfo.lines)
		{
			double offsetX = 0.0;
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
					const Vec2 childOffset{ offsetX + marginLeft, offsetY + marginTop + shiftY };
					const RectF parentRectInsidePadding
					{
						parentRect.x + padding.left,
						parentRect.y + padding.top,
						parentRect.w - (padding.left + padding.right),
						parentRect.h - (padding.top + padding.bottom)
					};
					const RectF finalRect = pBoxConstraint->applyConstraint(parentRectInsidePadding, childOffset);
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
