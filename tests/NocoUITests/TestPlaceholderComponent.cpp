#include <catch2/catch.hpp>
#include <Siv3D.hpp>
#include <NocoUI/Component/PlaceholderComponent.hpp>

// ========================================
// PlaceholderComponentのテスト
// ========================================

using namespace noco;

TEST_CASE("PlaceholderComponent with InteractionState values", "[PlaceholderComponent]")
{
	SECTION("InteractionState values are properly saved and loaded")
	{
		JSON originalData;
		originalData[U"type"] = U"CustomButton";
		originalData[U"color"] = JSON{
			{U"default", U"#FF0000"},
			{U"hovered", U"#00FF00"},
			{U"pressed", U"#0000FF"},
			{U"disabled", U"#808080"}
		};
		originalData[U"text"] = U"Click me";
		
		auto placeholder = PlaceholderComponent::Create(U"CustomButton", originalData);
		REQUIRE(placeholder != nullptr);
		
		// JSONに保存
		JSON savedJson = placeholder->toJSON();
		REQUIRE(savedJson[U"type"].getString() == U"CustomButton");
		
		// PropertyValue形式で保存されているか確認
		REQUIRE(savedJson[U"color"].isObject());
		REQUIRE(savedJson[U"color"][U"default"].getString() == U"#FF0000");
		REQUIRE(savedJson[U"color"][U"hovered"].getString() == U"#00FF00");
		REQUIRE(savedJson[U"color"][U"pressed"].getString() == U"#0000FF");
		REQUIRE(savedJson[U"color"][U"disabled"].getString() == U"#808080");
		
		// textは単純な文字列として保存
		REQUIRE(savedJson[U"text"].getString() == U"Click me");
		
		// 再読み込み
		auto loaded = std::make_shared<PlaceholderComponent>(U"", JSON{});
		bool loadSuccess = loaded->tryReadFromJSON(savedJson);
		REQUIRE(loadSuccess);
		
		JSON loadedJson = loaded->toJSON();
		REQUIRE(loadedJson[U"color"].isObject());
		REQUIRE(loadedJson[U"color"][U"default"].getString() == U"#FF0000");
		REQUIRE(loadedJson[U"color"][U"hovered"].getString() == U"#00FF00");
		REQUIRE(loadedJson[U"color"][U"pressed"].getString() == U"#0000FF");
		REQUIRE(loadedJson[U"color"][U"disabled"].getString() == U"#808080");
	}
	
	SECTION("Partial InteractionState values")
	{
		JSON originalData;
		originalData[U"type"] = U"CustomWidget";
		originalData[U"backgroundColor"] = JSON{
			{U"default", U"white"},
			{U"hovered", U"lightgray"}
		};
		
		auto placeholder = PlaceholderComponent::Create(U"CustomWidget", originalData);
		
		JSON savedJson = placeholder->toJSON();
		REQUIRE(savedJson[U"backgroundColor"].isObject());
		REQUIRE(savedJson[U"backgroundColor"][U"default"].getString() == U"white");
		REQUIRE(savedJson[U"backgroundColor"][U"hovered"].getString() == U"lightgray");
		REQUIRE_FALSE(savedJson[U"backgroundColor"].contains(U"pressed"));
		REQUIRE_FALSE(savedJson[U"backgroundColor"].contains(U"disabled"));
	}
	
	SECTION("Single value treated as default value")
	{
		JSON originalData;
		originalData[U"type"] = U"SimpleWidget";
		originalData[U"title"] = U"My Title";
		originalData[U"width"] = U"300";
		
		auto placeholder = PlaceholderComponent::Create(U"SimpleWidget", originalData);
		
		JSON savedJson = placeholder->toJSON();
		// 単一の値は文字列として保存される（PropertyValue形式ではない）
		REQUIRE(savedJson[U"title"].isString());
		REQUIRE(savedJson[U"title"].getString() == U"My Title");
		REQUIRE(savedJson[U"width"].getString() == U"300");
	}
}

TEST_CASE("PlaceholderComponent with parameter references", "[PlaceholderComponent]")
{
	SECTION("Parameter reference is saved and loaded")
	{
		JSON originalData;
		originalData[U"type"] = U"CustomLabel";
		originalData[U"fontSize"] = U"14";
		originalData[U"fontSize_paramRef"] = U"globalFontSize";
		originalData[U"color"] = U"black";
		originalData[U"color_paramRef"] = U"themeTextColor";
		
		auto placeholder = PlaceholderComponent::Create(U"CustomLabel", originalData);
		
		JSON savedJson = placeholder->toJSON();
		REQUIRE(savedJson[U"fontSize"].getString() == U"14");
		REQUIRE(savedJson[U"fontSize_paramRef"].getString() == U"globalFontSize");
		REQUIRE(savedJson[U"color"].getString() == U"black");
		REQUIRE(savedJson[U"color_paramRef"].getString() == U"themeTextColor");
		
		// 再読み込みしてパラメータ参照が保持されているか確認
		auto loaded = std::make_shared<PlaceholderComponent>(U"", JSON{});
		loaded->tryReadFromJSON(savedJson);
		
		JSON loadedJson = loaded->toJSON();
		REQUIRE(loadedJson[U"fontSize_paramRef"].getString() == U"globalFontSize");
		REQUIRE(loadedJson[U"color_paramRef"].getString() == U"themeTextColor");
	}
	
	SECTION("Parameter reference with InteractionState values")
	{
		JSON originalData;
		originalData[U"type"] = U"ComplexLabel";
		originalData[U"textColor"] = JSON{
			{U"default", U"black"},
			{U"hovered", U"blue"}
		};
		originalData[U"textColor_paramRef"] = U"globalTextColor";
		
		auto placeholder = PlaceholderComponent::Create(U"ComplexLabel", originalData);
		
		JSON savedJson = placeholder->toJSON();
		REQUIRE(savedJson[U"textColor"].isObject());
		REQUIRE(savedJson[U"textColor"][U"default"].getString() == U"black");
		REQUIRE(savedJson[U"textColor"][U"hovered"].getString() == U"blue");
		REQUIRE(savedJson[U"textColor_paramRef"].getString() == U"globalTextColor");
	}
}

TEST_CASE("PlaceholderComponent with smoothTime", "[PlaceholderComponent]")
{
	SECTION("SmoothTime is properly handled")
	{
		JSON originalData;
		originalData[U"type"] = U"AnimatedBox";
		originalData[U"position"] = JSON{
			{U"default", U"0,0"},
			{U"hovered", U"10,10"},
			{U"smoothTime", 0.3}
		};
		originalData[U"opacity"] = JSON{
			{U"default", U"1.0"},
			{U"smoothTime", 0.5}
		};
		
		auto placeholder = PlaceholderComponent::Create(U"AnimatedBox", originalData);
		
		JSON savedJson = placeholder->toJSON();
		REQUIRE(savedJson[U"position"].isObject());
		REQUIRE(savedJson[U"position"][U"default"].getString() == U"0,0");
		REQUIRE(savedJson[U"position"][U"hovered"].getString() == U"10,10");
		REQUIRE(savedJson[U"position"][U"smoothTime"].get<double>() == Approx(0.3));
		
		REQUIRE(savedJson[U"opacity"].isObject());
		REQUIRE(savedJson[U"opacity"][U"default"].getString() == U"1.0");
		REQUIRE(savedJson[U"opacity"][U"smoothTime"].get<double>() == Approx(0.5));
		
		// 再読み込みテスト
		auto loaded = std::make_shared<PlaceholderComponent>(U"", JSON{});
		loaded->tryReadFromJSON(savedJson);
		
		JSON loadedJson = loaded->toJSON();
		REQUIRE(loadedJson[U"position"][U"smoothTime"].get<double>() == Approx(0.3));
		REQUIRE(loadedJson[U"opacity"][U"smoothTime"].get<double>() == Approx(0.5));
	}
}

TEST_CASE("PlaceholderComponent complex PropertyValue combinations", "[PlaceholderComponent]")
{
	SECTION("Combined InteractionState, paramRef, and smoothTime")
	{
		JSON originalData;
		originalData[U"type"] = U"ComplexButton";
		originalData[U"borderColor"] = JSON{
			{U"default", U"gray"},
			{U"hovered", U"blue"},
			{U"pressed", U"darkblue"},
			{U"smoothTime", 0.2}
		};
		originalData[U"borderColor_paramRef"] = U"themeBorderColor";
		
		auto placeholder = PlaceholderComponent::Create(U"ComplexButton", originalData);
		
		JSON savedJson = placeholder->toJSON();
		
		// PropertyValue形式で保存されている
		REQUIRE(savedJson[U"borderColor"].isObject());
		REQUIRE(savedJson[U"borderColor"][U"default"].getString() == U"gray");
		REQUIRE(savedJson[U"borderColor"][U"hovered"].getString() == U"blue");
		REQUIRE(savedJson[U"borderColor"][U"pressed"].getString() == U"darkblue");
		REQUIRE(savedJson[U"borderColor"][U"smoothTime"].get<double>() == Approx(0.2));
		
		// パラメータ参照は別キーで保存
		REQUIRE(savedJson[U"borderColor_paramRef"].getString() == U"themeBorderColor");
		
		// 再読み込みして正しく復元されるか確認
		auto loaded = std::make_shared<PlaceholderComponent>(U"", JSON{});
		loaded->tryReadFromJSON(savedJson);
		
		JSON loadedJson = loaded->toJSON();
		REQUIRE(loadedJson[U"borderColor"][U"default"].getString() == U"gray");
		REQUIRE(loadedJson[U"borderColor"][U"hovered"].getString() == U"blue");
		REQUIRE(loadedJson[U"borderColor"][U"pressed"].getString() == U"darkblue");
		REQUIRE(loadedJson[U"borderColor"][U"smoothTime"].get<double>() == Approx(0.2));
		REQUIRE(loadedJson[U"borderColor_paramRef"].getString() == U"themeBorderColor");
	}
	
	SECTION("Mixed property types - some with InteractionState, some without")
	{
		JSON originalData;
		originalData[U"type"] = U"MixedWidget";
		originalData[U"title"] = U"My Widget";  // 単純な文字列
		originalData[U"backgroundColor"] = JSON{  // InteractionState付き
			{U"default", U"white"},
			{U"hovered", U"#F0F0F0"}
		};
		originalData[U"width"] = U"200";  // 単純な文字列
		originalData[U"width_paramRef"] = U"defaultWidth";  // パラメータ参照のみ
		
		auto placeholder = PlaceholderComponent::Create(U"MixedWidget", originalData);
		
		JSON savedJson = placeholder->toJSON();
		
		// 単純な文字列プロパティ
		REQUIRE(savedJson[U"title"].isString());
		REQUIRE(savedJson[U"title"].getString() == U"My Widget");
		
		// InteractionState付きプロパティ
		REQUIRE(savedJson[U"backgroundColor"].isObject());
		REQUIRE(savedJson[U"backgroundColor"][U"default"].getString() == U"white");
		REQUIRE(savedJson[U"backgroundColor"][U"hovered"].getString() == U"#F0F0F0");
		
		// パラメータ参照付きの単純プロパティ
		REQUIRE(savedJson[U"width"].getString() == U"200");
		REQUIRE(savedJson[U"width_paramRef"].getString() == U"defaultWidth");
	}
}

TEST_CASE("PlaceholderComponent edge cases", "[PlaceholderComponent]")
{
	SECTION("Empty property values")
	{
		JSON originalData;
		originalData[U"type"] = U"EmptyWidget";
		originalData[U"emptyProp"] = U"";
		originalData[U"nullProp"] = JSON{
			{U"default", U""},
			{U"hovered", U""}
		};
		
		auto placeholder = PlaceholderComponent::Create(U"EmptyWidget", originalData);
		
		JSON savedJson = placeholder->toJSON();
		REQUIRE(savedJson[U"emptyProp"].getString() == U"");
		REQUIRE(savedJson[U"nullProp"][U"default"].getString() == U"");
		REQUIRE(savedJson[U"nullProp"][U"hovered"].getString() == U"");
	}
	
	SECTION("Property modification via setPropertyValueString")
	{
		JSON originalData;
		originalData[U"type"] = U"ModifiableWidget";
		originalData[U"status"] = U"initial";
		
		auto placeholder = PlaceholderComponent::Create(U"ModifiableWidget", originalData);
		
		// プロパティを変更
		placeholder->setPropertyValueString(U"status", U"modified");
		placeholder->setPropertyValueString(U"newProp", U"newValue");
		
		JSON savedJson = placeholder->toJSON();
		REQUIRE(savedJson[U"status"].getString() == U"modified");
		REQUIRE(savedJson[U"newProp"].getString() == U"newValue");
	}
	
	SECTION("Non-string values in original data become empty strings")
	{
		JSON originalData;
		originalData[U"type"] = U"MixedTypeWidget";
		originalData[U"intValue"] = 42;
		originalData[U"floatValue"] = 3.14;
		originalData[U"boolValue"] = true;
		originalData[U"arrayValue"] = Array<JSON>{1, 2, 3};
		originalData[U"objectValue"] = JSON{{U"key", U"value"}};
		
		auto placeholder = PlaceholderComponent::Create(U"MixedTypeWidget", originalData);
		
		// 文字列以外の値は空文字列として扱われる
		REQUIRE(placeholder->getPropertyValueString(U"intValue") == U"");
		REQUIRE(placeholder->getPropertyValueString(U"floatValue") == U"");
		REQUIRE(placeholder->getPropertyValueString(U"boolValue") == U"");
		REQUIRE(placeholder->getPropertyValueString(U"arrayValue") == U"");
		REQUIRE(placeholder->getPropertyValueString(U"objectValue") == U"");
		
		JSON savedJson = placeholder->toJSON();
		REQUIRE(savedJson[U"intValue"].getString() == U"");
		REQUIRE(savedJson[U"floatValue"].getString() == U"");
		REQUIRE(savedJson[U"boolValue"].getString() == U"");
		REQUIRE(savedJson[U"arrayValue"].getString() == U"");
		REQUIRE(savedJson[U"objectValue"].getString() == U"");
	}
}

TEST_CASE("PlaceholderComponent property accessor methods", "[PlaceholderComponent]")
{
	SECTION("getPropertyNames returns all property names")
	{
		JSON originalData;
		originalData[U"type"] = U"TestWidget";
		originalData[U"prop1"] = U"value1";
		originalData[U"prop2"] = JSON{
			{U"default", U"value2"},
			{U"hovered", U"value2_hover"}
		};
		originalData[U"prop3"] = U"value3";
		originalData[U"prop3_paramRef"] = U"param3";
		
		auto placeholder = PlaceholderComponent::Create(U"TestWidget", originalData);
		
		Array<String> propNames = placeholder->getPropertyNames();
		REQUIRE(propNames.size() == 3);
		REQUIRE(propNames.contains(U"prop1"));
		REQUIRE(propNames.contains(U"prop2"));
		REQUIRE(propNames.contains(U"prop3"));
	}
	
	SECTION("hasProperty checks property existence")
	{
		JSON originalData;
		originalData[U"type"] = U"TestWidget";
		originalData[U"existingProp"] = U"value";
		
		auto placeholder = PlaceholderComponent::Create(U"TestWidget", originalData);
		
		REQUIRE(placeholder->hasProperty(U"existingProp"));
		REQUIRE_FALSE(placeholder->hasProperty(U"nonExistingProp"));
		
		// 新しいプロパティを追加
		placeholder->setPropertyValueString(U"newProp", U"newValue");
		REQUIRE(placeholder->hasProperty(U"newProp"));
	}
	
	SECTION("getProperty returns PlaceholderProperty pointer")
	{
		JSON originalData;
		originalData[U"type"] = U"TestWidget";
		originalData[U"testProp"] = JSON{
			{U"default", U"defaultValue"},
			{U"hovered", U"hoveredValue"}
		};
		originalData[U"testProp_paramRef"] = U"testParam";
		
		auto placeholder = PlaceholderComponent::Create(U"TestWidget", originalData);
		
		auto prop = placeholder->getProperty(U"testProp");
		REQUIRE(prop != nullptr);
		REQUIRE(prop->propertyValue().defaultValue == U"defaultValue");
		REQUIRE(prop->propertyValue().hoveredValue.has_value());
		REQUIRE(*prop->propertyValue().hoveredValue == U"hoveredValue");
		REQUIRE(prop->paramRef() == U"testParam");
		
		auto nonExistingProp = placeholder->getProperty(U"nonExisting");
		REQUIRE(nonExistingProp == nullptr);
	}
}