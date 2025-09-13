#include <catch2/catch.hpp>
#include <Siv3D.hpp>
#include <NocoUI.hpp>

// ========================================
// Toggleコンポーネントのテスト
// ========================================

TEST_CASE("Toggle component", "[Toggle]")
{
	SECTION("Toggle value and styleState override")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create();
		canvas->addChild(node);

		auto toggle = node->emplaceComponent<noco::Toggle>();

		// 初期値はfalse
		CHECK(toggle->value() == false);

		// update前はstyleStateは空
		CHECK(node->styleState() == U"");

		// canvas->update()でstyleStateが"off"に上書きされる
		canvas->update();
		CHECK(node->styleState() == U"off");

		// クリックして値を反転
		node->requestClick();
		canvas->update();
		CHECK(toggle->value() == true);
		CHECK(node->styleState() == U"on");

		// もう一度クリックして値を反転
		node->requestClick();
		canvas->update();
		CHECK(toggle->value() == false);
		CHECK(node->styleState() == U"off");

		// setValue()で直接値を設定
		toggle->setValue(true);
		canvas->update();
		CHECK(toggle->value() == true);
		CHECK(node->styleState() == U"on");

		toggle->setValue(false);
		canvas->update();
		CHECK(toggle->value() == false);
		CHECK(node->styleState() == U"off");
	}

	SECTION("Toggle tag property")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create();
		canvas->addChild(node);

		auto toggle = node->emplaceComponent<noco::Toggle>();

		// タグの設定と取得
		CHECK(toggle->tag() == U"");
		toggle->setTag(U"darkMode");
		CHECK(toggle->tag() == U"darkMode");

		// メソッドチェーン
		auto result = toggle->setTag(U"enableNotifications");
		CHECK(result == toggle);
		CHECK(toggle->tag() == U"enableNotifications");
	}

	SECTION("getToggleValueByTag and setToggleValueByTag on single node")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create();
		canvas->addChild(node);

		auto toggle = node->emplaceComponent<noco::Toggle>();
		toggle->setTag(U"darkMode");
		toggle->setValue(true);

		// 単一ノードからの取得
		auto value = node->getToggleValueByTag(U"darkMode");
		CHECK(value.has_value());
		CHECK(*value == true);

		// 存在しないタグ
		auto notFound = node->getToggleValueByTag(U"nonexistent");
		CHECK(!notFound.has_value());

		// 値の設定
		node->setToggleValueByTag(U"darkMode", false);
		CHECK(toggle->value() == false);
	}

	SECTION("getToggleValueByTag and setToggleValueByTag with multiple components")
	{
		auto canvas = noco::Canvas::Create();
		auto node1 = noco::Node::Create();
		auto node2 = noco::Node::Create();
		canvas->addChild(node1);
		canvas->addChild(node2);

		auto toggle1 = node1->emplaceComponent<noco::Toggle>();
		toggle1->setTag(U"option");
		toggle1->setValue(true);

		auto toggle2 = node2->emplaceComponent<noco::Toggle>();
		toggle2->setTag(U"option");
		toggle2->setValue(false);

		// 最初に見つかったものを取得
		auto value = canvas->getToggleValueByTag(U"option");
		CHECK(value.has_value());
		CHECK(*value == true);

		// すべてに設定
		canvas->setToggleValueByTag(U"option", false);
		CHECK(toggle1->value() == false);
		CHECK(toggle2->value() == false);

		canvas->setToggleValueByTag(U"option", true);
		CHECK(toggle1->value() == true);
		CHECK(toggle2->value() == true);
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

		auto toggle = grandchild->emplaceComponent<noco::Toggle>();
		toggle->setTag(U"deepToggle");
		toggle->setValue(true);

		// 再帰的検索（デフォルト）
		auto value = canvas->getToggleValueByTag(U"deepToggle");
		CHECK(value.has_value());
		CHECK(*value == true);

		// 再帰的設定（デフォルト）
		canvas->setToggleValueByTag(U"deepToggle", false);
		CHECK(toggle->value() == false);

		// 非再帰的検索
		auto nonRecursive = parent->getToggleValueByTag(U"deepToggle", noco::RecursiveYN::No);
		CHECK(!nonRecursive.has_value());

		// 非再帰的設定（grandchildには影響しない）
		toggle->setValue(true);
		parent->setToggleValueByTag(U"deepToggle", false, noco::RecursiveYN::No);
		CHECK(toggle->value() == true);  // 変更されない
	}

	SECTION("Empty tag handling")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create();
		canvas->addChild(node);

		auto toggle = node->emplaceComponent<noco::Toggle>();
		toggle->setValue(true);
		// タグを設定しない（空文字列のまま）

		// 空のタグでの検索は何も返さない
		auto value = canvas->getToggleValueByTag(U"");
		CHECK(!value.has_value());

		// 空のタグでの設定は何もしない
		canvas->setToggleValueByTag(U"", false);
		CHECK(toggle->value() == true);  // 変更されない
	}

	SECTION("Multiple toggles with same tag")
	{
		auto canvas = noco::Canvas::Create();
		auto node1 = noco::Node::Create();
		auto node2 = noco::Node::Create();
		auto node3 = noco::Node::Create();
		canvas->addChild(node1);
		canvas->addChild(node2);
		canvas->addChild(node3);

		auto toggle1 = node1->emplaceComponent<noco::Toggle>();
		toggle1->setTag(U"settings")->setValue(true);

		auto toggle2 = node2->emplaceComponent<noco::Toggle>();
		toggle2->setTag(U"settings")->setValue(false);

		auto toggle3 = node3->emplaceComponent<noco::Toggle>();
		toggle3->setTag(U"other")->setValue(true);

		// タグ"settings"の値を一括でfalseに
		canvas->setToggleValueByTag(U"settings", false);
		CHECK(toggle1->value() == false);
		CHECK(toggle2->value() == false);
		CHECK(toggle3->value() == true);  // tag="other"なので変更なし

		// タグ"settings"の値を一括でtrueに
		canvas->setToggleValueByTag(U"settings", true);
		CHECK(toggle1->value() == true);
		CHECK(toggle2->value() == true);
		CHECK(toggle3->value() == true);  // tag="other"なので変更なし
	}

	SECTION("Initial value in constructor")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create();
		canvas->addChild(node);

		// コンストラクタで初期値を設定
		auto toggle = std::make_shared<noco::Toggle>(true);
		node->addComponent(toggle);

		CHECK(toggle->value() == true);
		canvas->update();
		CHECK(node->styleState() == U"on");
	}
}