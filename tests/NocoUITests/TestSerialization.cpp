# include <catch2/catch.hpp>
# include <Siv3D.hpp>
# include <NocoUI.hpp>

// ========================================
// シリアライゼーションのテスト
// ========================================

TEST_CASE("Serialization", "[Node][Canvas][JSON]")
{
	SECTION("Node to JSON")
	{
		auto node = noco::Node::Create(U"TestNode");
		node->transformEffect().setPosition(Vec2{ 100, 200 });
		node->transformEffect().setScale(Vec2{ 2, 2 });
		
		// JSONに変換
		JSON json = node->toJSON();
		
		// JSON内容の確認
		REQUIRE(json[U"name"].getString() == U"TestNode");
	}
}

TEST_CASE("Min/Max Size Regions Serialization", "[Region][AnchorRegion][InlineRegion][JSON][Serialization]")
{
	SECTION("AnchorRegion min/max serialization and deserialization")
	{
		// AnchorRegionでmin/maxサイズを設定
		noco::AnchorRegion originalRegion;
		originalRegion.anchorMin = Vec2{ 0.1, 0.2 };
		originalRegion.anchorMax = Vec2{ 0.8, 0.9 };
		originalRegion.posDelta = Vec2{ 10, 20 };
		originalRegion.sizeDelta = Vec2{ 100, 150 };
		originalRegion.sizeDeltaPivot = noco::Anchor::TopLeft;
		originalRegion.minWidth = 80.0;
		originalRegion.minHeight = 60.0;
		originalRegion.maxWidth = 500.0;
		originalRegion.maxHeight = 400.0;
		
		// JSONにシリアライズ
		JSON json = originalRegion.toJSON();
		
		// JSONの内容確認
		REQUIRE(json[U"type"].getString() == U"AnchorRegion");
		REQUIRE(json[U"anchorMin"] == originalRegion.anchorMin);
		REQUIRE(json[U"anchorMax"] == originalRegion.anchorMax);
		REQUIRE(json[U"posDelta"] == originalRegion.posDelta);
		REQUIRE(json[U"sizeDelta"] == originalRegion.sizeDelta);
		REQUIRE(json[U"sizeDeltaPivot"] == originalRegion.sizeDeltaPivot);
		REQUIRE(json[U"minWidth"].get<double>() == 80.0);
		REQUIRE(json[U"minHeight"].get<double>() == 60.0);
		REQUIRE(json[U"maxWidth"].get<double>() == 500.0);
		REQUIRE(json[U"maxHeight"].get<double>() == 400.0);
		
		// JSONからデシリアライズ
		noco::AnchorRegion deserializedRegion = noco::AnchorRegion::FromJSON(json);
		
		// デシリアライズ結果の確認
		REQUIRE(deserializedRegion.anchorMin == originalRegion.anchorMin);
		REQUIRE(deserializedRegion.anchorMax == originalRegion.anchorMax);
		REQUIRE(deserializedRegion.posDelta == originalRegion.posDelta);
		REQUIRE(deserializedRegion.sizeDelta == originalRegion.sizeDelta);
		REQUIRE(deserializedRegion.sizeDeltaPivot == originalRegion.sizeDeltaPivot);
		REQUIRE(deserializedRegion.minWidth.has_value());
		REQUIRE(*deserializedRegion.minWidth == 80.0);
		REQUIRE(deserializedRegion.minHeight.has_value());
		REQUIRE(*deserializedRegion.minHeight == 60.0);
		REQUIRE(deserializedRegion.maxWidth.has_value());
		REQUIRE(*deserializedRegion.maxWidth == 500.0);
		REQUIRE(deserializedRegion.maxHeight.has_value());
		REQUIRE(*deserializedRegion.maxHeight == 400.0);
	}

	SECTION("AnchorRegion with partial min/max values")
	{
		// 一部のmin/maxのみ設定された場合
		noco::AnchorRegion originalRegion;
		originalRegion.anchorMin = Vec2{ 0, 0 };
		originalRegion.anchorMax = Vec2{ 1, 1 };
		originalRegion.minWidth = 100.0;
		// minHeight, maxWidth, maxHeightは設定しない
		originalRegion.maxHeight = 300.0;
		
		// JSONにシリアライズ
		JSON json = originalRegion.toJSON();
		
		// 設定されたもののみJSONに含まれる
		REQUIRE(json.contains(U"minWidth"));
		REQUIRE(!json.contains(U"minHeight"));
		REQUIRE(!json.contains(U"maxWidth"));
		REQUIRE(json.contains(U"maxHeight"));
		REQUIRE(json[U"minWidth"].get<double>() == 100.0);
		REQUIRE(json[U"maxHeight"].get<double>() == 300.0);
		
		// JSONからデシリアライズ
		noco::AnchorRegion deserializedRegion = noco::AnchorRegion::FromJSON(json);
		
		// デシリアライズ結果の確認
		REQUIRE(deserializedRegion.minWidth.has_value());
		REQUIRE(*deserializedRegion.minWidth == 100.0);
		REQUIRE(!deserializedRegion.minHeight.has_value());
		REQUIRE(!deserializedRegion.maxWidth.has_value());
		REQUIRE(deserializedRegion.maxHeight.has_value());
		REQUIRE(*deserializedRegion.maxHeight == 300.0);
	}

	SECTION("InlineRegion min/max serialization and deserialization")
	{
		// InlineRegionでmin/maxサイズを設定
		noco::InlineRegion originalRegion;
		originalRegion.sizeRatio = Vec2{ 0.8, 0.6 };
		originalRegion.sizeDelta = Vec2{ 50, 75 };
		originalRegion.flexibleWeight = 1.5;
		originalRegion.margin = noco::LRTB{ 5, 10, 15, 20 };
		originalRegion.minWidth = 120.0;
		originalRegion.minHeight = 90.0;
		originalRegion.maxWidth = 600.0;
		originalRegion.maxHeight = 450.0;
		
		// JSONにシリアライズ
		JSON json = originalRegion.toJSON();
		
		// JSONの内容確認
		REQUIRE(json[U"type"].getString() == U"InlineRegion");
		REQUIRE(json[U"sizeRatio"] == originalRegion.sizeRatio);
		REQUIRE(json[U"sizeDelta"] == originalRegion.sizeDelta);
		REQUIRE(json[U"flexibleWeight"].get<double>() == 1.5);
		REQUIRE(json[U"minWidth"].get<double>() == 120.0);
		REQUIRE(json[U"minHeight"].get<double>() == 90.0);
		REQUIRE(json[U"maxWidth"].get<double>() == 600.0);
		REQUIRE(json[U"maxHeight"].get<double>() == 450.0);
		
		// marginのJSONを確認
		REQUIRE(json.contains(U"margin"));
		const JSON& marginJson = json[U"margin"];
		// LRTBは文字列として保存される
		REQUIRE(marginJson.isString());
		REQUIRE(marginJson.getString() == U"(5, 10, 15, 20)");
		
		// JSONからデシリアライズ
		noco::InlineRegion deserializedRegion = noco::InlineRegion::FromJSON(json);
		
		// デシリアライズ結果の確認
		REQUIRE(deserializedRegion.sizeRatio == originalRegion.sizeRatio);
		REQUIRE(deserializedRegion.sizeDelta == originalRegion.sizeDelta);
		REQUIRE(deserializedRegion.flexibleWeight == originalRegion.flexibleWeight);
		REQUIRE(deserializedRegion.margin.left == originalRegion.margin.left);
		REQUIRE(deserializedRegion.margin.right == originalRegion.margin.right);
		REQUIRE(deserializedRegion.margin.top == originalRegion.margin.top);
		REQUIRE(deserializedRegion.margin.bottom == originalRegion.margin.bottom);
		REQUIRE(deserializedRegion.minWidth.has_value());
		REQUIRE(*deserializedRegion.minWidth == 120.0);
		REQUIRE(deserializedRegion.minHeight.has_value());
		REQUIRE(*deserializedRegion.minHeight == 90.0);
		REQUIRE(deserializedRegion.maxWidth.has_value());
		REQUIRE(*deserializedRegion.maxWidth == 600.0);
		REQUIRE(deserializedRegion.maxHeight.has_value());
		REQUIRE(*deserializedRegion.maxHeight == 450.0);
	}

	SECTION("InlineRegion with no min/max values")
	{
		// min/maxが設定されていない場合
		noco::InlineRegion originalRegion;
		originalRegion.sizeRatio = Vec2{ 1.0, 1.0 };
		originalRegion.sizeDelta = Vec2{ 200, 150 };
		originalRegion.flexibleWeight = 2.0;
		// minWidth, minHeight, maxWidth, maxHeightは設定しない
		
		// JSONにシリアライズ
		JSON json = originalRegion.toJSON();
		
		// min/maxプロパティはJSONに含まれない
		REQUIRE(!json.contains(U"minWidth"));
		REQUIRE(!json.contains(U"minHeight"));
		REQUIRE(!json.contains(U"maxWidth"));
		REQUIRE(!json.contains(U"maxHeight"));
		
		// JSONからデシリアライズ
		noco::InlineRegion deserializedRegion = noco::InlineRegion::FromJSON(json);
		
		// デシリアライズ結果の確認（min/maxは無効状態）
		REQUIRE(!deserializedRegion.minWidth.has_value());
		REQUIRE(!deserializedRegion.minHeight.has_value());
		REQUIRE(!deserializedRegion.maxWidth.has_value());
		REQUIRE(!deserializedRegion.maxHeight.has_value());
		REQUIRE(deserializedRegion.sizeRatio == originalRegion.sizeRatio);
		REQUIRE(deserializedRegion.sizeDelta == originalRegion.sizeDelta);
		REQUIRE(deserializedRegion.flexibleWeight == originalRegion.flexibleWeight);
	}

	SECTION("Node with min/max regions serialization")
	{
		// ノード全体でのシリアライゼーションテスト
		auto originalNode = noco::Node::Create(U"TestNodeWithRegions");
		
		// AnchorRegionを設定
		noco::AnchorRegion anchorRegion;
		anchorRegion.anchorMin = Vec2{ 0, 0 };
		anchorRegion.anchorMax = Vec2{ 1, 0 };
		anchorRegion.sizeDelta = Vec2{ 0, 100 };
		anchorRegion.minWidth = 200.0;
		anchorRegion.maxWidth = 800.0;
		originalNode->setRegion(anchorRegion);
		
		// 子ノードを追加
		auto childNode = noco::Node::Create(U"ChildNode");
		noco::InlineRegion inlineRegion;
		inlineRegion.sizeRatio = Vec2{ 0.5, 1.0 };
		inlineRegion.minHeight = 50.0;
		inlineRegion.maxHeight = 200.0;
		childNode->setRegion(inlineRegion);
		originalNode->addChild(childNode);
		
		// JSONにシリアライズ
		JSON json = originalNode->toJSON();
		
		// JSONからノードを復元
		auto deserializedNode = noco::Node::CreateFromJSON(json);
		
		// ノード名の確認
		REQUIRE(deserializedNode->name() == U"TestNodeWithRegions");
		
		// 親ノードの制約確認
		const auto* parentAnchorRegion = deserializedNode->anchorRegion();
		REQUIRE(parentAnchorRegion != nullptr);
		REQUIRE(parentAnchorRegion->anchorMin == Vec2{ 0, 0 });
		REQUIRE(parentAnchorRegion->anchorMax == Vec2{ 1, 0 });
		REQUIRE(parentAnchorRegion->sizeDelta == Vec2{ 0, 100 });
		REQUIRE(parentAnchorRegion->minWidth.has_value());
		REQUIRE(*parentAnchorRegion->minWidth == 200.0);
		REQUIRE(parentAnchorRegion->maxWidth.has_value());
		REQUIRE(*parentAnchorRegion->maxWidth == 800.0);
		REQUIRE(!parentAnchorRegion->minHeight.has_value());
		REQUIRE(!parentAnchorRegion->maxHeight.has_value());
		
		// 子ノードの確認
		REQUIRE(deserializedNode->children().size() == 1);
		const auto& restoredChild = deserializedNode->children()[0];
		REQUIRE(restoredChild->name() == U"ChildNode");
		
		// 子ノードの制約確認
		const auto* childInlineRegion = restoredChild->inlineRegion();
		REQUIRE(childInlineRegion != nullptr);
		REQUIRE(childInlineRegion->sizeRatio == Vec2{ 0.5, 1.0 });
		REQUIRE(!childInlineRegion->minWidth.has_value());
		REQUIRE(childInlineRegion->minHeight.has_value());
		REQUIRE(*childInlineRegion->minHeight == 50.0);
		REQUIRE(!childInlineRegion->maxWidth.has_value());
		REQUIRE(childInlineRegion->maxHeight.has_value());
		REQUIRE(*childInlineRegion->maxHeight == 200.0);
	}
}

TEST_CASE("LRTB Serialization", "[LRTB][JSON][Serialization]")
{
	SECTION("Basic LRTB serialization and deserialization")
	{
		// 基本的なLRTBの値
		noco::LRTB original{ 10.5, 20.5, 30.5, 40.5 };
		
		// JSONにシリアライズ
		JSON json = original.toJSON();
		
		// JSONの内容確認（文字列として保存される）
		REQUIRE(json.isString());
		REQUIRE(json.getString() == U"(10.5, 20.5, 30.5, 40.5)");
		
		// JSONからデシリアライズ
		noco::LRTB deserialized = noco::LRTB::fromJSON(json);
		
		// デシリアライズ結果の確認
		REQUIRE(deserialized.left == 10.5);
		REQUIRE(deserialized.right == 20.5);
		REQUIRE(deserialized.top == 30.5);
		REQUIRE(deserialized.bottom == 40.5);
		REQUIRE(deserialized == original);
	}
	
	SECTION("LRTB Zero values")
	{
		// ゼロ値のLRTB
		noco::LRTB original = noco::LRTB::Zero();
		
		// JSONにシリアライズ
		JSON json = original.toJSON();
		
		// JSONの内容確認（文字列として保存される）
		REQUIRE(json.isString());
		REQUIRE(json.getString() == U"(0, 0, 0, 0)");
		
		// JSONからデシリアライズ
		noco::LRTB deserialized = noco::LRTB::fromJSON(json);
		
		// デシリアライズ結果の確認
		REQUIRE(deserialized == noco::LRTB::Zero());
	}
	
	SECTION("LRTB with negative values")
	{
		// 負の値を含むLRTB
		noco::LRTB original{ -10, -20, -30, -40 };
		
		// JSONにシリアライズ
		JSON json = original.toJSON();
		
		// JSONの内容確認（文字列として保存される）
		REQUIRE(json.isString());
		REQUIRE(json.getString() == U"(-10, -20, -30, -40)");
		
		// JSONからデシリアライズ
		noco::LRTB deserialized = noco::LRTB::fromJSON(json);
		
		// デシリアライズ結果の確認
		REQUIRE(deserialized.left == -10);
		REQUIRE(deserialized.right == -20);
		REQUIRE(deserialized.top == -30);
		REQUIRE(deserialized.bottom == -40);
	}
	
	SECTION("LRTB with invalid JSON uses default values")
	{
		// 文字列形式でないJSONからのデシリアライゼーション
		JSON invalidJson;
		invalidJson[U"someField"] = 42;
		
		// デフォルト値を指定してデシリアライズ
		noco::LRTB defaultValue{ 1, 2, 3, 4 };
		noco::LRTB deserialized = noco::LRTB::fromJSON(invalidJson, defaultValue);
		
		// 文字列でないJSONの場合、デフォルト値が使用される
		REQUIRE(deserialized == defaultValue);
		REQUIRE(deserialized.left == 1.0);
		REQUIRE(deserialized.right == 2.0);
		REQUIRE(deserialized.top == 3.0);
		REQUIRE(deserialized.bottom == 4.0);
	}
	
	SECTION("LRTB with malformed string format uses default values")
	{
		// 不正な文字列形式のJSON
		JSON malformedJson = JSON(U"not a valid LRTB format");
		
		// デフォルト値を指定してデシリアライズ
		noco::LRTB defaultValue{ 10, 20, 30, 40 };
		noco::LRTB deserialized = noco::LRTB::fromJSON(malformedJson, defaultValue);
		
		// パースできない文字列の場合、デフォルト値が使用される
		REQUIRE(deserialized == defaultValue);
		REQUIRE(deserialized.left == 10.0);
		REQUIRE(deserialized.right == 20.0);
		REQUIRE(deserialized.top == 30.0);
		REQUIRE(deserialized.bottom == 40.0);
	}
	
	SECTION("LRTB in InlineRegion with GetFromJSONOr")
	{
		// InlineRegionのJSONでLRTBのテスト
		JSON regionJson;
		regionJson[U"type"] = U"InlineRegion";
		regionJson[U"sizeRatio"] = Vec2{ 1.0, 1.0 };
		regionJson[U"sizeDelta"] = Vec2{ 100, 100 };
		regionJson[U"flexibleWeight"] = 0.0;
		
		// marginフィールドが文字列として存在する場合
		regionJson[U"margin"] = U"(5, 10, 15, 20)";
		
		noco::InlineRegion region = noco::InlineRegion::FromJSON(regionJson);
		REQUIRE(region.margin.left == 5.0);
		REQUIRE(region.margin.right == 10.0);
		REQUIRE(region.margin.top == 15.0);
		REQUIRE(region.margin.bottom == 20.0);
	}
	
	SECTION("LRTB in InlineRegion with missing margin field")
	{
		// marginフィールドが欠落している場合
		JSON regionJson;
		regionJson[U"type"] = U"InlineRegion";
		regionJson[U"sizeRatio"] = Vec2{ 0.5, 0.5 };
		regionJson[U"sizeDelta"] = Vec2{ 50, 50 };
		regionJson[U"flexibleWeight"] = 1.0;
		// marginフィールドなし
		
		noco::InlineRegion region = noco::InlineRegion::FromJSON(regionJson);
		
		// デフォルト値(Zero)が使用される
		REQUIRE(region.margin == noco::LRTB::Zero());
		REQUIRE(region.margin.left == 0.0);
		REQUIRE(region.margin.right == 0.0);
		REQUIRE(region.margin.top == 0.0);
		REQUIRE(region.margin.bottom == 0.0);
	}
	
	SECTION("LRTB in InlineRegion with invalid margin format")
	{
		// marginフィールドに不正な文字列が含まれる場合
		JSON regionJson;
		regionJson[U"type"] = U"InlineRegion";
		regionJson[U"sizeRatio"] = Vec2{ 0.8, 0.8 };
		regionJson[U"sizeDelta"] = Vec2{ 80, 80 };
		regionJson[U"flexibleWeight"] = 0.5;
		
		// 不正な形式の文字列
		regionJson[U"margin"] = U"invalid margin format";
		
		noco::InlineRegion region = noco::InlineRegion::FromJSON(regionJson);
		
		// パースできない文字列の場合、デフォルト値(Zero)が使用される
		REQUIRE(region.margin == noco::LRTB::Zero());
		REQUIRE(region.margin.left == 0.0);
		REQUIRE(region.margin.right == 0.0);
		REQUIRE(region.margin.top == 0.0);
		REQUIRE(region.margin.bottom == 0.0);
	}
}