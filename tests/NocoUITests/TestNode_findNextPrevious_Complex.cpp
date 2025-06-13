# include <catch2/catch.hpp>
# include <NocoUI/Node.hpp>

TEST_CASE("Node findNext and findPrevious complex cases", "[Node]")
{
		SECTION("findNext and findPrevious with complex tree structure")
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
			auto prevNone = root->findPrevious(predicateNone);
			REQUIRE(prevNone == nullptr);
			
			// findPrevious with cyclic: rootからd1を探す
			auto predicateD1 = [](const noco::Node& n) { return n.name() == U"d1"; };
			auto prevD1 = root->findPrevious(predicateD1, noco::SkipsSelfYN::Yes, noco::IsCyclicYN::Yes);
			REQUIRE(prevD1 != nullptr);
			REQUIRE(prevD1->name() == U"d1");
		}

		SECTION("findNext and findPrevious with multiple matches")
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
			
			// findPrevious: item2から前のitemを探す（item3）
			auto prev1 = item2->findPrevious(predicateItem);
			REQUIRE(prev1 == item3);
			
			// findPrevious: item3から前のitemを探す（item1）
			auto prev2 = item3->findPrevious(predicateItem);
			REQUIRE(prev2 == item1);
		}

		SECTION("findNext and findPrevious edge cases")
		{
			// エッジケース: 単一ノード
			auto single = noco::Node::Create(U"single");
			auto predicate = [](const noco::Node& n) { return n.name() == U"single"; };
			
			// skipsSelf = Noの場合、自分自身を返す
			auto self = single->findNext(predicate, noco::SkipsSelfYN::No);
			REQUIRE(self == single);
			
			auto selfPrev = single->findPrevious(predicate, noco::SkipsSelfYN::No);
			REQUIRE(selfPrev == single);
			
			// skipsSelf = Yesの場合、何も見つからない
			auto none = single->findNext(predicate, noco::SkipsSelfYN::Yes);
			REQUIRE(none == nullptr);
			
			auto nonePrev = single->findPrevious(predicate, noco::SkipsSelfYN::Yes);
			REQUIRE(nonePrev == nullptr);
			
			// 循環探索でも自分自身はスキップ
			auto cyclicNone = single->findNext(predicate, noco::SkipsSelfYN::Yes, noco::IsCyclicYN::Yes);
			REQUIRE(cyclicNone == nullptr);
			
			auto cyclicNonePrev = single->findPrevious(predicate, noco::SkipsSelfYN::Yes, noco::IsCyclicYN::Yes);
			REQUIRE(cyclicNonePrev == nullptr);
		}

		SECTION("findNext and findPrevious performance characteristics")
		{
			// 深いツリー構造でのパフォーマンステスト
			auto root = noco::Node::Create(U"root");
			auto current = root;
			
			// 深さ100のツリーを作成
			for (int i = 0; i < 100; ++i)
			{
				auto child = noco::Node::Create(U"node{}"_fmt(i));
				current->addChild(child);
				current = child;
			}
			
			// 最深部から最上部を探す
			auto predicateRoot = [](const noco::Node& n) { return n.name() == U"root"; };
			auto found = current->findPrevious(predicateRoot);
			REQUIRE(found == root);
			
			// 最上部から最深部を探す
			auto predicateDeep = [](const noco::Node& n) { return n.name() == U"node99"; };
			auto foundDeep = root->findNext(predicateDeep);
			REQUIRE(foundDeep == current);
		}

		SECTION("findNext and findPrevious with skip patterns")
		{
			// スキップパターンのテスト
			auto root = noco::Node::Create(U"root");
			auto container1 = noco::Node::Create(U"container");
			auto target1 = noco::Node::Create(U"target");
			auto container2 = noco::Node::Create(U"container");
			auto target2 = noco::Node::Create(U"target");
			auto container3 = noco::Node::Create(U"container");
			auto target3 = noco::Node::Create(U"target");
			
			root->addChild(container1);
			container1->addChild(target1);
			root->addChild(container2);
			container2->addChild(target2);
			root->addChild(container3);
			container3->addChild(target3);
			
			// containerをスキップしてtargetだけを探す
			auto predicateTarget = [](const noco::Node& n) { return n.name() == U"target"; };
			
			// target1から次のtargetを探す
			auto next = target1->findNext(predicateTarget);
			REQUIRE(next == target2);
			
			// target2から前のtargetを探す
			auto prev = target2->findPrevious(predicateTarget);
			REQUIRE(prev == target1);
			
			// 条件付き検索: 親がcontainerであるtargetだけ
			auto predicateConditional = [](const noco::Node& n) { 
				return n.name() == U"target" && n.parent() && n.parent()->name() == U"container";
			};
			
			auto conditionalNext = root->findNext(predicateConditional);
			REQUIRE(conditionalNext == target1);
		}

		SECTION("findNext and findPrevious with dynamic tree modifications")
		{
			// 動的な変更のテスト
			auto root = noco::Node::Create(U"root");
			auto nodeA = noco::Node::Create(U"nodeA");
			auto nodeB = noco::Node::Create(U"nodeB");
			auto nodeC = noco::Node::Create(U"nodeC");
			
			root->addChild(nodeA);
			root->addChild(nodeB);
			root->addChild(nodeC);
			
			// 初期状態でのテスト
			auto predicateB = [](const noco::Node& n) { return n.name() == U"nodeB"; };
			auto found = nodeA->findNext(predicateB);
			REQUIRE(found == nodeB);
			
			// nodeBを削除
			root->removeChild(nodeB);
			
			// nodeBが見つからないことを確認
			auto notFound = nodeA->findNext(predicateB);
			REQUIRE(notFound == nullptr);
			
			// nodeBを最後に追加
			root->addChild(nodeB);
			
			// 新しい位置でnodeBが見つかることを確認
			auto foundAgain = nodeA->findNext(predicateB);
			REQUIRE(foundAgain == nodeB);
			
			// nodeAとnodeCの順番を入れ替え
			root->removeChild(nodeA);
			root->removeChild(nodeC);
			root->addChild(nodeC);
			root->addChild(nodeA);
			
			// 順序: root, nodeB, nodeC, nodeA
			auto predicateA = [](const noco::Node& n) { return n.name() == U"nodeA"; };
			auto prevA = nodeB->findPrevious(predicateA);
			REQUIRE(prevA == nullptr); // nodeAはnodeBの後にある
			
			auto nextA = nodeB->findNext(predicateA);
			REQUIRE(nextA == nodeA); // nodeAは見つかる
		}

		SECTION("findNext and findPrevious with nested modifications")
		{
			// ネストした構造での変更
			auto root = noco::Node::Create(U"root");
			auto parent1 = noco::Node::Create(U"parent1");
			auto parent2 = noco::Node::Create(U"parent2");
			auto child = noco::Node::Create(U"child");
			
			root->addChild(parent1);
			root->addChild(parent2);
			parent1->addChild(child);
			
			// childをparent1からparent2に移動
			auto predicateChild = [](const noco::Node& n) { return n.name() == U"child"; };
			
			// 移動前: parent1から見つかる
			auto foundInParent1 = parent1->findNext(predicateChild, noco::SkipsSelfYN::Yes);
			REQUIRE(foundInParent1 == child);
			
			// parent2からは見つからない（parent2より前にある）
			auto notFoundFromParent2 = parent2->findNext(predicateChild);
			REQUIRE(notFoundFromParent2 == nullptr);
			
			// childを移動
			parent1->removeChild(child);
			parent2->addChild(child);
			
			// 移動後: parent1から見つかる（parent2の子として）
			auto foundAfterMove = parent1->findNext(predicateChild);
			REQUIRE(foundAfterMove == child);
			
			// parent2の子として正しく見つかる
			REQUIRE(child->parent() == parent2);
		}

		SECTION("findNext and findPrevious with single node edge cases")
		{
			// 単一ノードの詳細なテスト
			auto single = noco::Node::Create(U"single");
			
			// 自分自身を探す
			auto predicateSelf = [](const noco::Node& n) { return n.name() == U"single"; };
			
			// skipsSelf = No の場合
			auto selfNext = single->findNext(predicateSelf, noco::SkipsSelfYN::No);
			REQUIRE(selfNext == single);
			
			auto selfPrev = single->findPrevious(predicateSelf, noco::SkipsSelfYN::No);
			REQUIRE(selfPrev == single);
			
			// skipsSelf = Yes の場合
			auto noSelfNext = single->findNext(predicateSelf, noco::SkipsSelfYN::Yes);
			REQUIRE(noSelfNext == nullptr);
			
			auto noSelfPrev = single->findPrevious(predicateSelf, noco::SkipsSelfYN::Yes);
			REQUIRE(noSelfPrev == nullptr);
			
			// 循環探索でも同じ結果
			auto cyclicSelfNext = single->findNext(predicateSelf, noco::SkipsSelfYN::No, noco::IsCyclicYN::Yes);
			REQUIRE(cyclicSelfNext == single);
			
			auto cyclicNoSelfNext = single->findNext(predicateSelf, noco::SkipsSelfYN::Yes, noco::IsCyclicYN::Yes);
			REQUIRE(cyclicNoSelfNext == nullptr);
			
			// 別の条件で探す（見つからない）
			auto predicateOther = [](const noco::Node& n) { return n.name() == U"other"; };
			auto notFound = single->findNext(predicateOther);
			REQUIRE(notFound == nullptr);
		}

		SECTION("findNext and findPrevious with practical UI scenarios")
		{
			// 実践的なUIシナリオ（フォーカス可能な要素の探索）
			auto window = noco::Node::Create(U"window");
			auto header = noco::Node::Create(U"header");
			auto content = noco::Node::Create(U"content");
			auto footer = noco::Node::Create(U"footer");
			
			auto button1 = noco::Node::Create(U"button1");
			auto button2 = noco::Node::Create(U"button2");
			auto textBox = noco::Node::Create(U"textBox");
			auto label = noco::Node::Create(U"label"); // フォーカス不可
			auto button3 = noco::Node::Create(U"button3");
			
			window->addChild(header);
			window->addChild(content);
			window->addChild(footer);
			
			header->addChild(button1);
			content->addChild(button2);
			content->addChild(textBox);
			content->addChild(label);
			footer->addChild(button3);
			
			// フォーカス可能な要素だけを探す述語
			auto isFocusable = [](const noco::Node& n) {
				return n.name().starts_with(U"button") || n.name() == U"textBox";
			};
			
			// Tab順序: button1 -> button2 -> textBox -> button3
			auto next1 = button1->findNext(isFocusable);
			REQUIRE(next1 == button2);
			
			auto next2 = button2->findNext(isFocusable);
			REQUIRE(next2 == textBox);
			
			auto next3 = textBox->findNext(isFocusable);
			REQUIRE(next3 == button3);
			
			// 循環Tab
			auto nextCyclic = button3->findNext(isFocusable, noco::SkipsSelfYN::Yes, noco::IsCyclicYN::Yes);
			REQUIRE(nextCyclic == button1);
			
			// Shift+Tab（逆順）
			auto prev1 = button3->findPrevious(isFocusable);
			REQUIRE(prev1 == textBox);
			
			auto prev2 = textBox->findPrevious(isFocusable);
			REQUIRE(prev2 == button2);
			
			// 動的に要素を無効化（フォーカス不可に）
			auto isFocusableExceptButton2 = [](const noco::Node& n) {
				return (n.name().starts_with(U"button") && n.name() != U"button2") || n.name() == U"textBox";
			};
			
			// button2をスキップ
			auto skipButton2 = button1->findNext(isFocusableExceptButton2);
			REQUIRE(skipButton2 == textBox);
		}

		SECTION("findNext and findPrevious with empty containers")
		{
			// 空のコンテナを含むケース
			auto root = noco::Node::Create(U"root");
			auto emptyContainer1 = noco::Node::Create(U"empty1");
			auto item = noco::Node::Create(U"item");
			auto emptyContainer2 = noco::Node::Create(U"empty2");
			
			root->addChild(emptyContainer1);
			root->addChild(item);
			root->addChild(emptyContainer2);
			
			// 空のコンテナをスキップして次の要素を見つける
			auto predicateItem = [](const noco::Node& n) { return n.name() == U"item"; };
			auto found = emptyContainer1->findNext(predicateItem);
			REQUIRE(found == item);
			
			// 逆方向でも同様
			auto foundReverse = emptyContainer2->findPrevious(predicateItem);
			REQUIRE(foundReverse == item);
			
			// コンテナ自体を探す
			auto predicateEmpty = [](const noco::Node& n) { return n.name().starts_with(U"empty"); };
			auto foundEmpty = root->findNext(predicateEmpty);
			REQUIRE(foundEmpty == emptyContainer1);
		}

		SECTION("findNext and findPrevious stress test with large tree")
		{
			// 大規模なツリーでのストレステスト
			auto root = noco::Node::Create(U"root");
			std::vector<std::shared_ptr<noco::Node>> nodes;
			
			// 幅10、深さ10のツリーを作成
			std::function<void(std::shared_ptr<noco::Node>, int, int&)> createTree;
			createTree = [&](std::shared_ptr<noco::Node> parent, int depth, int& counter) {
				if (depth <= 0) return;
				
				for (int i = 0; i < 10; ++i)
				{
					auto child = noco::Node::Create(U"node{}"_fmt(counter++));
					parent->addChild(child);
					nodes.push_back(child);
					
					if (depth > 1)
					{
						createTree(child, depth - 1, counter);
					}
				}
			};
			
			int counter = 0;
			createTree(root, 3, counter); // 1110個のノード
			
			// 特定のノードを探す
			auto targetIndex = 555;
			auto targetName = U"node{}"_fmt(targetIndex);
			auto predicate = [&targetName](const noco::Node& n) { return n.name() == targetName; };
			
			// rootから探索
			auto found = root->findNext(predicate);
			REQUIRE(found != nullptr);
			REQUIRE(found->name() == targetName);
			
			// 最後のノードから循環探索で最初のノードを見つける
			auto lastNode = nodes.back();
			auto firstPredicate = [](const noco::Node& n) { return n.name() == U"node0"; };
			auto foundFirst = lastNode->findNext(firstPredicate, noco::SkipsSelfYN::Yes, noco::IsCyclicYN::Yes);
			REQUIRE(foundFirst == nodes[0]);
		}

}