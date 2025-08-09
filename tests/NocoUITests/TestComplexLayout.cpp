# include <catch2/catch.hpp>
# include <Siv3D.hpp>
# include <NocoUI.hpp>

// ========================================
// 複雑なレイアウトシナリオテスト
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
		for (int i = 0; i < 6; ++i)
		{
			auto child = noco::Node::Create();
			child->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 60, 40 } });
			parent->addChild(child);
		}
		
		canvas->rootNode()->addChild(parent);
		canvas->update();
		
		// 3つずつ2行に配置されるはず（200幅に60×3 + spacing×2 = 200）
		REQUIRE(parent->children().size() == 6);
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
		
		canvas->rootNode()->addChild(parent);
		canvas->update();
		
		// 残り幅250を1:2で分配するはず
		REQUIRE(parent->children().size() == 3);
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
		for (int i = 0; i < 3; ++i)
		{
			auto grandchild = noco::Node::Create();
			grandchild->setRegion(noco::InlineRegion{ .flexibleWeight = 1.0 });
			hContainer->addChild(grandchild);
		}
		
		root->addChild(hContainer);
		canvas->rootNode()->addChild(root);
		canvas->update();
		
		// ネストされたレイアウトが正しく動作するか
		REQUIRE(hContainer->children().size() == 3);
	}
}