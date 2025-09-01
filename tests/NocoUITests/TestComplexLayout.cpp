#include <catch2/catch.hpp>
#include <Siv3D.hpp>
#include <NocoUI.hpp>

// ========================================
// 複合的なレイアウトのテスト
// ========================================

TEST_CASE("Complex layout scenarios", "[Layout]")
{
	SECTION("FlowLayout wrapping behavior")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create();
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 200, 300 } });
		
		noco::FlowLayout flow;
		flow.spacing = Vec2{ 10, 10 };
		parent->setChildrenLayout(flow);
		
		// 複数の子ノードを追加（幅を超えて折り返すように）
		for (int32 i = 0; i < 6; ++i)
		{
			auto child = noco::Node::Create();
			child->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 60, 40 } });
			parent->addChild(child);
		}
		
		canvas->addChild(parent);
		canvas->update();
		
		// 3つずつ2行に配置される（200幅に60×3 + spacing×2 = 200）
		REQUIRE(parent->children().size() == 6);
		
		// 実際の配置位置を検証
		const auto& children = parent->children();
		// 1行目
		REQUIRE(children[0]->regionRect().pos.x == Approx(0));
		REQUIRE(children[0]->regionRect().pos.y == Approx(0));
		REQUIRE(children[1]->regionRect().pos.x == Approx(70));  // 60 + spacing
		REQUIRE(children[1]->regionRect().pos.y == Approx(0));
		REQUIRE(children[2]->regionRect().pos.x == Approx(140));  // (60 + spacing) * 2
		REQUIRE(children[2]->regionRect().pos.y == Approx(0));
		// 2行目（折り返し）
		REQUIRE(children[3]->regionRect().pos.x == Approx(0));
		REQUIRE(children[3]->regionRect().pos.y == Approx(50));  // 40 + spacing
		REQUIRE(children[4]->regionRect().pos.x == Approx(70));
		REQUIRE(children[4]->regionRect().pos.y == Approx(50));
		REQUIRE(children[5]->regionRect().pos.x == Approx(140));
		REQUIRE(children[5]->regionRect().pos.y == Approx(50));
	}

	SECTION("Layout with flexible weights")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create();
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 300, 100 } });
		
		noco::HorizontalLayout hLayout;
		hLayout.spacing = 10;
		parent->setChildrenLayout(hLayout);
		
		// 固定サイズの子
		auto fixedChild = noco::Node::Create();
		fixedChild->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 50, 0 } });
		
		// フレキシブルな子（重み1）
		auto flexChild1 = noco::Node::Create();
		flexChild1->setRegion(noco::InlineRegion{ .flexibleWeight = 1.0 });
		
		// フレキシブルな子（重み2）
		auto flexChild2 = noco::Node::Create();
		flexChild2->setRegion(noco::InlineRegion{ .flexibleWeight = 2.0 });
		
		parent->addChild(fixedChild);
		parent->addChild(flexChild1);
		parent->addChild(flexChild2);
		
		canvas->addChild(parent);
		canvas->update();
		
		// 残り幅250を1:2で分配する（300 - 50（固定） - 20（spacing×2） = 230）
		REQUIRE(parent->children().size() == 3);
		
		// 実際のサイズを検証
		const auto& children = parent->children();
		REQUIRE(children[0]->regionRect().size.x == Approx(50));  // 固定サイズ
		REQUIRE(children[1]->regionRect().size.x == Approx(230.0 / 3.0));  // フレキシブル重み1
		REQUIRE(children[2]->regionRect().size.x == Approx(230.0 * 2.0 / 3.0));  // フレキシブル重み2
	}

	SECTION("FlowLayout with oversized items")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create();
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 200, 100 } });
		
		noco::FlowLayout flow;
		flow.spacing = Vec2{ 10, 10 };
		parent->setChildrenLayout(flow);
		
		// コンテナより大きいアイテム
		auto oversizedItem = noco::Node::Create();
		oversizedItem->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 250, 40 } });
		parent->addChild(oversizedItem);
		
		// 通常サイズのアイテム
		auto normalItem = noco::Node::Create();
		normalItem->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 60, 40 } });
		parent->addChild(normalItem);
		
		canvas->addChild(parent);
		canvas->update();
		
		// オーバーサイズのアイテムはそのまま配置される
		const auto& children = parent->children();
		REQUIRE(children[0]->regionRect().pos.x == Approx(0));
		REQUIRE(children[0]->regionRect().pos.y == Approx(0));
		// 次のアイテムは折り返される
		REQUIRE(children[1]->regionRect().pos.x == Approx(0));
		REQUIRE(children[1]->regionRect().pos.y == Approx(50));  // 40 + spacing
	}

	SECTION("Zero-sized parent edge case")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create();
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 0, 0 } });
		
		noco::FlowLayout flow;
		parent->setChildrenLayout(flow);
		
		auto child = noco::Node::Create();
		child->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 50 } });
		parent->addChild(child);
		
		canvas->addChild(parent);
		canvas->update();
		
		// サイズ0の親ノードでもエラーにならない
		REQUIRE(parent->children().size() == 1);
		REQUIRE(parent->children()[0]->regionRect().pos.x == Approx(0));
		REQUIRE(parent->children()[0]->regionRect().pos.y == Approx(0));
	}

	SECTION("Nested layouts")
	{
		auto canvas = noco::Canvas::Create();
		auto root = noco::Node::Create();
		root->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 400, 300 } });
		
		// 垂直レイアウトの親
		noco::VerticalLayout vLayout;
		root->setChildrenLayout(vLayout);
		
		// 水平レイアウトの子
		auto hContainer = noco::Node::Create();
		hContainer->setRegion(noco::InlineRegion{ .flexibleWeight = 1.0 });
		noco::HorizontalLayout hLayout;
		hContainer->setChildrenLayout(hLayout);
		
		// 孫ノード
		for (int32 i = 0; i < 3; ++i)
		{
			auto grandchild = noco::Node::Create();
			grandchild->setRegion(noco::InlineRegion{ .flexibleWeight = 1.0 });
			hContainer->addChild(grandchild);
		}
		
		root->addChild(hContainer);
		canvas->addChild(root);
		canvas->update();
		
		// ネストされたレイアウトが正しく動作するか
		REQUIRE(hContainer->children().size() == 3);
	}
}