#include "NocoUI/Component/Label.hpp"
#include "NocoUI/Node.hpp"

namespace noco
{
	bool Label::Cache::refreshIfDirty(StringView text, const Optional<Font>& fontOpt, StringView fontAssetName, double fontSize, const Vec2& spacing, HorizontalOverflow horizontalOverflow, VerticalOverflow verticalOverflow, const SizeF& rectSize, LabelSizingMode newSizingMode)
	{
		const bool hasCustomFont = fontOpt.has_value();
		const Font newFont = hasCustomFont ? *fontOpt : ((!fontAssetName.empty() && FontAsset::IsRegistered(fontAssetName)) ? FontAsset(fontAssetName) : SimpleGUI::GetFont());
		
		if (prevParams.has_value() && !prevParams->isDirty(text, fontAssetName, fontSize, horizontalOverflow, verticalOverflow, spacing, rectSize, hasCustomFont, newFont, newSizingMode))
		{
			return false;
		}
		prevParams = CacheParams
		{
			.text = String{ text },
			.fontAssetName = String{ fontAssetName },
			.fontSize = fontSize,
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
		const Array<Glyph> glyphs = newFont.getGlyphs(text);
		baseFontSize = newFont.fontSize();
		if (baseFontSize == 0)
		{
			scale = 1.0;
		}
		else
		{
			scale = fontSize / baseFontSize;
		}
		lineHeight = newFont.height(fontSize);

		lineCaches.clear();

		// AutoShrinkWidthモードで幅の縮小率を適用する場合は、呼び出し側で適切にscaleを調整する
		// refreshIfDirty内では常に1.0として扱う
		const double autoShrinkWidthScale = 1.0;

		double maxWidth = 0.0;
		Vec2 offset = Vec2::Zero();
		Array<Glyph> lineGlyphs;
		double minTopT = 1.0;
		double maxBottomT = 0.0;

		const auto fnPushLine =
			[&]() -> bool
			{
				// 現在の行の下端位置を計算
				const double currentLineBottom = offset.y + lineHeight;
				
				// Clipモードで現在の行が矩形を超える場合は追加しない
				if (verticalOverflow == VerticalOverflow::Clip && currentLineBottom > rectSize.y)
				{
					return false;
				}
				
				if (!lineGlyphs.empty())
				{
					// 行末文字の右側に余白を入れないため、その分を引く
					const double spacingScale = sizingMode == LabelSizingMode::AutoShrink ? scale :
						sizingMode == LabelSizingMode::AutoShrinkWidth ? autoShrinkWidthScale : 1.0;
					offset.x -= spacing.x * spacingScale;
				}

				if (minTopT > maxBottomT)
				{
					// 文字が一つもない場合など
					minTopT = 0.0;
					maxBottomT = 1.0;
				}

				// 矩形内に収まる行のみキャッシュに追加
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
				
				// 次の行の開始位置を計算
				offset.y = currentLineBottom + spacing.y;
				return true;
			};

		const double fontHeight = Max(newFont.height(fontSize), 1.0);
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

			// spacingはモードに応じてスケーリング
			const double spacingScale = sizingMode == LabelSizingMode::AutoShrink ? scale :
				sizingMode == LabelSizingMode::AutoShrinkWidth ? autoShrinkWidthScale : 1.0;
			const double xAdvance = glyph.xAdvance * scale * autoShrinkWidthScale + spacing.x * spacingScale;
			if (horizontalOverflow == HorizontalOverflow::Wrap && offset.x + xAdvance > rectSize.x)
			{
				if (!fnPushLine())
				{
					break;
				}
			}

			offset.x += xAdvance;
			lineGlyphs.push_back(glyph);

			const double glyphTop = glyph.getOffset().y;
			minTopT = Min(minTopT, glyphTop / fontHeight);
			maxBottomT = Max(maxBottomT, (glyphTop + glyph.texture.size.y) / fontHeight);
		}

		// 最後の行を追加
		fnPushLine();

		regionSize = { maxWidth, offset.y - spacing.y };
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
			m_characterSpacing.value(),
			HorizontalOverflow::Overflow, // rectSize指定なしでのサイズ計算は折り返さないようOverflowで固定
			VerticalOverflow::Overflow, // rectSize指定なしでのサイズ計算はクリップされないようOverflowで固定
			m_autoResizeCache.prevParams.has_value() ? m_autoResizeCache.prevParams->rectSize : Vec2::Zero(), // rectSizeは使われないので、キャッシュ再更新がなるべく走らないよう前回と同じ値を渡す
			sizingMode);

		// AutoResizeでは小数点以下を切り上げる
		// (誤差により右端で折り返しが発生するのを防ぐため)
		const SizeF regionSize = m_autoResizeCache.regionSize;
		const SizeF ceiledRegionSize = { Math::Ceil(regionSize.x), Math::Ceil(regionSize.y) };

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

		const bool wasUpdated = m_cache.refreshIfDirty(
			text,
			m_fontOpt,
			m_fontAssetName.value(),
			m_fontSize.value(),
			characterSpacing,
			m_horizontalOverflow.value(),
			m_verticalOverflow.value(),
			rect.size,
			m_sizingMode.value());

		if (m_sizingMode.value() == LabelSizingMode::AutoShrink)
		{
			const SizeF availableSize = rect.size;
			
			// フォントサイズはSmoothPropertyなので許容誤差を大きめに取る
			constexpr double Epsilon = 1e-2;
			const bool cacheValid = !wasUpdated &&
				m_cache.sizingMode == LabelSizingMode::AutoShrink &&
				Abs(m_cache.availableSize.x - availableSize.x) < Epsilon &&
				Abs(m_cache.availableSize.y - availableSize.y) < Epsilon &&
				Abs(m_cache.originalFontSize - m_fontSize.value()) < Epsilon &&
				Abs(m_cache.minFontSize - m_minFontSize.value()) < Epsilon;
			
			if (!cacheValid)
			{
				m_cache.sizingMode = LabelSizingMode::AutoShrink;
				m_cache.availableSize = availableSize;
				m_cache.originalFontSize = m_fontSize.value();
				m_cache.minFontSize = m_minFontSize.value();
				
				double effectiveFontSize = m_fontSize.value();
				const double minFontSize = m_minFontSize.value();

				while (effectiveFontSize >= minFontSize)
				{
					// 現在のフォントサイズで収まるかチェック
					m_cache.prevParams.reset();
					m_cache.refreshIfDirty(
						text,
						m_fontOpt,
						m_fontAssetName.value(),
						effectiveFontSize,
						characterSpacing,
						m_horizontalOverflow.value(),
						VerticalOverflow::Overflow, // AutoShrinkのフォントサイズ探索では常にOverflowとする
						availableSize,
						m_sizingMode.value());

					// 収まっていれば、このサイズで確定
					if (m_cache.regionSize.x <= availableSize.x &&
					    m_cache.regionSize.y <= availableSize.y)
					{
						break;
					}

					// 収まらない場合、フォントサイズを1下げる
					effectiveFontSize = effectiveFontSize - 1.0;

					// 最小フォントサイズを下回らないようにする
					if (effectiveFontSize < minFontSize)
					{
						effectiveFontSize = minFontSize;
						// 最小フォントサイズで再計算して終了
						m_cache.prevParams.reset();
						m_cache.refreshIfDirty(
							text,
							m_fontOpt,
							m_fontAssetName.value(),
							effectiveFontSize,
							characterSpacing,
							m_horizontalOverflow.value(),
							VerticalOverflow::Overflow, // AutoShrinkのフォントサイズ探索では常にOverflowとする
							availableSize,
							m_sizingMode.value());
						break;
					}
				}

				m_cache.effectiveFontSize = effectiveFontSize;

				// AutoShrinkの探索ではVerticalOverflow::Overflowで計算したため、
				// 実際のverticalOverflowプロパティ値で再計算する必要がある
				if (m_verticalOverflow.value() == VerticalOverflow::Clip)
				{
					m_cache.prevParams.reset();
					m_cache.refreshIfDirty(
						text,
						m_fontOpt,
						m_fontAssetName.value(),
						effectiveFontSize,
						characterSpacing,
						m_horizontalOverflow.value(),
						m_verticalOverflow.value(),  // 実際のプロパティ値を使用
						availableSize,
						m_sizingMode.value());
				}
			}
			else if (m_cache.effectiveFontSize != m_fontSize.value())
			{
				// キャッシュされた縮小フォントサイズで再適用（キャッシュはリセットしない）
				if (m_cache.baseFontSize != 0)
				{
					m_cache.scale = m_cache.effectiveFontSize / m_cache.baseFontSize;
				}
			}
		}
		else if (m_sizingMode.value() == LabelSizingMode::AutoShrinkWidth)
		{
			const SizeF availableSize = rect.size;

			constexpr double Epsilon = 1e-2;
			const bool cacheValid = !wasUpdated &&
				m_cache.sizingMode == LabelSizingMode::AutoShrinkWidth &&
				Abs(m_cache.availableSize.x - availableSize.x) < Epsilon &&
				Abs(m_cache.availableSize.y - availableSize.y) < Epsilon &&
				Abs(m_cache.originalAutoShrinkWidthScale - 1.0) < Epsilon;

			if (!cacheValid)
			{
				m_cache.sizingMode = LabelSizingMode::AutoShrinkWidth;
				m_cache.availableSize = availableSize;
				m_cache.originalAutoShrinkWidthScale = 1.0;

				// まず、autoShrinkWidthScale = 1.0でテキストサイズを計算
				m_cache.prevParams.reset();
				m_cache.refreshIfDirty(
					text,
					m_fontOpt,
					m_fontAssetName.value(),
					m_fontSize.value(),
					characterSpacing,
					HorizontalOverflow::Overflow, // AutoShrinkWidthでは常にOverflowとする
					VerticalOverflow::Overflow,
					availableSize,
					m_sizingMode.value());

				// ノードの幅に収まるスケールを計算
				double effectiveAutoShrinkWidthScale = 1.0;
				if (m_cache.regionSize.x > availableSize.x)
				{
					effectiveAutoShrinkWidthScale = availableSize.x / m_cache.regionSize.x;
				}

				// 計算された縮小率でキャッシュを更新
				m_cache.effectiveAutoShrinkWidthScale = effectiveAutoShrinkWidthScale;
			}
		}
		else
		{
			// Fixedモードに切り替わった場合、スケールを元に戻す
			if ((m_cache.sizingMode == LabelSizingMode::AutoShrink ||
			     m_cache.sizingMode == LabelSizingMode::AutoShrinkWidth) &&
			    m_cache.baseFontSize != 0)
			{
				m_cache.scale = m_fontSize.value() / m_cache.baseFontSize;
			}
			m_cache.sizingMode = LabelSizingMode::Fixed;
			m_cache.effectiveAutoShrinkWidthScale = 1.0;
		}

		// AutoShrinkモードでは、フォントサイズ探索時にrefreshIfDirtyを呼ぶため、
		// prevParams->fontSizeが縮小後のフォントサイズで保存される。
		// 次フレームで元のフォントサイズと比較されると常に変更ありと判定され、
		// 毎フレーム再計算が発生してしまう。
		// これを防ぐため、prevParams->fontSizeを元のフォントサイズに戻す。
		if (m_cache.prevParams.has_value() && m_sizingMode.value() == LabelSizingMode::AutoShrink)
		{
			m_cache.prevParams->fontSize = m_fontSize.value();
		}

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

		const HorizontalAlign& horizontalAlign = m_horizontalAlign.value();

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
				textStyle = TextStyle::OutlineShadow(outlineFactorInner, outlineFactorOuter, ColorF{ m_outlineColor.value() } * colorMul, m_shadowOffset.value(), ColorF{ m_shadowColor.value() } * colorMul);
			}
			else if (hasOutline)
			{
				textStyle = TextStyle::Outline(outlineFactorInner, outlineFactorOuter, ColorF{ m_outlineColor.value() } * colorMul);
			}
			else if (hasShadow)
			{
				textStyle = TextStyle::Shadow(m_shadowOffset.value(), ColorF{ m_shadowColor.value() } * colorMul);
			}
		}

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
				const double autoShrinkWidthScale = m_sizingMode.value() == LabelSizingMode::AutoShrinkWidth
					? m_cache.effectiveAutoShrinkWidthScale
					: 1.0;

				// AutoShrinkWidthモードの場合、lineCache.widthに縮小率を適用
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
					const Vec2 scaleVec = m_sizingMode.value() == LabelSizingMode::AutoShrinkWidth
						? Vec2{ m_cache.scale * autoShrinkWidthScale, m_cache.scale }
						: Vec2{ m_cache.scale, m_cache.scale };
					const Vec2 drawPos = pos + glyph.getOffset(scaleVec.y) * Vec2{ autoShrinkWidthScale, 1.0 };
					const auto scaledTexture = glyph.texture.scaled(scaleVec);

					switch (gradationType)
					{
					case LabelGradationType::TopBottom:
					{
						const double lineGradationHeight = Max(m_cache.lineHeight, 1e-6);
						const double topT = Clamp(glyph.getOffset().y / lineGradationHeight, 0.0, 1.0);
						const double bottomT = Clamp((glyph.getOffset().y + glyph.texture.size.y) / lineGradationHeight, 0.0, 1.0);
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
						const double glyphWidth = static_cast<double>(glyph.texture.size.x) * m_cache.scale * autoShrinkWidthScale;
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

					// AutoShrinkモードではspacingにもフォントサイズ比を適用
					const double spacingScale = m_sizingMode.value() == LabelSizingMode::AutoShrink ? m_cache.scale :
						m_sizingMode.value() == LabelSizingMode::AutoShrinkWidth ? autoShrinkWidthScale : 1.0;
					x += (glyph.xAdvance * m_cache.scale * autoShrinkWidthScale + characterSpacing.x * spacingScale);
				}
			}
		}

		if (m_underlineStyle.value() == LabelUnderlineStyle::Solid)
		{
			for (const auto& lineCache : m_cache.lineCaches)
			{
				const double startX = [&rect, &lineCache, horizontalAlign]()
					{
						switch (horizontalAlign)
						{
						case HorizontalAlign::Left:
							return rect.x;
						case HorizontalAlign::Center:
							return rect.x + (rect.w - lineCache.width) / 2;
						case HorizontalAlign::Right:
							return rect.x + rect.w - lineCache.width;
						default:
							throw Error{ U"Invalid HorizontalAlign: {}"_fmt(static_cast<std::underlying_type_t<HorizontalAlign>>(horizontalAlign)) };
						}
					}();

				const double thickness = m_underlineThickness.value();
				const double y = startY + (lineCache.offsetY + m_cache.lineHeight);
				Line{ startX, y, startX + lineCache.width, y }.draw(thickness, m_underlineColor.value());
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
			m_characterSpacing.value(),
			m_horizontalOverflow.value(),
			m_verticalOverflow.value(),
			rectSize,
			m_sizingMode.value());

		return m_cache.regionSize;
	}
}
