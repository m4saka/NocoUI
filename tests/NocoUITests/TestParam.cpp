#include <catch2/catch.hpp>
#include <NocoUI.hpp>

using namespace noco;

// ========================================
// Paramのテスト
// ========================================

TEST_CASE("Parameter values", "[Param]")
{
	SECTION("Create and get parameter values")
	{
		// bool型
		ParamValue boolParam = MakeParamValue(true);
		REQUIRE(GetParamType(boolParam) == ParamType::Bool);
		REQUIRE(GetParamValueAs<bool>(boolParam).value_or(false) == true);
		
		// number型
		ParamValue numberParam = MakeParamValue(42);
		REQUIRE(GetParamType(numberParam) == ParamType::Number);
		REQUIRE(GetParamValueAs<int32>(numberParam).value_or(0) == 42);
		
		// string型
		ParamValue stringParam = MakeParamValue(U"test");
		REQUIRE(GetParamType(stringParam) == ParamType::String);
		REQUIRE(GetParamValueAs<String>(stringParam).value_or(U"") == U"test");
		
		// color型
		ParamValue colorParam = MakeParamValue(Color{255, 0, 0});
		REQUIRE(GetParamType(colorParam) == ParamType::Color);
		auto colorValue = GetParamValueAs<Color>(colorParam);
		REQUIRE(colorValue.has_value());
		REQUIRE(colorValue->r == 255);
		
		// vec2型
		ParamValue vec2Param = MakeParamValue(Vec2{1, 2});
		REQUIRE(GetParamType(vec2Param) == ParamType::Vec2);
		auto vec2Value = GetParamValueAs<Vec2>(vec2Param);
		REQUIRE(vec2Value.has_value());
		REQUIRE(vec2Value->x == 1);
		REQUIRE(vec2Value->y == 2);
		
		// lrtb型
		ParamValue lrtbParam = MakeParamValue(LRTB{10, 20, 30, 40});
		REQUIRE(GetParamType(lrtbParam) == ParamType::LRTB);
		auto lrtbValue = GetParamValueAs<LRTB>(lrtbParam);
		REQUIRE(lrtbValue.has_value());
		REQUIRE(lrtbValue->left == 10);
	}
	
	SECTION("Number conversions")
	{
		// 各種数値型のテスト
		ParamValue intParam = MakeParamValue(42);
		REQUIRE(GetParamValueAs<int32>(intParam).value_or(0) == 42);
		REQUIRE(GetParamValueAs<uint32>(intParam).value_or(0) == 42);
		REQUIRE(GetParamValueAs<float>(intParam).value_or(0.0f) == 42.0f);
		REQUIRE(GetParamValueAs<double>(intParam).value_or(0.0) == 42.0);
		
		// 負の値
		ParamValue negativeParam = MakeParamValue(-50);
		REQUIRE(GetParamValueAs<int32>(negativeParam).value_or(0) == -50);
		// unsignedへの変換では0になる
		REQUIRE(GetParamValueAs<uint32>(negativeParam).value_or(1) == 0);
	}
	
	SECTION("Color conversions")
	{
		// Colorのパラメータ処理
		ParamValue colorParam = MakeParamValue(Palette::Yellow);

		// Colorとして取得
		auto color = GetParamValueAs<Color>(colorParam);
		REQUIRE(color.has_value());
		REQUIRE(color->r == 255);
		REQUIRE(color->g == 255);
		REQUIRE(color->b == 0);
	}
}

TEST_CASE("Canvas parameter management", "[Param]")
{
	SECTION("Add and get parameters")
	{
		auto canvas = Canvas::Create();
		
		canvas->setParamValue(U"testInt", 42);
		canvas->setParamValue(U"testString", U"Hello");
		
		auto retrievedInt = canvas->param(U"testInt");
		REQUIRE(retrievedInt.has_value());
		REQUIRE(GetParamType(*retrievedInt) == ParamType::Number);
		
		auto intValue = canvas->paramValueOpt<int32>(U"testInt");
		REQUIRE(intValue.has_value());
		REQUIRE(*intValue == 42);
		
		// 存在しないパラメータ
		auto notFound = canvas->param(U"notExist");
		REQUIRE(!notFound.has_value());
	}
	
	SECTION("Batch set parameters")
	{
		auto canvas = Canvas::Create();
		
		canvas->setParamValues({
			{U"param1", 100},
			{U"param2", U"test"},
			{U"param3", Vec2{10, 20}},
			{U"param4", true},
			{U"param5", Color{128, 128, 128}}
		});
		
		REQUIRE(canvas->paramValueOpt<int32>(U"param1").value_or(0) == 100);
		REQUIRE(canvas->paramValueOpt<String>(U"param2").value_or(U"") == U"test");
		auto vec = canvas->paramValueOpt<Vec2>(U"param3");
		REQUIRE(vec.has_value());
		REQUIRE(vec->x == 10);
		REQUIRE(vec->y == 20);
		REQUIRE(canvas->paramValueOpt<bool>(U"param4").value_or(false) == true);
		auto color = canvas->paramValueOpt<Color>(U"param5");
		REQUIRE(color.has_value());
		REQUIRE(color->r == 128);
	}
	
	SECTION("Remove and clear parameters")
	{
		auto canvas = Canvas::Create();
		
		canvas->setParamValue(U"param1", 1);
		canvas->setParamValue(U"param2", 2);
		canvas->setParamValue(U"param3", 3);
		
		REQUIRE(canvas->params().size() == 3);
		
		// 特定のパラメータを削除
		canvas->removeParam(U"param2");
		REQUIRE(canvas->params().size() == 2);
		REQUIRE(!canvas->param(U"param2").has_value());
		REQUIRE(canvas->param(U"param1").has_value());
		REQUIRE(canvas->param(U"param3").has_value());
		
		// すべてのパラメータをクリア
		canvas->clearParams();
		REQUIRE(canvas->params().size() == 0);
		REQUIRE(!canvas->param(U"param1").has_value());
	}
}

TEST_CASE("Parameter binding to properties", "[Param]")
{
	SECTION("Bind parameter to Label text")
	{
		auto canvas = Canvas::Create();
		auto node = Node::Create();
		canvas->addChild(node);
		
		// パラメータを作成
		canvas->setParamValue(U"labelText", U"Hello World");
		
		// LabelのプロパティはgetPropertyByNameでアクセス
		auto label = node->emplaceComponent<Label>(U"Initial");
		auto* textProperty = dynamic_cast<Property<String>*>(label->getPropertyByName(U"text"));
		REQUIRE(textProperty != nullptr);
		textProperty->setParamRef(U"labelText");
		
		// update時にパラメータ値が適用されることを確認
		canvas->update();
		REQUIRE(textProperty->value() == U"Hello World");
		
		// パラメータを更新
		canvas->setParamValue(U"labelText", U"Updated Text");
		canvas->update();
		REQUIRE(textProperty->value() == U"Updated Text");
	}
	
	SECTION("Bind parameter to transform position")
	{
		auto canvas = Canvas::Create();
		auto node = Node::Create();
		canvas->addChild(node);
		
		// Vec2パラメータを作成
		canvas->setParamValue(U"translateParam", Vec2{100, 200});
		
		// Transformのtranslateプロパティにパラメータをバインド
		node->transform().translate().setParamRef(U"translateParam");
		
		// update時にパラメータ値が適用されることを確認
		canvas->update();
		REQUIRE(node->transform().translate().value() == Vec2{100, 200});
		
		// パラメータを更新
		canvas->setParamValue(U"translateParam", Vec2{300, 400});
		canvas->update();
		REQUIRE(node->transform().translate().value() == Vec2{300, 400});
	}
	
	SECTION("Dynamic parameter binding in update")
	{
		// コンポーネントのupdate内でsetParamRefを呼んでも、
		// 同じフレームのdraw時点で反映されることを確認
		auto canvas = Canvas::Create();
		auto node = Node::Create();
		canvas->addChild(node);
		
		// パラメータを作成
		canvas->setParamValue(U"dynamicText", U"Dynamic Value");
		
		class TestComponent : public noco::ComponentBase
		{
		public:
			TestComponent() : noco::ComponentBase{ {} }
			{
			}
			bool firstUpdate = true;
			noco::Property<String>* textProperty = nullptr;
			
			void update(const std::shared_ptr<noco::Node>& node) override
			{
				if (firstUpdate && textProperty)
				{
					textProperty->setParamRef(U"dynamicText");
					firstUpdate = false;
				}
			}
			
			void draw(const noco::Node&) const override {}
		};
		
		// テストコンポーネントとラベルを追加
		auto testComponent = std::make_shared<TestComponent>();
		node->addComponent(testComponent);
		
		auto label = node->emplaceComponent<Label>(U"Initial");
		auto* textProperty = dynamic_cast<Property<String>*>(label->getPropertyByName(U"text"));
		testComponent->textProperty = textProperty;
		
		// 最初のupdate
		canvas->update();
		// update内でsetParamRefが呼ばれ、同じフレームで反映される
		REQUIRE(textProperty->value() == U"Dynamic Value");
	}
}

TEST_CASE("Canvas parameter serialization", "[Param]")
{
	SECTION("Save and load parameters")
	{
		// パラメータを持つCanvasを作成
		auto canvas1 = Canvas::Create();
		canvas1->setParamValue(U"bool", true);
		canvas1->setParamValue(U"number", 123.45);
		canvas1->setParamValue(U"string", U"test");
		canvas1->setParamValue(U"vec2", Vec2{10, 20});
		canvas1->setParamValue(U"color", Color{255, 128, 64, 255});
		
		JSON json = canvas1->toJSON();
		
		// 新しいCanvasに読み込み
		auto canvas2 = Canvas::Create();
		REQUIRE(canvas2->tryReadFromJSON(json));
		
		// パラメータが正しく復元されていることを確認
		auto boolParam = canvas2->paramValueOpt<bool>(U"bool");
		REQUIRE(boolParam.has_value());
		REQUIRE(*boolParam == true);
		
		auto numberParam = canvas2->paramValueOpt<double>(U"number");
		REQUIRE(numberParam.has_value());
		REQUIRE(*numberParam == Approx(123.45));
		
		auto stringParam = canvas2->paramValueOpt<String>(U"string");
		REQUIRE(stringParam.has_value());
		REQUIRE(*stringParam == U"test");
		
		auto vec2Param = canvas2->paramValueOpt<Vec2>(U"vec2");
		REQUIRE(vec2Param.has_value());
		REQUIRE(*vec2Param == Vec2{10, 20});
		
		auto colorParam = canvas2->paramValueOpt<Color>(U"color");
		REQUIRE(colorParam.has_value());
		REQUIRE(colorParam->r == 255);
		REQUIRE(colorParam->g == 128);
		REQUIRE(colorParam->b == 64);
		REQUIRE(colorParam->a == 255);
	}
}

TEST_CASE("Parameter edge cases and error handling", "[Param]")
{
	SECTION("Update existing parameter")
	{
		auto canvas = Canvas::Create();
		
		// 初期値を設定
		canvas->setParamValue(U"counter", 10);
		REQUIRE(canvas->paramValueOpt<int32>(U"counter").value_or(0) == 10);
		
		// 同じ名前で値を上書き
		canvas->setParamValue(U"counter", 20);
		REQUIRE(canvas->paramValueOpt<int32>(U"counter").value_or(0) == 20);
		
		// 型を変更して上書き
		canvas->setParamValue(U"counter", U"text");
		REQUIRE(canvas->paramValueOpt<int32>(U"counter").has_value() == false);
		REQUIRE(canvas->paramValueOpt<String>(U"counter").value_or(U"") == U"text");
	}
	
	SECTION("Type mismatch access")
	{
		auto canvas = Canvas::Create();
		canvas->setParamValue(U"number", 42);
		
		// 正しい型でのアクセス
		REQUIRE(canvas->paramValueOpt<int32>(U"number").value_or(0) == 42);
		
		// 間違った型でのアクセス
		REQUIRE(canvas->paramValueOpt<String>(U"number").has_value() == false);
		REQUIRE(canvas->paramValueOpt<bool>(U"number").has_value() == false);
		REQUIRE(canvas->paramValueOpt<Vec2>(U"number").has_value() == false);
	}
	
	SECTION("Parameter binding with deletion")
	{
		auto canvas = Canvas::Create();
		auto node = Node::Create();
		canvas->addChild(node);
		
		// パラメータを作成してバインド
		canvas->setParamValue(U"testParam", U"Initial");
		auto label = node->emplaceComponent<Label>(U"Default");
		auto* textProperty = dynamic_cast<Property<String>*>(label->getPropertyByName(U"text"));
		textProperty->setParamRef(U"testParam");
		
		canvas->update();
		REQUIRE(textProperty->value() == U"Initial");
		
		// パラメータを削除
		canvas->removeParam(U"testParam");
		canvas->update();
		// パラメータが削除されても、プロパティは最後の値を保持
		REQUIRE(textProperty->value() == U"Initial");
		
		// 参照がクリアされていることを確認
		REQUIRE(textProperty->hasParamRef() == true);  // 参照自体は残る
		REQUIRE(canvas->param(U"testParam").has_value() == false);
	}
	
	SECTION("Empty parameter name is rejected")
	{
		auto canvas = Canvas::Create();
		
		// 空文字列の名前でパラメータ設定は拒否される
		canvas->setParamValue(U"", 100);
		
		// 空文字列のパラメータは追加されない
		REQUIRE(canvas->param(U"").has_value() == false);
		REQUIRE(canvas->paramValueOpt<int32>(U"").value_or(0) == 0);
		
		// パラメータが追加されていないことを確認
		REQUIRE(canvas->params().size() == 0);
	}
	
	SECTION("Large number of parameters")
	{
		auto canvas = Canvas::Create();
		
		// 1000個のパラメータを設定
		for (int32 i = 0; i < 1000; ++i)
		{
			canvas->setParamValue(U"param_{}"_fmt(i), i);
		}
		
		// サイズ確認
		REQUIRE(canvas->params().size() == 1000);
		
		// ランダムアクセステスト
		REQUIRE(canvas->paramValueOpt<int32>(U"param_500").value_or(-1) == 500);
		REQUIRE(canvas->paramValueOpt<int32>(U"param_999").value_or(-1) == 999);
		REQUIRE(canvas->paramValueOpt<int32>(U"param_0").value_or(-1) == 0);
		
		// 全削除
		canvas->clearParams();
		REQUIRE(canvas->params().size() == 0);
	}
	
	SECTION("Invalid parameter reference cleanup")
	{
		auto canvas = Canvas::Create();
		auto node = Node::Create();
		canvas->addChild(node);
		
		// プロパティに存在しないパラメータ参照を設定
		auto label = node->emplaceComponent<Label>(U"Test");
		auto* textProperty = dynamic_cast<Property<String>*>(label->getPropertyByName(U"text"));
		textProperty->setParamRef(U"nonExistent");
		
		// 存在しないパラメータを参照しても、エラーにならない
		canvas->update();
		REQUIRE(textProperty->value() == U"Test");  // デフォルト値のまま
		
		// 後からパラメータを追加すると反映される
		canvas->setParamValue(U"nonExistent", U"NewValue");
		canvas->update();
		REQUIRE(textProperty->value() == U"NewValue");
	}
}

TEST_CASE("Parameter name validation", "[Param]")
{
	SECTION("IsValidParameterName function")
	{
		REQUIRE(IsValidParameterName(U"validName") == true);
		REQUIRE(IsValidParameterName(U"ValidName") == true);
		REQUIRE(IsValidParameterName(U"VALID_NAME") == true);
		REQUIRE(IsValidParameterName(U"valid_name_123") == true);
		REQUIRE(IsValidParameterName(U"v") == true); // 1文字
		REQUIRE(IsValidParameterName(U"V") == true); // 1文字大文字
		REQUIRE(IsValidParameterName(U"myParam123") == true);
		REQUIRE(IsValidParameterName(U"param_with_underscores") == true);
		REQUIRE(IsValidParameterName(U"CONSTANT_VALUE") == true);
		REQUIRE(IsValidParameterName(U"m_memberVariable") == true);
		REQUIRE(IsValidParameterName(U"value123456789") == true);
		REQUIRE(IsValidParameterName(U"abcdefghijklmnopqrstuvwxyz") == true);
		REQUIRE(IsValidParameterName(U"ABCDEFGHIJKLMNOPQRSTUVWXYZ") == true);
		REQUIRE(IsValidParameterName(U"a0123456789_") == true);
		REQUIRE(IsValidParameterName(U"_validName") == true); // アンダースコアで始まる
		REQUIRE(IsValidParameterName(U"_") == true); // アンダースコアのみ
		REQUIRE(IsValidParameterName(U"__double") == true); // 連続アンダースコアで始まる
		REQUIRE(IsValidParameterName(U"_test_value") == true);
		REQUIRE(IsValidParameterName(U"_123") == true); // アンダースコア＋数字
		
		REQUIRE(IsValidParameterName(U"") == false); // 空文字列
		REQUIRE(IsValidParameterName(U"123invalid") == false); // 数字で始まる
		REQUIRE(IsValidParameterName(U"9param") == false); // 数字で始まる
		REQUIRE(IsValidParameterName(U"invalid-name") == false);
		REQUIRE(IsValidParameterName(U"invalid.name") == false);
		REQUIRE(IsValidParameterName(U"invalid name") == false);
		REQUIRE(IsValidParameterName(U"invalid!name") == false);
		REQUIRE(IsValidParameterName(U"invalid@name") == false);
		REQUIRE(IsValidParameterName(U"invalid#name") == false);
		REQUIRE(IsValidParameterName(U"invalid$name") == false);
		REQUIRE(IsValidParameterName(U"invalid%name") == false);
		REQUIRE(IsValidParameterName(U"invalid^name") == false);
		REQUIRE(IsValidParameterName(U"invalid&name") == false);
		REQUIRE(IsValidParameterName(U"invalid*name") == false);
		REQUIRE(IsValidParameterName(U"invalid(name") == false);
		REQUIRE(IsValidParameterName(U"invalid)name") == false);
		REQUIRE(IsValidParameterName(U"invalid[name") == false);
		REQUIRE(IsValidParameterName(U"invalid]name") == false);
		REQUIRE(IsValidParameterName(U"invalid{name") == false);
		REQUIRE(IsValidParameterName(U"invalid}name") == false);
		REQUIRE(IsValidParameterName(U"invalid/name") == false);
		REQUIRE(IsValidParameterName(U"invalid\\name") == false);
		REQUIRE(IsValidParameterName(U"invalid|name") == false);
		REQUIRE(IsValidParameterName(U"invalid+name") == false);
		REQUIRE(IsValidParameterName(U"invalid=name") == false);
		REQUIRE(IsValidParameterName(U"日本語") == false); // 日本語
		REQUIRE(IsValidParameterName(U"パラメータ") == false); // カタカナ
		REQUIRE(IsValidParameterName(U"param日本語") == false);
		REQUIRE(IsValidParameterName(U"😀emoji") == false);
		REQUIRE(IsValidParameterName(U"param😀") == false);
	}
	
	SECTION("Valid parameter names are accepted")
	{
		auto canvas = Canvas::Create();
		
		canvas->setParamValue(U"validName", 1);
		canvas->setParamValue(U"validName2", 2);
		canvas->setParamValue(U"valid_name_3", 3);
		canvas->setParamValue(U"VALID_NAME", 4);
		canvas->setParamValue(U"v", 5); // 1文字でも有効
		canvas->setParamValue(U"ValidName123_456", 6);
		
		REQUIRE(canvas->hasParam(U"validName"));
		REQUIRE(canvas->hasParam(U"validName2"));
		REQUIRE(canvas->hasParam(U"valid_name_3"));
		REQUIRE(canvas->hasParam(U"VALID_NAME"));
		REQUIRE(canvas->hasParam(U"v"));
		REQUIRE(canvas->hasParam(U"ValidName123_456"));
		
		REQUIRE(canvas->params().size() == 6);
	}
	
	SECTION("Invalid parameter names are rejected in JSON loading")
	{
		JSON json;
		json[U"referenceSize"] = Array<double>{ 800.0, 600.0 }; // Canvas作成に必要
		json[U"version"] = noco::NocoUIVersion;
		json[U"serializedVersion"] = noco::CurrentSerializedVersion;
		json[U"children"] = Array<JSON>{}; // Canvas作成に必要
		json[U"params"] = JSON{};
		json[U"params"][U"validParam"] = JSON{};
		json[U"params"][U"validParam"][U"type"] = U"Number";
		json[U"params"][U"validParam"][U"value"] = 100;
		
		json[U"params"][U"123invalid"] = JSON{}; // 数字で始まる
		json[U"params"][U"123invalid"][U"type"] = U"Number";
		json[U"params"][U"123invalid"][U"value"] = 200;
		
		json[U"params"][U"invalid-name"] = JSON{};
		json[U"params"][U"invalid-name"][U"type"] = U"Number";
		json[U"params"][U"invalid-name"][U"value"] = 300;
		
		json[U"params"][U"日本語"] = JSON{}; // 日本語
		json[U"params"][U"日本語"][U"type"] = U"Number";
		json[U"params"][U"日本語"][U"value"] = 400;
		
		json[U"params"][U"_validParam"] = JSON{};
		json[U"params"][U"_validParam"][U"type"] = U"Number";
		json[U"params"][U"_validParam"][U"value"] = 500;
		
		auto canvas = Canvas::Create();
		REQUIRE(canvas != nullptr); // まずキャンバス作成確認
		REQUIRE(canvas->tryReadFromJSON(json));
		
		REQUIRE(canvas->hasParam(U"validParam"));
		REQUIRE(canvas->paramValueOpt<double>(U"validParam").value_or(0) == 100);
		REQUIRE(canvas->hasParam(U"_validParam"));
		REQUIRE(canvas->paramValueOpt<double>(U"_validParam").value_or(0) == 500);
		
		REQUIRE(!canvas->hasParam(U"123invalid"));
		REQUIRE(!canvas->hasParam(U"invalid-name"));
		REQUIRE(!canvas->hasParam(U"日本語"));
		
		REQUIRE(canvas->params().size() == 2);
	}
	
	SECTION("Invalid parameter names are not saved to JSON")
	{
		auto canvas = Canvas::Create();
		
		canvas->setParamValue(U"validParam", 100);
		canvas->setParamValue(U"_validParam", 150); // アンダースコアで始まる（有効）
		
		auto& mutableParams = const_cast<HashTable<String, ParamValue>&>(canvas->params());
		mutableParams[U"123invalid"] = MakeParamValue(200);
		mutableParams[U"invalid-name"] = MakeParamValue(300);
		mutableParams[U"日本語"] = MakeParamValue(400);
		
		JSON json = canvas->toJSON();
		
		REQUIRE(json.contains(U"params"));
		REQUIRE(json[U"params"].isObject());
		
		REQUIRE(json[U"params"].contains(U"validParam"));
		REQUIRE(json[U"params"].contains(U"_validParam"));
		
		REQUIRE(!json[U"params"].contains(U"123invalid"));
		REQUIRE(!json[U"params"].contains(U"invalid-name"));
		REQUIRE(!json[U"params"].contains(U"日本語"));
		
		REQUIRE(json[U"params"].size() == 2);
	}
	
	SECTION("Invalid parameter names are rejected when adding through API")
	{
		auto canvas = Canvas::Create();
		
		canvas->setParamValue(U"123invalid", 100); // 数字で始まる
		canvas->setParamValue(U"invalid-name", 200);
		canvas->setParamValue(U"invalid name", 300);
		canvas->setParamValue(U"日本語", 400); // 日本語
		canvas->setParamValue(U"", 500); // 空文字列
		
		canvas->setParamValue(U"validParam", 600);
		canvas->setParamValue(U"_validName", 700); // アンダースコアで始まる（有効）
		
		REQUIRE(!canvas->hasParam(U"123invalid"));
		REQUIRE(!canvas->hasParam(U"invalid-name"));
		REQUIRE(!canvas->hasParam(U"invalid name"));
		REQUIRE(!canvas->hasParam(U"日本語"));
		REQUIRE(!canvas->hasParam(U""));
		
		REQUIRE(canvas->hasParam(U"validParam"));
		REQUIRE(canvas->paramValueOpt<double>(U"validParam").value_or(0) == 600);
		REQUIRE(canvas->hasParam(U"_validName"));
		REQUIRE(canvas->paramValueOpt<double>(U"_validName").value_or(0) == 700);
		
		REQUIRE(canvas->params().size() == 2);
	}
}

TEST_CASE("ParamValueFromJSON type checking", "[Param]")
{
	SECTION("Bool type rejects string values")
	{
		JSON json;
		json[U"type"] = U"Bool";
		json[U"value"] = U"true";  // 文字列
		
		auto result = ParamValueFromJSON(json);
		REQUIRE(!result.has_value());  // 型不一致でnoneを返す
	}
	
	SECTION("Bool type accepts boolean values")
	{
		JSON json;
		json[U"type"] = U"Bool";
		json[U"value"] = true;  // 正しいbool型
		
		auto result = ParamValueFromJSON(json);
		REQUIRE(result.has_value());
		REQUIRE(GetParamType(*result) == ParamType::Bool);
		REQUIRE(GetParamValueAs<bool>(*result).value_or(false) == true);
	}
	
	SECTION("Number type rejects string values")
	{
		JSON json;
		json[U"type"] = U"Number";
		json[U"value"] = U"42";  // 文字列
		
		auto result = ParamValueFromJSON(json);
		REQUIRE(!result.has_value());  // 型不一致でnoneを返す
	}
	
	SECTION("Number type accepts numeric values")
	{
		JSON json;
		json[U"type"] = U"Number";
		json[U"value"] = 42.5;  // 正しい数値型
		
		auto result = ParamValueFromJSON(json);
		REQUIRE(result.has_value());
		REQUIRE(GetParamType(*result) == ParamType::Number);
		REQUIRE(GetParamValueAs<double>(*result).value_or(0.0) == 42.5);
	}
	
	SECTION("String type rejects non-string values")
	{
		JSON json;
		json[U"type"] = U"String";
		json[U"value"] = 123;  // 数値
		
		auto result = ParamValueFromJSON(json);
		REQUIRE(!result.has_value());  // 型不一致でnoneを返す
	}
	
	SECTION("String type accepts string values")
	{
		JSON json;
		json[U"type"] = U"String";
		json[U"value"] = U"test";  // 正しい文字列型
		
		auto result = ParamValueFromJSON(json);
		REQUIRE(result.has_value());
		REQUIRE(GetParamType(*result) == ParamType::String);
		REQUIRE(GetParamValueAs<String>(*result).value_or(U"") == U"test");
	}
	
	SECTION("Color type as array format")
	{
		JSON json;
		json[U"type"] = U"Color";
		json[U"value"] = Array<int32>{ 255, 0, 0, 255 };  // 配列形式

		auto result = ParamValueFromJSON(json);
		REQUIRE(result.has_value());
		REQUIRE(GetParamType(*result) == ParamType::Color);

		auto color = GetParamValueAs<Color>(*result);
		REQUIRE(color.has_value());
		REQUIRE(color->r == 255);
		REQUIRE(color->g == 0);
		REQUIRE(color->b == 0);
		REQUIRE(color->a == 255);
	}

	SECTION("Color type with string format returns default")
	{
		JSON json;
		json[U"type"] = U"Color";
		json[U"value"] = U"#FF0000FF";  // 文字列形式はエラー

		auto result = ParamValueFromJSON(json);
		REQUIRE(result.has_value());
		REQUIRE(GetParamType(*result) == ParamType::Color);

		auto color = GetParamValueAs<Color>(*result);
		REQUIRE(color.has_value());
		// デフォルト値(0, 0, 0, 0)が返される
		REQUIRE(color->r == 0);
		REQUIRE(color->g == 0);
		REQUIRE(color->b == 0);
		REQUIRE(color->a == 0);
	}

	SECTION("Vec2 type as array format")
	{
		JSON json;
		json[U"type"] = U"Vec2";
		json[U"value"] = Array<double>{ 100, 200 };  // 配列形式

		auto result = ParamValueFromJSON(json);
		REQUIRE(result.has_value());
		REQUIRE(GetParamType(*result) == ParamType::Vec2);

		auto vec = GetParamValueAs<Vec2>(*result);
		REQUIRE(vec.has_value());
		REQUIRE(vec->x == Approx(100));
		REQUIRE(vec->y == Approx(200));
	}

	SECTION("Vec2 type with string format returns default")
	{
		JSON json;
		json[U"type"] = U"Vec2";
		json[U"value"] = U"(100, 200)";  // 文字列形式はエラー

		auto result = ParamValueFromJSON(json);
		REQUIRE(result.has_value());
		REQUIRE(GetParamType(*result) == ParamType::Vec2);

		auto vec = GetParamValueAs<Vec2>(*result);
		REQUIRE(vec.has_value());
		// デフォルト値(0, 0)が返される
		REQUIRE(vec->x == Approx(0));
		REQUIRE(vec->y == Approx(0));
	}

	SECTION("LRTB type as array format")
	{
		JSON json;
		json[U"type"] = U"LRTB";
		json[U"value"] = Array<double>{ 10, 20, 30, 40 };  // 配列形式

		auto result = ParamValueFromJSON(json);
		REQUIRE(result.has_value());
		REQUIRE(GetParamType(*result) == ParamType::LRTB);

		auto lrtb = GetParamValueAs<LRTB>(*result);
		REQUIRE(lrtb.has_value());
		REQUIRE(lrtb->left == Approx(10));
		REQUIRE(lrtb->right == Approx(20));
		REQUIRE(lrtb->top == Approx(30));
		REQUIRE(lrtb->bottom == Approx(40));
	}

	SECTION("LRTB type with string format returns default")
	{
		JSON json;
		json[U"type"] = U"LRTB";
		json[U"value"] = U"(10, 20, 30, 40)";  // 文字列形式はエラー

		auto result = ParamValueFromJSON(json);
		REQUIRE(result.has_value());
		REQUIRE(GetParamType(*result) == ParamType::LRTB);

		auto lrtb = GetParamValueAs<LRTB>(*result);
		REQUIRE(lrtb.has_value());
		// デフォルト値(0, 0, 0, 0)が返される
		REQUIRE(lrtb->left == Approx(0));
		REQUIRE(lrtb->right == Approx(0));
		REQUIRE(lrtb->top == Approx(0));
		REQUIRE(lrtb->bottom == Approx(0));
	}
	
	SECTION("Missing required fields")
	{
		// typeフィールドがない場合
		JSON noType;
		noType[U"value"] = 42;
		auto resultNoType = ParamValueFromJSON(noType);
		REQUIRE(!resultNoType.has_value());

		// valueフィールドがない場合
		JSON noValue;
		noValue[U"type"] = U"Number";
		auto resultNoValue = ParamValueFromJSON(noValue);
		REQUIRE(!resultNoValue.has_value());

		// 両方のフィールドがない場合
		JSON empty;
		auto resultEmpty = ParamValueFromJSON(empty);
		REQUIRE(!resultEmpty.has_value());
	}

	SECTION("Canvas params are saved as arrays in JSON")
	{
		// Canvasにパラメータを設定
		auto canvas = Canvas::Create();
		canvas->setParamValue(U"testColor", Color{255, 128, 64, 32});
		canvas->setParamValue(U"testVec2", Vec2{100.5, 200.25});
		canvas->setParamValue(U"testLRTB", LRTB{10.1, 20.2, 30.3, 40.4});
		canvas->setParamValue(U"testBool", true);
		canvas->setParamValue(U"testNumber", 42.5);
		canvas->setParamValue(U"testString", U"test value");

		// JSON形式で保存
		JSON json = canvas->toJSON();

		// paramsオブジェクトが存在することを確認
		REQUIRE(json.contains(U"params"));
		REQUIRE(json[U"params"].isObject());

		// 各パラメータが正しい形式で保存されているか検証
		const JSON& params = json[U"params"];

		// Colorは配列形式 [r, g, b, a]
		REQUIRE(params.contains(U"testColor"));
		REQUIRE(params[U"testColor"].contains(U"type"));
		REQUIRE(params[U"testColor"][U"type"].getString() == U"Color");
		REQUIRE(params[U"testColor"].contains(U"value"));
		REQUIRE(params[U"testColor"][U"value"].isArray());
		REQUIRE(params[U"testColor"][U"value"].size() == 4);
		REQUIRE(params[U"testColor"][U"value"][0].get<int32>() == 255);
		REQUIRE(params[U"testColor"][U"value"][1].get<int32>() == 128);
		REQUIRE(params[U"testColor"][U"value"][2].get<int32>() == 64);
		REQUIRE(params[U"testColor"][U"value"][3].get<int32>() == 32);

		// Vec2は配列形式 [x, y]
		REQUIRE(params.contains(U"testVec2"));
		REQUIRE(params[U"testVec2"][U"type"].getString() == U"Vec2");
		REQUIRE(params[U"testVec2"][U"value"].isArray());
		REQUIRE(params[U"testVec2"][U"value"].size() == 2);
		REQUIRE(params[U"testVec2"][U"value"][0].get<double>() == Approx(100.5));
		REQUIRE(params[U"testVec2"][U"value"][1].get<double>() == Approx(200.25));

		// LRTBは配列形式 [left, right, top, bottom]
		REQUIRE(params.contains(U"testLRTB"));
		REQUIRE(params[U"testLRTB"][U"type"].getString() == U"LRTB");
		REQUIRE(params[U"testLRTB"][U"value"].isArray());
		REQUIRE(params[U"testLRTB"][U"value"].size() == 4);
		REQUIRE(params[U"testLRTB"][U"value"][0].get<double>() == Approx(10.1));
		REQUIRE(params[U"testLRTB"][U"value"][1].get<double>() == Approx(20.2));
		REQUIRE(params[U"testLRTB"][U"value"][2].get<double>() == Approx(30.3));
		REQUIRE(params[U"testLRTB"][U"value"][3].get<double>() == Approx(40.4));

		// boolは直接bool値として保存
		REQUIRE(params.contains(U"testBool"));
		REQUIRE(params[U"testBool"][U"type"].getString() == U"Bool");
		REQUIRE(params[U"testBool"][U"value"].isBool());
		REQUIRE(params[U"testBool"][U"value"].get<bool>() == true);

		// numberは直接double値として保存
		REQUIRE(params.contains(U"testNumber"));
		REQUIRE(params[U"testNumber"][U"type"].getString() == U"Number");
		REQUIRE(params[U"testNumber"][U"value"].isNumber());
		REQUIRE(params[U"testNumber"][U"value"].get<double>() == Approx(42.5));

		// stringは直接文字列として保存
		REQUIRE(params.contains(U"testString"));
		REQUIRE(params[U"testString"][U"type"].getString() == U"String");
		REQUIRE(params[U"testString"][U"value"].isString());
		REQUIRE(params[U"testString"][U"value"].getString() == U"test value");

		// デバッグ用: 実際のJSON出力を確認
		Logger << U"JSON output: " << Unicode::FromUTF8(json.formatUTF8());
	}

	SECTION("Component PropertyValues are saved as arrays in JSON")
	{
		// Canvasを作成してコンポーネントプロパティを設定
		auto canvas = Canvas::Create();
		auto node = Node::Create();
		canvas->addChild(node);

		// Color、Vec2、LRTBプロパティを持つコンポーネントを追加
		auto rectRenderer = node->emplaceComponent<RectRenderer>();
		rectRenderer->setFillColor(Color{200, 100, 50, 25});

		auto label = node->emplaceComponent<Label>();
		label->setColor(Color{150, 75, 25, 200});

		// Transformも確認
		node->transform().setColor(Color{180, 90, 45, 220});
		node->transform().setTranslate(Vec2{150.5, 250.75});
		node->transform().setScale(Vec2{1.5, 2.0});

		// JSON形式で保存
		JSON json = canvas->toJSON();

		// 最初の子ノードを取得
		REQUIRE(json.contains(U"children"));
		REQUIRE(json[U"children"].isArray());
		REQUIRE(json[U"children"].size() > 0);

		const JSON& childNode = json[U"children"][0];

		// RectRendererのfillColorプロパティを確認
		REQUIRE(childNode.contains(U"components"));
		REQUIRE(childNode[U"components"].isArray());

		// RectRendererコンポーネントを検索
		bool foundRectRenderer = false;
		for (size_t i = 0; i < childNode[U"components"].size(); ++i)
		{
			const JSON& component = childNode[U"components"][i];
			if (component[U"type"].getString() == U"RectRenderer")
			{
				foundRectRenderer = true;
				// fillColorが配列形式 [r, g, b, a] で保存されているか確認
				const JSON& fillColor = component[U"fillColor"];

				// PropertyValueは配列形式で保存されるべき
				REQUIRE(fillColor.isArray());
				REQUIRE(fillColor.size() == 4);
				REQUIRE(fillColor[0].get<int32>() == 200);
				REQUIRE(fillColor[1].get<int32>() == 100);
				REQUIRE(fillColor[2].get<int32>() == 50);
				REQUIRE(fillColor[3].get<int32>() == 25);
				break;
			}
		}
		REQUIRE(foundRectRenderer);

		// Labelコンポーネントのcolorプロパティを確認
		bool foundLabel = false;
		for (size_t i = 0; i < childNode[U"components"].size(); ++i)
		{
			const JSON& component = childNode[U"components"][i];
			if (component[U"type"].getString() == U"Label")
			{
				foundLabel = true;
				const JSON& color = component[U"color"];

				// PropertyValueは配列形式で保存されるべき
				REQUIRE(color.isArray());
				REQUIRE(color.size() == 4);
				REQUIRE(color[0].get<int32>() == 150);
				REQUIRE(color[1].get<int32>() == 75);
				REQUIRE(color[2].get<int32>() == 25);
				REQUIRE(color[3].get<int32>() == 200);
				break;
			}
		}
		REQUIRE(foundLabel);

		// Transformのプロパティを確認
		REQUIRE(childNode.contains(U"transform"));
		const JSON& transform = childNode[U"transform"];

		// Transform colorプロパティ
		REQUIRE(transform.contains(U"color"));
		const JSON& transformColor = transform[U"color"];
		// PropertyValueは配列形式で保存されるべき
		REQUIRE(transformColor.isArray());
		REQUIRE(transformColor.size() == 4);
		REQUIRE(transformColor[0].get<int32>() == 180);
		REQUIRE(transformColor[1].get<int32>() == 90);
		REQUIRE(transformColor[2].get<int32>() == 45);
		REQUIRE(transformColor[3].get<int32>() == 220);

		// Transform translateプロパティ
		REQUIRE(transform.contains(U"translate"));
		const JSON& translateValue = transform[U"translate"];
		// PropertyValueは配列形式で保存されるべき
		REQUIRE(translateValue.isArray());
		REQUIRE(translateValue.size() == 2);
		REQUIRE(translateValue[0].get<double>() == Approx(150.5));
		REQUIRE(translateValue[1].get<double>() == Approx(250.75));

		// Transform scaleプロパティ
		REQUIRE(transform.contains(U"scale"));
		const JSON& scaleValue = transform[U"scale"];
		// PropertyValueは配列形式で保存されるべき
		REQUIRE(scaleValue.isArray());
		REQUIRE(scaleValue.size() == 2);
		REQUIRE(scaleValue[0].get<double>() == Approx(1.5));
		REQUIRE(scaleValue[1].get<double>() == Approx(2.0));
	}
}
