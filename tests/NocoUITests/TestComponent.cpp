# include <catch2/catch.hpp>
# include <Siv3D.hpp>
# include <NocoUI.hpp>

// ========================================
// Component関連のテスト
// ========================================

// Componentの基本的なテスト
TEST_CASE("Component system", "[Component]")
{
	SECTION("Add component")
	{
		auto node = noco::Node::Create();
		auto label = node->emplaceComponent<noco::Label>();
		
		REQUIRE(label != nullptr);
		REQUIRE(node->getComponentOrNull<noco::Label>() == label);
	}

	SECTION("Multiple components")
	{
		auto node = noco::Node::Create();
		auto label = node->emplaceComponent<noco::Label>();
		auto rect = node->emplaceComponent<noco::RectRenderer>();
		
		REQUIRE(node->getComponentOrNull<noco::Label>() == label);
		REQUIRE(node->getComponentOrNull<noco::RectRenderer>() == rect);
	}

	SECTION("Remove component")
	{
		auto node = noco::Node::Create();
		auto label = node->emplaceComponent<noco::Label>();
		
		node->removeComponent(label);
		
		REQUIRE(node->getComponentOrNull<noco::Label>() == nullptr);
	}
}

// Labelコンポーネントのテスト
TEST_CASE("Label component", "[Component][Label]")
{
	SECTION("Basic text properties")
	{
		auto node = noco::Node::Create();
		auto label = node->emplaceComponent<noco::Label>();
		
		// テキストの設定
		label->setText(U"Hello, World!");
		REQUIRE(label->text().defaultValue == U"Hello, World!");
	}
}

// RectRendererコンポーネントのテスト
TEST_CASE("RectRenderer component", "[Component][RectRenderer]")
{
	SECTION("Basic rect renderer creation")
	{
		auto node = noco::Node::Create();
		auto rect = node->emplaceComponent<noco::RectRenderer>();
		
		REQUIRE(rect != nullptr);
	}
}

// TextBoxコンポーネントのテスト
TEST_CASE("TextBox component", "[Component][TextBox]")
{
	SECTION("Basic text properties")
	{
		auto node = noco::Node::Create();
		auto textBox = node->emplaceComponent<noco::TextBox>();
		
		// テキストの設定と取得
		textBox->setText(U"Initial text");
		REQUIRE(textBox->text() == U"Initial text");
	}
}

// Spriteコンポーネントのテスト
TEST_CASE("Sprite component", "[Component][Sprite]")
{
	SECTION("Basic sprite creation")
	{
		auto node = noco::Node::Create();
		auto sprite = node->emplaceComponent<noco::Sprite>();
		
		REQUIRE(sprite != nullptr);
	}
}

// TextAreaコンポーネントのテスト
TEST_CASE("TextArea component", "[Component][TextArea]")
{
	SECTION("Basic text properties")
	{
		auto node = noco::Node::Create();
		auto textArea = node->emplaceComponent<noco::TextArea>();
		
		// 複数行テキストの設定
		String multilineText = U"Line 1\nLine 2\nLine 3";
		textArea->setText(multilineText);
		REQUIRE(textArea->text() == multilineText);
	}
}

// InputBlockerコンポーネントのテスト
TEST_CASE("InputBlocker component", "[Component][InputBlocker]")
{
	SECTION("Basic functionality")
	{
		auto node = noco::Node::Create();
		auto blocker = node->emplaceComponent<noco::InputBlocker>();
		
		// InputBlockerは特定のプロパティを持たないシンプルなコンポーネント
		// 存在することで入力をブロックする
		REQUIRE(blocker != nullptr);
	}
}

// DragDropSource/Targetコンポーネントのテスト
TEST_CASE("DragDrop components", "[Component][DragDrop]")
{
	SECTION("DragDropSource")
	{
		auto node = noco::Node::Create();
		auto dragSource = node->emplaceComponent<noco::DragDropSource>();
		
		// DragDropSourceの基本機能
		REQUIRE(dragSource != nullptr);
	}

	SECTION("DragDropTarget")
	{
		auto node = noco::Node::Create();
		auto dropTarget = node->emplaceComponent<noco::DragDropTarget>(
			[](const Array<std::shared_ptr<noco::Node>>&) {}
		);
		
		// DragDropTargetの基本機能
		REQUIRE(dropTarget != nullptr);
	}
}