#include <catch2/catch.hpp>
#include <Siv3D.hpp>
#include <NocoUI.hpp>

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
		parent->setChildrenLayout(layout);
		
		// JSON変換
		JSON json = parent->toJSON();
		
		// レイアウト情報が含まれているか確認
		REQUIRE(json.hasElement(U"childrenLayout"));
	}

	SECTION("Complex hierarchy serialization")
	{
		auto canvas = noco::Canvas::Create();
		auto root = canvas->rootNode();
		
		// 複雑な階層構造を作成
		auto parent = noco::Node::Create(U"Parent");
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 300, 200 } });
		
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
		JSON rootNodeJson = canvasJson[U"rootNode"];
		REQUIRE(rootNodeJson.hasElement(U"children"));
		REQUIRE(rootNodeJson[U"children"].size() == 1);
		REQUIRE(rootNodeJson[U"children"][0][U"name"].get<String>() == U"Parent");
	}

	SECTION("Canvas round-trip serialization")
	{
		// Canvasを作成して複雑な構造を構築
		auto canvas1 = noco::Canvas::Create();
		auto root1 = canvas1->rootNode();
		
		auto node1 = noco::Node::Create(U"TestNode");
		node1->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 50 } });
		node1->emplaceComponent<noco::Label>()->setText(U"TestLabel");
		root1->addChild(node1);
		
		// JSON化
		JSON json = canvas1->toJSON();
		
		// JSONから新しいCanvasを作成
		auto canvas2 = noco::Canvas::CreateFromJSON(json);
		REQUIRE(canvas2 != nullptr);
		
		// 再度JSON化して同じ構造か確認
		JSON json2 = canvas2->toJSON();
		
		REQUIRE(json2[U"rootNode"][U"children"].size() == 1);
		REQUIRE(json2[U"rootNode"][U"children"][0][U"name"].get<String>() == U"TestNode");
	}

	SECTION("Error handling for missing rootNode field")
	{
		JSON invalidJson = JSON{};
		invalidJson[U"version"] = noco::NocoUIVersion;
		
		auto canvas = noco::Canvas::CreateFromJSON(invalidJson);
		REQUIRE(canvas == nullptr);
	}


	SECTION("Version field is correctly written and read")
	{
		auto canvas = noco::Canvas::Create();
		JSON json = canvas->toJSON();
		
		// versionフィールドが存在し、正しい値を持つことを確認
		REQUIRE(json.hasElement(U"version"));
		REQUIRE(json[U"version"].isString());
		REQUIRE(json[U"version"].get<String>() == noco::NocoUIVersion);
	}
}