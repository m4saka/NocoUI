# include <catch2/catch.hpp>
# include <Siv3D.hpp>
# include <NocoUI.hpp>

// ========================================
// Constraintのテスト
// ========================================

TEST_CASE("Constraint system", "[Constraint]")
{
	SECTION("BoxConstraint")
	{
		auto node = noco::Node::Create();
		noco::BoxConstraint constraint;
		constraint.sizeDelta = Vec2{ 100.0f, 50.0f };
		
		node->setConstraint(constraint);
		
		// Constraintが正しく設定されているか確認
		auto* boxConstraint = node->boxConstraint();
		REQUIRE(boxConstraint != nullptr);
		REQUIRE(boxConstraint->sizeDelta.x == 100.0f);
		REQUIRE(boxConstraint->sizeDelta.y == 50.0f);
	}

	SECTION("AnchorConstraint")
	{
		auto node = noco::Node::Create();
		noco::AnchorConstraint constraint;
		constraint.anchorMin = Vec2{ 0.0f, 0.0f };
		constraint.anchorMax = Vec2{ 1.0f, 1.0f };
		
		node->setConstraint(constraint);
		
		// Constraintが正しく設定されているか確認
		auto* anchorConstraint = std::get_if<noco::AnchorConstraint>(&node->constraint());
		REQUIRE(anchorConstraint != nullptr);
		REQUIRE(anchorConstraint->anchorMin == Vec2{ 0.0f, 0.0f });
		REQUIRE(anchorConstraint->anchorMax == Vec2{ 1.0f, 1.0f });
	}

	SECTION("AnchorConstraint with max size")
	{
		// 最大サイズの計算をテスト
		noco::AnchorConstraint constraint;
		constraint.anchorMin = Vec2{ 0.0f, 0.0f };
		constraint.anchorMax = Vec2{ 1.0f, 1.0f };
		constraint.sizeDelta = Vec2{ 0.0f, 0.0f };
		constraint.maxWidth = 500.0;
		constraint.maxHeight = 400.0;
		
		// 親の領域が大きい場合、最大サイズが適用される
		RectF parentRect{ 0, 0, 1000, 800 };
		RectF result = constraint.applyConstraint(parentRect, Vec2::Zero());
		REQUIRE(result.w == 500.0f);
		REQUIRE(result.h == 400.0f);
		
		// 親の領域が小さい場合、親のサイズに従う
		RectF smallParentRect{ 0, 0, 300, 200 };
		RectF smallResult = constraint.applyConstraint(smallParentRect, Vec2::Zero());
		REQUIRE(smallResult.w == 300.0f);
		REQUIRE(smallResult.h == 200.0f);
	}

	SECTION("AnchorConstraint with max size and centered pivot")
	{
		// 中央配置のピボットで最大サイズをテスト
		noco::AnchorConstraint constraint;
		constraint.anchorMin = Vec2{ 0.0f, 0.0f };
		constraint.anchorMax = Vec2{ 1.0f, 1.0f };
		constraint.sizeDelta = Vec2{ 0.0f, 0.0f };
		constraint.sizeDeltaPivot = noco::Anchor::MiddleCenter;
		constraint.maxWidth = 500.0;
		constraint.maxHeight = 400.0;
		
		// 親の領域が大きい場合、最大サイズが適用され、中央に配置される
		RectF parentRect{ 0, 0, 1000, 800 };
		RectF result = constraint.applyConstraint(parentRect, Vec2::Zero());
		REQUIRE(result.w == 500.0f);
		REQUIRE(result.h == 400.0f);
		// 中央配置の確認（親の中心と結果の中心が一致）
		REQUIRE(result.center() == parentRect.center());
	}
}

TEST_CASE("BoxConstraint detailed", "[Constraint][BoxConstraint]")
{
	SECTION("Size ratio and delta")
	{
		auto node = noco::Node::Create();
		noco::BoxConstraint constraint;
		
		// サイズ比率の設定
		constraint.sizeRatio = Vec2{ 0.5, 0.5 };
		constraint.sizeDelta = Vec2{ -20, -20 };
		
		node->setConstraint(constraint);
		
		auto* boxConstraint = node->boxConstraint();
		REQUIRE(boxConstraint != nullptr);
		REQUIRE(boxConstraint->sizeRatio == Vec2{ 0.5, 0.5 });
		REQUIRE(boxConstraint->sizeDelta == Vec2{ -20, -20 });
	}

	SECTION("Flexible weight")
	{
		auto parent = noco::Node::Create();
		auto child1 = noco::Node::Create();
		auto child2 = noco::Node::Create();
		
		// 水平レイアウトを設定
		noco::HorizontalLayout layout;
		parent->setBoxChildrenLayout(layout);
		
		// フレキシブルウェイトを設定
		noco::BoxConstraint constraint1;
		constraint1.flexibleWeight = 1.0;
		child1->setConstraint(constraint1);
		
		noco::BoxConstraint constraint2;
		constraint2.flexibleWeight = 2.0;
		child2->setConstraint(constraint2);
		
		parent->addChild(child1);
		parent->addChild(child2);
		
		// フレキシブルウェイトの確認
		REQUIRE(child1->boxConstraint()->flexibleWeight == 1.0);
		REQUIRE(child2->boxConstraint()->flexibleWeight == 2.0);
	}

	SECTION("Margins")
	{
		auto node = noco::Node::Create();
		noco::BoxConstraint constraint;
		
		// マージンの設定
		constraint.margin = noco::LRTB{ 10, 20, 30, 40 };
		
		node->setConstraint(constraint);
		
		auto* boxConstraint = node->boxConstraint();
		REQUIRE(boxConstraint->margin.left == 10);
		REQUIRE(boxConstraint->margin.right == 20);
		REQUIRE(boxConstraint->margin.top == 30);
		REQUIRE(boxConstraint->margin.bottom == 40);
	}

	SECTION("BoxConstraint with max size - sizeRatio")
	{
		// sizeRatioが設定されている場合の最大サイズテスト
		noco::BoxConstraint constraint;
		constraint.sizeRatio = Vec2{ 1.0, 1.0 };
		constraint.sizeDelta = Vec2{ 0.0, 0.0 };
		constraint.maxWidth = 500.0;
		constraint.maxHeight = 400.0;
		
		// 親の領域が大きい場合、最大サイズが適用される
		RectF parentRect{ 0, 0, 1000, 800 };
		RectF result = constraint.applyConstraint(parentRect, Vec2::Zero());
		REQUIRE(result.w == 500.0);
		REQUIRE(result.h == 400.0);
		
		// 親の領域が小さい場合、親のサイズに従う
		RectF smallParentRect{ 0, 0, 300, 200 };
		RectF smallResult = constraint.applyConstraint(smallParentRect, Vec2::Zero());
		REQUIRE(smallResult.w == 300.0);
		REQUIRE(smallResult.h == 200.0);
	}

	SECTION("BoxConstraint with max size - flexibleWeight")
	{
		// flexibleWeightが設定されている場合の最大サイズテスト
		noco::BoxConstraint constraint;
		constraint.flexibleWeight = 1.0;
		constraint.sizeDelta = Vec2{ 100.0, 50.0 };
		constraint.maxWidth = 600.0;
		constraint.maxHeight = 300.0;
		
		// 親の領域とオフセットを考慮
		RectF parentRect{ 0, 0, 800, 600 };
		Vec2 offset{ 0, 0 };
		
		// flexibleWeightの場合、sizeDeltaが基本サイズとなる
		RectF result = constraint.applyConstraint(parentRect, offset);
		REQUIRE(result.w == 100.0);
		REQUIRE(result.h == 50.0);
		
		// maxサイズはsizeDeltaより大きい場合のみ影響
		constraint.sizeDelta = Vec2{ 700.0, 400.0 };
		RectF limitedResult = constraint.applyConstraint(parentRect, offset);
		REQUIRE(limitedResult.w == 600.0);
		REQUIRE(limitedResult.h == 300.0);
	}

	SECTION("BoxConstraint with partial max size")
	{
		// 片方だけ最大サイズが設定されている場合
		noco::BoxConstraint constraint;
		constraint.sizeRatio = Vec2{ 1.0, 1.0 };
		constraint.maxWidth = 500.0;
		// maxHeightは設定しない
		
		RectF parentRect{ 0, 0, 1000, 800 };
		RectF result = constraint.applyConstraint(parentRect, Vec2::Zero());
		REQUIRE(result.w == 500.0);
		REQUIRE(result.h == 800.0); // 高さは親のサイズそのまま
	}
}

TEST_CASE("AnchorConstraint detailed", "[Constraint][AnchorConstraint]")
{
	SECTION("Anchor presets")
	{
		auto node = noco::Node::Create();
		
		// 左上アンカー
		noco::AnchorConstraint topLeft;
		topLeft.anchorMin = Vec2{ 0, 0 };
		topLeft.anchorMax = Vec2{ 0, 0 };
		topLeft.posDelta = Vec2{ 10, 10 };
		topLeft.sizeDelta = Vec2{ 100, 50 };
		
		node->setConstraint(topLeft);
		
		auto* constraint = std::get_if<noco::AnchorConstraint>(&node->constraint());
		REQUIRE(constraint != nullptr);
		REQUIRE(constraint->anchorMin == Vec2{ 0, 0 });
		REQUIRE(constraint->anchorMax == Vec2{ 0, 0 });
	}

	SECTION("Stretch anchors")
	{
		auto node = noco::Node::Create();
		
		// 水平ストレッチ
		noco::AnchorConstraint hStretch;
		hStretch.anchorMin = Vec2{ 0, 0.5 };
		hStretch.anchorMax = Vec2{ 1, 0.5 };
		hStretch.posDelta = Vec2{ 0, 0 };
		hStretch.sizeDelta = Vec2{ -20, 50 };
		
		node->setConstraint(hStretch);
		
		auto* constraint = std::get_if<noco::AnchorConstraint>(&node->constraint());
		REQUIRE(constraint->anchorMin.x == 0);
		REQUIRE(constraint->anchorMax.x == 1);
		REQUIRE(constraint->sizeDelta.x == -20);
	}

	SECTION("Size delta pivot")
	{
		auto node = noco::Node::Create();
		
		noco::AnchorConstraint constraint;
		constraint.sizeDeltaPivot = Vec2{ 0, 0 };
		
		node->setConstraint(constraint);
		
		auto* anchorConstraint = std::get_if<noco::AnchorConstraint>(&node->constraint());
		REQUIRE(anchorConstraint->sizeDeltaPivot == Vec2{ 0, 0 });
	}
}

TEST_CASE("Combined Constraint scenarios", "[Constraint][AnchorConstraint][BoxConstraint]")
{
	SECTION("Parent with AnchorConstraint, child with BoxConstraint")
	{
		auto parent = noco::Node::Create();
		auto child = noco::Node::Create();
		
		// 親ノードにAnchorConstraintを設定（ストレッチで最大サイズあり）
		noco::AnchorConstraint parentConstraint;
		parentConstraint.anchorMin = Vec2{ 0, 0 };
		parentConstraint.anchorMax = Vec2{ 1, 1 };
		parentConstraint.maxWidth = 600.0;
		parentConstraint.maxHeight = 400.0;
		parent->setConstraint(parentConstraint);
		
		// 子ノードにBoxConstraintを設定（親のサイズに対する比率）
		noco::BoxConstraint childConstraint;
		childConstraint.sizeRatio = Vec2{ 0.5, 0.5 };
		childConstraint.maxWidth = 200.0;
		childConstraint.maxHeight = 150.0;
		child->setConstraint(childConstraint);
		
		parent->addChild(child);
		
		// 大きな親領域での計算をシミュレート
		RectF grandParentRect{ 0, 0, 1000, 800 };
		RectF parentRect = parentConstraint.applyConstraint(grandParentRect, Vec2::Zero());
		
		// 親のサイズは最大サイズに制限される
		REQUIRE(parentRect.w == 600.0);
		REQUIRE(parentRect.h == 400.0);
		
		// 子のサイズ計算
		RectF childRect = childConstraint.applyConstraint(parentRect, Vec2::Zero());
		
		// 子は親の50%サイズを要求するが、自身の最大サイズに制限される
		// 親の50% = (300, 200)だが、maxで(200, 150)に制限
		REQUIRE(childRect.w == 200.0);
		REQUIRE(childRect.h == 150.0);
	}
	
	SECTION("Nested stretch constraints with max sizes")
	{
		auto grandParent = noco::Node::Create();
		auto parent = noco::Node::Create();
		auto child = noco::Node::Create();
		
		// 祖父母ノード（固定サイズ）
		noco::BoxConstraint grandParentConstraint;
		grandParentConstraint.sizeDelta = Vec2{ 1200, 900 };
		grandParent->setConstraint(grandParentConstraint);
		
		// 親ノード（水平ストレッチ、最大幅あり）
		noco::AnchorConstraint parentConstraint;
		parentConstraint.anchorMin = Vec2{ 0, 0.25 };
		parentConstraint.anchorMax = Vec2{ 1, 0.75 };
		parentConstraint.maxWidth = 800.0;
		parentConstraint.sizeDeltaPivot = noco::Anchor::MiddleCenter;
		parent->setConstraint(parentConstraint);
		
		// 子ノード（フルストレッチ、最大サイズあり）
		noco::AnchorConstraint childConstraint;
		childConstraint.anchorMin = Vec2{ 0, 0 };
		childConstraint.anchorMax = Vec2{ 1, 1 };
		childConstraint.maxWidth = 600.0;
		childConstraint.maxHeight = 300.0;
		childConstraint.sizeDeltaPivot = noco::Anchor::MiddleCenter;
		child->setConstraint(childConstraint);
		
		grandParent->addChild(parent);
		parent->addChild(child);
		
		// 計算をシミュレート
		RectF rootRect{ 0, 0, 1200, 900 };
		RectF parentRect = parentConstraint.applyConstraint(rootRect, Vec2::Zero());
		
		// 親の幅は最大800に制限、高さは50%（450）
		REQUIRE(parentRect.w == 800.0);
		REQUIRE(parentRect.h == 450.0);
		// 中央配置の確認
		REQUIRE(parentRect.center().x == 600.0);
		
		// 子の計算
		RectF childRect = childConstraint.applyConstraint(parentRect, Vec2::Zero());
		
		// 子の幅は最大600に制限、高さは最大300に制限
		REQUIRE(childRect.w == 600.0);
		REQUIRE(childRect.h == 300.0);
		// 親の中央に配置される
		REQUIRE(childRect.center() == parentRect.center());
	}
	
	SECTION("Mixed constraint types in layout")
	{
		auto parent = noco::Node::Create();
		auto child1 = noco::Node::Create();
		auto child2 = noco::Node::Create();
		auto child3 = noco::Node::Create();
		
		// 親に水平レイアウトを設定
		noco::HorizontalLayout layout;
		layout.spacing = 10;
		parent->setBoxChildrenLayout(layout);
		
		// 親のサイズ（AnchorConstraintでストレッチ）
		noco::AnchorConstraint parentConstraint;
		parentConstraint.anchorMin = Vec2{ 0, 0 };
		parentConstraint.anchorMax = Vec2{ 1, 0 };
		parentConstraint.sizeDelta = Vec2{ 0, 200 };
		parentConstraint.maxWidth = 1000.0;
		parent->setConstraint(parentConstraint);
		
		// 子1: 固定サイズ
		noco::BoxConstraint child1Constraint;
		child1Constraint.sizeDelta = Vec2{ 100, 0 };
		child1Constraint.sizeRatio = Vec2{ 0, 1 };
		child1->setConstraint(child1Constraint);
		
		// 子2: フレキシブル、最大幅あり
		noco::BoxConstraint child2Constraint;
		child2Constraint.flexibleWeight = 1.0;
		child2Constraint.sizeRatio = Vec2{ 0, 1 };
		child2Constraint.maxWidth = 300.0;
		child2->setConstraint(child2Constraint);
		
		// 子3: フレキシブル、最大幅なし
		noco::BoxConstraint child3Constraint;
		child3Constraint.flexibleWeight = 1.0;
		child3Constraint.sizeRatio = Vec2{ 0, 1 };
		child3->setConstraint(child3Constraint);
		
		parent->addChild(child1);
		parent->addChild(child2);
		parent->addChild(child3);
		
		// 検証
		REQUIRE(child1->boxConstraint()->sizeDelta.x == 100.0);
		REQUIRE(child2->boxConstraint()->maxWidth.has_value());
		REQUIRE(*child2->boxConstraint()->maxWidth == 300.0);
		REQUIRE(!child3->boxConstraint()->maxWidth.has_value());
	}
}