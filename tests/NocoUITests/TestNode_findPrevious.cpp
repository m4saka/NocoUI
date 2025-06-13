# include <catch2/catch.hpp>
# include <NocoUI/Node.hpp>

TEST_CASE("Node findPrevious", "[Node]")
{
	SECTION("findPrevious basic traversal")
	{
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
		
		// child2から前のchild1を探す
		auto predicate = [](const noco::Node& n) { return n.name() == U"child1"; };
		auto prev = child2->findPrevious(predicate);
		REQUIRE(prev != nullptr);
		REQUIRE(prev->name() == U"child1");
		
		// grandchild3から前のgrandchild2を探す
		auto predicate2 = [](const noco::Node& n) { return n.name() == U"grandchild2"; };
		auto prev2 = grandchild3->findPrevious(predicate2);
		REQUIRE(prev2 != nullptr);
		REQUIRE(prev2->name() == U"grandchild2");
	}
	
	SECTION("findPrevious with cyclic search")
	{
		auto root = noco::Node::Create(U"root");
		auto child1 = noco::Node::Create(U"child1");
		auto child2 = noco::Node::Create(U"child2");
		
		root->addChild(child1);
		root->addChild(child2);
		
		// child1から循環探索でchild2を見つける
		auto predicate = [](const noco::Node& n) { return n.name() == U"child2"; };
		auto prev = child1->findPrevious(predicate, noco::SkipsSelfYN::Yes, noco::IsCyclicYN::Yes);
		REQUIRE(prev != nullptr);
		REQUIRE(prev->name() == U"child2");
		
		// 循環探索なしの場合は見つからない
		auto prev2 = child1->findPrevious(predicate, noco::SkipsSelfYN::Yes, noco::IsCyclicYN::No);
		REQUIRE(prev2 == nullptr);
	}
	
	SECTION("findPrevious with const node")
	{
		auto root = noco::Node::Create(U"root");
		auto child1 = noco::Node::Create(U"child1");
		auto child2 = noco::Node::Create(U"child2");
		
		root->addChild(child1);
		root->addChild(child2);
		
		// const nodeからもfindPreviousが使える
		const auto constChild1 = std::const_pointer_cast<const noco::Node>(child1);
		
		auto predicate = [](const noco::Node& n) { return n.name() == U"root"; };
		auto prev = constChild1->findPrevious(predicate);
		REQUIRE(prev != nullptr);
		REQUIRE(prev->name() == U"root");
	}
	
	SECTION("findPrevious with complex tree structure")
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
		
		// findPrevious: c2からb1を探す
		auto predicateB1 = [](const noco::Node& n) { return n.name() == U"b1"; };
		auto prevB1 = c2->findPrevious(predicateB1);
		REQUIRE(prevB1 != nullptr);
		REQUIRE(prevB1->name() == U"b1");
		
		// findPrevious: b3からc1を探す（前の兄弟の子孫）
		auto predicateC1 = [](const noco::Node& n) { return n.name() == U"c1"; };
		auto prevC1 = b3->findPrevious(predicateC1);
		REQUIRE(prevC1 != nullptr);
		REQUIRE(prevC1->name() == U"c1");
		
		// findPrevious: rootから何も見つからない（最初の要素）
		auto predicateNone = [](const noco::Node& n) { return n.name() == U"nonexistent"; };
		auto prevNone = root->findPrevious(predicateNone);
		REQUIRE(prevNone == nullptr);
		
		// findPrevious with cyclic: rootからd1を探す
		auto predicateD1 = [](const noco::Node& n) { return n.name() == U"d1"; };
		auto prevD1 = root->findPrevious(predicateD1, noco::SkipsSelfYN::Yes, noco::IsCyclicYN::Yes);
		REQUIRE(prevD1 != nullptr);
		REQUIRE(prevD1->name() == U"d1");
	}
	
	SECTION("findPrevious with multiple matches")
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
		
		// findPrevious: item2から前のitemを探す（item3）
		auto predicateItem = [](const noco::Node& n) { return n.name() == U"item"; };
		auto prev1 = item2->findPrevious(predicateItem);
		REQUIRE(prev1 == item3);
		
		// findPrevious: item3から前のitemを探す（item1）
		auto prev2 = item3->findPrevious(predicateItem);
		REQUIRE(prev2 == item1);
		
		// findPrevious: item1から前のitemを探す（なし）
		auto prev3 = item1->findPrevious(predicateItem);
		REQUIRE(prev3 == nullptr);
	}
	
	SECTION("findPrevious edge cases")
	{
		// エッジケース: 単一ノード
		auto single = noco::Node::Create(U"single");
		auto predicate = [](const noco::Node& n) { return n.name() == U"single"; };
		
		// skipsSelf = Noの場合、自分自身を返す
		auto selfPrev = single->findPrevious(predicate, noco::SkipsSelfYN::No);
		REQUIRE(selfPrev == single);
		
		// skipsSelf = Yesの場合、何も見つからない
		auto nonePrev = single->findPrevious(predicate, noco::SkipsSelfYN::Yes);
		REQUIRE(nonePrev == nullptr);
		
		// 循環探索でも自分自身はスキップ
		auto cyclicNonePrev = single->findPrevious(predicate, noco::SkipsSelfYN::Yes, noco::IsCyclicYN::Yes);
		REQUIRE(cyclicNonePrev == nullptr);
	}
}

TEST_CASE("Node findPrevious additional edge cases", "[Node]")
{
	SECTION("findPrevious with detached nodes")
	{
		auto node1 = noco::Node::Create(U"node1");
		auto node2 = noco::Node::Create(U"node2");
		// 親子関係なし
		
		auto predicate = [](const noco::Node& n) { return n.name() == U"node1"; };
		auto result = node2->findPrevious(predicate);
		REQUIRE(result == nullptr);
	}
	
	SECTION("findPrevious with predicate exceptions")
	{
		auto root = noco::Node::Create(U"root");
		auto child = noco::Node::Create(U"child");
		root->addChild(child);
		
		// 例外を投げる述語
		auto throwingPredicate = [](const noco::Node&) -> bool 
		{
			throw std::runtime_error("Test exception");
		};
		
		REQUIRE_THROWS_AS(child->findPrevious(throwingPredicate), std::runtime_error);
	}
	
	SECTION("findPrevious with very wide trees")
	{
		auto root = noco::Node::Create(U"root");
		constexpr int numChildren = 100;
		
		for (int i = 0; i < numChildren; ++i)
		{
			root->addChild(noco::Node::Create(U"child" + ToString(i)));
		}
		
		// 最後の子から最初の子を探す
		auto lastChild = root->children().back();
		auto firstPred = [](const noco::Node& n) { return n.name() == U"child0"; };
		auto first = lastChild->findPrevious(firstPred);
		REQUIRE(first != nullptr);
		REQUIRE(first->name() == U"child0");
		
		// rootを探す
		auto rootPred = [](const noco::Node& n) { return n.name() == U"root"; };
		auto foundRoot = lastChild->findPrevious(rootPred);
		REQUIRE(foundRoot != nullptr);
		REQUIRE(foundRoot->name() == U"root");
	}
	
	SECTION("findPrevious with stateful predicates")
	{
		auto root = noco::Node::Create(U"root");
		auto child1 = noco::Node::Create(U"child1");
		auto child2 = noco::Node::Create(U"child2");
		auto child3 = noco::Node::Create(U"child3");
		
		root->addChild(child1);
		root->addChild(child2);
		root->addChild(child3);
		
		// カウンタ付き述語（2番目にマッチするノードを探す）
		int counter = 0;
		auto statefulPred = [&counter](const noco::Node&) -> bool 
		{
			++counter;
			return counter == 2;
		};
		
		auto result = child3->findPrevious(statefulPred);
		REQUIRE(result == child1); // child2(1), child1(2)
	}
	
	SECTION("findPrevious with deep recursion")
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
		
		// 最深部からrootを探す
		auto deepPred = [](const noco::Node& n) { return n.name() == U"root"; };
		auto foundRoot = current->findPrevious(deepPred);
		REQUIRE(foundRoot != nullptr);
		REQUIRE(foundRoot->name() == U"root");
	}
	
	SECTION("findPrevious with complex predicates")
	{
		auto root = noco::Node::Create(U"root");
		auto nodeA = noco::Node::Create(U"nodeA");
		auto nodeB = noco::Node::Create(U"nodeB");
		auto nodeAB = noco::Node::Create(U"nodeAB");
		auto nodeC = noco::Node::Create(U"nodeC");
		
		root->addChild(nodeA);
		root->addChild(nodeB);
		root->addChild(nodeAB);
		root->addChild(nodeC);
		
		// 複合条件の述語
		auto complexPred = [](const noco::Node& n) 
		{
			return n.name().contains(U'A') && !n.name().contains(U'B');
		};
		
		auto result = nodeC->findPrevious(complexPred);
		REQUIRE(result == nodeA);
	}
}