#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"
#include "ITextBox.hpp"
#include "../Enums.hpp"

namespace noco
{
	class TextBox : public SerializableComponentBase, public ITextBox, public std::enable_shared_from_this<TextBox>
	{
	private:
		static constexpr double CursorWidth = 1.5;

		PropertyNonInteractive<String> m_text;
		Property<String> m_fontAssetName;
		SmoothProperty<double> m_fontSize;
		SmoothProperty<ColorF> m_color;
		SmoothProperty<Vec2> m_horizontalPadding;
		SmoothProperty<Vec2> m_verticalPadding;
		Property<HorizontalAlign> m_horizontalAlign;
		Property<VerticalAlign> m_verticalAlign;
		SmoothProperty<ColorF> m_cursorColor;
		SmoothProperty<ColorF> m_selectionColor;
		Property<bool> m_readOnly;

		/* NonSerialized */ double m_cursorBlinkTime = 0.0;
		/* NonSerialized */ bool m_isEditing = false;
		/* NonSerialized */ bool m_isDragging = false;
		/* NonSerialized */ size_t m_selectionAnchor = 0;
		/* NonSerialized */ String m_prevText;
		/* NonSerialized */ size_t m_cursorIndex = 0;
		/* NonSerialized */ Stopwatch m_leftPressStopwatch;
		/* NonSerialized */ Stopwatch m_rightPressStopwatch;
		/* NonSerialized */ Stopwatch m_backspacePressStopwatch;
		/* NonSerialized */ Stopwatch m_deletePressStopwatch;
		/* NonSerialized */ Stopwatch m_dragScrollStopwatch;
		/* NonSerialized */ size_t m_scrollOffset = 0;
		/* NonSerialized */ bool m_prevEditingTextExists = false;
		/* NonSerialized */ bool m_isChanged = false;

		enum class FitDirection
		{
			Left,
			Right,
		};
		/* NonSerialized */ FitDirection m_fitDirection = FitDirection::Left;

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

		struct Cache
		{
			Array<Glyph> glyphs;
			double scale = 1.0;
			double lineHeight = 0.0;
			SizeF regionSize = SizeF::Zero();
			Optional<CacheParams> prevParams = std::nullopt;
			FontMethod fontMethod = FontMethod::Bitmap;

			void refreshIfDirty(StringView text, StringView fontAssetName, double fontSize, const SizeF& rectSize);

			[[nodiscard]]
			double getCursorPosX(double drawOffsetX, size_t scrollOffset, size_t cursorIndex) const;

			[[nodiscard]]
			size_t getCursorIndex(double drawOffsetX, size_t scrollOffset, double cursorPosX) const;
		};

		/* NonSerialized */ mutable Cache m_cache;
		/* NonSerialized */ mutable Cache m_editingCache;

		double getDrawOffsetX() const;

		Vec2 getAlignOffset(const RectF& rect) const;

		size_t moveCursorToMousePos(const RectF& rect, const std::shared_ptr<Node>& node);

		bool hasSelection() const;

		std::pair<size_t, size_t> getSelectionRange() const;

		String getSelectedText() const;

		void deleteSelection();

		void insertTextAtCursor(StringView text);

		void handleClipboardShortcut();


		// IFocusableインタフェースの実装
		void focus(const std::shared_ptr<Node>& node) override;
		void blur(const std::shared_ptr<Node>& node) override;

	public:
		explicit TextBox(
			const PropertyValue<String>& fontAssetName = U"",
			const PropertyValue<double>& fontSize = 24.0,
			const PropertyValue<ColorF>& color = Palette::Black,
			const PropertyValue<Vec2>& horizontalPadding = Vec2{ 8.0, 8.0 },
			const PropertyValue<Vec2>& verticalPadding = Vec2{ 4.0, 4.0 },
			const PropertyValue<HorizontalAlign>& horizontalAlign = HorizontalAlign::Left,
			const PropertyValue<VerticalAlign>& verticalAlign = VerticalAlign::Middle,
			const Optional<PropertyValue<ColorF>>& cursorColor = unspecified,
			const Optional<PropertyValue<ColorF>>& selectionColor = unspecified,
			const PropertyValue<bool>& readOnly = false)
			: SerializableComponentBase{ U"TextBox", { &m_text, &m_fontAssetName, &m_fontSize, &m_color, &m_horizontalPadding, &m_verticalPadding, &m_cursorColor, &m_selectionColor, &m_horizontalAlign, &m_verticalAlign, &m_readOnly } }
			, m_text{ U"text", U"" }
			, m_fontAssetName{ U"fontAssetName", fontAssetName }
			, m_fontSize{ U"fontSize", fontSize }
			, m_color{ U"color", color }
			, m_horizontalPadding{ U"horizontalPadding", horizontalPadding }
			, m_verticalPadding{ U"verticalPadding", verticalPadding }
			, m_horizontalAlign{ U"horizontalAlign", horizontalAlign }
			, m_verticalAlign{ U"verticalAlign", verticalAlign }
			, m_cursorColor{ U"cursorColor", cursorColor.value_or(color) }
			, m_selectionColor{ U"selectionColor", selectionColor.value_or(ColorF{ 0.0, 0.1, 0.3, 0.5 }) }
			, m_readOnly{ U"readOnly", readOnly }
		{
		}

		void updateKeyInput(const std::shared_ptr<Node>& node) override;

		void updateKeyInputInactive(const std::shared_ptr<Node>& node) override;

		void updateScrollOffset(const RectF& rect);

		void draw(const Node& node) const override;

		void onActivated(const std::shared_ptr<Node>& node) override;

		void onDeactivated(const std::shared_ptr<Node>& node) override;

		[[nodiscard]]
		StringView text() const override
		{
			return m_text.value();
		}

		std::shared_ptr<TextBox> setText(StringView text, IgnoreIsChangedYN ignoreIsChanged = IgnoreIsChangedYN::No);

		[[nodiscard]]
		const PropertyValue<String>& fontAssetName() const
		{
			return m_fontAssetName.propertyValue();
		}

		std::shared_ptr<TextBox> setFontAssetName(const PropertyValue<String>& fontAssetName)
		{
			m_fontAssetName.setPropertyValue(fontAssetName);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<double>& fontSize() const
		{
			return m_fontSize.propertyValue();
		}

		std::shared_ptr<TextBox> setFontSize(const PropertyValue<double>& fontSize)
		{
			m_fontSize.setPropertyValue(fontSize);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<ColorF>& color() const
		{
			return m_color.propertyValue();
		}

		std::shared_ptr<TextBox> setColor(const PropertyValue<ColorF>& color)
		{
			m_color.setPropertyValue(color);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<Vec2>& horizontalPadding() const
		{
			return m_horizontalPadding.propertyValue();
		}

		std::shared_ptr<TextBox> setHorizontalPadding(const PropertyValue<Vec2>& horizontalPadding)
		{
			m_horizontalPadding.setPropertyValue(horizontalPadding);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<Vec2>& verticalPadding() const
		{
			return m_verticalPadding.propertyValue();
		}

		std::shared_ptr<TextBox> setVerticalPadding(const PropertyValue<Vec2>& verticalPadding)
		{
			m_verticalPadding.setPropertyValue(verticalPadding);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<ColorF>& cursorColor() const
		{
			return m_cursorColor.propertyValue();
		}

		std::shared_ptr<TextBox> setCursorColor(const PropertyValue<ColorF>& cursorColor)
		{
			m_cursorColor.setPropertyValue(cursorColor);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<ColorF>& selectionColor() const
		{
			return m_selectionColor.propertyValue();
		}

		std::shared_ptr<TextBox> setSelectionColor(const PropertyValue<ColorF>& selectionColor)
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
		const PropertyValue<HorizontalAlign>& horizontalAlign() const
		{
			return m_horizontalAlign.propertyValue();
		}

		std::shared_ptr<TextBox> setHorizontalAlign(const PropertyValue<HorizontalAlign>& horizontalAlign)
		{
			m_horizontalAlign.setPropertyValue(horizontalAlign);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<VerticalAlign>& verticalAlign() const
		{
			return m_verticalAlign.propertyValue();
		}

		std::shared_ptr<TextBox> setVerticalAlign(const PropertyValue<VerticalAlign>& verticalAlign)
		{
			m_verticalAlign.setPropertyValue(verticalAlign);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<bool>& readOnly() const
		{
			return m_readOnly.propertyValue();
		}

		std::shared_ptr<TextBox> setReadOnly(const PropertyValue<bool>& readOnly)
		{
			m_readOnly.setPropertyValue(readOnly);
			return shared_from_this();
		}
	};
}
