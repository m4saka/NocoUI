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

		Vec2 spacing = Vec2::Zero();

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
}
