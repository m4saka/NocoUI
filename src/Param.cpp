#include "NocoUI/Param.hpp"

namespace noco
{
	JSON ParamValueToJSONValue(const ParamValue& value)
	{
		return std::visit([](const auto& v) -> JSON {
			using T = std::decay_t<decltype(v)>;

			// 注意: JSONは波括弧初期化だとinitializer_list扱いで配列になってしまうため、必ず丸括弧初期化でないといけない
			if constexpr (std::is_same_v<T, bool>)
			{
				return JSON(v);
			}
			else if constexpr (std::is_arithmetic_v<T> && !std::is_same_v<T, bool>)
			{
				return JSON(static_cast<double>(v));
			}
			else if constexpr (std::is_same_v<T, String>)
			{
				return JSON(v);
			}
			else if constexpr (std::is_same_v<T, Color>)
			{
				// Colorは配列形式で保存 [r, g, b, a]
				return JSON(Array<int32>{ v.r, v.g, v.b, v.a });
			}
			else if constexpr (std::is_same_v<T, Vec2>)
			{
				// Vec2は配列形式で保存 [x, y]
				return JSON(Array<double>{ v.x, v.y });
			}
			else if constexpr (std::is_same_v<T, LRTB>)
			{
				// LRTBは配列形式で保存 [left, right, top, bottom]
				return JSON(Array<double>{ v.left, v.right, v.top, v.bottom });
			}

			return JSON{};
		}, value);
	}

	JSON ParamValueToParamObjectJSON(const ParamValue& value)
	{
		return JSON
		{
			{ U"type", ParamTypeToString(GetParamType(value)) },
			{ U"value", ParamValueToJSONValue(value) },
		};
	}

	Optional<ParamValue> ParamValueFromJSONValue(const JSON& json, ParamType type)
	{
		if (type == ParamType::Bool && json.isBool())
		{
			return ParamValue{ json.get<bool>() };
		}
		else if (type == ParamType::Number && json.isNumber())
		{
			return ParamValue{ json.get<double>() };
		}
		else if (type == ParamType::String && json.isString())
		{
			return ParamValue{ json.getString() };
		}
		else if (type == ParamType::Color && json.isArray() && json.size() == 4)
		{
			const int32 r = json[0].get<int32>();
			const int32 g = json[1].get<int32>();
			const int32 b = json[2].get<int32>();
			const int32 a = json[3].get<int32>();
			return ParamValue{ Color{ static_cast<uint8>(Clamp(r, 0, 255)), static_cast<uint8>(Clamp(g, 0, 255)), static_cast<uint8>(Clamp(b, 0, 255)), static_cast<uint8>(Clamp(a, 0, 255)) } };
		}
		else if (type == ParamType::Vec2 && json.isArray() && json.size() == 2)
		{
			const double x = json[0].get<double>();
			const double y = json[1].get<double>();
			return ParamValue{ Vec2{ x, y } };
		}
		else if (type == ParamType::LRTB && json.isArray() && json.size() == 4)
		{
			const double left = json[0].get<double>();
			const double right = json[1].get<double>();
			const double top = json[2].get<double>();
			const double bottom = json[3].get<double>();
			return ParamValue{ LRTB{ left, right, top, bottom } };
		}
		return none;
	}

	Optional<ParamValue> ParamValueFromParamObjectJSON(const JSON& json)
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
