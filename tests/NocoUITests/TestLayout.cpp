#include <catch2/catch.hpp>
#include <Siv3D.hpp>
#include <NocoUI.hpp>

// ========================================
// Layoutのテスト
// ========================================

TEST_CASE("Layout system", "[Layout]")
{
	SECTION("HorizontalLayout")
	{
		auto parent = noco::Node::Create();
		noco::HorizontalLayout layout;
		layout.spacing = 10.0f;
		
		parent->setChildrenLayout(layout);
		
		// 子ノードを追加
		auto child1 = noco::Node::Create();
		auto child2 = noco::Node::Create();
		parent->addChild(child1);
		parent->addChild(child2);
		
		// Layoutが正しく設定されているか確認
		auto* hLayout = std::get_if<noco::HorizontalLayout>(&parent->childrenLayout());
		REQUIRE(hLayout != nullptr);
		REQUIRE(hLayout->spacing == 10.0f);
		
		// Canvasに追加してレイアウトを計算
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 300, 100 } });
		child1->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 50, 80 } });
		child2->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 60, 80 } });
		
		auto canvas = noco::Canvas::Create();
		canvas->addChild(parent);
		canvas->update();
		
		// 子ノードの位置が正しく計算されているか確認
		REQUIRE(child1->regionRect().pos.x == Approx(0));
		REQUIRE(child1->regionRect().pos.y == Approx(10));  // (100 - 80) / 2 = 10 (Middle alignment)
		REQUIRE(child2->regionRect().pos.x == Approx(60));  // 50 + spacing(10)
		REQUIRE(child2->regionRect().pos.y == Approx(10));
	}

	SECTION("VerticalLayout")
	{
		auto parent = noco::Node::Create();
		noco::VerticalLayout layout;
		layout.spacing = 5.0f;
		
		parent->setChildrenLayout(layout);
		
		// Layoutが正しく設定されているか確認
		auto* vLayout = std::get_if<noco::VerticalLayout>(&parent->childrenLayout());
		REQUIRE(vLayout != nullptr);
		REQUIRE(vLayout->spacing == 5.0f);
		
		// 子ノードを追加してレイアウトを計算
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 200, 300 } });
		
		auto child1 = noco::Node::Create();
		child1->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 150, 40 } });
		auto child2 = noco::Node::Create();
		child2->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 150, 50 } });
		parent->addChild(child1);
		parent->addChild(child2);
		
		auto canvas = noco::Canvas::Create();
		canvas->addChild(parent);
		canvas->update();
		
		// 子ノードの位置が正しく計算されているか確認
		REQUIRE(child1->regionRect().pos.x == Approx(25));  // (200 - 150) / 2 = 25 (Center alignment)
		REQUIRE(child1->regionRect().pos.y == Approx(0));
		REQUIRE(child2->regionRect().pos.x == Approx(25));
		REQUIRE(child2->regionRect().pos.y == Approx(45));  // 40 + spacing(5)
	}
}

TEST_CASE("HorizontalLayout detailed", "[Layout][HorizontalLayout]")
{
	SECTION("Spacing and padding")
	{
		auto parent = noco::Node::Create();
		noco::HorizontalLayout layout;
		
		// スペーシングの設定
		layout.spacing = 15.0f;
		
		// パディングの設定
		layout.padding = noco::LRTB{ 10, 10, 5, 5 };
		
		parent->setChildrenLayout(layout);
		
		auto* hLayout = std::get_if<noco::HorizontalLayout>(&parent->childrenLayout());
		REQUIRE(hLayout != nullptr);
		REQUIRE(hLayout->spacing == 15.0f);
		REQUIRE(hLayout->padding.left == 10);
		REQUIRE(hLayout->padding.top == 5);
	}

	SECTION("Child alignment")
	{
		auto parent = noco::Node::Create();
		noco::HorizontalLayout layout;
		
		// 子要素のアライメント
		layout.verticalAlign = noco::VerticalAlign::Middle;
		
		parent->setChildrenLayout(layout);
		
		auto* hLayout = std::get_if<noco::HorizontalLayout>(&parent->childrenLayout());
		REQUIRE(hLayout->verticalAlign == noco::VerticalAlign::Middle);
	}

	SECTION("Flexible weight distribution")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create();
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 300, 100 } });
		
		noco::HorizontalLayout layout;
		layout.spacing = 10.0f;
		parent->setChildrenLayout(layout);
		
		// 固定サイズの子
		auto fixed = noco::Node::Create();
		fixed->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 50, 50 } });
		
		// フレキシブルな子（重み1）
		auto flex1 = noco::Node::Create();
		flex1->setRegion(noco::InlineRegion{ .flexibleWeight = 1.0 });
		
		// フレキシブルな子（重み2）
		auto flex2 = noco::Node::Create();
		flex2->setRegion(noco::InlineRegion{ .flexibleWeight = 2.0 });
		
		parent->addChild(fixed);
		parent->addChild(flex1);
		parent->addChild(flex2);
		
		canvas->addChild(parent);
		canvas->update();
		
		// サイズ計算の検証
		// 利用可能幅: 300 - 50(固定) - 20(spacing*2) = 230
		// flex1: 230 * (1/3) = 76.67
		// flex2: 230 * (2/3) = 153.33
		REQUIRE(fixed->regionRect().size.x == Approx(50));
		REQUIRE(flex1->regionRect().size.x == Approx(230.0 / 3.0));
		REQUIRE(flex2->regionRect().size.x == Approx(230.0 * 2.0 / 3.0));
		
		// 位置の検証
		REQUIRE(fixed->regionRect().pos.x == Approx(0));
		REQUIRE(flex1->regionRect().pos.x == Approx(60));  // 50 + spacing
		REQUIRE(flex2->regionRect().pos.x == Approx(60 + 230.0 / 3.0 + 10));  // flex1の後
	}

	SECTION("refreshLayoutImmediately updates regionRect")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create();
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 400, 300 } });
		
		noco::HorizontalLayout layout;
		layout.spacing = 10.0f;
		parent->setChildrenLayout(layout);
		
		auto child1 = noco::Node::Create();
		child1->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 50 } });
		auto child2 = noco::Node::Create();
		child2->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 150, 50 } });
		
		parent->addChild(child1);
		parent->addChild(child2);
		canvas->addChild(parent);
		
		// refreshLayoutImmediately呼び出し前：regionRectは未更新
		REQUIRE(child1->regionRect().pos == Vec2{ 0, 0 });
		REQUIRE(child1->regionRect().size == Vec2{ 0, 0 });
		REQUIRE(child2->regionRect().pos == Vec2{ 0, 0 });
		REQUIRE(child2->regionRect().size == Vec2{ 0, 0 });
		
		// レイアウトを即座に更新
		canvas->refreshLayoutImmediately();
		
		// refreshLayoutImmediately呼び出し後：regionRectが更新済み
		REQUIRE(child1->regionRect().pos.x == Approx(0));
		REQUIRE(child1->regionRect().pos.y == Approx(125));  // (300 - 50) / 2 = 125
		REQUIRE(child1->regionRect().size == Vec2{ 100, 50 });
		
		REQUIRE(child2->regionRect().pos.x == Approx(110));  // 100 + spacing(10)
		REQUIRE(child2->regionRect().pos.y == Approx(125));
		REQUIRE(child2->regionRect().size == Vec2{ 150, 50 });
		
		// サイズ変更後も正しく更新されることを確認
		child1->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 80, 40 } });
		
		canvas->refreshLayoutImmediately();
		
		REQUIRE(child1->regionRect().pos.x == Approx(0));
		REQUIRE(child1->regionRect().pos.y == Approx(130));  // (300 - 40) / 2 = 130
		REQUIRE(child1->regionRect().size == Vec2{ 80, 40 });
		
		REQUIRE(child2->regionRect().pos.x == Approx(90));  // 80 + spacing(10)
		REQUIRE(child2->regionRect().pos.y == Approx(125));  // child2のサイズは変わらない
		REQUIRE(child2->regionRect().size == Vec2{ 150, 50 });
	}
}
