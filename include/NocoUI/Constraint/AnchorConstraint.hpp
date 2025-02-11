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
		Vec2 pivot = Anchor::MiddleCenter;

		/* NonSerialized */ bool isCustomAnchorInEditor = false;

		[[nodiscard]]
		RectF applyConstraint(const RectF& parentRect, const Vec2&) const
		{
			// AnchorConstraintはオフセットの影響を受けないため、第2引数は無視
			const Vec2 size{ parentRect.size * (anchorMax - anchorMin) + sizeDelta };
			const Vec2 position{ parentRect.pos + parentRect.size * anchorMin + posDelta - sizeDelta * pivot };
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
				{ U"pivot", pivot },
			};
		}

		[[nodiscard]]
		static AnchorConstraint FromJSON(const JSON& json)
		{
			return AnchorConstraint
			{
				.anchorMin = json[U"anchorMin"].getOr<Vec2>(Anchor::MiddleCenter),
				.anchorMax = json[U"anchorMax"].getOr<Vec2>(Anchor::MiddleCenter),
				.posDelta = json[U"posDelta"].getOr<Vec2>(Vec2::Zero()),
				.sizeDelta = json[U"sizeDelta"].getOr<Vec2>(Vec2::Zero()),
				.pivot = json[U"pivot"].getOr<Vec2>(Anchor::MiddleCenter),
			};
		}

		[[nodiscard]]
		bool operator==(const AnchorConstraint& other) const = default;
	};
}
