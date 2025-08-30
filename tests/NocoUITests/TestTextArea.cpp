#include <catch2/catch.hpp>
#include <Siv3D.hpp>
#include <NocoUI.hpp>

// ========================================
// TextAreaのテスト
// ========================================

TEST_CASE("TextArea StyleState Functionality", "[TextArea][StyleState]")
{
	SECTION("TextArea initial styleState should be empty")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto node = noco::Node::Create(U"TestNode");
		auto textArea = node->emplaceComponent<noco::TextArea>();
		canvas->addChild(node);
		
		REQUIRE(node->styleState() == U"");
	}

	SECTION("User-set styleState is overridden by TextArea")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto node = noco::Node::Create(U"TestNode");
		auto textArea = node->emplaceComponent<noco::TextArea>();
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
	
	SECTION("TextArea styleState override during active state")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto node = noco::Node::Create(U"TestNode");
		auto textArea = node->emplaceComponent<noco::TextArea>();
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