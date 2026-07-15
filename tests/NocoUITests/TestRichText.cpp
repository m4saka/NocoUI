#include <catch2/catch.hpp>
#include <Siv3D.hpp>
#include <NocoUI.hpp>
#include <NocoUI/detail/RichText.hpp>

// ========================================
// リッチテキストのパースのテスト
// ========================================

TEST_CASE("ParseRichText", "[RichText]")
{
	SECTION("Plain text passthrough")
	{
		const auto result = noco::detail::ParseRichText(U"Hello\nWorld", 24.0);
		REQUIRE(result.text == U"Hello\nWorld");
		REQUIRE(result.charStyles.size() == result.text.size());
		for (const auto& style : result.charStyles)
		{
			REQUIRE(style.sizeScale == 1.0);
			REQUIRE(!style.color.has_value());
			REQUIRE(!style.outlineColor.has_value());
		}
	}

	SECTION("Size tag with percent")
	{
		const auto result = noco::detail::ParseRichText(U"<size=150%>AB</size>C", 24.0);
		REQUIRE(result.text == U"ABC");
		REQUIRE(result.charStyles[0].sizeScale == Approx(1.5));
		REQUIRE(result.charStyles[1].sizeScale == Approx(1.5));
		REQUIRE(result.charStyles[2].sizeScale == 1.0);
	}

	SECTION("Size tag with absolute value")
	{
		const auto result = noco::detail::ParseRichText(U"<size=24>A</size>", 48.0);
		REQUIRE(result.text == U"A");
		REQUIRE(result.charStyles[0].sizeScale == Approx(0.5));
	}

	SECTION("Nested size tags use inner value")
	{
		const auto result = noco::detail::ParseRichText(U"<size=200%>A<size=50%>B</size>C</size>", 24.0);
		REQUIRE(result.text == U"ABC");
		REQUIRE(result.charStyles[0].sizeScale == Approx(2.0));
		REQUIRE(result.charStyles[1].sizeScale == Approx(0.5));
		REQUIRE(result.charStyles[2].sizeScale == Approx(2.0));
	}

	SECTION("Invalid size values are ignored")
	{
		// 数値でない
		const auto badSize = noco::detail::ParseRichText(U"<size=abc>A", 24.0);
		REQUIRE(badSize.text == U"A");
		REQUIRE(badSize.charStyles[0].sizeScale == 1.0);

		// 非正の値
		const auto zeroSize = noco::detail::ParseRichText(U"<size=0>A", 24.0);
		REQUIRE(zeroSize.text == U"A");
		REQUIRE(zeroSize.charStyles[0].sizeScale == 1.0);

		const auto negativeSize = noco::detail::ParseRichText(U"<size=-24>A", 24.0);
		REQUIRE(negativeSize.text == U"A");
		REQUIRE(negativeSize.charStyles[0].sizeScale == 1.0);

		// 非有限値(ParseOptは"inf"を数値として受理するため明示的に弾く)
		const auto infSize = noco::detail::ParseRichText(U"<size=inf>A", 24.0);
		REQUIRE(infSize.text == U"A");
		REQUIRE(infSize.charStyles[0].sizeScale == 1.0);

		const auto infPercent = noco::detail::ParseRichText(U"<size=inf%>A", 24.0);
		REQUIRE(infPercent.text == U"A");
		REQUIRE(infPercent.charStyles[0].sizeScale == 1.0);
	}

	SECTION("Color tag with single color")
	{
		const auto result = noco::detail::ParseRichText(U"<color=#FF0000>A</color>B", 24.0);
		REQUIRE(result.text == U"AB");
		REQUIRE(result.charStyles[0].color.has_value());
		REQUIRE(result.charStyles[0].color->color1 == Color{ 255, 0, 0, 255 });
		REQUIRE(!result.charStyles[0].color->isGradation());
		REQUIRE(!result.charStyles[1].color.has_value());
	}

	SECTION("Color tag with alpha")
	{
		const auto result = noco::detail::ParseRichText(U"<color=#FF000080>A</color>", 24.0);
		REQUIRE(result.text == U"A");
		REQUIRE(result.charStyles[0].color->color1 == Color{ 255, 0, 0, 128 });
	}

	SECTION("Color tag with lowercase hex")
	{
		const auto result = noco::detail::ParseRichText(U"<color=#ff8040>A</color>", 24.0);
		REQUIRE(result.text == U"A");
		REQUIRE(result.charStyles[0].color->color1 == Color{ 255, 128, 64, 255 });
	}

	SECTION("Color tag with gradation pair")
	{
		const auto result = noco::detail::ParseRichText(U"<color=#FF0000,#0000FF>A</color>", 24.0);
		REQUIRE(result.text == U"A");
		REQUIRE(result.charStyles[0].color.has_value());
		REQUIRE(result.charStyles[0].color->isGradation());
		REQUIRE(result.charStyles[0].color->color1 == Color{ 255, 0, 0, 255 });
		REQUIRE(*result.charStyles[0].color->color2 == Color{ 0, 0, 255, 255 });
	}

	SECTION("Invalid color values are ignored")
	{
		// 色名は受け付けない
		const auto colorName = noco::detail::ParseRichText(U"<color=red>A</color>", 24.0);
		REQUIRE(colorName.text == U"A");
		REQUIRE(!colorName.charStyles[0].color.has_value());

		// 16進数でない色コード
		const auto badHex = noco::detail::ParseRichText(U"<color=#GGGGGG>A", 24.0);
		REQUIRE(badHex.text == U"A");
		REQUIRE(!badHex.charStyles[0].color.has_value());

		// 桁数不足の色コード
		const auto shortHex = noco::detail::ParseRichText(U"<color=#FFF>A", 24.0);
		REQUIRE(shortHex.text == U"A");
		REQUIRE(!shortHex.charStyles[0].color.has_value());

		// '#'なし
		const auto noHash = noco::detail::ParseRichText(U"<color=FF0000>A", 24.0);
		REQUIRE(noHash.text == U"A");
		REQUIRE(!noHash.charStyles[0].color.has_value());

		// 3色以上の指定
		const auto threeColors = noco::detail::ParseRichText(U"<color=#FF0000,#00FF00,#0000FF>A", 24.0);
		REQUIRE(threeColors.text == U"A");
		REQUIRE(!threeColors.charStyles[0].color.has_value());

		// 片方が不正なグラデーション指定
		const auto halfBad = noco::detail::ParseRichText(U"<color=#FF0000,#GGGGGG>A", 24.0);
		REQUIRE(halfBad.text == U"A");
		REQUIRE(!halfBad.charStyles[0].color.has_value());
	}

	SECTION("Outline color tag")
	{
		const auto result = noco::detail::ParseRichText(U"<outlinecolor=#00FF00>A</outlinecolor>B", 24.0);
		REQUIRE(result.text == U"AB");
		REQUIRE(result.charStyles[0].outlineColor == Color{ 0, 255, 0, 255 });
		REQUIRE(!result.charStyles[1].outlineColor.has_value());
	}

	SECTION("Outline color tag does not accept comma-separated values")
	{
		const auto result = noco::detail::ParseRichText(U"<outlinecolor=#FF0000,#0000FF>A", 24.0);
		REQUIRE(result.text == U"A");
		REQUIRE(!result.charStyles[0].outlineColor.has_value());
	}

	SECTION("Escape tags")
	{
		const auto result = noco::detail::ParseRichText(U"<lt>tag<gt>", 24.0);
		REQUIRE(result.text == U"<tag>");
	}

	SECTION("Escaped character has current style")
	{
		const auto result = noco::detail::ParseRichText(U"<color=#FF0000><lt></color>", 24.0);
		REQUIRE(result.text == U"<");
		REQUIRE(result.charStyles[0].color.has_value());
		REQUIRE(result.charStyles[0].color->color1 == Color{ 255, 0, 0, 255 });
	}

	SECTION("Unclosed tag applies until end")
	{
		const auto result = noco::detail::ParseRichText(U"<color=#FF0000>AB", 24.0);
		REQUIRE(result.text == U"AB");
		REQUIRE(result.charStyles[0].color->color1 == Color{ 255, 0, 0, 255 });
		REQUIRE(result.charStyles[1].color->color1 == Color{ 255, 0, 0, 255 });
	}

	SECTION("Excess close tag is removed")
	{
		const auto result = noco::detail::ParseRichText(U"</color>A", 24.0);
		REQUIRE(result.text == U"A");
		REQUIRE(!result.charStyles[0].color.has_value());
	}

	SECTION("Crossed tags are tolerated")
	{
		// タグ種別ごとに独立したスタックのため、交差した閉じ順でも各範囲が正しく適用される
		const auto result = noco::detail::ParseRichText(U"<size=200%>A<color=#FF0000>B</size>C</color>D", 24.0);
		REQUIRE(result.text == U"ABCD");
		REQUIRE(result.charStyles[0].sizeScale == Approx(2.0));
		REQUIRE(!result.charStyles[0].color.has_value());
		REQUIRE(result.charStyles[1].sizeScale == Approx(2.0));
		REQUIRE(result.charStyles[1].color->color1 == Color{ 255, 0, 0, 255 });
		REQUIRE(result.charStyles[2].sizeScale == 1.0);
		REQUIRE(result.charStyles[2].color->color1 == Color{ 255, 0, 0, 255 });
		REQUIRE(result.charStyles[3].sizeScale == 1.0);
		REQUIRE(!result.charStyles[3].color.has_value());
	}

	SECTION("Unknown tags are removed without effect")
	{
		const auto result = noco::detail::ParseRichText(U"<unknown>A</unknown>", 24.0);
		REQUIRE(result.text == U"A");
		REQUIRE(result.charStyles[0].sizeScale == 1.0);
		REQUIRE(!result.charStyles[0].color.has_value());
	}

	SECTION("Lone angle bracket is kept as literal")
	{
		const auto result = noco::detail::ParseRichText(U"a < b", 24.0);
		REQUIRE(result.text == U"a < b");
	}

	SECTION("Newline keeps styles aligned with text")
	{
		const auto result = noco::detail::ParseRichText(U"<size=200%>A\nB</size>", 24.0);
		REQUIRE(result.text == U"A\nB");
		REQUIRE(result.charStyles.size() == 3);
		REQUIRE(result.charStyles[1].sizeScale == Approx(2.0));
		REQUIRE(result.charStyles[2].sizeScale == Approx(2.0));
	}

	SECTION("Tags only results in empty text")
	{
		const auto result = noco::detail::ParseRichText(U"<color=#FF0000></color>", 24.0);
		REQUIRE(result.text.isEmpty());
		REQUIRE(result.charStyles.isEmpty());
	}
}

// ========================================
// EscapeRichTextのテスト
// ========================================

TEST_CASE("EscapeRichText", "[RichText]")
{
	SECTION("Escapes angle brackets")
	{
		REQUIRE(noco::EscapeRichText(U"a<b>c") == U"a<lt>b<gt>c");
		REQUIRE(noco::EscapeRichText(U"no brackets") == U"no brackets");
		REQUIRE(noco::EscapeRichText(U"") == U"");
	}

	SECTION("Escaped text roundtrips through parser without styles")
	{
		const String original = U"<size=48>a<color=#FF0000,b</color>";
		const auto result = noco::detail::ParseRichText(noco::EscapeRichText(original), 24.0);
		REQUIRE(result.text == original);
		for (const auto& style : result.charStyles)
		{
			REQUIRE(style.sizeScale == 1.0);
			REQUIRE(!style.color.has_value());
			REQUIRE(!style.outlineColor.has_value());
		}
	}
}
