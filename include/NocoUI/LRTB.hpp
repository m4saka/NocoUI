#pragma once
#include <Siv3D.hpp>
#include "Serialization.hpp"

namespace noco
{
	struct LRTB
	{
		double left = 0.0;
		double right = 0.0;
		double top = 0.0;
		double bottom = 0.0;

		[[nodiscard]]
		JSON toJSON() const
		{
			if (left == right && right == top && top == bottom)
			{
				return JSON{ left };
			}
			else
			{
				return JSON
				{
					{ U"type", U"LRTB" },
					{ U"left", left },
					{ U"right", right },
					{ U"top", top },
					{ U"bottom", bottom },
				};
			}
		}

		[[nodiscard]]
		static LRTB fromJSON(const JSON& json)
		{
			if (json.isNumber())
			{
				const double value = json.get<double>();
				return All(value);
			}
			else
			{
				return LRTB
				{
					.left = GetFromJSONOr(json, U"left", 0.0),
					.right = GetFromJSONOr(json, U"right", 0.0),
					.top = GetFromJSONOr(json, U"top", 0.0),
					.bottom = GetFromJSONOr(json, U"bottom", 0.0),
				};
			}
		}

		[[nodiscard]]
		LRTB lerp(const LRTB& other, double rate)
		{
			return LRTB
			{
				.left = Math::Lerp(left, other.left, rate),
				.right = Math::Lerp(right, other.right, rate),
				.top = Math::Lerp(top, other.top, rate),
				.bottom = Math::Lerp(bottom, other.bottom, rate),
			};
		}

		[[nodiscard]]
		static constexpr LRTB Zero()
		{
			return All(0.0);
		}

		[[nodiscard]]
		static constexpr LRTB All(double value)
		{
			return LRTB{ value, value, value, value };
		}

		[[nodiscard]]
		static LRTB SmoothDamp(const LRTB& current, const LRTB& target, LRTB& currentVelocity, double smoothTime, const Optional<double>& maxSpeed = unspecified, double deltaTime = Scene::DeltaTime())
		{
			return LRTB
			{
				.left = Math::SmoothDamp(current.left, target.left, currentVelocity.left, smoothTime, maxSpeed, deltaTime),
				.right = Math::SmoothDamp(current.right, target.right, currentVelocity.right, smoothTime, maxSpeed, deltaTime),
				.top = Math::SmoothDamp(current.top, target.top, currentVelocity.top, smoothTime, maxSpeed, deltaTime),
				.bottom = Math::SmoothDamp(current.bottom, target.bottom, currentVelocity.bottom, smoothTime, maxSpeed, deltaTime),
			};
		}

		friend void Formatter(FormatData& formatData, const LRTB& value)
		{
			formatData.string += U"({}, {}, {}, {})"_fmt(value.left, value.right, value.top, value.bottom);
		}

		template <class CharType>
		friend std::basic_istream<CharType>& operator >>(std::basic_istream<CharType>& input, LRTB& value)
		{
			CharType unused;
			return input >> unused
				>> value.left >> unused
				>> value.right >> unused
				>> value.top >> unused
				>> value.bottom >> unused;
		}

		[[nodiscard]]
		bool operator==(const LRTB& other) const = default;
	};
}
