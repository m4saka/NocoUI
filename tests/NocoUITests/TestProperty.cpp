#include <catch2/catch.hpp>
#include <Siv3D.hpp>
#include <NocoUI.hpp>

// ========================================
// Propertyのテスト
// ========================================

TEST_CASE("Property vs SmoothProperty behavior", "[Property]")
{
	SECTION("Property immediate value change")
	{
		noco::Property<double> property{ U"test", 100.0 };
		
		noco::PropertyValue<double> newValue{ 200.0 };
		property.setPropertyValue(newValue);
		
		// Propertyは即座に値が変わる
		property.update(noco::InteractionState::Default, {}, 0.016, {});
		REQUIRE(property.value() == 200.0);
		
		property.update(noco::InteractionState::Default, {}, 1.0, {});
		REQUIRE(property.value() == 200.0);
	}
	
	SECTION("SmoothProperty gradual value change")
	{
		noco::SmoothProperty<double> smoothProperty{ U"test", 100.0 };
		
		noco::PropertyValue<double> targetValue{ 200.0 };
		targetValue.smoothTime = 1.0;
		smoothProperty.setPropertyValue(targetValue);
		
		REQUIRE(smoothProperty.value() == 100.0);
		
		// 0.5秒後は100と200の中間値になる
		smoothProperty.update(noco::InteractionState::Default, {}, 0.5, {});
		double halfWayValue = smoothProperty.value();
		REQUIRE(halfWayValue > 100.0);
		REQUIRE(halfWayValue < 200.0);
		
		// さらに更新すると目標値に近づく
		smoothProperty.update(noco::InteractionState::Default, {}, 0.5, {});
		double laterValue = smoothProperty.value();
		REQUIRE(laterValue > halfWayValue);
		REQUIRE(laterValue <= 200.0);
	}
}

TEST_CASE("SmoothProperty smoothTime behavior", "[Property]")
{
	SECTION("Zero smoothTime acts like immediate change")
	{
		noco::SmoothProperty<ColorF> smoothColor{ U"color", ColorF{1,0,0} };
		
		noco::PropertyValue<ColorF> targetValue{ ColorF{0,1,0} };
		targetValue.smoothTime = 0.0;
		smoothColor.setPropertyValue(targetValue);
		
		// smoothTime=0なら即座に変わる
		smoothColor.update(noco::InteractionState::Default, {}, 0.016, {});
		REQUIRE(smoothColor.value() == ColorF{0,1,0});
	}
	
	SECTION("Different smoothTime affects transition speed")
	{
		noco::SmoothProperty<double> fastProperty{ U"fast", 0.0 };
		noco::SmoothProperty<double> slowProperty{ U"slow", 0.0 };
		
		noco::PropertyValue<double> fastTarget{ 100.0 };
		fastTarget.smoothTime = 0.1;
		noco::PropertyValue<double> slowTarget{ 100.0 };
		slowTarget.smoothTime = 1.0;
		
		fastProperty.setPropertyValue(fastTarget);
		slowProperty.setPropertyValue(slowTarget);
		
		fastProperty.update(noco::InteractionState::Default, {}, 0.05, {});
		slowProperty.update(noco::InteractionState::Default, {}, 0.05, {});
		
		// 短いsmoothTimeの方が目標値により近い
		REQUIRE(fastProperty.value() > slowProperty.value());
	}
}

TEST_CASE("Property parameter reference", "[Property][Param]")
{
	SECTION("SmoothProperty parameter binding")
	{
		noco::SmoothProperty<ColorF> colorProperty{ U"color", ColorF{1,0,0} };
		colorProperty.setParamRef(U"themeColor");
		
		HashTable<String, noco::ParamValue> params;
		params[U"themeColor"] = noco::MakeParamValue(ColorF{0,1,0});
		
		// パラメータ参照により値が変わる
		colorProperty.update(noco::InteractionState::Default, {}, 0.016, params);
		REQUIRE(colorProperty.value() == ColorF{0,1,0});
		
		params[U"themeColor"] = noco::MakeParamValue(ColorF{0,0,1});
		colorProperty.update(noco::InteractionState::Default, {}, 0.016, params);
		REQUIRE(colorProperty.value() == ColorF{0,0,1});
	}
	
	SECTION("Property parameter binding")
	{
		noco::Property<double> sizeProperty{ U"size", 100.0 };
		sizeProperty.setParamRef(U"baseSize");
		
		HashTable<String, noco::ParamValue> params;
		params[U"baseSize"] = noco::MakeParamValue(200.0);
		
		sizeProperty.update(noco::InteractionState::Default, {}, 0.016, params);
		REQUIRE(sizeProperty.value() == 200.0);
	}
	
	SECTION("Parameter reference cleared when param removed")
	{
		noco::Property<int32> intProperty{ U"value", 10 };
		intProperty.setParamRef(U"count");
		
		HashTable<String, noco::ParamValue> params;
		params[U"count"] = noco::MakeParamValue(50);
		intProperty.update(noco::InteractionState::Default, {}, 0.016, params);
		REQUIRE(intProperty.value() == 50);
		
		// パラメータを削除してフレームを進める
		params.clear();
		System::Update();
		intProperty.update(noco::InteractionState::Default, {}, 0.016, params);
		// パラメータが削除されると元の値に戻る
		REQUIRE(intProperty.value() == 10);
	}
}

TEST_CASE("SmoothProperty InteractionState transitions", "[Property]")
{
	SECTION("Smooth transition between interaction states")
	{
		noco::SmoothProperty<double> opacityProperty{ U"opacity", 1.0 };
		
		noco::PropertyValue<double> opacityValue{ 1.0 };
		opacityValue.hoveredValue = 0.8;
		opacityValue.pressedValue = 0.6;
		opacityValue.smoothTime = 0.5;
		opacityProperty.setPropertyValue(opacityValue);
		
		opacityProperty.update(noco::InteractionState::Default, {}, 0.016, {});
		REQUIRE(opacityProperty.value() == 1.0);
		
		// Hovered状態では目標値に向かって変化中
		opacityProperty.update(noco::InteractionState::Hovered, {}, 0.1, {});
		double hoveredValue = opacityProperty.value();
		REQUIRE(hoveredValue < 1.0);
		REQUIRE(hoveredValue > 0.8);
		
		// Pressed状態では更に小さい値へ変化中
		opacityProperty.update(noco::InteractionState::Pressed, {}, 0.1, {});
		double pressedValue = opacityProperty.value();
		REQUIRE(pressedValue < hoveredValue);
	}
}

TEST_CASE("Property currentFrameOverride", "[Property]")
{
	SECTION("SmoothProperty override behavior")
	{
		noco::SmoothProperty<Vec2> posProperty{ U"position", Vec2{0,0} };
		
		posProperty.update(noco::InteractionState::Default, {}, 0.016, {});
		REQUIRE(posProperty.value() == Vec2{0,0});
		
		// overrideにより一時的に値を変更
		posProperty.setCurrentFrameOverride(Vec2{100,200});
		REQUIRE(posProperty.value() == Vec2{100,200});
		REQUIRE(posProperty.hasCurrentFrameOverride());
	}
}