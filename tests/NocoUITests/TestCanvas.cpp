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
		REQUIRE(canvas->rootNode() != nullptr);
		REQUIRE(canvas->rootNode()->name() == U"Canvas");
	}

	SECTION("Add node to canvas")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create(U"TestNode");
		
		canvas->rootNode()->addChild(node);
		
		REQUIRE(canvas->rootNode()->children().size() == 1);
		REQUIRE(canvas->rootNode()->children()[0] == node);
	}
}