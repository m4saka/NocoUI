# include <catch2/catch.hpp>
# include <Siv3D.hpp>
# include <NocoUI.hpp>

// ========================================
// イベント処理テスト
// ========================================

TEST_CASE("Event handling", "[Events]")
{
	SECTION("EventTrigger - Click events")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create();
		node->setConstraint(noco::BoxConstraint{ .sizeDelta = Vec2{ 100, 100 } });
		
		// EventTriggerコンポーネントを追加
		auto trigger = node->emplaceComponent<noco::EventTrigger>(U"testButton", noco::EventTriggerType::Click);
		
		canvas->rootNode()->addChild(node);
		
		// requestClickメソッドでクリックをシミュレート
		node->requestClick();
		canvas->update();
		
		// isClickedの確認(updateを呼ぶことでrequestClickが反映される)
		REQUIRE(node->isClicked());
		
		// イベントが発火したか確認
		REQUIRE(canvas->isEventFiredWithTag(U"testButton"));
		auto event = canvas->getFiredEventWithTag(U"testButton");
		REQUIRE(event.has_value());
		REQUIRE(event->triggerType == noco::EventTriggerType::Click);
		REQUIRE(event->sourceNode.lock() == node);
	}
	
	SECTION("EventTrigger - Right click events")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create();
		node->setConstraint(noco::BoxConstraint{ .sizeDelta = Vec2{ 100, 100 } });
		
		// ClickトリガーのEventTriggerを追加（右クリックも通常のClickイベントとして扱われる）
		auto trigger = node->emplaceComponent<noco::EventTrigger>(U"rightClickTest", noco::EventTriggerType::RightClick);
		
		canvas->rootNode()->addChild(node);
		
		// requestRightClickメソッドで右クリックをシミュレート
		node->requestRightClick();
		canvas->update();
		
		// isRightClickedの確認(updateを呼ぶことでrequestRightClickが反映される)
		REQUIRE(node->isRightClicked());
		
		// イベントが発火したか確認
		REQUIRE(canvas->isEventFiredWithTag(U"rightClickTest"));
	}
	
	SECTION("EventTrigger - Multiple nodes with events")
	{
		auto canvas = noco::Canvas::Create();
		auto node1 = noco::Node::Create(U"Node1");
		auto node2 = noco::Node::Create(U"Node2");
		
		node1->setConstraint(noco::BoxConstraint{ .sizeDelta = Vec2{ 100, 100 } });
		node2->setConstraint(noco::BoxConstraint{ .sizeDelta = Vec2{ 100, 100 } });
		
		// 異なるタグでEventTriggerを設定
		node1->emplaceComponent<noco::EventTrigger>(U"button1");
		node2->emplaceComponent<noco::EventTrigger>(U"button2");
		
		canvas->rootNode()->addChild(node1);
		canvas->rootNode()->addChild(node2);
		
		// node1をクリック
		node1->requestClick();
		canvas->update();
		
		REQUIRE(canvas->isEventFiredWithTag(U"button1"));
		REQUIRE_FALSE(canvas->isEventFiredWithTag(U"button2"));
		
		// node2をクリック
		node2->requestClick();
		canvas->update();
		
		// 前のフレームのイベントはクリアされているはず
		REQUIRE_FALSE(canvas->isEventFiredWithTag(U"button1"));
		REQUIRE(canvas->isEventFiredWithTag(U"button2"));
	}
	
	SECTION("EventTrigger - Nested nodes with click")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create(U"Parent");
		auto child = noco::Node::Create(U"Child");
		
		parent->setConstraint(noco::BoxConstraint{ .sizeDelta = Vec2{ 200, 200 } });
		child->setConstraint(noco::BoxConstraint{ .sizeDelta = Vec2{ 100, 100 } });
		
		// 子ノードにEventTriggerを設定
		child->emplaceComponent<noco::EventTrigger>(U"childButton");
		
		canvas->rootNode()->addChild(parent);
		parent->addChild(child);
		
		// 子ノードをクリック
		child->requestClick();
		canvas->update();
		
		REQUIRE(canvas->isEventFiredWithTag(U"childButton"));
	}
	
	SECTION("EventTrigger - Get all fired events")
	{
		auto canvas = noco::Canvas::Create();
		auto node1 = noco::Node::Create();
		auto node2 = noco::Node::Create();
		auto node3 = noco::Node::Create();
		
		node1->setConstraint(noco::BoxConstraint{ .sizeDelta = Vec2{ 100, 100 } });
		node2->setConstraint(noco::BoxConstraint{ .sizeDelta = Vec2{ 100, 100 } });
		node3->setConstraint(noco::BoxConstraint{ .sizeDelta = Vec2{ 100, 100 } });
		
		// EventTriggerを設定
		node1->emplaceComponent<noco::EventTrigger>(U"button1");
		node2->emplaceComponent<noco::EventTrigger>(U"button2");
		node3->emplaceComponent<noco::EventTrigger>(U"button3");
		
		canvas->rootNode()->addChild(node1);
		canvas->rootNode()->addChild(node2);
		canvas->rootNode()->addChild(node3);
		
		// 複数のノードをクリック（同一フレーム内）
		node1->requestClick();
		node3->requestClick();
		canvas->update();
		
		// 全てのイベントを取得
		const auto& allEvents = canvas->getFiredEventsAll();
		REQUIRE(allEvents.size() == 2);
		
		// タグでイベントを確認
		REQUIRE(canvas->isEventFiredWithTag(U"button1"));
		REQUIRE_FALSE(canvas->isEventFiredWithTag(U"button2"));
		REQUIRE(canvas->isEventFiredWithTag(U"button3"));
	}
}