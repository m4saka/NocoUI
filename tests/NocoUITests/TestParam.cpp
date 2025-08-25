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
		REQUIRE(GetParamValueAs<int>(numberParam).value_or(0) == 42);
		
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
		// ColorとColorFの相互変換
		ParamValue colorParam = MakeParamValue(Palette::Yellow);
		
		// ColorFとして取得
		auto colorF = GetParamValueAs<ColorF>(colorParam);
		REQUIRE(colorF.has_value());
		REQUIRE(colorF->r == Approx(1.0).epsilon(0.01));
		REQUIRE(colorF->g == Approx(1.0).epsilon(0.01));
		REQUIRE(colorF->b == Approx(0.0).epsilon(0.01));
		
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
		
		auto intValue = canvas->paramValueOpt<int>(U"testInt");
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
			{U"param5", ColorF{0.5, 0.5, 0.5}}
		});
		
		REQUIRE(canvas->paramValueOpt<int>(U"param1").value_or(0) == 100);
		REQUIRE(canvas->paramValueOpt<String>(U"param2").value_or(U"") == U"test");
		auto vec = canvas->paramValueOpt<Vec2>(U"param3");
		REQUIRE(vec.has_value());
		REQUIRE(vec->x == 10);
		REQUIRE(vec->y == 20);
		REQUIRE(canvas->paramValueOpt<bool>(U"param4").value_or(false) == true);
		auto color = canvas->paramValueOpt<ColorF>(U"param5");
		REQUIRE(color.has_value());
		REQUIRE(color->r == Approx(0.5));
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
		canvas1->setParamValue(U"color", ColorF{1.0, 0.5, 0.25, 1.0});
		
		// JSONに保存
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
		REQUIRE(canvas->paramValueOpt<int>(U"counter").value_or(0) == 10);
		
		// 同じ名前で値を上書き
		canvas->setParamValue(U"counter", 20);
		REQUIRE(canvas->paramValueOpt<int>(U"counter").value_or(0) == 20);
		
		// 型を変更して上書き
		canvas->setParamValue(U"counter", U"text");
		REQUIRE(canvas->paramValueOpt<int>(U"counter").has_value() == false);
		REQUIRE(canvas->paramValueOpt<String>(U"counter").value_or(U"") == U"text");
	}
	
	SECTION("Type mismatch access")
	{
		auto canvas = Canvas::Create();
		canvas->setParamValue(U"number", 42);
		
		// 正しい型でのアクセス
		REQUIRE(canvas->paramValueOpt<int>(U"number").value_or(0) == 42);
		
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
	
	SECTION("Empty parameter name handling")
	{
		auto canvas = Canvas::Create();
		
		// 空文字列の名前でパラメータ設定
		canvas->setParamValue(U"", 100);
		
		// 空文字列でも取得可能
		REQUIRE(canvas->param(U"").has_value() == true);
		REQUIRE(canvas->paramValueOpt<int>(U"").value_or(0) == 100);
		
		// 空文字列の削除も可能
		canvas->removeParam(U"");
		REQUIRE(canvas->param(U"").has_value() == false);
	}
	
	SECTION("Large number of parameters")
	{
		auto canvas = Canvas::Create();
		
		// 1000個のパラメータを設定
		for (int i = 0; i < 1000; ++i)
		{
			canvas->setParamValue(U"param_{}"_fmt(i), i);
		}
		
		// サイズ確認
		REQUIRE(canvas->params().size() == 1000);
		
		// ランダムアクセステスト
		REQUIRE(canvas->paramValueOpt<int>(U"param_500").value_or(-1) == 500);
		REQUIRE(canvas->paramValueOpt<int>(U"param_999").value_or(-1) == 999);
		REQUIRE(canvas->paramValueOpt<int>(U"param_0").value_or(-1) == 0);
		
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