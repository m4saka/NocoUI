#include <catch2/catch.hpp>
#include <NocoUI.hpp>

using namespace noco;

TEST_CASE("Param creation and basic operations", "[Param]")
{
	SECTION("Create parameters with different types")
	{
		auto boolParam = std::make_shared<Param>(U"bool", true);
		REQUIRE(boolParam->name() == U"bool");
		REQUIRE(boolParam->type() == ParamType::Bool);
		
		auto numberParam = std::make_shared<Param>(U"num", 42);
		REQUIRE(numberParam->name() == U"num");
		REQUIRE(numberParam->type() == ParamType::Number);
		
		auto stringParam = std::make_shared<Param>(U"str", U"test");
		REQUIRE(stringParam->name() == U"str");
		REQUIRE(stringParam->type() == ParamType::String);
		
		auto colorParam = std::make_shared<Param>(U"col", Color{255, 0, 0});
		REQUIRE(colorParam->name() == U"col");
		REQUIRE(colorParam->type() == ParamType::Color);
		
		auto vec2Param = std::make_shared<Param>(U"v2", Vec2{1, 2});
		REQUIRE(vec2Param->name() == U"v2");
		REQUIRE(vec2Param->type() == ParamType::Vec2);
		
		auto lrtbParam = std::make_shared<Param>(U"lrtb", LRTB{10, 20, 30, 40});
		REQUIRE(lrtbParam->name() == U"lrtb");
		REQUIRE(lrtbParam->type() == ParamType::LRTB);
	}
	
	SECTION("Get and set values")
	{
		auto param = std::make_shared<Param>(U"test", 42);
		
		// 値の取得（valueAsOptを使用）
		auto intValue = param->valueAsOpt<int32>();
		REQUIRE(intValue.has_value());
		REQUIRE(*intValue == 42);
		
		// 値の変更
		param->setValue(100);
		auto newIntValue = param->valueAsOpt<int32>();
		REQUIRE(newIntValue.has_value());
		REQUIRE(*newIntValue == 100);
	}
	
	SECTION("Type checking functions")
	{
		auto boolParam = std::make_shared<Param>(U"bool", true);
		REQUIRE(boolParam->isBool());
		REQUIRE_FALSE(boolParam->isNumber());
		REQUIRE_FALSE(boolParam->isString());
		
		auto numberParam = std::make_shared<Param>(U"num", 42);
		REQUIRE_FALSE(numberParam->isBool());
		REQUIRE(numberParam->isNumber());
		REQUIRE_FALSE(numberParam->isString());
		
		auto stringParam = std::make_shared<Param>(U"str", U"test");
		REQUIRE_FALSE(stringParam->isBool());
		REQUIRE_FALSE(stringParam->isNumber());
		REQUIRE(stringParam->isString());
		
		auto colorParam = std::make_shared<Param>(U"col", Color{255, 0, 0});
		REQUIRE(colorParam->isColor());
		REQUIRE_FALSE(colorParam->isVec2());
		
		auto vec2Param = std::make_shared<Param>(U"v2", Vec2{1, 2});
		REQUIRE_FALSE(vec2Param->isColor());
		REQUIRE(vec2Param->isVec2());
		
		auto lrtbParam = std::make_shared<Param>(U"lrtb", LRTB{10, 20, 30, 40});
		REQUIRE(lrtbParam->isLRTB());
		REQUIRE_FALSE(lrtbParam->isNumber());
	}
}

TEST_CASE("Param type conversions", "[Param]")
{
	SECTION("Arithmetic type conversions")
	{
		auto param = std::make_shared<Param>(U"number", 3.14);
		
		// floatへの変換
		auto floatVal = param->valueAsOpt<float>();
		REQUIRE(floatVal.has_value());
		REQUIRE(*floatVal == Approx(3.14f));
		
		// int32への変換（切り捨て）
		auto intVal = param->valueAsOpt<int32>();
		REQUIRE(intVal.has_value());
		REQUIRE(*intVal == 3);
		
		// uint8への変換
		auto byteVal = param->valueAsOpt<uint8>();
		REQUIRE(byteVal.has_value());
		REQUIRE(*byteVal == 3);
	}
	
	SECTION("Unsigned type handling")
	{
		// 正の値
		auto positiveParam = std::make_shared<Param>(U"positive", 100);
		auto uintValue = positiveParam->valueAsOpt<uint32>();
		REQUIRE(uintValue.has_value());
		REQUIRE(*uintValue == 100);
		
		// 負の値（0にクランプされる）
		auto negativeParam = std::make_shared<Param>(U"negative", -50);
		auto clampedValue = negativeParam->valueAsOpt<uint32>();
		REQUIRE(clampedValue.has_value());
		REQUIRE(*clampedValue == 0);
	}
	
	SECTION("Color to ColorF implicit conversion")
	{
		// Palette定数（Color型）をParamに渡してColorFとして正しく取得できることを確認
		auto param = std::make_shared<Param>(U"color", Palette::Yellow);
		
		auto colorF = param->valueAsOpt<ColorF>();
		REQUIRE(colorF.has_value());
		REQUIRE(colorF->r == Approx(1.0));
		REQUIRE(colorF->g == Approx(1.0));
		REQUIRE(colorF->b == Approx(0.0));
		REQUIRE(colorF->a == Approx(1.0));
		
		auto param2 = std::make_shared<Param>(U"color2", Palette::Red);
		auto colorF2 = param2->valueAsOpt<ColorF>();
		REQUIRE(colorF2.has_value());
		REQUIRE(colorF2->r == Approx(1.0));
		REQUIRE(colorF2->g == Approx(0.0));
		REQUIRE(colorF2->b == Approx(0.0));
		REQUIRE(colorF2->a == Approx(1.0));
	}
	
	SECTION("valueAs with fallback")
	{
		auto param = std::make_shared<Param>(U"test", 42);
		
		// 正しい型で取得
		REQUIRE(param->valueAs<int32>() == 42);
		REQUIRE(param->valueAs<double>() == Approx(42.0));
		
		// 間違った型で取得（fallback値が返る）
		REQUIRE(param->valueAs<String>(U"fallback") == U"fallback");
		REQUIRE(param->valueAs<bool>(true) == true);
		REQUIRE(param->valueAs<Vec2>(Vec2{1, 2}) == Vec2{1, 2});
	}
}

TEST_CASE("Canvas parameter management", "[Param]")
{
	SECTION("Add and retrieve parameters")
	{
		auto canvas = Canvas::Create();
		
		canvas->setParam(Param{U"testInt", 42});
		canvas->setParam(Param{U"testString", U"Hello"});
		
		auto retrievedInt = canvas->getParam(U"testInt");
		REQUIRE(retrievedInt.has_value());
		REQUIRE(retrievedInt->type() == ParamType::Number);
		
		auto intValue = retrievedInt->valueAsOpt<int32>();
		REQUIRE(intValue.has_value());
		REQUIRE(*intValue == 42);
		
		// 存在しないパラメータ
		auto notFound = canvas->getParam(U"notExist");
		REQUIRE(!notFound.has_value());
	}
	
	SECTION("Remove and clear parameters")
	{
		auto canvas = Canvas::Create();
		
		canvas->setParam(Param{U"param1", 1});
		canvas->setParam(Param{U"param2", 2});
		canvas->setParam(Param{U"param3", 3});
		
		REQUIRE(canvas->params().size() == 3);
		
		// 特定のパラメータを削除
		canvas->removeParam(U"param2");
		REQUIRE(canvas->params().size() == 2);
		REQUIRE(!canvas->getParam(U"param2").has_value());
		REQUIRE(canvas->getParam(U"param1").has_value());
		REQUIRE(canvas->getParam(U"param3").has_value());
		
		// すべてのパラメータをクリア
		canvas->clearParams();
		REQUIRE(canvas->params().size() == 0);
		REQUIRE(!canvas->getParam(U"param1").has_value());
	}
}

TEST_CASE("Parameter binding to properties", "[Param]")
{
	SECTION("Bind to Property")
	{
		auto canvas = Canvas::Create();
		auto node = Node::Create();
		canvas->rootNode()->addChild(node);
		
		// パラメータを作成
		canvas->setParam(Param{U"labelText", U"Hello World"});
		
		// LabelのプロパティはgetPropertyByNameでアクセス
		auto label = node->emplaceComponent<Label>(U"Initial");
		auto textProperty = static_cast<Property<String>*>(label->getPropertyByName(U"text"));
		REQUIRE(textProperty != nullptr);
		textProperty->setParamRef(U"labelText");
		
		// update時にパラメータ値が適用されることを確認
		canvas->update();
		REQUIRE(textProperty->value() == U"Hello World");
		
		// パラメータを更新
		canvas->setParam(Param{U"labelText", U"Updated Text"});
		canvas->update();
		REQUIRE(textProperty->value() == U"Updated Text");
	}
	
	SECTION("Bind to PropertyNonInteractive")
	{
		// PropertyNonInteractiveをテストするために、Transformのプロパティを使用
		auto canvas = Canvas::Create();
		auto node = Node::Create();
		canvas->rootNode()->addChild(node);
		
		// Vec2パラメータを作成
		canvas->setParam(Param{U"translateParam", Vec2{100, 200}});
		
		// Transformのtranslateプロパティにパラメータをバインド
		node->transform().translate().setParamRef(U"translateParam");
		
		// update時にパラメータ値が適用されることを確認
		canvas->update();
		REQUIRE(node->transform().translate().value() == Vec2{100, 200});
		
		// パラメータを更新
		canvas->setParam(Param{U"translateParam", Vec2{300, 400}});
		canvas->update();
		REQUIRE(node->transform().translate().value() == Vec2{300, 400});
	}
	
	SECTION("Dynamic parameter binding in update")
	{
		// コンポーネントのupdate内でsetParamRefを呼んでも、
		// 同じフレームのdraw時点で反映されることを確認
		auto canvas = Canvas::Create();
		auto node = Node::Create();
		canvas->rootNode()->addChild(node);
		
		// パラメータを作成
		canvas->setParam(Param{U"dynamicText", U"Dynamic Value"});
		
		// カスタムコンポーネントでupdate内でsetParamRefを呼ぶ
		class TestComponent : public ComponentBase
		{
		public:
			TestComponent() : ComponentBase({})
			{
			}
			bool firstUpdate = true;
			Property<String>* textProperty = nullptr;
			
			void update(const std::shared_ptr<Node>& node) override
			{
				if (firstUpdate && textProperty)
				{
					textProperty->setParamRef(U"dynamicText");
					firstUpdate = false;
				}
			}
			
			void draw(const Node&) const override {}
		};
		
		auto label = node->emplaceComponent<Label>(U"Initial");
		auto testComponent = std::make_shared<TestComponent>();
		testComponent->textProperty = static_cast<Property<String>*>(label->getPropertyByName(U"text"));
		node->addComponent(testComponent);
		
		// 最初のフレーム前はまだ反映されていない
		auto textProperty = static_cast<Property<String>*>(label->getPropertyByName(U"text"));
		REQUIRE(textProperty->value() == U"Initial");
		
		// update実行
		canvas->update();
		
		// update後（draw時点）では反映されている
		REQUIRE(textProperty->value() == U"Dynamic Value");
	}
}

TEST_CASE("Param serialization", "[Param]")
{
	SECTION("Canvas with params serialization")
	{
		auto canvas = Canvas::Create();
		
		// 単純な値のパラメータ
		canvas->setParam(Param{U"bool", true});
		canvas->setParam(Param{U"int", 123});
		canvas->setParam(Param{U"double", 3.14});
		canvas->setParam(Param{U"string", U"test"});
		canvas->setParam(Param{U"vec2", Vec2{10, 20}});
		canvas->setParam(Param{U"color", Color{255, 128, 64, 255}});
		
		// JSON化
		auto json = canvas->toJSON();
		REQUIRE(json.contains(U"params"));
		REQUIRE(json[U"params"].isArray());
		REQUIRE(json[U"params"].size() == 6);
		
		// 別のCanvasに読み込み
		auto canvas2 = Canvas::Create();
		REQUIRE(canvas2->tryReadFromJSON(json));
		
		// パラメータが復元されていることを確認
		auto boolParam = canvas2->getParam(U"bool");
		REQUIRE(boolParam.has_value());
		auto boolValue = boolParam->valueAsOpt<bool>();
		REQUIRE(boolValue.has_value());
		REQUIRE(*boolValue == true);
		
		auto intParam = canvas2->getParam(U"int");
		REQUIRE(intParam.has_value());
		auto intValue = intParam->valueAsOpt<int32>();
		REQUIRE(intValue.has_value());
		REQUIRE(*intValue == 123);
		
		auto doubleParam = canvas2->getParam(U"double");
		REQUIRE(doubleParam.has_value());
		auto doubleValue = doubleParam->valueAsOpt<double>();
		REQUIRE(doubleValue.has_value());
		REQUIRE(*doubleValue == Approx(3.14));
		
		auto stringParam = canvas2->getParam(U"string");
		REQUIRE(stringParam.has_value());
		auto stringValue = stringParam->valueAsOpt<String>();
		REQUIRE(stringValue.has_value());
		REQUIRE(*stringValue == U"test");
		
		auto vec2Param = canvas2->getParam(U"vec2");
		REQUIRE(vec2Param.has_value());
		auto vec2Value = vec2Param->valueAsOpt<Vec2>();
		REQUIRE(vec2Value.has_value());
		REQUIRE(*vec2Value == Vec2{10, 20});
		
		auto colorParam = canvas2->getParam(U"color");
		REQUIRE(colorParam.has_value());
		auto colorValue = colorParam->valueAsOpt<Color>();
		REQUIRE(colorValue.has_value());
		REQUIRE(*colorValue == Color{255, 128, 64, 255});
	}
}