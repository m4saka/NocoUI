#include <catch2/catch.hpp>
#include <Siv3D.hpp>
#include <NocoUI.hpp>

// ========================================
// シリアライズの詳細テスト
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
		canvas->addChild(parent);
		
		// Canvas全体をJSON化
		JSON canvasJson = canvas->toJSON();
		
		// 階層構造が保持されているか確認
		REQUIRE(canvasJson.hasElement(U"children"));
		REQUIRE(canvasJson[U"children"].size() == 1);
		REQUIRE(canvasJson[U"children"][0][U"name"].get<String>() == U"Parent");
	}

	SECTION("Canvas round-trip serialization")
	{
		// Canvasを作成して複雑な構造を構築
		auto canvas1 = noco::Canvas::Create();
		canvas1->setSize(1024, 768);
		
		// ルートノードを追加
		auto root1 = noco::Node::Create(U"Root");
		canvas1->addChild(root1);
		
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
		
		// ルートノードの下に1つの子ノードがある
		REQUIRE(json2[U"children"].size() == 1);
		REQUIRE(json2[U"children"][0][U"children"].size() == 1);
		REQUIRE(json2[U"children"][0][U"children"][0][U"name"].get<String>() == U"TestNode");
	}

	SECTION("Error handling for missing required fields")
	{
		// 必須フィールドのテスト用構造体
		struct RequiredFieldTest
		{
			String fieldName;
			std::function<void(JSON&)> removeField;
		};
		
		Array<RequiredFieldTest> requiredFields =
		{
			{ U"version", [](JSON& j) { j.erase(U"version"); } },
			{ U"serializedVersion", [](JSON& j) { j.erase(U"serializedVersion"); } },
			{ U"referenceSize", [](JSON& j) { j.erase(U"referenceSize"); } },
			{ U"children", [](JSON& j) { j.erase(U"children"); } },
		};
		
		// 各必須フィールドの欠落をテスト
		for (const auto& test : requiredFields)
		{
			JSON invalidJson = JSON{};
			invalidJson[U"version"] = noco::NocoUIVersion;
			invalidJson[U"serializedVersion"] = noco::CurrentSerializedVersion;
			invalidJson[U"referenceSize"] = Array<double>{ 800.0, 600.0 };
			invalidJson[U"children"] = Array<JSON>{};
			
			// フィールドを削除
			test.removeField(invalidJson);
			
			auto canvas = noco::Canvas::CreateFromJSON(invalidJson);
			REQUIRE(canvas == nullptr);
		}
		
		// 有効なJSON（空のchildren配列）
		JSON validJson = JSON{};
		validJson[U"version"] = noco::NocoUIVersion;
		validJson[U"serializedVersion"] = noco::CurrentSerializedVersion;
		validJson[U"referenceSize"] = Array<double>{ 800.0, 600.0 };
		validJson[U"children"] = Array<JSON>{};
		
		auto canvas = noco::Canvas::CreateFromJSON(validJson);
		REQUIRE(canvas != nullptr);
		REQUIRE(canvas->children().size() == 0);
	}


	SECTION("Version fields handling")
	{
		auto canvas = noco::Canvas::Create();
		REQUIRE(canvas->serializedVersion() == noco::CurrentSerializedVersion);
		
		JSON json = canvas->toJSON();
		
		// versionとserializedVersionが正しく保存される
		REQUIRE(json[U"version"].get<String>() == noco::NocoUIVersion);
		REQUIRE(json[U"serializedVersion"].get<int32>() == noco::CurrentSerializedVersion);
		
		// カスタムserializedVersionでの読み込み
		json[U"serializedVersion"] = 1234567;
		auto loadedCanvas = noco::Canvas::CreateFromJSON(json);
		REQUIRE(loadedCanvas != nullptr);
		REQUIRE(loadedCanvas->serializedVersion() == 1234567);
		
		// 再保存時は常にCurrentSerializedVersionを使用
		JSON savedJson = loadedCanvas->toJSON();
		REQUIRE(savedJson[U"serializedVersion"].get<int32>() == noco::CurrentSerializedVersion);
		REQUIRE(loadedCanvas->serializedVersion() == 1234567);
	}
}
