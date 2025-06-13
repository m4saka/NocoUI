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

TEST_CASE("Min/Max size constraints", "[Constraint][AnchorConstraint][BoxConstraint]")
{
	SECTION("AnchorConstraint with min/max size constraints")
	{
		// min/maxサイズを両方設定した場合のテスト
		noco::AnchorConstraint constraint;
		constraint.anchorMin = Vec2{ 0, 0 };
		constraint.anchorMax = Vec2{ 1, 1 };
		constraint.minWidth = 200.0;
		constraint.minHeight = 150.0;
		constraint.maxWidth = 600.0;
		constraint.maxHeight = 400.0;
		constraint.sizeDeltaPivot = noco::Anchor::MiddleCenter;
		
		// 親領域が小さい場合、最小サイズが適用される
		RectF smallParentRect{ 0, 0, 100, 80 };
		RectF smallResult = constraint.applyConstraint(smallParentRect, Vec2::Zero());
		REQUIRE(smallResult.w == 200.0);
		REQUIRE(smallResult.h == 150.0);
		// 中央配置の確認（親より大きくなるが中央に配置）
		REQUIRE(smallResult.center().x == 50.0);
		REQUIRE(smallResult.center().y == 40.0);
		
		// 親領域が大きい場合、最大サイズが適用される
		RectF largeParentRect{ 0, 0, 1000, 800 };
		RectF largeResult = constraint.applyConstraint(largeParentRect, Vec2::Zero());
		REQUIRE(largeResult.w == 600.0);
		REQUIRE(largeResult.h == 400.0);
		// 中央配置の確認
		REQUIRE(largeResult.center().x == 500.0);
		REQUIRE(largeResult.center().y == 400.0);
		
		// 親領域がmin/maxの間の場合、親のサイズに従う
		RectF mediumParentRect{ 0, 0, 400, 300 };
		RectF mediumResult = constraint.applyConstraint(mediumParentRect, Vec2::Zero());
		REQUIRE(mediumResult.w == 400.0);
		REQUIRE(mediumResult.h == 300.0);
		REQUIRE(mediumResult.center().x == 200.0);
		REQUIRE(mediumResult.center().y == 150.0);
	}

	SECTION("AnchorConstraint with invalid min/max values")
	{
		// minがmaxより大きい場合のテスト（minが適用された後maxで制限される）
		noco::AnchorConstraint constraint;
		constraint.anchorMin = Vec2{ 0, 0 };
		constraint.anchorMax = Vec2{ 1, 1 };
		constraint.minWidth = 600.0;
		constraint.minHeight = 400.0;
		constraint.maxWidth = 300.0;  // minより小さい値
		constraint.maxHeight = 200.0; // minより小さい値
		constraint.sizeDeltaPivot = noco::Anchor::MiddleCenter;
		
		RectF parentRect{ 0, 0, 1000, 800 };
		RectF result = constraint.applyConstraint(parentRect, Vec2::Zero());
		// minが適用され、その後maxで制限されるため、maxの値になる
		REQUIRE(result.w == 300.0);
		REQUIRE(result.h == 200.0);
	}

	SECTION("BoxConstraint with min/max size constraints")
	{
		// BoxConstraintでmin/maxサイズを両方設定
		noco::BoxConstraint constraint;
		constraint.sizeRatio = Vec2{ 1.0, 1.0 };
		constraint.minWidth = 150.0;
		constraint.minHeight = 100.0;
		constraint.maxWidth = 500.0;
		constraint.maxHeight = 350.0;
		
		// 親領域が小さい場合、最小サイズが適用される
		RectF smallParentRect{ 0, 0, 80, 60 };
		RectF smallResult = constraint.applyConstraint(smallParentRect, Vec2::Zero());
		REQUIRE(smallResult.w == 150.0);
		REQUIRE(smallResult.h == 100.0);
		
		// 親領域が大きい場合、最大サイズが適用される
		RectF largeParentRect{ 0, 0, 800, 600 };
		RectF largeResult = constraint.applyConstraint(largeParentRect, Vec2::Zero());
		REQUIRE(largeResult.w == 500.0);
		REQUIRE(largeResult.h == 350.0);
		
		// 親領域がmin/maxの間の場合、親のサイズに従う
		RectF mediumParentRect{ 0, 0, 300, 200 };
		RectF mediumResult = constraint.applyConstraint(mediumParentRect, Vec2::Zero());
		REQUIRE(mediumResult.w == 300.0);
		REQUIRE(mediumResult.h == 200.0);
	}

	SECTION("BoxConstraint with conflicting min/max values")
	{
		// minがmaxより大きい場合のテスト
		noco::BoxConstraint constraint;
		constraint.sizeDelta = Vec2{ 400, 300 };
		constraint.minWidth = 500.0;
		constraint.minHeight = 350.0;
		constraint.maxWidth = 300.0;  // minより小さい値
		constraint.maxHeight = 200.0; // minより小さい値
		
		RectF parentRect{ 0, 0, 1000, 800 };
		RectF result = constraint.applyConstraint(parentRect, Vec2::Zero());
		// minが適用され、その後maxで制限されるため、maxの値になる
		REQUIRE(result.w == 300.0);
		REQUIRE(result.h == 200.0);
	}

	SECTION("Mixed constraints with min/max in hierarchy")
	{
		// 階層内でmin/max制約が混在する場合
		noco::AnchorConstraint parentConstraint;
		parentConstraint.anchorMin = Vec2{ 0, 0 };
		parentConstraint.anchorMax = Vec2{ 1, 1 };
		parentConstraint.minWidth = 400.0;
		parentConstraint.maxWidth = 800.0;
		
		noco::BoxConstraint childConstraint;
		childConstraint.sizeRatio = Vec2{ 0.75, 0.75 };
		childConstraint.minWidth = 200.0;
		childConstraint.maxWidth = 500.0;
		
		// 大きな親領域での計算
		RectF grandParentRect{ 0, 0, 1000, 600 };
		RectF parentRect = parentConstraint.applyConstraint(grandParentRect, Vec2::Zero());
		
		// 親は最大サイズで制限される
		REQUIRE(parentRect.w == 800.0);
		REQUIRE(parentRect.h == 600.0);
		
		// 子は親の75%を要求するが、自身のmax制限で500に制限される
		RectF childRect = childConstraint.applyConstraint(parentRect, Vec2::Zero());
		REQUIRE(childRect.w == 500.0); // 親の75% = 600だが、maxで500に制限
		REQUIRE(childRect.h == 450.0); // 親の75% = 450（制限なし）
	}
}

TEST_CASE("Min/Max size constraints with TransformEffect", "[Constraint][AnchorConstraint][BoxConstraint][TransformEffect]")
{
	SECTION("AnchorConstraint min/max independent of TransformEffect scale")
	{
		// TransformEffectのscaleが制約サイズに影響しないことを確認
		auto node = noco::Node::Create();
		
		noco::AnchorConstraint constraint;
		constraint.anchorMin = Vec2{ 0, 0 };
		constraint.anchorMax = Vec2{ 1, 1 };
		constraint.minWidth = 100.0;
		constraint.minHeight = 80.0;
		constraint.maxWidth = 300.0;
		constraint.maxHeight = 200.0;
		constraint.sizeDeltaPivot = noco::Anchor::MiddleCenter;
		
		node->setConstraint(constraint);
		
		// TransformEffectでスケールを2倍に設定
		node->transformEffect().setScale(Vec2{ 2.0, 2.0 });
		
		// 制約計算はTransformEffectの影響を受けない
		RectF parentRect{ 0, 0, 50, 40 }; // 小さい親領域
		RectF result = constraint.applyConstraint(parentRect, Vec2::Zero());
		
		// minサイズが適用される（TransformEffectのscaleは無関係）
		REQUIRE(result.w == 100.0);
		REQUIRE(result.h == 80.0);
		
		// 大きい親領域でのテスト
		RectF largeParentRect{ 0, 0, 500, 400 };
		RectF largeResult = constraint.applyConstraint(largeParentRect, Vec2::Zero());
		
		// maxサイズが適用される（TransformEffectのscaleは無関係）
		REQUIRE(largeResult.w == 300.0);
		REQUIRE(largeResult.h == 200.0);
	}

	SECTION("BoxConstraint min/max independent of TransformEffect scale")
	{
		// BoxConstraintでもTransformEffectのscaleが制約サイズに影響しないことを確認
		auto node = noco::Node::Create();
		
		noco::BoxConstraint constraint;
		constraint.sizeRatio = Vec2{ 1.0, 1.0 };
		constraint.minWidth = 120.0;
		constraint.minHeight = 90.0;
		constraint.maxWidth = 400.0;
		constraint.maxHeight = 300.0;
		
		node->setConstraint(constraint);
		
		// TransformEffectでスケールを0.5倍に設定
		node->transformEffect().setScale(Vec2{ 0.5, 0.5 });
		
		// 制約計算はTransformEffectの影響を受けない
		RectF parentRect{ 0, 0, 80, 60 }; // 小さい親領域
		RectF result = constraint.applyConstraint(parentRect, Vec2::Zero());
		
		// minサイズが適用される（TransformEffectのscaleは無関係）
		REQUIRE(result.w == 120.0);
		REQUIRE(result.h == 90.0);
		
		// 大きい親領域でのテスト
		RectF largeParentRect{ 0, 0, 600, 500 };
		RectF largeResult = constraint.applyConstraint(largeParentRect, Vec2::Zero());
		
		// maxサイズが適用される（TransformEffectのscaleは無関係）
		REQUIRE(largeResult.w == 400.0);
		REQUIRE(largeResult.h == 300.0);
	}

	SECTION("Parent and child with different TransformEffect scales")
	{
		// 親と子で異なるTransformEffectが設定されている場合のテスト
		auto parent = noco::Node::Create();
		auto child = noco::Node::Create();
		
		// 親ノードの設定
		noco::AnchorConstraint parentConstraint;
		parentConstraint.anchorMin = Vec2{ 0, 0 };
		parentConstraint.anchorMax = Vec2{ 1, 1 };
		parentConstraint.maxWidth = 500.0;
		parentConstraint.maxHeight = 400.0;
		parent->setConstraint(parentConstraint);
		
		// 親のTransformEffect
		parent->transformEffect().setScale(Vec2{ 1.5, 1.5 });
		
		// 子ノードの設定
		noco::BoxConstraint childConstraint;
		childConstraint.sizeRatio = Vec2{ 0.8, 0.8 };
		childConstraint.minWidth = 150.0;
		childConstraint.minHeight = 120.0;
		child->setConstraint(childConstraint);
		
		// 子のTransformEffect
		child->transformEffect().setScale(Vec2{ 0.8, 0.8 });
		
		parent->addChild(child);
		
		// 制約計算の確認
		RectF grandParentRect{ 0, 0, 800, 600 };
		RectF parentRect = parentConstraint.applyConstraint(grandParentRect, Vec2::Zero());
		
		// 親のサイズはmaxで制限される（親のTransformEffectのscaleは無関係）
		REQUIRE(parentRect.w == 500.0);
		REQUIRE(parentRect.h == 400.0);
		
		// 子のサイズ計算
		RectF childRect = childConstraint.applyConstraint(parentRect, Vec2::Zero());
		
		// 子は親の80% = (400, 320)を要求、minサイズ(150, 120)より大きいのでそのまま
		// TransformEffectのscaleは制約計算に影響しない
		REQUIRE(childRect.w == 400.0);
		REQUIRE(childRect.h == 320.0);
	}

	SECTION("Complex hierarchy with multiple scales and constraints")
	{
		// 複雑な階層でのTransformEffectとmin/max制約の独立性を確認
		auto grandParent = noco::Node::Create();
		auto parent = noco::Node::Create();
		auto child = noco::Node::Create();
		
		// 祖父母ノード
		noco::BoxConstraint grandParentConstraint;
		grandParentConstraint.sizeDelta = Vec2{ 1000, 800 };
		grandParent->setConstraint(grandParentConstraint);
		
		grandParent->transformEffect().setScale(Vec2{ 2.0, 2.0 });
		
		// 親ノード
		noco::AnchorConstraint parentConstraint;
		parentConstraint.anchorMin = Vec2{ 0.1, 0.1 };
		parentConstraint.anchorMax = Vec2{ 0.9, 0.9 };
		parentConstraint.minWidth = 200.0;
		parentConstraint.maxWidth = 600.0;
		parent->setConstraint(parentConstraint);
		
		parent->transformEffect().setScale(Vec2{ 0.7, 0.7 });
		
		// 子ノード
		noco::BoxConstraint childConstraint;
		childConstraint.sizeRatio = Vec2{ 1.2, 1.2 }; // 親より大きく要求
		childConstraint.maxWidth = 500.0;
		childConstraint.maxHeight = 400.0;
		child->setConstraint(childConstraint);
		
		child->transformEffect().setScale(Vec2{ 3.0, 3.0 });
		
		grandParent->addChild(parent);
		parent->addChild(child);
		
		// 制約計算の確認
		RectF rootRect{ 0, 0, 1000, 800 };
		
		// 親の制約計算（祖父母のTransformEffectは影響しない）
		RectF parentRect = parentConstraint.applyConstraint(rootRect, Vec2::Zero());
		
		// 親のサイズ = 祖父母の80%サイズの領域 = (800, 640)
		// minWidth(200)より大きく、maxWidth(600)で制限される
		REQUIRE(parentRect.w == 600.0);
		REQUIRE(parentRect.h == 640.0);
		
		// 子の制約計算（親のTransformEffectは影響しない）
		RectF childRect = childConstraint.applyConstraint(parentRect, Vec2::Zero());
		
		// 子のサイズ = 親の120% = (720, 768)を要求
		// maxWidth(500), maxHeight(400)で制限される
		REQUIRE(childRect.w == 500.0);
		REQUIRE(childRect.h == 400.0);
	}
}