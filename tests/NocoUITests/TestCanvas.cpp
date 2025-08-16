# include <catch2/catch.hpp>
# include <Siv3D.hpp>
# include <NocoUI.hpp>

// ========================================
// Canvasの基本的なテスト
// ========================================

TEST_CASE("Canvas system", "[Canvas]")
{
	SECTION("Create canvas")
	{
		auto canvas = noco::Canvas::Create();
		REQUIRE(canvas != nullptr);
		REQUIRE(canvas->children().size() == 0);
	}

	SECTION("Add node to canvas")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create(U"TestNode");
		
		canvas->addChild(node);
		
		REQUIRE(canvas->children().size() == 1);
		REQUIRE(canvas->children()[0] == node);
	}

	SECTION("Strict parent checking for addChild")
	{
		auto canvas1 = noco::Canvas::Create();
		auto canvas2 = noco::Canvas::Create();
		
		// Canvas1にトップレベルノードを追加
		auto nodeA = noco::Node::Create(U"NodeA");
		auto nodeB = noco::Node::Create(U"NodeB");
		canvas1->addChild(nodeA);
		canvas1->addChild(nodeB);
		
		// 初期状態の確認
		REQUIRE(canvas1->children().size() == 2);
		REQUIRE(nodeA->isTopLevelNode());
		REQUIRE(nodeB->isTopLevelNode());
		
		// 既にトップレベルノードとして存在するnodeAを他のCanvasに追加しようとすると例外
		REQUIRE_THROWS_AS(canvas2->addChild(nodeA), s3d::Error);
		
		// Canvas1内での階層構造を作成
		auto child = noco::Node::Create(U"Child");
		nodeA->addChild(child);
		REQUIRE(!child->isTopLevelNode());
		
		// 既に親を持つchildをCanvasに追加しようとすると例外
		REQUIRE_THROWS_AS(canvas2->addChild(child), s3d::Error);
		
		// 正しい方法：まず親から削除してから追加
		nodeA->removeChild(child);
		REQUIRE_NOTHROW(canvas2->addChild(child));
		REQUIRE(child->isTopLevelNode());
		REQUIRE(child->containedCanvas() == canvas2);
	}
	
	SECTION("Node movement between canvases using setParent")
	{
		auto canvas1 = noco::Canvas::Create();
		auto canvas2 = noco::Canvas::Create();
		
		// Canvas1に階層構造を作成
		auto parent1 = noco::Node::Create(U"Parent1");
		auto child1 = noco::Node::Create(U"Child1");
		canvas1->addChild(parent1);
		parent1->addChild(child1);
		
		// Canvas2にノードを追加
		auto parent2 = noco::Node::Create(U"Parent2");
		canvas2->addChild(parent2);
		
		// 初期状態の確認
		REQUIRE(child1->containedCanvas() == canvas1);
		REQUIRE(parent2->containedCanvas() == canvas2);
		
		// setParentによる移動は正常に動作する（内部的に適切な処理が行われる）
		child1->setParent(parent2);
		
		// 移動後の確認
		REQUIRE(parent1->children().size() == 0);
		REQUIRE(parent2->children().size() == 1);
		REQUIRE(parent2->children()[0] == child1);
		REQUIRE(child1->containedCanvas() == canvas2);
		REQUIRE(child1->parentNode() == parent2);
	}
}

TEST_CASE("Canvas quad method", "[Canvas]")
{
	SECTION("Default Canvas quad")
	{
		auto canvas = noco::Canvas::Create(400, 300);
		auto quad = canvas->quad();
		
		// デフォルト状態では変換なしの矩形
		REQUIRE(quad.p0.x == Approx(0.0));
		REQUIRE(quad.p0.y == Approx(0.0));
		REQUIRE(quad.p1.x == Approx(400.0));
		REQUIRE(quad.p1.y == Approx(0.0));
		REQUIRE(quad.p2.x == Approx(400.0));
		REQUIRE(quad.p2.y == Approx(300.0));
		REQUIRE(quad.p3.x == Approx(0.0));
		REQUIRE(quad.p3.y == Approx(300.0));
	}
	
	SECTION("Canvas with position offset")
	{
		auto canvas = noco::Canvas::Create(200, 100);
		canvas->setPosition({50, 25});
		auto quad = canvas->quad();
		
		// 位置オフセットが適用される
		REQUIRE(quad.p0.x == Approx(50.0));
		REQUIRE(quad.p0.y == Approx(25.0));
		REQUIRE(quad.p1.x == Approx(250.0));
		REQUIRE(quad.p1.y == Approx(25.0));
		REQUIRE(quad.p2.x == Approx(250.0));
		REQUIRE(quad.p2.y == Approx(125.0));
		REQUIRE(quad.p3.x == Approx(50.0));
		REQUIRE(quad.p3.y == Approx(125.0));
	}
	
	SECTION("Canvas with scale")
	{
		auto canvas = noco::Canvas::Create(100, 100);
		canvas->setScale({2.0, 1.5});
		auto quad = canvas->quad();
		
		// スケールが適用される
		REQUIRE(quad.p0.x == Approx(0.0));
		REQUIRE(quad.p0.y == Approx(0.0));
		REQUIRE(quad.p1.x == Approx(200.0));
		REQUIRE(quad.p1.y == Approx(0.0));
		REQUIRE(quad.p2.x == Approx(200.0));
		REQUIRE(quad.p2.y == Approx(150.0));
		REQUIRE(quad.p3.x == Approx(0.0));
		REQUIRE(quad.p3.y == Approx(150.0));
	}
	
	SECTION("Canvas with position and scale")
	{
		auto canvas = noco::Canvas::Create(100, 50);
		canvas->setPosition({10, 20});
		canvas->setScale({2.0, 3.0});
		auto quad = canvas->quad();
		
		// 位置とスケール両方が適用される
		REQUIRE(quad.p0.x == Approx(10.0));
		REQUIRE(quad.p0.y == Approx(20.0));
		REQUIRE(quad.p1.x == Approx(210.0));
		REQUIRE(quad.p1.y == Approx(20.0));
		REQUIRE(quad.p2.x == Approx(210.0));
		REQUIRE(quad.p2.y == Approx(170.0));
		REQUIRE(quad.p3.x == Approx(10.0));
		REQUIRE(quad.p3.y == Approx(170.0));
	}
}

TEST_CASE("Canvas center methods", "[Canvas]")
{
	SECTION("Default Canvas center")
	{
		auto canvas = noco::Canvas::Create(400, 300);
		auto center = canvas->center();
		
		// デフォルト位置(0,0)でのCanvas中央
		REQUIRE(center.x == Approx(200.0));  // width/2
		REQUIRE(center.y == Approx(150.0));  // height/2
	}
	
	SECTION("Canvas with position offset center")
	{
		auto canvas = noco::Canvas::Create(200, 100);
		canvas->setPosition({50, 25});
		auto center = canvas->center();
		
		// 位置オフセット後のCanvas中央
		REQUIRE(center.x == Approx(150.0));  // 50 + 200/2
		REQUIRE(center.y == Approx(75.0));   // 25 + 100/2
	}
	
	SECTION("setCenter method")
	{
		auto canvas = noco::Canvas::Create(400, 300);
		canvas->setCenter({100, 80});
		
		// 中央を指定した場合の位置確認
		auto center = canvas->center();
		REQUIRE(center.x == Approx(100.0));
		REQUIRE(center.y == Approx(80.0));
		
		// 対応するpositionの確認
		auto position = canvas->position();
		REQUIRE(position.x == Approx(-100.0));  // 100 - 400/2
		REQUIRE(position.y == Approx(-70.0));   // 80 - 300/2
	}
	
	SECTION("setCenter and position consistency")
	{
		auto canvas = noco::Canvas::Create(600, 400);
		
		// 中央を設定
		canvas->setCenter({300, 200});
		
		// positionが正しく計算されているか確認
		auto position = canvas->position();
		REQUIRE(position.x == Approx(0.0));   // 300 - 600/2
		REQUIRE(position.y == Approx(0.0));   // 200 - 400/2
		
		// centerが正しく取得できるか確認
		auto center = canvas->center();
		REQUIRE(center.x == Approx(300.0));
		REQUIRE(center.y == Approx(200.0));
	}
	
	SECTION("Method chaining for setCenter")
	{
		auto canvas = noco::Canvas::Create(100, 100);
		auto result = canvas->setCenter({50, 50});
		
		// setCenterはCanvasのshared_ptrを返す
		REQUIRE(result == canvas);
		REQUIRE(canvas->center().x == Approx(50.0));
		REQUIRE(canvas->center().y == Approx(50.0));
	}
}