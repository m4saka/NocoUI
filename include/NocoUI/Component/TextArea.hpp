﻿#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"
#include "ITextBox.hpp"

namespace noco
{
	class TextArea : public SerializableComponentBase, public ITextBox, public std::enable_shared_from_this<TextArea>
	{
	private:
		static constexpr double CursorWidth = 1.5;

		PropertyNonInteractive<String> m_text;
		Property<String> m_fontAssetName;
		SmoothProperty<double> m_fontSize;
		SmoothProperty<ColorF> m_color;
		SmoothProperty<Vec2> m_horizontalPadding;
		SmoothProperty<Vec2> m_verticalPadding;
		SmoothProperty<ColorF> m_cursorColor;
		SmoothProperty<ColorF> m_selectionColor;
		Property<bool> m_readOnly;

		/* NonSerialized */ double m_cursorBlinkTime = 0.0;
		/* NonSerialized */ bool m_isEditing = false;
		/* NonSerialized */ bool m_isDragging = false;
		/* NonSerialized */ size_t m_selectionAnchorLine = 0;
		/* NonSerialized */ size_t m_selectionAnchorColumn = 0;
		/* NonSerialized */ String m_prevText;
		/* NonSerialized */ size_t m_cursorLine = 0;
		/* NonSerialized */ size_t m_cursorColumn = 0;
		/* NonSerialized */ Stopwatch m_leftPressStopwatch;
		/* NonSerialized */ Stopwatch m_rightPressStopwatch;
		/* NonSerialized */ Stopwatch m_upPressStopwatch;
		/* NonSerialized */ Stopwatch m_downPressStopwatch;
		/* NonSerialized */ Stopwatch m_pageUpPressStopwatch;
		/* NonSerialized */ Stopwatch m_pageDownPressStopwatch;
		/* NonSerialized */ Stopwatch m_backspacePressStopwatch;
		/* NonSerialized */ Stopwatch m_deletePressStopwatch;
		/* NonSerialized */ Stopwatch m_enterPressStopwatch;
		/* NonSerialized */ Stopwatch m_dragScrollStopwatch;
		/* NonSerialized */ size_t m_scrollOffsetX = 0;
		/* NonSerialized */ size_t m_scrollOffsetY = 0;
		/* NonSerialized */ bool m_prevActiveInHierarchy = false;
		/* NonSerialized */ bool m_isChanged = false;
		/* NonSerialized */ bool m_prevEditingTextExists = false;

		struct CacheParams
		{
			String text;
			String fontAssetName;
			double fontSize;
			SizeF rectSize;

			[[nodiscard]]
			bool isDirty(
				StringView newText,
				StringView newFontAssetName,
				double newFontSize,
				const SizeF& newRectSize) const
			{
				return text != newText
					|| fontAssetName != newFontAssetName
					|| fontSize != newFontSize
					|| rectSize != newRectSize;
			}
		};

		struct LineCache
		{
			Array<Glyph> glyphs;
			size_t textBeginIndex;
			size_t textEndIndex;
			double width = 0.0;
		};

		struct Cache
		{
			Array<LineCache> lines;
			double scale = 1.0;
			double lineHeight = 0.0;
			SizeF regionSize = SizeF::Zero();
			Optional<CacheParams> prevParams = std::nullopt;
			FontMethod fontMethod = FontMethod::Bitmap;

			void refreshIfDirty(StringView text, StringView fontAssetName, double fontSize, const SizeF& rectSize);

			[[nodiscard]]
			Vec2 getCursorPos(size_t line, size_t column, size_t scrollOffsetX, size_t scrollOffsetY) const;

			[[nodiscard]]
			std::pair<size_t, size_t> getCursorIndex(const Vec2& pos, size_t scrollOffsetX, size_t scrollOffsetY) const;

			[[nodiscard]]
			size_t getLineColumnToIndex(size_t line, size_t column) const;

			[[nodiscard]]
			std::pair<size_t, size_t> getIndexToLineColumn(size_t index) const;
		};

		/* NonSerialized */ mutable Cache m_cache;
		/* NonSerialized */ mutable Cache m_editingCache;

		std::pair<size_t, size_t> moveCursorToMousePos(const RectF& rect, const Vec2& effectScale);

		bool hasSelection() const;

		std::pair<size_t, size_t> getSelectionRange() const;

		String getSelectedText() const;

		void deleteSelection();

		std::pair<size_t, size_t> insertTextAtCursor(StringView text);

		std::tuple<bool, size_t, size_t> handleShortcut();

		void onDeactivated(const std::shared_ptr<Node>& node);

		void updateScrollOffset(const RectF& rect, const Vec2& effectScale);

		size_t getLineCount() const;

		size_t getColumnCount(size_t line) const;

		// IFocusableインタフェースの実装
		void focus(const std::shared_ptr<Node>& node) override;
		void blur(const std::shared_ptr<Node>& node) override;

	public:
		explicit TextArea(
			const PropertyValue<String>& fontAssetName = U"",
			const PropertyValue<double>& fontSize = 24.0,
			const PropertyValue<ColorF>& color = Palette::Black,
			const PropertyValue<Vec2>& horizontalPadding = Vec2{ 8.0, 8.0 },
			const PropertyValue<Vec2>& verticalPadding = Vec2{ 4.0, 4.0 },
			const Optional<PropertyValue<ColorF>>& cursorColor = unspecified,
			const Optional<PropertyValue<ColorF>>& selectionColor = unspecified,
			const PropertyValue<bool>& readOnly = false)
			: SerializableComponentBase{ U"TextArea", { &m_text, &m_fontAssetName, &m_fontSize, &m_color, &m_horizontalPadding, &m_verticalPadding, &m_cursorColor, &m_selectionColor, &m_readOnly } }
			, m_text{ U"text", U"" }
			, m_fontAssetName{ U"fontAssetName", fontAssetName }
			, m_fontSize{ U"fontSize", fontSize }
			, m_color{ U"color", color }
			, m_horizontalPadding{ U"horizontalPadding", horizontalPadding }
			, m_verticalPadding{ U"verticalPadding", verticalPadding }
			, m_cursorColor{ U"cursorColor", cursorColor.value_or(color) }
			, m_selectionColor{ U"selectionColor", selectionColor.value_or(ColorF{ 0.0, 0.1, 0.3, 0.5 }) }
			, m_readOnly{ U"readOnly", readOnly }
		{
		}

		void updateInput(const std::shared_ptr<Node>& node) override;

		void updateInputInactive(const std::shared_ptr<Node>& node) override;

		void draw(const Node& node) const override;

		[[nodiscard]]
		StringView text() const override
		{
			return m_text.value();
		}

		std::shared_ptr<TextArea> setText(StringView text, IgnoreIsChangedYN ignoreIsChanged = IgnoreIsChangedYN::No);

		[[nodiscard]]
		const PropertyValue<String>& fontAssetName() const
		{
			return m_fontAssetName.propertyValue();
		}

		std::shared_ptr<TextArea> setFontAssetName(const PropertyValue<String>& fontAssetName)
		{
			m_fontAssetName.setPropertyValue(fontAssetName);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<double>& fontSize() const
		{
			return m_fontSize.propertyValue();
		}

		std::shared_ptr<TextArea> setFontSize(const PropertyValue<double>& fontSize)
		{
			m_fontSize.setPropertyValue(fontSize);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<ColorF>& color() const
		{
			return m_color.propertyValue();
		}

		std::shared_ptr<TextArea> setColor(const PropertyValue<ColorF>& color)
		{
			m_color.setPropertyValue(color);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<Vec2>& horizontalPadding() const
		{
			return m_horizontalPadding.propertyValue();
		}

		std::shared_ptr<TextArea> setHorizontalPadding(const PropertyValue<Vec2>& horizontalPadding)
		{
			m_horizontalPadding.setPropertyValue(horizontalPadding);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<Vec2>& verticalPadding() const
		{
			return m_verticalPadding.propertyValue();
		}

		std::shared_ptr<TextArea> setVerticalPadding(const PropertyValue<Vec2>& verticalPadding)
		{
			m_verticalPadding.setPropertyValue(verticalPadding);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<ColorF>& cursorColor() const
		{
			return m_cursorColor.propertyValue();
		}

		std::shared_ptr<TextArea> setCursorColor(const PropertyValue<ColorF>& cursorColor)
		{
			m_cursorColor.setPropertyValue(cursorColor);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<ColorF>& selectionColor() const
		{
			return m_selectionColor.propertyValue();
		}

		std::shared_ptr<TextArea> setSelectionColor(const PropertyValue<ColorF>& selectionColor)
		{
			m_selectionColor.setPropertyValue(selectionColor);
			return shared_from_this();
		}

		[[nodiscard]]
		bool isChanged() const override
		{
			return m_isChanged;
		}

		[[nodiscard]]
		bool isEditing() const override
		{
			return m_isEditing;
		}

		[[nodiscard]]
		const PropertyValue<bool>& readOnly() const
		{
			return m_readOnly.propertyValue();
		}

		std::shared_ptr<TextArea> setReadOnly(const PropertyValue<bool>& readOnly)
		{
			m_readOnly.setPropertyValue(readOnly);
			return shared_from_this();
		}
	};
}