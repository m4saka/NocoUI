# include <catch2/catch.hpp>
# include <Siv3D.hpp>
# include <NocoUI.hpp>

// ========================================
// Nodeの基本的なテスト
// ========================================

TEST_CASE("Node creation and basic properties", "[Node]")
{
	SECTION("Create empty node")
	{
		auto node = noco::Node::Create();
		REQUIRE(node != nullptr);
		REQUIRE(node->name() == U"Node");
		REQUIRE(node->children().empty());
		REQUIRE(node->parentNode() == nullptr);
	}

	SECTION("Create node with name")
	{
		auto node = noco::Node::Create(U"TestNode");
		REQUIRE(node->name() == U"TestNode");
	}
}

// Nodeの階層構造テスト
TEST_CASE("Node hierarchy", "[Node]")
{
	SECTION("Add child")
	{
		auto parent = noco::Node::Create(U"Parent");
		auto child = noco::Node::Create(U"Child");
		
		parent->addChild(child);
		
		REQUIRE(parent->children().size() == 1);
		REQUIRE(parent->children()[0] == child);
		REQUIRE(child->parentNode() == parent);
	}

	SECTION("Remove child")
	{
		auto parent = noco::Node::Create(U"Parent");
		auto child = noco::Node::Create(U"Child");
		
		parent->addChild(child);
		parent->removeChild(child);
		
		REQUIRE(parent->children().empty());
		REQUIRE(child->parentNode() == nullptr);
	}

	SECTION("Multiple children")
	{
		auto parent = noco::Node::Create(U"Parent");
		auto child1 = noco::Node::Create(U"Child1");
		auto child2 = noco::Node::Create(U"Child2");
		auto child3 = noco::Node::Create(U"Child3");
		
		parent->addChild(child1);
		parent->addChild(child2);
		parent->addChild(child3);
		
		REQUIRE(parent->children().size() == 3);
		REQUIRE(parent->children()[0] == child1);
		REQUIRE(parent->children()[1] == child2);
		REQUIRE(parent->children()[2] == child3);
	}
}

// Nodeのプロパティと状態管理
TEST_CASE("Node properties and state management", "[Node]")
{
	SECTION("Active state")
	{
		auto node = noco::Node::Create();
		
		// デフォルトではアクティブ
		REQUIRE(node->activeSelf() == noco::ActiveYN::Yes);
		
		// 非アクティブに設定
		node->setActive(noco::ActiveYN::No);
		REQUIRE(node->activeSelf() == noco::ActiveYN::No);
		
		// 再度アクティブに設定
		node->setActive(noco::ActiveYN::Yes);
		REQUIRE(node->activeSelf() == noco::ActiveYN::Yes);
	}

	SECTION("Active in hierarchy - Canvas配下にない場合")
	{
		auto node = noco::Node::Create();
		
		// Canvas配下にないノードはactiveInHierarchyがfalse
		REQUIRE(node->activeInHierarchy() == noco::ActiveYN::No);
		
		auto parent = noco::Node::Create();
		auto child = noco::Node::Create();
		parent->addChild(child);
		
		// Canvas配下にない階層でもactiveInHierarchyはfalse
		REQUIRE(parent->activeInHierarchy() == noco::ActiveYN::No);
		REQUIRE(child->activeInHierarchy() == noco::ActiveYN::No);
	}

	SECTION("Active in hierarchy - Canvas配下の場合")
	{
		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto parent = noco::Node::Create();
		auto child = noco::Node::Create();
		
		parent->addChild(child);
		canvas->addChild(parent);
		
		// Canvas配下ではactiveInHierarchyがtrue
		REQUIRE(parent->activeInHierarchy() == noco::ActiveYN::Yes);
		REQUIRE(child->activeInHierarchy() == noco::ActiveYN::Yes);
		
		// 親を非アクティブに設定
		parent->setActive(noco::ActiveYN::No);
		REQUIRE(child->activeInHierarchy() == noco::ActiveYN::No);
	}

	SECTION("Transform properties")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create();
		canvas->addChild(node);
		
		// 初期値の確認
		auto& transform = node->transform();
		REQUIRE(transform.translate().value() == Vec2{ 0, 0 });
		REQUIRE(transform.scale().value() == Vec2{ 1, 1 });
		
		// Transformを使用して変換を設定
		transform.setTranslate(Vec2{ 100, 200 });
		transform.setScale(Vec2{ 2.0, 3.0 });
		
		// updateを呼んで値を更新
		canvas->update();
		
		// 更新後の値の確認
		REQUIRE(transform.translate().value() == Vec2{ 100, 200 });
		REQUIRE(transform.scale().value() == Vec2{ 2.0, 3.0 });
	}

	SECTION("Interaction states")
	{
		auto node = noco::Node::Create();
		
		// デフォルトの状態
		REQUIRE(node->currentInteractionState() == noco::InteractionState::Default);
		
		// styleStateの設定
		node->setStyleState(U"focused");
		REQUIRE(node->styleState() == U"focused");
	}

	SECTION("Hit test properties")
	{
		auto node = noco::Node::Create();
		
		// デフォルトではヒット対象
		REQUIRE(node->isHitTarget() == noco::IsHitTargetYN::Yes);
		
		// ヒット対象から除外
		node->setIsHitTarget(noco::IsHitTargetYN::No);
		REQUIRE(node->isHitTarget() == noco::IsHitTargetYN::No);
		
		// ヒットテストパディング
		node->setHitPadding(noco::LRTB{ 10, 20, 30, 40 });
		REQUIRE(node->hitPadding() == noco::LRTB{ 10, 20, 30, 40 });
	}
}

// Node階層の高度な操作
TEST_CASE("Node hierarchy advanced operations", "[Node]")
{
	SECTION("Insert child at index")
	{
		auto parent = noco::Node::Create();
		auto child1 = noco::Node::Create(U"Child1");
		auto child2 = noco::Node::Create(U"Child2");
		auto child3 = noco::Node::Create(U"Child3");
		
		parent->addChild(child1);
		parent->addChild(child2);
		
		// インデックス1に挿入
		parent->addChildAtIndex(child3, 1);
		
		REQUIRE(parent->children().size() == 3);
		REQUIRE(parent->children()[0] == child1);
		REQUIRE(parent->children()[1] == child3);
		REQUIRE(parent->children()[2] == child2);
	}

	SECTION("Remove all children")
	{
		auto parent = noco::Node::Create();
		
		for (int i = 0; i < 5; ++i)
		{
			parent->addChild(noco::Node::Create());
		}
		
		REQUIRE(parent->children().size() == 5);
		
		parent->removeChildrenAll();
		
		REQUIRE(parent->children().empty());
	}

	SECTION("Find child by name")
	{
		auto parent = noco::Node::Create();
		auto child1 = noco::Node::Create(U"UniqueChild");
		auto child2 = noco::Node::Create(U"AnotherChild");
		
		parent->addChild(child1);
		parent->addChild(child2);
		
		// 再帰的に名前で検索
		auto found = parent->getChildByName(U"UniqueChild", noco::RecursiveYN::Yes);
		REQUIRE(found == child1);
		
		// 存在しない名前
		auto notFound = parent->getChildByNameOrNull(U"NonExistent", noco::RecursiveYN::Yes);
		REQUIRE(notFound == nullptr);
	}

	SECTION("Child index operations")
	{
		auto parent = noco::Node::Create();
		auto child1 = noco::Node::Create();
		auto child2 = noco::Node::Create();
		auto child3 = noco::Node::Create();
		
		parent->addChild(child1);
		parent->addChild(child2);
		parent->addChild(child3);
		
		// インデックスの取得
		REQUIRE(parent->indexOfChild(child1) == 0);
		REQUIRE(parent->indexOfChild(child2) == 1);
		REQUIRE(parent->indexOfChild(child3) == 2);
		
		// 子ノードの入れ替え
		parent->swapChildren(0, 2);
		REQUIRE(parent->children()[0] == child3);
		REQUIRE(parent->children()[2] == child1);
	}

	SECTION("Deep hierarchy")
	{
		auto root = noco::Node::Create(U"Root");
		auto level1 = noco::Node::Create(U"Level1");
		auto level2 = noco::Node::Create(U"Level2");
		auto level3 = noco::Node::Create(U"Level3");
		
		root->addChild(level1);
		level1->addChild(level2);
		level2->addChild(level3);
		
		// 深い階層での検索
		auto found = root->getChildByName(U"Level3", noco::RecursiveYN::Yes);
		REQUIRE(found == level3);
		
		// hasChildrenの確認
		REQUIRE(root->hasChildren());
		REQUIRE(level1->hasChildren());
		REQUIRE(level2->hasChildren());
		REQUIRE_FALSE(level3->hasChildren());
	}

	SECTION("emplaceChild with RefreshesLayoutYN only")
	{
		auto parent = noco::Node::Create(U"Parent");
		
		// RefreshesLayoutYN引数のみでemplaceChild
		const auto& child1 = parent->emplaceChild(noco::RefreshesLayoutYN::Yes);
		REQUIRE(child1 != nullptr);
		REQUIRE(child1->parentNode() == parent);
		REQUIRE(parent->children().size() == 1);
		REQUIRE(parent->children()[0] == child1);
		
		// デフォルト値の確認
		REQUIRE(child1->name() == U"Node");
		REQUIRE(child1->hasInlineRegion());
		REQUIRE(child1->isHitTarget() == noco::IsHitTargetYN::Yes);
		REQUIRE(child1->inheritChildrenStateFlags() == noco::InheritChildrenStateFlags::None);
		
		// RefreshesLayoutYN::Noで別の子を追加
		const auto& child2 = parent->emplaceChild(noco::RefreshesLayoutYN::No);
		REQUIRE(child2 != nullptr);
		REQUIRE(child2->parentNode() == parent);
		REQUIRE(parent->children().size() == 2);
		REQUIRE(parent->children()[1] == child2);
		
		// 異なるインスタンスであることを確認
		REQUIRE(child1 != child2);
	}
}

// Nodeの座標変換
TEST_CASE("Node coordinate transformations", "[Node]")
{
	SECTION("Local to world transformation")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create();
		auto child = noco::Node::Create();
		
		canvas->addChild(parent);
		parent->addChild(child);
		
		// 初期値の確認
		REQUIRE(parent->transform().translate().value() == Vec2{ 0, 0 });
		REQUIRE(child->transform().translate().value() == Vec2{ 0, 0 });
		
		parent->transform().setTranslate(Vec2{ 100, 100 });
		child->transform().setTranslate(Vec2{ 50, 50 });
		
		// Canvasをupdateして変換を適用
		canvas->update();
		
		// 更新後の座標の確認
		REQUIRE(parent->transform().translate().value() == Vec2{ 100, 100 });
		REQUIRE(child->transform().translate().value() == Vec2{ 50, 50 });
	}

	SECTION("Scale inheritance")
	{
		auto canvas = noco::Canvas::Create();
		auto parent = noco::Node::Create();
		auto child = noco::Node::Create();
		
		canvas->addChild(parent);
		parent->addChild(child);
		
		// 初期値の確認
		REQUIRE(parent->transform().scale().value() == Vec2{ 1, 1 });
		REQUIRE(child->transform().scale().value() == Vec2{ 1, 1 });
		
		parent->transform().setScale(Vec2{ 2.0, 2.0 });
		child->transform().setScale(Vec2{ 0.5, 0.5 });
		
		// Canvasをupdateして変換を適用
		canvas->update();
		
		// 更新後のスケールの確認
		REQUIRE(parent->transform().scale().value() == Vec2{ 2.0, 2.0 });
		REQUIRE(child->transform().scale().value() == Vec2{ 0.5, 0.5 });
	}

}

// Nodeのスクロール機能
TEST_CASE("Node scrolling", "[Node]")
{
	SECTION("Basic scroll")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create();
		canvas->addChild(node);
		
		// スクロールには子ノードが必要
		auto child = noco::Node::Create();
		child->setRegion(noco::InlineRegion{ .sizeDelta = Vec2{ 1000, 1000 } });
		node->addChild(child);
		
		// スクロール可能に設定
		node->setScrollableAxisFlags(noco::ScrollableAxisFlags::Horizontal | noco::ScrollableAxisFlags::Vertical);
		REQUIRE(node->horizontalScrollable());
		REQUIRE(node->verticalScrollable());
		
		// 初期値の確認
		REQUIRE(node->scrollOffset() == Vec2{ 0, 0 });
		
		// スクロール
		node->scroll(Vec2{ 100, 200 });
		
		// scrollメソッドは即座に反映されるはず
		REQUIRE(node->scrollOffset() == Vec2{ 100, 200 });
		
		// スクロールオフセットをリセット
		node->resetScrollOffset();
		REQUIRE(node->scrollOffset() == Vec2{ 0, 0 });
	}

	SECTION("Scroll axis restrictions")
	{
		auto node = noco::Node::Create();
		
		// X軸のみスクロール可能
		node->setScrollableAxisFlags(noco::ScrollableAxisFlags::Horizontal);
		REQUIRE(node->horizontalScrollable());
		REQUIRE_FALSE(node->verticalScrollable());
		
		// Y軸のみスクロール可能
		node->setScrollableAxisFlags(noco::ScrollableAxisFlags::Vertical);
		REQUIRE_FALSE(node->horizontalScrollable());
		REQUIRE(node->verticalScrollable());
		
		// スクロール無効
		node->setScrollableAxisFlags(noco::ScrollableAxisFlags::None);
		REQUIRE_FALSE(node->horizontalScrollable());
		REQUIRE_FALSE(node->verticalScrollable());
	}

	SECTION("Scroll via setters")
	{
		auto node = noco::Node::Create();
		
		// 水平スクロール可能に設定
		node->setHorizontalScrollable(true);
		REQUIRE(node->horizontalScrollable());
		
		// 垂直スクロール可能に設定
		node->setVerticalScrollable(true);
		REQUIRE(node->verticalScrollable());
	}
}

// Transformのテスト
TEST_CASE("Transform", "[Node][Transform]")
{
	SECTION("Basic transform effects")
	{
		auto canvas = noco::Canvas::Create();
		auto node = noco::Node::Create();
		canvas->addChild(node);
		
		// 初期値の確認
		auto& transform = node->transform();
		REQUIRE(transform.translate().value() == Vec2{ 0, 0 });
		REQUIRE(transform.scale().value() == Vec2{ 1, 1 });
		REQUIRE(transform.pivot().value() == Vec2{ 0.5, 0.5 });
		
		// Transformの設定
		transform.setTranslate(Vec2{ 10, 20 });
		transform.setScale(Vec2{ 1.5, 1.5 });
		transform.setPivot(Vec2{ 0.5, 0.5 });
		
		// Canvasをupdateして変換を適用
		canvas->update();
		
		// 更新後のTransformの確認
		REQUIRE(node->transform().translate().value() == Vec2{ 10, 20 });
		REQUIRE(node->transform().scale().value() == Vec2{ 1.5, 1.5 });
		REQUIRE(node->transform().pivot().value() == Vec2{ 0.5, 0.5 });
	}
}

// removeComponentsAllのテスト
TEST_CASE("Node removeComponentsAll", "[Node][Component]")
{
	SECTION("Remove specific component type non-recursively")
	{
		auto node = noco::Node::Create();
		
		// 複数のコンポーネントを追加
		auto label1 = node->emplaceComponent<noco::Label>();
		auto label2 = node->emplaceComponent<noco::Label>();
		auto textBox = node->emplaceComponent<noco::TextBox>();
		
		// 初期状態の確認
		auto labelCount = std::count_if(node->components().begin(), node->components().end(), [](const auto& c) {
			return std::dynamic_pointer_cast<noco::Label>(c) != nullptr;
		});
		REQUIRE(labelCount == 2);
		REQUIRE(node->getComponentOrNull<noco::TextBox>() != nullptr);
		
		// Labelコンポーネントを全て削除（非再帰）
		node->removeComponentsAll<noco::Label>(noco::RecursiveYN::No);
		
		// 削除後の確認
		auto labelCountAfter = std::count_if(node->components().begin(), node->components().end(), [](const auto& c) {
			return std::dynamic_pointer_cast<noco::Label>(c) != nullptr;
		});
		REQUIRE(labelCountAfter == 0);
		REQUIRE(node->getComponentOrNull<noco::TextBox>() != nullptr);  // TextBoxは残っている
	}
	
	SECTION("Remove specific component type recursively")
	{
		auto parent = noco::Node::Create();
		auto child1 = noco::Node::Create();
		auto child2 = noco::Node::Create();
		auto grandchild = noco::Node::Create();
		
		parent->addChild(child1);
		parent->addChild(child2);
		child1->addChild(grandchild);
		
		// 各ノードにLabelコンポーネントを追加
		parent->emplaceComponent<noco::Label>();
		child1->emplaceComponent<noco::Label>();
		child2->emplaceComponent<noco::Label>();
		grandchild->emplaceComponent<noco::Label>();
		
		// 各ノードにTextBoxも追加（削除されないことを確認するため）
		parent->emplaceComponent<noco::TextBox>();
		child1->emplaceComponent<noco::TextBox>();
		
		// 初期状態の確認
		REQUIRE(parent->getComponentOrNull<noco::Label>() != nullptr);
		REQUIRE(child1->getComponentOrNull<noco::Label>() != nullptr);
		REQUIRE(child2->getComponentOrNull<noco::Label>() != nullptr);
		REQUIRE(grandchild->getComponentOrNull<noco::Label>() != nullptr);
		
		// Labelコンポーネントを再帰的に削除
		parent->removeComponentsAll<noco::Label>(noco::RecursiveYN::Yes);
		
		// 削除後の確認
		REQUIRE(parent->getComponentOrNull<noco::Label>() == nullptr);
		REQUIRE(child1->getComponentOrNull<noco::Label>() == nullptr);
		REQUIRE(child2->getComponentOrNull<noco::Label>() == nullptr);
		REQUIRE(grandchild->getComponentOrNull<noco::Label>() == nullptr);
		
		// TextBoxは残っていることを確認
		REQUIRE(parent->getComponentOrNull<noco::TextBox>() != nullptr);
		REQUIRE(child1->getComponentOrNull<noco::TextBox>() != nullptr);
	}
	
	SECTION("Remove components when none exist")
	{
		auto node = noco::Node::Create();
		
		// コンポーネントが存在しない状態で削除を実行
		REQUIRE_NOTHROW(node->removeComponentsAll<noco::Label>(noco::RecursiveYN::No));
		REQUIRE_NOTHROW(node->removeComponentsAll<noco::Label>(noco::RecursiveYN::Yes));
	}
	
	SECTION("Complex hierarchy removal")
	{
		// 複雑な階層構造でのテスト
		auto root = noco::Node::Create();
		
		// 3階層のノード構造を作成
		for (int i = 0; i < 3; ++i)
		{
			auto level1 = noco::Node::Create();
			root->addChild(level1);
			
			// 各レベル1ノードに複数のコンポーネントを追加
			level1->emplaceComponent<noco::Label>();
			level1->emplaceComponent<noco::RectRenderer>();
			
			for (int j = 0; j < 2; ++j)
			{
				auto level2 = noco::Node::Create();
				level1->addChild(level2);
				
				// 各レベル2ノードにもコンポーネントを追加
				level2->emplaceComponent<noco::Label>();
				level2->emplaceComponent<noco::Label>();  // 同じ型を複数追加
			}
		}
		
		// 再帰的に削除
		root->removeComponentsAll<noco::Label>(noco::RecursiveYN::Yes);
		
		// 全てのノードからLabelが削除されたことを確認
		std::function<void(const std::shared_ptr<noco::Node>&)> checkNode;
		checkNode = [&checkNode](const std::shared_ptr<noco::Node>& node)
		{
			auto labelCount = std::count_if(node->components().begin(), node->components().end(), [](const auto& c) {
				return std::dynamic_pointer_cast<noco::Label>(c) != nullptr;
			});
			REQUIRE(labelCount == 0);
			for (const auto& child : node->children())
			{
				checkNode(child);
			}
		};
		checkNode(root);
		
		// RectRendererは残っていることを確認
		int rectRendererCount = 0;
		std::function<void(const std::shared_ptr<noco::Node>&)> countRectRenderers;
		countRectRenderers = [&rectRendererCount, &countRectRenderers](const std::shared_ptr<noco::Node>& node)
		{
			if (node->getComponentOrNull<noco::RectRenderer>())
			{
				rectRendererCount++;
			}
			for (const auto& child : node->children())
			{
				countRectRenderers(child);
			}
		};
		countRectRenderers(root);
		REQUIRE(rectRendererCount == 3);  // レベル1ノード3つ分
	}
}