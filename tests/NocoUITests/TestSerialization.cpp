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

TEST_CASE("Min/Max Size Constraints Serialization", "[Constraint][AnchorConstraint][BoxConstraint][JSON][Serialization]")
{
	SECTION("AnchorConstraint min/max serialization and deserialization")
	{
		// AnchorConstraintでmin/maxサイズを設定
		noco::AnchorConstraint originalConstraint;
		originalConstraint.anchorMin = Vec2{ 0.1, 0.2 };
		originalConstraint.anchorMax = Vec2{ 0.8, 0.9 };
		originalConstraint.posDelta = Vec2{ 10, 20 };
		originalConstraint.sizeDelta = Vec2{ 100, 150 };
		originalConstraint.sizeDeltaPivot = noco::Anchor::TopLeft;
		originalConstraint.minWidth = 80.0;
		originalConstraint.minHeight = 60.0;
		originalConstraint.maxWidth = 500.0;
		originalConstraint.maxHeight = 400.0;
		
		// JSONにシリアライズ
		JSON json = originalConstraint.toJSON();
		
		// JSONの内容確認
		REQUIRE(json[U"type"].getString() == U"AnchorConstraint");
		REQUIRE(json[U"anchorMin"] == originalConstraint.anchorMin);
		REQUIRE(json[U"anchorMax"] == originalConstraint.anchorMax);
		REQUIRE(json[U"posDelta"] == originalConstraint.posDelta);
		REQUIRE(json[U"sizeDelta"] == originalConstraint.sizeDelta);
		REQUIRE(json[U"sizeDeltaPivot"] == originalConstraint.sizeDeltaPivot);
		REQUIRE(json[U"minWidth"].get<double>() == 80.0);
		REQUIRE(json[U"minHeight"].get<double>() == 60.0);
		REQUIRE(json[U"maxWidth"].get<double>() == 500.0);
		REQUIRE(json[U"maxHeight"].get<double>() == 400.0);
		
		// JSONからデシリアライズ
		noco::AnchorConstraint deserializedConstraint = noco::AnchorConstraint::FromJSON(json);
		
		// デシリアライズ結果の確認
		REQUIRE(deserializedConstraint.anchorMin == originalConstraint.anchorMin);
		REQUIRE(deserializedConstraint.anchorMax == originalConstraint.anchorMax);
		REQUIRE(deserializedConstraint.posDelta == originalConstraint.posDelta);
		REQUIRE(deserializedConstraint.sizeDelta == originalConstraint.sizeDelta);
		REQUIRE(deserializedConstraint.sizeDeltaPivot == originalConstraint.sizeDeltaPivot);
		REQUIRE(deserializedConstraint.minWidth.has_value());
		REQUIRE(*deserializedConstraint.minWidth == 80.0);
		REQUIRE(deserializedConstraint.minHeight.has_value());
		REQUIRE(*deserializedConstraint.minHeight == 60.0);
		REQUIRE(deserializedConstraint.maxWidth.has_value());
		REQUIRE(*deserializedConstraint.maxWidth == 500.0);
		REQUIRE(deserializedConstraint.maxHeight.has_value());
		REQUIRE(*deserializedConstraint.maxHeight == 400.0);
	}

	SECTION("AnchorConstraint with partial min/max values")
	{
		// 一部のmin/maxのみ設定された場合
		noco::AnchorConstraint originalConstraint;
		originalConstraint.anchorMin = Vec2{ 0, 0 };
		originalConstraint.anchorMax = Vec2{ 1, 1 };
		originalConstraint.minWidth = 100.0;
		// minHeight, maxWidth, maxHeightは設定しない
		originalConstraint.maxHeight = 300.0;
		
		// JSONにシリアライズ
		JSON json = originalConstraint.toJSON();
		
		// 設定されたもののみJSONに含まれる
		REQUIRE(json.contains(U"minWidth"));
		REQUIRE(!json.contains(U"minHeight"));
		REQUIRE(!json.contains(U"maxWidth"));
		REQUIRE(json.contains(U"maxHeight"));
		REQUIRE(json[U"minWidth"].get<double>() == 100.0);
		REQUIRE(json[U"maxHeight"].get<double>() == 300.0);
		
		// JSONからデシリアライズ
		noco::AnchorConstraint deserializedConstraint = noco::AnchorConstraint::FromJSON(json);
		
		// デシリアライズ結果の確認
		REQUIRE(deserializedConstraint.minWidth.has_value());
		REQUIRE(*deserializedConstraint.minWidth == 100.0);
		REQUIRE(!deserializedConstraint.minHeight.has_value());
		REQUIRE(!deserializedConstraint.maxWidth.has_value());
		REQUIRE(deserializedConstraint.maxHeight.has_value());
		REQUIRE(*deserializedConstraint.maxHeight == 300.0);
	}

	SECTION("BoxConstraint min/max serialization and deserialization")
	{
		// BoxConstraintでmin/maxサイズを設定
		noco::BoxConstraint originalConstraint;
		originalConstraint.sizeRatio = Vec2{ 0.8, 0.6 };
		originalConstraint.sizeDelta = Vec2{ 50, 75 };
		originalConstraint.flexibleWeight = 1.5;
		originalConstraint.margin = noco::LRTB{ 5, 10, 15, 20 };
		originalConstraint.minWidth = 120.0;
		originalConstraint.minHeight = 90.0;
		originalConstraint.maxWidth = 600.0;
		originalConstraint.maxHeight = 450.0;
		
		// JSONにシリアライズ
		JSON json = originalConstraint.toJSON();
		
		// JSONの内容確認
		REQUIRE(json[U"type"].getString() == U"BoxConstraint");
		REQUIRE(json[U"sizeRatio"] == originalConstraint.sizeRatio);
		REQUIRE(json[U"sizeDelta"] == originalConstraint.sizeDelta);
		REQUIRE(json[U"flexibleWeight"].get<double>() == 1.5);
		REQUIRE(json[U"minWidth"].get<double>() == 120.0);
		REQUIRE(json[U"minHeight"].get<double>() == 90.0);
		REQUIRE(json[U"maxWidth"].get<double>() == 600.0);
		REQUIRE(json[U"maxHeight"].get<double>() == 450.0);
		
		// JSONからデシリアライズ
		noco::BoxConstraint deserializedConstraint = noco::BoxConstraint::FromJSON(json);
		
		// デシリアライズ結果の確認
		REQUIRE(deserializedConstraint.sizeRatio == originalConstraint.sizeRatio);
		REQUIRE(deserializedConstraint.sizeDelta == originalConstraint.sizeDelta);
		REQUIRE(deserializedConstraint.flexibleWeight == originalConstraint.flexibleWeight);
		REQUIRE(deserializedConstraint.margin.left == originalConstraint.margin.left);
		REQUIRE(deserializedConstraint.margin.right == originalConstraint.margin.right);
		REQUIRE(deserializedConstraint.margin.top == originalConstraint.margin.top);
		REQUIRE(deserializedConstraint.margin.bottom == originalConstraint.margin.bottom);
		REQUIRE(deserializedConstraint.minWidth.has_value());
		REQUIRE(*deserializedConstraint.minWidth == 120.0);
		REQUIRE(deserializedConstraint.minHeight.has_value());
		REQUIRE(*deserializedConstraint.minHeight == 90.0);
		REQUIRE(deserializedConstraint.maxWidth.has_value());
		REQUIRE(*deserializedConstraint.maxWidth == 600.0);
		REQUIRE(deserializedConstraint.maxHeight.has_value());
		REQUIRE(*deserializedConstraint.maxHeight == 450.0);
	}

	SECTION("BoxConstraint with no min/max values")
	{
		// min/maxが設定されていない場合
		noco::BoxConstraint originalConstraint;
		originalConstraint.sizeRatio = Vec2{ 1.0, 1.0 };
		originalConstraint.sizeDelta = Vec2{ 200, 150 };
		originalConstraint.flexibleWeight = 2.0;
		// minWidth, minHeight, maxWidth, maxHeightは設定しない
		
		// JSONにシリアライズ
		JSON json = originalConstraint.toJSON();
		
		// min/maxプロパティはJSONに含まれない
		REQUIRE(!json.contains(U"minWidth"));
		REQUIRE(!json.contains(U"minHeight"));
		REQUIRE(!json.contains(U"maxWidth"));
		REQUIRE(!json.contains(U"maxHeight"));
		
		// JSONからデシリアライズ
		noco::BoxConstraint deserializedConstraint = noco::BoxConstraint::FromJSON(json);
		
		// デシリアライズ結果の確認（min/maxは無効状態）
		REQUIRE(!deserializedConstraint.minWidth.has_value());
		REQUIRE(!deserializedConstraint.minHeight.has_value());
		REQUIRE(!deserializedConstraint.maxWidth.has_value());
		REQUIRE(!deserializedConstraint.maxHeight.has_value());
		REQUIRE(deserializedConstraint.sizeRatio == originalConstraint.sizeRatio);
		REQUIRE(deserializedConstraint.sizeDelta == originalConstraint.sizeDelta);
		REQUIRE(deserializedConstraint.flexibleWeight == originalConstraint.flexibleWeight);
	}

	SECTION("Node with min/max constraints serialization")
	{
		// ノード全体でのシリアライゼーションテスト
		auto originalNode = noco::Node::Create(U"TestNodeWithConstraints");
		
		// AnchorConstraintを設定
		noco::AnchorConstraint anchorConstraint;
		anchorConstraint.anchorMin = Vec2{ 0, 0 };
		anchorConstraint.anchorMax = Vec2{ 1, 0 };
		anchorConstraint.sizeDelta = Vec2{ 0, 100 };
		anchorConstraint.minWidth = 200.0;
		anchorConstraint.maxWidth = 800.0;
		originalNode->setConstraint(anchorConstraint);
		
		// 子ノードを追加
		auto childNode = noco::Node::Create(U"ChildNode");
		noco::BoxConstraint boxConstraint;
		boxConstraint.sizeRatio = Vec2{ 0.5, 1.0 };
		boxConstraint.minHeight = 50.0;
		boxConstraint.maxHeight = 200.0;
		childNode->setConstraint(boxConstraint);
		originalNode->addChild(childNode);
		
		// JSONにシリアライズ
		JSON json = originalNode->toJSON();
		
		// JSONからノードを復元
		auto deserializedNode = noco::Node::CreateFromJSON(json);
		
		// ノード名の確認
		REQUIRE(deserializedNode->name() == U"TestNodeWithConstraints");
		
		// 親ノードの制約確認
		const auto* parentAnchorConstraint = deserializedNode->anchorConstraint();
		REQUIRE(parentAnchorConstraint != nullptr);
		REQUIRE(parentAnchorConstraint->anchorMin == Vec2{ 0, 0 });
		REQUIRE(parentAnchorConstraint->anchorMax == Vec2{ 1, 0 });
		REQUIRE(parentAnchorConstraint->sizeDelta == Vec2{ 0, 100 });
		REQUIRE(parentAnchorConstraint->minWidth.has_value());
		REQUIRE(*parentAnchorConstraint->minWidth == 200.0);
		REQUIRE(parentAnchorConstraint->maxWidth.has_value());
		REQUIRE(*parentAnchorConstraint->maxWidth == 800.0);
		REQUIRE(!parentAnchorConstraint->minHeight.has_value());
		REQUIRE(!parentAnchorConstraint->maxHeight.has_value());
		
		// 子ノードの確認
		REQUIRE(deserializedNode->children().size() == 1);
		const auto& restoredChild = deserializedNode->children()[0];
		REQUIRE(restoredChild->name() == U"ChildNode");
		
		// 子ノードの制約確認
		const auto* childBoxConstraint = restoredChild->boxConstraint();
		REQUIRE(childBoxConstraint != nullptr);
		REQUIRE(childBoxConstraint->sizeRatio == Vec2{ 0.5, 1.0 });
		REQUIRE(!childBoxConstraint->minWidth.has_value());
		REQUIRE(childBoxConstraint->minHeight.has_value());
		REQUIRE(*childBoxConstraint->minHeight == 50.0);
		REQUIRE(!childBoxConstraint->maxWidth.has_value());
		REQUIRE(childBoxConstraint->maxHeight.has_value());
		REQUIRE(*childBoxConstraint->maxHeight == 200.0);
	}
}