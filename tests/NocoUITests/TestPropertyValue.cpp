#include <catch2/catch.hpp>
#include <Siv3D.hpp>
#include <NocoUI.hpp>

// ========================================
// PropertyValueのシリアライズのテスト
// ========================================

TEST_CASE("PropertyValue JSON serialization", "[PropertyValue][Serialization]")
{
	SECTION("Regular interaction states serialization")
	{
		// interactionStateのシリアライズテスト
		noco::PropertyValue<ColorF> colorProp = noco::PropertyValue<ColorF>{ ColorF{ 1.0, 0.0, 0.0, 1.0 } } // 赤（default）
			.withHovered(ColorF{ 0.0, 1.0, 0.0, 1.0 }) // 緑（hovered）
			.withPressed(ColorF{ 0.0, 0.0, 1.0, 1.0 }) // 青（pressed）
			.withDisabled(ColorF{ 0.5, 0.5, 0.5, 1.0 }) // グレー（disabled）
			.withSmoothTime(0.3);
		
		// JSONへシリアライズ
		JSON json = colorProp.toJSON();
		
		// 正しいキーが存在することを確認
		REQUIRE(json.isObject());
		REQUIRE(json.contains(U"default"));
		REQUIRE(json.contains(U"hovered"));
		REQUIRE(json.contains(U"pressed"));
		REQUIRE(json.contains(U"disabled"));
		REQUIRE(json.contains(U"smoothTime"));
		
		// デシリアライズして値が正しく復元されることを確認
		auto deserializedProp = noco::PropertyValue<ColorF>::fromJSON(json);
		s3d::Array<s3d::String> emptyActiveStyleStates;
		
		REQUIRE(deserializedProp.defaultValue() == ColorF{ 1.0, 0.0, 0.0, 1.0 });
		REQUIRE(deserializedProp.hoveredValue().has_value());
		REQUIRE(*deserializedProp.hoveredValue() == ColorF{ 0.0, 1.0, 0.0, 1.0 });
		REQUIRE(deserializedProp.pressedValue().has_value());
		REQUIRE(*deserializedProp.pressedValue() == ColorF{ 0.0, 0.0, 1.0, 1.0 });
		REQUIRE(deserializedProp.disabledValue().has_value());
		REQUIRE(*deserializedProp.disabledValue() == ColorF{ 0.5, 0.5, 0.5, 1.0 });
		REQUIRE(deserializedProp.smoothTime() == 0.3);
	}
	
	SECTION("StyleState serialization")
	{
		// styleState内のinteractionStateのシリアライズテスト
		noco::PropertyValue<double> alphaProp = noco::PropertyValue<double>{ 1.0 }
			.withSmoothTime(0.2)
			.withStyleStateInteraction(U"selected", noco::InteractionState::Default, 0.8)
			.withStyleStateInteraction(U"selected", noco::InteractionState::Hovered, 0.9)
			.withStyleStateInteraction(U"selected", noco::InteractionState::Pressed, 0.7)
			.withStyleStateInteraction(U"selected", noco::InteractionState::Disabled, 0.4)
			.withStyleState(U"active", 0.95);
		
		// JSONへシリアライズ
		JSON json = alphaProp.toJSON();
		
		// 構造を確認
		REQUIRE(json.isObject());
		REQUIRE(json.contains(U"default"));
		REQUIRE(json.contains(U"smoothTime"));
		REQUIRE(json.contains(U"styleStates"));
		
		// styleStatesの内容を確認
		const JSON& styleStatesJson = json[U"styleStates"];
		REQUIRE(styleStatesJson.isObject());
		REQUIRE(styleStatesJson.contains(U"selected"));
		REQUIRE(styleStatesJson.contains(U"active"));
		
		// "selected"のinteractionStateキーを確認
		const JSON& selectedJson = styleStatesJson[U"selected"];
		REQUIRE(selectedJson.isObject());
		REQUIRE(selectedJson.contains(U"default"));
		REQUIRE(selectedJson.contains(U"hovered"));
		REQUIRE(selectedJson.contains(U"pressed"));
		REQUIRE(selectedJson.contains(U"disabled"));
		
		// "active"はdefaultのみなので値として保存される
		const JSON& activeJson = styleStatesJson[U"active"];
		REQUIRE(activeJson.isNumber());
		REQUIRE(activeJson.get<double>() == 0.95);
		
		// デシリアライズして値が正しく復元されることを確認
		auto deserializedProp = noco::PropertyValue<double>::fromJSON(json);
		
		REQUIRE(deserializedProp.defaultValue() == 1.0);
		REQUIRE(deserializedProp.smoothTime() == 0.2);
		REQUIRE(deserializedProp.styleStateValues() != nullptr);
		
		// "selected"の値を確認
		auto selectedIt = deserializedProp.styleStateValues()->find(U"selected");
		REQUIRE(selectedIt != deserializedProp.styleStateValues()->end());
		REQUIRE(selectedIt->second.defaultValue == 0.8);
		REQUIRE(selectedIt->second.hoveredValue.has_value());
		REQUIRE(*selectedIt->second.hoveredValue == 0.9);
		REQUIRE(selectedIt->second.pressedValue.has_value());
		REQUIRE(*selectedIt->second.pressedValue == 0.7);
		REQUIRE(selectedIt->second.disabledValue.has_value());
		REQUIRE(*selectedIt->second.disabledValue == 0.4);
		
		// "active"の値を確認
		auto activeIt = deserializedProp.styleStateValues()->find(U"active");
		REQUIRE(activeIt != deserializedProp.styleStateValues()->end());
		REQUIRE(activeIt->second.defaultValue == 0.95);
		REQUIRE_FALSE(activeIt->second.hoveredValue.has_value());
		REQUIRE_FALSE(activeIt->second.pressedValue.has_value());
		REQUIRE_FALSE(activeIt->second.disabledValue.has_value());
	}
	
	SECTION("Mixed regular and styleState serialization")
	{
		// 通常のinteractionStateとstyleStateの両方を含むテスト
		noco::PropertyValue<int32> sizeProp = noco::PropertyValue<int32>{ 100 }
			.withHovered(110)
			.withPressed(90)
			.withSmoothTime(0.15)
			.withStyleStateInteraction(U"focused", noco::InteractionState::Default, 105)
			.withStyleStateInteraction(U"focused", noco::InteractionState::Hovered, 115);
		
		// JSONへシリアライズ
		JSON json = sizeProp.toJSON();
		
		// interactionStateのキーを確認
		REQUIRE(json.contains(U"default"));
		REQUIRE(json.contains(U"hovered"));
		REQUIRE(json.contains(U"pressed"));
		
		// styleStates内のキーを確認
		const JSON& focusedJson = json[U"styleStates"][U"focused"];
		REQUIRE(focusedJson.contains(U"default"));
		REQUIRE(focusedJson.contains(U"hovered"));
		
		// デシリアライズして確認
		auto deserializedProp = noco::PropertyValue<int32>::fromJSON(json);
		REQUIRE(deserializedProp.defaultValue() == 100);
		REQUIRE(*deserializedProp.hoveredValue() == 110);
		REQUIRE(*deserializedProp.pressedValue() == 90);
		
		auto focusedIt = deserializedProp.styleStateValues()->find(U"focused");
		REQUIRE(focusedIt->second.defaultValue == 105);
		REQUIRE(*focusedIt->second.hoveredValue == 115);
	}
	
	SECTION("Enum type serialization with styleState")
	{
		// Enum型でのテスト
		noco::PropertyValue<CursorStyle> cursorProp = noco::PropertyValue<CursorStyle>{ CursorStyle::Default }
			.withHovered(CursorStyle::Hand)
			.withPressed(CursorStyle::Cross)
			.withStyleStateInteraction(U"busy", noco::InteractionState::Default, CursorStyle::Hidden)
			.withStyleStateInteraction(U"busy", noco::InteractionState::Hovered, CursorStyle::NotAllowed);
		
		// JSONへシリアライズ
		JSON json = cursorProp.toJSON();
		
		// キーの確認
		REQUIRE(json.contains(U"default"));
		REQUIRE(json.contains(U"hovered"));
		REQUIRE(json.contains(U"pressed"));
		
		const JSON& busyJson = json[U"styleStates"][U"busy"];
		REQUIRE(busyJson.contains(U"default"));
		REQUIRE(busyJson.contains(U"hovered"));
		
		// デシリアライズして確認
		auto deserializedProp = noco::PropertyValue<CursorStyle>::fromJSON(json);
		REQUIRE(deserializedProp.defaultValue() == CursorStyle::Default);
		REQUIRE(*deserializedProp.hoveredValue() == CursorStyle::Hand);
		REQUIRE(*deserializedProp.pressedValue() == CursorStyle::Cross);
		
		auto busyIt = deserializedProp.styleStateValues()->find(U"busy");
		REQUIRE(busyIt->second.defaultValue == CursorStyle::Hidden);
		REQUIRE(*busyIt->second.hoveredValue == CursorStyle::NotAllowed);
	}
}

TEST_CASE("PropertyValue fromJSON type checking", "[PropertyValue]")
{
	SECTION("PropertyValue<bool> requires JSON boolean")
	{
		// 正しいbool値
		JSON validJson = true;
		auto validProp = noco::PropertyValue<bool>::fromJSON(validJson);
		REQUIRE(validProp.defaultValue() == true);
		
		// 文字列は受け付けない（デフォルト値になる）
		JSON stringJson = U"true";
		auto stringProp = noco::PropertyValue<bool>::fromJSON(stringJson);
		REQUIRE(stringProp.defaultValue() == false);  // デフォルト値
		
		// 数値も受け付けない
		JSON numberJson = 1;
		auto numberProp = noco::PropertyValue<bool>::fromJSON(numberJson);
		REQUIRE(numberProp.defaultValue() == false);  // デフォルト値
	}
	
	SECTION("PropertyValue<int32> requires JSON number")
	{
		// 正しい数値
		JSON validJson = 42;
		auto validProp = noco::PropertyValue<int32>::fromJSON(validJson);
		REQUIRE(validProp.defaultValue() == 42);
		
		// 文字列は受け付けない（デフォルト値になる）
		JSON stringJson = U"42";
		auto stringProp = noco::PropertyValue<int32>::fromJSON(stringJson);
		REQUIRE(stringProp.defaultValue() == 0);  // デフォルト値
		
		// boolも受け付けない
		JSON boolJson = true;
		auto boolProp = noco::PropertyValue<int32>::fromJSON(boolJson);
		REQUIRE(boolProp.defaultValue() == 0);  // デフォルト値
	}
	
	SECTION("PropertyValue<double> requires JSON number")
	{
		// 正しい数値
		JSON validJson = 3.14;
		auto validProp = noco::PropertyValue<double>::fromJSON(validJson);
		REQUIRE(validProp.defaultValue() == Approx(3.14));
		
		// 文字列は受け付けない（デフォルト値になる）
		JSON stringJson = U"3.14";
		auto stringProp = noco::PropertyValue<double>::fromJSON(stringJson);
		REQUIRE(stringProp.defaultValue() == 0.0);  // デフォルト値
	}
	
	SECTION("PropertyValue<String> requires JSON string")
	{
		// 正しい文字列
		JSON validJson = U"test";
		auto validProp = noco::PropertyValue<String>::fromJSON(validJson);
		REQUIRE(validProp.defaultValue() == U"test");
		
		// 数値は受け付けない（デフォルト値になる）
		JSON numberJson = 123;
		auto numberProp = noco::PropertyValue<String>::fromJSON(numberJson);
		REQUIRE(numberProp.defaultValue() == U"");  // デフォルト値
		
		// boolも受け付けない
		JSON boolJson = false;
		auto boolProp = noco::PropertyValue<String>::fromJSON(boolJson);
		REQUIRE(boolProp.defaultValue() == U"");  // デフォルト値
	}
	
	SECTION("PropertyValue with object format")
	{
		// オブジェクト形式でも型チェックが行われる
		JSON objJson;
		objJson[U"default"] = U"42";  // 文字列
		objJson[U"hovered"] = 50;  // 数値（正しい）
		
		auto prop = noco::PropertyValue<int32>::fromJSON(objJson);
		REQUIRE(prop.defaultValue() == 0);  // 文字列は受け付けない
		REQUIRE(prop.hoveredValue().has_value());
		REQUIRE(*prop.hoveredValue() == 50);  // 数値は正しく読み込まれる
	}
}

