#pragma once
#include <Siv3D.hpp>

namespace noco
{
	struct AnchorConstraint
	{
		Vec2 anchorMin = Anchor::MiddleCenter;
		Vec2 anchorMax = Anchor::MiddleCenter;
		Vec2 posDelta = Vec2::Zero();
		Vec2 sizeDelta = Vec2::Zero();
		Vec2 sizeDeltaPivot = Anchor::MiddleCenter;

		/* NonSerialized */ bool isCustomAnchorInEditor = false;

		[[nodiscard]]
		RectF applyConstraint(const RectF& parentRect, const Vec2&) const
		{
			// AnchorConstraintはオフセットの影響を受けないため、第2引数は無視
			const Vec2 size{ parentRect.size * (anchorMax - anchorMin) + sizeDelta };
			const Vec2 position{ parentRect.pos + parentRect.size * anchorMin + posDelta - sizeDelta * sizeDeltaPivot };
			return RectF{ position, size };
		}

		[[nodiscard]]
		JSON toJSON() const
		{
			return JSON
			{
				{ U"type", U"AnchorConstraint" },
				{ U"anchorMin", anchorMin },
				{ U"anchorMax", anchorMax },
				{ U"posDelta", posDelta },
				{ U"sizeDelta", sizeDelta },
				{ U"sizeDeltaPivot", sizeDeltaPivot },
			};
		}

		[[nodiscard]]
		static AnchorConstraint FromJSON(const JSON& json)
		{
			return AnchorConstraint
			{
				.anchorMin = GetFromJSONOr(json, U"anchorMin", Anchor::MiddleCenter),
				.anchorMax = GetFromJSONOr(json, U"anchorMax", Anchor::MiddleCenter),
				.posDelta = GetFromJSONOr(json, U"posDelta", Vec2::Zero()),
				.sizeDelta = GetFromJSONOr(json, U"sizeDelta", Vec2::Zero()),
				.sizeDeltaPivot = GetFromJSONOr(json, U"sizeDeltaPivot", Anchor::MiddleCenter),
			};
		}

		[[nodiscard]]
		bool operator==(const AnchorConstraint& other) const = default;
	};
}
