#include <catch2/catch.hpp>
#include <Siv3D.hpp>
#include <NocoUI.hpp>

// ========================================
// 親子関係のテスト
// ========================================

TEST_CASE("Parent-child relationships", "[Parent]")
{
	SECTION("Move top-level node to child within same Canvas")
	{
		auto canvas = noco::Canvas::Create();
		
		auto nodeA = noco::Node::Create(U"NodeA");
		auto nodeB = noco::Node::Create(U"NodeB");
		canvas->addChild(nodeA);
		canvas->addChild(nodeB);
		
		REQUIRE(canvas->children().size() == 2);
		REQUIRE(nodeA->isTopLevelNode());
		REQUIRE(nodeB->isTopLevelNode());
		
		// 同じCanvas内でトップレベルノードAをノードBの子に移動
		nodeA->setParent(nodeB);
		
		REQUIRE(canvas->children().size() == 1);  // nodeAがCanvas直下から削除
		REQUIRE(canvas->children()[0] == nodeB);
		REQUIRE(nodeB->children().size() == 1);
		REQUIRE(nodeB->children()[0] == nodeA);
		REQUIRE(!nodeA->isTopLevelNode());
		REQUIRE(nodeA->parentNode() == nodeB);
		REQUIRE(nodeA->containedCanvas() == canvas);  // Canvasは変わらない
	}
	
	SECTION("Circular reference prevention - parent to child")
	{
		auto canvas = noco::Canvas::Create();
		
		auto parent = noco::Node::Create(U"Parent");
		auto child = noco::Node::Create(U"Child");
		auto grandChild = noco::Node::Create(U"GrandChild");
		
		canvas->addChild(parent);
		parent->addChild(child);
		child->addChild(grandChild);
		
		// 親を子の子にしようとする（循環参照）
		REQUIRE_THROWS_AS(parent->setParent(child), s3d::Error);
		
		// setParentで例外が発生した場合でも、元のCanvasから削除されない
		REQUIRE(canvas->children().size() == 1);
		REQUIRE(parent->containedCanvas() == canvas);
		REQUIRE(parent->children().size() == 1);
		REQUIRE(child->children().size() == 1);
		
		
		// 孫への循環参照も確認
		REQUIRE_THROWS_AS(parent->setParent(grandChild), s3d::Error);
	}
	
	SECTION("Circular reference prevention - self reference")
	{
		auto canvas = noco::Canvas::Create();
		
		auto node = noco::Node::Create(U"Node");
		canvas->addChild(node);
		
		// 自分自身を自分の子にしようとする
		REQUIRE_THROWS_AS(node->setParent(node), s3d::Error);
		
		// setParentで例外が発生した場合でも、元のCanvasから削除されない
		REQUIRE(canvas->children().size() == 1);
		REQUIRE(node->containedCanvas() == canvas);
		REQUIRE(node->children().size() == 0);
	}
	
	SECTION("setParent with same parent does nothing")
	{
		auto canvas = noco::Canvas::Create();
		
		auto parent = noco::Node::Create(U"Parent");
		auto child = noco::Node::Create(U"Child");
		canvas->addChild(parent);
		parent->addChild(child);
		
		// 同じ親を再設定
		child->setParent(parent);
		
		// 何も変わらないことを確認
		REQUIRE(parent->children().size() == 1);
		REQUIRE(parent->children()[0] == child);
		REQUIRE(child->parentNode() == parent);
	}
	
	SECTION("addChild with nullptr")
	{
		auto canvas = noco::Canvas::Create();
		
		// nullptrをaddChild
		REQUIRE_THROWS_AS(canvas->addChild(nullptr), s3d::Error);
		
		// Canvasは変わっていないことを確認
		REQUIRE(canvas->children().size() == 0);
	}
	
	SECTION("removeFromParent for top-level node")
	{
		auto canvas = noco::Canvas::Create();
		
		auto node = noco::Node::Create(U"Node");
		canvas->addChild(node);
		
		// トップレベルノードでremoveFromParent
		REQUIRE(node->isTopLevelNode());
		bool removed = node->removeFromParent();
		
		REQUIRE(removed == true);
		REQUIRE(canvas->children().size() == 0);
		REQUIRE(node->containedCanvas() == nullptr);
		REQUIRE(!node->isTopLevelNode());
	}
	
	SECTION("removeFromParent for child node")
	{
		auto canvas = noco::Canvas::Create();
		
		auto parent = noco::Node::Create(U"Parent");
		auto child = noco::Node::Create(U"Child");
		canvas->addChild(parent);
		parent->addChild(child);
		
		// 子ノードでremoveFromParent
		REQUIRE(!child->isTopLevelNode());
		bool removed = child->removeFromParent();
		
		REQUIRE(removed == true);
		REQUIRE(parent->children().size() == 0);
		REQUIRE(child->parentNode() == nullptr);
		REQUIRE(child->containedCanvas() == nullptr);
	}
	
	SECTION("removeFromParent for orphan node")
	{
		auto node = noco::Node::Create(U"OrphanNode");
		
		// 親もCanvasも持たないノードでremoveFromParent
		REQUIRE(node->parentNode() == nullptr);
		REQUIRE(node->containedCanvas() == nullptr);
		bool removed = node->removeFromParent();
		
		REQUIRE(removed == false);
		// 状態は変わらない
		REQUIRE(node->parentNode() == nullptr);
		REQUIRE(node->containedCanvas() == nullptr);
	}
	
	SECTION("Parent removal consistency")
	{
		auto canvas = noco::Canvas::Create();
		
		auto parent = noco::Node::Create(U"Parent");
		auto child = noco::Node::Create(U"Child");
		
		canvas->addChild(parent);
		parent->addChild(child);
		
		REQUIRE(child->parentNode() == parent);
		REQUIRE(!child->isTopLevelNode());
		
		// 親から削除
		parent->removeChild(child);
		
		REQUIRE(parent->children().size() == 0);
		REQUIRE(child->parentNode() == nullptr);
		REQUIRE(child->containedCanvas() == nullptr);  // Canvasからも切り離される
		
		// Canvasの構造は変わらない
		REQUIRE(canvas->children().size() == 1);
		REQUIRE(parent->children().size() == 0);
	}
	
	SECTION("siblingIndex for top-level nodes")
	{
		auto canvas = noco::Canvas::Create();
		
		auto nodeA = noco::Node::Create(U"NodeA");
		auto nodeB = noco::Node::Create(U"NodeB");
		auto nodeC = noco::Node::Create(U"NodeC");
		
		canvas->addChild(nodeA);
		canvas->addChild(nodeB);
		canvas->addChild(nodeC);
		
		// トップレベルノードもCanvasの子としてsiblingIndexが取得できる
		REQUIRE(nodeA->siblingIndex() == 0);
		REQUIRE(nodeB->siblingIndex() == 1);
		REQUIRE(nodeC->siblingIndex() == 2);
		
		// siblingIndexOptも同様に値を返す
		REQUIRE(nodeA->siblingIndexOpt().has_value());
		REQUIRE(nodeA->siblingIndexOpt().value() == 0);
		REQUIRE(nodeB->siblingIndexOpt().value() == 1);
		REQUIRE(nodeC->siblingIndexOpt().value() == 2);
		
		// トップレベルノードの順序確認
		REQUIRE(canvas->children()[0] == nodeA);
		REQUIRE(canvas->children()[1] == nodeB);
		REQUIRE(canvas->children()[2] == nodeC);
	}
	
	SECTION("siblingIndex for child nodes")
	{
		auto canvas = noco::Canvas::Create();
		
		auto parent = noco::Node::Create(U"Parent");
		auto childA = noco::Node::Create(U"ChildA");
		auto childB = noco::Node::Create(U"ChildB");
		auto childC = noco::Node::Create(U"ChildC");
		
		canvas->addChild(parent);
		parent->addChild(childA);
		parent->addChild(childB);
		parent->addChild(childC);
		
		// 子ノードのsiblingIndex確認
		REQUIRE(childA->siblingIndex() == 0);
		REQUIRE(childB->siblingIndex() == 1);
		REQUIRE(childC->siblingIndex() == 2);
		
		// siblingIndexOptも確認
		REQUIRE(childA->siblingIndexOpt().has_value());
		REQUIRE(childA->siblingIndexOpt().value() == 0);
		REQUIRE(childB->siblingIndexOpt().value() == 1);
		REQUIRE(childC->siblingIndexOpt().value() == 2);
		
		// ノードを削除して順序が変わることを確認
		parent->removeChild(childB);
		REQUIRE(childA->siblingIndex() == 0);
		REQUIRE(childC->siblingIndex() == 1);  // childBが削除されたのでインデックスが繰り上がる
	}
	
	SECTION("siblingIndex for orphan node")
	{
		auto orphanNode = noco::Node::Create(U"OrphanNode");
		
		// 親もCanvasも持たないノードのsiblingIndexOpt
		REQUIRE(!orphanNode->siblingIndexOpt().has_value());
		
		// siblingIndex()は例外を投げる
		REQUIRE_THROWS_AS(orphanNode->siblingIndex(), s3d::Error);
	}
}