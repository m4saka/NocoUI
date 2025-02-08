#pragma once
#include <Siv3D.hpp>
#include "../LRTB.hpp"

namespace noco
{
	struct BoxConstraint
	{
		Vec2 sizeRatio = Vec2::Zero();
		Vec2 sizeDelta = Vec2::Zero();
		LRTB margin = LRTB::Zero();

		[[nodiscard]]
		RectF applyConstraint(const RectF& parentRect, const Vec2& offset) const
		{
			const Vec2 size{ parentRect.size * sizeRatio + sizeDelta };
			const Vec2 position{ parentRect.pos + offset + Vec2{ margin.left, margin.top } };
			return RectF{ position, size };
		}

		[[nodiscard]]
		JSON toJSON() const
		{
			return JSON
			{
				{ U"type", U"BoxConstraint" },
				{ U"sizeRatio", sizeRatio },
				{ U"sizeDelta", sizeDelta },
				{ U"margin", margin.toJSON() },
			};
		}

		[[nodiscard]]
		static BoxConstraint FromJSON(const JSON& json)
		{
			return BoxConstraint
			{
				.sizeRatio = GetFromJSONOr(json, U"sizeRatio", Vec2::Zero()),
				.sizeDelta = GetFromJSONOr(json, U"sizeDelta", Vec2::Zero()),
				.margin = json.contains(U"margin") ? LRTB::fromJSON(json[U"margin"]) : LRTB::Zero(),
			};
		}

		[[nodiscard]]
		bool operator==(const BoxConstraint& other) const = default;
	};
}
