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