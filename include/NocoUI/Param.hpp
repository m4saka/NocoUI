#pragma once
#include <Siv3D.hpp>
#include <variant>
#include "Serialization.hpp"
#include "LRTB.hpp"

namespace noco
{
	// パラメータ値の型定義
	using ParamValue = std::variant<bool, double, String, Color, Vec2, LRTB>;
	
	// UI表示用の型列挙
	enum class ParamType : uint8
	{
		Unknown,
		Bool,
		Number,  // 数値は全てdouble型で保持
		String,
		Color,   // 色は全てColor型で保持
		Vec2,
		LRTB,
	};

	[[nodiscard]]
	inline String ParamTypeToString(ParamType type)
	{
		switch (type)
		{
		case ParamType::Bool:
			return U"Bool";
		case ParamType::Number:
			return U"Number";
		case ParamType::String:
			return U"String";
		case ParamType::Color:
			return U"Color";
		case ParamType::Vec2:
			return U"Vec2";
		case ParamType::LRTB:
			return U"LRTB";
		case ParamType::Unknown:
		default:
			return U"Unknown";
		}
	}
	
	// ParamValueから型情報を取得
	[[nodiscard]]
	inline ParamType GetParamType(const ParamValue& value)
	{
		return std::visit([](const auto& v) -> ParamType
			{
				using T = std::decay_t<decltype(v)>;
				if constexpr (std::is_same_v<T, bool>)
				{
					return ParamType::Bool;
				}
				else if constexpr (std::is_arithmetic_v<T> && !std::is_same_v<T, bool>)
				{
					return ParamType::Number;
				}
				else if constexpr (std::is_same_v<T, String>)
				{
					return ParamType::String;
				}
				else if constexpr (std::is_same_v<T, Color>)
				{
					return ParamType::Color;
				}
				else if constexpr (std::is_same_v<T, Vec2>)
				{
					return ParamType::Vec2;
				}
				else if constexpr (std::is_same_v<T, LRTB>)
				{
					return ParamType::LRTB;
				}
				else
				{
					return ParamType::Unknown;
				}
			}, value);
	}
	
	// 型からParamTypeを取得
	template<typename T>
	constexpr ParamType GetParamTypeOf()
	{
		if constexpr (std::is_same_v<T, bool>)
		{
			return ParamType::Bool;
		}
		else if constexpr (std::is_arithmetic_v<T>)
		{
			return ParamType::Number;
		}
		else if constexpr (std::is_same_v<T, String>)
		{
			return ParamType::String;
		}
		else if constexpr (std::is_same_v<T, Color>)
		{
			return ParamType::Color;
		}
		else if constexpr (std::is_same_v<T, Vec2>)
		{
			return ParamType::Vec2;
		}
		else if constexpr (std::is_same_v<T, LRTB>)
		{
			return ParamType::LRTB;
		}
		else
		{
			static_assert(!std::is_same_v<T, T>, "Unsupported parameter type");
		}
	}
	
	// 型がパラメータ参照をサポートしているか返す
	template<typename T>
	constexpr bool IsParamSupportedType()
	{
		return std::is_same_v<T, bool> ||
			std::is_same_v<T, String> ||
			std::is_same_v<T, Color> ||
			std::is_same_v<T, Vec2> ||
			std::is_same_v<T, LRTB> ||
			std::is_arithmetic_v<T> ||
			std::is_enum_v<T>;
	}
	
	// 値をParamValue用の型に変換
	template<typename T>
	inline ParamValue MakeParamValue(const T& value)
	{
		if constexpr (std::is_arithmetic_v<T> && !std::is_same_v<T, bool>)
		{
			return static_cast<double>(value); // 全ての数値型はdoubleとして保持
		}
		// Colorは直接保持するため変換不要
		else if constexpr (std::is_same_v<T, const char32_t*>)
		{
			return String{ value }; // 文字列リテラル
		}
		else
		{
			return value;
		}
	}

	// ParamValue自体を受け取る場合はそのまま返す
	inline ParamValue MakeParamValue(const ParamValue& value)
	{
		return value;
	}

	// ParamValueから型安全に値を取得
	template<typename T>
	[[nodiscard]]
	inline Optional<T> GetParamValueAs(const ParamValue& value)
	{
		if constexpr (std::is_arithmetic_v<T> && !std::is_same_v<T, bool>)
		{
			// 全ての数値型はdoubleで保持している
			if (auto* ptr = std::get_if<double>(&value))
			{
				if constexpr (std::is_unsigned_v<T>)
				{
					if (*ptr < 0)
					{
						// unsignedの場合、負の値は0に丸める
						return T{ 0 };
					}
				}
				return static_cast<T>(*ptr);
			}
		}
		// Colorは直接保持しているため変換不要
		else if constexpr (std::is_enum_v<T>)
		{
			// enum型の場合、String型パラメータからenum値に変換
			if (auto* ptr = std::get_if<String>(&value))
			{
				return StringToEnum<T>(*ptr, T{});
			}
		}
		else if (auto* ptr = std::get_if<T>(&value))
		{
			return *ptr;
		}
		return none;
	}
	
	// ParamValueを文字列に変換
	[[nodiscard]]
	inline String ParamValueToString(const ParamValue& value)
	{
		return std::visit([](const auto& v) {
			return ValueToString(v);
		}, value);
	}
	
	// 文字列からParamValueを作成
	[[nodiscard]]
	inline Optional<ParamValue> ParamValueFromString(ParamType type, const String& str)
	{
		switch (type)
		{
		case ParamType::Bool:
			if (auto opt = StringToValueOpt<bool>(str))
			{
				return ParamValue{ *opt };
			}
			break;
		case ParamType::Number:
			if (auto opt = StringToValueOpt<double>(str))
			{
				return ParamValue{ *opt };
			}
			break;
		case ParamType::String:
			return ParamValue{ str };
		case ParamType::Color:
			if (auto opt = StringToValueOpt<Color>(str))
			{
				return ParamValue{ *opt };
			}
			break;
		case ParamType::Vec2:
			if (auto opt = StringToValueOpt<Vec2>(str))
			{
				return ParamValue{ *opt };
			}
			break;
		case ParamType::LRTB:
			if (auto opt = StringToValueOpt<LRTB>(str))
			{
				return ParamValue{ *opt };
			}
			break;
		case ParamType::Unknown:
		default:
			break;
		}
		return none;
	}
	
	// ParamValueを{"type": "...", "value": ...}形式のJSONに変換
	[[nodiscard]]
	JSON ParamValueToParamObjectJSON(const ParamValue& value);

	// 型を指定してJSONの値をParamValueに変換
	[[nodiscard]]
	Optional<ParamValue> ParamValueFromJSONValue(const JSON& json, ParamType type);

	// {"type": "...", "value": ...}形式のJSONからParamValueを作成
	[[nodiscard]]
	Optional<ParamValue> ParamValueFromParamObjectJSON(const JSON& json);
}
