#include <catch2/catch.hpp>
#include <Siv3D.hpp>
#include <NocoUI.hpp>

// ========================================
// TextBoxのテスト
// ========================================

TEST_CASE("TextBox StyleState Functionality", "[TextBox][StyleState]")
{
	SECTION("TextBox initial styleState should be empty")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto node = noco::Node::Create(U"TestNode");
		auto textBox = node->emplaceComponent<noco::TextBox>();
		canvas->addChild(node);
		
		REQUIRE(node->styleState() == U"");
	}

	SECTION("User-set styleState is overridden by TextBox")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto node = noco::Node::Create(U"TestNode");
		auto textBox = node->emplaceComponent<noco::TextBox>();
		canvas->addChild(node);
		
		node->setStyleState(U"custom");
		REQUIRE(node->styleState() == U"custom");
		
		System::Update();
		canvas->update();
		REQUIRE(node->styleState() == U"unfocused");
		
		// ノードを非アクティブにしても現在のフレームではまだオーバーライドされている
		node->setActive(noco::ActiveYN::No);
		REQUIRE(node->styleState() == U"unfocused");
		
		node->setActive(noco::ActiveYN::Yes);
		System::Update();
		canvas->update();
		REQUIRE(node->styleState() == U"unfocused");
	}
	
	SECTION("TextBox styleState override during active state")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto node = noco::Node::Create(U"TestNode");
		auto textBox = node->emplaceComponent<noco::TextBox>();
		canvas->addChild(node);
		
		REQUIRE(node->styleState() == U"");
		
		System::Update();
		canvas->update();
		REQUIRE(node->styleState() == U"unfocused");
		
		// ユーザーが別の値を設定した場合
		node->setStyleState(U"mystate");
		System::Update();
		REQUIRE(node->styleState() == U"mystate");
		canvas->update();
		REQUIRE(node->styleState() == U"unfocused");
	}
}