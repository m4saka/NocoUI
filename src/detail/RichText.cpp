#include <cmath>
#include "NocoUI/detail/RichText.hpp"

namespace noco::detail
{
	namespace
	{
		// リッチテキストのタグ長上限('<'の次の文字から'>'までの文字数)
		constexpr size_t MaxRichTextTagLength = 64;

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

		/// @brief sizeタグの値をパースしてスケール値を返す(数値はピクセル指定、%付きは割合指定。解釈できない場合はnone)
		[[nodiscard]]
		Optional<double> ParseRichTextSizeScale(const String& value, double baseFontSize)
		{
			if (baseFontSize <= 0.0)
			{
				return none;
			}
			if (value.ends_with(U'%'))
			{
				// ParseOptは"inf"等も数値として受理するため有限値のみ許容する
				const auto percentOpt = ParseOpt<double>(value.substr(0, value.size() - 1));
				if (percentOpt && std::isfinite(*percentOpt) && *percentOpt > 0.0)
				{
					return *percentOpt / 100.0;
				}
				return none;
			}
			const auto sizeOpt = ParseOpt<double>(value);
			if (sizeOpt && std::isfinite(*sizeOpt) && *sizeOpt > 0.0)
			{
				return *sizeOpt / baseFontSize;
			}
			return none;
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
				// 上限文字数以内に'>'があればタグとして解釈(見つからなければ'<'を通常文字として扱う)
				size_t closePos = 0;
				bool hasClose = false;
				const size_t searchEnd = Min(text.size(), i + 1 + MaxRichTextTagLength + 1);
				for (size_t j = i + 1; j < searchEnd; ++j)
				{
					if (text[j] == U'>')
					{
						closePos = j;
						hasClose = true;
						break;
					}
					if (text[j] == U'<')
					{
						break;
					}
				}
				if (hasClose)
				{
					String tagContent = text.substr(i + 1, closePos - i - 1);
					const bool isClosing = tagContent.starts_with(U'/');
					if (isClosing)
					{
						tagContent = tagContent.substr(1);
					}

					// '='で名前と値に分割
					String name;
					String value;
					if (const size_t eqPos = tagContent.indexOf(U'='); eqPos != String::npos)
					{
						name = tagContent.substr(0, eqPos);
						value = tagContent.substr(eqPos + 1);
					}
					else
					{
						name = tagContent;
					}
					name = name.trimmed().lowercased();
					value = value.trimmed();

					// 引用符で囲まれた値を許容
					if (value.size() >= 2 && value.starts_with(U'"') && value.ends_with(U'"'))
					{
						value = value.substr(1, value.size() - 2);
					}

					if (name == U"lt" || name == U"gt")
					{
						// リテラルの'<'または'>'を出力する置換型エスケープ(閉じタグ形式は効果なし)
						if (!isClosing)
						{
							fnPushChar(name == U"lt" ? U'<' : U'>');
						}
					}
					else if (name == U"size")
					{
						if (isClosing)
						{
							if (!sizeScaleStack.isEmpty())
							{
								sizeScaleStack.pop_back();
							}
						}
						else if (const auto scaleOpt = ParseRichTextSizeScale(value, baseFontSize))
						{
							sizeScaleStack.push_back(*scaleOpt);
						}
					}
					else if (name == U"color")
					{
						if (isClosing)
						{
							if (!colorStack.isEmpty())
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
								if (const auto colorOpt = ParseRichTextColor(colorValues[0].trimmed()))
								{
									colorStack.push_back(RichTextColor{ .color1 = *colorOpt });
								}
							}
							else if (colorValues.size() == 2)
							{
								const auto color1Opt = ParseRichTextColor(colorValues[0].trimmed());
								const auto color2Opt = ParseRichTextColor(colorValues[1].trimmed());
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
							if (!outlineColorStack.isEmpty())
							{
								outlineColorStack.pop_back();
							}
						}
						else if (const auto colorOpt = ParseRichTextColor(value))
						{
							outlineColorStack.push_back(*colorOpt);
						}
					}
					// 未知のタグは効果なしで読み飛ばす(表示もしない)

					i = closePos + 1;
					continue;
				}
			}

			fnPushChar(text[i]);
			++i;
		}
		return result;
	}
}
