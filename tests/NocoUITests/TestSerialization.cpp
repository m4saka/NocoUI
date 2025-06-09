# include <catch2/catch.hpp>
# include <Siv3D.hpp>
# include <NocoUI.hpp>

// ========================================
// シリアライゼーションのテスト
// ========================================

TEST_CASE("Serialization", "[Node][Canvas][JSON]")
{
	SECTION("Node to JSON")
	{
		auto node = noco::Node::Create(U"TestNode");
		node->transformEffect().setPosition(Vec2{ 100, 200 });
		node->transformEffect().setScale(Vec2{ 2, 2 });
		
		// JSONに変換
		JSON json = node->toJSON();
		
		// JSON内容の確認
		REQUIRE(json[U"name"].getString() == U"TestNode");
	}
}