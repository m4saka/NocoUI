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
		
		REQUIRE(deserializedProp.defaultValue == ColorF{ 1.0, 0.0, 0.0, 1.0 });
		REQUIRE(deserializedProp.hoveredValue().has_value());
		REQUIRE(*deserializedProp.hoveredValue() == ColorF{ 0.0, 1.0, 0.0, 1.0 });
		REQUIRE(deserializedProp.pressedValue().has_value());
		REQUIRE(*deserializedProp.pressedValue() == ColorF{ 0.0, 0.0, 1.0, 1.0 });
		REQUIRE(deserializedProp.disabledValue().has_value());
		REQUIRE(*deserializedProp.disabledValue() == ColorF{ 0.5, 0.5, 0.5, 1.0 });
		REQUIRE(deserializedProp.smoothTime == 0.3);
	}
	
	SECTION("StyleState serialization")
	{
		// styleState内のinteractionStateのシリアライズテスト
		noco::PropertyValue<double> alphaProp{ 1.0 };
		alphaProp.smoothTime = 0.2;
		
		// styleStateValues を初期化
		alphaProp.styleStateValues = std::make_unique<HashTable<String, noco::PropertyStyleStateValue<double>>>();
		
		// "selected" スタイル状態を追加
		noco::PropertyStyleStateValue<double> selectedValues{ 0.8 }; // default
		selectedValues.hoveredValue = 0.9;
		selectedValues.pressedValue = 0.7;
		selectedValues.disabledValue = 0.4;
		(*alphaProp.styleStateValues)[U"selected"] = selectedValues;
		
		// "active" スタイル状態を追加（defaultのみ）
		noco::PropertyStyleStateValue<double> activeValues{ 0.95 };
		(*alphaProp.styleStateValues)[U"active"] = activeValues;
		
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
		
		REQUIRE(deserializedProp.defaultValue == 1.0);
		REQUIRE(deserializedProp.smoothTime == 0.2);
		REQUIRE(deserializedProp.styleStateValues != nullptr);
		
		// "selected"の値を確認
		auto selectedIt = deserializedProp.styleStateValues->find(U"selected");
		REQUIRE(selectedIt != deserializedProp.styleStateValues->end());
		REQUIRE(selectedIt->second.defaultValue == 0.8);
		REQUIRE(selectedIt->second.hoveredValue.has_value());
		REQUIRE(*selectedIt->second.hoveredValue == 0.9);
		REQUIRE(selectedIt->second.pressedValue.has_value());
		REQUIRE(*selectedIt->second.pressedValue == 0.7);
		REQUIRE(selectedIt->second.disabledValue.has_value());
		REQUIRE(*selectedIt->second.disabledValue == 0.4);
		
		// "active"の値を確認
		auto activeIt = deserializedProp.styleStateValues->find(U"active");
		REQUIRE(activeIt != deserializedProp.styleStateValues->end());
		REQUIRE(activeIt->second.defaultValue == 0.95);
		REQUIRE_FALSE(activeIt->second.hoveredValue.has_value());
		REQUIRE_FALSE(activeIt->second.pressedValue.has_value());
		REQUIRE_FALSE(activeIt->second.disabledValue.has_value());
	}
	
	SECTION("Mixed regular and styleState serialization")
	{
		// 通常のinteractionStateとstyleStateの両方を含むテスト
		noco::PropertyValue<int32> sizeProp{ 100 };
		sizeProp = sizeProp.withHovered(110).withPressed(90);
		sizeProp.smoothTime = 0.15;
		
		// styleStateも追加
		sizeProp.styleStateValues = std::make_unique<HashTable<String, noco::PropertyStyleStateValue<int32>>>();
		noco::PropertyStyleStateValue<int32> focusedValues{ 105 };
		focusedValues.hoveredValue = 115;
		(*sizeProp.styleStateValues)[U"focused"] = focusedValues;
		
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
		REQUIRE(deserializedProp.defaultValue == 100);
		REQUIRE(*deserializedProp.hoveredValue() == 110);
		REQUIRE(*deserializedProp.pressedValue() == 90);
		
		auto focusedIt = deserializedProp.styleStateValues->find(U"focused");
		REQUIRE(focusedIt->second.defaultValue == 105);
		REQUIRE(*focusedIt->second.hoveredValue == 115);
	}
	
	SECTION("Enum type serialization with styleState")
	{
		// Enum型でのテスト
		noco::PropertyValue<CursorStyle> cursorProp{ CursorStyle::Default };
		cursorProp = cursorProp.withHovered(CursorStyle::Hand).withPressed(CursorStyle::Cross);
		
		// styleState追加
		cursorProp.styleStateValues = std::make_unique<HashTable<String, noco::PropertyStyleStateValue<CursorStyle>>>();
		noco::PropertyStyleStateValue<CursorStyle> busyValues{ CursorStyle::Hidden };
		busyValues.hoveredValue = CursorStyle::NotAllowed;
		(*cursorProp.styleStateValues)[U"busy"] = busyValues;
		
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
		REQUIRE(deserializedProp.defaultValue == CursorStyle::Default);
		REQUIRE(*deserializedProp.hoveredValue() == CursorStyle::Hand);
		REQUIRE(*deserializedProp.pressedValue() == CursorStyle::Cross);
		
		auto busyIt = deserializedProp.styleStateValues->find(U"busy");
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
		REQUIRE(validProp.defaultValue == true);
		
		// 文字列は受け付けない（デフォルト値になる）
		JSON stringJson = U"true";
		auto stringProp = noco::PropertyValue<bool>::fromJSON(stringJson);
		REQUIRE(stringProp.defaultValue == false);  // デフォルト値
		
		// 数値も受け付けない
		JSON numberJson = 1;
		auto numberProp = noco::PropertyValue<bool>::fromJSON(numberJson);
		REQUIRE(numberProp.defaultValue == false);  // デフォルト値
	}
	
	SECTION("PropertyValue<int32> requires JSON number")
	{
		// 正しい数値
		JSON validJson = 42;
		auto validProp = noco::PropertyValue<int32>::fromJSON(validJson);
		REQUIRE(validProp.defaultValue == 42);
		
		// 文字列は受け付けない（デフォルト値になる）
		JSON stringJson = U"42";
		auto stringProp = noco::PropertyValue<int32>::fromJSON(stringJson);
		REQUIRE(stringProp.defaultValue == 0);  // デフォルト値
		
		// boolも受け付けない
		JSON boolJson = true;
		auto boolProp = noco::PropertyValue<int32>::fromJSON(boolJson);
		REQUIRE(boolProp.defaultValue == 0);  // デフォルト値
	}
	
	SECTION("PropertyValue<double> requires JSON number")
	{
		// 正しい数値
		JSON validJson = 3.14;
		auto validProp = noco::PropertyValue<double>::fromJSON(validJson);
		REQUIRE(validProp.defaultValue == Approx(3.14));
		
		// 文字列は受け付けない（デフォルト値になる）
		JSON stringJson = U"3.14";
		auto stringProp = noco::PropertyValue<double>::fromJSON(stringJson);
		REQUIRE(stringProp.defaultValue == 0.0);  // デフォルト値
	}
	
	SECTION("PropertyValue<String> requires JSON string")
	{
		// 正しい文字列
		JSON validJson = U"test";
		auto validProp = noco::PropertyValue<String>::fromJSON(validJson);
		REQUIRE(validProp.defaultValue == U"test");
		
		// 数値は受け付けない（デフォルト値になる）
		JSON numberJson = 123;
		auto numberProp = noco::PropertyValue<String>::fromJSON(numberJson);
		REQUIRE(numberProp.defaultValue == U"");  // デフォルト値
		
		// boolも受け付けない
		JSON boolJson = false;
		auto boolProp = noco::PropertyValue<String>::fromJSON(boolJson);
		REQUIRE(boolProp.defaultValue == U"");  // デフォルト値
	}
	
	SECTION("PropertyValue with object format")
	{
		// オブジェクト形式でも型チェックが行われる
		JSON objJson;
		objJson[U"default"] = U"42";  // 文字列
		objJson[U"hovered"] = 50;  // 数値（正しい）
		
		auto prop = noco::PropertyValue<int32>::fromJSON(objJson);
		REQUIRE(prop.defaultValue == 0);  // 文字列は受け付けない
		REQUIRE(prop.hoveredValue().has_value());
		REQUIRE(*prop.hoveredValue() == 50);  // 数値は正しく読み込まれる
	}
}

