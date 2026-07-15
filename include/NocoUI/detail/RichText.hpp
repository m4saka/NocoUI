#pragma once
#include <Siv3D.hpp>

namespace noco::detail
{
	/// @brief リッチテキストのcolorタグの色指定(単色または上下グラデーション)
	struct RichTextColor
	{
		Color color1;
		Optional<Color> color2 = none; // 2色指定時の下端色(上下グラデーション)

		/// @brief 上下グラデーション指定かどうか
		[[nodiscard]]
		bool isGradation() const
		{
			return color2.has_value();
		}
	};

	/// @brief リッチテキストの1文字分の装飾情報
	struct RichTextCharStyle
	{
		double sizeScale = 1.0;
		Optional<RichTextColor> color = none;
		Optional<Color> outlineColor = none;
	};

	/// @brief リッチテキストのパース結果(タグ除去後のテキストと文字ごとの装飾情報)
	struct RichTextParseResult
	{
		String text;
		Array<RichTextCharStyle> charStyles;
	};

	/// @brief リッチテキストをパースしてタグ除去後のテキストと文字ごとの装飾情報を返す(不正なタグは黙って無視する)
	[[nodiscard]]
	RichTextParseResult ParseRichText(const String& text, double baseFontSize);
}
