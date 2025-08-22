#include <catch2/catch.hpp>
#include <Siv3D.hpp>
#include <NocoUI.hpp>

// ========================================
// PropertyValueのシリアライズに関するテスト
// ========================================

TEST_CASE("PropertyValue JSON serialization", "[PropertyValue][Serialization]")
{
	SECTION("Regular interaction states serialization")
	{
		// interactionStateのシリアライズテスト
		noco::PropertyValue<ColorF> colorProp{ ColorF{ 1.0, 0.0, 0.0, 1.0 } }; // 赤（default）
		colorProp.hoveredValue = ColorF{ 0.0, 1.0, 0.0, 1.0 }; // 緑（hovered）
		colorProp.pressedValue = ColorF{ 0.0, 0.0, 1.0, 1.0 }; // 青（pressed）
		colorProp.disabledValue = ColorF{ 0.5, 0.5, 0.5, 1.0 }; // グレー（disabled）
		colorProp.smoothTime = 0.3;
		
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
		REQUIRE(deserializedProp.hoveredValue.has_value());
		REQUIRE(*deserializedProp.hoveredValue == ColorF{ 0.0, 1.0, 0.0, 1.0 });
		REQUIRE(deserializedProp.pressedValue.has_value());
		REQUIRE(*deserializedProp.pressedValue == ColorF{ 0.0, 0.0, 1.0, 1.0 });
		REQUIRE(deserializedProp.disabledValue.has_value());
		REQUIRE(*deserializedProp.disabledValue == ColorF{ 0.5, 0.5, 0.5, 1.0 });
		REQUIRE(deserializedProp.smoothTime == 0.3);
	}
	
	SECTION("StyleState serialization")
	{
		// styleState内のinteractionStateのシリアライズテスト
		noco::PropertyValue<double> alphaProp{ 1.0 };
		alphaProp.smoothTime = 0.2;
		
		// styleStateValues を初期化
		alphaProp.styleStateValues = std::make_unique<HashTable<String, noco::InteractionValues<double>>>();
		
		// "selected" スタイル状態を追加
		noco::InteractionValues<double> selectedValues{ 0.8 }; // default
		selectedValues.hoveredValue = 0.9;
		selectedValues.pressedValue = 0.7;
		selectedValues.disabledValue = 0.4;
		(*alphaProp.styleStateValues)[U"selected"] = selectedValues;
		
		// "active" スタイル状態を追加（defaultのみ）
		noco::InteractionValues<double> activeValues{ 0.95 };
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
		sizeProp.hoveredValue = 110;
		sizeProp.pressedValue = 90;
		sizeProp.smoothTime = 0.15;
		
		// styleStateも追加
		sizeProp.styleStateValues = std::make_unique<HashTable<String, noco::InteractionValues<int32>>>();
		noco::InteractionValues<int32> focusedValues{ 105 };
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
		REQUIRE(*deserializedProp.hoveredValue == 110);
		REQUIRE(*deserializedProp.pressedValue == 90);
		
		auto focusedIt = deserializedProp.styleStateValues->find(U"focused");
		REQUIRE(focusedIt->second.defaultValue == 105);
		REQUIRE(*focusedIt->second.hoveredValue == 115);
	}
	
	SECTION("Enum type serialization with styleState")
	{
		// Enum型でのテスト
		noco::PropertyValue<CursorStyle> cursorProp{ CursorStyle::Default };
		cursorProp.hoveredValue = CursorStyle::Hand;
		cursorProp.pressedValue = CursorStyle::Cross;
		
		// styleState追加
		cursorProp.styleStateValues = std::make_unique<HashTable<String, noco::InteractionValues<CursorStyle>>>();
		noco::InteractionValues<CursorStyle> busyValues{ CursorStyle::Hidden };
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
		REQUIRE(*deserializedProp.hoveredValue == CursorStyle::Hand);
		REQUIRE(*deserializedProp.pressedValue == CursorStyle::Cross);
		
		auto busyIt = deserializedProp.styleStateValues->find(U"busy");
		REQUIRE(busyIt->second.defaultValue == CursorStyle::Hidden);
		REQUIRE(*busyIt->second.hoveredValue == CursorStyle::NotAllowed);
	}
}

