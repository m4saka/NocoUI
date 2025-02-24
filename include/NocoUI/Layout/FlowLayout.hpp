#pragma once
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

	private:
		RectF executeChild(const RectF& parentRect, const std::shared_ptr<Node>& child, const MeasureInfo::MeasuredChild& measuredChild, double offsetY, double lineHeight, double* pOffsetX) const;
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
				const RectF finalRect = executeChild(parentRect, child, measureInfo.measuredChildren[index], offsetY, lineHeight, &offsetX);
				fnSetRect(child, finalRect);
			}
			offsetY += lineHeight;
		}
	}
}
