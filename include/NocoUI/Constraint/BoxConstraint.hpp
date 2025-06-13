#pragma once
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
		Optional<double> maxWidth = none;
		Optional<double> maxHeight = none;

		[[nodiscard]]
		RectF applyConstraint(const RectF& parentRect, const Vec2& offset) const
		{
			Vec2 size{ parentRect.size * sizeRatio + sizeDelta };
			
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
			BoxConstraint constraint
			{
				.sizeRatio = GetFromJSONOr(json, U"sizeRatio", Vec2::Zero()),
				.sizeDelta = GetFromJSONOr(json, U"sizeDelta", Vec2::Zero()),
				.flexibleWeight = Max(GetFromJSONOr(json, U"flexibleWeight", 0.0), 0.0),
				.margin = GetFromJSONOr(json, U"margin", LRTB::Zero()),
			};
			
			if (json.hasElement(U"maxWidth"))
			{
				constraint.maxWidth = json[U"maxWidth"].get<double>();
			}
			if (json.hasElement(U"maxHeight"))
			{
				constraint.maxHeight = json[U"maxHeight"].get<double>();
			}
			
			return constraint;
		}

		[[nodiscard]]
		bool operator==(const BoxConstraint& other) const = default;
	};
}
