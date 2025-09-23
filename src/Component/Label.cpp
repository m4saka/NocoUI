#include "NocoUI/Component/Label.hpp"
#include "NocoUI/Node.hpp"

namespace noco
{
	bool Label::Cache::refreshIfDirty(const String& text, const Optional<Font>& fontOpt, const String& fontAssetName, double fontSize, double minFontSize, const Vec2& spacing, HorizontalOverflow horizontalOverflow, VerticalOverflow verticalOverflow, const SizeF& rectSize, LabelSizingMode newSizingMode)
	{
		const bool hasCustomFont = fontOpt.has_value();
		const Font newFont = hasCustomFont ? *fontOpt : ((!fontAssetName.empty() && FontAsset::IsRegistered(fontAssetName)) ? FontAsset(fontAssetName) : SimpleGUI::GetFont());

		if (prevParams.has_value() &&
			!prevParams->isDirty(text, fontAssetName, fontSize, minFontSize, horizontalOverflow, verticalOverflow, spacing, rectSize, hasCustomFont, newFont, newSizingMode))
		{
			return false;
		}

		prevParams = CacheParams
		{
			.text = text,
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
				double minTopT = 1.0;
				double maxBottomT = 0.0;

				const auto fnPushLine =
					[&]() -> bool
					{
						const double currentLineBottom = offset.y + this->lineHeight;

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

						if (minTopT > maxBottomT)
						{
							// 行内に文字がない場合
							minTopT = 0.0;
							maxBottomT = 1.0;
						}

						lineCaches.push_back({
							.glyphs = lineGlyphs,
							.width = offset.x,
							.offsetY = offset.y,
							.minTopT = minTopT,
							.maxBottomT = maxBottomT,
						});
						lineGlyphs.clear();
						maxWidth = Max(maxWidth, offset.x);
						offset.x = 0;
						minTopT = 1.0;
						maxBottomT = 0.0;

						offset.y = currentLineBottom + spacing.y;
						return true;
					};

				const double fontHeight = Max(currentFont.height(targetFontSize), 1.0);
				const Array<Glyph> glyphs = currentFont.getGlyphs(text);
				for (const auto& glyph : glyphs)
				{
					if (glyph.codePoint == U'\n')
					{
						if (!fnPushLine())
						{
							break;
						}
						continue;
					}

					// AutoShrinkの場合はスケールを適用
					// (AutoShrinkWidthの場合、通常スケールで計算したサイズとノード幅を元にスケールを決めるため、ここではスケールを適用しない)
					const double spacingScale = newSizingMode == LabelSizingMode::AutoShrink ? targetFontSize / fontSize : 1.0;

					const double xAdvance = glyph.xAdvance * this->assetFontSizeScale + spacing.x * spacingScale;
					if (hov == HorizontalOverflow::Wrap && offset.x + xAdvance > rectSize.x)
					{
						if (!fnPushLine())
						{
							break;
						}
					}

					offset.x += xAdvance;
					lineGlyphs.push_back(glyph);

					const double glyphTop = glyph.getOffset(this->assetFontSizeScale).y;
					minTopT = Min(minTopT, glyphTop / fontHeight);
					maxBottomT = Max(maxBottomT, (glyphTop + glyph.texture.size.y * this->assetFontSizeScale) / fontHeight);
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
		else if (newSizingMode == LabelSizingMode::AutoResize)
		{
			// AutoResizeではノードサイズの誤差による折り返しやクリップが発生しないよう、両方Overflowとする
			this->effectiveFontSize = fontSize;
			this->effectiveAutoShrinkWidthScale = 1.0;
			this->regionSize = refreshCacheAndGetRegionSize(fontSize, HorizontalOverflow::Overflow, VerticalOverflow::Overflow);
		}
		else // LabelSizingMode::Fixed
		{
			this->effectiveFontSize = fontSize;
			this->effectiveAutoShrinkWidthScale = 1.0;
			this->regionSize = refreshCacheAndGetRegionSize(fontSize, horizontalOverflow, verticalOverflow);
		}

		return true;
	}

	SizeF Label::getContentSizeForAutoResize() const
	{
		// rectSize指定なしでのサイズ計算は縮小されないようAutoShrinkはFixedとして扱う
		auto sizingMode = m_sizingMode.value();
		if (sizingMode == LabelSizingMode::AutoShrink || sizingMode == LabelSizingMode::AutoShrinkWidth)
		{
			sizingMode = LabelSizingMode::Fixed;
		}

		m_autoResizeCache.refreshIfDirty(
			m_text.value(),
			m_fontOpt,
			m_fontAssetName.value(),
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
		if (m_sizingMode.value() == LabelSizingMode::AutoResize)
		{
			const SizeF size = getContentSizeForAutoResize();
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

		m_cache.refreshIfDirty(
			text,
			m_fontOpt,
			m_fontAssetName.value(),
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

		TextStyle textStyle = TextStyle::Default();
		if (isSDF || isMSDF)
		{
			// SDFアウトラインの色にはScopedColorMul2Dの色が自動では乗らないため乗算が必要
			const ColorF colorMul{ Graphics2D::GetColorMul() };
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

		const double autoShrinkWidthScale = m_sizingMode.value() == LabelSizingMode::AutoShrinkWidth
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

				for (const auto& glyph : lineCache.glyphs)
				{
					if (glyph.codePoint == U'\n')
					{
						continue;
					}

					const Vec2 pos{ startX + x, startY + lineCache.offsetY };
					const Vec2 drawPos = pos + glyph.getOffset(m_cache.assetFontSizeScale) * Vec2{ autoShrinkWidthScale, 1.0 };
					const auto scaledTexture = glyph.texture.scaled(m_cache.assetFontSizeScale * autoShrinkWidthScale, m_cache.assetFontSizeScale);

					switch (gradationType)
					{
					case LabelGradationType::TopBottom:
					{
						const double lineGradationHeight = Max(m_cache.lineHeight, 1e-6);
						const double topT = Clamp(glyph.getOffset(m_cache.assetFontSizeScale).y / lineGradationHeight, 0.0, 1.0);
						const double bottomT = Clamp((glyph.getOffset(m_cache.assetFontSizeScale).y + glyph.texture.size.y * m_cache.assetFontSizeScale) / lineGradationHeight, 0.0, 1.0);
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
						const double glyphWidth = static_cast<double>(glyph.texture.size.x) * m_cache.assetFontSizeScale * autoShrinkWidthScale;
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

					double spacingScale;
					switch (m_sizingMode.value())
					{
					case LabelSizingMode::AutoShrink:
						spacingScale = m_cache.effectiveFontSize / m_fontSize.value();
						break;
					case LabelSizingMode::AutoShrinkWidth:
						// 描画時はAutoShrinkWidthもスケール適用
						spacingScale = autoShrinkWidthScale;
						break;
					case LabelSizingMode::Fixed:
					case LabelSizingMode::AutoResize:
					default:
						spacingScale = 1.0;
						break;
					}
					x += (glyph.xAdvance * m_cache.assetFontSizeScale * autoShrinkWidthScale + characterSpacing.x * spacingScale);
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
				const double y = startY + (lineCache.offsetY + m_cache.lineHeight);
				Line{ startX, y, startX + effectiveLineWidth, y }.draw(thickness, m_underlineColor.value());
			}
		}
	}

	SizeF Label::getContentSize() const
	{
		// rectSize指定なしでのサイズ計算は縮小されないようAutoShrinkはFixedとして扱う
		auto sizingMode = m_sizingMode.value();
		if (sizingMode == LabelSizingMode::AutoShrink || sizingMode == LabelSizingMode::AutoShrinkWidth)
		{
			sizingMode = LabelSizingMode::Fixed;
		}

		m_cache.refreshIfDirty(
			m_text.value(),
			m_fontOpt,
			m_fontAssetName.value(),
			m_fontSize.value(),
			m_minFontSize.value(),
			m_characterSpacing.value(),
			HorizontalOverflow::Overflow, // rectSize指定なしでのサイズ計算は折り返さないようOverflowで固定
			VerticalOverflow::Overflow, // rectSize指定なしでのサイズ計算はクリップされないようOverflowで固定
			m_cache.prevParams.has_value() ? m_cache.prevParams->rectSize : Vec2::Zero(), // rectSizeは使われないので、キャッシュ再更新がなるべく走らないよう前回と同じ値を渡す
			sizingMode);

		return m_cache.regionSize;
	}

	SizeF Label::getContentSize(const SizeF& rectSize) const
	{
		m_cache.refreshIfDirty(
			m_text.value(),
			m_fontOpt,
			m_fontAssetName.value(),
			m_fontSize.value(),
			m_minFontSize.value(),
			m_characterSpacing.value(),
			m_horizontalOverflow.value(),
			m_verticalOverflow.value(),
			rectSize,
			m_sizingMode.value());

		return m_cache.regionSize;
	}
}
