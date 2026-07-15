#include <cmath>
#include "NocoUI/Component/Label.hpp"
#include "NocoUI/Node.hpp"
#include "NocoUI/Canvas.hpp"
#include "NocoUI/DefaultFont.hpp"

namespace noco
{
	namespace
	{
		static const String EmptyString = U"";

		// リッチテキストのタグ長上限('<'の次の文字から'>'までの文字数)
		constexpr size_t MaxRichTextTagLength = 64;

		/// @brief リッチテキストの1文字分の装飾情報
		struct RichTextCharStyle
		{
			double sizeScale = 1.0;
			Optional<detail::RichTextColor> color = none;
			Optional<Color> outlineColor = none;
		};

		/// @brief リッチテキストのパース結果(タグ除去後のテキストと文字ごとの装飾情報)
		struct RichTextParseResult
		{
			String text;
			Array<RichTextCharStyle> charStyles;
		};

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

		/// @brief リッチテキストをパースしてタグ除去後のテキストと文字ごとの装飾情報を返す(不正なタグは黙って無視する)
		[[nodiscard]]
		RichTextParseResult ParseRichText(const String& text, double baseFontSize)
		{
			RichTextParseResult result;
			result.text.reserve(text.size());
			result.charStyles.reserve(text.size());

			// タグ種別ごとに独立したスタックを持つ(交差したタグも許容するため)
			Array<double> sizeScaleStack;
			Array<detail::RichTextColor> colorStack;
			Array<Color> outlineColorStack;

			// 現在のスタイルで1文字追加
			const auto fnPushChar =
				[&](char32 ch)
				{
					result.text.push_back(ch);
					result.charStyles.push_back(RichTextCharStyle{
						.sizeScale = sizeScaleStack.isEmpty() ? 1.0 : sizeScaleStack.back(),
						.color = colorStack.isEmpty() ? Optional<detail::RichTextColor>{ none } : Optional<detail::RichTextColor>{ colorStack.back() },
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
										colorStack.push_back(detail::RichTextColor{ .color1 = *colorOpt });
									}
								}
								else if (colorValues.size() == 2)
								{
									const auto color1Opt = ParseRichTextColor(colorValues[0].trimmed());
									const auto color2Opt = ParseRichTextColor(colorValues[1].trimmed());
									if (color1Opt && color2Opt)
									{
										colorStack.push_back(detail::RichTextColor{ .color1 = *color1Opt, .color2 = *color2Opt });
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

	String EscapeRichText(const StringView text)
	{
		String result;
		result.reserve(text.size());
		for (const char32 ch : text)
		{
			if (ch == U'<')
			{
				result += U"<lt>";
			}
			else if (ch == U'>')
			{
				result += U"<gt>";
			}
			else
			{
				result.push_back(ch);
			}
		}
		return result;
	}

	bool Label::Cache::refreshIfDirty(const String& text, bool richTextEnabled, const Optional<Font>& fontOpt, const String& fontAssetName, const String& canvasDefaultFontAssetName, double fontSize, double minFontSize, const Vec2& spacing, HorizontalOverflow horizontalOverflow, VerticalOverflow verticalOverflow, const SizeF& rectSize, LabelSizingMode newSizingMode)
	{
		const bool hasCustomFont = fontOpt.has_value();
		const Font newFont = [&]() -> Font {
			if (hasCustomFont)
			{
				return *fontOpt;
			}

			if (!fontAssetName.empty() && FontAsset::IsRegistered(fontAssetName))
			{
				return FontAsset(fontAssetName);
			}

			if (!canvasDefaultFontAssetName.empty() && FontAsset::IsRegistered(canvasDefaultFontAssetName))
			{
				return FontAsset(canvasDefaultFontAssetName);
			}

			if (auto globalFont = noco::detail::GetGlobalDefaultFont())
			{
				return *globalFont;
			}

			return SimpleGUI::GetFont();
		}();

		if (prevParams.has_value() &&
			!prevParams->isDirty(text, richTextEnabled, fontAssetName, fontSize, minFontSize, horizontalOverflow, verticalOverflow, spacing, rectSize, hasCustomFont, newFont, newSizingMode))
		{
			return false;
		}

		prevParams = CacheParams
		{
			.text = text,
			.richTextEnabled = richTextEnabled,
			.fontAssetName = fontAssetName,
			.fontSize = fontSize,
			.minFontSize = minFontSize,
			.horizontalOverflow = horizontalOverflow,
			.verticalOverflow = verticalOverflow,
			.spacing = spacing,
			.rectSize = rectSize,
			.hasCustomFont = hasCustomFont,
			.customFont = newFont,
			.sizingMode = newSizingMode,
		};

		currentFont = newFont;
		fontMethod = newFont.method();
		assetFontSize = newFont.fontSize();

		// リッチテキストが有効な場合はタグを解釈して文字ごとの装飾情報を作成
		RichTextParseResult richTextParseResult;
		if (richTextEnabled)
		{
			richTextParseResult = ParseRichText(text, fontSize);
		}
		const String& displayText = richTextEnabled ? richTextParseResult.text : text;
		richTextHasOutlineColor = richTextParseResult.charStyles.any(
			[](const RichTextCharStyle& style)
			{
				return style.outlineColor.has_value();
			});

		auto refreshCacheAndGetRegionSize = [&](double targetFontSize, HorizontalOverflow hov, VerticalOverflow vov) -> SizeF
			{
				if (assetFontSize == 0)
				{
					this->assetFontSizeScale = 1.0;
				}
				else
				{
					this->assetFontSizeScale = targetFontSize / assetFontSize;
				}
				this->lineHeight = currentFont.height(targetFontSize);

				lineCaches.clear();

				double maxWidth = 0.0;
				Vec2 offset = Vec2::Zero();
				Array<Glyph> lineGlyphs;
				Array<GlyphStyle> lineGlyphStyles;

				const auto fnPushLine =
					[&]() -> bool
					{
						// 行の高さは行内で最も大きい文字のスケールに合わせる
						double lineMaxScale = 1.0;
						for (const auto& style : lineGlyphStyles)
						{
							lineMaxScale = Max(lineMaxScale, style.scale);
						}
						const double currentLineHeight = this->lineHeight * lineMaxScale;
						const double currentLineBottom = offset.y + currentLineHeight;

						if (vov == VerticalOverflow::Clip && currentLineBottom > rectSize.y)
						{
							return false;
						}

						if (!lineGlyphs.empty())
						{
							// AutoShrinkの場合はスケールを適用
							// (AutoShrinkWidthの場合、通常スケールで計算したサイズとノード幅を元にスケールを決めるため、ここではスケールを適用しない)
							const double spacingScale = newSizingMode == LabelSizingMode::AutoShrink ? targetFontSize / fontSize : 1.0;
							offset.x -= spacing.x * spacingScale;
						}

						// ベースライン揃えのためのYオフセットを確定
						const double scaledAscender = currentFont.ascender() * this->assetFontSizeScale;
						for (auto& style : lineGlyphStyles)
						{
							style.yOffset = (lineMaxScale - style.scale) * scaledAscender;
						}

						// グラデーション用に行内の文字の上端・下端の割合を計算
						double minTopT = 1.0;
						double maxBottomT = 0.0;
						const double normalizeHeight = Max(currentLineHeight, 1.0);
						for (size_t glyphIndex = 0; glyphIndex < lineGlyphs.size(); ++glyphIndex)
						{
							const auto& glyph = lineGlyphs[glyphIndex];
							const double glyphScale = glyphIndex < lineGlyphStyles.size() ? lineGlyphStyles[glyphIndex].scale : 1.0;
							const double glyphYOffset = glyphIndex < lineGlyphStyles.size() ? lineGlyphStyles[glyphIndex].yOffset : 0.0;
							const double effectiveScale = this->assetFontSizeScale * glyphScale;
							const double glyphTop = glyph.getOffset(effectiveScale).y + glyphYOffset;
							minTopT = Min(minTopT, glyphTop / normalizeHeight);
							maxBottomT = Max(maxBottomT, (glyphTop + glyph.texture.size.y * effectiveScale) / normalizeHeight);
						}
						if (minTopT > maxBottomT)
						{
							// 行内に文字がない場合
							minTopT = 0.0;
							maxBottomT = 1.0;
						}

						lineCaches.push_back({
							.glyphs = lineGlyphs,
							.glyphStyles = lineGlyphStyles,
							.width = offset.x,
							.offsetY = offset.y,
							.height = currentLineHeight,
							.minTopT = minTopT,
							.maxBottomT = maxBottomT,
						});
						lineGlyphs.clear();
						lineGlyphStyles.clear();
						maxWidth = Max(maxWidth, offset.x);
						offset.x = 0;

						offset.y = currentLineBottom + spacing.y;
						return true;
					};

				const Array<Glyph> glyphs = currentFont.getGlyphs(displayText);
				for (size_t glyphIndex = 0; glyphIndex < glyphs.size(); ++glyphIndex)
				{
					const auto& glyph = glyphs[glyphIndex];
					if (glyph.codePoint == U'\n')
					{
						if (!fnPushLine())
						{
							break;
						}
						continue;
					}

					RichTextCharStyle charStyle;
					if (richTextEnabled && glyphIndex < richTextParseResult.charStyles.size())
					{
						charStyle = richTextParseResult.charStyles[glyphIndex];
					}

					// AutoShrinkの場合はスケールを適用
					// (AutoShrinkWidthの場合、通常スケールで計算したサイズとノード幅を元にスケールを決めるため、ここではスケールを適用しない)
					const double spacingScale = newSizingMode == LabelSizingMode::AutoShrink ? targetFontSize / fontSize : 1.0;

					const double xAdvance = glyph.xAdvance * this->assetFontSizeScale * charStyle.sizeScale + spacing.x * spacingScale;
					if (hov == HorizontalOverflow::Wrap && offset.x + xAdvance > rectSize.x)
					{
						if (!fnPushLine())
						{
							break;
						}
					}

					offset.x += xAdvance;
					lineGlyphs.push_back(glyph);
					if (richTextEnabled)
					{
						lineGlyphStyles.push_back(GlyphStyle{
							.scale = charStyle.sizeScale,
							.yOffset = 0.0,
							.color = charStyle.color,
							.outlineColor = charStyle.outlineColor,
						});
					}
				}

				fnPushLine();

				return { maxWidth, offset.y - spacing.y };
			};

		if (newSizingMode == LabelSizingMode::AutoShrink)
		{
			double currentFontSize = fontSize;
			while (currentFontSize >= minFontSize)
			{
				// 現在のフォントサイズで収まるかチェック(Overflowで計算)
				const SizeF requiredSize = refreshCacheAndGetRegionSize(currentFontSize, horizontalOverflow, VerticalOverflow::Overflow);
				if (requiredSize.x <= rectSize.x && requiredSize.y <= rectSize.y)
				{
					break;
				}

				currentFontSize -= 1.0;
				if (currentFontSize < minFontSize)
				{
					currentFontSize = minFontSize;
					break;
				}
			}
			this->effectiveFontSize = currentFontSize;
			this->effectiveAutoShrinkWidthScale = 1.0;

			// 最終的なフォントサイズと、実際のVerticalOverflowでキャッシュを確定
			this->regionSize = refreshCacheAndGetRegionSize(this->effectiveFontSize, horizontalOverflow, verticalOverflow);
		}
		else if (newSizingMode == LabelSizingMode::AutoShrinkWidth)
		{
			this->effectiveFontSize = fontSize;

			// AutoShrinkWidthでは折り返さないため常にHorizontalOverflow::Overflowとする
			this->regionSize = refreshCacheAndGetRegionSize(fontSize, HorizontalOverflow::Overflow, verticalOverflow);
			if (this->regionSize.x > rectSize.x && this->regionSize.x > 0.0)
			{
				this->effectiveAutoShrinkWidthScale = rectSize.x / this->regionSize.x;
			}
			else
			{
				this->effectiveAutoShrinkWidthScale = 1.0;
			}
		}
		else if (newSizingMode == LabelSizingMode::AutoShrinkWidthResizeHeight)
		{
			this->effectiveFontSize = fontSize;

			// 折り返さないためHorizontalOverflow::Overflow、高さはリサイズするためVerticalOverflow::Overflow
			this->regionSize = refreshCacheAndGetRegionSize(fontSize, HorizontalOverflow::Overflow, VerticalOverflow::Overflow);
			if (this->regionSize.x > rectSize.x && this->regionSize.x > 0.0)
			{
				this->effectiveAutoShrinkWidthScale = rectSize.x / this->regionSize.x;
			}
			else
			{
				this->effectiveAutoShrinkWidthScale = 1.0;
			}
		}
		else if (newSizingMode == LabelSizingMode::AutoResize)
		{
			// AutoResizeではノードサイズの誤差による折り返しやクリップが発生しないよう、両方Overflowとする
			this->effectiveFontSize = fontSize;
			this->effectiveAutoShrinkWidthScale = 1.0;
			this->regionSize = refreshCacheAndGetRegionSize(fontSize, HorizontalOverflow::Overflow, VerticalOverflow::Overflow);
		}
		else if (newSizingMode == LabelSizingMode::AutoResizeHeight)
		{
			// AutoResizeHeightでは幅は固定し、HorizontalOverflowの設定に従って折り返し、高さのみOverflowとする
			this->effectiveFontSize = fontSize;
			this->effectiveAutoShrinkWidthScale = 1.0;
			this->regionSize = refreshCacheAndGetRegionSize(fontSize, horizontalOverflow, VerticalOverflow::Overflow);
		}
		else // LabelSizingMode::Fixed
		{
			this->effectiveFontSize = fontSize;
			this->effectiveAutoShrinkWidthScale = 1.0;
			this->regionSize = refreshCacheAndGetRegionSize(fontSize, horizontalOverflow, verticalOverflow);
		}

		return true;
	}

	SizeF Label::getContentSizeForAutoResize(const String& canvasDefaultFontAssetName) const
	{
		// rectSize指定なしでのサイズ計算は縮小されないようAutoShrinkはFixedとして扱う
		auto sizingMode = m_sizingMode.value();
		if (sizingMode == LabelSizingMode::AutoShrink || sizingMode == LabelSizingMode::AutoShrinkWidth || sizingMode == LabelSizingMode::AutoShrinkWidthResizeHeight)
		{
			sizingMode = LabelSizingMode::Fixed;
		}

		m_autoResizeCache.refreshIfDirty(
			m_text.value(),
			m_richTextEnabled.value(),
			m_fontOpt,
			m_fontAssetName.value(),
			canvasDefaultFontAssetName,
			m_fontSize.value(),
			m_minFontSize.value(),
			m_characterSpacing.value(),
			HorizontalOverflow::Overflow, // rectSize指定なしでのサイズ計算は折り返さないようOverflowで固定
			VerticalOverflow::Overflow, // rectSize指定なしでのサイズ計算はクリップされないようOverflowで固定
			m_autoResizeCache.prevParams.has_value() ? m_autoResizeCache.prevParams->rectSize : Vec2::Zero(), // rectSizeは使われないので、キャッシュ再更新がなるべく走らないよう前回と同じ値を渡す
			sizingMode);

		// AutoResizeでは小数点以下を切り上げたサイズをノードサイズとして使用
		const SizeF& regionSize = m_autoResizeCache.regionSize;
		const SizeF ceiledRegionSize{ Math::Ceil(regionSize.x), Math::Ceil(regionSize.y) };

		// AutoResizeでは余白を加えたサイズを使用
		const LRTB& padding = m_padding.value();
		return ceiledRegionSize + Vec2{ padding.totalWidth(), padding.totalHeight() };
	}

	void Label::update(const std::shared_ptr<Node>& node)
	{
		refreshAutoResizeImmediately(node);
	}

	void Label::draw(const Node& node) const
	{
		const auto& text = m_text.value();

		if (text.empty())
		{
			return;
		}

		const Vec2& characterSpacing = m_characterSpacing.value();
		const LRTB& padding = m_padding.value();

		const RectF rect = node.regionRect().stretched(
			-padding.top,
			-padding.right,
			-padding.bottom,
			-padding.left
		);

		const String& canvasDefaultFontAssetName = [&node]() -> const String&
			{
				if (const auto canvas = node.containedCanvas())
				{
					return canvas->defaultFontAssetName();
				}
				return EmptyString;
			}();

		m_cache.refreshIfDirty(
			text,
			m_richTextEnabled.value(),
			m_fontOpt,
			m_fontAssetName.value(),
			canvasDefaultFontAssetName,
			m_fontSize.value(),
			m_minFontSize.value(),
			characterSpacing,
			m_horizontalOverflow.value(),
			m_verticalOverflow.value(),
			rect.size,
			m_sizingMode.value());

		const double startY = [this, &rect]()
			{
				const VerticalAlign& verticalAlign = m_verticalAlign.value();
				switch (verticalAlign)
				{
				case VerticalAlign::Top:
					return rect.y;
				case VerticalAlign::Middle:
					return rect.y + (rect.h - m_cache.regionSize.y) / 2;
				case VerticalAlign::Bottom:
					return rect.y + rect.h - m_cache.regionSize.y;
				default:
					throw Error{ U"Invalid VerticalAlign: {}"_fmt(static_cast<std::underlying_type_t<VerticalAlign>>(verticalAlign)) };
				}
			}();

		const HorizontalAlign horizontalAlign = m_horizontalAlign.value();

		const double outlineFactorInner = Max(m_outlineFactorInner.value(), 0.0);
		const double outlineFactorOuter = Max(m_outlineFactorOuter.value(), 0.0);
		const bool hasOutline = (outlineFactorInner != 0.0 || outlineFactorOuter != 0.0) && m_outlineColor.value().a > 0.0;
		const bool hasShadow = m_shadowColor.value().a > 0.0;
		const bool isSDF = m_cache.fontMethod == FontMethod::SDF;
		const bool isMSDF = m_cache.fontMethod == FontMethod::MSDF;

		// SDFアウトラインの色にはScopedColorMul2Dの色が自動では乗らないため乗算が必要
		const ColorF colorMul{ Graphics2D::GetColorMul() };

		TextStyle textStyle = TextStyle::Default();
		if (isSDF || isMSDF)
		{
			if (hasOutline && hasShadow)
			{
				textStyle = TextStyle::OutlineShadow(outlineFactorInner, outlineFactorOuter, ColorF{ m_outlineColor.value() } *colorMul, m_shadowOffset.value(), ColorF{ m_shadowColor.value() } *colorMul);
			}
			else if (hasOutline)
			{
				textStyle = TextStyle::Outline(outlineFactorInner, outlineFactorOuter, ColorF{ m_outlineColor.value() } *colorMul);
			}
			else if (hasShadow)
			{
				textStyle = TextStyle::Shadow(m_shadowOffset.value(), ColorF{ m_shadowColor.value() } *colorMul);
			}
		}

		// リッチテキストのタグによるアウトライン色変更を適用するのはアウトラインが有効な場合のみ
		const bool hasTagOutlineColor = hasOutline && (isSDF || isMSDF) && m_cache.richTextHasOutlineColor;

		const double autoShrinkWidthScale = (m_sizingMode.value() == LabelSizingMode::AutoShrinkWidth || m_sizingMode.value() == LabelSizingMode::AutoShrinkWidthResizeHeight)
			? m_cache.effectiveAutoShrinkWidthScale
			: 1.0;

		{
			const ScopedCustomShader2D shader{ Font::GetPixelShader(m_cache.fontMethod, textStyle.type) };

			if (hasOutline || hasShadow)
			{
				if (isSDF)
				{
					Graphics2D::SetSDFParameters(textStyle);
				}
				else if (isMSDF)
				{
					Graphics2D::SetMSDFParameters(textStyle);
				}
			}

			// 現在適用中のアウトライン色(タグで色が変わる場合、変わり目でのみSDFパラメータを更新する)
			ColorF appliedOutlineColor = ColorF{ m_outlineColor.value() } * colorMul;

			const double horizontalGradationWidth = m_cache.regionSize.x <= 0.0 ? 1.0 : m_cache.regionSize.x;
			const double gradientLeft = [&rect, horizontalAlign, horizontalGradationWidth]()
				{
					switch (horizontalAlign)
					{
					case HorizontalAlign::Left:
						return rect.x;
					case HorizontalAlign::Center:
						return rect.x + (rect.w - horizontalGradationWidth) / 2;
					case HorizontalAlign::Right:
						return rect.x + rect.w - horizontalGradationWidth;
					default:
						throw Error{ U"Invalid HorizontalAlign: {}"_fmt(static_cast<std::underlying_type_t<HorizontalAlign>>(horizontalAlign)) };
					}
				}();
			const LabelGradationType gradationType = m_gradationType.value();
			const ColorF& color = m_color.value();
			const ColorF& gradationColor1 = m_gradationColor1.value();
			const ColorF& gradationColor2 = m_gradationColor2.value();

			for (const auto& lineCache : m_cache.lineCaches)
			{
				const double effectiveLineWidth = lineCache.width * autoShrinkWidthScale;

				const double startX = [&rect, effectiveLineWidth, horizontalAlign]()
					{
						switch (horizontalAlign)
						{
						case HorizontalAlign::Left:
							return rect.x;
						case HorizontalAlign::Center:
							return rect.x + (rect.w - effectiveLineWidth) / 2;
						case HorizontalAlign::Right:
							return rect.x + rect.w - effectiveLineWidth;
						default:
							throw Error{ U"Invalid HorizontalAlign: {}"_fmt(static_cast<std::underlying_type_t<HorizontalAlign>>(horizontalAlign)) };
						}
					}();

				double x = 0;

				for (size_t glyphIndex = 0; glyphIndex < lineCache.glyphs.size(); ++glyphIndex)
				{
					const auto& glyph = lineCache.glyphs[glyphIndex];
					if (glyph.codePoint == U'\n')
					{
						continue;
					}

					Cache::GlyphStyle glyphStyle;
					if (glyphIndex < lineCache.glyphStyles.size())
					{
						glyphStyle = lineCache.glyphStyles[glyphIndex];
					}
					const double drawScale = m_cache.assetFontSizeScale * glyphStyle.scale;

					if (hasTagOutlineColor)
					{
						const ColorF glyphOutlineColor = ColorF{ glyphStyle.outlineColor.value_or(m_outlineColor.value()) } * colorMul;
						if (glyphOutlineColor != appliedOutlineColor)
						{
							TextStyle glyphTextStyle = TextStyle::Default();
							if (hasShadow)
							{
								glyphTextStyle = TextStyle::OutlineShadow(outlineFactorInner, outlineFactorOuter, glyphOutlineColor, m_shadowOffset.value(), ColorF{ m_shadowColor.value() } * colorMul);
							}
							else
							{
								glyphTextStyle = TextStyle::Outline(outlineFactorInner, outlineFactorOuter, glyphOutlineColor);
							}
							if (isSDF)
							{
								Graphics2D::SetSDFParameters(glyphTextStyle);
							}
							else
							{
								Graphics2D::SetMSDFParameters(glyphTextStyle);
							}
							appliedOutlineColor = glyphOutlineColor;
						}
					}

					const Vec2 pos{ startX + x, startY + lineCache.offsetY };
					const Vec2 drawPos = pos + (glyph.getOffset(drawScale) + Vec2{ 0.0, glyphStyle.yOffset }) * Vec2{ autoShrinkWidthScale, 1.0 };
					const auto scaledTexture = glyph.texture.scaled(drawScale * autoShrinkWidthScale, drawScale);

					// colorタグによる色指定はグラデーションより優先
					if (glyphStyle.color.has_value())
					{
						if (glyphStyle.color->isGradation())
						{
							// 2色指定時は行内の文字の上端から下端にかけての上下グラデーション
							const double lineGradationHeight = Max(lineCache.height, 1e-6);
							const double glyphTop = glyph.getOffset(drawScale).y + glyphStyle.yOffset;
							const double topT = Clamp(glyphTop / lineGradationHeight, 0.0, 1.0);
							const double bottomT = Clamp((glyphTop + glyph.texture.size.y * drawScale) / lineGradationHeight, 0.0, 1.0);
							const double minMaxTAbsDiff = Max(lineCache.maxBottomT - lineCache.minTopT, 1e-6);
							const double scaledTopT = (topT - lineCache.minTopT) / minMaxTAbsDiff;
							const double scaledBottomT = (bottomT - lineCache.minTopT) / minMaxTAbsDiff;
							const ColorF tagColor1{ glyphStyle.color->color1 };
							const ColorF tagColor2{ *glyphStyle.color->color2 };
							const ColorF topColor = tagColor1.lerp(tagColor2, scaledTopT);
							const ColorF bottomColor = tagColor1.lerp(tagColor2, scaledBottomT);
							scaledTexture.draw(drawPos, Arg::top = topColor, Arg::bottom = bottomColor);
						}
						else
						{
							scaledTexture.draw(drawPos, ColorF{ glyphStyle.color->color1 });
						}
					}
					else
					{
						switch (gradationType)
						{
						case LabelGradationType::TopBottom:
						{
							const double lineGradationHeight = Max(lineCache.height, 1e-6);
							const double glyphTop = glyph.getOffset(drawScale).y + glyphStyle.yOffset;
							const double topT = Clamp(glyphTop / lineGradationHeight, 0.0, 1.0);
							const double bottomT = Clamp((glyphTop + glyph.texture.size.y * drawScale) / lineGradationHeight, 0.0, 1.0);
							const double minMaxTAbsDiff = Max(lineCache.maxBottomT - lineCache.minTopT, 1e-6);
							const double scaledTopT = (topT - lineCache.minTopT) / minMaxTAbsDiff;
							const double scaledBottomT = (bottomT - lineCache.minTopT) / minMaxTAbsDiff;
							const ColorF topColor = gradationColor1.lerp(gradationColor2, scaledTopT);
							const ColorF bottomColor = gradationColor1.lerp(gradationColor2, scaledBottomT);
							scaledTexture.draw(drawPos, Arg::top = topColor, Arg::bottom = bottomColor);
							break;
						}

						case LabelGradationType::LeftRight:
						{
							const double glyphLeft = drawPos.x;
							const double glyphWidth = static_cast<double>(glyph.texture.size.x) * drawScale * autoShrinkWidthScale;
							const double glyphRight = glyphLeft + glyphWidth;
							const double leftT = Clamp((glyphLeft - gradientLeft) / horizontalGradationWidth, 0.0, 1.0);
							const double rightT = Clamp((glyphRight - gradientLeft) / horizontalGradationWidth, 0.0, 1.0);
							const ColorF leftColor = gradationColor1.lerp(gradationColor2, leftT);
							const ColorF rightColor = gradationColor1.lerp(gradationColor2, rightT);
							scaledTexture.draw(drawPos, Arg::left = leftColor, Arg::right = rightColor);
							break;
						}

						case LabelGradationType::None:
						default:
							scaledTexture.draw(drawPos, color);
							break;
						}
					}

					double spacingScale;
					switch (m_sizingMode.value())
					{
					case LabelSizingMode::AutoShrink:
						spacingScale = m_cache.effectiveFontSize / m_fontSize.value();
						break;
					case LabelSizingMode::AutoShrinkWidth:
					case LabelSizingMode::AutoShrinkWidthResizeHeight:
						// 描画時はAutoShrinkWidthもスケール適用
						spacingScale = autoShrinkWidthScale;
						break;
					case LabelSizingMode::Fixed:
					case LabelSizingMode::AutoResize:
					default:
						spacingScale = 1.0;
						break;
					}
					x += (glyph.xAdvance * drawScale * autoShrinkWidthScale + characterSpacing.x * spacingScale);
				}
			}
		}

		if (m_underlineStyle.value() == LabelUnderlineStyle::Solid)
		{
			for (const auto& lineCache : m_cache.lineCaches)
			{
				const double effectiveLineWidth = lineCache.width * autoShrinkWidthScale;
				const double startX = [&rect, effectiveLineWidth, horizontalAlign]()
					{
						switch (horizontalAlign)
						{
						case HorizontalAlign::Left:
							return rect.x;
						case HorizontalAlign::Center:
							return rect.x + (rect.w - effectiveLineWidth) / 2;
						case HorizontalAlign::Right:
							return rect.x + rect.w - effectiveLineWidth;
						default:
							throw Error{ U"Invalid HorizontalAlign: {}"_fmt(static_cast<std::underlying_type_t<HorizontalAlign>>(horizontalAlign)) };
						}
					}();

				const double thickness = m_underlineThickness.value();
				const double y = startY + (lineCache.offsetY + lineCache.height);
				Line{ startX, y, startX + effectiveLineWidth, y }.draw(thickness, m_underlineColor.value());
			}
		}
	}

	SizeF Label::getContentSize(const String& canvasDefaultFontAssetName) const
	{
		// rectSize指定なしでのサイズ計算は縮小されないようAutoShrink/AutoShrinkWidth/AutoResizeHeightはFixedとして扱う
		auto sizingMode = m_sizingMode.value();
		if (sizingMode == LabelSizingMode::AutoShrink || sizingMode == LabelSizingMode::AutoShrinkWidth || sizingMode == LabelSizingMode::AutoResizeHeight || sizingMode == LabelSizingMode::AutoShrinkWidthResizeHeight)
		{
			sizingMode = LabelSizingMode::Fixed;
		}

		m_cache.refreshIfDirty(
			m_text.value(),
			m_richTextEnabled.value(),
			m_fontOpt,
			m_fontAssetName.value(),
			canvasDefaultFontAssetName,
			m_fontSize.value(),
			m_minFontSize.value(),
			m_characterSpacing.value(),
			HorizontalOverflow::Overflow, // rectSize指定なしでのサイズ計算は折り返さないようOverflowで固定
			VerticalOverflow::Overflow, // rectSize指定なしでのサイズ計算はクリップされないようOverflowで固定
			m_cache.prevParams.has_value() ? m_cache.prevParams->rectSize : Vec2::Zero(), // rectSizeは使われないので、キャッシュ再更新がなるべく走らないよう前回と同じ値を渡す
			sizingMode);

		return m_cache.regionSize;
	}

	SizeF Label::getContentSize(const SizeF& rectSize, const String& canvasDefaultFontAssetName) const
	{
		m_cache.refreshIfDirty(
			m_text.value(),
			m_richTextEnabled.value(),
			m_fontOpt,
			m_fontAssetName.value(),
			canvasDefaultFontAssetName,
			m_fontSize.value(),
			m_minFontSize.value(),
			m_characterSpacing.value(),
			m_horizontalOverflow.value(),
			m_verticalOverflow.value(),
			rectSize,
			m_sizingMode.value());

		return m_cache.regionSize;
	}

	void Label::refreshAutoResizeImmediately(const std::shared_ptr<Node>& node)
	{
		if (m_sizingMode.value() == LabelSizingMode::AutoResize)
		{
			const String& canvasDefaultFontAssetName = [&node]() -> const String&
				{
					if (const auto canvas = node->containedCanvas())
					{
						return canvas->defaultFontAssetName();
					}
					return EmptyString;
				}();
			const SizeF size = getContentSizeForAutoResize(canvasDefaultFontAssetName);
			if (node->regionRect().size != size)
			{
				if (const AnchorRegion* pAnchorRegion = node->anchorRegion())
				{
					AnchorRegion newRegion = *pAnchorRegion;
					newRegion.sizeDelta = size;
					newRegion.anchorMax = newRegion.anchorMin;
					node->setRegion(newRegion);
				}
				else if (const InlineRegion* pInlineRegion = node->inlineRegion())
				{
					InlineRegion newRegion = *pInlineRegion;
					newRegion.sizeDelta = size;
					newRegion.sizeRatio = Vec2::Zero();
					newRegion.flexibleWeight = 0.0;
					node->setRegion(newRegion);
				}
			}
		}
		else if (m_sizingMode.value() == LabelSizingMode::AutoResizeHeight || m_sizingMode.value() == LabelSizingMode::AutoShrinkWidthResizeHeight)
		{
			// AutoResizeHeightはAnchorやsizeRatioによる幅をあらかじめ確定させておく必要があるため、事前にレイアウト更新
			node->refreshContainedCanvasLayoutImmediately();

			const String& canvasDefaultFontAssetName = [&node]() -> const String&
				{
					if (const auto canvas = node->containedCanvas())
					{
						return canvas->defaultFontAssetName();
					}
					return EmptyString;
				}();

			const LRTB& padding = m_padding.value();
			const RectF paddedRect = node->regionRect().stretched(
				-padding.top,
				-padding.right,
				-padding.bottom,
				-padding.left
			);

			// 現在のノード幅を使ってコンテンツサイズを計算
			const SizeF contentSize = getContentSize(paddedRect.size, canvasDefaultFontAssetName);

			// 高さのみを切り上げてpaddingを加算
			const double newHeight = Math::Ceil(contentSize.y) + padding.totalHeight();

			if (node->regionRect().h != newHeight)
			{
				if (const AnchorRegion* pAnchorRegion = node->anchorRegion())
				{
					AnchorRegion newRegion = *pAnchorRegion;
					// 高さのみを更新(幅のsizeDeltaは維持)
					newRegion.sizeDelta.y = newHeight;
					newRegion.anchorMax.y = newRegion.anchorMin.y;
					node->setRegion(newRegion);
				}
				else if (const InlineRegion* pInlineRegion = node->inlineRegion())
				{
					InlineRegion newRegion = *pInlineRegion;
					newRegion.sizeDelta.y = newHeight;
					newRegion.sizeRatio.y = 0.0;
					// 幅に関する設定は維持
					node->setRegion(newRegion);
				}
			}
		}
	}
}
