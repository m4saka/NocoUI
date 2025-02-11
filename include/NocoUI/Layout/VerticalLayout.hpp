#pragma once
#include <Siv3D.hpp>
#include "../Serialization.hpp"
#include "../LRTB.hpp"
#include "../YN.hpp"
#include "../Enums.hpp"

namespace noco
{
	class Node;

	struct VerticalLayout
	{
		LRTB padding = LRTB::Zero();
		HorizontalAlign horizontalAlign = HorizontalAlign::Center;

		[[nodiscard]]
		JSON toJSON() const;

		[[nodiscard]]
		static VerticalLayout FromJSON(const JSON& json);

		template <class Fty>
		void execute(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children, Fty fnSetRect) const
			requires std::invocable<Fty, const std::shared_ptr<Node>&, const RectF&>;

		[[nodiscard]]
		SizeF fittingSizeToChildren(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children) const;

		void setBoxConstraintToFitToChildren(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children, Node& node, FitTarget fitTarget, RefreshesLayoutYN refreshesLayout) const;
	};
}
