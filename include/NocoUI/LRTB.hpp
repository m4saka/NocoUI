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
			return Array<double>{ left, right, top, bottom };
		}

		[[nodiscard]]
		static LRTB FromJSON(const JSON& json, const LRTB& defaultValue = Zero())
		{
			if (json.isArray() && json.size() == 4)
			{
				return LRTB{
					.left = json[0].getOr<double>(0.0),
					.right = json[1].getOr<double>(0.0),
					.top = json[2].getOr<double>(0.0),
					.bottom = json[3].getOr<double>(0.0),
				};
			}
			else if (json.isString())
			{
				Logger << U"[NocoUI warning] String format LRTB found, returning default value";
				return defaultValue;
			}
			return defaultValue;
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
		double totalWidth() const
		{
			return left + right;
		}

		[[nodiscard]]
		double totalHeight() const
		{
			return top + bottom;
		}

		[[nodiscard]]
		Vec2 totalSize() const
		{
			return Vec2{ left + right, top + bottom };
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
		
		[[nodiscard]]
		LRTB operator+(const LRTB& other) const
		{
			return LRTB
			{
				.left = left + other.left,
				.right = right + other.right,
				.top = top + other.top,
				.bottom = bottom + other.bottom,
			};
		}
		
		[[nodiscard]]
		LRTB operator*(double weight) const
		{
			return LRTB
			{
				.left = left * weight,
				.right = right * weight,
				.top = top * weight,
				.bottom = bottom * weight,
			};
		}
	};

	template<>
	[[nodiscard]]
	inline LRTB GetFromJSONOr<LRTB>(const JSON& json, const String& key, const LRTB& defaultValue)
	{
		if (json.isObject() && json.contains(key))
		{
			return LRTB::FromJSON(json[key], defaultValue);
		}
		return defaultValue;
	}

	template<>
	[[nodiscard]]
	inline Optional<LRTB> GetFromJSONOpt<LRTB>(const JSON& json, const String& key)
	{
		if (json.isObject() && json.contains(key))
		{
			const JSON& valueJson = json[key];
			if (valueJson.isArray() && valueJson.size() == 4)
			{
				return LRTB::FromJSON(valueJson);
			}
		}
		return none;
	}
}
