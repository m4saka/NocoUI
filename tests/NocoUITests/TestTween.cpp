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
			->setTarget(noco::TweenTarget::Translate)
			->setFromVec2(Vec2{ 0.0, 0.0 })
			->setToVec2(Vec2{ 100.0, 100.0 })
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
			->setTarget(noco::TweenTarget::Translate)
			->setFromVec2(Vec2{ 0.0, 0.0 })
			->setToVec2(Vec2{ 12.0, 34.0 })
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
			->setTarget(noco::TweenTarget::Translate)
			->setFromVec2(Vec2{ 12.0, 34.0 })
			->setToVec2(Vec2{ -45.0, -67.0 })
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
			->setTarget(noco::TweenTarget::Translate)
			->setFromVec2(Vec2{ 0.0, 0.0 })
			->setToVec2(Vec2{ 100.0, 100.0 })
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
}