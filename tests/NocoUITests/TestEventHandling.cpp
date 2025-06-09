# include <catch2/catch.hpp>
# include <Siv3D.hpp>
# include <NocoUI.hpp>

// ========================================
// Canvasレベルのイベント処理テスト
// ========================================

TEST_CASE("Canvas event handling", "[Canvas][Events]")
{
	SECTION("Mouse hover tracking")
	{
		auto canvas = noco::Canvas::Create();
		auto node1 = noco::Node::Create();
		auto node2 = noco::Node::Create();
		
		node1->setConstraint(noco::BoxConstraint{ .sizeDelta = Vec2{ 100, 100 } });
		node2->setConstraint(noco::BoxConstraint{ .sizeDelta = Vec2{ 100, 100 } });
		node2->transformEffect().setPosition(Vec2{ 50, 50 });
		
		canvas->rootNode()->addChild(node1);
		canvas->rootNode()->addChild(node2);
		
		canvas->update();
		
		// node2の上でマウスオーバー（重なり優先順位のテスト）
		auto result = canvas->rootNode()->hitTest(Vec2{ 75, 75 });
		REQUIRE(result == node2);
	}

	SECTION("Focus management")
	{
		auto canvas = noco::Canvas::Create();
		auto textBox1 = noco::Node::Create();
		auto textBox2 = noco::Node::Create();
		
		auto tb1 = textBox1->emplaceComponent<noco::TextBox>();
		auto tb2 = textBox2->emplaceComponent<noco::TextBox>();
		
		canvas->rootNode()->addChild(textBox1);
		canvas->rootNode()->addChild(textBox2);
		
		// フォーカスの取得・喪失のテストは実際のイベントループが必要
		// ここでは基本的な構造のみテスト
		REQUIRE(tb1 != nullptr);
		REQUIRE(tb2 != nullptr);
	}

	SECTION("Drag operation")
	{
		auto canvas = noco::Canvas::Create();
		auto sourceNode = noco::Node::Create();
		auto targetNode = noco::Node::Create();
		
		auto dragSource = sourceNode->emplaceComponent<noco::DragDropSource>();
		bool dropReceived = false;
		auto dropTarget = targetNode->emplaceComponent<noco::DragDropTarget>(
			[&dropReceived](const Array<std::shared_ptr<noco::Node>>& nodes) {
				dropReceived = true;
			}
		);
		
		canvas->rootNode()->addChild(sourceNode);
		canvas->rootNode()->addChild(targetNode);
		
		// ドラッグ＆ドロップの基本構造のテスト
		REQUIRE(dragSource != nullptr);
		REQUIRE(dropTarget != nullptr);
		REQUIRE_FALSE(dropReceived);
	}
}