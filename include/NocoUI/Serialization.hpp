#pragma once
#include <Siv3D.hpp>
#include "YN.hpp"
#include "magic_enum.hpp" // https://github.com/Neargye/magic_enum

namespace noco
{
	struct LRTB;
}

namespace noco
{
	constexpr int32 CurrentSerializedVersion = 1;

	template <typename T>
	[[nodiscard]]
	T StringToEnum(StringView value, T defaultValue) requires std::is_enum_v<T>
	{
		const auto u8Value = value.toUTF8();
		if (magic_enum::enum_contains<T>(u8Value))
		{
			return magic_enum::enum_cast<T>(u8Value).value();
		}
		return defaultValue;
	}

	template <typename T>
	[[nodiscard]]
	Optional<T> StringToEnumOpt(StringView value) requires std::is_enum_v<T>
	{
		const auto u8Value = value.toUTF8();
		if (magic_enum::enum_contains<T>(u8Value))
		{
			return magic_enum::enum_cast<T>(u8Value).value();
		}
		return none;
	}

	template <typename T>
	[[nodiscard]]
	String EnumToString(T value) requires std::is_enum_v<T>;

	template <typename T>
	[[nodiscard]]
	T GetFromJSONOr(const JSON& json, const String& key, const T& defaultValue)
	{
		if (json.isObject() && json.contains(key))
		{
			if constexpr (std::is_enum_v<T>)
			{
				return StringToEnum<T>(json[key].getOr<String>(EnumToString(defaultValue)), defaultValue);
			}
			else
			{
				return json[key].getOr<T>(defaultValue);
			}
		}
		return defaultValue;
	}

	template <typename T>
	[[nodiscard]]
	Optional<T> GetFromJSONOpt(const JSON& json, const String& key)
	{
		if (json.isObject() && json.contains(key))
		{
			if constexpr (std::is_enum_v<T>)
			{
				return StringToEnumOpt<T>(json[key].getOr<String>(U""));
			}
			else
			{
				return json[key].getOpt<T>();
			}
		}
		return none;
	}

	template <typename T>
	[[nodiscard]]
	String EnumToString(T value) requires std::is_enum_v<T>
	{
		return Unicode::FromUTF8(magic_enum::enum_name(value));
	}

	template <typename T>
	[[nodiscard]]
	bool EnumContains(StringView value) requires std::is_enum_v<T>
	{
		return magic_enum::enum_contains<T>(value.toUTF8());
	}

	template <typename T>
	[[nodiscard]]
	Array<String> EnumNames() requires std::is_enum_v<T>
	{
		Array<String> result;
		result.reserve(magic_enum::enum_count<T>());
		for (const auto& name : magic_enum::enum_names<T>())
		{
			result.push_back(Unicode::FromUTF8(name));
		}
		return result;
	}

	template <typename T>
	[[nodiscard]]
	Optional<T> StringToValueOpt(StringView value)
	{
		if constexpr (std::is_enum_v<T>)
		{
			return StringToEnumOpt<T>(value);
		}
		else if constexpr (std::same_as<T, String>)
		{
			return String{ value };
		}
		else
		{
			return ParseOpt<T>(value);
		}
	}

	template <typename T>
	[[nodiscard]]
	T StringToValueOr(StringView value, const T& defaultValue)
	{
		if (auto opt = StringToValueOpt<T>(value))
		{
			return *opt;
		}
		return defaultValue;
	}

	template <typename T>
	[[nodiscard]]
	String ValueToString(const T& value)
	{
		if constexpr (std::is_enum_v<T>)
		{
			return EnumToString(value);
		}
		else if constexpr (std::same_as<T, String>)
		{
			return value;
		}
		else
		{
			return Format(value);
		}
	}

	template <class T>
	concept HasToJSON = requires(const T & t)
	{
		{ t.toJSON() } -> std::convertible_to<JSON>;
	};

	template <class T>
	concept HasFromJSON = requires()
	{
		{ T::fromJSON(JSON{}, T{}) } -> std::convertible_to<T>;
	};

	template<typename T>
	[[nodiscard]]
	JSON ToArrayJSON(const T& value);

	template<typename T>
	[[nodiscard]]
	T FromArrayJSON(const JSON& json, const T& defaultValue = T{});

	template<>
	[[nodiscard]]
	inline JSON ToArrayJSON<Vec2>(const Vec2& value)
	{
		return Array<double>{ value.x, value.y };
	}

	template<>
	[[nodiscard]]
	inline Vec2 FromArrayJSON<Vec2>(const JSON& json, const Vec2& defaultValue)
	{
		if (json.isArray() && json.size() == 2)
		{
			return Vec2{ json[0].getOr<double>(0.0), json[1].getOr<double>(0.0) };
		}
		else if (json.isString())
		{
			Logger << U"[NocoUI warning] String format Vec2 found, returning default value";
			return defaultValue;
		}
		return json.getOr<Vec2>(defaultValue);
	}

	template<>
	[[nodiscard]]
	inline JSON ToArrayJSON<Color>(const Color& value)
	{
		return Array<int32>{ value.r, value.g, value.b, value.a };
	}

	template<>
	[[nodiscard]]
	inline Color FromArrayJSON<Color>(const JSON& json, const Color& defaultValue)
	{
		if (json.isArray() && json.size() == 4)
		{
			return Color{
				static_cast<uint8>(Clamp(json[0].getOr<int32>(0), 0, 255)),
				static_cast<uint8>(Clamp(json[1].getOr<int32>(0), 0, 255)),
				static_cast<uint8>(Clamp(json[2].getOr<int32>(0), 0, 255)),
				static_cast<uint8>(Clamp(json[3].getOr<int32>(255), 0, 255))
			};
		}
		else if (json.isString())
		{
			Logger << U"[NocoUI warning] String format Color found, returning default value";
			return defaultValue;
		}
		return json.getOr<Color>(defaultValue);
	}

}
