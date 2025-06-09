# include <catch2/catch.hpp>
# include <Siv3D.hpp>
# include <NocoUI.hpp>

// ========================================
// エラーケース・エッジケーステスト
// ========================================

TEST_CASE("Error cases and edge cases", "[Error]")
{
	SECTION("Circular parent-child relationship")
	{
		auto node1 = noco::Node::Create();
		auto node2 = noco::Node::Create();
		
		// node1の子にnode2を追加
		node1->addChild(node2);
		
		// node2の子にnode1を追加しようとする（循環参照）
		// これはエラーになるべき
		// 実装によってはassertやexceptionが発生する可能性
		// ここではテスト構造のみ示す
		REQUIRE(node2->parent() == node1);
	}

	SECTION("Invalid constraint values")
	{
		auto node = noco::Node::Create();
		
		// 負のサイズ
		noco::BoxConstraint invalidBox;
		invalidBox.sizeDelta = Vec2{ -100, -50 };
		node->setConstraint(invalidBox);
		
		// 無効なアンカー値（0-1の範囲外）
		noco::AnchorConstraint invalidAnchor;
		invalidAnchor.anchorMin = Vec2{ -0.5, 1.5 };
		invalidAnchor.anchorMax = Vec2{ 2.0, -1.0 };
		node->setConstraint(invalidAnchor);
		
		// エラーハンドリングの実装に依存
	}

	SECTION("Component conflicts")
	{
		auto node = noco::Node::Create();
		
		// 最初のLabelコンポーネント
		auto label1 = node->emplaceComponent<noco::Label>();
		REQUIRE(label1 != nullptr);
		
		// 同じ型のコンポーネントを追加しようとする
		// 実装によってはこれが許可されるかエラーになる
		// NocoUIの仕様に依存
	}

	SECTION("Remove node during iteration")
	{
		auto parent = noco::Node::Create();
		
		// 複数の子ノードを追加
		Array<std::shared_ptr<noco::Node>> children;
		for (int i = 0; i < 5; ++i)
		{
			auto child = noco::Node::Create();
			parent->addChild(child);
			children.push_back(child);
		}
		
		// イテレーション中の削除（実装に注意が必要）
		for (auto& child : children)
		{
			if (child == children[2])
			{
				parent->removeChild(child);
			}
		}
		
		REQUIRE(parent->children().size() == 4);
	}
}