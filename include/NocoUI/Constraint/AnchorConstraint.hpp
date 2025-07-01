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
		Optional<double> minWidth = none;
		Optional<double> minHeight = none;
		Optional<double> maxWidth = none;
		Optional<double> maxHeight = none;

		/* NonSerialized */ bool isCustomAnchorInEditor = false;

		[[nodiscard]]
		RectF applyConstraint(const RectF& parentRect, const Vec2&) const
		{
			// AnchorConstraintはオフセットの影響を受けないため、第2引数は無視
			const Vec2 originalSize{ parentRect.size * (anchorMax - anchorMin) + sizeDelta };
			Vec2 size = originalSize;
			
			// 最小サイズの適用
			if (minWidth)
			{
				size.x = Max(size.x, *minWidth);
			}
			if (minHeight)
			{
				size.y = Max(size.y, *minHeight);
			}
			
			// 最大サイズの適用
			if (maxWidth)
			{
				size.x = Min(size.x, *maxWidth);
			}
			if (maxHeight)
			{
				size.y = Min(size.y, *maxHeight);
			}
			
			// サイズの差分を計算（min/maxサイズ適用による変化分）
			const Vec2 sizeDiff = originalSize - size;
			
			// 位置計算（sizeDeltaPivotを考慮して、サイズ差分を反映）
			const Vec2 position{ parentRect.pos + parentRect.size * anchorMin + posDelta - sizeDelta * sizeDeltaPivot + sizeDiff * sizeDeltaPivot };
			return RectF{ position, size };
		}

		[[nodiscard]]
		JSON toJSON() const
		{
			JSON json
			{
				{ U"type", U"AnchorConstraint" },
				{ U"anchorMin", anchorMin },
				{ U"anchorMax", anchorMax },
				{ U"posDelta", posDelta },
				{ U"sizeDelta", sizeDelta },
				{ U"sizeDeltaPivot", sizeDeltaPivot },
			};
			
			if (minWidth)
			{
				json[U"minWidth"] = *minWidth;
			}
			if (minHeight)
			{
				json[U"minHeight"] = *minHeight;
			}
			if (maxWidth)
			{
				json[U"maxWidth"] = *maxWidth;
			}
			if (maxHeight)
			{
				json[U"maxHeight"] = *maxHeight;
			}
			
			return json;
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
				.minWidth = GetFromJSONOpt<double>(json, U"minWidth"),
				.minHeight = GetFromJSONOpt<double>(json, U"minHeight"),
				.maxWidth = GetFromJSONOpt<double>(json, U"maxWidth"),
				.maxHeight = GetFromJSONOpt<double>(json, U"maxHeight"),
			};
		}

		[[nodiscard]]
		bool operator==(const AnchorConstraint& other) const = default;
	};
}
