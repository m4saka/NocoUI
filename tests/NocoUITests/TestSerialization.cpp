#include <catch2/catch.hpp>
#include <Siv3D.hpp>
#include <NocoUI.hpp>

// ========================================
// シリアライズのテスト
// ========================================

TEST_CASE("Serialization", "[Node][Canvas][JSON]")
{
	SECTION("Node to JSON and back")
	{
		auto node = noco::Node::Create(U"TestNode");
		node->transform().setTranslate(Vec2{ 100, 200 });
		node->transform().setScale(Vec2{ 2, 2 });
		node->transform().setRotation(45);
		node->setRegion(noco::InlineRegion
		{
			.sizeDelta = Vec2{ 300, 150 },
			.flexibleWeight = 1.5,
		});
		
		// コンポーネントを追加
		auto label = node->emplaceComponent<noco::Label>();
		label->setText(U"Test Label");
		
		// JSONに変換
		JSON json = node->toJSON();
		
		// JSON内容の確認
		REQUIRE(json[U"name"].getString() == U"TestNode");
		REQUIRE(json.contains(U"transform"));
		REQUIRE(json.contains(U"region"));
		REQUIRE(json.contains(U"components"));
		REQUIRE(json[U"components"].isArray());
		REQUIRE(json[U"components"].size() == 1);
		
		// JSONからノードを再構築
		auto restoredNode = noco::Node::CreateFromJSON(json);
		REQUIRE(restoredNode != nullptr);
		REQUIRE(restoredNode->name() == U"TestNode");
		REQUIRE(restoredNode->transform().translate().value() == Vec2{ 100, 200 });
		REQUIRE(restoredNode->transform().scale().value() == Vec2{ 2, 2 });
		REQUIRE(restoredNode->transform().rotation().value() == Approx(45));
		
		// Regionの確認
		auto* inlineRegion = restoredNode->inlineRegion();
		REQUIRE(inlineRegion != nullptr);
		REQUIRE(inlineRegion->sizeDelta == Vec2{ 300, 150 });
		REQUIRE(inlineRegion->flexibleWeight == Approx(1.5));
		
		// コンポーネントの確認
		auto restoredLabel = restoredNode->getComponentOrNull<noco::Label>();
		REQUIRE(restoredLabel != nullptr);
		REQUIRE(restoredLabel->text().defaultValue() == U"Test Label");
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
		// Vec2は配列フォーマット [x, y] でシリアライズされる
		REQUIRE(json[U"anchorMin"].isArray());
		REQUIRE(json[U"anchorMin"].size() == 2);
		REQUIRE(json[U"anchorMin"][0].get<double>() == Approx(originalRegion.anchorMin.x));
		REQUIRE(json[U"anchorMin"][1].get<double>() == Approx(originalRegion.anchorMin.y));

		REQUIRE(json[U"anchorMax"].isArray());
		REQUIRE(json[U"anchorMax"].size() == 2);
		REQUIRE(json[U"anchorMax"][0].get<double>() == Approx(originalRegion.anchorMax.x));
		REQUIRE(json[U"anchorMax"][1].get<double>() == Approx(originalRegion.anchorMax.y));

		REQUIRE(json[U"posDelta"].isArray());
		REQUIRE(json[U"posDelta"].size() == 2);
		REQUIRE(json[U"posDelta"][0].get<double>() == Approx(originalRegion.posDelta.x));
		REQUIRE(json[U"posDelta"][1].get<double>() == Approx(originalRegion.posDelta.y));

		REQUIRE(json[U"sizeDelta"].isArray());
		REQUIRE(json[U"sizeDelta"].size() == 2);
		REQUIRE(json[U"sizeDelta"][0].get<double>() == Approx(originalRegion.sizeDelta.x));
		REQUIRE(json[U"sizeDelta"][1].get<double>() == Approx(originalRegion.sizeDelta.y));

		REQUIRE(json[U"sizeDeltaPivot"].isArray());
		REQUIRE(json[U"sizeDeltaPivot"].size() == 2);
		REQUIRE(json[U"sizeDeltaPivot"][0].get<double>() == Approx(originalRegion.sizeDeltaPivot.x));
		REQUIRE(json[U"sizeDeltaPivot"][1].get<double>() == Approx(originalRegion.sizeDeltaPivot.y));
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
		// Vec2は配列フォーマット [x, y] でシリアライズされる
		REQUIRE(json[U"sizeRatio"].isArray());
		REQUIRE(json[U"sizeRatio"].size() == 2);
		REQUIRE(json[U"sizeRatio"][0].get<double>() == Approx(originalRegion.sizeRatio.x));
		REQUIRE(json[U"sizeRatio"][1].get<double>() == Approx(originalRegion.sizeRatio.y));

		REQUIRE(json[U"sizeDelta"].isArray());
		REQUIRE(json[U"sizeDelta"].size() == 2);
		REQUIRE(json[U"sizeDelta"][0].get<double>() == Approx(originalRegion.sizeDelta.x));
		REQUIRE(json[U"sizeDelta"][1].get<double>() == Approx(originalRegion.sizeDelta.y));
		REQUIRE(json[U"flexibleWeight"].get<double>() == 1.5);
		REQUIRE(json[U"minWidth"].get<double>() == 120.0);
		REQUIRE(json[U"minHeight"].get<double>() == 90.0);
		REQUIRE(json[U"maxWidth"].get<double>() == 600.0);
		REQUIRE(json[U"maxHeight"].get<double>() == 450.0);
		
		// marginのJSONを確認
		REQUIRE(json.contains(U"margin"));
		const JSON& marginJson = json[U"margin"];
		// LRTBは配列フォーマット [left, right, top, bottom] として保存される
		REQUIRE(marginJson.isArray());
		REQUIRE(marginJson.size() == 4);
		REQUIRE(marginJson[0].get<double>() == Approx(5.0));
		REQUIRE(marginJson[1].get<double>() == Approx(10.0));
		REQUIRE(marginJson[2].get<double>() == Approx(15.0));
		REQUIRE(marginJson[3].get<double>() == Approx(20.0));
		
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
		// 通常の値
		noco::LRTB original{ 10.5, 20.5, 30.5, 40.5 };
		JSON json = original.toJSON();
		// LRTBは配列フォーマット [left, right, top, bottom] でシリアライズされる
		REQUIRE(json.isArray());
		REQUIRE(json.size() == 4);
		REQUIRE(json[0].get<double>() == Approx(10.5));
		REQUIRE(json[1].get<double>() == Approx(20.5));
		REQUIRE(json[2].get<double>() == Approx(30.5));
		REQUIRE(json[3].get<double>() == Approx(40.5));
		noco::LRTB deserialized = noco::LRTB::fromJSON(json);
		REQUIRE(deserialized == original);
		
		// ゼロ値
		noco::LRTB zero = noco::LRTB::Zero();
		JSON zeroJson = zero.toJSON();
		// LRTBは配列フォーマット [left, right, top, bottom] でシリアライズされる
		REQUIRE(zeroJson.isArray());
		REQUIRE(zeroJson.size() == 4);
		REQUIRE(zeroJson[0].get<double>() == Approx(0.0));
		REQUIRE(zeroJson[1].get<double>() == Approx(0.0));
		REQUIRE(zeroJson[2].get<double>() == Approx(0.0));
		REQUIRE(zeroJson[3].get<double>() == Approx(0.0));
		REQUIRE(noco::LRTB::fromJSON(zeroJson) == zero);
		
		// 負の値
		noco::LRTB negative{ -10, -20, -30, -40 };
		JSON negJson = negative.toJSON();
		REQUIRE(negJson.isArray());
		REQUIRE(negJson.size() == 4);
		REQUIRE(negJson[0].getOr<double>(0.0) == -10.0);
		REQUIRE(negJson[1].getOr<double>(0.0) == -20.0);
		REQUIRE(negJson[2].getOr<double>(0.0) == -30.0);
		REQUIRE(negJson[3].getOr<double>(0.0) == -40.0);
		REQUIRE(noco::LRTB::fromJSON(negJson) == negative);
	}
	
	SECTION("LRTB error handling with default values")
	{
		// 不正なJSON形式
		JSON invalidJson;
		invalidJson[U"someField"] = 42;
		noco::LRTB defaultValue{ 1, 2, 3, 4 };
		noco::LRTB result = noco::LRTB::fromJSON(invalidJson, defaultValue);
		REQUIRE(result == defaultValue);
		
		// 不正な文字列形式
		JSON malformedJson = JSON(U"not a valid LRTB format");
		result = noco::LRTB::fromJSON(malformedJson, defaultValue);
		REQUIRE(result == defaultValue);
	}
	
	SECTION("LRTB in InlineRegion")
	{
		JSON regionJson;
		regionJson[U"type"] = U"InlineRegion";
		regionJson[U"sizeRatio"] = Vec2{ 1.0, 1.0 };
		regionJson[U"sizeDelta"] = Vec2{ 100, 100 };
		regionJson[U"flexibleWeight"] = 0.0;
		
		// marginあり
		regionJson[U"margin"] = Array<double>{ 5.0, 10.0, 15.0, 20.0 };
		noco::InlineRegion withMargin = noco::InlineRegion::FromJSON(regionJson);
		REQUIRE(withMargin.margin == noco::LRTB{ 5, 10, 15, 20 });
		
		// marginなし
		regionJson.erase(U"margin");
		noco::InlineRegion noMargin = noco::InlineRegion::FromJSON(regionJson);
		REQUIRE(noMargin.margin == noco::LRTB::Zero());
		
		// 不正なmargin
		regionJson[U"margin"] = U"invalid";
		noco::InlineRegion invalidMargin = noco::InlineRegion::FromJSON(regionJson);
		REQUIRE(invalidMargin.margin == noco::LRTB::Zero());
	}
}
