#include <catch2/catch.hpp>
#include <Siv3D.hpp>
#include <NocoUI.hpp>

// ========================================
// Canvasのテスト
// ========================================

TEST_CASE("Canvas system", "[Canvas]")
{
	SECTION("Strict parent checking for addChild")
	{
		auto canvas1 = noco::Canvas::Create();
		auto canvas2 = noco::Canvas::Create();
		
		// Canvas1にトップレベルノードを追加
		auto nodeA = noco::Node::Create(U"NodeA");
		auto nodeB = noco::Node::Create(U"NodeB");
		canvas1->addChild(nodeA);
		canvas1->addChild(nodeB);
		
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
		
		REQUIRE(child1->containedCanvas() == canvas1);
		REQUIRE(parent2->containedCanvas() == canvas2);
		
		// setParentによる移動は正常に動作する（既存の親から切り離される）
		child1->setParent(parent2);
		
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

// activeInHierarchy管理のテスト
TEST_CASE("Canvas activeInHierarchy management", "[Canvas][ActiveInHierarchy]")
{
	SECTION("addChild makes node activeInHierarchy")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto node = noco::Node::Create(U"TestNode");
		
		// Canvas配下にない場合はactiveInHierarchyはfalse
		REQUIRE(node->activeInHierarchy() == noco::ActiveYN::No);
		
		// Canvas::addChild後にactiveInHierarchyがtrueになる
		canvas->addChild(node);
		REQUIRE(node->activeInHierarchy() == noco::ActiveYN::Yes);
	}
	
	SECTION("emplaceChild creates node with activeInHierarchy")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		
		// Canvas::emplaceChildで作成されたノードは直接activeInHierarchyがtrue
		auto& node = canvas->emplaceChild(U"EmplacedNode");
		REQUIRE(node->activeInHierarchy() == noco::ActiveYN::Yes);
	}
	
	SECTION("Hierarchical activeInHierarchy propagation")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto parent = noco::Node::Create(U"Parent");
		auto child = noco::Node::Create(U"Child");
		auto grandchild = noco::Node::Create(U"Grandchild");
		
		// 階層構造を作成（まだCanvas配下にない）
		parent->addChild(child);
		child->addChild(grandchild);
		
		// Canvas配下にない場合は全てfalse
		REQUIRE(parent->activeInHierarchy() == noco::ActiveYN::No);
		REQUIRE(child->activeInHierarchy() == noco::ActiveYN::No);
		REQUIRE(grandchild->activeInHierarchy() == noco::ActiveYN::No);
		
		// 親をCanvas配下に追加すると、子孫も含めてactiveInHierarchyがtrueになる
		canvas->addChild(parent);
		REQUIRE(parent->activeInHierarchy() == noco::ActiveYN::Yes);
		REQUIRE(child->activeInHierarchy() == noco::ActiveYN::Yes);
		REQUIRE(grandchild->activeInHierarchy() == noco::ActiveYN::Yes);
	}
	
	SECTION("removeChild makes node activeInHierarchy false")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto parent = noco::Node::Create(U"Parent");
		auto child = noco::Node::Create(U"Child");
		
		parent->addChild(child);
		canvas->addChild(parent);
		
		// Canvas配下でactiveInHierarchyがtrue
		REQUIRE(parent->activeInHierarchy() == noco::ActiveYN::Yes);
		REQUIRE(child->activeInHierarchy() == noco::ActiveYN::Yes);
		
		// Canvas::removeChild後にactiveInHierarchyがfalseになる
		canvas->removeChild(parent);
		REQUIRE(parent->activeInHierarchy() == noco::ActiveYN::No);
		REQUIRE(child->activeInHierarchy() == noco::ActiveYN::No);
	}
	
	SECTION("Node activeSelf affects activeInHierarchy under Canvas")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto parent = noco::Node::Create(U"Parent");
		auto child = noco::Node::Create(U"Child");
		
		parent->addChild(child);
		canvas->addChild(parent);
		
		// 初期状態：両方ともactiveInHierarchyがtrue
		REQUIRE(parent->activeInHierarchy() == noco::ActiveYN::Yes);
		REQUIRE(child->activeInHierarchy() == noco::ActiveYN::Yes);
		
		// 親を非アクティブにすると子のactiveInHierarchyもfalseになる
		parent->setActive(noco::ActiveYN::No);
		REQUIRE(parent->activeInHierarchy() == noco::ActiveYN::No);
		REQUIRE(child->activeInHierarchy() == noco::ActiveYN::No);
		
		// 親を再アクティブにすると子のactiveInHierarchyもtrueになる
		parent->setActive(noco::ActiveYN::Yes);
		REQUIRE(parent->activeInHierarchy() == noco::ActiveYN::Yes);
		REQUIRE(child->activeInHierarchy() == noco::ActiveYN::Yes);
	}
	
	SECTION("removeChildrenAll makes all nodes activeInHierarchy false")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto node1 = noco::Node::Create(U"Node1");
		auto node2 = noco::Node::Create(U"Node2");
		auto child = noco::Node::Create(U"Child");
		
		// 階層構造を作成
		node1->addChild(child);
		canvas->addChild(node1);
		canvas->addChild(node2);
		
		// Canvas配下でactiveInHierarchyがtrue
		REQUIRE(node1->activeInHierarchy() == noco::ActiveYN::Yes);
		REQUIRE(node2->activeInHierarchy() == noco::ActiveYN::Yes);
		REQUIRE(child->activeInHierarchy() == noco::ActiveYN::Yes);
		REQUIRE(canvas->children().size() == 2);
		
		// removeChildrenAll後にactiveInHierarchyがfalseになる
		canvas->removeChildrenAll();
		REQUIRE(node1->activeInHierarchy() == noco::ActiveYN::No);
		REQUIRE(node2->activeInHierarchy() == noco::ActiveYN::No);
		REQUIRE(child->activeInHierarchy() == noco::ActiveYN::No);
		REQUIRE(canvas->children().size() == 0);
	}
}

TEST_CASE("Canvas styleState integration", "[Canvas][StyleState]")
{
	SECTION("emplaceChild with TextBox sets unfocused")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto node = canvas->emplaceChild(U"TextBoxNode");
		auto textBox = node->emplaceComponent<noco::TextBox>();
		
		// emplaceChildで作成したノードにTextBoxを追加すると自動的にunfocused
		REQUIRE(node->styleState() == U"unfocused");
	}
	
	SECTION("emplaceChild with TextArea sets unfocused")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto node = canvas->emplaceChild(U"TextAreaNode");
		auto textArea = node->emplaceComponent<noco::TextArea>();
		
		// emplaceChildで作成したノードにTextAreaを追加すると自動的にunfocused
		REQUIRE(node->styleState() == U"unfocused");
	}
	
	SECTION("Complex hierarchy with text components styleState")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto form = canvas->emplaceChild(U"Form");
		auto nameField = form->emplaceChild(U"NameField");
		auto descField = form->emplaceChild(U"DescField");
		
		// テキストコンポーネントを追加
		auto nameTextBox = nameField->emplaceComponent<noco::TextBox>();
		auto descTextArea = descField->emplaceComponent<noco::TextArea>();
		
		// 階層構造でもそれぞれ正しくunfocusedになる
		REQUIRE(form->styleState() == U"");  // テキストコンポーネントなし
		REQUIRE(nameField->styleState() == U"unfocused");  // TextBox
		REQUIRE(descField->styleState() == U"unfocused");  // TextArea
	}
}