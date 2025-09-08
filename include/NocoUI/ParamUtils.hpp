#pragma once
#include <Siv3D.hpp>
#include "Param.hpp"
#include "Serialization.hpp"

namespace noco
{
	[[nodiscard]]
	inline bool IsValidParameterName(StringView paramName)
	{
		if (paramName.isEmpty())
		{
			return false;
		}
		
		// 最初の文字はアルファベットまたはアンダースコアでなければならない
		const char32 firstChar = paramName[0];
		if (!((firstChar >= U'a' && firstChar <= U'z') || 
		      (firstChar >= U'A' && firstChar <= U'Z') ||
		      (firstChar == U'_')))
		{
			return false;
		}
		
		// 2文字目以降は英数字またはアンダースコアでなければならない
		for (size_t i = 1; i < paramName.length(); ++i)
		{
			const char32 ch = paramName[i];
			if (!((ch >= U'a' && ch <= U'z') || 
			      (ch >= U'A' && ch <= U'Z') || 
			      (ch >= U'0' && ch <= U'9') || 
			      (ch == U'_')))
			{
				return false;
			}
		}
		
		return true;
	}

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
