#pragma once
#include <Siv3D.hpp>
#include "magic_enum.hpp" // https://github.com/Neargye/magic_enum

namespace noco
{
	template <typename T>
	[[nodiscard]]
	T GetFromJSONOr(const JSON& json, const String& key, const T& defaultValue)
	{
		if (json.isObject() && json.contains(key))
		{
			if constexpr (std::is_enum_v<T>)
			{
				return StringToEnum<T>(json[key].getString(), defaultValue);
			}
			else
			{
				return json[key].getOr<T>(defaultValue);
			}
		}
		return defaultValue;
	}

	template <typename T>
	String EnumToString(T value) requires std::is_enum_v<T>
	{
		return Unicode::FromUTF8(magic_enum::enum_name(value));
	}

	template <typename T>
	bool EnumContains(StringView value) requires std::is_enum_v<T>
	{
		return magic_enum::enum_contains<T>(value.toUTF8());
	}

	template <typename T>
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
}
