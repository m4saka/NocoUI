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

	struct HorizontalLayout
	{
		LRTB padding = LRTB::Zero();

		VerticalAlign verticalAlign = VerticalAlign::Middle;

		[[nodiscard]]
		JSON toJSON() const;

		[[nodiscard]]
		static HorizontalLayout FromJSON(const JSON& json);

		template <class Fty>
		void execute(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children, Fty fnSetRect) const
			requires std::invocable<Fty, const std::shared_ptr<Node>&, const RectF&>;

		[[nodiscard]]
		SizeF fittingSizeToChildren(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children) const;

		void setBoxConstraintToFitToChildren(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children, Node& node, FitTarget fitTarget, RefreshesLayoutYN refreshesLayout) const;

		[[nodiscard]]
		Vec2 scrollOffsetAnchor() const
		{
			// TODO: 要精査
			return Anchor::TopLeft;
		}
	};
}
