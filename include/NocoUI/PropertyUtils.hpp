#pragma once
#include <Siv3D.hpp>
#include "Property.hpp"
#include "Param.hpp"

namespace noco
{
	// PropertyEditTypeからParamTypeへの変換
	[[nodiscard]]
	inline ParamType PropertyEditTypeToParamType(PropertyEditType editType)
	{
		switch (editType)
		{
		case PropertyEditType::Bool:
			return ParamType::Bool;
		case PropertyEditType::Number:
			return ParamType::Number;
		case PropertyEditType::Text:
		case PropertyEditType::Enum:
			return ParamType::String;
		case PropertyEditType::Vec2:
			return ParamType::Vec2;
		case PropertyEditType::Color:
			return ParamType::Color;
		case PropertyEditType::LRTB:
			return ParamType::LRTB;
		default:
			return ParamType::String;
		}
	}

	// IPropertyからParamTypeを取得する関数
	[[nodiscard]]
	inline ParamType GetRequiredParamType(IProperty* prop)
	{
		if (!prop)
		{
			return ParamType::String;
		}

		const PropertyEditType editType = prop->editType();
		return PropertyEditTypeToParamType(editType);
	}

	// IPropertyに対してパラメータ参照を設定する統一関数
	inline bool SetPropertyParamRef(IProperty* prop, const String& paramRef)
	{
		if (!prop)
		{
			return false;
		}
		
		prop->setParamRef(paramRef);
		return true;
	}

	// プロパティの型に基づいて処理を実行する統一関数
	template <typename Visitor>
	inline bool VisitPropertyType(IProperty* prop, Visitor&& visitor)
	{
		if (!prop)
		{
			return false;
		}

		const PropertyEditType editType = prop->editType();
		
		switch (editType)
		{
		case PropertyEditType::Bool:
			return visitor(PropertyEditType::Bool);
		case PropertyEditType::Number:
			return visitor(PropertyEditType::Number);
		case PropertyEditType::Text:
			return visitor(PropertyEditType::Text);
		case PropertyEditType::Enum:
			return visitor(PropertyEditType::Enum);
		case PropertyEditType::Vec2:
			return visitor(PropertyEditType::Vec2);
		case PropertyEditType::Color:
			return visitor(PropertyEditType::Color);
		case PropertyEditType::LRTB:
			return visitor(PropertyEditType::LRTB);
		default:
			return false;
		}
	}
}
