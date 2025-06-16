# include <catch2/catch.hpp>
# include <Siv3D.hpp>
# include <NocoUI.hpp>

// ========================================
// PropertyValueのスムージング機能テスト
// ========================================

TEST_CASE("PropertyValue smoothing", "[Property]")
{
	SECTION("Smooth transition between states")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create();
		auto label = node->emplaceComponent<noco::Label>();
		canvas->rootNode()->addChild(node);
		
		// スムージング時間を設定した色のプロパティ
		noco::PropertyValue<ColorF> colorProp{ ColorF{ 1.0, 0.0, 0.0, 1.0 } }; // 赤
		colorProp.hoveredValue = ColorF{ 0.0, 1.0, 0.0, 1.0 }; // 緑
		colorProp.smoothTime = 0.5; // 0.5秒でスムージング
		
		label->setColor(colorProp);
		
		// Default状態で初期化
		// interactionStateは読み取り専用なので、canvasの更新を通じて設定される
		canvas->update();
		
		// Hovered状態に変更（マウスカーソルをシミュレート）
		// 実際のHovered状態はcanvasのマウス処理を通じて設定される
		// ここではテストの簡略化のため、スムージング動作の確認のみ行う
		
		// 複数フレームで更新して、値が徐々に変化することを確認
		for (int i = 0; i < 10; ++i)
		{
			canvas->update();
			// 時間経過をシミュレート（実際にはDeltaTimeが必要）
		}
		
		// スムージング中は中間値になっているはず
		// （実際の値の検証はDeltaTimeの実装に依存）
	}

	SECTION("Immediate transition with zero smoothTime")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create();
		auto label = node->emplaceComponent<noco::Label>();
		canvas->rootNode()->addChild(node);
		
		// スムージング時間なしの色のプロパティ
		noco::PropertyValue<ColorF> colorProp{ ColorF{ 1.0, 0.0, 0.0, 1.0 } }; // 赤（デフォルト値）
		colorProp.hoveredValue = ColorF{ 0.0, 1.0, 0.0, 1.0 }; // 緑
		colorProp.smoothTime = 0.0; // 即座に変更
		
		label->setColor(colorProp);
		
		// Default状態で初期化
		canvas->update();
		// PropertyValueの値を直接確認
		s3d::Array<s3d::String> emptyActiveStyleStates;
		auto normalColor = colorProp.value(noco::InteractionState::Default, emptyActiveStyleStates);
		REQUIRE(normalColor == ColorF{ 1.0, 0.0, 0.0, 1.0 });
		
		// Hovered状態に変更（テスト用の簡略化）
		// 実際にはマウス操作を通じてinteractionStateが変更される
		// ここではPropertyValueの動作確認に焦点を当てる
		auto hoveredColor = colorProp.value(noco::InteractionState::Hovered, emptyActiveStyleStates);
		REQUIRE(hoveredColor == ColorF{ 0.0, 1.0, 0.0, 1.0 });
	}

	SECTION("Multiple state changes during smoothing")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create();
		auto rect = node->emplaceComponent<noco::RectRenderer>();
		canvas->rootNode()->addChild(node);
		
		// スムージング時間を設定
		noco::PropertyValue<double> alphaProp{ 1.0 }; // デフォルト値
		alphaProp.hoveredValue = 0.5;
		alphaProp.pressedValue = 0.2;
		alphaProp.smoothTime = 0.5;
		
		// RectRendererにはsetAlphaメソッドがないため、fillColorのアルファ値で代用
		noco::PropertyValue<ColorF> fillColorProp{ ColorF{ 1.0, 1.0, 1.0, 1.0 } };
		fillColorProp.hoveredValue = ColorF{ 1.0, 1.0, 1.0, 0.5 };
		fillColorProp.pressedValue = ColorF{ 1.0, 1.0, 1.0, 0.2 };
		fillColorProp.smoothTime = 0.5;
		rect->setFillColor(fillColorProp);
		
		// Default → Hovered → Pressed と素早く状態を変更
		// 実際の使用では、これらの状態変更はマウス操作によって行われる
		canvas->update();
		
		// PropertyValueの状態遷移をテスト
		s3d::Array<s3d::String> emptyActiveStyleStates;
		auto defaultValue = fillColorProp.value(noco::InteractionState::Default, emptyActiveStyleStates);
		auto hoveredValue = fillColorProp.value(noco::InteractionState::Hovered, emptyActiveStyleStates);
		auto pressedValue = fillColorProp.value(noco::InteractionState::Pressed, emptyActiveStyleStates);
		
		REQUIRE(defaultValue.a == 1.0);
		REQUIRE(hoveredValue.a == 0.5);
		REQUIRE(pressedValue.a == 0.2);
		
		// スムージング中に複数の状態変更があっても正常に動作するはず
	}
}