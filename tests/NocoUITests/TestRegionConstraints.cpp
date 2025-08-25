#include <catch2/catch.hpp>
#include <Siv3D.hpp>
#include <NocoUI.hpp>

// ========================================
// Regionの制約のテスト
// ========================================

TEST_CASE("InlineRegion constraints with sizeRatio", "[Region][Constraints]")
{
	SECTION("sizeRatio with minWidth and minHeight")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto parent = noco::Node::Create(U"Parent");
		auto child = noco::Node::Create(U"Child");
		
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 200, 150 } });
		
		// 子ノードに最小サイズ制約を設定
		// sizeRatio=0.3で親の30% = 60x45になるが、min制約で100x80になるべき
		child->setRegion(noco::InlineRegion{
			.sizeRatio = Vec2{ 0.3, 0.3 },
			.minWidth = 100.0,
			.minHeight = 80.0,
		});
		
		parent->addChild(child);
		canvas->addChild(parent);
		canvas->update();
		
		auto childRect = child->regionRect();
		REQUIRE(childRect.w == Approx(100.0));
		REQUIRE(childRect.h == Approx(80.0));
	}
	
	SECTION("sizeRatio with maxWidth and maxHeight")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto parent = noco::Node::Create(U"Parent");
		auto child = noco::Node::Create(U"Child");
		
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 400, 300 } });
		
		// 子ノードに最大サイズ制約を設定
		// sizeRatio=0.8で親の80% = 320x240になるが、max制約で200x150になるべき
		child->setRegion(noco::InlineRegion{
			.sizeRatio = Vec2{ 0.8, 0.8 },
			.maxWidth = 200.0,
			.maxHeight = 150.0,
		});
		
		parent->addChild(child);
		canvas->addChild(parent);
		canvas->update();
		
		auto childRect = child->regionRect();
		REQUIRE(childRect.w == Approx(200.0));
		REQUIRE(childRect.h == Approx(150.0));
	}
	
	SECTION("sizeDelta with min and max constraints")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto node = noco::Node::Create(U"Node");
		
		// sizeDeltaと矛盾する制約を設定
		node->setRegion(noco::InlineRegion{
			.sizeDelta = Vec2{ 50, 50 },
			.minWidth = 100.0,  // sizeDeltaより大きい
			.minHeight = 80.0,
		});
		
		canvas->addChild(node);
		canvas->update();
		
		auto rect = node->regionRect();
		REQUIRE(rect.w == Approx(100.0));
		REQUIRE(rect.h == Approx(80.0));
	}
	
	SECTION("sizeDelta with max constraints smaller than delta")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto node = noco::Node::Create(U"Node");
		
		// sizeDeltaと矛盾する制約を設定
		node->setRegion(noco::InlineRegion{
			.sizeDelta = Vec2{ 200, 200 },
			.maxWidth = 150.0,  // sizeDeltaより小さい
			.maxHeight = 100.0,
		});
		
		canvas->addChild(node);
		canvas->update();
		
		auto rect = node->regionRect();
		REQUIRE(rect.w == Approx(150.0));
		REQUIRE(rect.h == Approx(100.0));
	}
}

TEST_CASE("AnchorRegion constraints", "[Region][Constraints]")
{
	SECTION("AnchorRegion with min and max constraints")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto parent = noco::Node::Create(U"Parent");
		auto child = noco::Node::Create(U"Child");
		
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 300, 200 } });
		
		// 子ノードをアンカーで配置し、min/max制約を設定
		child->setRegion(noco::AnchorRegion{
			.anchorMin = Vec2{ 0.0, 0.0 },
			.anchorMax = Vec2{ 1.0, 1.0 },  // 親と同じサイズ(300x200)
			.minWidth = 100.0,
			.minHeight = 50.0,
			.maxWidth = 250.0,
			.maxHeight = 180.0,
		});
		
		parent->addChild(child);
		canvas->addChild(parent);
		canvas->update();
		
		// 親のサイズ(300x200)に対して、幅はmaxWidth(250)、高さはmaxHeight(180)で制約される
		auto childRect = child->regionRect();
		REQUIRE(childRect.w == Approx(250.0));
		REQUIRE(childRect.h == Approx(180.0));
	}
	
	SECTION("AnchorRegion with sizeDelta and constraints")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto parent = noco::Node::Create(U"Parent");
		auto child = noco::Node::Create(U"Child");
		
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 400, 300 } });
		
		// アンカーをセンターにして、sizeDeltaとmin/max制約を設定
		child->setRegion(noco::AnchorRegion{
			.anchorMin = Vec2{ 0.5, 0.5 },
			.anchorMax = Vec2{ 0.5, 0.5 },
			.sizeDelta = Vec2{ 50, 50 },
			.minWidth = 100.0,
			.minHeight = 80.0,
		});
		
		parent->addChild(child);
		canvas->addChild(parent);
		canvas->update();
		
		// sizeDelta(50x50)よりminWidth/minHeight(100x80)が優先される
		auto childRect = child->regionRect();
		REQUIRE(childRect.w == Approx(100.0));
		REQUIRE(childRect.h == Approx(80.0));
	}
}

TEST_CASE("FlowLayout with constraints", "[Region][Constraints]")
{
	SECTION("FlowLayout with flexibleWeight and minWidth")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto parent = noco::Node::Create(U"Parent");
		
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 300, 100 } });
		
		parent->setChildrenLayout(noco::FlowLayout{
			.spacing = Vec2{ 0, 0 },
		});
		
		// 3つの子ノード（flexibleWeight=1で等分割）
		auto child1 = noco::Node::Create(U"Child1");
		auto child2 = noco::Node::Create(U"Child2");
		auto child3 = noco::Node::Create(U"Child3");
		
		// child1にminWidth制約を設定
		child1->setRegion(noco::InlineRegion{
			.flexibleWeight = 1.0,
			.minWidth = 150.0,
		});
		
		child2->setRegion(noco::InlineRegion{
			.flexibleWeight = 1.0,
		});
		
		child3->setRegion(noco::InlineRegion{
			.flexibleWeight = 1.0,
		});
		
		parent->addChild(child1);
		parent->addChild(child2);
		parent->addChild(child3);
		
		canvas->addChild(parent);
		canvas->update();
		
		auto rect1 = child1->regionRect();
		auto rect2 = child2->regionRect();
		auto rect3 = child3->regionRect();
		
		// minWidth制約適用後にflexibleWeight分配
		REQUIRE(rect1.w == Approx(200.0));
		REQUIRE(rect2.w == Approx(50.0));
		REQUIRE(rect3.w == Approx(50.0));
	}
	
	SECTION("FlowLayout with flexibleWeight and maxWidth")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto parent = noco::Node::Create(U"Parent");
		
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 300, 100 } });
		
		parent->setChildrenLayout(noco::FlowLayout{
			.spacing = Vec2{ 0, 0 },
		});
		
		// 2つの子ノード
		auto child1 = noco::Node::Create(U"Child1");
		auto child2 = noco::Node::Create(U"Child2");
		
		// child1にmaxWidth制約を設定
		child1->setRegion(noco::InlineRegion{
			.flexibleWeight = 1.0,
			.maxWidth = 100.0,
		});
		
		child2->setRegion(noco::InlineRegion{
			.flexibleWeight = 1.0,
		});
		
		parent->addChild(child1);
		parent->addChild(child2);
		
		canvas->addChild(parent);
		canvas->update();
		
		auto rect1 = child1->regionRect();
		auto rect2 = child2->regionRect();
		
		// maxWidth制約適用後にflexibleWeight分配するため、max値を超えるのが想定動作
		REQUIRE(rect1.w == Approx(150.0));
		REQUIRE(rect2.w == Approx(150.0));
	}
	
	SECTION("FlowLayout with sizeDelta and constraints")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto parent = noco::Node::Create(U"Parent");
		
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 400, 100 } });
		
		parent->setChildrenLayout(noco::FlowLayout{
			.spacing = Vec2{ 10, 10 },
		});
		
		// 3つの子ノード（固定サイズ + 制約）
		auto child1 = noco::Node::Create(U"Child1");
		auto child2 = noco::Node::Create(U"Child2");
		auto child3 = noco::Node::Create(U"Child3");
		
		// sizeDeltaと制約の組み合わせ
		child1->setRegion(noco::InlineRegion{
			.sizeDelta = Vec2{ 50, 50 },
			.minWidth = 80.0,  // sizeDeltaより大きい
		});
		
		child2->setRegion(noco::InlineRegion{
			.sizeDelta = Vec2{ 150, 50 },
			.maxWidth = 100.0,  // sizeDeltaより小さい
		});
		
		child3->setRegion(noco::InlineRegion{
			.sizeDelta = Vec2{ 70, 50 },
		});
		
		parent->addChild(child1);
		parent->addChild(child2);
		parent->addChild(child3);
		
		canvas->addChild(parent);
		canvas->update();
		
		auto rect1 = child1->regionRect();
		auto rect2 = child2->regionRect();
		auto rect3 = child3->regionRect();
		
		// child1はminWidth制約により80になる
		REQUIRE(rect1.w == Approx(80.0));
		// child2はmaxWidth制約により100になる
		REQUIRE(rect2.w == Approx(100.0));
		// child3はsizeDeltaのまま70
		REQUIRE(rect3.w == Approx(70.0));
	}
}

TEST_CASE("Region constraints with flexibleWeight", "[Region][Constraints]")
{
	SECTION("HorizontalLayout with flexibleWeight and minWidth")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto parent = noco::Node::Create(U"Parent");
		
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 300, 100 } });
		
		parent->setChildrenLayout(noco::HorizontalLayout{
			.spacing = 0,
		});
		
		// 3つの子ノード（flexibleWeight=1で等分割）
		auto child1 = noco::Node::Create(U"Child1");
		auto child2 = noco::Node::Create(U"Child2");
		auto child3 = noco::Node::Create(U"Child3");
		
		// child1にminWidth制約を設定
		// flexibleWeightで100ずつ分配されるが、minWidth=150で制約
		child1->setRegion(noco::InlineRegion{
			.flexibleWeight = 1.0,
			.minWidth = 150.0,
		});
		
		child2->setRegion(noco::InlineRegion{
			.flexibleWeight = 1.0,
		});
		
		child3->setRegion(noco::InlineRegion{
			.flexibleWeight = 1.0,
		});
		
		parent->addChild(child1);
		parent->addChild(child2);
		parent->addChild(child3);
		
		canvas->addChild(parent);
		canvas->update();
		
		auto rect1 = child1->regionRect();
		auto rect2 = child2->regionRect();
		auto rect3 = child3->regionRect();
		
		// minWidth制約適用後にflexibleWeight分配
		REQUIRE(rect1.w == Approx(200.0));
		REQUIRE(rect2.w == Approx(50.0));
		REQUIRE(rect3.w == Approx(50.0));
	}
	
	SECTION("HorizontalLayout with flexibleWeight and maxWidth")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto parent = noco::Node::Create(U"Parent");
		
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 300, 100 } });
		
		parent->setChildrenLayout(noco::HorizontalLayout{
			.spacing = 0,
		});
		
		// 2つの子ノード
		auto child1 = noco::Node::Create(U"Child1");
		auto child2 = noco::Node::Create(U"Child2");
		
		// child1にmaxWidth制約を設定
		// flexibleWeightで150ずつ分配されるが、maxWidth=100で制約
		child1->setRegion(noco::InlineRegion{
			.flexibleWeight = 1.0,
			.maxWidth = 100.0,
		});
		
		child2->setRegion(noco::InlineRegion{
			.flexibleWeight = 1.0,
		});
		
		parent->addChild(child1);
		parent->addChild(child2);
		
		canvas->addChild(parent);
		canvas->update();
		
		auto rect1 = child1->regionRect();
		auto rect2 = child2->regionRect();
		
		// maxWidth制約適用後にflexibleWeight分配するため、max値を超えるのが想定動作
		REQUIRE(rect1.w == Approx(150.0));
		REQUIRE(rect2.w == Approx(150.0));
	}
	
	SECTION("VerticalLayout with flexibleWeight and minHeight")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto parent = noco::Node::Create(U"Parent");
		
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 300 } });
		
		parent->setChildrenLayout(noco::VerticalLayout{
			.spacing = 0,
		});
		
		// 3つの子ノード
		auto child1 = noco::Node::Create(U"Child1");
		auto child2 = noco::Node::Create(U"Child2");
		auto child3 = noco::Node::Create(U"Child3");
		
		// child2にminHeight制約を設定
		child1->setRegion(noco::InlineRegion{
			.flexibleWeight = 1.0,
		});
		
		child2->setRegion(noco::InlineRegion{
			.flexibleWeight = 1.0,
			.minHeight = 150.0,
		});
		
		child3->setRegion(noco::InlineRegion{
			.flexibleWeight = 1.0,
		});
		
		parent->addChild(child1);
		parent->addChild(child2);
		parent->addChild(child3);
		
		canvas->addChild(parent);
		canvas->update();
		
		auto rect1 = child1->regionRect();
		auto rect2 = child2->regionRect();
		auto rect3 = child3->regionRect();
		
		// minHeight制約適用後にflexibleWeight分配
		REQUIRE(rect2.h == Approx(200.0));
		REQUIRE(rect1.h == Approx(50.0));
		REQUIRE(rect3.h == Approx(50.0));
	}
	
	SECTION("VerticalLayout with flexibleWeight and maxHeight")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto parent = noco::Node::Create(U"Parent");
		
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 300 } });
		
		parent->setChildrenLayout(noco::VerticalLayout{
			.spacing = 0,
		});
		
		// 2つの子ノード
		auto child1 = noco::Node::Create(U"Child1");
		auto child2 = noco::Node::Create(U"Child2");
		
		// 両方にflexibleWeight設定、child1にmaxHeight制約
		child1->setRegion(noco::InlineRegion{
			.flexibleWeight = 1.0,
			.maxHeight = 100.0,
		});
		
		child2->setRegion(noco::InlineRegion{
			.flexibleWeight = 1.0,
		});
		
		parent->addChild(child1);
		parent->addChild(child2);
		
		canvas->addChild(parent);
		canvas->update();
		
		auto rect1 = child1->regionRect();
		auto rect2 = child2->regionRect();
		
		// maxHeight制約適用後にflexibleWeight分配するため、max値を超えるのが想定動作
		REQUIRE(rect1.h == Approx(150.0));
		REQUIRE(rect2.h == Approx(150.0));
	}
}