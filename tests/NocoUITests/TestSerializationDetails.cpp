# include <catch2/catch.hpp>
# include <Siv3D.hpp>
# include <NocoUI.hpp>

// ========================================
// シリアライゼーション詳細テスト
// ========================================

TEST_CASE("Serialization details", "[Serialization]")
{
	SECTION("Component serialization")
	{
		auto node = noco::Node::Create();
		
		// Labelコンポーネントの追加と設定
		auto label = node->emplaceComponent<noco::Label>();
		label->setText(U"Test Label");
		
		// RectRendererコンポーネントの追加
		auto rect = node->emplaceComponent<noco::RectRenderer>();
		
		// JSON変換
		JSON json = node->toJSON();
		
		// コンポーネント情報が含まれているか確認
		REQUIRE(json.hasElement(U"components"));
		REQUIRE(json[U"components"].isArray());
	}

	SECTION("Layout serialization")
	{
		auto parent = noco::Node::Create();
		
		// HorizontalLayoutの設定
		noco::HorizontalLayout layout;
		layout.spacing = 15.0f;
		layout.padding = noco::LRTB{ 10, 10, 5, 5 };
		parent->setBoxChildrenLayout(layout);
		
		// JSON変換
		JSON json = parent->toJSON();
		
		// レイアウト情報が含まれているか確認
		REQUIRE(json.hasElement(U"boxChildrenLayout"));
	}

	SECTION("Complex hierarchy serialization")
	{
		auto canvas = noco::Canvas::Create();
		auto root = canvas->rootNode();
		
		// 複雑な階層構造を作成
		auto parent = noco::Node::Create(U"Parent");
		parent->setConstraint(noco::BoxConstraint{ .sizeDelta = Vec2{ 300, 200 } });
		
		auto child1 = noco::Node::Create(U"Child1");
		child1->emplaceComponent<noco::Label>()->setText(U"Label1");
		
		auto child2 = noco::Node::Create(U"Child2");
		child2->emplaceComponent<noco::TextBox>()->setText(U"TextBox1");
		
		auto grandchild = noco::Node::Create(U"Grandchild");
		grandchild->emplaceComponent<noco::RectRenderer>();
		
		parent->addChild(child1);
		parent->addChild(child2);
		child2->addChild(grandchild);
		root->addChild(parent);
		
		// Canvas全体をJSON化
		JSON canvasJson = canvas->toJSON();
		
		// 階層構造が保持されているか確認
		REQUIRE(canvasJson.hasElement(U"rootNode"));
		REQUIRE(canvasJson[U"rootNode"].hasElement(U"children"));
	}
}