#include <catch2/catch.hpp>
#include <Siv3D.hpp>
#include <NocoUI.hpp>

// ========================================
// Tweenコンポーネントのテスト
// ========================================

TEST_CASE("Tween component", "[Tween]")
{
	SECTION("Manual mode without loopDuration - delay not included in loop")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create();
		canvas->addChild(node);

		// delay=1で0,0から100,100へduration=1で移動
		auto tween = std::make_shared<noco::Tween>();
		tween->setManualMode(true)
			->setActive(true)
			->setTranslateEnabled(true)
			->setTranslateFrom(Vec2{ 0.0, 0.0 })
			->setTranslateTo(Vec2{ 100.0, 100.0 })
			->setDelay(1.0)
			->setDuration(1.0)
			->setEasing(noco::TweenEasing::Linear)
			->setLoopType(noco::TweenLoopType::Loop)
			->setLoopDuration(0.0)  // loopDuration=0の場合、delayはループに含めない
			->setApplyDuringDelay(false);
		node->addComponent(tween);

		// Test cases
		struct TestCase
		{
			double manualTime;
			Vec2 expectedPos;
			String description;
		};

		Array<TestCase> testCases = {
			{ 0.0, Vec2{ 0.0, 0.0 }, U"t=0: delay期間中、初期位置" },
			{ 0.5, Vec2{ 0.0, 0.0 }, U"t=0.5: delay期間中、初期位置" },
			{ 1.0, Vec2{ 0.0, 0.0 }, U"t=1.0: delay終了、アニメーション開始時" },
			{ 1.5, Vec2{ 50.0, 50.0 }, U"t=1.5: アニメーション50%" },
			{ 2.0, Vec2{ 0.0, 0.0 }, U"t=2.0: アニメーション完了後、ループして0%に戻る" },
			{ 2.5, Vec2{ 50.0, 50.0 }, U"t=2.5: 2周目の50%" },
			{ 3.0, Vec2{ 0.0, 0.0 }, U"t=3.0: 2周目完了、3周目の0%" },
		};

		for (const auto& testCase : testCases)
		{
			INFO(testCase.description);

			// Set manual time
			tween->setManualTime(testCase.manualTime);

			// Update canvas
			canvas->update();

			// Check position
			const Vec2 actualPos = node->transform().translate().value();

			INFO("manualTime: " << testCase.manualTime);
			INFO("Expected: " << testCase.expectedPos);
			INFO("Actual: " << actualPos);

			CHECK(actualPos.x == Approx(testCase.expectedPos.x).margin(0.01));
			CHECK(actualPos.y == Approx(testCase.expectedPos.y).margin(0.01));
		}
	}

	SECTION("Manual mode with loopDuration and sequential Tweens")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create();
		canvas->addChild(node);

		// 1秒のdelayのあと0,0から12,34に1秒かけて移動
		auto tween1 = std::make_shared<noco::Tween>();
		tween1->setManualMode(true)
			->setActive(true)
			->setTranslateEnabled(true)
			->setTranslateFrom(Vec2{ 0.0, 0.0 })
			->setTranslateTo(Vec2{ 12.0, 34.0 })
			->setDelay(1.0)
			->setDuration(1.0)
			->setEasing(noco::TweenEasing::Linear)
			->setLoopType(noco::TweenLoopType::Loop)
			->setLoopDuration(5.0)
			->setApplyDuringDelay(false);
		node->addComponent(tween1);

		// 12,34から-45,-67へ1秒かけて移動
		auto tween2 = std::make_shared<noco::Tween>();
		tween2->setManualMode(true)
			->setActive(true)
			->setTranslateEnabled(true)
			->setTranslateFrom(Vec2{ 12.0, 34.0 })
			->setTranslateTo(Vec2{ -45.0, -67.0 })
			->setDelay(2.0)  // 1秒delay + 1秒のtween1の後
			->setDuration(1.0)
			->setEasing(noco::TweenEasing::Linear)
			->setLoopType(noco::TweenLoopType::Loop)
			->setLoopDuration(5.0)
			->setApplyDuringDelay(false);
		node->addComponent(tween2);

		// Test cases
		struct TestCase
		{
			double manualTime;
			Vec2 expectedPos;
			String description;
		};

		Array<TestCase> testCases = {
			{ 0.0, Vec2{ 0.0, 0.0 }, U"t=0: delay期間中、初期位置" },
			{ 0.5, Vec2{ 0.0, 0.0 }, U"t=0.5: delay期間中、初期位置" },
			{ 1.0, Vec2{ 0.0, 0.0 }, U"t=1.0: tween1のdelay終了、アニメーション開始時" },
			{ 1.5, Vec2{ 6.0, 17.0 }, U"t=1.5: tween1の50%" },
			{ 2.0, Vec2{ 12.0, 34.0 }, U"t=2.0: tween1完了、tween2開始" },
			{ 2.5, Vec2{ -16.5, -16.5 }, U"t=2.5: tween2の50%" },
			{ 3.0, Vec2{ -45.0, -67.0 }, U"t=3.0: tween2完了" },
			{ 4.0, Vec2{ -45.0, -67.0 }, U"t=4.0: アニメーション完了後維持" },
			{ 4.9, Vec2{ -45.0, -67.0 }, U"t=4.9: ループ直前" },
			{ 5.0, Vec2{ -45.0, -67.0 }, U"t=5.0: ループして時間0に戻るが、delay期間なので前回値維持" },
			{ 6.0, Vec2{ 0.0, 0.0 }, U"t=6.0: 2周目のtween1開始" },
			{ 6.5, Vec2{ 6.0, 17.0 }, U"t=6.5: 2周目のtween1の50%" },
			{ 7.0, Vec2{ 12.0, 34.0 }, U"t=7.0: 2周目のtween1完了、tween2開始" },
			{ 7.5, Vec2{ -16.5, -16.5 }, U"t=7.5: 2周目のtween2の50%" },
			{ 8.0, Vec2{ -45.0, -67.0 }, U"t=8.0: 2周目のtween2完了" },
			{ 10.0, Vec2{ -45.0, -67.0 }, U"t=10.0: 3周目、delay期間なので前回値維持" },
			{ 11.0, Vec2{ 0.0, 0.0 }, U"t=11.0: 3周目のtween1開始" },
			{ 11.5, Vec2{ 6.0, 17.0 }, U"t=11.5: 3周目のtween1の50%" },
		};

		for (const auto& testCase : testCases)
		{
			INFO(testCase.description);

			// Set manual time
			tween1->setManualTime(testCase.manualTime);
			tween2->setManualTime(testCase.manualTime);

			// Update canvas
			canvas->update();

			// Check position
			const Vec2 actualPos = node->transform().translate().value();

			INFO("manualTime: " << testCase.manualTime);
			INFO("Expected: " << testCase.expectedPos);
			INFO("Actual: " << actualPos);

			CHECK(actualPos.x == Approx(testCase.expectedPos.x).margin(0.01));
			CHECK(actualPos.y == Approx(testCase.expectedPos.y).margin(0.01));
		}
	}

	SECTION("PingPong loop without loopDuration - delay not included in loop")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create();
		canvas->addChild(node);

		// delay=1で0,0から100,100へduration=1で移動、PingPongループ
		auto tween = std::make_shared<noco::Tween>();
		tween->setManualMode(true)
			->setActive(true)
			->setTranslateEnabled(true)
			->setTranslateFrom(Vec2{ 0.0, 0.0 })
			->setTranslateTo(Vec2{ 100.0, 100.0 })
			->setDelay(1.0)
			->setDuration(1.0)
			->setEasing(noco::TweenEasing::Linear)
			->setLoopType(noco::TweenLoopType::PingPong)
			->setLoopDuration(0.0)  // loopDuration=0の場合、delayはループに含めない
			->setApplyDuringDelay(false);
		node->addComponent(tween);

		// Test cases
		struct TestCase
		{
			double manualTime;
			Vec2 expectedPos;
			String description;
		};

		Array<TestCase> testCases = {
			{ 0.0, Vec2{ 0.0, 0.0 }, U"t=0: delay期間中、初期位置" },
			{ 0.5, Vec2{ 0.0, 0.0 }, U"t=0.5: delay期間中、初期位置" },
			{ 1.0, Vec2{ 0.0, 0.0 }, U"t=1.0: delay終了、アニメーション開始時" },
			{ 1.5, Vec2{ 50.0, 50.0 }, U"t=1.5: 順方向50%" },
			{ 2.0, Vec2{ 100.0, 100.0 }, U"t=2.0: 順方向100%完了" },
			{ 2.5, Vec2{ 50.0, 50.0 }, U"t=2.5: 逆方向50% (PingPong)" },
			{ 3.0, Vec2{ 0.0, 0.0 }, U"t=3.0: 逆方向100%完了、原点に戻る" },
			{ 3.5, Vec2{ 50.0, 50.0 }, U"t=3.5: 順方向50% (2サイクル目)" },
			{ 4.0, Vec2{ 100.0, 100.0 }, U"t=4.0: 順方向100%完了 (2サイクル目)" },
			{ 4.5, Vec2{ 50.0, 50.0 }, U"t=4.5: 逆方向50% (2サイクル目)" },
			{ 5.0, Vec2{ 0.0, 0.0 }, U"t=5.0: 逆方向100%完了 (2サイクル目)" },
		};

		for (const auto& testCase : testCases)
		{
			INFO(testCase.description);

			// Set manual time
			tween->setManualTime(testCase.manualTime);

			// Update canvas
			canvas->update();

			// Check position
			const Vec2 actualPos = node->transform().translate().value();

			INFO("manualTime: " << testCase.manualTime);
			INFO("Expected: " << testCase.expectedPos);
			INFO("Actual: " << actualPos);

			CHECK(actualPos.x == Approx(testCase.expectedPos.x).margin(0.01));
			CHECK(actualPos.y == Approx(testCase.expectedPos.y).margin(0.01));
		}
	}

	SECTION("Tween tag and batch control")
	{
		auto canvas = noco::Canvas::Create();

		// ノード1: tag="in"のTween
		auto node1 = noco::Node::Create();
		auto tween1 = std::make_shared<noco::Tween>();
		tween1->setActive(true)
			->setTranslateEnabled(true)
			->setTranslateFrom(Vec2{ 100.0, 100.0 })
			->setTranslateTo(Vec2{ 200.0, 100.0 })
			->setDuration(1.0)
			->setEasing(noco::TweenEasing::Linear)
			->setTag(U"in");
		node1->addComponent(tween1);

		// ノード2: tag="in"のTween
		auto node2 = noco::Node::Create();
		auto tween2 = std::make_shared<noco::Tween>();
		tween2->setActive(true)
			->setTranslateEnabled(true)
			->setTranslateFrom(Vec2{ 100.0, 200.0 })
			->setTranslateTo(Vec2{ 200.0, 200.0 })
			->setDuration(1.0)
			->setEasing(noco::TweenEasing::Linear)
			->setTag(U"in");
		node2->addComponent(tween2);

		// ノード3: tag="out"のTween
		auto node3 = noco::Node::Create();
		auto tween3 = std::make_shared<noco::Tween>();
		tween3->setActive(true)
			->setTranslateEnabled(true)
			->setTranslateFrom(Vec2{ 100.0, 300.0 })
			->setTranslateTo(Vec2{ 200.0, 300.0 })
			->setDuration(1.0)
			->setEasing(noco::TweenEasing::Linear)
			->setTag(U"out");
		node3->addComponent(tween3);

		// ノード4: tagなしのTween
		auto node4 = noco::Node::Create();
		auto tween4 = std::make_shared<noco::Tween>();
		tween4->setActive(true)
			->setTranslateEnabled(true)
			->setTranslateFrom(Vec2{ 100.0, 400.0 })
			->setTranslateTo(Vec2{ 200.0, 400.0 })
			->setDuration(1.0)
			->setEasing(noco::TweenEasing::Linear)
			->setTag(U"");
		node4->addComponent(tween4);

		canvas->addChild(node1);
		canvas->addChild(node2);
		canvas->addChild(node3);
		canvas->addChild(node4);

		// 初期状態確認
		CHECK(tween1->active().defaultValue() == true);
		CHECK(tween2->active().defaultValue() == true);
		CHECK(tween3->active().defaultValue() == true);
		CHECK(tween4->active().defaultValue() == true);

		// tag="in"のTweenを非アクティブに
		canvas->setTweenActiveByTag(U"in", false);
		CHECK(tween1->active().defaultValue() == false);
		CHECK(tween2->active().defaultValue() == false);
		CHECK(tween3->active().defaultValue() == true);  // tag="out"なので変更なし
		CHECK(tween4->active().defaultValue() == true);  // tagなしなので変更なし

		// tag="out"のTweenを非アクティブに
		canvas->setTweenActiveByTag(U"out", false);
		CHECK(tween1->active().defaultValue() == false);
		CHECK(tween2->active().defaultValue() == false);
		CHECK(tween3->active().defaultValue() == false);
		CHECK(tween4->active().defaultValue() == true);  // tagなしなので変更なし

		// tag="in"のTweenをアクティブに
		canvas->setTweenActiveByTag(U"in", true);
		CHECK(tween1->active().defaultValue() == true);
		CHECK(tween2->active().defaultValue() == true);
		CHECK(tween3->active().defaultValue() == false);  // tag="out"なので変更なし
		CHECK(tween4->active().defaultValue() == true);

		// すべてのTweenを非アクティブに
		canvas->setTweenActiveAll(false);
		CHECK(tween1->active().defaultValue() == false);
		CHECK(tween2->active().defaultValue() == false);
		CHECK(tween3->active().defaultValue() == false);
		CHECK(tween4->active().defaultValue() == false);

		// すべてのTweenをアクティブに
		canvas->setTweenActiveAll(true);
		CHECK(tween1->active().defaultValue() == true);
		CHECK(tween2->active().defaultValue() == true);
		CHECK(tween3->active().defaultValue() == true);
		CHECK(tween4->active().defaultValue() == true);
	}

	SECTION("Tween tag control with nested nodes")
	{
		auto canvas = noco::Canvas::Create();

		// 親ノード
		auto parentNode = noco::Node::Create();

		// 親ノードのTween (tag="in")
		auto parentTween = std::make_shared<noco::Tween>();
		parentTween->setActive(true)
			->setTag(U"in");
		parentNode->addComponent(parentTween);

		// 子ノード1
		auto childNode1 = noco::Node::Create();
		auto childTween1 = std::make_shared<noco::Tween>();
		childTween1->setActive(true)
			->setTag(U"in");
		childNode1->addComponent(childTween1);

		// 子ノード2
		auto childNode2 = noco::Node::Create();
		auto childTween2 = std::make_shared<noco::Tween>();
		childTween2->setActive(true)
			->setTag(U"out");
		childNode2->addComponent(childTween2);

		// 孫ノード
		auto grandChildNode = noco::Node::Create();
		auto grandChildTween = std::make_shared<noco::Tween>();
		grandChildTween->setActive(true)
			->setTag(U"in");
		grandChildNode->addComponent(grandChildTween);

		// 階層構造を作成
		parentNode->addChild(childNode1);
		parentNode->addChild(childNode2);
		childNode1->addChild(grandChildNode);
		canvas->addChild(parentNode);

		// 初期状態確認
		CHECK(parentTween->active().defaultValue() == true);
		CHECK(childTween1->active().defaultValue() == true);
		CHECK(childTween2->active().defaultValue() == true);
		CHECK(grandChildTween->active().defaultValue() == true);

		// tag="in"のTweenを非アクティブに（再帰的）
		canvas->setTweenActiveByTag(U"in", false);
		CHECK(parentTween->active().defaultValue() == false);
		CHECK(childTween1->active().defaultValue() == false);
		CHECK(childTween2->active().defaultValue() == true);  // tag="out"
		CHECK(grandChildTween->active().defaultValue() == false);

		// ノードレベルでの制御（非再帰）
		parentNode->setTweenActiveByTag(U"in", true, noco::RecursiveYN::No);
		CHECK(parentTween->active().defaultValue() == true);   // 親ノードのみ変更
		CHECK(childTween1->active().defaultValue() == false);  // 子は変更なし
		CHECK(childTween2->active().defaultValue() == true);
		CHECK(grandChildTween->active().defaultValue() == false);

		// ノードレベルでの制御（再帰的）
		childNode1->setTweenActiveByTag(U"in", true, noco::RecursiveYN::Yes);
		CHECK(parentTween->active().defaultValue() == true);
		CHECK(childTween1->active().defaultValue() == true);   // 子ノード1変更
		CHECK(childTween2->active().defaultValue() == true);
		CHECK(grandChildTween->active().defaultValue() == true);  // 孫も変更

		// ノードレベルですべてのTweenを非アクティブに（非再帰）
		parentNode->setTweenActiveAll(false, noco::RecursiveYN::No);
		CHECK(parentTween->active().defaultValue() == false);  // 親のみ変更
		CHECK(childTween1->active().defaultValue() == true);   // 子は変更なし
		CHECK(childTween2->active().defaultValue() == true);
		CHECK(grandChildTween->active().defaultValue() == true);

		// ノードレベルですべてのTweenをアクティブに（再帰的）
		parentNode->setTweenActiveAll(true, noco::RecursiveYN::Yes);
		CHECK(parentTween->active().defaultValue() == true);
		CHECK(childTween1->active().defaultValue() == true);
		CHECK(childTween2->active().defaultValue() == true);
		CHECK(grandChildTween->active().defaultValue() == true);
	}
}