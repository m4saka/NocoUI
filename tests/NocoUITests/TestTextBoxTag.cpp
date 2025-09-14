#include <catch2/catch.hpp>
#include <Siv3D.hpp>
#include <NocoUI.hpp>

// ========================================
// TextBox/TextAreaのtagプロパティのテスト
// ========================================

TEST_CASE("TextBox and TextArea tag functionality", "[TextBox][TextArea][tag]")
{
	SECTION("TextBox tag property")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create();
		canvas->addChild(node);

		auto textBox = node->emplaceComponent<noco::TextBox>();

		// タグの設定と取得
		CHECK(textBox->tag() == U"");
		textBox->setTag(U"username");
		CHECK(textBox->tag() == U"username");

		// メソッドチェーン
		auto result = textBox->setTag(U"email");
		CHECK(result == textBox);
		CHECK(textBox->tag() == U"email");
	}

	SECTION("TextArea tag property")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create();
		canvas->addChild(node);

		auto textArea = node->emplaceComponent<noco::TextArea>();

		// タグの設定と取得
		CHECK(textArea->tag() == U"");
		textArea->setTag(U"description");
		CHECK(textArea->tag() == U"description");

		// メソッドチェーン
		auto result = textArea->setTag(U"notes");
		CHECK(result == textArea);
		CHECK(textArea->tag() == U"notes");
	}

	SECTION("getTextValueByTag and setTextValueByTag on single node")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create();
		canvas->addChild(node);

		auto textBox = node->emplaceComponent<noco::TextBox>();
		textBox->setTag(U"username");
		textBox->setText(U"John Doe");

		// 単一ノードからの取得
		auto value = node->getTextValueByTag(U"username");
		CHECK(value == U"John Doe");

		// 存在しないタグ
		auto notFound = node->getTextValueByTagOpt(U"nonexistent");
		CHECK(!notFound.has_value());

		// テキストの設定
		node->setTextValueByTag(U"username", U"Jane Smith");
		CHECK(textBox->text() == U"Jane Smith");
	}

	SECTION("getTextValueByTag and setTextValueByTag with multiple components")
	{
		auto canvas = noco::Canvas::Create();
		auto node1 = noco::Node::Create();
		auto node2 = noco::Node::Create();
		canvas->addChild(node1);
		canvas->addChild(node2);

		auto textBox1 = node1->emplaceComponent<noco::TextBox>();
		textBox1->setTag(U"field");
		textBox1->setText(U"Value1");

		auto textBox2 = node2->emplaceComponent<noco::TextBox>();
		textBox2->setTag(U"field");
		textBox2->setText(U"Value2");

		// 最初に見つかったものを取得
		auto value = canvas->getTextValueByTag(U"field");
		CHECK(value == U"Value1");

		// すべてに設定
		canvas->setTextValueByTag(U"field", U"NewValue");
		CHECK(textBox1->text() == U"NewValue");
		CHECK(textBox2->text() == U"NewValue");
	}

	SECTION("Recursive search")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create();
		auto child = noco::Node::Create();
		auto grandchild = noco::Node::Create();

		canvas->addChild(parent);
		parent->addChild(child);
		child->addChild(grandchild);

		auto textBox = grandchild->emplaceComponent<noco::TextBox>();
		textBox->setTag(U"deep");
		textBox->setText(U"Deep Value");

		// 再帰的検索（デフォルト）
		auto value = canvas->getTextValueByTag(U"deep");
		CHECK(value == U"Deep Value");

		// 再帰的設定（デフォルト）
		canvas->setTextValueByTag(U"deep", U"Updated Deep");
		CHECK(textBox->text() == U"Updated Deep");

		// 非再帰的検索
		auto nonRecursive = parent->getTextValueByTagOpt(U"deep", noco::RecursiveYN::No);
		CHECK(!nonRecursive.has_value());

		// 非再帰的設定（grandchildには影響しない）
		parent->setTextValueByTag(U"deep", U"Should not change", noco::RecursiveYN::No);
		CHECK(textBox->text() == U"Updated Deep");
	}

	SECTION("Mixed TextBox and TextArea")
	{
		auto canvas = noco::Canvas::Create();
		auto node1 = noco::Node::Create();
		auto node2 = noco::Node::Create();
		canvas->addChild(node1);
		canvas->addChild(node2);

		auto textBox = node1->emplaceComponent<noco::TextBox>();
		textBox->setTag(U"input");
		textBox->setText(U"Single line");

		auto textArea = node2->emplaceComponent<noco::TextArea>();
		textArea->setTag(U"input");
		textArea->setText(U"Multi\nline\ntext");

		// TextBoxが先に見つかる
		auto value = canvas->getTextValueByTag(U"input");
		CHECK(value == U"Single line");

		// 両方に設定される
		canvas->setTextValueByTag(U"input", U"Same value");
		CHECK(textBox->text() == U"Same value");
		CHECK(textArea->text() == U"Same value");
	}

	SECTION("Empty tag handling")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create();
		canvas->addChild(node);

		auto textBox = node->emplaceComponent<noco::TextBox>();
		textBox->setText(U"Some text");
		// タグを設定しない（空文字列のまま）

		// 空のタグでの検索は何も返さない
		auto value = canvas->getTextValueByTag(U"");
		CHECK(value == U"");

		// 空のタグでの設定は何もしない
		canvas->setTextValueByTag(U"", U"Should not change");
		CHECK(textBox->text() == U"Some text");
	}

	SECTION("Multiple components with same tag")
	{
		auto canvas = noco::Canvas::Create();
		auto node1 = noco::Node::Create();
		auto node2 = noco::Node::Create();
		auto node3 = noco::Node::Create();
		canvas->addChild(node1);
		canvas->addChild(node2);
		canvas->addChild(node3);

		auto textBox1 = node1->emplaceComponent<noco::TextBox>();
		textBox1->setTag(U"form")->setText(U"Field1");

		auto textBox2 = node2->emplaceComponent<noco::TextBox>();
		textBox2->setTag(U"form")->setText(U"Field2");

		auto textBox3 = node3->emplaceComponent<noco::TextBox>();
		textBox3->setTag(U"other")->setText(U"OtherField");

		// タグ"form"のテキストを一括クリア
		canvas->setTextValueByTag(U"form", U"");
		CHECK(textBox1->text() == U"");
		CHECK(textBox2->text() == U"");
		CHECK(textBox3->text() == U"OtherField");  // tag="other"なので変更なし

		// タグ"form"のテキストを一括設定
		canvas->setTextValueByTag(U"form", U"Updated");
		CHECK(textBox1->text() == U"Updated");
		CHECK(textBox2->text() == U"Updated");
		CHECK(textBox3->text() == U"OtherField");  // tag="other"なので変更なし
	}

	SECTION("getTextValueByTagOpt tests")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create();
		auto child = noco::Node::Create();
		canvas->addChild(parent);
		parent->addChild(child);

		auto textBox = child->emplaceComponent<noco::TextBox>();
		textBox->setTag(U"inputField");
		textBox->setText(U"Test Value");

		// Optバージョンで存在するタグを取得
		auto optValue = canvas->getTextValueByTagOpt(U"inputField");
		CHECK(optValue.has_value());
		CHECK(*optValue == U"Test Value");

		// Optバージョンで存在しないタグを取得
		auto notFound = canvas->getTextValueByTagOpt(U"nonExistent");
		CHECK(!notFound.has_value());

		// 非再帰的検索で存在しないケース
		auto nonRecursive = parent->getTextValueByTagOpt(U"inputField", noco::RecursiveYN::No);
		CHECK(!nonRecursive.has_value());

		// 非再帰的検索で存在するケース
		auto parentTextBox = parent->emplaceComponent<noco::TextBox>();
		parentTextBox->setTag(U"parentInput");
		parentTextBox->setText(U"Parent Value");

		auto parentOpt = parent->getTextValueByTagOpt(U"parentInput", noco::RecursiveYN::No);
		CHECK(parentOpt.has_value());
		CHECK(*parentOpt == U"Parent Value");
	}

	SECTION("getTextValueByTagOpt with TextArea")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create();
		canvas->addChild(node);

		auto textArea = node->emplaceComponent<noco::TextArea>();
		textArea->setTag(U"description");
		textArea->setText(U"Multi\nLine\nText");

		// TextAreaからも取得できる
		auto optValue = node->getTextValueByTagOpt(U"description");
		CHECK(optValue.has_value());
		CHECK(*optValue == U"Multi\nLine\nText");

		// 存在しないタグ
		auto notFound = node->getTextValueByTagOpt(U"missing");
		CHECK(!notFound.has_value());
	}

	SECTION("getTextValueByTag with empty string default")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create();
		canvas->addChild(node);

		// タグが存在しない場合は空文字列を返す
		auto value = node->getTextValueByTag(U"notFound");
		CHECK(value == U"");

		// タグが存在する場合は実際の値を返す
		auto textBox = node->emplaceComponent<noco::TextBox>();
		textBox->setTag(U"field");
		textBox->setText(U"Actual Value");

		auto actualValue = node->getTextValueByTag(U"field");
		CHECK(actualValue == U"Actual Value");
	}
}