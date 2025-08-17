#include <catch2/catch.hpp>
#include <Siv3D.hpp>
#include <NocoUI.hpp>

// ========================================
// TextBoxのテスト
// ========================================

TEST_CASE("TextBox StyleState Functionality", "[TextBox][StyleState]")
{
	SECTION("TextBox initial styleState should be unfocused")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto node = noco::Node::Create(U"TestNode");
		auto textBox = node->emplaceComponent<noco::TextBox>();
		canvas->addChild(node);
		
		// 初期状態ではunfocusedであること
		REQUIRE(node->styleState() == U"unfocused");
	}

	SECTION("TextBox styleState should be cleared when deactivated")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto node = noco::Node::Create(U"TestNode");
		auto textBox = node->emplaceComponent<noco::TextBox>();
		canvas->addChild(node);
		
		// 初期状態ではunfocusedであること
		REQUIRE(node->styleState() == U"unfocused");
		
		// focusedに変更
		node->setStyleState(U"focused");
		REQUIRE(node->styleState() == U"focused");
		
		// ノードを非アクティブにしてstyleStateがクリアされることを確認
		node->setActive(noco::ActiveYN::No);
		REQUIRE(node->styleState() == U"");
		
		// 再度アクティブにしてunfocusedに戻ることを確認
		node->setActive(noco::ActiveYN::Yes);
		REQUIRE(node->styleState() == U"unfocused");
	}

	SECTION("TextBox styleState should be cleared when component is removed")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto node = noco::Node::Create(U"TestNode");
		auto textBox = node->emplaceComponent<noco::TextBox>();
		canvas->addChild(node);
		
		// focusedに変更
		node->setStyleState(U"focused");
		REQUIRE(node->styleState() == U"focused");
		
		// コンポーネントを削除してstyleStateがクリアされることを確認
		node->removeComponent(textBox);
		REQUIRE(node->styleState() == U"");
	}

	SECTION("TextBox with Canvas::emplaceChild")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto node = canvas->emplaceChild(U"TextBoxNode");
		auto textBox = node->emplaceComponent<noco::TextBox>();
		
		// emplaceChildでも正しくunfocusedになること
		REQUIRE(node->styleState() == U"unfocused");
	}
	
	SECTION("TextBox addComponent vs emplaceComponent")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto node1 = noco::Node::Create(U"Node1");
		auto node2 = noco::Node::Create(U"Node2");
		canvas->addChild(node1);
		canvas->addChild(node2);
		
		// addComponentでの追加
		auto textBox1 = std::make_shared<noco::TextBox>();
		node1->addComponent(textBox1);
		REQUIRE(node1->styleState() == U"unfocused");
		
		// emplaceComponentでの追加
		auto textBox2 = node2->emplaceComponent<noco::TextBox>();
		REQUIRE(node2->styleState() == U"unfocused");
	}
	
	SECTION("TextBox serialization preserves styleState behavior")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto node = noco::Node::Create(U"TextBoxNode");
		canvas->addChild(node);
		auto textBox = node->emplaceComponent<noco::TextBox>();
		
		// 初期状態の確認
		REQUIRE(node->styleState() == U"unfocused");
		
		// JSON出力
		JSON json = node->toJSON();
		
		// 復元後のテスト用に新しいCanvas
		auto canvas2 = noco::Canvas::Create(SizeF{ 800, 600 });
		auto restoredNode = noco::Node::CreateFromJSON(json);
		canvas2->addChild(restoredNode);
		
		// 復元後も自動的にunfocusedになること
		REQUIRE(restoredNode->styleState() == U"unfocused");
	}
}