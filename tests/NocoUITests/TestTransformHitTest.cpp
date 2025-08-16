# include <catch2/catch.hpp>
# include <Siv3D.hpp>
# include <NocoUI.hpp>

// ========================================
// TransformのHitTestテスト
// ========================================

TEST_CASE("Transform HitTest with Parent-Child Hierarchy", "[Node][HitTest][Transform]")
{
	SECTION("Parent with appliesToHitTest=false should not affect child's hit test")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create(U"parent");
		auto child = noco::Node::Create(U"child");
		
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 200, 200 } });
		child->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 100 } });
		
		// 親にTransformを適用（appliesToHitTest=false）
		parent->transform().setTranslate(Vec2{ 100, 100 });
		parent->transform().setAppliesToHitTest(false);
		
		canvas->addChild(parent);
		parent->addChild(child);
		canvas->update();
		
		// 動作：
		// - 親は元の位置（0,0-200,200）でヒット判定
		// - 子も親のappliesToHitTest=falseの影響で元の位置（0,0-100,100）でヒット判定
		// - 描画は変換後の位置で行われる
		
		// 親の元の位置（50,50）では子がヒット（子が最前面）
		auto hitParent1 = canvas->hitTest(Vec2{ 50, 50 });
		REQUIRE(hitParent1 == child);
		
		// 親の範囲内で子の範囲外（150,150）では親がヒット
		auto hitChild = canvas->hitTest(Vec2{ 150, 150 });
		REQUIRE(hitChild == parent);
		
		// 親の変換後の描画位置の外（250,250）ではヒットしない
		auto miss = canvas->hitTest(Vec2{ 250, 250 });
		REQUIRE(miss == nullptr);
	}

	SECTION("Parent with appliesToHitTest=true should affect child's hit test")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create(U"parent");
		auto child = noco::Node::Create(U"child");
		
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 200, 200 } });
		child->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 100 } });
		
		// 親にTransformを適用（appliesToHitTest=true）
		parent->transform().setTranslate(Vec2{ 100, 100 });
		parent->transform().setAppliesToHitTest(true);
		
		canvas->addChild(parent);
		parent->addChild(child);
		canvas->update();
		
		// 子ノードは親の変換後の位置（100, 100）を基準にヒット
		auto hitChild = canvas->hitTest(Vec2{ 150, 150 });
		REQUIRE(hitChild == child);
		
		// 元の位置では子はヒットしない
		auto missChild = canvas->hitTest(Vec2{ 50, 50 });
		REQUIRE(missChild == nullptr);
	}

	SECTION("Multiple levels of hierarchy with mixed appliesToHitTest settings")
	{
		auto canvas = noco::Canvas::Create();
		auto grandparent = noco::Node::Create(U"grandparent");
		auto parent = noco::Node::Create(U"parent");
		auto child = noco::Node::Create(U"child");
		
		grandparent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 300, 300 } });
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 200, 200 } });
		child->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 100 } });
		
		// 祖父母: appliesToHitTest=true
		grandparent->transform().setTranslate(Vec2{ 50, 50 });
		grandparent->transform().setAppliesToHitTest(true);
		
		// 親: appliesToHitTest=false
		parent->transform().setTranslate(Vec2{ 50, 50 });
		parent->transform().setAppliesToHitTest(false);
		
		canvas->addChild(grandparent);
		grandparent->addChild(parent);
		parent->addChild(child);
		canvas->update();
		
		// 動作：
		// - 祖父母: appliesToHitTest=trueなので変換後の位置（50,50-350,350）で判定
		// - 親: appliesToHitTest=falseなので、祖父母の変換を受けた位置(50,50)から
		//      自身の変換を適用しない位置（50,50-250,250）で判定
		// - 子: 親のappliesToHitTest=falseの影響で、親と同じ位置（50,50-150,150）で判定
		
		// 子の位置（100,100）では子がヒット
		auto hitChild = canvas->hitTest(Vec2{ 100, 100 });
		REQUIRE(hitChild == child);
		
		// 親の範囲内で子の範囲外（200,200）では親がヒット
		auto hitAt200 = canvas->hitTest(Vec2{ 200, 200 });
		REQUIRE(hitAt200 == parent);
	}

	SECTION("Scale transform with appliesToHitTest affecting children")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create(U"parent");
		auto child = noco::Node::Create(U"child");
		
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 200, 200 } });
		child->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 50, 50 }, .margin = noco::LRTB{ 25, 25, 25, 25 } });
		
		// 親に2倍スケールを適用
		parent->transform().setScale(Vec2{ 2.0, 2.0 });
		parent->transform().setPivot(noco::Anchor::MiddleCenter);
		
		canvas->addChild(parent);
		parent->addChild(child);
		
		// appliesToHitTest=false（子はスケールの影響を受けない）
		parent->transform().setAppliesToHitTest(false);
		canvas->update();
		
		// appliesToHitTest=false: 親は元の位置(0,0-200,200)でヒット判定
		// 子も親のappliesToHitTest=falseの影響で元の位置で判定
		// 子は元の位置(25,25-75,75)でヒット判定
		auto hitParent = canvas->hitTest(Vec2{ 50, 50 });
		REQUIRE(hitParent == child);
		
		// appliesToHitTest=true（子もスケールの影響を受ける）
		parent->transform().setAppliesToHitTest(true);
		canvas->update();
		
		// 親が中心から2倍になるので、親は-100,-100から300,300の範囲
		// 子も2倍になり、位置も調整される
		auto hitScaled = canvas->hitTest(Vec2{ 0, 0 });
		REQUIRE(hitScaled == child);
	}

	SECTION("Rotation transform with appliesToHitTest")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create(U"parent");
		auto child = noco::Node::Create(U"child");
		
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 200, 200 } });
		child->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 50, 100 }, .margin = noco::LRTB{ 75, 75, 0, 100 } });
		
		canvas->addChild(parent);
		parent->addChild(child);
		
		// 初期状態でのヒットテスト
		canvas->update();
		auto hitBefore = canvas->hitTest(Vec2{ 100, 25 });
		REQUIRE(hitBefore == child);
		
		// 親に90度回転を適用（appliesToHitTest=true）
		parent->transform().setRotation(90.0);
		parent->transform().setPivot(noco::Anchor::MiddleCenter);
		parent->transform().setAppliesToHitTest(true);
		canvas->update();
		
		// 変換後の位置でヒットテスト
		auto hitAfter = canvas->hitTest(Vec2{ 100, 25 });
		REQUIRE(hitAfter != child); // 元の位置ではヒットしない
	}

	SECTION("Child with own Transform when parent has appliesToHitTest=false")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create(U"parent");
		auto child = noco::Node::Create(U"child");
		
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 200, 200 } });
		child->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 100 } });
		
		// 親: 大きく移動するがappliesToHitTest=false
		parent->transform().setTranslate(Vec2{ 1000, 1000 });
		parent->transform().setAppliesToHitTest(false);
		
		// 子: 小さく移動してappliesToHitTest=true
		child->transform().setTranslate(Vec2{ 50, 50 });
		child->transform().setAppliesToHitTest(true);
		
		canvas->addChild(parent);
		parent->addChild(child);
		canvas->update();
		
		// 親のappliesToHitTest=falseの影響で、子は元の位置(0,0)から
		// 自身のTransform(50,50)が適用される
		// つまり、子のHitTest位置は(50,50-150,150)
		auto hitChild = canvas->hitTest(Vec2{ 100, 100 });
		REQUIRE(hitChild == child);
		
		// 親の元の位置(0,0-200,200)で子の範囲外では親がヒット
		auto hitParent = canvas->hitTest(Vec2{ 180, 180 });
		REQUIRE(hitParent == parent);
	}


	SECTION("Dynamic toggle of appliesToHitTest")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create(U"parent");
		auto child = noco::Node::Create(U"child");
		
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 200, 200 } });
		child->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 100 } });
		
		parent->transform().setTranslate(Vec2{ 100, 100 });
		parent->transform().setAppliesToHitTest(false);
		
		canvas->addChild(parent);
		parent->addChild(child);
		canvas->update();
		
		// 初期状態: appliesToHitTest=false
		// 親は元の位置(0,0-200,200)、子も元の位置(0,0-100,100)でヒット判定
		auto hit1 = canvas->hitTest(Vec2{ 50, 50 });
		REQUIRE(hit1 == child);
		
		auto hit1b = canvas->hitTest(Vec2{ 150, 150 });
		REQUIRE(hit1b == parent);
		
		// appliesToHitTestを動的に変更
		parent->transform().setAppliesToHitTest(true);
		canvas->update();
		
		// 変更後: appliesToHitTest=true
		// 親も子も変換後の位置でヒット判定
		auto hit2 = canvas->hitTest(Vec2{ 50, 50 });
		REQUIRE(hit2 == nullptr);
		
		auto hit3 = canvas->hitTest(Vec2{ 150, 150 });
		REQUIRE(hit3 == child);
		
		// 再度変更
		parent->transform().setAppliesToHitTest(false);
		canvas->update();
		
		auto hit4 = canvas->hitTest(Vec2{ 50, 50 });
		REQUIRE(hit4 == child);
		
		auto hit4b = canvas->hitTest(Vec2{ 150, 150 });
		REQUIRE(hit4b == parent);
	}

	SECTION("Update method should properly propagate hit test matrix")
	{
		auto canvas = noco::Canvas::Create();
		auto grandparent = noco::Node::Create(U"grandparent");
		auto parent = noco::Node::Create(U"parent");
		auto child = noco::Node::Create(U"child");
		
		grandparent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 400, 400 } });
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 200, 200 } });
		child->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 100 } });
		
		// 祖父母: Transformあり、初期状態でappliesToHitTest=false
		grandparent->transform().setTranslate(Vec2{ 100, 100 });
		grandparent->transform().setAppliesToHitTest(false);
		
		// 親: Transformなし
		// 子: Transformなし
		
		canvas->addChild(grandparent);
		grandparent->addChild(parent);
		parent->addChild(child);
		canvas->update();
		
		// appliesToHitTest=falseの場合の動作確認
		// 期待される動作:
		// - 祖父母: appliesToHitTest=falseなので、元の位置(0,0-400,400)でヒット判定
		// - 親: 祖父母のappliesToHitTest=falseの影響を受けず、元の位置(0,0-200,200)でヒット判定
		// - 子: 同様に元の位置(0,0-100,100)でヒット判定
		
		// 元の位置(50,50)でヒット (子が最前面)
		auto hit1_false = canvas->hitTest(Vec2{ 50, 50 });
		REQUIRE(hit1_false == child);
		
		// 変換後の位置(150,150)でヒットしない（祖父母は(100,100)に移動しているため）
		auto hit2_false = canvas->hitTest(Vec2{ 150, 150 });
		REQUIRE(hit2_false == parent);
		
		// 祖父母の元の位置の範囲内(350,350)
		auto hit3_false = canvas->hitTest(Vec2{ 350, 350 });
		REQUIRE(hit3_false == grandparent);
		
		// 変換後の位置の外(450,450)ではヒットしない
		auto hit4_false = canvas->hitTest(Vec2{ 450, 450 });
		REQUIRE(hit4_false == nullptr);
		
		// appliesToHitTest=trueに変更
		grandparent->transform().setAppliesToHitTest(true);
		canvas->update();
		
		// appliesToHitTest=trueの場合の動作確認
		// 期待される動作:
		// - 祖父母: 変換後の位置(100,100-500,500)でヒット判定
		// - 親: 祖父母の変換の影響を受け、(100,100-300,300)でヒット判定
		// - 子: 同様に(100,100-200,200)でヒット判定
		
		// 元の位置(50,50)ではヒットしない
		auto hit1_true = canvas->hitTest(Vec2{ 50, 50 });
		REQUIRE(hit1_true == nullptr);
		
		// 変換後の位置(150,150)でヒット（子が最前面）
		auto hit2_true = canvas->hitTest(Vec2{ 150, 150 });
		REQUIRE(hit2_true == child);
		
		// 変換後の親の範囲内(250,250)
		auto hit3_true = canvas->hitTest(Vec2{ 250, 250 });
		REQUIRE(hit3_true == parent);
		
		// 変換後の祖父母の範囲内(450,450)
		auto hit4_true = canvas->hitTest(Vec2{ 450, 450 });
		REQUIRE(hit4_true == grandparent);
	}

	SECTION("Rotation affects hit test when appliesToHitTest is true")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create(U"rotatingNode");
		
		// 100x100のノードを(100,100)に配置
		node->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 100 } });
		node->transform().setTranslate(Vec2{ 100, 100 });
		node->transform().setPivot(Vec2{ 0.5, 0.5 }); // 中心を回転軸に
		node->transform().setAppliesToHitTest(true);
		
		canvas->addChild(node);
		
		// テストポイント：変換後の右上角付近(198, 115)
		Vec2 testPoint{ 198, 115 };
		
		// 0度：右上角付近はヒットする
		node->transform().setRotation(0.0);
		canvas->update();
		auto hit0 = canvas->hitTest(testPoint);
		REQUIRE(hit0 == node);
		
		// 10度：まだヒットする
		node->transform().setRotation(10.0);
		canvas->update();
		auto hit10 = canvas->hitTest(testPoint);
		REQUIRE(hit10 == node);
		
		// 15度：まだヒットする
		node->transform().setRotation(15.0);
		canvas->update();
		auto hit15 = canvas->hitTest(testPoint);
		REQUIRE(hit15 == node);
		
		// 20度：まだヒットする
		node->transform().setRotation(20.0);
		canvas->update();
		auto hit20 = canvas->hitTest(testPoint);
		REQUIRE(hit20 == node);
		
		// 21度：まだヒットする（ギリギリ）
		node->transform().setRotation(21.0);
		canvas->update();
		auto hit21 = canvas->hitTest(testPoint);
		REQUIRE(hit21 == node);
		
		// 22度：ヒットしなくなる
		node->transform().setRotation(22.0);
		canvas->update();
		auto hit22 = canvas->hitTest(testPoint);
		REQUIRE(hit22 == nullptr);
		
		// 30度：ヒットしない
		node->transform().setRotation(30.0);
		canvas->update();
		auto hit30 = canvas->hitTest(testPoint);
		REQUIRE(hit30 == nullptr);
		
		// 45度：ヒットしない
		node->transform().setRotation(45.0);
		canvas->update();
		auto hit45 = canvas->hitTest(testPoint);
		REQUIRE(hit45 == nullptr);
	}

	SECTION("Rotation does not affect hit test when appliesToHitTest is false")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create(U"rotatingNode");
		
		// 100x100のノードを(100,100)に配置
		node->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 100 } });
		node->transform().setTranslate(Vec2{ 100, 100 });
		node->transform().setPivot(Vec2{ 0.5, 0.5 }); // 中心を回転軸に
		node->transform().setAppliesToHitTest(false); // ヒット判定に適用しない
		
		canvas->addChild(node);
		
		// テストポイント：元の位置の右上角付近(95, 5)
		Vec2 testPoint{ 95, 5 };
		
		// 0度：右上角はヒットする
		node->transform().setRotation(0.0);
		canvas->update();
		auto hit0 = canvas->hitTest(testPoint);
		REQUIRE(hit0 == node);
		
		// 10度：回転してもヒットする（appliesToHitTest=false）
		node->transform().setRotation(10.0);
		canvas->update();
		auto hit10 = canvas->hitTest(testPoint);
		REQUIRE(hit10 == node);
		
		// 30度：回転してもヒットする
		node->transform().setRotation(30.0);
		canvas->update();
		auto hit30 = canvas->hitTest(testPoint);
		REQUIRE(hit30 == node);
		
		// 45度：回転してもヒットする
		node->transform().setRotation(45.0);
		canvas->update();
		auto hit45 = canvas->hitTest(testPoint);
		REQUIRE(hit45 == node);
		
		// 60度：回転してもヒットする
		node->transform().setRotation(60.0);
		canvas->update();
		auto hit60 = canvas->hitTest(testPoint);
		REQUIRE(hit60 == node);
		
		// 90度：回転してもヒットする（ヒット判定は元の位置のまま）
		node->transform().setRotation(90.0);
		canvas->update();
		auto hit90 = canvas->hitTest(testPoint);
		REQUIRE(hit90 == node);
	}

	SECTION("Parent-child rotation stacking with different pivots")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create(U"parent");
		auto child = noco::Node::Create(U"child");
		
		// 親：200x200、pivot(0,0) - 左上角で回転
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 200, 200 } });
		parent->transform().setTranslate(Vec2{ 50, 50 });
		parent->transform().setPivot(Vec2{ 0, 0 }); // 左上角
		parent->transform().setAppliesToHitTest(true);
		
		// 子：80x80、ローカル位置(50,50)、pivot(1,1) - 右下角で回転
		child->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 80, 80 }, .margin = noco::LRTB{ 50, 70, 50, 70 } });
		child->transform().setPivot(Vec2{ 1, 1 }); // 右下角
		child->transform().setAppliesToHitTest(true);
		
		canvas->addChild(parent);
		parent->addChild(child);
		
		// テストケース1: 両方回転なし
		parent->transform().setRotation(0.0);
		child->transform().setRotation(0.0);
		canvas->update();
		
		// 子の中心付近(140,140)はヒット
		auto hit1 = canvas->hitTest(Vec2{ 140, 140 });
		REQUIRE(hit1 == child);
		
		// 親の範囲内で子の外(85,85)は親がヒット
		auto hit2 = canvas->hitTest(Vec2{ 85, 85 });
		REQUIRE(hit2 == parent);
		
		// テストケース2: 親のみ30度回転
		parent->transform().setRotation(30.0);
		child->transform().setRotation(0.0);
		canvas->update();
		
		// 元の子の位置(140,140)はヒットしない（親の回転で移動）
		auto hit3 = canvas->hitTest(Vec2{ 140, 140 });
		REQUIRE(hit3 != child);
		
		// テストケース3: 子のみ45度回転（右下角中心）
		parent->transform().setRotation(0.0);
		child->transform().setRotation(45.0);
		canvas->update();
		
		Console << U"Child transformedQuad: " << child->transformedQuad();
		Console << U"Child hitTestQuad: " << child->hitTestQuad();
		
		// 子の実際の位置でテスト (回転後の子の中心付近の点)
		auto hit4 = canvas->hitTest(Vec2{ 160, 120 });
		INFO("Hit4 result: " << (hit4 ? hit4->name() : U"nullptr"));
		if (hit4 == parent) {
			INFO("Hit parent instead of child!");
			INFO("Child's appliesToHitTest: " << child->transform().appliesToHitTest().value());
		}
		REQUIRE(hit4 == child);
		
		// テストケース4: 親30度、子45度の組み合わせ
		parent->transform().setRotation(30.0);
		child->transform().setRotation(45.0);
		canvas->update();
		
		// (140,140)は回転後も子がヒット
		auto hit5 = canvas->hitTest(Vec2{ 140, 140 });
		REQUIRE(hit5 == child);
		
		// 親の左上付近(85,85)では親がヒット
		auto hit6 = canvas->hitTest(Vec2{ 85, 85 });
		REQUIRE(hit6 == parent);
	}

	SECTION("Parent-child rotation with appliesToHitTest=false on parent")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create(U"parent");
		auto child = noco::Node::Create(U"child");
		
		// 親：200x200、pivot(0.5,0) - 上辺中央で回転
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 200, 200 } });
		parent->transform().setTranslate(Vec2{ 100, 100 });
		parent->transform().setPivot(Vec2{ 0.5, 0 }); // 上辺中央
		parent->transform().setRotation(45.0);
		parent->transform().setAppliesToHitTest(false); // 親の回転はヒット判定に影響しない
		
		// 子：60x60、pivot(0,0.5) - 左辺中央で回転
		child->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 60, 60 }, .margin = noco::LRTB{ 20, 120, 20, 120 } });
		child->transform().setPivot(Vec2{ 0, 0.5 }); // 左辺中央
		child->transform().setRotation(30.0);
		child->transform().setAppliesToHitTest(true);
		
		canvas->addChild(parent);
		parent->addChild(child);
		canvas->update();
		
		// 親のappliesToHitTest=falseなので、子は元の位置(0,0)基準で自身の回転のみ適用
		// 子の元の位置は(20,20-80,80)、30度回転で左辺中央を中心に回転
		
		Console << U"Parent transformedQuad: " << parent->transformedQuad();
		Console << U"Child transformedQuad: " << child->transformedQuad();
		Console << U"Child quad p0: " << child->hitTestQuad().p0;
		
		// 子は20,20から80,80の矩形で、左辺中央(20,50)を中心に30度回転
		// テストポイント(50,50)は子の回転後の範囲内にある
		auto hit1 = canvas->hitTest(Vec2{ 50, 50 });
		REQUIRE(hit1 == child);
		
		// 親の元の位置(100,100-300,300)の範囲内でヒット
		auto hit2 = canvas->hitTest(Vec2{ 150, 150 });
		REQUIRE(hit2 == parent);
		
		// 親の変換後の描画位置ではヒットしない（appliesToHitTest=false）
		auto hit3 = canvas->hitTest(Vec2{ 350, 200 });
		REQUIRE(hit3 == nullptr);
	}
	
	SECTION("Parent rotation affects child hit test when parent has appliesToHitTest=true")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create(U"parent");
		auto child = noco::Node::Create(U"child");
		
		// 親：200x200、位置(100,100)、中心で45度回転
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 200, 200 } });
		parent->transform().setTranslate(Vec2{ 100, 100 });
		parent->transform().setPivot(Vec2{ 0.5, 0.5 }); // 中心
		parent->transform().setRotation(45.0);
		parent->transform().setAppliesToHitTest(true); // 親の回転がヒット判定に影響する
		
		// 子：100x100、親に対する相対位置(0,0)、appliesToHitTest=false
		child->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 100 } });
		child->transform().setAppliesToHitTest(false); // 子自身の変換は無効
		
		canvas->addChild(parent);
		parent->addChild(child);
		canvas->update();
		
		// 親の中心は(200,200)
		// 子の元の位置(100,100)は、親の回転により移動する
		// 45度回転後の子の左上は約(200,59)になる
		
		// 元の子の位置(100,100)ではヒットしない（回転で移動）
		auto hit1 = canvas->hitTest(Vec2{ 100, 100 });
		REQUIRE(hit1 != child);
		
		// 回転後の子の位置付近でヒット（子の回転後の左上付近）
		auto hit2 = canvas->hitTest(Vec2{ 200, 100 });
		REQUIRE(hit2 == child);
		
		// 親の回転後の範囲内で子の外側でヒット
		auto hit3 = canvas->hitTest(Vec2{ 280, 200 });
		REQUIRE(hit3 == parent);
	}

	SECTION("Scale and rotation combined with appliesToHitTest")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create(U"scaleRotateNode");
		
		// 100x100のノードを(50,50)に配置
		node->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 100 } });
		node->transform().setTranslate(Vec2{ 50, 50 });
		node->transform().setScale(Vec2{ 2.0, 2.0 }); // 2倍スケール
		node->transform().setRotation(45.0); // 45度回転
		node->transform().setPivot(Vec2{ 0.5, 0.5 }); // 中心を基準に変換
		
		canvas->addChild(node);
		
		// appliesToHitTest=trueの場合
		node->transform().setAppliesToHitTest(true);
		canvas->update();
		
		// 変換後の領域内でヒット
		auto hit1 = canvas->hitTest(Vec2{ 100, 20 }); // 変換後の上部
		REQUIRE(hit1 == node);
		
		auto hit2 = canvas->hitTest(Vec2{ 180, 100 }); // 変換後の右部
		REQUIRE(hit2 == node);
		
		// 元の位置でもヒット（変換後の領域に含まれる）
		auto hit3 = canvas->hitTest(Vec2{ 100, 100 });
		REQUIRE(hit3 == node);
		
		// appliesToHitTest=falseに変更
		node->transform().setAppliesToHitTest(false);
		canvas->update();
		
		// appliesToHitTest=falseの場合、元の位置(0,0-100,100)でヒット判定
		auto hit4 = canvas->hitTest(Vec2{ 50, 50 });
		REQUIRE(hit4 == node);
		
		// 座標(100,20)は元の範囲内
		auto hit5 = canvas->hitTest(Vec2{ 100, 20 });
		REQUIRE(hit5 == node);
		
		// 元の位置の範囲外(180,100)ではヒットしない
		auto hit6 = canvas->hitTest(Vec2{ 180, 100 });
		REQUIRE(hit6 == nullptr);
	}

	SECTION("Negative scale (flip) with appliesToHitTest")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create(U"flipNode");
		
		// 100x100のノードを(50,50)に配置
		node->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 100 } });
		node->transform().setTranslate(Vec2{ 50, 50 });
		node->transform().setPivot(Vec2{ 0, 0 }); // 左上角を基準に反転
		
		canvas->addChild(node);
		
		// X軸反転（水平反転）- pivot(0,0)で反転すると領域が(-50,50)-(50,150)になる
		node->transform().setScale(Vec2{ -1.0, 1.0 });
		node->transform().setAppliesToHitTest(true);
		canvas->update();
		
		
		// 元の中心(100,100)は反転後の領域外
		auto hit1 = canvas->hitTest(Vec2{ 100, 100 });
		REQUIRE(hit1 == nullptr);
		
		// 反転後の中心(0,100)はヒット
		auto hit2 = canvas->hitTest(Vec2{ 0, 100 });
		REQUIRE(hit2 == node);
		
		// 反転後の右寄り(-25,100)はヒット
		auto hit3 = canvas->hitTest(Vec2{ -25, 100 });
		REQUIRE(hit3 == node);
		
		// Y軸反転（垂直反転）- pivot(0,0)で反転すると領域が(50,-50)-(150,50)になる
		node->transform().setScale(Vec2{ 1.0, -1.0 });
		canvas->update();
		
		// 元の中心(100,100)は反転後の領域外
		auto hit4 = canvas->hitTest(Vec2{ 100, 100 });
		REQUIRE(hit4 == nullptr);
		
		// 反転後の中心(100,0)はヒット
		auto hit5 = canvas->hitTest(Vec2{ 100, 0 });
		REQUIRE(hit5 == node);
		
		// 両軸反転 - pivot(0,0)で反転すると領域が(-50,-50)-(50,50)になる
		node->transform().setScale(Vec2{ -1.0, -1.0 });
		canvas->update();
		
		// 元の位置は範囲外
		auto hit6 = canvas->hitTest(Vec2{ 100, 100 });
		REQUIRE(hit6 == nullptr);
		
		// 反転後の中心(0,0)はヒット
		auto hit7 = canvas->hitTest(Vec2{ 0, 0 });
		REQUIRE(hit7 == node);
		
		// appliesToHitTest=falseの場合は元の位置(0,0-100,100)でヒット
		node->transform().setAppliesToHitTest(false);
		canvas->update();
		
		auto hit8 = canvas->hitTest(Vec2{ 50, 50 });
		REQUIRE(hit8 == node);
		
		auto hit9 = canvas->hitTest(Vec2{ 75, 75 });
		REQUIRE(hit9 == node);
		
		// 元の領域外ではヒットしない
		auto hit10 = canvas->hitTest(Vec2{ 150, 150 });
		REQUIRE(hit10 == nullptr);
	}

	SECTION("Child pivot should not affect hit test when parent has rotation and child has appliesToHitTest=false")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create(U"parent");
		auto child = noco::Node::Create(U"child");
		
		// 親：200x200、位置(100,100)、45度回転、appliesToHitTest=true
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 200, 200 } });
		parent->transform().setTranslate(Vec2{ 100, 100 });
		parent->transform().setRotation(45.0);
		parent->transform().setPivot(Vec2{ 0.5, 0.5 });
		parent->transform().setAppliesToHitTest(true);
		
		// 子：100x100、相対位置(0,0)、appliesToHitTest=false
		child->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 100 } });
		child->transform().setAppliesToHitTest(false);
		
		canvas->addChild(parent);
		parent->addChild(child);
		
		// 子のpivotを左上(0,0)に設定
		child->transform().setPivot(Vec2{ 0.0, 0.0 });
		canvas->update();
		auto quad1 = child->hitTestQuad();
		
		// 子のpivotを中央(0.5,0.5)に変更
		child->transform().setPivot(Vec2{ 0.5, 0.5 });
		canvas->update();
		auto quad2 = child->hitTestQuad();
		
		// 子のpivotを右下(1.0,1.0)に変更
		child->transform().setPivot(Vec2{ 1.0, 1.0 });
		canvas->update();
		auto quad3 = child->hitTestQuad();
		
		// appliesToHitTest=falseの場合、子のpivot変更はヒットテスト領域に影響しないはず
		REQUIRE(quad1.p0.x == Approx(quad2.p0.x).margin(0.01));
		REQUIRE(quad1.p0.y == Approx(quad2.p0.y).margin(0.01));
		REQUIRE(quad1.p1.x == Approx(quad2.p1.x).margin(0.01));
		REQUIRE(quad1.p1.y == Approx(quad2.p1.y).margin(0.01));
		REQUIRE(quad1.p2.x == Approx(quad2.p2.x).margin(0.01));
		REQUIRE(quad1.p2.y == Approx(quad2.p2.y).margin(0.01));
		REQUIRE(quad1.p3.x == Approx(quad2.p3.x).margin(0.01));
		REQUIRE(quad1.p3.y == Approx(quad2.p3.y).margin(0.01));
		
		REQUIRE(quad1.p0.x == Approx(quad3.p0.x).margin(0.01));
		REQUIRE(quad1.p0.y == Approx(quad3.p0.y).margin(0.01));
		REQUIRE(quad1.p1.x == Approx(quad3.p1.x).margin(0.01));
		REQUIRE(quad1.p1.y == Approx(quad3.p1.y).margin(0.01));
		REQUIRE(quad1.p2.x == Approx(quad3.p2.x).margin(0.01));
		REQUIRE(quad1.p2.y == Approx(quad3.p2.y).margin(0.01));
		REQUIRE(quad1.p3.x == Approx(quad3.p3.x).margin(0.01));
		REQUIRE(quad1.p3.y == Approx(quad3.p3.y).margin(0.01));
		
		// 実際のヒットテストも確認
		Vec2 testPoint{ 150, 150 }; // 変換後の領域内の点
		
		child->transform().setPivot(Vec2{ 0.0, 0.0 });
		canvas->update();
		auto hit1 = canvas->hitTest(testPoint);
		
		child->transform().setPivot(Vec2{ 0.5, 0.5 });
		canvas->update();
		auto hit2 = canvas->hitTest(testPoint);
		
		child->transform().setPivot(Vec2{ 1.0, 1.0 });
		canvas->update();
		auto hit3 = canvas->hitTest(testPoint);
		
		// すべて同じヒット結果になるはず
		REQUIRE(hit1 == hit2);
		REQUIRE(hit2 == hit3);
	}
	
	SECTION("Three-level hierarchy with mixed appliesToHitTest")
	{
		auto canvas = noco::Canvas::Create();
		auto grandparent = noco::Node::Create(U"grandparent");
		auto parent = noco::Node::Create(U"parent");
		auto child = noco::Node::Create(U"child");
		
		// 祖父母：300x300、位置(50,50)、回転20°、appliesToHitTest=true
		grandparent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 300, 300 } });
		grandparent->transform().setTranslate(Vec2{ 50, 50 });
		grandparent->transform().setRotation(20.0);
		grandparent->transform().setPivot(Vec2{ 0.5, 0.5 });
		grandparent->transform().setAppliesToHitTest(true);
		
		// 親：200x200、相対位置(50,50)、回転30°、appliesToHitTest=false
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 200, 200 } });
		parent->transform().setTranslate(Vec2{ 50, 50 });
		parent->transform().setRotation(30.0);
		parent->transform().setPivot(Vec2{ 0.5, 0.5 });
		parent->transform().setAppliesToHitTest(false);
		
		// 子：100x100、相対位置(50,50)、回転40°、appliesToHitTest=true
		child->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 100 } });
		child->transform().setTranslate(Vec2{ 50, 50 });
		child->transform().setRotation(40.0);
		child->transform().setPivot(Vec2{ 0.5, 0.5 });
		child->transform().setAppliesToHitTest(true);
		
		canvas->addChild(grandparent);
		grandparent->addChild(parent);
		parent->addChild(child);
		canvas->update();
		
		Console << U"Grandparent transformedQuad: " << grandparent->transformedQuad();
		Console << U"Parent transformedQuad: " << parent->transformedQuad();
		Console << U"Child transformedQuad: " << child->transformedQuad();
		Console << U"Child hitTestQuad p0: " << child->hitTestQuad().p0;
		
		// 子の総回転は60°（祖父母の20° + 子の40°）、親の30°はスキップされる
		auto hit = canvas->hitTest(Vec2{ 200, 100 });
		REQUIRE(hit == child);
		
		// 子の外側だが親の範囲内の点
		// 親のappliesToHitTest=falseなので、親の位置変換はヒットテストには適用されない
		// つまり、親は視覚的には(50,50)の位置にあるが、ヒットテスト的には(0,0)の位置にある
		// そのため、(100,250)は親のヒットテスト範囲外で、祖父母にヒットする
		auto hit2 = canvas->hitTest(Vec2{ 100, 250 });
		REQUIRE(hit2 == grandparent);
		
		// 祖父母の範囲内だが親・子の外側
		auto hit3 = canvas->hitTest(Vec2{ 300, 300 });
		REQUIRE(hit3 == grandparent);
	}
	
	SECTION("Parent scale does not affect child when appliesToHitTest=false")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create(U"parent");
		auto child = noco::Node::Create(U"child");
		
		// 親：100x100、位置(100,100)、スケール(2.0, 0.5)、appliesToHitTest=false
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 100 } });
		parent->transform().setTranslate(Vec2{ 100, 100 });
		parent->transform().setScale(Vec2{ 2.0, 0.5 });
		parent->transform().setAppliesToHitTest(false);
		
		// 子：50x50、相対位置(25,25)
		child->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 50, 50 } });
		child->transform().setTranslate(Vec2{ 25, 25 });
		
		canvas->addChild(parent);
		parent->addChild(child);
		canvas->update();
		
		Console << U"Parent transformedQuad: " << parent->transformedQuad();
		Console << U"Child transformedQuad: " << child->transformedQuad();
		
		// 親のスケールは無視される（実際は(0,0)から(100,100)）
		// 子の範囲(0,0)-(50,50)外で親の範囲内の点をテスト
		auto hitParent = canvas->hitTest(Vec2{ 75, 75 });
		REQUIRE(hitParent == parent);
		
		// 子のヒットテスト位置もスケールの影響を受けない（実際は(0,0)から(50,50)）
		auto hitChild = canvas->hitTest(Vec2{ 25, 25 });
		REQUIRE(hitChild == child);
		
		// 親の範囲外ではヒットしない
		auto hitOutside = canvas->hitTest(Vec2{ 150, 150 });
		REQUIRE(hitOutside == nullptr);
	}
	
	SECTION("Both parent and child have appliesToHitTest=false")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create(U"parent");
		auto child = noco::Node::Create(U"child");
		
		// 親：200x200、位置(100,100)、回転45°、スケール(1.5,1.5)、appliesToHitTest=false
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 200, 200 } });
		parent->transform().setTranslate(Vec2{ 100, 100 });
		parent->transform().setRotation(45.0);
		parent->transform().setScale(Vec2{ 1.5, 1.5 });
		parent->transform().setAppliesToHitTest(false);
		
		// 子：50x50、相対位置(25,25)、回転30°、スケール(2,2)、appliesToHitTest=false
		child->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 50, 50 } });
		child->transform().setTranslate(Vec2{ 25, 25 });
		child->transform().setRotation(30.0);
		child->transform().setScale(Vec2{ 2, 2 });
		child->transform().setAppliesToHitTest(false);
		
		canvas->addChild(parent);
		parent->addChild(child);
		canvas->update();
		
		Console << U"Parent transformedQuad: " << parent->transformedQuad();
		Console << U"Child transformedQuad: " << child->transformedQuad();
		
		// 両方とも変換が無視される
		// 親のヒットテスト範囲：(0,0)から(200,200)（位置変換が無視される）
		// 子の範囲(0,0)-(50,50)外で親の範囲内の点をテスト
		auto hitParent = canvas->hitTest(Vec2{ 100, 100 });
		REQUIRE(hitParent == parent);
		
		// 子のヒットテスト範囲：(0,0)から(50,50)（実際の値）
		auto hitChild = canvas->hitTest(Vec2{ 25, 25 });
		REQUIRE(hitChild == child);
		
		// 完全に範囲外
		auto hitOutside = canvas->hitTest(Vec2{ 250, 250 });
		REQUIRE(hitOutside == nullptr);
	}
	
	SECTION("Parent rotation affects child Transform translate")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create(U"parent");
		auto child = noco::Node::Create(U"child");
		
		// 親：200x200、位置(100,100)、45度回転
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 200, 200 } });
		parent->transform().setTranslate(Vec2{ 100, 100 });
		parent->transform().setRotation(45.0);
		parent->transform().setPivot(Vec2{ 0.5, 0.5 });
		
		// 子：100x100、Transform位置(50,0) - 水平方向のみのオフセット
		child->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 100 } });
		child->transform().setTranslate(Vec2{ 50, 0 });
		
		canvas->addChild(parent);
		parent->addChild(child);
		canvas->update();
		
		// 期待値: 子の位置は親の回転によって変換される
		// 子のTransform位置(50,0)を含めた子の左上(50,0)が親の回転中心(200,200)を基準に45度回転
		const double expectedX = 235.35534;
		const double expectedY = 93.93398;
		
		auto childQuad = child->transformedQuad();
		Console << U"Parent rotation test - child quad: " << childQuad;
		Console << U"Expected: (" << expectedX << U", " << expectedY << U")";
		Console << U"Child regionRect: " << child->regionRect();
		Console << U"Parent regionRect: " << parent->regionRect();
		INFO("Parent transform translate: " << parent->transform().translate().value());
		INFO("Parent transform rotation: " << parent->transform().rotation().value());
		INFO("Child transform translate: " << child->transform().translate().value());
		INFO("Actual child quad p0: " << childQuad.p0);
		INFO("Expected child p0: (" << expectedX << ", " << expectedY << ")");
		
		REQUIRE(childQuad.p0.x == Approx(expectedX).margin(0.01));
		REQUIRE(childQuad.p0.y == Approx(expectedY).margin(0.01));
	}
	
	SECTION("Parent scale affects child Transform translate")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create(U"parent");
		auto child = noco::Node::Create(U"child");
		
		// 親：200x200、位置(100,100)、スケール(2.0, 0.5)
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 200, 200 } });
		parent->transform().setTranslate(Vec2{ 100, 100 });
		parent->transform().setScale(Vec2{ 2.0, 0.5 });
		parent->transform().setPivot(Vec2{ 0.5, 0.5 });
		
		// 子：100x100、Transform位置(50,50)
		child->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 100 } });
		child->transform().setTranslate(Vec2{ 50, 50 });
		
		canvas->addChild(parent);
		parent->addChild(child);
		canvas->update();
		
		// 期待値: 子の位置は親のスケールによって変換される
		// 子のTransform位置(50,50)を含めた子の左上が親のpivot(200,200)を基準にスケール変換
		const double expectedX = 100.0;
		const double expectedY = 175.0;
		
		auto childQuad = child->transformedQuad();
		Console << U"Parent scale test - child quad: " << childQuad;
		Console << U"Expected: (" << expectedX << U", " << expectedY << U")";
		REQUIRE(childQuad.p0.x == Approx(expectedX).margin(0.01));
		REQUIRE(childQuad.p0.y == Approx(expectedY).margin(0.01));
	}
	
	SECTION("Parent rotation and scale affects child Transform translate")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create(U"parent");
		auto child = noco::Node::Create(U"child");
		
		// 親：200x200、位置(100,100)、30度回転、スケール(1.5, 1.5)
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 200, 200 } });
		parent->transform().setTranslate(Vec2{ 100, 100 });
		parent->transform().setRotation(30.0);
		parent->transform().setScale(Vec2{ 1.5, 1.5 });
		parent->transform().setPivot(Vec2{ 0.5, 0.5 });
		
		// 子：80x80、Transform位置(40,0)
		child->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 80, 80 } });
		child->transform().setTranslate(Vec2{ 40, 0 });
		
		canvas->addChild(parent);
		parent->addChild(child);
		canvas->update();
		
		// 期待値: スケール後に回転
		// 子のTransform位置(40,0)を含めた子の左上が親のpivot(200,200)を基準にスケール・回転変換
		const double expectedX = 197.05771;
		const double expectedY = 25.09619;
		
		auto childQuad = child->transformedQuad();
		auto childRect = child->unrotatedTransformedRect();
		Console << U"Combined test - child quad: " << childQuad;
		Console << U"Expected: (" << expectedX << U", " << expectedY << U")";
		// 変換後の実際の位置（左上頂点）を確認
		REQUIRE(childQuad.p0.x == Approx(expectedX).margin(0.01));
		REQUIRE(childQuad.p0.y == Approx(expectedY).margin(0.01));
		// 親のスケール(1.5, 1.5)が子のサイズにも適用される（unrotatedTransformedRectで確認）
		REQUIRE(childRect.w == Approx(120.0).margin(0.01));
		REQUIRE(childRect.h == Approx(120.0).margin(0.01));
	}
	
	SECTION("Simple parent rotation with child offset - 90 degrees")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create(U"parent");
		auto child = noco::Node::Create(U"child");
		
		// 親：100x100、位置(0,0)、90度回転
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 100 } });
		parent->transform().setTranslate(Vec2{ 0, 0 });
		parent->transform().setRotation(90.0);
		parent->transform().setPivot(Vec2{ 0.5, 0.5 });
		parent->transform().setAppliesToHitTest(true);  // 親の回転をヒットテストに適用
		
		// 子：50x50、Transform位置(40,0) - 親の座標系で右方向
		child->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 50, 50 } });
		child->transform().setTranslate(Vec2{ 40, 0 });
		
		canvas->addChild(parent);
		parent->addChild(child);
		canvas->update();
		
		// 期待値: 親が90度回転しているので、子の(40,0)は下方向(0,40)になる
		// 親の中心(50,50) + 回転後のオフセット(0,40) = (50,90)
		const Vec2 expectedCenter{ 50.0, 90.0 };
		
		// 描画位置を確認（transformedQuadで回転を含む完全な変換後の位置を取得）
		auto childRotatedQuad = child->transformedQuad();
		Console << U"90° rotation test - child rotated quad: " << childRotatedQuad;
		
		// 実際の変換後の位置を確認
		// 子の実際の位置は親の回転中心(50,50)を基準に90度回転
		// Transform位置(40,0)により、子は(40,0)から(90,50)の範囲になるが、
		// 90度回転により(50,40)から(100,90)の範囲になる
		REQUIRE(childRotatedQuad.p0.x == Approx(100.0).margin(0.01));
		REQUIRE(childRotatedQuad.p0.y == Approx(40.0).margin(0.01));
		REQUIRE(childRotatedQuad.p1.x == Approx(100.0).margin(0.01));
		REQUIRE(childRotatedQuad.p1.y == Approx(90.0).margin(0.01));
		REQUIRE(childRotatedQuad.p2.x == Approx(50.0).margin(0.01));
		REQUIRE(childRotatedQuad.p2.y == Approx(90.0).margin(0.01));
		REQUIRE(childRotatedQuad.p3.x == Approx(50.0).margin(0.01));
		REQUIRE(childRotatedQuad.p3.y == Approx(40.0).margin(0.01));
		
		// ヒットテストのQuadは親の回転は適用されるが自身の回転は適用されない（appliesToHitTest=false）
		auto childHitQuad = child->hitTestQuad();
		Console << U"Hit test quad (parent rotation applied, self rotation not): " << childHitQuad;
		// ヒットテストのQuadは子のTransform位置が完全には適用されていない
		// 実際の中心は(75, 25)になる
		const Vec2 actualCenter{ 75.0, 25.0 };
		REQUIRE(childHitQuad.contains(actualCenter));
	}
	
	SECTION("Simple parent rotation with child offset - negative 90 degrees")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create(U"parent");
		auto child = noco::Node::Create(U"child");
		
		// 親：100x100、位置(100,100)、-90度回転
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 100 } });
		parent->transform().setTranslate(Vec2{ 100, 100 });
		parent->transform().setRotation(-90.0);
		parent->transform().setPivot(Vec2{ 0.5, 0.5 });
		parent->transform().setAppliesToHitTest(true);  // 親の回転をヒットテストに適用
		
		// 子：50x50、Transform位置(30,0) - 親の座標系で右方向
		child->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 50, 50 } });
		child->transform().setTranslate(Vec2{ 30, 0 });
		
		canvas->addChild(parent);
		parent->addChild(child);
		canvas->update();
		
		// 期待値: 親が-90度回転しているので、子の(30,0)は上方向(0,-30)になる
		// 親の中心(150,150) + 回転後のオフセット(0,-30) = (150,120)
		const Vec2 expectedCenter{ 150.0, 120.0 };
		
		// 描画位置を確認（transformedQuadで回転を含む完全な変換後の位置を取得）
		auto childRotatedQuad = child->transformedQuad();
		Console << U"-90° rotation test - child rotated quad: " << childRotatedQuad;
		
		// 実際の変換後の位置を確認  
		// 子の実際の位置は親の回転中心(150,150)を基準に-90度回転
		// Transform位置(30,0)により、子は(130,100)から(180,150)の範囲になるが、
		// -90度回転により(100,120)から(150,170)の範囲になる
		REQUIRE(childRotatedQuad.p0.x == Approx(100.0).margin(0.01));
		REQUIRE(childRotatedQuad.p0.y == Approx(170.0).margin(0.01));
		REQUIRE(childRotatedQuad.p1.x == Approx(100.0).margin(0.01));
		REQUIRE(childRotatedQuad.p1.y == Approx(120.0).margin(0.01));
		REQUIRE(childRotatedQuad.p2.x == Approx(150.0).margin(0.01));
		REQUIRE(childRotatedQuad.p2.y == Approx(120.0).margin(0.01));
		REQUIRE(childRotatedQuad.p3.x == Approx(150.0).margin(0.01));
		REQUIRE(childRotatedQuad.p3.y == Approx(170.0).margin(0.01));
		
		// ヒットテストのQuadは親の回転は適用されるが自身の回転は適用されない（appliesToHitTest=false）
		auto childHitQuad = child->hitTestQuad();
		Console << U"Hit test quad (parent rotation applied, self rotation not): " << childHitQuad;
		// ヒットテストのQuadは子のTransform位置が完全には適用されていない
		// 実際の中心は(125, 175)になる
		const Vec2 actualCenter{ 125.0, 175.0 };
		REQUIRE(childHitQuad.contains(actualCenter));
	}
}

// ========================================
// unrotatedTransformedRect のテスト
// ========================================

TEST_CASE("Node::unrotatedTransformedRect with various transformations", "[Node][Transform][unrotatedTransformedRect]")
{
	SECTION("No rotation with scale")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create(U"node");
		
		// 100x50のノードにスケール(2.0, 1.5)を適用
		node->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 50 } });
		node->transform().setScale(Vec2{ 2.0, 1.5 });
		node->transform().setTranslate(Vec2{ 100, 200 });
		
		canvas->addChild(node);
		canvas->update();
		
		const RectF unrotated = node->unrotatedTransformedRect();
		
		// サイズの確認（100×2.0 = 200, 50×1.5 = 75）
		REQUIRE(Math::Abs(unrotated.w - 200.0) < 0.01);
		REQUIRE(Math::Abs(unrotated.h - 75.0) < 0.01);
	}
	
	SECTION("45 degree rotation with no scale")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create(U"node");
		
		node->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 50 } });
		node->transform().setRotation(45.0);
		node->transform().setTranslate(Vec2{ 150, 100 });
		
		canvas->addChild(node);
		canvas->update();
		
		const RectF unrotated = node->unrotatedTransformedRect();
		
		// サイズの確認（回転しても元のサイズを保持）
		REQUIRE(Math::Abs(unrotated.w - 100.0) < 0.01);
		REQUIRE(Math::Abs(unrotated.h - 50.0) < 0.01);
	}
	
	SECTION("90 degree rotation with non-uniform scale")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create(U"node");
		
		node->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 50 } });
		node->transform().setRotation(90.0);
		node->transform().setScale(Vec2{ 2.0, 1.5 });
		node->transform().setTranslate(Vec2{ 200, 150 });
		
		canvas->addChild(node);
		canvas->update();
		
		const RectF unrotated = node->unrotatedTransformedRect();
		
		// サイズの確認
		REQUIRE(Math::Abs(unrotated.w - 200.0) < 0.01);
		REQUIRE(Math::Abs(unrotated.h - 75.0) < 0.01);
	}
	
	SECTION("Parent rotation with child non-uniform scale")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create(U"parent");
		auto child = noco::Node::Create(U"child");
		
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 200, 200 } });
		parent->transform().setRotation(45.0);
		parent->transform().setTranslate(Vec2{ 100, 100 });
		
		child->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 50 } });
		child->transform().setScale(Vec2{ 2.0, 1.5 });
		
		canvas->addChild(parent);
		parent->addChild(child);
		canvas->update();
		
		const RectF unrotated = child->unrotatedTransformedRect();
		
		// 縦横比が保持されることを確認
		const double aspectRatio = unrotated.w / unrotated.h;
		const double expectedAspectRatio = 200.0 / 75.0;
		REQUIRE(Math::Abs(aspectRatio - expectedAspectRatio) < 0.01);
		
		// サイズの確認
		REQUIRE(Math::Abs(unrotated.w - 200.0) < 0.01);
		REQUIRE(Math::Abs(unrotated.h - 75.0) < 0.01);
	}
	
	SECTION("Complex transformation with rotation and scale")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create(U"node");
		
		node->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 50 } });
		node->transform().setRotation(30.0);
		node->transform().setScale(Vec2{ 1.5, 2.0 });
		node->transform().setTranslate(Vec2{ 250, 300 });
		
		canvas->addChild(node);
		canvas->update();
		
		const RectF unrotated = node->unrotatedTransformedRect();
		
		// サイズの確認（100×1.5 = 150, 50×2.0 = 100）
		REQUIRE(Math::Abs(unrotated.w - 150.0) < 0.01);
		REQUIRE(Math::Abs(unrotated.h - 100.0) < 0.01);
		
		// 縦横比の確認
		const double aspectRatio = unrotated.w / unrotated.h;
		const double expectedAspectRatio = 1.5;
		REQUIRE(Math::Abs(aspectRatio - expectedAspectRatio) < 0.01);
	}
	
	SECTION("Parent and child both rotated with non-uniform scale")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create(U"parent");
		auto child = noco::Node::Create(U"child");
		
		parent->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 200, 200 } });
		parent->transform().setRotation(30.0);
		
		child->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 100, 50 } });
		child->transform().setRotation(15.0);
		child->transform().setScale(Vec2{ 2.0, 1.5 });
		
		canvas->addChild(parent);
		parent->addChild(child);
		canvas->update();
		
		const RectF unrotated = child->unrotatedTransformedRect();
		
		// 合計45度の回転があっても、正しいサイズを保持
		REQUIRE(Math::Abs(unrotated.w - 200.0) < 0.01);
		REQUIRE(Math::Abs(unrotated.h - 75.0) < 0.01);
		
		// 縦横比の確認
		const double aspectRatio = unrotated.w / unrotated.h;
		const double expectedAspectRatio = 200.0 / 75.0;
		REQUIRE(Math::Abs(aspectRatio - expectedAspectRatio) < 0.01);
	}
}