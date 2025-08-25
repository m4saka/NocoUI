#include <catch2/catch.hpp>
#include <Siv3D.hpp>
#include <NocoUI.hpp>

// ========================================
// Regionのテスト
// ========================================

TEST_CASE("Region system", "[Region]")
{
	SECTION("InlineRegion")
	{
		auto node = noco::Node::Create();
		noco::InlineRegion region;
		region.sizeDelta = Vec2{ 100.0f, 50.0f };
		
		node->setRegion(region);
		
		// Regionが正しく設定されているか確認
		auto* inlineRegion = node->inlineRegion();
		REQUIRE(inlineRegion != nullptr);
		REQUIRE(inlineRegion->sizeDelta.x == 100.0f);
		REQUIRE(inlineRegion->sizeDelta.y == 50.0f);
	}

	SECTION("AnchorRegion")
	{
		auto node = noco::Node::Create();
		noco::AnchorRegion region;
		region.anchorMin = Vec2{ 0.0f, 0.0f };
		region.anchorMax = Vec2{ 1.0f, 1.0f };
		
		node->setRegion(region);
		
		// Regionが正しく設定されているか確認
		auto* anchorRegion = std::get_if<noco::AnchorRegion>(&node->region());
		REQUIRE(anchorRegion != nullptr);
		REQUIRE(anchorRegion->anchorMin == Vec2{ 0.0f, 0.0f });
		REQUIRE(anchorRegion->anchorMax == Vec2{ 1.0f, 1.0f });
	}

	SECTION("AnchorRegion with max size")
	{
		// 最大サイズの計算をテスト
		noco::AnchorRegion region;
		region.anchorMin = Vec2{ 0.0f, 0.0f };
		region.anchorMax = Vec2{ 1.0f, 1.0f };
		region.sizeDelta = Vec2{ 0.0f, 0.0f };
		region.maxWidth = 500.0;
		region.maxHeight = 400.0;
		
		// 親の領域が大きい場合、最大サイズが適用される
		RectF parentRect{ 0, 0, 1000, 800 };
		RectF result = region.applyRegion(parentRect, Vec2::Zero());
		REQUIRE(result.w == 500.0f);
		REQUIRE(result.h == 400.0f);
		
		// 親の領域が小さい場合、親のサイズに従う
		RectF smallParentRect{ 0, 0, 300, 200 };
		RectF smallResult = region.applyRegion(smallParentRect, Vec2::Zero());
		REQUIRE(smallResult.w == 300.0f);
		REQUIRE(smallResult.h == 200.0f);
	}

	SECTION("AnchorRegion with max size and centered pivot")
	{
		// 中央配置のピボットで最大サイズをテスト
		noco::AnchorRegion region;
		region.anchorMin = Vec2{ 0.0f, 0.0f };
		region.anchorMax = Vec2{ 1.0f, 1.0f };
		region.sizeDelta = Vec2{ 0.0f, 0.0f };
		region.sizeDeltaPivot = noco::Anchor::MiddleCenter;
		region.maxWidth = 500.0;
		region.maxHeight = 400.0;
		
		// 親の領域が大きい場合、最大サイズが適用され、中央に配置される
		RectF parentRect{ 0, 0, 1000, 800 };
		RectF result = region.applyRegion(parentRect, Vec2::Zero());
		REQUIRE(result.w == 500.0f);
		REQUIRE(result.h == 400.0f);
		// 中央配置の確認（親の中心と結果の中心が一致）
		REQUIRE(result.center() == parentRect.center());
	}
}

TEST_CASE("InlineRegion detailed", "[Region][InlineRegion]")
{
	SECTION("Size ratio and delta")
	{
		auto node = noco::Node::Create();
		noco::InlineRegion region;
		
		// サイズ比率の設定
		region.sizeRatio = Vec2{ 0.5, 0.5 };
		region.sizeDelta = Vec2{ -20, -20 };
		
		node->setRegion(region);
		
		auto* inlineRegion = node->inlineRegion();
		REQUIRE(inlineRegion != nullptr);
		REQUIRE(inlineRegion->sizeRatio == Vec2{ 0.5, 0.5 });
		REQUIRE(inlineRegion->sizeDelta == Vec2{ -20, -20 });
	}

	SECTION("Flexible weight")
	{
		auto parent = noco::Node::Create();
		auto child1 = noco::Node::Create();
		auto child2 = noco::Node::Create();
		
		// 水平レイアウトを設定
		noco::HorizontalLayout layout;
		parent->setChildrenLayout(layout);
		
		// フレキシブルウェイトを設定
		noco::InlineRegion region1;
		region1.flexibleWeight = 1.0;
		child1->setRegion(region1);
		
		noco::InlineRegion region2;
		region2.flexibleWeight = 2.0;
		child2->setRegion(region2);
		
		parent->addChild(child1);
		parent->addChild(child2);
		
		// フレキシブルウェイトの確認
		REQUIRE(child1->inlineRegion()->flexibleWeight == 1.0);
		REQUIRE(child2->inlineRegion()->flexibleWeight == 2.0);
	}

	SECTION("Margins")
	{
		auto node = noco::Node::Create();
		noco::InlineRegion region;
		
		// マージンの設定
		region.margin = noco::LRTB{ 10, 20, 30, 40 };
		
		node->setRegion(region);
		
		auto* inlineRegion = node->inlineRegion();
		REQUIRE(inlineRegion->margin.left == 10);
		REQUIRE(inlineRegion->margin.right == 20);
		REQUIRE(inlineRegion->margin.top == 30);
		REQUIRE(inlineRegion->margin.bottom == 40);
	}

	SECTION("InlineRegion with max size - sizeRatio")
	{
		// sizeRatioが設定されている場合の最大サイズテスト
		noco::InlineRegion region;
		region.sizeRatio = Vec2{ 1.0, 1.0 };
		region.sizeDelta = Vec2{ 0.0, 0.0 };
		region.maxWidth = 500.0;
		region.maxHeight = 400.0;
		
		// 親の領域が大きい場合、最大サイズが適用される
		RectF parentRect{ 0, 0, 1000, 800 };
		RectF result = region.applyRegion(parentRect, Vec2::Zero());
		REQUIRE(result.w == 500.0);
		REQUIRE(result.h == 400.0);
		
		// 親の領域が小さい場合、親のサイズに従う
		RectF smallParentRect{ 0, 0, 300, 200 };
		RectF smallResult = region.applyRegion(smallParentRect, Vec2::Zero());
		REQUIRE(smallResult.w == 300.0);
		REQUIRE(smallResult.h == 200.0);
	}

	SECTION("InlineRegion with max size - flexibleWeight")
	{
		// flexibleWeightが設定されている場合の最大サイズテスト
		noco::InlineRegion region;
		region.flexibleWeight = 1.0;
		region.sizeDelta = Vec2{ 100.0, 50.0 };
		region.maxWidth = 600.0;
		region.maxHeight = 300.0;
		
		// 親の領域とオフセットを考慮
		RectF parentRect{ 0, 0, 800, 600 };
		Vec2 offset{ 0, 0 };
		
		// flexibleWeightの場合、sizeDeltaが基本サイズとなる
		RectF result = region.applyRegion(parentRect, offset);
		REQUIRE(result.w == 100.0);
		REQUIRE(result.h == 50.0);
		
		// maxサイズはsizeDeltaより大きい場合のみ影響
		region.sizeDelta = Vec2{ 700.0, 400.0 };
		RectF limitedResult = region.applyRegion(parentRect, offset);
		REQUIRE(limitedResult.w == 600.0);
		REQUIRE(limitedResult.h == 300.0);
	}

	SECTION("InlineRegion with partial max size")
	{
		// 片方だけ最大サイズが設定されている場合
		noco::InlineRegion region;
		region.sizeRatio = Vec2{ 1.0, 1.0 };
		region.maxWidth = 500.0;
		// maxHeightは設定しない
		
		RectF parentRect{ 0, 0, 1000, 800 };
		RectF result = region.applyRegion(parentRect, Vec2::Zero());
		REQUIRE(result.w == 500.0);
		REQUIRE(result.h == 800.0); // 高さは親のサイズそのまま
	}
}

TEST_CASE("AnchorRegion detailed", "[Region][AnchorRegion]")
{
	SECTION("Anchor presets")
	{
		auto node = noco::Node::Create();
		
		// 左上アンカー
		noco::AnchorRegion topLeft;
		topLeft.anchorMin = Vec2{ 0, 0 };
		topLeft.anchorMax = Vec2{ 0, 0 };
		topLeft.posDelta = Vec2{ 10, 10 };
		topLeft.sizeDelta = Vec2{ 100, 50 };
		
		node->setRegion(topLeft);
		
		auto* region = std::get_if<noco::AnchorRegion>(&node->region());
		REQUIRE(region != nullptr);
		REQUIRE(region->anchorMin == Vec2{ 0, 0 });
		REQUIRE(region->anchorMax == Vec2{ 0, 0 });
	}

	SECTION("Stretch anchors")
	{
		auto node = noco::Node::Create();
		
		// 水平ストレッチ
		noco::AnchorRegion hStretch;
		hStretch.anchorMin = Vec2{ 0, 0.5 };
		hStretch.anchorMax = Vec2{ 1, 0.5 };
		hStretch.posDelta = Vec2{ 0, 0 };
		hStretch.sizeDelta = Vec2{ -20, 50 };
		
		node->setRegion(hStretch);
		
		auto* region = std::get_if<noco::AnchorRegion>(&node->region());
		REQUIRE(region->anchorMin.x == 0);
		REQUIRE(region->anchorMax.x == 1);
		REQUIRE(region->sizeDelta.x == -20);
	}

	SECTION("Size delta pivot")
	{
		auto node = noco::Node::Create();
		
		noco::AnchorRegion region;
		region.sizeDeltaPivot = Vec2{ 0, 0 };
		
		node->setRegion(region);
		
		auto* anchorRegion = std::get_if<noco::AnchorRegion>(&node->region());
		REQUIRE(anchorRegion->sizeDeltaPivot == Vec2{ 0, 0 });
	}
}

TEST_CASE("Combined Region scenarios", "[Region][AnchorRegion][InlineRegion]")
{
	SECTION("Parent with AnchorRegion, child with InlineRegion")
	{
		auto parent = noco::Node::Create();
		auto child = noco::Node::Create();
		
		// 親ノードにAnchorRegionを設定（ストレッチで最大サイズあり）
		noco::AnchorRegion parentRegion;
		parentRegion.anchorMin = Vec2{ 0, 0 };
		parentRegion.anchorMax = Vec2{ 1, 1 };
		parentRegion.maxWidth = 600.0;
		parentRegion.maxHeight = 400.0;
		parent->setRegion(parentRegion);
		
		// 子ノードにInlineRegionを設定（親のサイズに対する比率）
		noco::InlineRegion childRegion;
		childRegion.sizeRatio = Vec2{ 0.5, 0.5 };
		childRegion.maxWidth = 200.0;
		childRegion.maxHeight = 150.0;
		child->setRegion(childRegion);
		
		parent->addChild(child);
		
		// 大きな親領域での計算をシミュレート
		RectF grandParentRect{ 0, 0, 1000, 800 };
		RectF parentRect = parentRegion.applyRegion(grandParentRect, Vec2::Zero());
		
		// 親のサイズは最大サイズに制限される
		REQUIRE(parentRect.w == 600.0);
		REQUIRE(parentRect.h == 400.0);
		
		// 子のサイズ計算
		RectF childRect = childRegion.applyRegion(parentRect, Vec2::Zero());
		
		// 子は親の50%サイズを要求するが、自身の最大サイズに制限される
		// 親の50% = (300, 200)だが、maxで(200, 150)に制限
		REQUIRE(childRect.w == 200.0);
		REQUIRE(childRect.h == 150.0);
	}
	
	SECTION("Nested stretch regions with max sizes")
	{
		auto grandParent = noco::Node::Create();
		auto parent = noco::Node::Create();
		auto child = noco::Node::Create();
		
		// 祖父母ノード（固定サイズ）
		noco::InlineRegion grandParentRegion;
		grandParentRegion.sizeDelta = Vec2{ 1200, 900 };
		grandParent->setRegion(grandParentRegion);
		
		// 親ノード（水平ストレッチ、最大幅あり）
		noco::AnchorRegion parentRegion;
		parentRegion.anchorMin = Vec2{ 0, 0.25 };
		parentRegion.anchorMax = Vec2{ 1, 0.75 };
		parentRegion.maxWidth = 800.0;
		parentRegion.sizeDeltaPivot = noco::Anchor::MiddleCenter;
		parent->setRegion(parentRegion);
		
		// 子ノード（フルストレッチ、最大サイズあり）
		noco::AnchorRegion childRegion;
		childRegion.anchorMin = Vec2{ 0, 0 };
		childRegion.anchorMax = Vec2{ 1, 1 };
		childRegion.maxWidth = 600.0;
		childRegion.maxHeight = 300.0;
		childRegion.sizeDeltaPivot = noco::Anchor::MiddleCenter;
		child->setRegion(childRegion);
		
		grandParent->addChild(parent);
		parent->addChild(child);
		
		// 計算をシミュレート
		RectF rootRect{ 0, 0, 1200, 900 };
		RectF parentRect = parentRegion.applyRegion(rootRect, Vec2::Zero());
		
		// 親の幅は最大800に制限、高さは50%（450）
		REQUIRE(parentRect.w == 800.0);
		REQUIRE(parentRect.h == 450.0);
		// 中央配置の確認
		REQUIRE(parentRect.center().x == 600.0);
		
		// 子の計算
		RectF childRect = childRegion.applyRegion(parentRect, Vec2::Zero());
		
		// 子の幅は最大600に制限、高さは最大300に制限
		REQUIRE(childRect.w == 600.0);
		REQUIRE(childRect.h == 300.0);
		// 親の中央に配置される
		REQUIRE(childRect.center() == parentRect.center());
	}
	
	SECTION("Mixed region types in layout")
	{
		auto parent = noco::Node::Create();
		auto child1 = noco::Node::Create();
		auto child2 = noco::Node::Create();
		auto child3 = noco::Node::Create();
		
		// 親に水平レイアウトを設定
		noco::HorizontalLayout layout;
		layout.spacing = 10;
		parent->setChildrenLayout(layout);
		
		// 親のサイズ（AnchorRegionでストレッチ）
		noco::AnchorRegion parentRegion;
		parentRegion.anchorMin = Vec2{ 0, 0 };
		parentRegion.anchorMax = Vec2{ 1, 0 };
		parentRegion.sizeDelta = Vec2{ 0, 200 };
		parentRegion.maxWidth = 1000.0;
		parent->setRegion(parentRegion);
		
		// 子1: 固定サイズ
		noco::InlineRegion child1Region;
		child1Region.sizeDelta = Vec2{ 100, 0 };
		child1Region.sizeRatio = Vec2{ 0, 1 };
		child1->setRegion(child1Region);
		
		// 子2: フレキシブル、最大幅あり
		noco::InlineRegion child2Region;
		child2Region.flexibleWeight = 1.0;
		child2Region.sizeRatio = Vec2{ 0, 1 };
		child2Region.maxWidth = 300.0;
		child2->setRegion(child2Region);
		
		// 子3: フレキシブル、最大幅なし
		noco::InlineRegion child3Region;
		child3Region.flexibleWeight = 1.0;
		child3Region.sizeRatio = Vec2{ 0, 1 };
		child3->setRegion(child3Region);
		
		parent->addChild(child1);
		parent->addChild(child2);
		parent->addChild(child3);
		
		// 検証
		REQUIRE(child1->inlineRegion()->sizeDelta.x == 100.0);
		REQUIRE(child2->inlineRegion()->maxWidth.has_value());
		REQUIRE(*child2->inlineRegion()->maxWidth == 300.0);
		REQUIRE(!child3->inlineRegion()->maxWidth.has_value());
	}
}

TEST_CASE("Min/Max size regions", "[Region][AnchorRegion][InlineRegion]")
{
	SECTION("AnchorRegion with min/max size regions")
	{
		// min/maxサイズを両方設定した場合のテスト
		noco::AnchorRegion region;
		region.anchorMin = Vec2{ 0, 0 };
		region.anchorMax = Vec2{ 1, 1 };
		region.minWidth = 200.0;
		region.minHeight = 150.0;
		region.maxWidth = 600.0;
		region.maxHeight = 400.0;
		region.sizeDeltaPivot = noco::Anchor::MiddleCenter;
		
		// 親領域が小さい場合、最小サイズが適用される
		RectF smallParentRect{ 0, 0, 100, 80 };
		RectF smallResult = region.applyRegion(smallParentRect, Vec2::Zero());
		REQUIRE(smallResult.w == 200.0);
		REQUIRE(smallResult.h == 150.0);
		// 中央配置の確認（親より大きくなるが中央に配置）
		REQUIRE(smallResult.center().x == 50.0);
		REQUIRE(smallResult.center().y == 40.0);
		
		// 親領域が大きい場合、最大サイズが適用される
		RectF largeParentRect{ 0, 0, 1000, 800 };
		RectF largeResult = region.applyRegion(largeParentRect, Vec2::Zero());
		REQUIRE(largeResult.w == 600.0);
		REQUIRE(largeResult.h == 400.0);
		// 中央配置の確認
		REQUIRE(largeResult.center().x == 500.0);
		REQUIRE(largeResult.center().y == 400.0);
		
		// 親領域がmin/maxの間の場合、親のサイズに従う
		RectF mediumParentRect{ 0, 0, 400, 300 };
		RectF mediumResult = region.applyRegion(mediumParentRect, Vec2::Zero());
		REQUIRE(mediumResult.w == 400.0);
		REQUIRE(mediumResult.h == 300.0);
		REQUIRE(mediumResult.center().x == 200.0);
		REQUIRE(mediumResult.center().y == 150.0);
	}

	SECTION("AnchorRegion with invalid min/max values")
	{
		// minがmaxより大きい場合のテスト（minが適用された後maxで制限される）
		noco::AnchorRegion region;
		region.anchorMin = Vec2{ 0, 0 };
		region.anchorMax = Vec2{ 1, 1 };
		region.minWidth = 600.0;
		region.minHeight = 400.0;
		region.maxWidth = 300.0;  // minより小さい値
		region.maxHeight = 200.0; // minより小さい値
		region.sizeDeltaPivot = noco::Anchor::MiddleCenter;
		
		RectF parentRect{ 0, 0, 1000, 800 };
		RectF result = region.applyRegion(parentRect, Vec2::Zero());
		// minが適用され、その後maxで制限されるため、maxの値になる
		REQUIRE(result.w == 300.0);
		REQUIRE(result.h == 200.0);
	}

	SECTION("InlineRegion with min/max size regions")
	{
		// InlineRegionでmin/maxサイズを両方設定
		noco::InlineRegion region;
		region.sizeRatio = Vec2{ 1.0, 1.0 };
		region.minWidth = 150.0;
		region.minHeight = 100.0;
		region.maxWidth = 500.0;
		region.maxHeight = 350.0;
		
		// 親領域が小さい場合、最小サイズが適用される
		RectF smallParentRect{ 0, 0, 80, 60 };
		RectF smallResult = region.applyRegion(smallParentRect, Vec2::Zero());
		REQUIRE(smallResult.w == 150.0);
		REQUIRE(smallResult.h == 100.0);
		
		// 親領域が大きい場合、最大サイズが適用される
		RectF largeParentRect{ 0, 0, 800, 600 };
		RectF largeResult = region.applyRegion(largeParentRect, Vec2::Zero());
		REQUIRE(largeResult.w == 500.0);
		REQUIRE(largeResult.h == 350.0);
		
		// 親領域がmin/maxの間の場合、親のサイズに従う
		RectF mediumParentRect{ 0, 0, 300, 200 };
		RectF mediumResult = region.applyRegion(mediumParentRect, Vec2::Zero());
		REQUIRE(mediumResult.w == 300.0);
		REQUIRE(mediumResult.h == 200.0);
	}

	SECTION("InlineRegion with conflicting min/max values")
	{
		// minがmaxより大きい場合のテスト
		noco::InlineRegion region;
		region.sizeDelta = Vec2{ 400, 300 };
		region.minWidth = 500.0;
		region.minHeight = 350.0;
		region.maxWidth = 300.0;  // minより小さい値
		region.maxHeight = 200.0; // minより小さい値
		
		RectF parentRect{ 0, 0, 1000, 800 };
		RectF result = region.applyRegion(parentRect, Vec2::Zero());
		// minが適用され、その後maxで制限されるため、maxの値になる
		REQUIRE(result.w == 300.0);
		REQUIRE(result.h == 200.0);
	}

	SECTION("Mixed regions with min/max in hierarchy")
	{
		// 階層内でmin/max制約が混在する場合
		noco::AnchorRegion parentRegion;
		parentRegion.anchorMin = Vec2{ 0, 0 };
		parentRegion.anchorMax = Vec2{ 1, 1 };
		parentRegion.minWidth = 400.0;
		parentRegion.maxWidth = 800.0;
		
		noco::InlineRegion childRegion;
		childRegion.sizeRatio = Vec2{ 0.75, 0.75 };
		childRegion.minWidth = 200.0;
		childRegion.maxWidth = 500.0;
		
		// 大きな親領域での計算
		RectF grandParentRect{ 0, 0, 1000, 600 };
		RectF parentRect = parentRegion.applyRegion(grandParentRect, Vec2::Zero());
		
		// 親は最大サイズで制限される
		REQUIRE(parentRect.w == 800.0);
		REQUIRE(parentRect.h == 600.0);
		
		// 子は親の75%を要求するが、自身のmax制限で500に制限される
		RectF childRect = childRegion.applyRegion(parentRect, Vec2::Zero());
		REQUIRE(childRect.w == 500.0); // 親の75% = 600だが、maxで500に制限
		REQUIRE(childRect.h == 450.0); // 親の75% = 450（制限なし）
	}
}

TEST_CASE("Min/Max size regions with Transform", "[Region][AnchorRegion][InlineRegion][Transform]")
{
	SECTION("AnchorRegion min/max independent of Transform scale")
	{
		// Transformのscaleが制約サイズに影響しないことを確認
		auto node = noco::Node::Create();
		
		noco::AnchorRegion region;
		region.anchorMin = Vec2{ 0, 0 };
		region.anchorMax = Vec2{ 1, 1 };
		region.minWidth = 100.0;
		region.minHeight = 80.0;
		region.maxWidth = 300.0;
		region.maxHeight = 200.0;
		region.sizeDeltaPivot = noco::Anchor::MiddleCenter;
		
		node->setRegion(region);
		
		// Transformでスケールを2倍に設定
		node->transform().setScale(Vec2{ 2.0, 2.0 });
		
		// 制約計算はTransformの影響を受けない
		RectF parentRect{ 0, 0, 50, 40 }; // 小さい親領域
		RectF result = region.applyRegion(parentRect, Vec2::Zero());
		
		// minサイズが適用される（Transformのscaleは無関係）
		REQUIRE(result.w == 100.0);
		REQUIRE(result.h == 80.0);
		
		// 大きい親領域でのテスト
		RectF largeParentRect{ 0, 0, 500, 400 };
		RectF largeResult = region.applyRegion(largeParentRect, Vec2::Zero());
		
		// maxサイズが適用される（Transformのscaleは無関係）
		REQUIRE(largeResult.w == 300.0);
		REQUIRE(largeResult.h == 200.0);
	}

	SECTION("InlineRegion min/max independent of Transform scale")
	{
		// InlineRegionでもTransformのscaleが制約サイズに影響しないことを確認
		auto node = noco::Node::Create();
		
		noco::InlineRegion region;
		region.sizeRatio = Vec2{ 1.0, 1.0 };
		region.minWidth = 120.0;
		region.minHeight = 90.0;
		region.maxWidth = 400.0;
		region.maxHeight = 300.0;
		
		node->setRegion(region);
		
		// Transformでスケールを0.5倍に設定
		node->transform().setScale(Vec2{ 0.5, 0.5 });
		
		// 制約計算はTransformの影響を受けない
		RectF parentRect{ 0, 0, 80, 60 }; // 小さい親領域
		RectF result = region.applyRegion(parentRect, Vec2::Zero());
		
		// minサイズが適用される（Transformのscaleは無関係）
		REQUIRE(result.w == 120.0);
		REQUIRE(result.h == 90.0);
		
		// 大きい親領域でのテスト
		RectF largeParentRect{ 0, 0, 600, 500 };
		RectF largeResult = region.applyRegion(largeParentRect, Vec2::Zero());
		
		// maxサイズが適用される（Transformのscaleは無関係）
		REQUIRE(largeResult.w == 400.0);
		REQUIRE(largeResult.h == 300.0);
	}

	SECTION("Parent and child with different Transform scales")
	{
		// 親と子で異なるTransformが設定されている場合のテスト
		auto parent = noco::Node::Create();
		auto child = noco::Node::Create();
		
		// 親ノードの設定
		noco::AnchorRegion parentRegion;
		parentRegion.anchorMin = Vec2{ 0, 0 };
		parentRegion.anchorMax = Vec2{ 1, 1 };
		parentRegion.maxWidth = 500.0;
		parentRegion.maxHeight = 400.0;
		parent->setRegion(parentRegion);
		
		// 親のTransform
		parent->transform().setScale(Vec2{ 1.5, 1.5 });
		
		// 子ノードの設定
		noco::InlineRegion childRegion;
		childRegion.sizeRatio = Vec2{ 0.8, 0.8 };
		childRegion.minWidth = 150.0;
		childRegion.minHeight = 120.0;
		child->setRegion(childRegion);
		
		// 子のTransform
		child->transform().setScale(Vec2{ 0.8, 0.8 });
		
		parent->addChild(child);
		
		// 制約計算の確認
		RectF grandParentRect{ 0, 0, 800, 600 };
		RectF parentRect = parentRegion.applyRegion(grandParentRect, Vec2::Zero());
		
		// 親のサイズはmaxで制限される（親のTransformのscaleは無関係）
		REQUIRE(parentRect.w == 500.0);
		REQUIRE(parentRect.h == 400.0);
		
		// 子のサイズ計算
		RectF childRect = childRegion.applyRegion(parentRect, Vec2::Zero());
		
		// 子は親の80% = (400, 320)を要求、minサイズ(150, 120)より大きいのでそのまま
		// Transformのscaleは制約計算に影響しない
		REQUIRE(childRect.w == 400.0);
		REQUIRE(childRect.h == 320.0);
	}

	SECTION("Complex hierarchy with multiple scales and regions")
	{
		// 複雑な階層でのTransformとmin/max制約の独立性を確認
		auto grandParent = noco::Node::Create();
		auto parent = noco::Node::Create();
		auto child = noco::Node::Create();
		
		// 祖父母ノード
		noco::InlineRegion grandParentRegion;
		grandParentRegion.sizeDelta = Vec2{ 1000, 800 };
		grandParent->setRegion(grandParentRegion);
		
		grandParent->transform().setScale(Vec2{ 2.0, 2.0 });
		
		// 親ノード
		noco::AnchorRegion parentRegion;
		parentRegion.anchorMin = Vec2{ 0.1, 0.1 };
		parentRegion.anchorMax = Vec2{ 0.9, 0.9 };
		parentRegion.minWidth = 200.0;
		parentRegion.maxWidth = 600.0;
		parent->setRegion(parentRegion);
		
		parent->transform().setScale(Vec2{ 0.7, 0.7 });
		
		// 子ノード
		noco::InlineRegion childRegion;
		childRegion.sizeRatio = Vec2{ 1.2, 1.2 }; // 親より大きく要求
		childRegion.maxWidth = 500.0;
		childRegion.maxHeight = 400.0;
		child->setRegion(childRegion);
		
		child->transform().setScale(Vec2{ 3.0, 3.0 });
		
		grandParent->addChild(parent);
		parent->addChild(child);
		
		// 制約計算の確認
		RectF rootRect{ 0, 0, 1000, 800 };
		
		// 親の制約計算（祖父母のTransformは影響しない）
		RectF parentRect = parentRegion.applyRegion(rootRect, Vec2::Zero());
		
		// 親のサイズ = 祖父母の80%サイズの領域 = (800, 640)
		// minWidth(200)より大きく、maxWidth(600)で制限される
		REQUIRE(parentRect.w == 600.0);
		REQUIRE(parentRect.h == 640.0);
		
		// 子の制約計算（親のTransformは影響しない）
		RectF childRect = childRegion.applyRegion(parentRect, Vec2::Zero());
		
		// 子のサイズ = 親の120% = (720, 768)を要求
		// maxWidth(500), maxHeight(400)で制限される
		REQUIRE(childRect.w == 500.0);
		REQUIRE(childRect.h == 400.0);
	}
	
	SECTION("Invalid region values handling")
	{
		auto node = noco::Node::Create();
		
		// 負のサイズを持つInlineRegion
		{
			node->setRegion(noco::InlineRegion
			{
				.sizeDelta = Vec2{ -100, -50 },
			});
			
			// 負のサイズでも設定はされる（クラッシュしない）
			auto* inlineRegion = node->inlineRegion();
			REQUIRE(inlineRegion != nullptr);
			REQUIRE(inlineRegion->sizeDelta == Vec2{ -100, -50 });
		}
		
		// 範囲外のアンカー値を持つAnchorRegion
		{
			node->setRegion(noco::AnchorRegion
			{
				.anchorMin = Vec2{ -0.5, 1.5 },
				.anchorMax = Vec2{ 2.0, -1.0 },
			});
			
			// 範囲外のアンカー値でも設定はされる（クラッシュしない）
			auto* anchorRegion = std::get_if<noco::AnchorRegion>(&node->region());
			REQUIRE(anchorRegion != nullptr);
			REQUIRE(anchorRegion->anchorMin == Vec2{ -0.5, 1.5 });
			REQUIRE(anchorRegion->anchorMax == Vec2{ 2.0, -1.0 });
		}
		
		// 負のフレキシブルウェイト
		{
			node->setRegion(noco::InlineRegion
			{
				.flexibleWeight = -5.0,
			});
			
			auto* inlineRegion = node->inlineRegion();
			REQUIRE(inlineRegion != nullptr);
			REQUIRE(inlineRegion->flexibleWeight == -5.0);
		}
		
		// 負のマージン
		{
			node->setRegion(noco::InlineRegion
			{
				.margin = noco::LRTB{ -10, -20, -30, -40 },
			});
			
			auto* inlineRegion = node->inlineRegion();
			REQUIRE(inlineRegion != nullptr);
			REQUIRE(inlineRegion->margin.left == -10);
			REQUIRE(inlineRegion->margin.right == -20);
			REQUIRE(inlineRegion->margin.top == -30);
			REQUIRE(inlineRegion->margin.bottom == -40);
		}
	}
}