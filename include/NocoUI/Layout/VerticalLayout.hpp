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

	struct VerticalLayout
	{
		LRTB padding = LRTB::Zero();

		double spacing = 0.0;

		HorizontalAlign horizontalAlign = HorizontalAlign::Center;

		VerticalAlign verticalAlign = VerticalAlign::Top;

	private:
		static thread_local Array<SizeF> t_tempSizes;
		static thread_local Array<LRTB> t_tempMargins;

	public:
		[[nodiscard]]
		JSON toJSON() const;

		[[nodiscard]]
		static VerticalLayout FromJSON(const JSON& json);

		template <class Fty>
		void execute(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children, Fty fnSetRect) const
			requires std::invocable<Fty, const std::shared_ptr<Node>&, const RectF&>;

		[[nodiscard]]
		SizeF getFittingSizeToChildren(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children) const;

		void setInlineRegionToFitToChildren(const RectF& parentRect, const Array<std::shared_ptr<Node>>& children, Node& node, FitTarget fitTarget) const;
		
		[[nodiscard]]
		Vec2 scrollOffsetAnchor() const
		{
			return Anchor::FromAlign(horizontalAlign, verticalAlign);
		}
	};
}
