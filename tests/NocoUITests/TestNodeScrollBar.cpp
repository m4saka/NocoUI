#include <catch2/catch.hpp>
#include <Siv3D.hpp>
#include <NocoUI.hpp>

// ========================================
// Nodeのスクロールバーのテスト
// ========================================

namespace
{
	// 縦スクロール可能な200x100のノードと縦長の中身を持つCanvasを作成
	struct ScrollableNodeEnv
	{
		std::shared_ptr<noco::Canvas> canvas;
		std::shared_ptr<noco::Node> scrollableNode;
		std::shared_ptr<noco::Node> contentNode;

		explicit ScrollableNodeEnv(double contentHeight)
			: canvas(noco::Canvas::Create())
			, scrollableNode(noco::Node::Create(U"Scrollable", noco::AnchorRegion
			{
				.anchorMin = noco::Anchor::TopLeft,
				.anchorMax = noco::Anchor::TopLeft,
				.sizeDelta = Vec2{ 200, 100 },
				.sizeDeltaPivot = noco::Anchor::TopLeft,
			}))
			, contentNode(noco::Node::Create(U"Content", noco::InlineRegion
			{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = Vec2{ 0, contentHeight },
			}))
		{
			canvas->addChild(scrollableNode);
			scrollableNode->setVerticalScrollable(true);
			scrollableNode->addChild(contentNode);
			canvas->refreshLayoutImmediately();
		}
	};
}

TEST_CASE("Node scroll bar", "[Node][ScrollBar]")
{
	SECTION("Default scroll bar settings")
	{
		auto node = noco::Node::Create();
		CHECK(node->scrollBarType() == noco::ScrollBarType::Interactive);
		CHECK(node->scrollBarHandleColor() == Color{ 255, 255, 255, 160 });
		CHECK(node->scrollBarThickness() == 8.0);
		CHECK(node->scrollBarMargin() == noco::LRTB{ 2.0, 2.0, 2.0, 2.0 });
		CHECK(node->childrenRectInset() == noco::LRTB::Zero());
	}

	SECTION("Interactive scroll bar reserves children rect inset")
	{
		ScrollableNodeEnv env{ 1000.0 };
		env.scrollableNode->setScrollBarType(noco::ScrollBarType::Interactive);

		env.canvas->update();
		env.canvas->refreshLayoutImmediately();

		// 縦バーの太さ分だけ右側が占有される
		CHECK(env.scrollableNode->childrenRectInset() == noco::LRTB{ 0, 12, 0, 0 });
		CHECK(env.scrollableNode->childrenRect() == RectF{ 0, 0, 188, 100 });

		// sizeRatio指定の子はバーを避けた幅になる
		CHECK(env.contentNode->regionRect().w == 188.0);

		// 右側の占有は縦スクロール範囲に影響しない
		const auto [minScroll, maxScroll] = env.scrollableNode->validScrollRange();
		CHECK(minScroll.y == 0.0);
		CHECK(maxScroll.y == 900.0);
	}

	SECTION("Inset is not applied when content fits")
	{
		ScrollableNodeEnv env{ 50.0 };
		env.scrollableNode->setScrollBarType(noco::ScrollBarType::Interactive);

		env.canvas->update();
		env.canvas->refreshLayoutImmediately();

		CHECK(env.scrollableNode->childrenRectInset() == noco::LRTB::Zero());
		CHECK(env.contentNode->regionRect().w == 200.0);
	}

	SECTION("Inset is not applied for Overlay and Hidden")
	{
		for (const auto scrollBarType : { noco::ScrollBarType::Overlay, noco::ScrollBarType::Hidden })
		{
			ScrollableNodeEnv env{ 1000.0 };
			env.scrollableNode->setScrollBarType(scrollBarType);

			env.canvas->update();
			env.canvas->refreshLayoutImmediately();

			CHECK(env.scrollableNode->childrenRectInset() == noco::LRTB::Zero());
			CHECK(env.contentNode->regionRect().w == 200.0);
		}
	}

	SECTION("Inset is removed when scroll bar type is changed at runtime")
	{
		ScrollableNodeEnv env{ 1000.0 };
		env.scrollableNode->setScrollBarType(noco::ScrollBarType::Interactive);

		env.canvas->update();
		CHECK(env.scrollableNode->childrenRectInset() == noco::LRTB{ 0, 12, 0, 0 });

		env.scrollableNode->setScrollBarType(noco::ScrollBarType::Hidden);
		env.canvas->update();
		CHECK(env.scrollableNode->childrenRectInset() == noco::LRTB::Zero());
	}

	SECTION("AnchorRegion child avoids scroll bar area")
	{
		ScrollableNodeEnv env{ 1000.0 };
		env.scrollableNode->setScrollBarType(noco::ScrollBarType::Interactive);

		// 親の全面を覆うAnchorRegionの子
		auto anchorChild = noco::Node::Create(U"AnchorChild", noco::AnchorRegion
		{
			.anchorMin = noco::Anchor::TopLeft,
			.anchorMax = noco::Anchor::BottomRight,
			.sizeDelta = Vec2{ 0, 0 },
			.sizeDeltaPivot = noco::Anchor::TopLeft,
		});
		env.scrollableNode->addChild(anchorChild);

		env.canvas->update();
		env.canvas->refreshLayoutImmediately();

		CHECK(anchorChild->regionRect().w == 188.0);
		CHECK(anchorChild->regionRect().h == 100.0);
	}

	SECTION("scrollBarThickness and scrollBarMargin affect inset")
	{
		ScrollableNodeEnv env{ 1000.0 };
		env.scrollableNode->setScrollBarType(noco::ScrollBarType::Interactive);
		env.scrollableNode->setScrollBarThickness(20.0);

		env.canvas->update();
		env.canvas->refreshLayoutImmediately();

		// 占有幅はmargin.left + thickness + margin.right
		CHECK(env.scrollableNode->childrenRectInset() == noco::LRTB{ 0, 24, 0, 0 });
		CHECK(env.contentNode->regionRect().w == 176.0);

		env.scrollableNode->setScrollBarMargin(noco::LRTB{ 4.0, 1.0, 0.0, 0.0 });
		env.canvas->update();
		env.canvas->refreshLayoutImmediately();

		CHECK(env.scrollableNode->childrenRectInset() == noco::LRTB{ 0, 25, 0, 0 });
		CHECK(env.contentNode->regionRect().w == 175.0);
	}

	SECTION("Horizontal scroll bar reserves bottom inset")
	{
		auto canvas = noco::Canvas::Create();
		auto scrollableNode = noco::Node::Create(U"Scrollable", noco::AnchorRegion
		{
			.anchorMin = noco::Anchor::TopLeft,
			.anchorMax = noco::Anchor::TopLeft,
			.sizeDelta = Vec2{ 200, 100 },
			.sizeDeltaPivot = noco::Anchor::TopLeft,
		});
		canvas->addChild(scrollableNode);
		scrollableNode->setHorizontalScrollable(true);
		scrollableNode->setScrollBarType(noco::ScrollBarType::Interactive);

		auto contentNode = noco::Node::Create(U"Content", noco::InlineRegion
		{
			.sizeRatio = Vec2{ 0, 1 },
			.sizeDelta = Vec2{ 1000, 0 },
		});
		scrollableNode->addChild(contentNode);
		canvas->refreshLayoutImmediately();

		canvas->update();
		canvas->refreshLayoutImmediately();

		CHECK(scrollableNode->childrenRectInset() == noco::LRTB{ 0, 0, 0, 12 });
		CHECK(contentNode->regionRect().h == 88.0);
	}

	SECTION("Serialization round-trip")
	{
		auto node = noco::Node::Create();
		node->setScrollBarType(noco::ScrollBarType::Interactive);
		node->setScrollBarHandleColor(Color{ 10, 20, 30, 40 });
		node->setScrollBarThickness(6.0);
		node->setScrollBarMargin(noco::LRTB{ 1.0, 2.0, 3.0, 4.0 });

		const JSON json = node->toJSON();
		CHECK(json[U"scrollBarType"].getString() == U"Interactive");
		CHECK(json.hasElement(U"scrollBarHandleColor"));
		CHECK(json.hasElement(U"scrollBarThickness"));
		CHECK(json.hasElement(U"scrollBarMargin"));

		auto loadedNode = noco::Node::CreateFromJSON(json);
		REQUIRE(loadedNode != nullptr);
		CHECK(loadedNode->scrollBarType() == noco::ScrollBarType::Interactive);
		CHECK(loadedNode->scrollBarHandleColor() == Color{ 10, 20, 30, 40 });
		CHECK(loadedNode->scrollBarThickness() == 6.0);
		CHECK(loadedNode->scrollBarMargin() == noco::LRTB{ 1.0, 2.0, 3.0, 4.0 });
	}

	SECTION("Serialization defaults when keys are missing")
	{
		const JSON json = noco::Node::Create()->toJSON();
		JSON jsonWithoutKeys = json;
		jsonWithoutKeys.erase(U"scrollBarHandleColor");
		jsonWithoutKeys.erase(U"scrollBarThickness");
		jsonWithoutKeys.erase(U"scrollBarMargin");

		auto loadedNode = noco::Node::CreateFromJSON(jsonWithoutKeys);
		REQUIRE(loadedNode != nullptr);
		CHECK(loadedNode->scrollBarHandleColor() == Color{ 255, 255, 255, 160 });
		CHECK(loadedNode->scrollBarThickness() == 8.0);
		CHECK(loadedNode->scrollBarMargin() == noco::LRTB{ 2.0, 2.0, 2.0, 2.0 });
	}

	SECTION("setScrollOffset clamps to valid range")
	{
		ScrollableNodeEnv env{ 1000.0 };

		env.scrollableNode->setScrollOffset(Vec2{ 0, 400 });
		CHECK(env.scrollableNode->scrollOffset() == Vec2{ 0, 400 });

		// 範囲外の値は制限される
		env.scrollableNode->setScrollOffset(Vec2{ 0, 10000 });
		CHECK(env.scrollableNode->scrollOffset() == Vec2{ 0, 900 });

		env.scrollableNode->setScrollOffset(Vec2{ 0, -10000 });
		CHECK(env.scrollableNode->scrollOffset() == Vec2{ 0, 0 });

		// スクロール不可の軸は0に制限される
		env.scrollableNode->setScrollOffset(Vec2{ 100, 200 });
		CHECK(env.scrollableNode->scrollOffset() == Vec2{ 0, 200 });
	}
}
