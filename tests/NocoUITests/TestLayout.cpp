# include <catch2/catch.hpp>
# include <Siv3D.hpp>
# include <NocoUI.hpp>

// ========================================
// Layoutのテスト
// ========================================

TEST_CASE("Layout system", "[Layout]")
{
	SECTION("HorizontalLayout")
	{
		auto parent = noco::Node::Create();
		noco::HorizontalLayout layout;
		layout.spacing = 10.0f;
		
		parent->setBoxChildrenLayout(layout);
		
		// 子ノードを追加
		auto child1 = noco::Node::Create();
		auto child2 = noco::Node::Create();
		parent->addChild(child1);
		parent->addChild(child2);
		
		// Layoutが正しく設定されているか確認
		auto* hLayout = std::get_if<noco::HorizontalLayout>(&parent->boxChildrenLayout());
		REQUIRE(hLayout != nullptr);
		REQUIRE(hLayout->spacing == 10.0f);
	}

	SECTION("VerticalLayout")
	{
		auto parent = noco::Node::Create();
		noco::VerticalLayout layout;
		layout.spacing = 5.0f;
		
		parent->setBoxChildrenLayout(layout);
		
		// Layoutが正しく設定されているか確認
		auto* vLayout = std::get_if<noco::VerticalLayout>(&parent->boxChildrenLayout());
		REQUIRE(vLayout != nullptr);
		REQUIRE(vLayout->spacing == 5.0f);
	}
}

TEST_CASE("HorizontalLayout detailed", "[Layout][HorizontalLayout]")
{
	SECTION("Spacing and padding")
	{
		auto parent = noco::Node::Create();
		noco::HorizontalLayout layout;
		
		// スペーシングの設定
		layout.spacing = 15.0f;
		
		// パディングの設定
		layout.padding = noco::LRTB{ 10, 10, 5, 5 };
		
		parent->setBoxChildrenLayout(layout);
		
		auto* hLayout = std::get_if<noco::HorizontalLayout>(&parent->boxChildrenLayout());
		REQUIRE(hLayout != nullptr);
		REQUIRE(hLayout->spacing == 15.0f);
		REQUIRE(hLayout->padding.left == 10);
		REQUIRE(hLayout->padding.top == 5);
	}

	SECTION("Child alignment")
	{
		auto parent = noco::Node::Create();
		noco::HorizontalLayout layout;
		
		// 子要素のアライメント
		layout.verticalAlign = noco::VerticalAlign::Middle;
		
		parent->setBoxChildrenLayout(layout);
		
		auto* hLayout = std::get_if<noco::HorizontalLayout>(&parent->boxChildrenLayout());
		REQUIRE(hLayout->verticalAlign == noco::VerticalAlign::Middle);
	}

}

TEST_CASE("VerticalLayout detailed", "[Layout][VerticalLayout]")
{
	SECTION("Basic properties")
	{
		auto parent = noco::Node::Create();
		noco::VerticalLayout layout;
		
		// スペーシングの設定
		layout.spacing = 10.0f;
		
		// パディングの設定
		layout.padding = noco::LRTB{ 5, 5, 10, 10 };
		
		parent->setBoxChildrenLayout(layout);
		
		auto* vLayout = std::get_if<noco::VerticalLayout>(&parent->boxChildrenLayout());
		REQUIRE(vLayout != nullptr);
		REQUIRE(vLayout->spacing == 10.0f);
		REQUIRE(vLayout->padding.left == 5);
	}
}

TEST_CASE("FlowLayout detailed", "[Layout][FlowLayout]")
{
	SECTION("Basic properties")
	{
		auto parent = noco::Node::Create();
		noco::FlowLayout layout;
		
		// スペーシングの設定
		layout.spacing = Vec2{ 10.0f, 5.0f };
		
		// パディングの設定
		layout.padding = noco::LRTB{ 5, 5, 10, 10 };
		
		parent->setBoxChildrenLayout(layout);
		
		auto* flowLayout = std::get_if<noco::FlowLayout>(&parent->boxChildrenLayout());
		REQUIRE(flowLayout != nullptr);
		REQUIRE(flowLayout->spacing == Vec2{ 10.0f, 5.0f });
		REQUIRE(flowLayout->padding.left == 5);
	}
}