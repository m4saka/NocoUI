﻿#pragma once
#include <Siv3D.hpp>
#include "../LRTB.hpp"

namespace noco
{
	struct BoxConstraint
	{
		Vec2 sizeRatio = Vec2::Zero();
		Vec2 sizeDelta = Vec2::Zero();
		double flexibleWeight = 0.0;
		LRTB margin = LRTB::Zero();
		Optional<double> minWidth = none;
		Optional<double> minHeight = none;
		Optional<double> maxWidth = none;
		Optional<double> maxHeight = none;

		[[nodiscard]]
		RectF applyConstraint(const RectF& parentRect, const Vec2& offset) const
		{
			Vec2 size{ parentRect.size * sizeRatio + sizeDelta };
			
			// 最小サイズ制限を適用
			if (minWidth.has_value())
			{
				size.x = Max(size.x, *minWidth);
			}
			if (minHeight.has_value())
			{
				size.y = Max(size.y, *minHeight);
			}
			
			// 最大サイズ制限を適用
			if (maxWidth.has_value())
			{
				size.x = Min(size.x, *maxWidth);
			}
			if (maxHeight.has_value())
			{
				size.y = Min(size.y, *maxHeight);
			}
			
			const Vec2 position{ parentRect.pos + offset + Vec2{ margin.left, margin.top } };
			return RectF{ position, size };
		}

		[[nodiscard]]
		JSON toJSON() const
		{
			JSON json
			{
				{ U"type", U"BoxConstraint" },
				{ U"sizeRatio", sizeRatio },
				{ U"sizeDelta", sizeDelta },
				{ U"flexibleWeight", flexibleWeight },
				{ U"margin", margin.toJSON() },
			};
			
			if (minWidth.has_value())
			{
				json[U"minWidth"] = *minWidth;
			}
			if (minHeight.has_value())
			{
				json[U"minHeight"] = *minHeight;
			}
			if (maxWidth.has_value())
			{
				json[U"maxWidth"] = *maxWidth;
			}
			if (maxHeight.has_value())
			{
				json[U"maxHeight"] = *maxHeight;
			}
			
			return json;
		}

		[[nodiscard]]
				static BoxConstraint FromJSON(const JSON& json)
		{
			return BoxConstraint
			{
				.sizeRatio = GetFromJSONOr(json, U"sizeRatio", Vec2::Zero()),
				.sizeDelta = GetFromJSONOr(json, U"sizeDelta", Vec2::Zero()),
				.flexibleWeight = Max(GetFromJSONOr(json, U"flexibleWeight", 0.0), 0.0),
				.margin = GetFromJSONOr(json, U"margin", LRTB::Zero()),
				.minWidth = GetFromJSONOpt<double>(json, U"minWidth"),
				.minHeight = GetFromJSONOpt<double>(json, U"minHeight"),
				.maxWidth = GetFromJSONOpt<double>(json, U"maxWidth"),
				.maxHeight = GetFromJSONOpt<double>(json, U"maxHeight"),
			};
		}

		[[nodiscard]]
		bool operator==(const BoxConstraint& other) const = default;
	};
}
