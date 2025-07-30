# include <catch2/catch.hpp>
# include <Siv3D.hpp>
# include <NocoUI.hpp>

// ========================================
// TransformEffectのHitTestテスト
// ========================================

TEST_CASE("TransformEffect HitTest with Parent-Child Hierarchy", "[Node][HitTest][TransformEffect]")
{
	SECTION("Parent with appliesToHitTest=false should not affect child's hit test")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create(U"parent");
		auto child = noco::Node::Create(U"child");
		
		parent->setConstraint(noco::BoxConstraint{ .sizeDelta = Vec2{ 200, 200 } });
		child->setConstraint(noco::BoxConstraint{ .sizeDelta = Vec2{ 100, 100 } });
		
		// 親にTransformEffectを適用（appliesToHitTest=false）
		parent->transformEffect().setPosition(Vec2{ 100, 100 });
		parent->transformEffect().setAppliesToHitTest(false);
		
		canvas->rootNode()->addChild(parent);
		parent->addChild(child);
		canvas->update();
		
		// 動作：
		// - 親は元の位置（0,0-200,200）でヒット判定
		// - 子も親のappliesToHitTest=falseの影響で元の位置（0,0-100,100）でヒット判定
		// - 描画は変換後の位置で行われる
		
		// 親の元の位置（50,50）では子がヒット（子が最前面）
		auto hitParent1 = canvas->rootNode()->hitTest(Vec2{ 50, 50 });
		REQUIRE(hitParent1 == child);
		
		// 親の範囲内で子の範囲外（150,150）では親がヒット
		auto hitChild = canvas->rootNode()->hitTest(Vec2{ 150, 150 });
		REQUIRE(hitChild == parent);
		
		// 親の変換後の描画位置の外（250,250）ではヒットしない
		auto miss = canvas->rootNode()->hitTest(Vec2{ 250, 250 });
		REQUIRE(miss == nullptr);
	}

	SECTION("Parent with appliesToHitTest=true should affect child's hit test")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create(U"parent");
		auto child = noco::Node::Create(U"child");
		
		parent->setConstraint(noco::BoxConstraint{ .sizeDelta = Vec2{ 200, 200 } });
		child->setConstraint(noco::BoxConstraint{ .sizeDelta = Vec2{ 100, 100 } });
		
		// 親にTransformEffectを適用（appliesToHitTest=true）
		parent->transformEffect().setPosition(Vec2{ 100, 100 });
		parent->transformEffect().setAppliesToHitTest(true);
		
		canvas->rootNode()->addChild(parent);
		parent->addChild(child);
		canvas->update();
		
		// 子ノードは親の変換後の位置（100, 100）を基準にヒット
		auto hitChild = canvas->rootNode()->hitTest(Vec2{ 150, 150 });
		REQUIRE(hitChild == child);
		
		// 元の位置では子はヒットしない
		auto missChild = canvas->rootNode()->hitTest(Vec2{ 50, 50 });
		REQUIRE(missChild == nullptr);
	}

	SECTION("Multiple levels of hierarchy with mixed appliesToHitTest settings")
	{
		auto canvas = noco::Canvas::Create();
		auto grandparent = noco::Node::Create(U"grandparent");
		auto parent = noco::Node::Create(U"parent");
		auto child = noco::Node::Create(U"child");
		
		grandparent->setConstraint(noco::BoxConstraint{ .sizeDelta = Vec2{ 300, 300 } });
		parent->setConstraint(noco::BoxConstraint{ .sizeDelta = Vec2{ 200, 200 } });
		child->setConstraint(noco::BoxConstraint{ .sizeDelta = Vec2{ 100, 100 } });
		
		// 祖父母: appliesToHitTest=true
		grandparent->transformEffect().setPosition(Vec2{ 50, 50 });
		grandparent->transformEffect().setAppliesToHitTest(true);
		
		// 親: appliesToHitTest=false
		parent->transformEffect().setPosition(Vec2{ 50, 50 });
		parent->transformEffect().setAppliesToHitTest(false);
		
		canvas->rootNode()->addChild(grandparent);
		grandparent->addChild(parent);
		parent->addChild(child);
		canvas->update();
		
		// 動作：
		// - 祖父母: appliesToHitTest=trueなので変換後の位置（50,50-350,350）で判定
		// - 親: appliesToHitTest=falseなので、祖父母の変換を受けた位置(50,50)から
		//      自身の変換を適用しない位置（50,50-250,250）で判定
		// - 子: 親のappliesToHitTest=falseの影響で、親と同じ位置（50,50-150,150）で判定
		
		// 子の位置（100,100）では子がヒット
		auto hitChild = canvas->rootNode()->hitTest(Vec2{ 100, 100 });
		REQUIRE(hitChild == child);
		
		// 親の範囲内で子の範囲外（200,200）では親がヒット
		auto hitAt200 = canvas->rootNode()->hitTest(Vec2{ 200, 200 });
		REQUIRE(hitAt200 == parent);
	}

	SECTION("Scale transform with appliesToHitTest affecting children")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create(U"parent");
		auto child = noco::Node::Create(U"child");
		
		parent->setConstraint(noco::BoxConstraint{ .sizeDelta = Vec2{ 200, 200 } });
		child->setConstraint(noco::BoxConstraint{ .sizeDelta = Vec2{ 50, 50 }, .margin = noco::LRTB{ 25, 25, 25, 25 } });
		
		// 親に2倍スケールを適用
		parent->transformEffect().setScale(Vec2{ 2.0, 2.0 });
		parent->transformEffect().setPivot(noco::Anchor::MiddleCenter);
		
		canvas->rootNode()->addChild(parent);
		parent->addChild(child);
		
		// appliesToHitTest=false（子はスケールの影響を受けない）
		parent->transformEffect().setAppliesToHitTest(false);
		canvas->update();
		
		// appliesToHitTest=false: 親は元の位置(0,0-200,200)でヒット判定
		// 子も親のappliesToHitTest=falseの影響で元の位置で判定
		// 子は元の位置(25,25-75,75)でヒット判定
		auto hitParent = canvas->rootNode()->hitTest(Vec2{ 50, 50 });
		REQUIRE(hitParent == child);
		
		// appliesToHitTest=true（子もスケールの影響を受ける）
		parent->transformEffect().setAppliesToHitTest(true);
		canvas->update();
		
		// 親が中心から2倍になるので、親は-100,-100から300,300の範囲
		// 子も2倍になり、位置も調整される
		auto hitScaled = canvas->rootNode()->hitTest(Vec2{ 0, 0 });
		REQUIRE(hitScaled == child);
	}

	SECTION("Rotation transform with appliesToHitTest")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create(U"parent");
		auto child = noco::Node::Create(U"child");
		
		parent->setConstraint(noco::BoxConstraint{ .sizeDelta = Vec2{ 200, 200 } });
		child->setConstraint(noco::BoxConstraint{ .sizeDelta = Vec2{ 50, 100 }, .margin = noco::LRTB{ 75, 75, 0, 100 } });
		
		canvas->rootNode()->addChild(parent);
		parent->addChild(child);
		
		// 初期状態でのヒットテスト
		canvas->update();
		auto hitBefore = canvas->rootNode()->hitTest(Vec2{ 100, 25 });
		REQUIRE(hitBefore == child);
		
		// 親に90度回転を適用（appliesToHitTest=true）
		parent->transformEffect().setRotation(90.0);
		parent->transformEffect().setPivot(noco::Anchor::MiddleCenter);
		parent->transformEffect().setAppliesToHitTest(true);
		canvas->update();
		
		// 変換後の位置でヒットテスト
		auto hitAfter = canvas->rootNode()->hitTest(Vec2{ 100, 25 });
		REQUIRE(hitAfter != child); // 元の位置ではヒットしない
	}

	SECTION("Child with own TransformEffect when parent has appliesToHitTest=false")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create(U"parent");
		auto child = noco::Node::Create(U"child");
		
		parent->setConstraint(noco::BoxConstraint{ .sizeDelta = Vec2{ 200, 200 } });
		child->setConstraint(noco::BoxConstraint{ .sizeDelta = Vec2{ 100, 100 } });
		
		// 親: 大きく移動するがappliesToHitTest=false
		parent->transformEffect().setPosition(Vec2{ 1000, 1000 });
		parent->transformEffect().setAppliesToHitTest(false);
		
		// 子: 小さく移動してappliesToHitTest=true
		child->transformEffect().setPosition(Vec2{ 50, 50 });
		child->transformEffect().setAppliesToHitTest(true);
		
		canvas->rootNode()->addChild(parent);
		parent->addChild(child);
		canvas->update();
		
		// 親のappliesToHitTest=falseの影響で、子は元の位置(0,0)から
		// 自身のTransformEffect(50,50)が適用される
		// つまり、子のHitTest位置は(50,50-150,150)
		auto hitChild = canvas->rootNode()->hitTest(Vec2{ 100, 100 });
		REQUIRE(hitChild == child);
		
		// 親の元の位置(0,0-200,200)で子の範囲外では親がヒット
		auto hitParent = canvas->rootNode()->hitTest(Vec2{ 180, 180 });
		REQUIRE(hitParent == parent);
	}


	SECTION("Dynamic toggle of appliesToHitTest")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create(U"parent");
		auto child = noco::Node::Create(U"child");
		
		parent->setConstraint(noco::BoxConstraint{ .sizeDelta = Vec2{ 200, 200 } });
		child->setConstraint(noco::BoxConstraint{ .sizeDelta = Vec2{ 100, 100 } });
		
		parent->transformEffect().setPosition(Vec2{ 100, 100 });
		parent->transformEffect().setAppliesToHitTest(false);
		
		canvas->rootNode()->addChild(parent);
		parent->addChild(child);
		canvas->update();
		
		// 初期状態: appliesToHitTest=false
		// 親は元の位置(0,0-200,200)、子も元の位置(0,0-100,100)でヒット判定
		auto hit1 = canvas->rootNode()->hitTest(Vec2{ 50, 50 });
		REQUIRE(hit1 == child);
		
		auto hit1b = canvas->rootNode()->hitTest(Vec2{ 150, 150 });
		REQUIRE(hit1b == parent);
		
		// appliesToHitTestを動的に変更
		parent->transformEffect().setAppliesToHitTest(true);
		canvas->update();
		
		// 変更後: appliesToHitTest=true
		// 親も子も変換後の位置でヒット判定
		auto hit2 = canvas->rootNode()->hitTest(Vec2{ 50, 50 });
		REQUIRE(hit2 == nullptr);
		
		auto hit3 = canvas->rootNode()->hitTest(Vec2{ 150, 150 });
		REQUIRE(hit3 == child);
		
		// 再度変更
		parent->transformEffect().setAppliesToHitTest(false);
		canvas->update();
		
		auto hit4 = canvas->rootNode()->hitTest(Vec2{ 50, 50 });
		REQUIRE(hit4 == child);
		
		auto hit4b = canvas->rootNode()->hitTest(Vec2{ 150, 150 });
		REQUIRE(hit4b == parent);
	}

	SECTION("Update method should properly propagate hit test matrix")
	{
		auto canvas = noco::Canvas::Create();
		auto grandparent = noco::Node::Create(U"grandparent");
		auto parent = noco::Node::Create(U"parent");
		auto child = noco::Node::Create(U"child");
		
		grandparent->setConstraint(noco::BoxConstraint{ .sizeDelta = Vec2{ 400, 400 } });
		parent->setConstraint(noco::BoxConstraint{ .sizeDelta = Vec2{ 200, 200 } });
		child->setConstraint(noco::BoxConstraint{ .sizeDelta = Vec2{ 100, 100 } });
		
		// 祖父母: TransformEffectあり、appliesToHitTest=false
		grandparent->transformEffect().setPosition(Vec2{ 100, 100 });
		grandparent->transformEffect().setAppliesToHitTest(false);
		
		// 親: TransformEffectなし
		// 子: TransformEffectなし
		
		canvas->rootNode()->addChild(grandparent);
		grandparent->addChild(parent);
		parent->addChild(child);
		canvas->update();
		
		// 期待される動作:
		// - 祖父母: appliesToHitTest=falseなので、元の位置(0,0-400,400)でヒット判定
		// - 親: 祖父母のappliesToHitTest=falseの影響を受けず、元の位置(0,0-200,200)でヒット判定
		// - 子: 同様に元の位置(0,0-100,100)でヒット判定
		
		
		// 祖父母の元の位置でヒット (子が最前面)
		auto hitGrandparent = canvas->rootNode()->hitTest(Vec2{ 50, 50 });
		REQUIRE(hitGrandparent == child); // 最前面の子がヒット
		
		// 親の位置でヒット
		auto hitParent = canvas->rootNode()->hitTest(Vec2{ 150, 150 });
		REQUIRE(hitParent == parent);
		
		// 祖父母の元の位置で親と重なる位置
		auto hitTransformed = canvas->rootNode()->hitTest(Vec2{ 100, 100 });
		REQUIRE(hitTransformed == parent);
		
		// 祖父母の元の位置の範囲内(350,350)
		auto hitOutside = canvas->rootNode()->hitTest(Vec2{ 350, 350 });
		REQUIRE(hitOutside == grandparent);
	}
}