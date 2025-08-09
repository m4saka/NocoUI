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

// UISoundコンポーネントのテスト
TEST_CASE("UISound component", "[Component][UISound]")
{
	SECTION("Basic audio player creation")
	{
		auto node = noco::Node::Create();
		auto uiSound = node->emplaceComponent<noco::UISound>();
		
		REQUIRE(uiSound != nullptr);
	}

	SECTION("Set properties")
	{
		auto node = noco::Node::Create();
		auto uiSound = node->emplaceComponent<noco::UISound>(
			U"test.wav",
			U"testAsset",
			noco::UISound::TriggerType::HoverStart,
			0.5
		);

		REQUIRE(uiSound->audioFilePath().defaultValue == U"test.wav");
		REQUIRE(uiSound->audioAssetName().defaultValue == U"testAsset");
		REQUIRE(uiSound->triggerType() == noco::UISound::TriggerType::HoverStart);
		REQUIRE(uiSound->volume().defaultValue == 0.5);
	}

	SECTION("Method chaining for setters")
	{
		auto node = noco::Node::Create();
		auto uiSound = node->emplaceComponent<noco::UISound>();

		// メソッドチェインでプロパティを設定
		auto result = uiSound->setAudioFilePath(U"chain_test.wav")
			->setAudioAssetName(U"chainAsset")
			->setVolume(0.75)
			->setTriggerType(noco::UISound::TriggerType::Click);

		// 返り値が同じインスタンスであることを確認
		REQUIRE(result == uiSound);

		// 設定された値を確認
		REQUIRE(uiSound->audioFilePath().defaultValue == U"chain_test.wav");
		REQUIRE(uiSound->audioAssetName().defaultValue == U"chainAsset");
		REQUIRE(uiSound->volume().defaultValue == 0.75);
		REQUIRE(uiSound->triggerType() == noco::UISound::TriggerType::Click);
	}

	SECTION("Setter methods with PropertyValue")
	{
		auto node = noco::Node::Create();
		auto uiSound = node->emplaceComponent<noco::UISound>();

		// PropertyValueを使った設定
		noco::PropertyValue<String> pathValue{ U"property_test.wav" };
		uiSound->setAudioFilePath(pathValue);
		REQUIRE(uiSound->audioFilePath().defaultValue == U"property_test.wav");

		// インタラクション状態ごとの値を設定
		noco::PropertyValue<double> volumeValue{ 1.0, 0.8, 0.6, none };
		uiSound->setVolume(volumeValue);
		
		// デフォルト値を確認
		REQUIRE(uiSound->volume().defaultValue == 1.0);
		REQUIRE(uiSound->volume().hoveredValue == 0.8);
		REQUIRE(uiSound->volume().pressedValue == 0.6);
	}
}
