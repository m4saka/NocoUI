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
	
	// ParamValueをJSONに変換
	[[nodiscard]]
	inline JSON ParamValueToJSON(const ParamValue& value)
	{
		return std::visit([](const auto& v) -> JSON {
			using T = std::decay_t<decltype(v)>;
			
			JSON json;
			json[U"type"] = ParamTypeToString(GetParamType(ParamValue{v}));
			
			if constexpr (std::is_same_v<T, bool>)
			{
				json[U"value"] = v;
			}
			else if constexpr (std::is_arithmetic_v<T> && !std::is_same_v<T, bool>)
			{
				json[U"value"] = static_cast<double>(v);
			}
			else if constexpr (std::is_same_v<T, String>)
			{
				json[U"value"] = v;
			}
			else if constexpr (std::is_same_v<T, Color>)
			{
				// Colorは配列形式で保存 [r, g, b, a]
				json[U"value"] = Array<int32>{ v.r, v.g, v.b, v.a };
			}
			else if constexpr (std::is_same_v<T, Vec2>)
			{
				// Vec2は配列形式で保存 [x, y]
				json[U"value"] = Array<double>{ v.x, v.y };
			}
			else if constexpr (std::is_same_v<T, LRTB>)
			{
				// LRTBは配列形式で保存 [left, right, top, bottom]
				json[U"value"] = Array<double>{ v.left, v.right, v.top, v.bottom };
			}
			
			return json;
		}, value);
	}
	
	// JSONからParamValueを作成
	[[nodiscard]]
	inline Optional<ParamValue> ParamValueFromJSON(const JSON& json)
	{
		if (!json.contains(U"type") || !json.contains(U"value"))
		{
			return none;
		}
		
		// typeが文字列でない場合のチェック
		if (!json[U"type"].isString())
		{
			Logger << U"[NocoUI warning] Parameter type is not a string. Skipping.";
			return none;
		}
		
		const String typeStr = json[U"type"].getString();
		const JSON& valueJson = json[U"value"];
		
		if (typeStr == U"Bool")
		{
			if (valueJson.isBool())
			{
				return ParamValue{ valueJson.get<bool>() };
			}
			else
			{
				Logger << U"[NocoUI warning] Parameter value for Bool type is not a boolean. Skipping.";
				return none;
			}
		}
		else if (typeStr == U"Number")
		{
			if (valueJson.isNumber())
			{
				return ParamValue{ valueJson.get<double>() };
			}
			else
			{
				Logger << U"[NocoUI warning] Parameter value for Number type is not a number. Skipping.";
				return none;
			}
		}
		else if (typeStr == U"String")
		{
			if (valueJson.isString())
			{
				return ParamValue{ valueJson.getString() };
			}
			else
			{
				Logger << U"[NocoUI warning] Parameter value for String type is not a string. Skipping.";
				return none;
			}
		}
		else if (typeStr == U"Color")
		{
			// Colorは配列形式 [r, g, b, a] のみ受け付ける
			if (valueJson.isArray() && valueJson.size() == 4)
			{
				const int32 r = valueJson[0].get<int32>();
				const int32 g = valueJson[1].get<int32>();
				const int32 b = valueJson[2].get<int32>();
				const int32 a = valueJson[3].get<int32>();
				return ParamValue{ Color{ static_cast<uint8>(Clamp(r, 0, 255)), static_cast<uint8>(Clamp(g, 0, 255)), static_cast<uint8>(Clamp(b, 0, 255)), static_cast<uint8>(Clamp(a, 0, 255)) } };
			}
			else if (valueJson.isString())
			{
				Logger << U"[NocoUI warning] Color parameter must be an array [r, g, b, a], not a string. Using default Color(0, 0, 0, 0).";
				return ParamValue{ Color{ 0, 0, 0, 0 } };
			}
			Logger << U"[NocoUI warning] Failed to parse Color parameter. Expected array format [r, g, b, a]. Using default Color(0, 0, 0, 0).";
			return ParamValue{ Color{ 0, 0, 0, 0 } };
		}
		else if (typeStr == U"Vec2")
		{
			// Vec2は配列形式 [x, y] のみ受け付ける
			if (valueJson.isArray() && valueJson.size() == 2)
			{
				const double x = valueJson[0].get<double>();
				const double y = valueJson[1].get<double>();
				return ParamValue{ Vec2{ x, y } };
			}
			else if (valueJson.isString())
			{
				Logger << U"[NocoUI warning] Vec2 parameter must be an array [x, y], not a string. Using default Vec2(0, 0).";
				return ParamValue{ Vec2{ 0.0, 0.0 } };
			}
			Logger << U"[NocoUI warning] Failed to parse Vec2 parameter. Expected array format [x, y]. Using default Vec2(0, 0).";
			return ParamValue{ Vec2{ 0.0, 0.0 } };
		}
		else if (typeStr == U"LRTB")
		{
			// LRTBは配列形式 [left, right, top, bottom] のみ受け付ける
			if (valueJson.isArray() && valueJson.size() == 4)
			{
				const double left = valueJson[0].get<double>();
				const double right = valueJson[1].get<double>();
				const double top = valueJson[2].get<double>();
				const double bottom = valueJson[3].get<double>();
				return ParamValue{ LRTB{ left, right, top, bottom } };
			}
			else if (valueJson.isString())
			{
				Logger << U"[NocoUI warning] LRTB parameter must be an array [left, right, top, bottom], not a string. Using default LRTB(0, 0, 0, 0).";
				return ParamValue{ LRTB{ 0.0, 0.0, 0.0, 0.0 } };
			}
			Logger << U"[NocoUI warning] Failed to parse LRTB parameter. Expected array format [left, right, top, bottom]. Using default LRTB(0, 0, 0, 0).";
			return ParamValue{ LRTB{ 0.0, 0.0, 0.0, 0.0 } };
		}
		else
		{
			Logger << U"[NocoUI warning] Unknown parameter type '{}'. Skipping."_fmt(typeStr);
			return none;
		}
	}
}
