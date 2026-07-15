#include <cmath>
#include "NocoUI/detail/RichText.hpp"

namespace noco::detail
{
	namespace
	{
		/// @brief colorタグの値をパース(#RRGGBBまたは#RRGGBBAAのみ対応。解釈できない場合はnone)
		[[nodiscard]]
		Optional<Color> ParseRichTextColor(const String& value)
		{
			if (!value.starts_with(U'#'))
			{
				return none;
			}
			const size_t hexLength = value.size() - 1;
			if (hexLength != 6 && hexLength != 8)
			{
				return none;
			}
			uint32 parsed = 0;
			for (size_t i = 1; i < value.size(); ++i)
			{
				const char32 ch = value[i];
				uint32 digit;
				if (U'0' <= ch && ch <= U'9')
				{
					digit = ch - U'0';
				}
				else if (U'a' <= ch && ch <= U'f')
				{
					digit = ch - U'a' + 10;
				}
				else if (U'A' <= ch && ch <= U'F')
				{
					digit = ch - U'A' + 10;
				}
				else
				{
					return none;
				}
				parsed = parsed * 16 + digit;
			}
			if (hexLength == 6)
			{
				return Color{ static_cast<uint8>((parsed >> 16) & 0xFF), static_cast<uint8>((parsed >> 8) & 0xFF), static_cast<uint8>(parsed & 0xFF) };
			}
			return Color{ static_cast<uint8>((parsed >> 24) & 0xFF), static_cast<uint8>((parsed >> 16) & 0xFF), static_cast<uint8>((parsed >> 8) & 0xFF), static_cast<uint8>(parsed & 0xFF) };
		}

		/// @brief 数値文字列(数字、小数点のみ)かどうか
		/// @remark 指数表記(1e3など)は受け付けない
		[[nodiscard]]
		bool IsStrictNumber(StringView s)
		{
			bool hasDigit = false;
			bool hasDot = false;
			for (const char32 ch : s)
			{
				if (U'0' <= ch && ch <= U'9')
				{
					hasDigit = true;
				}
				else if (ch == U'.')
				{
					if (hasDot)
					{
						return false;
					}
					hasDot = true;
				}
				else
				{
					return false;
				}
			}
			return hasDigit;
		}

		/// @brief sizeタグの値をパースしてスケール値を返す(数値指定はピクセル数、%指定は現在のサイズに対する割合。解釈できない場合はnone)
		[[nodiscard]]
		Optional<double> ParseRichTextSizeScale(const String& value, double baseFontSize, double currentScale)
		{
			if (baseFontSize <= 0.0)
			{
				return none;
			}

			const bool isPercent = value.ends_with(U'%');
			const StringView numberPart = StringView{ value }.substr(0, isPercent ? value.size() - 1 : value.size());

			// 数値以外の文字を含む値は不正
			if (!IsStrictNumber(numberPart))
			{
				return none;
			}

			const auto parsedOpt = ParseOpt<double>(numberPart);
			if (!parsedOpt || !std::isfinite(*parsedOpt) || *parsedOpt <= 0.0)
			{
				return none;
			}
			if (isPercent)
			{
				return currentScale * *parsedOpt / 100.0;
			}
			return *parsedOpt / baseFontSize;
		}
	}

	RichTextParseResult ParseRichText(const String& text, double baseFontSize)
	{
		RichTextParseResult result;
		result.text.reserve(text.size());
		result.charStyles.reserve(text.size());

		// タグ種別ごとに独立したスタックを持つ(交差したタグも許容するため)
		Array<double> sizeScaleStack;
		Array<RichTextColor> colorStack;
		Array<Color> outlineColorStack;

		// 現在のスタイルで1文字追加
		const auto fnPushChar =
			[&](char32 ch)
			{
				result.text.push_back(ch);
				result.charStyles.push_back(RichTextCharStyle{
					.sizeScale = sizeScaleStack.isEmpty() ? 1.0 : sizeScaleStack.back(),
					.color = colorStack.isEmpty() ? Optional<RichTextColor>{ none } : Optional<RichTextColor>{ colorStack.back() },
					.outlineColor = outlineColorStack.isEmpty() ? Optional<Color>{ none } : Optional<Color>{ outlineColorStack.back() },
				});
			};

		for (size_t i = 0; i < text.size();)
		{
			if (text[i] == U'<')
			{
				const size_t closePos = text.indexOf(U'>', i + 1);
				if (closePos == String::npos)
				{
					// '>'で閉じられていない'<'以降は不正なタグとして除去
					break;
				}

				String tagContent = text.substr(i + 1, closePos - i - 1);
				const bool isClosing = tagContent.starts_with(U'/');
				if (isClosing)
				{
					tagContent = tagContent.substr(1);
				}

				// タグ名と値に分割
				String name;
				String value;
				bool hasValue = false;
				if (const size_t eqPos = tagContent.indexOf(U'='); eqPos != String::npos)
				{
					name = tagContent.substr(0, eqPos);
					value = tagContent.substr(eqPos + 1);
					hasValue = true;
				}
				else
				{
					name = tagContent;
				}

				if (name == U"lt" || name == U"gt")
				{
					// リテラルの'<'または'>'を出力する置換型エスケープ(閉じタグ形式は効果なし)
					if (!isClosing && !hasValue)
					{
						fnPushChar(name == U"lt" ? U'<' : U'>');
					}
				}
				else if (name == U"size")
				{
					if (isClosing)
					{
						if (!hasValue && !sizeScaleStack.isEmpty())
						{
							sizeScaleStack.pop_back();
						}
					}
					else
					{
						// %指定は現在のサイズに対する割合として計算
						const double currentScale = sizeScaleStack.isEmpty() ? 1.0 : sizeScaleStack.back();
						if (const auto scaleOpt = ParseRichTextSizeScale(value, baseFontSize, currentScale))
						{
							sizeScaleStack.push_back(*scaleOpt);
						}
					}
				}
				else if (name == U"color")
				{
					if (isClosing)
					{
						if (!hasValue && !colorStack.isEmpty())
						{
							colorStack.pop_back();
						}
					}
					else
					{
						// カンマ区切りで2色指定した場合は上下グラデーション
						const Array<String> colorValues = value.split(U',');
						if (colorValues.size() == 1)
						{
							if (const auto colorOpt = ParseRichTextColor(colorValues[0]))
							{
								colorStack.push_back(RichTextColor{ .color1 = *colorOpt });
							}
						}
						else if (colorValues.size() == 2)
						{
							const auto color1Opt = ParseRichTextColor(colorValues[0]);
							const auto color2Opt = ParseRichTextColor(colorValues[1]);
							if (color1Opt && color2Opt)
							{
								colorStack.push_back(RichTextColor{ .color1 = *color1Opt, .color2 = *color2Opt });
							}
						}
						// 3個以上の指定や不正な色はタグを無視
					}
				}
				else if (name == U"outlinecolor")
				{
					if (isClosing)
					{
						if (!hasValue && !outlineColorStack.isEmpty())
						{
							outlineColorStack.pop_back();
						}
					}
					else if (const auto colorOpt = ParseRichTextColor(value))
					{
						outlineColorStack.push_back(*colorOpt);
					}
				}
				// 未知のタグは無視して読み飛ばす(表示もしない)

				i = closePos + 1;
				continue;
			}

			fnPushChar(text[i]);
			++i;
		}
		return result;
	}
}
