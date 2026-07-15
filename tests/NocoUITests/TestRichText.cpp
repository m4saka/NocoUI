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
		// %指定は外側の実効サイズに対する割合として計算
		const auto result = noco::detail::ParseRichText(U"<size=200%>A<size=50%>B</size>C</size>", 24.0);
		REQUIRE(result.text == U"ABC");
		REQUIRE(result.charStyles[0].sizeScale == Approx(2.0));
		REQUIRE(result.charStyles[1].sizeScale == Approx(1.0));
		REQUIRE(result.charStyles[2].sizeScale == Approx(2.0));
	}

	SECTION("Percent size tag is relative to outer absolute size")
	{
		// 絶対指定の内側の%はそのサイズに対する割合として計算
		const auto result = noco::detail::ParseRichText(U"<size=48><size=50%>A</size>B</size>", 24.0);
		REQUIRE(result.text == U"AB");
		REQUIRE(result.charStyles[0].sizeScale == Approx(1.0));
		REQUIRE(result.charStyles[1].sizeScale == Approx(2.0));
	}

	SECTION("Nested percent size tags are cumulative")
	{
		const auto result = noco::detail::ParseRichText(U"<size=200%><size=200%>A</size></size>", 24.0);
		REQUIRE(result.text == U"A");
		REQUIRE(result.charStyles[0].sizeScale == Approx(4.0));
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

	SECTION("Value-less tags do not accept values")
	{
		// 値付きのlt/gtは不正タグとして除去され、文字は出力されない
		const auto ltWithValue = noco::detail::ParseRichText(U"A<lt=invalid>B", 24.0);
		REQUIRE(ltWithValue.text == U"AB");

		const auto gtWithValue = noco::detail::ParseRichText(U"A<gt=invalid>B", 24.0);
		REQUIRE(gtWithValue.text == U"AB");
	}

	SECTION("Malformed close tags do not change styles")
	{
		// 値付きの閉じタグは不正タグとして無視され、popは行われない
		const auto color = noco::detail::ParseRichText(U"<color=#FF0000>A</color=invalid>B</color>C", 24.0);
		REQUIRE(color.text == U"ABC");
		REQUIRE(color.charStyles[0].color.has_value());
		REQUIRE(color.charStyles[1].color.has_value());
		REQUIRE(!color.charStyles[2].color.has_value());

		const auto size = noco::detail::ParseRichText(U"<size=200%>A</size=invalid>B</size>C", 24.0);
		REQUIRE(size.text == U"ABC");
		REQUIRE(size.charStyles[0].sizeScale == Approx(2.0));
		REQUIRE(size.charStyles[1].sizeScale == Approx(2.0));
		REQUIRE(size.charStyles[2].sizeScale == 1.0);

		const auto outline = noco::detail::ParseRichText(U"<outlinecolor=#00FF00>A</outlinecolor=invalid>B", 24.0);
		REQUIRE(outline.text == U"AB");
		REQUIRE(outline.charStyles[0].outlineColor.has_value());
		REQUIRE(outline.charStyles[1].outlineColor.has_value());
	}

	SECTION("Invalid open tag does not consume close tag")
	{
		// 不正な開始タグは存在しなかったものとして扱われ、次の閉じタグは外側の有効なタグを閉じる
		const auto result = noco::detail::ParseRichText(U"<color=#FF0000>A<color=invalid>B</color>C</color>", 24.0);
		REQUIRE(result.text == U"ABC");
		REQUIRE(result.charStyles[0].color.has_value());
		REQUIRE(result.charStyles[1].color.has_value());
		REQUIRE(!result.charStyles[2].color.has_value());
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

	SECTION("Strict tag syntax")
	{
		// タグ名や値の空白文字は許容せず未知タグとして除去
		const auto spaced = noco::detail::ParseRichText(U"< size = 48 >A", 24.0);
		REQUIRE(spaced.text == U"A");
		REQUIRE(spaced.charStyles[0].sizeScale == 1.0);

		// タグ名は小文字のみ有効
		const auto upper = noco::detail::ParseRichText(U"<SIZE=48>A", 24.0);
		REQUIRE(upper.text == U"A");
		REQUIRE(upper.charStyles[0].sizeScale == 1.0);

		// 引用符で囲まれた値は許容しない
		const auto quoted = noco::detail::ParseRichText(U"<color=\"#FF0000\">A", 24.0);
		REQUIRE(quoted.text == U"A");
		REQUIRE(!quoted.charStyles[0].color.has_value());

		// 色コードのカンマ区切りの空白も許容しない
		const auto spacedGradation = noco::detail::ParseRichText(U"<color=#FF0000, #0000FF>A", 24.0);
		REQUIRE(spacedGradation.text == U"A");
		REQUIRE(!spacedGradation.charStyles[0].color.has_value());

		// サイズの値の先頭の空白も許容しない
		const auto spacedSize = noco::detail::ParseRichText(U"<size= 48>A", 24.0);
		REQUIRE(spacedSize.text == U"A");
		REQUIRE(spacedSize.charStyles[0].sizeScale == 1.0);

		const auto spacedPercent = noco::detail::ParseRichText(U"<size= 200%>A", 24.0);
		REQUIRE(spacedPercent.text == U"A");
		REQUIRE(spacedPercent.charStyles[0].sizeScale == 1.0);

		// サイズの値は余分なsuffix等を受け付けない
		const auto trailingGarbage = noco::detail::ParseRichText(U"<size=48px>A", 24.0);
		REQUIRE(trailingGarbage.text == U"A");
		REQUIRE(trailingGarbage.charStyles[0].sizeScale == 1.0);

		const auto trailingSpace = noco::detail::ParseRichText(U"<size=48 >A", 24.0);
		REQUIRE(trailingSpace.text == U"A");
		REQUIRE(trailingSpace.charStyles[0].sizeScale == 1.0);

		const auto exponent = noco::detail::ParseRichText(U"<size=1e2>A", 24.0);
		REQUIRE(exponent.text == U"A");
		REQUIRE(exponent.charStyles[0].sizeScale == 1.0);

		const auto plusSign = noco::detail::ParseRichText(U"<size=+48>A", 24.0);
		REQUIRE(plusSign.text == U"A");
		REQUIRE(plusSign.charStyles[0].sizeScale == 1.0);

		// 小数はそのまま有効
		const auto decimal = noco::detail::ParseRichText(U"<size=12.5>A", 24.0);
		REQUIRE(decimal.text == U"A");
		REQUIRE(decimal.charStyles[0].sizeScale == Approx(12.5 / 24.0));
	}

	SECTION("Unterminated tag removes rest of text")
	{
		const auto result = noco::detail::ParseRichText(U"AB<color", 24.0);
		REQUIRE(result.text == U"AB");

		const auto loneBracket = noco::detail::ParseRichText(U"a < b", 24.0);
		REQUIRE(loneBracket.text == U"a ");
	}

	SECTION("Angle bracket inside tag is consumed as part of the tag")
	{
		// タグ内にさらに'<'があった場合も'>'までを1つの不正タグとして除去
		const auto result = noco::detail::ParseRichText(U"a<b<size=48>c", 24.0);
		REQUIRE(result.text == U"ac");
		REQUIRE(result.charStyles[0].sizeScale == 1.0);
		REQUIRE(result.charStyles[1].sizeScale == 1.0);
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
