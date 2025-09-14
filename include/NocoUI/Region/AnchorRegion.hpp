#pragma once
#include <Siv3D.hpp>
#include "../Serialization.hpp"

namespace noco
{
	struct AnchorRegion
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
		RectF applyRegion(const RectF& parentRect, const Vec2&) const
		{
			// AnchorRegionはオフセットの影響を受けないため、第2引数は無視
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
				{ U"type", U"AnchorRegion" },
				{ U"anchorMin", ToArrayJSON(anchorMin) },
				{ U"anchorMax", ToArrayJSON(anchorMax) },
				{ U"posDelta", ToArrayJSON(posDelta) },
				{ U"sizeDelta", ToArrayJSON(sizeDelta) },
				{ U"sizeDeltaPivot", ToArrayJSON(sizeDeltaPivot) },
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
		static AnchorRegion FromJSON(const JSON& json)
		{
			return AnchorRegion
			{
				.anchorMin = json.contains(U"anchorMin") ? FromArrayJSON<Vec2>(json[U"anchorMin"], Anchor::MiddleCenter) : Anchor::MiddleCenter,
				.anchorMax = json.contains(U"anchorMax") ? FromArrayJSON<Vec2>(json[U"anchorMax"], Anchor::MiddleCenter) : Anchor::MiddleCenter,
				.posDelta = json.contains(U"posDelta") ? FromArrayJSON<Vec2>(json[U"posDelta"], Vec2::Zero()) : Vec2::Zero(),
				.sizeDelta = json.contains(U"sizeDelta") ? FromArrayJSON<Vec2>(json[U"sizeDelta"], Vec2::Zero()) : Vec2::Zero(),
				.sizeDeltaPivot = json.contains(U"sizeDeltaPivot") ? FromArrayJSON<Vec2>(json[U"sizeDeltaPivot"], Anchor::MiddleCenter) : Anchor::MiddleCenter,
				.minWidth = GetFromJSONOpt<double>(json, U"minWidth"),
				.minHeight = GetFromJSONOpt<double>(json, U"minHeight"),
				.maxWidth = GetFromJSONOpt<double>(json, U"maxWidth"),
				.maxHeight = GetFromJSONOpt<double>(json, U"maxHeight"),
			};
		}

		[[nodiscard]]
		bool operator==(const AnchorRegion& other) const = default;
	};
}
