# include <catch2/catch.hpp>
# include <NocoUI/Node.hpp>

TEST_CASE("Node findNext", "[Node]")
{
	SECTION("findNext basic traversal")
	{
		// ツリー構造を作成
		//   root
		//   ├── child1
		//   │   ├── grandchild1
		//   │   └── grandchild2
		//   └── child2
		//       └── grandchild3
		
		auto root = noco::Node::Create(U"root");
		auto child1 = noco::Node::Create(U"child1");
		auto child2 = noco::Node::Create(U"child2");
		auto grandchild1 = noco::Node::Create(U"grandchild1");
		auto grandchild2 = noco::Node::Create(U"grandchild2");
		auto grandchild3 = noco::Node::Create(U"grandchild3");
		
		root->addChild(child1);
		root->addChild(child2);
		child1->addChild(grandchild1);
		child1->addChild(grandchild2);
		child2->addChild(grandchild3);
		
		// child1から次のchild2を探す
		auto predicate = [](const noco::Node& n) { return n.name() == U"child2"; };
		auto next = child1->findNext(predicate);
		REQUIRE(next != nullptr);
		REQUIRE(next->name() == U"child2");
		
		// grandchild2から次のgrandchild3を探す
		auto predicate2 = [](const noco::Node& n) { return n.name() == U"grandchild3"; };
		auto next2 = grandchild2->findNext(predicate2);
		REQUIRE(next2 != nullptr);
		REQUIRE(next2->name() == U"grandchild3");
	}
	
	SECTION("findNext with skipsSelf")
	{
		auto node = noco::Node::Create(U"target");
		
		// 自分自身を含まない（デフォルト）
		auto predicate = [](const noco::Node& n) { return n.name() == U"target"; };
		auto next1 = node->findNext(predicate, noco::SkipsSelfYN::Yes);
		REQUIRE(next1 == nullptr);
		
		// 自分自身を含む
		auto next2 = node->findNext(predicate, noco::SkipsSelfYN::No);
		REQUIRE(next2 != nullptr);
		REQUIRE(next2 == node);
	}
	
	SECTION("findNext with cyclic search")
	{
		auto root = noco::Node::Create(U"root");
		auto child1 = noco::Node::Create(U"child1");
		auto child2 = noco::Node::Create(U"child2");
		
		root->addChild(child1);
		root->addChild(child2);
		
		// child2から循環探索でchild1を見つける
		auto predicate = [](const noco::Node& n) { return n.name() == U"child1"; };
		auto next = child2->findNext(predicate, noco::SkipsSelfYN::Yes, noco::IsCyclicYN::Yes);
		REQUIRE(next != nullptr);
		REQUIRE(next->name() == U"child1");
		
		// 循環探索なしの場合は見つからない
		auto next2 = child2->findNext(predicate, noco::SkipsSelfYN::Yes, noco::IsCyclicYN::No);
		REQUIRE(next2 == nullptr);
	}
	
	SECTION("findNext with const node")
	{
		auto root = noco::Node::Create(U"root");
		auto child1 = noco::Node::Create(U"child1");
		auto child2 = noco::Node::Create(U"child2");
		
		root->addChild(child1);
		root->addChild(child2);
		
		// const nodeからもfindNextが使える
		const auto constChild1 = std::const_pointer_cast<const noco::Node>(child1);
		
		auto predicate = [](const noco::Node& n) { return n.name() == U"child2"; };
		auto next = constChild1->findNext(predicate);
		REQUIRE(next != nullptr);
		REQUIRE(next->name() == U"child2");
	}
	
	SECTION("findNext with complex tree structure")
	{
		// 複雑なツリー構造を作成
		//       root
		//      /    \
		//    a1      a2
		//   /  \    /  \
		//  b1  b2  b3  b4
		//  |       |    |
		//  c1      c2   c3
		//               |
		//               d1
		auto root = noco::Node::Create(U"root");
		auto a1 = noco::Node::Create(U"a1");
		auto a2 = noco::Node::Create(U"a2");
		auto b1 = noco::Node::Create(U"b1");
		auto b2 = noco::Node::Create(U"b2");
		auto b3 = noco::Node::Create(U"b3");
		auto b4 = noco::Node::Create(U"b4");
		auto c1 = noco::Node::Create(U"c1");
		auto c2 = noco::Node::Create(U"c2");
		auto c3 = noco::Node::Create(U"c3");
		auto d1 = noco::Node::Create(U"d1");
		
		root->addChild(a1);
		root->addChild(a2);
		a1->addChild(b1);
		a1->addChild(b2);
		a2->addChild(b3);
		a2->addChild(b4);
		b1->addChild(c1);
		b3->addChild(c2);
		b4->addChild(c3);
		c3->addChild(d1);
		
		// 深さ優先順序: root, a1, b1, c1, b2, a2, b3, c2, b4, c3, d1
		
		// findNext: b1からc2を探す（兄弟の子孫を越えて）
		auto predicateC2 = [](const noco::Node& n) { return n.name() == U"c2"; };
		auto nextC2 = b1->findNext(predicateC2);
		REQUIRE(nextC2 != nullptr);
		REQUIRE(nextC2->name() == U"c2");
		
		// findNext: c1からb3を探す（親の兄弟へ）
		auto predicateB3 = [](const noco::Node& n) { return n.name() == U"b3"; };
		auto nextB3 = c1->findNext(predicateB3);
		REQUIRE(nextB3 != nullptr);
		REQUIRE(nextB3->name() == U"b3");
		
		// findNext: d1から何も見つからない（最後の要素）
		auto predicateNone = [](const noco::Node& n) { return n.name() == U"nonexistent"; };
		auto nextNone = d1->findNext(predicateNone);
		REQUIRE(nextNone == nullptr);
		
		// findNext with cyclic: d1からrootを探す
		auto predicateRoot = [](const noco::Node& n) { return n.name() == U"root"; };
		auto nextRoot = d1->findNext(predicateRoot, noco::SkipsSelfYN::Yes, noco::IsCyclicYN::Yes);
		REQUIRE(nextRoot != nullptr);
		REQUIRE(nextRoot->name() == U"root");
	}
	
	SECTION("findNext with multiple matches")
	{
		// 同じ名前のノードが複数ある場合
		auto root = noco::Node::Create(U"root");
		auto item1 = noco::Node::Create(U"item");
		auto item2 = noco::Node::Create(U"item");
		auto item3 = noco::Node::Create(U"item");
		auto other = noco::Node::Create(U"other");
		
		root->addChild(item1);
		root->addChild(other);
		root->addChild(item2);
		other->addChild(item3);
		
		// 深さ優先順序: root, item1, other, item3, item2
		
		// findNext: rootから最初のitemを探す
		auto predicateItem = [](const noco::Node& n) { return n.name() == U"item"; };
		auto next1 = root->findNext(predicateItem);
		REQUIRE(next1 == item1);
		
		// findNext: item1から次のitemを探す（item3）
		auto next2 = item1->findNext(predicateItem);
		REQUIRE(next2 == item3);
		
		// findNext: item3から次のitemを探す（item2）
		auto next3 = item3->findNext(predicateItem);
		REQUIRE(next3 == item2);
	}
	
	SECTION("findNext edge cases")
	{
		// エッジケース: 単一ノード
		auto single = noco::Node::Create(U"single");
		auto predicate = [](const noco::Node& n) { return n.name() == U"single"; };
		
		// skipsSelf = Noの場合、自分自身を返す
		auto self = single->findNext(predicate, noco::SkipsSelfYN::No);
		REQUIRE(self == single);
		
		// skipsSelf = Yesの場合、何も見つからない
		auto none = single->findNext(predicate, noco::SkipsSelfYN::Yes);
		REQUIRE(none == nullptr);
		
		// 循環探索でも自分自身はスキップ
		auto cyclicNone = single->findNext(predicate, noco::SkipsSelfYN::Yes, noco::IsCyclicYN::Yes);
		REQUIRE(cyclicNone == nullptr);
	}
}

TEST_CASE("Node findNext additional edge cases", "[Node]")
{
	SECTION("findNext with detached nodes")
	{
		auto node1 = noco::Node::Create(U"node1");
		auto node2 = noco::Node::Create(U"node2");
		// 親子関係なし
		
		auto predicate = [](const noco::Node& n) { return n.name() == U"node2"; };
		auto result = node1->findNext(predicate);
		REQUIRE(result == nullptr);
	}
	
	SECTION("findNext with predicate exceptions")
	{
		auto root = noco::Node::Create(U"root");
		auto child = noco::Node::Create(U"child");
		root->addChild(child);
		
		// 例外を投げる述語
		auto throwingPredicate = [](const noco::Node&) -> bool 
		{
			throw std::runtime_error("Test exception");
		};
		
		REQUIRE_THROWS_AS(root->findNext(throwingPredicate), std::runtime_error);
	}
	
	SECTION("findNext with very wide trees")
	{
		auto root = noco::Node::Create(U"root");
		constexpr int numChildren = 100;
		
		for (int i = 0; i < numChildren; ++i)
		{
			root->addChild(noco::Node::Create(U"child" + ToString(i)));
		}
		
		// 最初の子を探す
		auto firstPred = [](const noco::Node& n) { return n.name() == U"child0"; };
		auto first = root->findNext(firstPred);
		REQUIRE(first != nullptr);
		REQUIRE(first->name() == U"child0");
		
		// 最後の子を探す
		auto lastPred = [](const noco::Node& n) { return n.name() == U"child99"; };
		auto last = root->findNext(lastPred);
		REQUIRE(last != nullptr);
		REQUIRE(last->name() == U"child99");
	}
	
	SECTION("findNext with stateful predicates")
	{
		auto root = noco::Node::Create(U"root");
		auto child1 = noco::Node::Create(U"child1");
		auto child2 = noco::Node::Create(U"child2");
		auto child3 = noco::Node::Create(U"child3");
		
		root->addChild(child1);
		root->addChild(child2);
		root->addChild(child3);
		
		// カウンタ付き述語（3番目にマッチするノードを探す）
		int counter = 0;
		auto statefulPred = [&counter](const noco::Node&) -> bool 
		{
			++counter;
			return counter == 3;
		};
		
		auto result = root->findNext(statefulPred);
		REQUIRE(result == child3); // skipsSelf=Yesなので: child1(1), child2(2), child3(3)
	}
	
	SECTION("findNext with deep recursion")
	{
		auto root = noco::Node::Create(U"root");
		auto current = root;
		
		// 深い階層構造を作成
		for (int i = 0; i < 50; ++i)
		{
			auto child = noco::Node::Create(U"node" + ToString(i));
			current->addChild(child);
			current = child;
		}
		
		// 最深部のノードを探す
		auto deepPred = [](const noco::Node& n) { return n.name() == U"node49"; };
		auto deepNode = root->findNext(deepPred);
		REQUIRE(deepNode != nullptr);
		REQUIRE(deepNode->name() == U"node49");
	}
	
	SECTION("findNext with complex predicates")
	{
		auto root = noco::Node::Create(U"root");
		auto nodeA = noco::Node::Create(U"nodeA");
		auto nodeB = noco::Node::Create(U"nodeB");
		auto nodeAB = noco::Node::Create(U"nodeAB");
		
		root->addChild(nodeA);
		root->addChild(nodeB);
		root->addChild(nodeAB);
		
		// 複合条件の述語
		auto complexPred = [](const noco::Node& n) 
		{
			return n.name().contains(U'A') && n.name().contains(U'B');
		};
		
		auto result = root->findNext(complexPred);
		REQUIRE(result == nodeAB);
	}
}