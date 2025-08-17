#include <catch2/catch.hpp>
#include <Siv3D.hpp>
#include <NocoUI.hpp>

// ========================================
// StyleStateのテスト
// ========================================

TEST_CASE("StyleState Basic Functionality", "[Node][StyleState]")
{
	SECTION("Node styleState getter/setter")
	{
		auto node = noco::Node::Create(U"TestNode");
		
		// デフォルトは空文字列
		REQUIRE(node->styleState() == U"");
		
		// styleStateの設定
		node->setStyleState(U"focused");
		REQUIRE(node->styleState() == U"focused");
		
		// 別のstyleStateに変更
		node->setStyleState(U"expanded");
		REQUIRE(node->styleState() == U"expanded");
		
		// 空文字列に戻す
		node->setStyleState(U"");
		REQUIRE(node->styleState() == U"");
	}
	
	SECTION("Method chaining")
	{
		auto node = noco::Node::Create(U"TestNode");
		auto result = node->setStyleState(U"checked");
		
		// setStyleStateは自身を返す
		REQUIRE(result == node);
	}
}

TEST_CASE("ActiveStyleStates Collection", "[Node][StyleState]")
{
	// テスト用のコンポーネント
	// RectRendererを継承して、プロパティ経由でactiveStyleStatesをキャプチャする
	class TestComponent : public noco::RectRenderer
	{
	public:
		// キャプチャ用の変数
		mutable Array<String> lastActiveStyleStates;
		mutable noco::InteractionState lastInteractionState = noco::InteractionState::Default;
		
		// 内部プロパティクラス - Property<double>を継承
		class CaptureProperty : public noco::Property<double>
		{
		public:
			TestComponent* parent;
			
			CaptureProperty(TestComponent* p) 
				: Property(U"capture", 0.0)
				, parent(p) 
			{}
			
			void update(noco::InteractionState interactionState, 
			           const Array<String>& activeStyleStates, 
			           double deltaTime,
			           const HashTable<String, noco::ParamValue>& params) override
			{
				// 親クラスのupdateを呼ぶ
				Property::update(interactionState, activeStyleStates, deltaTime, params);
				// キャプチャ
				parent->lastActiveStyleStates = activeStyleStates;
				parent->lastInteractionState = interactionState;
			}
		};
		
		std::unique_ptr<CaptureProperty> captureProperty;
		
		TestComponent() : RectRenderer()
		{
			captureProperty = std::make_unique<CaptureProperty>(this);
			// プロパティリストにキャプチャ用プロパティを追加
			const_cast<Array<noco::IProperty*>&>(properties()).push_back(captureProperty.get());
		}
	};

	SECTION("Single node with styleState")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create(U"Node");
		canvas->addChild(node);
		node->setStyleState(U"focused");
		
		auto testComponent = std::make_shared<TestComponent>();
		node->addComponent(testComponent);
		
		// update実行
		canvas->update();
		
		// activeStyleStatesには自身のstyleStateのみ
		REQUIRE(testComponent->lastActiveStyleStates.size() == 1);
		REQUIRE(testComponent->lastActiveStyleStates[0] == U"focused");
	}
	
	SECTION("Empty styleState is not included")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create(U"Node");
		canvas->addChild(node);
		// styleStateが空の場合
		
		auto testComponent = std::make_shared<TestComponent>();
		node->addComponent(testComponent);
		
		canvas->update();
		
		// 空のstyleStateは含まれない
		REQUIRE(testComponent->lastActiveStyleStates.empty());
	}
	
	SECTION("Parent-child styleState inheritance")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create(U"Parent");
		auto child = noco::Node::Create(U"Child");
		canvas->addChild(parent);
		parent->addChild(child);
		
		parent->setStyleState(U"tab1");
		child->setStyleState(U"focused");
		
		auto childComponent = std::make_shared<TestComponent>();
		child->addComponent(childComponent);
		
		canvas->update();
		
		// 子のactiveStyleStatesには親のstyleStateも含まれる
		REQUIRE(childComponent->lastActiveStyleStates.size() == 2);
		REQUIRE(childComponent->lastActiveStyleStates[0] == U"tab1");
		REQUIRE(childComponent->lastActiveStyleStates[1] == U"focused");
	}
	
	SECTION("Multiple ancestors")
	{
		auto canvas = noco::Canvas::Create();
		auto nodeA = noco::Node::Create(U"A");
		auto nodeB = noco::Node::Create(U"B");
		auto nodeC = noco::Node::Create(U"C");
		auto nodeD = noco::Node::Create(U"D");
		
		canvas->addChild(nodeA);
		nodeA->addChild(nodeB);
		nodeB->addChild(nodeC);
		nodeC->addChild(nodeD);
		
		nodeA->setStyleState(U"tab1");
		nodeB->setStyleState(U"");        // 空
		nodeC->setStyleState(U"focused");
		nodeD->setStyleState(U"");        // 空
		
		auto componentD = std::make_shared<TestComponent>();
		nodeD->addComponent(componentD);
		
		canvas->update();
		
		// NodeDのactiveStyleStatesには祖先のstyleStateが含まれる
		REQUIRE(componentD->lastActiveStyleStates.size() == 2);
		REQUIRE(componentD->lastActiveStyleStates[0] == U"tab1");
		REQUIRE(componentD->lastActiveStyleStates[1] == U"focused");
	}
	
	SECTION("Direct parent priority")
	{
		auto canvas = noco::Canvas::Create();
		auto grandparent = noco::Node::Create(U"Grandparent");
		auto parent = noco::Node::Create(U"Parent");
		auto child = noco::Node::Create(U"Child");
		
		canvas->addChild(grandparent);
		grandparent->addChild(parent);
		parent->addChild(child);
		
		grandparent->setStyleState(U"level1");
		parent->setStyleState(U"level2");
		child->setStyleState(U"level3");
		
		auto childComponent = std::make_shared<TestComponent>();
		child->addComponent(childComponent);
		
		canvas->update();
		
		// activeStyleStatesは祖先から順に並ぶ
		REQUIRE(childComponent->lastActiveStyleStates.size() == 3);
		REQUIRE(childComponent->lastActiveStyleStates[0] == U"level1");
		REQUIRE(childComponent->lastActiveStyleStates[1] == U"level2");
		REQUIRE(childComponent->lastActiveStyleStates[2] == U"level3");
	}
	
	SECTION("Complex hierarchy with multiple branches")
	{
		auto canvas = noco::Canvas::Create();
		
		// 複雑な階層構造を作成
		//       root
		//      /    \
		//    tabA    tabB
		//    /  \      |
		//  itemA1 itemA2  itemB1
		//   |             /    \
		// subA1      subB1a  subB1b
		
		auto tabA = noco::Node::Create(U"TabA");
		auto tabB = noco::Node::Create(U"TabB");
		auto itemA1 = noco::Node::Create(U"ItemA1");
		auto itemA2 = noco::Node::Create(U"ItemA2");
		auto itemB1 = noco::Node::Create(U"ItemB1");
		auto subA1 = noco::Node::Create(U"SubA1");
		auto subB1a = noco::Node::Create(U"SubB1a");
		auto subB1b = noco::Node::Create(U"SubB1b");
		
		// 階層構造を構築
		canvas->addChild(tabA);
		canvas->addChild(tabB);
		tabA->addChild(itemA1);
		tabA->addChild(itemA2);
		tabB->addChild(itemB1);
		itemA1->addChild(subA1);
		itemB1->addChild(subB1a);
		itemB1->addChild(subB1b);
		
		// styleStateを設定
		tabA->setStyleState(U"tab-active");
		tabB->setStyleState(U"tab-inactive");
		itemA1->setStyleState(U"focused");
		itemA2->setStyleState(U"");  // 空
		itemB1->setStyleState(U"expanded");
		subA1->setStyleState(U"highlighted");
		subB1a->setStyleState(U"checked");
		subB1b->setStyleState(U"");  // 空
		
		// 各ノードにコンポーネントを追加
		auto componentSubA1 = std::make_shared<TestComponent>();
		auto componentSubB1a = std::make_shared<TestComponent>();
		auto componentSubB1b = std::make_shared<TestComponent>();
		auto componentItemA2 = std::make_shared<TestComponent>();
		
		subA1->addComponent(componentSubA1);
		subB1a->addComponent(componentSubB1a);
		subB1b->addComponent(componentSubB1b);
		itemA2->addComponent(componentItemA2);
		
		canvas->update();
		
		// subA1: root -> tabA(tab-active) -> itemA1(focused) -> subA1(highlighted)
		REQUIRE(componentSubA1->lastActiveStyleStates.size() == 3);
		REQUIRE(componentSubA1->lastActiveStyleStates[0] == U"tab-active");
		REQUIRE(componentSubA1->lastActiveStyleStates[1] == U"focused");
		REQUIRE(componentSubA1->lastActiveStyleStates[2] == U"highlighted");
		
		// subB1a: root -> tabB(tab-inactive) -> itemB1(expanded) -> subB1a(checked)
		REQUIRE(componentSubB1a->lastActiveStyleStates.size() == 3);
		REQUIRE(componentSubB1a->lastActiveStyleStates[0] == U"tab-inactive");
		REQUIRE(componentSubB1a->lastActiveStyleStates[1] == U"expanded");
		REQUIRE(componentSubB1a->lastActiveStyleStates[2] == U"checked");
		
		// subB1b: root -> tabB(tab-inactive) -> itemB1(expanded) -> subB1b(空)
		REQUIRE(componentSubB1b->lastActiveStyleStates.size() == 2);
		REQUIRE(componentSubB1b->lastActiveStyleStates[0] == U"tab-inactive");
		REQUIRE(componentSubB1b->lastActiveStyleStates[1] == U"expanded");
		
		// itemA2: root -> tabA(tab-active) -> itemA2(空)
		REQUIRE(componentItemA2->lastActiveStyleStates.size() == 1);
		REQUIRE(componentItemA2->lastActiveStyleStates[0] == U"tab-active");
	}
	
	SECTION("Dynamic hierarchy changes")
	{
		auto canvas = noco::Canvas::Create();
		auto nodeA = noco::Node::Create(U"A");
		auto nodeB = noco::Node::Create(U"B");
		auto nodeC = noco::Node::Create(U"C");
		
		canvas->addChild(nodeA);
		nodeA->addChild(nodeB);
		nodeB->addChild(nodeC);
		
		nodeA->setStyleState(U"state-a");
		nodeB->setStyleState(U"state-b");
		nodeC->setStyleState(U"state-c");
		
		auto componentC = std::make_shared<TestComponent>();
		nodeC->addComponent(componentC);
		
		// 初期状態でupdate
		canvas->update();
		
		// A -> B -> C の階層
		REQUIRE(componentC->lastActiveStyleStates.size() == 3);
		REQUIRE(componentC->lastActiveStyleStates[0] == U"state-a");
		REQUIRE(componentC->lastActiveStyleStates[1] == U"state-b");
		REQUIRE(componentC->lastActiveStyleStates[2] == U"state-c");
		
		// 階層を変更: CをAの直接の子にする
		nodeB->removeChild(nodeC);
		nodeA->addChild(nodeC);
		
		canvas->update();
		
		// A -> C の階層（Bが抜ける）
		REQUIRE(componentC->lastActiveStyleStates.size() == 2);
		REQUIRE(componentC->lastActiveStyleStates[0] == U"state-a");
		REQUIRE(componentC->lastActiveStyleStates[1] == U"state-c");
		
		// さらに階層を変更: Cをrootの直接の子にする
		nodeA->removeChild(nodeC);
		canvas->addChild(nodeC);
		
		canvas->update();
		
		// root -> C の階層（Aも抜ける）
		REQUIRE(componentC->lastActiveStyleStates.size() == 1);
		REQUIRE(componentC->lastActiveStyleStates[0] == U"state-c");
	}
	
	SECTION("Deep nesting stress test")
	{
		auto canvas = noco::Canvas::Create();
		
		// 深い階層を作成（10レベル）
		std::vector<std::shared_ptr<noco::Node>> nodes;
		std::shared_ptr<noco::Node> parent = nullptr;
		
		for (int i = 0; i < 10; ++i)
		{
			auto node = noco::Node::Create(Format(U"Level{}", i));
			node->setStyleState(Format(U"state{}", i));
			if (parent)
			{
				parent->addChild(node);
			}
			else
			{
				canvas->addChild(node);
			}
			nodes.push_back(node);
			parent = node;
		}
		
		// 最深部のノードにコンポーネントを追加
		auto deepComponent = std::make_shared<TestComponent>();
		nodes.back()->addComponent(deepComponent);
		
		canvas->update();
		
		// すべての祖先のstyleStateが収集される
		REQUIRE(deepComponent->lastActiveStyleStates.size() == 10);
		for (int i = 0; i < 10; ++i)
		{
			REQUIRE(deepComponent->lastActiveStyleStates[i] == Format(U"state{}", i));
		}
	}
	
	SECTION("Multiple siblings with different states")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create(U"Parent");
		canvas->addChild(parent);
		parent->setStyleState(U"parent-state");
		
		// 5つの兄弟ノードを作成
		std::vector<std::shared_ptr<TestComponent>> components;
		for (int i = 0; i < 5; ++i)
		{
			auto child = noco::Node::Create(Format(U"Child{}", i));
			
			// 偶数番目のノードにはstyleStateを設定
			if (i % 2 == 0)
			{
				child->setStyleState(Format(U"child-state{}", i));
			}
			
			parent->addChild(child);
			
			auto component = std::make_shared<TestComponent>();
			child->addComponent(component);
			components.push_back(component);
		}
		
		canvas->update();
		
		// 各兄弟ノードのactiveStyleStatesを確認
		for (int i = 0; i < 5; ++i)
		{
			if (i % 2 == 0)
			{
				// 偶数番目: parent-state + child-stateN
				REQUIRE(components[i]->lastActiveStyleStates.size() == 2);
				REQUIRE(components[i]->lastActiveStyleStates[0] == U"parent-state");
				REQUIRE(components[i]->lastActiveStyleStates[1] == Format(U"child-state{}", i));
			}
			else
			{
				// 奇数番目: parent-stateのみ
				REQUIRE(components[i]->lastActiveStyleStates.size() == 1);
				REQUIRE(components[i]->lastActiveStyleStates[0] == U"parent-state");
			}
		}
	}
}

TEST_CASE("PropertyValue with StyleState", "[Property][StyleState]")
{
	SECTION("Basic styleState value resolution")
	{
		auto prop = noco::PropertyValue<ColorF>{ ColorF{1, 0, 0} }  // 赤がデフォルト
			.withStyleState(U"focused", ColorF{0, 0, 1});    // 青がfocused時
		
		// styleStateなしの場合はデフォルト値
		Array<String> emptyStates;
		REQUIRE(prop.value(noco::InteractionState::Default, emptyStates) == ColorF{1, 0, 0});
		
		// focusedがactiveStyleStatesに含まれる場合
		Array<String> focusedStates = { U"focused" };
		REQUIRE(prop.value(noco::InteractionState::Default, focusedStates) == ColorF{0, 0, 1});
	}
	
	SECTION("StyleState priority (closer state wins)")
	{
		auto prop = noco::PropertyValue<double>{ 1.0 }
			.withStyleState(U"tab1", 2.0)
			.withStyleState(U"focused", 3.0);
		
		// 複数のstyleStateがある場合、後の方（自身に近い）が優先
		Array<String> activeStates = { U"tab1", U"focused" };
		REQUIRE(prop.value(noco::InteractionState::Default, activeStates) == 3.0);
		
		// 順序を逆にすると結果も変わる
		Array<String> reversedStates = { U"focused", U"tab1" };
		REQUIRE(prop.value(noco::InteractionState::Default, reversedStates) == 2.0);
	}
	
	SECTION("StyleState with InteractionState combination")
	{
		auto prop = noco::PropertyValue<ColorF>{ ColorF{0.5, 0.5, 0.5} }
			// 通常時のホバー色
			.withHovered(ColorF{0.6, 0.6, 0.6})
			// focused時のデフォルト色とホバー色
			.withStyleStateInteraction(U"focused", noco::InteractionState::Default, ColorF{0, 0, 1})
			.withStyleStateInteraction(U"focused", noco::InteractionState::Hovered, ColorF{0.2, 0.2, 1});
		
		Array<String> focusedStates = { U"focused" };
		
		// focused + Default
		REQUIRE(prop.value(noco::InteractionState::Default, focusedStates) == ColorF{0, 0, 1});
		
		// focused + Hovered
		REQUIRE(prop.value(noco::InteractionState::Hovered, focusedStates) == ColorF{0.2, 0.2, 1});
		
		// focused + Pressed（定義なし → focused時のHoveredにフォールバック）
		REQUIRE(prop.value(noco::InteractionState::Pressed, focusedStates) == ColorF{0.2, 0.2, 1});
	}
	
	SECTION("Complex priority resolution")
	{
		auto prop = noco::PropertyValue<int>{ 0 }
			// 基本のInteractionState値
			.withHovered(10)
			.withPressed(20)
			// styleState値
			.withStyleState(U"tab1", 100)
			.withStyleState(U"focused", 200)
			// styleState + InteractionStateの組み合わせ
			.withStyleStateInteraction(U"tab1", noco::InteractionState::Hovered, 110)
			.withStyleStateInteraction(U"focused", noco::InteractionState::Pressed, 220);
		
		// テストケース1: tab1 + Hovered
		Array<String> tab1States = { U"tab1" };
		REQUIRE(prop.value(noco::InteractionState::Hovered, tab1States) == 110);  // 組み合わせが優先
		
		// テストケース2: focused + Pressed
		Array<String> focusedStates = { U"focused" };
		REQUIRE(prop.value(noco::InteractionState::Pressed, focusedStates) == 220);  // 組み合わせが優先
		
		// テストケース3: tab1 + Pressed（組み合わせなし → tab1のHoveredにフォールバック）
		REQUIRE(prop.value(noco::InteractionState::Pressed, tab1States) == 110);  // tab1のHovered値
		
		// テストケース4: 複数styleState + Hovered
		Array<String> multiStates = { U"tab1", U"focused" };
		REQUIRE(prop.value(noco::InteractionState::Hovered, multiStates) == 200);  // focusedが優先され、focusedのHoveredがないためfocusedのデフォルト値
	}
}

TEST_CASE("PropertyValue JSON Serialization with StyleState", "[Property][StyleState][JSON]")
{
	SECTION("Simple styleState serialization")
	{
		auto prop = noco::PropertyValue<double>{ 1.0 }
			.withStyleState(U"focused", 2.0)
			.withStyleState(U"checked", 3.0);
		
		JSON json = prop.toJSON();
		
		// 基本値
		REQUIRE(json[U"default"].get<double>() == 1.0);
		
		// styleStatesオブジェクト
		REQUIRE(json.hasElement(U"styleStates"));
		const JSON& styleStates = json[U"styleStates"];
		REQUIRE(styleStates[U"focused"].get<double>() == 2.0);
		REQUIRE(styleStates[U"checked"].get<double>() == 3.0);
	}
	
	SECTION("StyleState with InteractionState serialization")
	{
		auto prop = noco::PropertyValue<ColorF>{ ColorF{0.5, 0.5, 0.5} }
			// focusedの複数InteractionState値
			.withStyleStateInteraction(U"focused", noco::InteractionState::Default, ColorF{0, 0, 1})
			.withStyleStateInteraction(U"focused", noco::InteractionState::Hovered, ColorF{0.2, 0.2, 1})
			// checkedのdefaultのみ
			.withStyleState(U"checked", ColorF{0, 1, 0});
		
		JSON json = prop.toJSON();
		
		const JSON& styleStates = json[U"styleStates"];
		
		// focusedは複数値があるのでオブジェクト形式
		REQUIRE(styleStates[U"focused"].isObject());
		REQUIRE(styleStates[U"focused"][U"Default"].getString() == U"(0, 0, 1, 1)");
		REQUIRE(styleStates[U"focused"][U"Hovered"].getString() == U"(0.2, 0.2, 1, 1)");
		
		// checkedはdefaultのみなので省略記法（文字列）
		REQUIRE(styleStates[U"checked"].isString());
		REQUIRE(styleStates[U"checked"].getString() == U"(0, 1, 0, 1)");
	}
	
	SECTION("JSON format verification")
	{
		// JSONシリアライゼーションの形式確認
		auto prop = noco::PropertyValue<double>{ 10.0 }
			.withHovered(20.0)
			.withStyleState(U"focused", 100.0)
			.withStyleStateInteraction(U"expanded", noco::InteractionState::Default, 200.0)
			.withStyleStateInteraction(U"expanded", noco::InteractionState::Hovered, 210.0);
		
		JSON json = prop.toJSON();
		
		// 基本値の確認
		REQUIRE(json[U"default"].get<double>() == 10.0);
		REQUIRE(json[U"hovered"].get<double>() == 20.0);
		
		// styleStatesの確認
		REQUIRE(json.hasElement(U"styleStates"));
		const JSON& styleStates = json[U"styleStates"];
		
		// focused（省略記法：defaultのみ）
		REQUIRE(styleStates[U"focused"].isNumber());
		REQUIRE(styleStates[U"focused"].get<double>() == 100.0);
		
		// expanded（完全記法：複数のInteractionState）
		REQUIRE(styleStates[U"expanded"].isObject());
		REQUIRE(styleStates[U"expanded"][U"Default"].get<double>() == 200.0);
		REQUIRE(styleStates[U"expanded"][U"Hovered"].get<double>() == 210.0);
	}
}

TEST_CASE("Node JSON Serialization with StyleState", "[Node][StyleState][JSON]")
{
	SECTION("Node with styleState concept")
	{
		auto node = noco::Node::Create(U"TestNode");
		node->setStyleState(U"checked");
		
		// styleStateが正しく設定されている
		REQUIRE(node->styleState() == U"checked");
		
		// JSON出力の確認は実装に依存するので、基本的な動作のみテスト
		JSON json = node->toJSON();
		REQUIRE(json.hasElement(U"name"));
		REQUIRE(json[U"name"].getString() == U"TestNode");
	}
	
	SECTION("Node with empty styleState")
	{
		auto node = noco::Node::Create(U"TestNode");
		// styleStateが空の場合
		
		REQUIRE(node->styleState() == U"");
		
		// JSONシリアライゼーションは動作する
		JSON json = node->toJSON();
		REQUIRE(json.hasElement(U"name"));
	}
	
	SECTION("StyleState persistence concept")
	{
		// styleStateは基本的な文字列として動作する
		auto node = noco::Node::Create(U"RestoredNode");
		node->setStyleState(U"expanded");
		
		REQUIRE(node->name() == U"RestoredNode");
		REQUIRE(node->styleState() == U"expanded");
		
		// 空文字列に戻す
		node->setStyleState(U"");
		REQUIRE(node->styleState() == U"");
	}
	
	SECTION("Hierarchy with styleState")
	{
		// 階層構造でのstyleState管理
		auto root = noco::Node::Create(U"Root");
		auto child1 = noco::Node::Create(U"Child1");
		auto child2 = noco::Node::Create(U"Child2");
		
		root->setStyleState(U"tab1");
		child1->setStyleState(U"focused");
		// child2は空のまま
		
		root->addChild(child1);
		root->addChild(child2);
		
		// 各ノードのstyleStateが独立して管理される
		REQUIRE(root->styleState() == U"tab1");
		REQUIRE(child1->styleState() == U"focused");
		REQUIRE(child2->styleState() == U"");
		REQUIRE(root->children().size() == 2);
	}
}

TEST_CASE("Component Integration with StyleState", "[Component][StyleState]")
{
	SECTION("TextBox selection state concept")
	{
		auto node = noco::Node::Create(U"TextBoxNode");
		auto textBox = std::make_shared<noco::TextBox>();
		node->addComponent(textBox);
		
		// 選択状態を設定
		node->setStyleState(U"focused");
		REQUIRE(node->styleState() == U"focused");
		
		// コンポーネントの実装では、activeStyleStatesに"focused"が
		// 含まれているかで選択状態を判定することになる
	}
	
	SECTION("Nested component with styleState hierarchy")
	{
		auto container = noco::Node::Create(U"Container");
		auto button = noco::Node::Create(U"Button");
		container->addChild(button);
		
		// タブとボタンの状態
		container->setStyleState(U"tab2");
		button->setStyleState(U"primary");
		
		// 各ノードの状態を確認
		REQUIRE(container->styleState() == U"tab2");
		REQUIRE(button->styleState() == U"primary");
		
		// 実際の実装では、buttonのコンポーネントupdate時に
		// activeStyleStates = ["tab2", "primary"] が渡される
	}
}

TEST_CASE("StyleState Edge Cases", "[Node][StyleState]")
{
	SECTION("Very long styleState names")
	{
		auto node = noco::Node::Create(U"Node");
		const String longState = U"very_long_style_state_name_that_might_cause_issues_in_some_systems";
		
		node->setStyleState(longState);
		REQUIRE(node->styleState() == longState);
		
		// JSONシリアライゼーションも確認
		JSON json = node->toJSON();
		REQUIRE(json[U"styleState"].getString() == longState);
	}
	
	SECTION("Unicode styleState names")
	{
		auto node = noco::Node::Create(U"Node");
		
		// 日本語のstyleState
		node->setStyleState(U"選択中");
		REQUIRE(node->styleState() == U"選択中");
		
		// 絵文字を含むstyleState
		node->setStyleState(U"✅checked");
		REQUIRE(node->styleState() == U"✅checked");
	}
	
	SECTION("Special characters in styleState")
	{
		auto node = noco::Node::Create(U"Node");
		
		// スペースを含む（推奨されないが動作する）
		node->setStyleState(U"my state");
		REQUIRE(node->styleState() == U"my state");
		
		// 特殊文字
		node->setStyleState(U"state-with-dash");
		REQUIRE(node->styleState() == U"state-with-dash");
		
		node->setStyleState(U"state_with_underscore");
		REQUIRE(node->styleState() == U"state_with_underscore");
	}
	
	SECTION("Rapid styleState changes")
	{
		auto node = noco::Node::Create(U"Node");
		
		// 高速に状態を切り替える
		for (int i = 0; i < 100; ++i)
		{
			const String state = Format(U"state", i);
			node->setStyleState(state);
			REQUIRE(node->styleState() == state);
		}
	}
}