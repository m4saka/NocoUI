# include <catch2/catch.hpp>
# include <Siv3D.hpp>
# include <NocoUI.hpp>

// ========================================
// ヒットテスト
// ========================================

TEST_CASE("Hit Testing", "[Node][HitTest]")
{
	SECTION("Basic hit test")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create();
		node->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 100 } });
		canvas->rootNode()->addChild(node);
		
		// Canvasをupdateしてレイアウトを適用
		canvas->update();
		
		// ノードの範囲内のポイント
		auto hitNode = node->hitTest(Vec2{ 50, 50 });
		REQUIRE(hitNode == node);
		
		// ノードの範囲外のポイント
		auto missNode = node->hitTest(Vec2{ 150, 150 });
		REQUIRE(missNode == nullptr);
	}

	SECTION("Hit test with transform")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create();
		node->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 100 } });
		node->transform().setPosition(Vec2{ 100, 100 });
		node->transform().setAppliesToHitTest(true);
		canvas->rootNode()->addChild(node);
		
		// Canvasをupdateしてレイアウトを適用
		canvas->update();
		
		// 変換後の位置でヒットテスト
		auto hitNode = node->hitTest(Vec2{ 150, 150 });
		REQUIRE(hitNode == node);
		
		// 元の位置ではヒットしない
		auto missNode = node->hitTest(Vec2{ 50, 50 });
		REQUIRE(missNode == nullptr);
	}

	SECTION("Hit test with appliesToHitTest = false")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create();
		node->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 100 } });
		node->transform().setPosition(Vec2{ 100, 100 });
		node->transform().setAppliesToHitTest(false); // 明示的にfalseに設定
		canvas->rootNode()->addChild(node);
		
		// Canvasをupdateしてレイアウトを適用
		canvas->update();
		
		// 元の位置でヒットする（Transformが適用されない）
		auto hitNode = node->hitTest(Vec2{ 50, 50 });
		REQUIRE(hitNode == node);
		
		// 変換後の位置ではヒットしない
		auto missNode = node->hitTest(Vec2{ 150, 150 });
		REQUIRE(missNode == nullptr);
	}

	SECTION("Hit test with appliesToHitTest = true")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create();
		node->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 100 } });
		node->transform().setPosition(Vec2{ 100, 100 });
		node->transform().setAppliesToHitTest(true);
		canvas->rootNode()->addChild(node);
		
		// Canvasをupdateしてレイアウトを適用
		canvas->update();
		
		// 変換後の位置でヒットする
		auto hitNode = node->hitTest(Vec2{ 150, 150 });
		REQUIRE(hitNode == node);
		
		// 元の位置ではヒットしない
		auto missNode = node->hitTest(Vec2{ 50, 50 });
		REQUIRE(missNode == nullptr);
	}

	SECTION("Hit test with scale and appliesToHitTest")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create();
		node->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 100 } });
		node->transform().setScale(Vec2{ 2.0, 2.0 }); // 2倍にスケール
		canvas->rootNode()->addChild(node);
		
		// Canvasをupdateしてレイアウトを適用
		canvas->update();
		
		// appliesToHitTest = false（デフォルト）
		node->transform().setAppliesToHitTest(false);
		canvas->update();
		
		// 元のサイズの範囲内でヒット
		auto hitOriginal = node->hitTest(Vec2{ 50, 50 });
		REQUIRE(hitOriginal == node);
		
		// スケール後の範囲（元の範囲外）ではヒットしない
		auto missScaled = node->hitTest(Vec2{ 150, 150 });
		REQUIRE(missScaled == nullptr);
		
		// appliesToHitTest = true
		node->transform().setAppliesToHitTest(true);
		canvas->update();
		
		// スケール後の範囲でヒット（中心から拡大されるので-50～150の範囲）
		auto hitScaled = node->hitTest(Vec2{ 120, 120 });
		REQUIRE(hitScaled == node);
		
		// 中心付近（両方の範囲内）でもヒット
		auto hitCenter = node->hitTest(Vec2{ 50, 50 });
		REQUIRE(hitCenter == node);
		
		// スケール後の範囲外ではヒットしない
		auto missOutside = node->hitTest(Vec2{ 200, 200 });
		REQUIRE(missOutside == nullptr);
	}

	SECTION("Hit test with nested nodes")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create();
		auto child = noco::Node::Create();
		
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 200, 200 } });
		child->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 100 } });
		
		canvas->rootNode()->addChild(parent);
		parent->addChild(child);
		
		// Canvasをupdateしてレイアウトを適用
		canvas->update();
		
		// 子ノードの範囲内
		auto hitChild = parent->hitTest(Vec2{ 50, 50 });
		REQUIRE(hitChild == child);
		
		// 親ノードのみの範囲内
		auto hitParent = parent->hitTest(Vec2{ 150, 150 });
		REQUIRE(hitParent == parent);
		
		// どちらの範囲外
		auto miss = parent->hitTest(Vec2{ 250, 250 });
		REQUIRE(miss == nullptr);
	}

	SECTION("Hit test with inactive nodes")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create();
		node->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 100 } });
		canvas->rootNode()->addChild(node);
		
		// Canvasをupdateしてレイアウトを適用
		canvas->update();
		
		// アクティブな状態でヒット
		auto hitActive = node->hitTest(Vec2{ 50, 50 });
		REQUIRE(hitActive == node);
		
		// 非アクティブに設定
		node->setActive(noco::ActiveYN::No);
		canvas->update();
		
		// 非アクティブなノードはヒットしない
		auto hitInactive = node->hitTest(Vec2{ 50, 50 });
		REQUIRE(hitInactive == nullptr);
	}

	SECTION("Hit test with clipping")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create();
		auto child = noco::Node::Create();
		
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 100 } });
		child->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 100 } });
		child->transform().setPosition(Vec2{ 50, 50 }); // 部分的に親の外に出る
		
		canvas->rootNode()->addChild(parent);
		parent->addChild(child);
		
		// クリッピングを有効化
		parent->setClippingEnabled(noco::ClippingEnabledYN::Yes);
		
		// Canvasをupdateしてレイアウトを適用
		canvas->update();
		
		// 親の範囲内で子と重なる部分
		auto hitInside = parent->hitTest(Vec2{ 75, 75 });
		REQUIRE(hitInside == child);
		
		// 親の範囲外で子と重なる部分（クリッピングされる）
		auto hitOutside = parent->hitTest(Vec2{ 125, 125 });
		REQUIRE(hitOutside == nullptr);
	}

	SECTION("Hit test with scroll - no clipping")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create();
		auto child = noco::Node::Create();
		
		// スクロール可能な親ノード
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 200, 200 } });
		parent->setScrollableAxisFlags(noco::ScrollableAxisFlags::Horizontal | noco::ScrollableAxisFlags::Vertical);
		
		// 大きな子ノード（スクロールが必要）
		child->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 400, 400 } });
		
		canvas->rootNode()->addChild(parent);
		parent->addChild(child);
		
		// Canvasをupdateしてレイアウトを適用
		canvas->update();
		
		// スクロール前のヒットテスト
		auto hitBeforeScroll = parent->hitTest(Vec2{ 100, 100 });
		REQUIRE(hitBeforeScroll == child);
		
		// スクロールする
		parent->scroll(Vec2{ -50, -50 });
		canvas->update();
		
		// スクロール後、元の位置はまだ子ノードにヒット
		auto hitAfterScroll1 = parent->hitTest(Vec2{ 100, 100 });
		REQUIRE(hitAfterScroll1 == child);
		
		// スクロール前には範囲外だった位置がヒット
		auto hitAfterScroll2 = parent->hitTest(Vec2{ 250, 250 });
		REQUIRE(hitAfterScroll2 == child);
		
		// クリッピングがないので、親の範囲外でもヒットする
		auto hitOutsideParent = parent->hitTest(Vec2{ 350, 350 });
		REQUIRE(hitOutsideParent == child);
	}

	SECTION("Hit test with scroll - with clipping")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create();
		auto child = noco::Node::Create();
		
		// スクロール可能でクリッピングを有効にした親ノード
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 200, 200 } });
		parent->setScrollableAxisFlags(noco::ScrollableAxisFlags::Horizontal | noco::ScrollableAxisFlags::Vertical);
		parent->setClippingEnabled(noco::ClippingEnabledYN::Yes);
		
		// 大きな子ノード（スクロールが必要）
		child->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 400, 400 } });
		
		canvas->rootNode()->addChild(parent);
		parent->addChild(child);
		
		// Canvasをupdateしてレイアウトを適用
		canvas->update();
		
		// スクロール前のヒットテスト
		auto hitBeforeScroll = parent->hitTest(Vec2{ 100, 100 });
		REQUIRE(hitBeforeScroll == child);
		
		// 親の範囲外はクリッピングされてヒットしない
		auto hitOutsideBeforeScroll = parent->hitTest(Vec2{ 250, 250 });
		REQUIRE(hitOutsideBeforeScroll == nullptr);
		
		// スクロールする
		parent->scroll(Vec2{ -50, -50 });
		canvas->update();
		
		// スクロール後、親の可視範囲内のみヒット
		auto hitInsideAfterScroll = parent->hitTest(Vec2{ 100, 100 });
		REQUIRE(hitInsideAfterScroll == child);
		
		// 親の範囲外はクリッピングされてヒットしない
		auto hitOutsideAfterScroll = parent->hitTest(Vec2{ 250, 250 });
		REQUIRE(hitOutsideAfterScroll == nullptr);
		
		// スクロールによって新たに可視範囲に入った部分はヒット
		auto hitNewlyVisible = parent->hitTest(Vec2{ 190, 190 });
		REQUIRE(hitNewlyVisible == child);
	}
}