#pragma once
#include <Siv3D.hpp>
#include <variant>
#include "Serialization.hpp"
#include "LRTB.hpp"

namespace noco
{
	// パラメータ値の型定義
	using ParamValue = std::variant<bool, double, String, ColorF, Vec2, LRTB>;
	
	// UI表示用の型列挙
	enum class ParamType : uint8
	{
		Unknown,
		Bool,
		Number,  // 数値は全てdouble型で保持
		String,
		Color,   // 色は全てColorF型で保持
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
				else if constexpr (std::is_same_v<T, ColorF>)
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
		else if constexpr (std::is_same_v<T, Color> || std::is_same_v<T, ColorF>)
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
	
	// 型がParamValueでサポートされているか判定
	template<typename T>
	constexpr bool IsParamValueType()
	{
		return std::is_same_v<T, bool> ||
			std::is_same_v<T, String> ||
			std::is_same_v<T, ColorF> ||
			std::is_same_v<T, Vec2> ||
			std::is_same_v<T, LRTB> ||
			std::is_same_v<T, Color> ||
			std::is_arithmetic_v<T>;
	}
	
	// 値をParamValue用の型に変換
	template<typename T>
	inline ParamValue MakeParamValue(const T& value)
	{
		if constexpr (std::is_arithmetic_v<T> && !std::is_same_v<T, bool>)
		{
			return static_cast<double>(value); // 全ての数値型はdoubleとして保持
		}
		else if constexpr (std::is_same_v<T, Color>)
		{
			return ColorF{ value }; // ColorはColorFとして保持
		}
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
		else if constexpr (std::is_same_v<T, Color>)
		{
			// ColorはColorFで保持している
			if (auto* ptr = std::get_if<ColorF>(&value))
			{
				return Color{ *ptr };
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
			if (auto opt = StringToValueOpt<ColorF>(str))
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
			else if constexpr (std::is_same_v<T, ColorF>)
			{
				// ColorFは文字列形式で保存（PropertyValueと同様）
				json[U"value"] = ValueToString(v);
			}
			else if constexpr (std::is_same_v<T, Vec2>)
			{
				// Vec2は文字列形式で保存（PropertyValueと同様）
				json[U"value"] = ValueToString(v);
			}
			else if constexpr (std::is_same_v<T, LRTB>)
			{
				// LRTBは文字列形式で保存（PropertyValueと同様）
				json[U"value"] = ValueToString(v);
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
			// ColorFは文字列形式で保存されている
			// カラーコード文字列はSiv3D側で実装されているためデシリアライズされる
			if (valueJson.isString())
			{
				if (auto opt = StringToValueOpt<ColorF>(valueJson.getString()))
				{
					return ParamValue{ *opt };
				}
			}
			Logger << U"[NocoUI warning] Failed to parse Color parameter. Skipping.";
			return none;
		}
		else if (typeStr == U"Vec2")
		{
			// Vec2は文字列形式で保存されている
			if (valueJson.isString())
			{
				if (auto opt = StringToValueOpt<Vec2>(valueJson.getString()))
				{
					return ParamValue{ *opt };
				}
			}
			Logger << U"[NocoUI warning] Failed to parse Vec2 parameter. Skipping.";
			return none;
		}
		else if (typeStr == U"LRTB")
		{
			// LRTBは文字列形式で保存されている
			if (valueJson.isString())
			{
				if (auto opt = StringToValueOpt<LRTB>(valueJson.getString()))
				{
					return ParamValue{ *opt };
				}
			}
			Logger << U"[NocoUI warning] Failed to parse LRTB parameter. Skipping.";
			return none;
		}
		else
		{
			Logger << U"[NocoUI warning] Unknown parameter type '{}'. Skipping."_fmt(typeStr);
			return none;
		}
	}
}
