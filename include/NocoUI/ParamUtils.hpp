#pragma once
#include <Siv3D.hpp>
#include "Param.hpp"
#include "Serialization.hpp"

namespace noco
{
	// StringViewとParamTypeからParamValueを作成
	[[nodiscard]]
	inline ParamValue StringToParamValue(StringView value, ParamType type)
	{
		switch (type)
		{
		case ParamType::Bool:
			return StringToValueOr<bool>(value, false);
		case ParamType::Number:
			return StringToValueOr<double>(value, 0.0);
		case ParamType::String:
			return StringToValueOr<String>(value, String{});
		case ParamType::Color:
			return StringToValueOr<ColorF>(value, ColorF{});
		case ParamType::Vec2:
			return StringToValueOr<Vec2>(value, Vec2{});
		case ParamType::LRTB:
			return StringToValueOr<LRTB>(value, LRTB{});
		case ParamType::Unknown:
		default:
			return String{};
		}
	}
}