#include <catch2/catch.hpp>
#include <Siv3D.hpp>
#include <NocoUI.hpp>

// ========================================
// Canvasのテスト
// ========================================

TEST_CASE("Canvas system", "[Canvas]")
{
	SECTION("Strict parent checking for addChild")
	{
		auto canvas1 = noco::Canvas::Create();
		auto canvas2 = noco::Canvas::Create();
		
		// Canvas1にトップレベルノードを追加
		auto nodeA = noco::Node::Create(U"NodeA");
		auto nodeB = noco::Node::Create(U"NodeB");
		canvas1->addChild(nodeA);
		canvas1->addChild(nodeB);
		
		REQUIRE(canvas1->children().size() == 2);
		REQUIRE(nodeA->isTopLevelNode());
		REQUIRE(nodeB->isTopLevelNode());
		
		// 既にトップレベルノードとして存在するnodeAを他のCanvasに追加しようとすると例外
		REQUIRE_THROWS_AS(canvas2->addChild(nodeA), s3d::Error);
		
		// Canvas1内での階層構造を作成
		auto child = noco::Node::Create(U"Child");
		nodeA->addChild(child);
		REQUIRE(!child->isTopLevelNode());
		
		// 既に親を持つchildをCanvasに追加しようとすると例外
		REQUIRE_THROWS_AS(canvas2->addChild(child), s3d::Error);
		
		// 正しい方法：まず親から削除してから追加
		nodeA->removeChild(child);
		REQUIRE_NOTHROW(canvas2->addChild(child));
		REQUIRE(child->isTopLevelNode());
		REQUIRE(child->containedCanvas() == canvas2);
	}
	
	SECTION("Node movement between canvases using setParent")
	{
		auto canvas1 = noco::Canvas::Create();
		auto canvas2 = noco::Canvas::Create();
		
		// Canvas1に階層構造を作成
		auto parent1 = noco::Node::Create(U"Parent1");
		auto child1 = noco::Node::Create(U"Child1");
		canvas1->addChild(parent1);
		parent1->addChild(child1);
		
		// Canvas2にノードを追加
		auto parent2 = noco::Node::Create(U"Parent2");
		canvas2->addChild(parent2);
		
		REQUIRE(child1->containedCanvas() == canvas1);
		REQUIRE(parent2->containedCanvas() == canvas2);
		
		// setParentによる移動は正常に動作する（既存の親から切り離される）
		child1->setParent(parent2);
		
		REQUIRE(parent1->children().size() == 0);
		REQUIRE(parent2->children().size() == 1);
		REQUIRE(parent2->children()[0] == child1);
		REQUIRE(child1->containedCanvas() == canvas2);
		REQUIRE(child1->parentNode() == parent2);
	}
}

TEST_CASE("Canvas quad method", "[Canvas]")
{
	SECTION("Default Canvas quad")
	{
		auto canvas = noco::Canvas::Create(400, 300);
		auto quad = canvas->quad();
		
		// デフォルト状態では変換なしの矩形
		REQUIRE(quad.p0.x == Approx(0.0));
		REQUIRE(quad.p0.y == Approx(0.0));
		REQUIRE(quad.p1.x == Approx(400.0));
		REQUIRE(quad.p1.y == Approx(0.0));
		REQUIRE(quad.p2.x == Approx(400.0));
		REQUIRE(quad.p2.y == Approx(300.0));
		REQUIRE(quad.p3.x == Approx(0.0));
		REQUIRE(quad.p3.y == Approx(300.0));
	}
	
	SECTION("Canvas with position offset")
	{
		auto canvas = noco::Canvas::Create(200, 100);
		canvas->setPosition({50, 25});
		auto quad = canvas->quad();
		
		// 位置オフセットが適用される
		REQUIRE(quad.p0.x == Approx(50.0));
		REQUIRE(quad.p0.y == Approx(25.0));
		REQUIRE(quad.p1.x == Approx(250.0));
		REQUIRE(quad.p1.y == Approx(25.0));
		REQUIRE(quad.p2.x == Approx(250.0));
		REQUIRE(quad.p2.y == Approx(125.0));
		REQUIRE(quad.p3.x == Approx(50.0));
		REQUIRE(quad.p3.y == Approx(125.0));
	}
	
	SECTION("Canvas with scale")
	{
		auto canvas = noco::Canvas::Create(100, 100);
		canvas->setScale({2.0, 1.5});
		auto quad = canvas->quad();
		
		// スケールが適用される
		REQUIRE(quad.p0.x == Approx(0.0));
		REQUIRE(quad.p0.y == Approx(0.0));
		REQUIRE(quad.p1.x == Approx(200.0));
		REQUIRE(quad.p1.y == Approx(0.0));
		REQUIRE(quad.p2.x == Approx(200.0));
		REQUIRE(quad.p2.y == Approx(150.0));
		REQUIRE(quad.p3.x == Approx(0.0));
		REQUIRE(quad.p3.y == Approx(150.0));
	}
	
	SECTION("Canvas with position and scale")
	{
		auto canvas = noco::Canvas::Create(100, 50);
		canvas->setPosition({10, 20});
		canvas->setScale({2.0, 3.0});
		auto quad = canvas->quad();
		
		// 位置とスケール両方が適用される
		REQUIRE(quad.p0.x == Approx(10.0));
		REQUIRE(quad.p0.y == Approx(20.0));
		REQUIRE(quad.p1.x == Approx(210.0));
		REQUIRE(quad.p1.y == Approx(20.0));
		REQUIRE(quad.p2.x == Approx(210.0));
		REQUIRE(quad.p2.y == Approx(170.0));
		REQUIRE(quad.p3.x == Approx(10.0));
		REQUIRE(quad.p3.y == Approx(170.0));
	}
}

TEST_CASE("Canvas center methods", "[Canvas]")
{
	SECTION("Default Canvas center")
	{
		auto canvas = noco::Canvas::Create(400, 300);
		auto center = canvas->center();
		
		// デフォルト位置(0,0)でのCanvas中央
		REQUIRE(center.x == Approx(200.0));  // width/2
		REQUIRE(center.y == Approx(150.0));  // height/2
	}
	
	SECTION("Canvas with position offset center")
	{
		auto canvas = noco::Canvas::Create(200, 100);
		canvas->setPosition({50, 25});
		auto center = canvas->center();
		
		// 位置オフセット後のCanvas中央
		REQUIRE(center.x == Approx(150.0));  // 50 + 200/2
		REQUIRE(center.y == Approx(75.0));   // 25 + 100/2
	}
	
	SECTION("setCenter method")
	{
		auto canvas = noco::Canvas::Create(400, 300);
		canvas->setCenter({100, 80});
		
		// 中央を指定した場合の位置確認
		auto center = canvas->center();
		REQUIRE(center.x == Approx(100.0));
		REQUIRE(center.y == Approx(80.0));
		
		// 対応するpositionの確認
		auto position = canvas->position();
		REQUIRE(position.x == Approx(-100.0));  // 100 - 400/2
		REQUIRE(position.y == Approx(-70.0));   // 80 - 300/2
	}
	
	SECTION("setCenter and position consistency")
	{
		auto canvas = noco::Canvas::Create(600, 400);
		
		// 中央を設定
		canvas->setCenter({300, 200});
		
		// positionが正しく計算されているか確認
		auto position = canvas->position();
		REQUIRE(position.x == Approx(0.0));   // 300 - 600/2
		REQUIRE(position.y == Approx(0.0));   // 200 - 400/2
		
		// centerが正しく取得できるか確認
		auto center = canvas->center();
		REQUIRE(center.x == Approx(300.0));
		REQUIRE(center.y == Approx(200.0));
	}
	
	SECTION("Method chaining for setCenter")
	{
		auto canvas = noco::Canvas::Create(100, 100);
		auto result = canvas->setCenter({50, 50});
		
		// setCenterはCanvasのshared_ptrを返す
		REQUIRE(result == canvas);
		REQUIRE(canvas->center().x == Approx(50.0));
		REQUIRE(canvas->center().y == Approx(50.0));
	}
}

// Canvasのレイアウトテスト
TEST_CASE("Canvas children layout", "[Canvas][Layout]")
{
	SECTION("Default layout is FlowLayout")
	{
		auto canvas = noco::Canvas::Create(400, 300);
		
		// デフォルトはFlowLayout
		REQUIRE(canvas->childrenFlowLayout() != nullptr);
		REQUIRE(canvas->childrenHorizontalLayout() == nullptr);
		REQUIRE(canvas->childrenVerticalLayout() == nullptr);
	}
	
	SECTION("FlowLayout arranges top-level nodes")
	{
		auto canvas = noco::Canvas::Create(400, 300);
		canvas->setChildrenLayout(noco::FlowLayout{
			.padding = noco::LRTB{ 10, 10, 10, 10 },
			.spacing = Vec2{ 10, 10 },
		});
		
		// トップレベルノードを追加
		auto node1 = noco::Node::Create(U"Node1", noco::InlineRegion{ .sizeDelta = Vec2{ 100, 50 } });
		auto node2 = noco::Node::Create(U"Node2", noco::InlineRegion{ .sizeDelta = Vec2{ 100, 50 } });
		auto node3 = noco::Node::Create(U"Node3", noco::InlineRegion{ .sizeDelta = Vec2{ 100, 50 } });
		
		canvas->addChild(node1);
		canvas->addChild(node2);
		canvas->addChild(node3);
		
		// レイアウトを更新
		canvas->refreshLayoutImmediately();
		
		// FlowLayoutにより横に並ぶ（padding考慮）
		REQUIRE(node1->regionRect().x == Approx(10.0));
		REQUIRE(node1->regionRect().y == Approx(10.0));
		REQUIRE(node2->regionRect().x == Approx(120.0)); // 10 + 100 + 10(spacing)
		REQUIRE(node2->regionRect().y == Approx(10.0));
		REQUIRE(node3->regionRect().x == Approx(230.0)); // 120 + 100 + 10(spacing)
		REQUIRE(node3->regionRect().y == Approx(10.0));
	}
	
	SECTION("HorizontalLayout arranges top-level nodes")
	{
		auto canvas = noco::Canvas::Create(400, 300);
		canvas->setChildrenLayout(noco::HorizontalLayout{
			.padding = noco::LRTB{ 20, 20, 20, 20 },
			.spacing = 15,
			.horizontalAlign = noco::HorizontalAlign::Left,
			.verticalAlign = noco::VerticalAlign::Middle,
		});
		
		// トップレベルノードを追加
		auto node1 = noco::Node::Create(U"Node1", noco::InlineRegion{ .sizeDelta = Vec2{ 80, 60 } });
		auto node2 = noco::Node::Create(U"Node2", noco::InlineRegion{ .sizeDelta = Vec2{ 80, 40 } });
		auto node3 = noco::Node::Create(U"Node3", noco::InlineRegion{ .sizeDelta = Vec2{ 80, 80 } });
		
		canvas->addChild(node1);
		canvas->addChild(node2);
		canvas->addChild(node3);
		
		// レイアウトを更新
		canvas->refreshLayoutImmediately();
		
		// HorizontalLayoutにより横一列に並ぶ
		REQUIRE(node1->regionRect().x == Approx(20.0));
		REQUIRE(node2->regionRect().x == Approx(115.0)); // 20 + 80 + 15(spacing)
		REQUIRE(node3->regionRect().x == Approx(210.0)); // 115 + 80 + 15(spacing)
		
		// 垂直方向は中央揃え（キャンバス高さ300、padding上下40、利用可能高さ260）
		double availableHeight = 260.0;
		REQUIRE(node1->regionRect().y == Approx(20.0 + (availableHeight - 60) / 2));
		REQUIRE(node2->regionRect().y == Approx(20.0 + (availableHeight - 40) / 2));
		REQUIRE(node3->regionRect().y == Approx(20.0 + (availableHeight - 80) / 2));
	}
	
	SECTION("VerticalLayout arranges top-level nodes")
	{
		auto canvas = noco::Canvas::Create(400, 300);
		canvas->setChildrenLayout(noco::VerticalLayout{
			.padding = noco::LRTB{ 15, 15, 15, 15 },
			.spacing = 20,
			.horizontalAlign = noco::HorizontalAlign::Center,
			.verticalAlign = noco::VerticalAlign::Top,
		});
		
		// トップレベルノードを追加
		auto node1 = noco::Node::Create(U"Node1", noco::InlineRegion{ .sizeDelta = Vec2{ 120, 50 } });
		auto node2 = noco::Node::Create(U"Node2", noco::InlineRegion{ .sizeDelta = Vec2{ 100, 50 } });
		auto node3 = noco::Node::Create(U"Node3", noco::InlineRegion{ .sizeDelta = Vec2{ 140, 50 } });
		
		canvas->addChild(node1);
		canvas->addChild(node2);
		canvas->addChild(node3);
		
		// レイアウトを更新
		canvas->refreshLayoutImmediately();
		
		// VerticalLayoutにより縦一列に並ぶ
		REQUIRE(node1->regionRect().y == Approx(15.0));
		REQUIRE(node2->regionRect().y == Approx(85.0)); // 15 + 50 + 20(spacing)
		REQUIRE(node3->regionRect().y == Approx(155.0)); // 85 + 50 + 20(spacing)
		
		// 水平方向は中央揃え（キャンバス幅400、padding左右30、利用可能幅370）
		double availableWidth = 370.0;
		REQUIRE(node1->regionRect().x == Approx(15.0 + (availableWidth - 120) / 2));
		REQUIRE(node2->regionRect().x == Approx(15.0 + (availableWidth - 100) / 2));
		REQUIRE(node3->regionRect().x == Approx(15.0 + (availableWidth - 140) / 2));
	}
	
	SECTION("Layout change updates node positions")
	{
		auto canvas = noco::Canvas::Create(400, 300);
		
		// 初期はFlowLayout
		auto node1 = noco::Node::Create(U"Node1", noco::InlineRegion{ .sizeDelta = Vec2{ 100, 50 } });
		auto node2 = noco::Node::Create(U"Node2", noco::InlineRegion{ .sizeDelta = Vec2{ 100, 50 } });
		
		canvas->addChild(node1);
		canvas->addChild(node2);
		canvas->refreshLayoutImmediately();
		
		// FlowLayoutでの位置を記録
		double flowX1 = node1->regionRect().x;
		double flowX2 = node2->regionRect().x;
		double flowY1 = node1->regionRect().y;
		double flowY2 = node2->regionRect().y;
		
		// FlowLayoutでは横に並ぶのでy座標は同じ
		REQUIRE(flowY1 == flowY2);
		REQUIRE(flowX1 < flowX2);
		
		// VerticalLayoutに変更
		canvas->setChildrenLayout(noco::VerticalLayout{});
		canvas->refreshLayoutImmediately();
		
		// 位置が変わることを確認
		REQUIRE(node1->regionRect().x != flowX1);
		REQUIRE(node2->regionRect().x != flowX2);
		REQUIRE(node1->regionRect().y < node2->regionRect().y); // 縦に並ぶ
	}
}

// activeInHierarchy管理のテスト
TEST_CASE("Canvas activeInHierarchy management", "[Canvas][ActiveInHierarchy]")
{
	SECTION("addChild makes node activeInHierarchy")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto node = noco::Node::Create(U"TestNode");
		
		// Canvas配下にない場合はactiveInHierarchyはfalse
		REQUIRE(node->activeInHierarchy() == false);
		
		// Canvas::addChild後にactiveInHierarchyがtrueになる
		canvas->addChild(node);
		REQUIRE(node->activeInHierarchy() == true);
	}
	
	SECTION("emplaceChild creates node with activeInHierarchy")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		
		// Canvas::emplaceChildで作成されたノードは直接activeInHierarchyがtrue
		auto& node = canvas->emplaceChild(U"EmplacedNode");
		REQUIRE(node->activeInHierarchy() == true);
	}
	
	SECTION("Hierarchical activeInHierarchy propagation")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto parent = noco::Node::Create(U"Parent");
		auto child = noco::Node::Create(U"Child");
		auto grandchild = noco::Node::Create(U"Grandchild");
		
		// 階層構造を作成（まだCanvas配下にない）
		parent->addChild(child);
		child->addChild(grandchild);
		
		// Canvas配下にない場合は全てfalse
		REQUIRE(parent->activeInHierarchy() == false);
		REQUIRE(child->activeInHierarchy() == false);
		REQUIRE(grandchild->activeInHierarchy() == false);
		
		// 親をCanvas配下に追加すると、子孫も含めてactiveInHierarchyがtrueになる
		canvas->addChild(parent);
		REQUIRE(parent->activeInHierarchy() == true);
		REQUIRE(child->activeInHierarchy() == true);
		REQUIRE(grandchild->activeInHierarchy() == true);
	}
	
	SECTION("removeChild makes node activeInHierarchy false")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto parent = noco::Node::Create(U"Parent");
		auto child = noco::Node::Create(U"Child");
		
		parent->addChild(child);
		canvas->addChild(parent);
		
		// Canvas配下でactiveInHierarchyがtrue
		REQUIRE(parent->activeInHierarchy() == true);
		REQUIRE(child->activeInHierarchy() == true);
		
		// Canvas::removeChild後にactiveInHierarchyがfalseになる
		canvas->removeChild(parent);
		REQUIRE(parent->activeInHierarchy() == false);
		REQUIRE(child->activeInHierarchy() == false);
	}
	
	SECTION("Node activeSelf affects activeInHierarchy under Canvas")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto parent = noco::Node::Create(U"Parent");
		auto child = noco::Node::Create(U"Child");
		
		parent->addChild(child);
		canvas->addChild(parent);
		
		// 初期状態：両方ともactiveInHierarchyがtrue
		REQUIRE(parent->activeInHierarchy() == true);
		REQUIRE(child->activeInHierarchy() == true);
		
		// 親を非アクティブにすると子のactiveInHierarchyもfalseになる
		parent->setActive(noco::ActiveYN::No);
		REQUIRE(parent->activeInHierarchy() == false);
		REQUIRE(child->activeInHierarchy() == false);
		
		// 親を再アクティブにすると子のactiveInHierarchyもtrueになる
		parent->setActive(noco::ActiveYN::Yes);
		REQUIRE(parent->activeInHierarchy() == true);
		REQUIRE(child->activeInHierarchy() == true);
	}
	
	SECTION("removeChildrenAll makes all nodes activeInHierarchy false")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto node1 = noco::Node::Create(U"Node1");
		auto node2 = noco::Node::Create(U"Node2");
		auto child = noco::Node::Create(U"Child");
		
		// 階層構造を作成
		node1->addChild(child);
		canvas->addChild(node1);
		canvas->addChild(node2);
		
		// Canvas配下でactiveInHierarchyがtrue
		REQUIRE(node1->activeInHierarchy() == true);
		REQUIRE(node2->activeInHierarchy() == true);
		REQUIRE(child->activeInHierarchy() == true);
		REQUIRE(canvas->children().size() == 2);
		
		// removeChildrenAll後にactiveInHierarchyがfalseになる
		canvas->removeChildrenAll();
		REQUIRE(node1->activeInHierarchy() == false);
		REQUIRE(node2->activeInHierarchy() == false);
		REQUIRE(child->activeInHierarchy() == false);
		REQUIRE(canvas->children().size() == 0);
	}
}

TEST_CASE("Canvas styleState integration", "[Canvas][StyleState]")
{
	SECTION("emplaceChild with TextBox sets unfocused")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto node = canvas->emplaceChild(U"TextBoxNode");
		auto textBox = node->emplaceComponent<noco::TextBox>();
		
		// updateを呼ぶことでstyleStateがオーバーライドされる
		System::Update();
		canvas->update();
		
		// emplaceChildで作成したノードにTextBoxを追加すると自動的にunfocused
		REQUIRE(node->styleState() == U"unfocused");
	}
	
	SECTION("emplaceChild with TextArea sets unfocused")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto node = canvas->emplaceChild(U"TextAreaNode");
		auto textArea = node->emplaceComponent<noco::TextArea>();
		
		// updateを呼ぶことでstyleStateがオーバーライドされる
		System::Update();
		canvas->update();
		
		// emplaceChildで作成したノードにTextAreaを追加すると自動的にunfocused
		REQUIRE(node->styleState() == U"unfocused");
	}
	
	SECTION("Complex hierarchy with text components styleState")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto form = canvas->emplaceChild(U"Form");
		auto nameField = form->emplaceChild(U"NameField");
		auto descField = form->emplaceChild(U"DescField");
		
		// テキストコンポーネントを追加
		auto nameTextBox = nameField->emplaceComponent<noco::TextBox>();
		auto descTextArea = descField->emplaceComponent<noco::TextArea>();
		
		// updateを呼ぶことでstyleStateがオーバーライドされる
		System::Update();
		canvas->update();
		
		// 階層構造でもそれぞれ正しくunfocusedになる
		REQUIRE(form->styleState() == U"");  // テキストコンポーネントなし
		REQUIRE(nameField->styleState() == U"unfocused");  // TextBox
		REQUIRE(descField->styleState() == U"unfocused");  // TextArea
	}
	
	SECTION("styleState parameter reference")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		
		// パラメータを設定
		canvas->setParamValue(U"currentStyle", U"hover");
		
		// ノードを作成
		auto node = canvas->emplaceChild(U"TestNode");
		node->setStyleState(U"default");
		
		// パラメータ参照を設定
		node->setStyleStateParamRef(U"currentStyle");
		REQUIRE(node->styleStateParamRef() == U"currentStyle");
		
		// update前はまだパラメータ値が適用されない
		REQUIRE(node->styleState() == U"default");
		
		// updateするとパラメータ値が適用される
		canvas->update();
		REQUIRE(node->styleState() == U"hover");
		
		// パラメータを変更
		canvas->setParamValue(U"currentStyle", U"pressed");
		canvas->update();
		REQUIRE(node->styleState() == U"pressed");
		
		// パラメータ参照をクリア
		node->setStyleStateParamRef(U"");
		node->setStyleState(U"manual");
		System::Update();  // フレームを進める
		canvas->update();
		REQUIRE(node->styleState() == U"manual");
	}
	
	SECTION("styleState serialization with parameter reference")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		canvas->setParamValue(U"testStyle", U"active");
		
		// 元のノードを作成
		auto node = canvas->emplaceChild(U"Original");
		node->setStyleState(U"inactive");
		node->setStyleStateParamRef(U"testStyle");
		
		// JSONにシリアライズ
		const JSON json = node->toJSON();
		
		// JSONから新しいノードを作成
		auto loadedNode = noco::Node::CreateFromJSON(json);
		canvas->addChild(loadedNode);
		
		// 値とパラメータ参照が保持されているか確認
		REQUIRE(loadedNode->styleState() == U"inactive");  // 保存された値
		REQUIRE(loadedNode->styleStateParamRef() == U"testStyle");  // パラメータ参照
		
		// updateでパラメータ値が適用される
		canvas->update();
		REQUIRE(loadedNode->styleState() == U"active");
	}
	
	SECTION("activeSelf parameter reference")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		
		// パラメータを設定
		canvas->setParamValue(U"nodeActive", true);
		
		// ノードを作成
		auto node = canvas->emplaceChild(U"TestNode");
		node->setActive(false);
		
		// パラメータ参照を設定
		node->setActiveSelfParamRef(U"nodeActive");
		REQUIRE(node->activeSelfParamRef() == U"nodeActive");
		
		// update前はまだパラメータ値が適用されない
		REQUIRE(node->activeSelf() == false);
		
		// updateするとパラメータ値が適用される
		canvas->update();
		REQUIRE(node->activeSelf() == true);
		
		// パラメータを変更
		canvas->setParamValue(U"nodeActive", false);
		canvas->update();
		REQUIRE(node->activeSelf() == false);
		
		// パラメータ参照をクリア
		node->setActiveSelfParamRef(U"");
		node->setActive(true);
		System::Update();  // フレームを進める
		canvas->update();
		REQUIRE(node->activeSelf() == true);
	}
	
	SECTION("interactable parameter reference")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		
		// パラメータを設定
		canvas->setParamValue(U"allowInteraction", false);
		
		// ノードを作成
		auto node = canvas->emplaceChild(U"TestNode");
		node->setInteractable(true);
		
		// パラメータ参照を設定
		node->setInteractableParamRef(U"allowInteraction");
		REQUIRE(node->interactableParamRef() == U"allowInteraction");
		
		// update前はまだパラメータ値が適用されない
		REQUIRE(node->interactable() == true);
		
		// updateするとパラメータ値が適用される
		canvas->update();
		REQUIRE(node->interactable() == false);
		
		// パラメータを変更
		canvas->setParamValue(U"allowInteraction", true);
		canvas->update();
		REQUIRE(node->interactable() == true);
		
		// パラメータ参照をクリア
		node->setInteractableParamRef(U"");
		node->setInteractable(false);
		System::Update();  // フレームを進める
		canvas->update();
		REQUIRE(node->interactable() == false);
	}
	
	SECTION("zOrderInSiblings parameter reference")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		
		// パラメータを設定
		canvas->setParamValue(U"layerIndex", 10);
		
		// ノードを作成
		auto node = canvas->emplaceChild(U"TestNode");
		node->setZOrderInSiblings(noco::PropertyValue<int32>{ 0 });
		
		// パラメータ参照を設定
		node->setZOrderInSiblingsParamRef(U"layerIndex");
		REQUIRE(node->zOrderInSiblingsParamRef() == U"layerIndex");
		
		// update前はまだパラメータ値が適用されない
		REQUIRE(node->zOrderInSiblings() == 0);
		
		// updateするとパラメータ値が適用される
		canvas->update();
		REQUIRE(node->zOrderInSiblings() == 10);
		
		// パラメータを変更
		canvas->setParamValue(U"layerIndex", -5);
		canvas->update();
		REQUIRE(node->zOrderInSiblings() == -5);
		
		// パラメータ参照をクリア
		node->setZOrderInSiblingsParamRef(U"");
		node->setZOrderInSiblings(noco::PropertyValue<int32>{ 100 });
		System::Update();  // フレームを進める
		canvas->update();
		REQUIRE(node->zOrderInSiblings() == 100);
	}
	
	SECTION("activeSelf and interactable serialization with parameter reference")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		canvas->setParamValue(U"isActive", false);
		canvas->setParamValue(U"canInteract", true);
		canvas->setParamValue(U"zLayer", 99);
		
		// 元のノードを作成
		auto node = canvas->emplaceChild(U"Original");
		node->setActive(true);
		node->setInteractable(false);
		node->setZOrderInSiblings(noco::PropertyValue<int32>{ 5 });
		node->setActiveSelfParamRef(U"isActive");
		node->setInteractableParamRef(U"canInteract");
		node->setZOrderInSiblingsParamRef(U"zLayer");
		
		// JSONにシリアライズ
		const JSON json = node->toJSON();
		
		// JSONから新しいノードを作成
		auto loadedNode = noco::Node::CreateFromJSON(json);
		canvas->addChild(loadedNode);
		
		// 値とパラメータ参照が保持されているか確認
		REQUIRE(loadedNode->activeSelf() == true);  // 保存された値
		REQUIRE(loadedNode->interactable() == false);  // 保存された値
		REQUIRE(loadedNode->zOrderInSiblings() == 5);  // 保存された値
		REQUIRE(loadedNode->activeSelfParamRef() == U"isActive");  // パラメータ参照
		REQUIRE(loadedNode->interactableParamRef() == U"canInteract");  // パラメータ参照
		REQUIRE(loadedNode->zOrderInSiblingsParamRef() == U"zLayer");  // パラメータ参照
		
		// updateでパラメータ値が適用される
		canvas->update();
		REQUIRE(loadedNode->activeSelf() == false);
		REQUIRE(loadedNode->interactable() == true);
		REQUIRE(loadedNode->zOrderInSiblings() == 99);
	}
	
	SECTION("Properties without parameter reference should reflect values immediately")
	{
		// 単純に値をセットして確認するだけのテストに見えるが、
		// 内部実装がPropertyNonInteractiveのため即時反映されることを検証している
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto node = canvas->emplaceChild(U"TestNode");
		
		// activeSelfのテスト（パラメータ参照なし）
		node->setActive(true);
		REQUIRE(node->activeSelf() == true); // 即座に反映される
		
		node->setActive(false);
		REQUIRE(node->activeSelf() == false); // 即座に反映される
		
		// interactableのテスト（パラメータ参照なし）
		node->setInteractable(false);
		REQUIRE(node->interactable() == false); // 即座に反映される
		
		node->setInteractable(true);
		REQUIRE(node->interactable() == true); // 即座に反映される
		
		// styleStateのテスト（パラメータ参照なし）
		node->setStyleState(U"hover");
		REQUIRE(node->styleState() == U"hover"); // 即座に反映される
		
		node->setStyleState(U"pressed");
		REQUIRE(node->styleState() == U"pressed"); // 即座に反映される
		
		// zOrderInSiblingsのテスト（パラメータ参照なし） - Propertyなので即時反映される
		node->setZOrderInSiblings(noco::PropertyValue<int32>{ 10 });
		REQUIRE(node->zOrderInSiblings() == 10); // 即座に反映される
		
		node->setZOrderInSiblings(noco::PropertyValue<int32>{ -5 });
		REQUIRE(node->zOrderInSiblings() == -5); // 即座に反映される
		
		node->setStyleState(U"");
		REQUIRE(node->styleState() == U""); // 即座に反映される
	}
	
	SECTION("PropertyNonInteractive parameter reference")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		
		// パラメータを設定
		canvas->setParamValue(U"nodeActive", false);
		canvas->setParamValue(U"nodeInteractable", false);
		canvas->setParamValue(U"nodeStyle", U"selected");
		
		// ノードを作成
		auto node = canvas->emplaceChild(U"TestNode");
		
		// デフォルト値を設定
		node->setActive(true);
		node->setInteractable(true);
		node->setStyleState(U"");
		
		// パラメータ参照を設定
		node->setActiveSelfParamRef(U"nodeActive");
		node->setInteractableParamRef(U"nodeInteractable");
		node->setStyleStateParamRef(U"nodeStyle");
		
		// update前はまだパラメータ値が適用されない
		REQUIRE(node->activeSelf() == true);
		REQUIRE(node->interactable() == true);
		REQUIRE(node->styleState() == U"");
		
		// updateするとパラメータ値が適用される
		canvas->update();
		REQUIRE(node->activeSelf() == false);
		REQUIRE(node->interactable() == false);
		REQUIRE(node->styleState() == U"selected");
		
		// パラメータ値を変更
		canvas->setParamValue(U"nodeActive", true);
		canvas->setParamValue(U"nodeInteractable", true);
		canvas->setParamValue(U"nodeStyle", U"hover");
		canvas->update();
		REQUIRE(node->activeSelf() == true);
		REQUIRE(node->interactable() == true);
		REQUIRE(node->styleState() == U"hover");
		
		// パラメータ参照を削除
		node->setActiveSelfParamRef(U"");
		node->setInteractableParamRef(U"");
		node->setStyleStateParamRef(U"");
		
		System::Update();  // フレームを進める
		canvas->update();
		
		// デフォルト値に戻る
		REQUIRE(node->activeSelf() == true);
		REQUIRE(node->interactable() == true);
		REQUIRE(node->styleState() == U"");
	}
	
	SECTION("PropertyNonInteractive enum parameter reference")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		
		// String型パラメータでEnum値を設定
		canvas->setParamValue(U"shapeType", U"Pentagon");
		
		// ノードを作成
		auto node = canvas->emplaceChild(U"TestNode");
		auto shapeRenderer = std::make_shared<noco::ShapeRenderer>();
		node->addComponent(shapeRenderer);
		
		// デフォルト値を確認（ShapeRendererのデフォルトはStar）
		REQUIRE(shapeRenderer->shapeType() == noco::ShapeType::Star);
		
		// PropertyNonInteractiveプロパティにアクセス
		auto* shapeTypeProp = shapeRenderer->getPropertyByName(U"shapeType");
		REQUIRE(shapeTypeProp != nullptr);
		
		// パラメータ参照を設定
		shapeTypeProp->setParamRef(U"shapeType");
		
		// update前はまだパラメータ値が適用されない
		REQUIRE(shapeRenderer->shapeType() == noco::ShapeType::Star);
		
		// updateするとパラメータ値が適用される
		canvas->update();
		REQUIRE(shapeRenderer->shapeType() == noco::ShapeType::Pentagon);
		
		// パラメータ値を変更
		canvas->setParamValue(U"shapeType", U"Hexagon");
		canvas->update();
		REQUIRE(shapeRenderer->shapeType() == noco::ShapeType::Hexagon);
		
		// パラメータ参照を削除
		shapeTypeProp->setParamRef(U"");
		
		System::Update();  // フレームを進める
		canvas->update();
		
		// デフォルト値に戻る
		REQUIRE(shapeRenderer->shapeType() == noco::ShapeType::Star);
	}
}

TEST_CASE("Canvas interactable property", "[Canvas]")
{
	SECTION("Canvas interactable affects child nodes immediately")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create(U"TestNode");
		auto rectRenderer = std::make_shared<noco::RectRenderer>()
			->setFillColor(noco::PropertyValue<Color>{ Palette::White }
				.withDisabled(Palette::Gray));
		node->addComponent(rectRenderer);
		canvas->addChild(node);
		
		// デフォルトはinteractable=true
		REQUIRE(canvas->interactable() == true);
		REQUIRE(node->currentInteractionState() != noco::InteractionState::Disabled);
		
		// Canvasをdisableすると子ノードも即座にdisabledになる
		canvas->setInteractable(false);
		REQUIRE(canvas->interactable() == false);
		REQUIRE(node->currentInteractionState() == noco::InteractionState::Disabled);
		
		// プロパティ値も即座に更新される（deltaTime=0で適用）
		auto* fillColorProp = dynamic_cast<noco::SmoothProperty<Color>*>(rectRenderer->getPropertyByName(U"fillColor"));
		REQUIRE(fillColorProp != nullptr);
		REQUIRE(fillColorProp->value() == Color{ Palette::Gray });
		
		// Canvasを再度enableすると子ノードもenabledになる
		canvas->setInteractable(true);
		REQUIRE(canvas->interactable() == true);
		REQUIRE(node->currentInteractionState() == noco::InteractionState::Default);
		REQUIRE(fillColorProp->value() == Color{ Palette::White });
	}
	
	SECTION("Canvas interactable with node hierarchy")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create(U"Parent");
		auto child = noco::Node::Create(U"Child");
		auto grandchild = noco::Node::Create(U"Grandchild");
		
		canvas->addChild(parent);
		parent->addChild(child);
		child->addChild(grandchild);
		
		// 全てenabledな状態
		REQUIRE(parent->currentInteractionState() == noco::InteractionState::Default);
		REQUIRE(child->currentInteractionState() == noco::InteractionState::Default);
		REQUIRE(grandchild->currentInteractionState() == noco::InteractionState::Default);
		
		// Canvasをdisableすると全ての子孫がdisabledになる
		canvas->setInteractable(false);
		REQUIRE(parent->currentInteractionState() == noco::InteractionState::Disabled);
		REQUIRE(child->currentInteractionState() == noco::InteractionState::Disabled);
		REQUIRE(grandchild->currentInteractionState() == noco::InteractionState::Disabled);
		
		// 中間ノードをdisableしてからCanvasをenableしても、中間ノード以下はdisabledのまま
		child->setInteractable(false);
		canvas->setInteractable(true);
		REQUIRE(parent->currentInteractionState() == noco::InteractionState::Default);
		REQUIRE(child->currentInteractionState() == noco::InteractionState::Disabled);
		REQUIRE(grandchild->currentInteractionState() == noco::InteractionState::Disabled);
	}
	
	SECTION("Canvas interactable preserves hover/pressed state")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create(U"TestNode");
		canvas->addChild(node);
		
		canvas->setInteractable(false);
		REQUIRE(node->currentInteractionState() == noco::InteractionState::Disabled);
		
		canvas->setInteractable(true);
		REQUIRE(node->currentInteractionState() == noco::InteractionState::Default);
	}
	
	SECTION("Node setInteractable considers Canvas state")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create(U"TestNode");
		canvas->addChild(node);
		
		// Canvasがdisabledの状態でノードをenableしても、ノードはdisabledのまま
		canvas->setInteractable(false);
		node->setInteractable(true);
		REQUIRE(node->currentInteractionState() == noco::InteractionState::Disabled);
		
		canvas->setInteractable(true);
		REQUIRE(node->currentInteractionState() == noco::InteractionState::Default);
		
		// ノードをdisableしてからCanvasをdisable→enableしても、ノードはdisabledのまま
		node->setInteractable(false);
		canvas->setInteractable(false);
		canvas->setInteractable(true);
		REQUIRE(node->currentInteractionState() == noco::InteractionState::Disabled);
	}
}

// ========================================
// Canvas zOrderInSiblingsのテスト
// ========================================

TEST_CASE("Canvas top-level nodes zOrderInSiblings basic properties", "[Canvas][ZIndex]")
{
	SECTION("Default zOrderInSiblings value for top-level nodes")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create(U"TestNode");
		canvas->addChild(node);
		REQUIRE(node->zOrderInSiblings() == 0);
	}

	SECTION("Set and get zOrderInSiblings for top-level nodes")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create(U"TestNode");
		canvas->addChild(node);
		
		// 基本的な値設定
		node->setZOrderInSiblings(noco::PropertyValue<int32>{ 5 });
		REQUIRE(node->zOrderInSiblings() == 5);
		
		// 負の値も設定可能
		node->setZOrderInSiblings(noco::PropertyValue<int32>{ -10 });
		REQUIRE(node->zOrderInSiblings() == -10);
	}

	SECTION("zOrderInSiblings affects draw order for top-level nodes")
	{
		auto canvas = noco::Canvas::Create();
		
		// 3つのトップレベルノードを作成、ZIndexを設定
		auto node1 = noco::Node::Create(U"Node1");
		auto node2 = noco::Node::Create(U"Node2");
		auto node3 = noco::Node::Create(U"Node3");
		
		node1->setZOrderInSiblings(noco::PropertyValue<int32>{ 10 });  // 最前面
		node2->setZOrderInSiblings(noco::PropertyValue<int32>{ 5 });   // 中間
		node3->setZOrderInSiblings(noco::PropertyValue<int32>{ 1 });   // 最背面
		
		canvas->addChild(node3);  // 最背面を最初に追加
		canvas->addChild(node1);  // 最前面を次に追加
		canvas->addChild(node2);  // 中間を最後に追加
		
		// プロパティ値が正しく設定されていることを確認
		REQUIRE(node1->zOrderInSiblings() == 10);
		REQUIRE(node2->zOrderInSiblings() == 5);
		REQUIRE(node3->zOrderInSiblings() == 1);
		
		// ZIndexの順序が保たれることを期待（描画順序はCanvas内部で制御される）
		REQUIRE(node1->zOrderInSiblings() > node2->zOrderInSiblings());
		REQUIRE(node2->zOrderInSiblings() > node3->zOrderInSiblings());
	}
}

TEST_CASE("Canvas top-level nodes zOrderInSiblings with styleState", "[Canvas][ZIndex][StyleState]")
{
	SECTION("zOrderInSiblings with different styleState values for top-level nodes")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create(U"TestNode");
		canvas->addChild(node);
		
		// デフォルト値とstyleState毎の値を設定
		auto zIndexProperty = noco::PropertyValue<int32>{ 0 }  // デフォルト値
			.withStyleState(U"highlighted", 10)           // styleState "highlighted"時は10
			.withStyleState(U"selected", 20);             // styleState "selected"時は20
		
		node->setZOrderInSiblings(zIndexProperty);
		
		// 初期状態（デフォルト値）
		canvas->update();
		REQUIRE(node->zOrderInSiblings() == 0);
		
		// styleStateを"highlighted"に設定
		node->setStyleState(U"highlighted");
		canvas->update();
		REQUIRE(node->zOrderInSiblings() == 10);
		
		// styleStateを"selected"に設定  
		node->setStyleState(U"selected");
		canvas->update();
		REQUIRE(node->zOrderInSiblings() == 20);
		
		// styleStateをクリア（デフォルト値に戻る）
		node->clearStyleState();
		canvas->update();
		REQUIRE(node->zOrderInSiblings() == 0);
	}
}

TEST_CASE("Canvas top-level nodes zOrderInSiblings with parameter reference", "[Canvas][ZIndex][Param]")
{
	SECTION("zOrderInSiblings with parameter reference for top-level nodes")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create(U"TestNode");
		canvas->addChild(node);
		
		// パラメータ参照を設定
		node->setZOrderInSiblingsParamRef(U"layerIndex");
		
		// 初期状態（パラメータなし）
		canvas->update();
		REQUIRE(node->zOrderInSiblings() == 0);  // デフォルト値
		
		// パラメータを設定
		canvas->setParamValue(U"layerIndex", 15);
		canvas->update();
		REQUIRE(node->zOrderInSiblings() == 15);
		
		// パラメータ値を変更
		canvas->setParamValue(U"layerIndex", -5);
		canvas->update();
		REQUIRE(node->zOrderInSiblings() == -5);
		
		// パラメータ参照をクリア
		node->setZOrderInSiblingsParamRef(U"");
		System::Update();  // フレームを進める
		canvas->update();
		REQUIRE(node->zOrderInSiblings() == 0);  // デフォルト値に戻る
	}

	SECTION("zOrderInSiblings parameter reference with styleState for top-level nodes")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create(U"TestNode");
		canvas->addChild(node);
		
		// styleState毎の値とパラメータ参照を併用
		auto zIndexProperty = noco::PropertyValue<int32>{ 1 }  // デフォルト値
			.withStyleState(U"active", 5);                // styleState "active"時は5
		
		node->setZOrderInSiblings(zIndexProperty);
		node->setZOrderInSiblingsParamRef(U"dynamicLayer");
		
		// パラメータが設定されている場合はパラメータ値が優先される
		canvas->setParamValue(U"dynamicLayer", 100);
		canvas->update();
		REQUIRE(node->zOrderInSiblings() == 100);
		
		// styleStateを設定してもパラメータ値が優先される
		node->setStyleState(U"active");
		canvas->update();
		REQUIRE(node->zOrderInSiblings() == 100);  // パラメータ値が優先
		
		// パラメータを削除するとstyleStateの値が使用される
		canvas->removeParam(U"dynamicLayer");
		System::Update();  // フレームを進める
		canvas->update();
		REQUIRE(node->zOrderInSiblings() == 5);  // styleState "active"の値
		
		// styleStateをクリアするとデフォルト値が使用される
		node->clearStyleState();
		canvas->update();
		REQUIRE(node->zOrderInSiblings() == 1);  // デフォルト値
	}
}
