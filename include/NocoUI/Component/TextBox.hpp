#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"

namespace noco
{
	class TextBox : public SerializableComponentBase, public std::enable_shared_from_this<TextBox>
	{
	private:
		static constexpr double CursorWidth = 1.5;

		Property<String> m_fontAssetName;
		SmoothProperty<double> m_fontSize;
		SmoothProperty<ColorF> m_color;
		SmoothProperty<Vec2> m_horizontalPadding;
		SmoothProperty<Vec2> m_verticalPadding;
		SmoothProperty<ColorF> m_cursorColor;
		SmoothProperty<ColorF> m_selectionColor;

		/* NonSerialized */ double m_cursorBlinkTime = 0.0;
		/* NonSerialized */ bool m_isEditing = false;
		/* NonSerialized */ bool m_isDragging = false;
		/* NonSerialized */ size_t m_selectionAnchor = 0;
		/* NonSerialized */ String m_text;
		/* NonSerialized */ String m_prevText;
		/* NonSerialized */ size_t m_cursorIndex = 0;
		/* NonSerialized */ Stopwatch m_leftPressStopwatch;
		/* NonSerialized */ Stopwatch m_rightPressStopwatch;
		/* NonSerialized */ Stopwatch m_backspacePressStopwatch;
		/* NonSerialized */ Stopwatch m_deletePressStopwatch;
		/* NonSerialized */ Stopwatch m_dragScrollStopwatch;
		/* NonSerialized */ size_t m_scrollOffset = 0;
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

		size_t moveCursorToMousePos(const RectF& rect, const Vec2& effectScale);

	public:
		explicit TextBox(
			const PropertyValue<String>& fontAssetName = U"Font14",
			const PropertyValue<double>& fontSize = 14.0,
			const PropertyValue<ColorF>& color = Palette::Black,
			const PropertyValue<Vec2>& horizontalPadding = Vec2{ 8.0, 8.0 },
			const PropertyValue<Vec2>& verticalPadding = Vec2{ 4.0, 4.0 },
			const Optional<PropertyValue<ColorF>>& cursorColor = unspecified,
			const Optional<PropertyValue<ColorF>>& selectionColor = unspecified)
			: SerializableComponentBase{ U"TextBox", { &m_fontAssetName, &m_fontSize, &m_color, &m_horizontalPadding, &m_verticalPadding, &m_cursorColor, &m_selectionColor } }
			, m_fontAssetName{ U"fontAssetName", fontAssetName }
			, m_fontSize{ U"fontSize", fontSize }
			, m_color{ U"color", color }
			, m_horizontalPadding{ U"horizontalPadding", horizontalPadding }
			, m_verticalPadding{ U"verticalPadding", verticalPadding }
			, m_cursorColor{ U"cursorColor", cursorColor.value_or(color) }
			, m_selectionColor{ U"selectionColor", selectionColor.value_or(ColorF{ 0.0, 0.1, 0.3, 0.5 }) }
		{
		}

		void onDeactivated(CanvasUpdateContext* pContext, const std::shared_ptr<Node>& node) override;

		void update(CanvasUpdateContext* pContext, const std::shared_ptr<Node>& node) override;

		void updateScrollOffset(const RectF& rect, const Vec2& effectScale);

		void draw(const Node& node) const override;

		void deselect(const std::shared_ptr<Node>& node);

		[[nodiscard]]
		StringView text() const
		{
			return m_text;
		}

		void setText(StringView text, IgnoreIsChangedYN ignoreIsChanged = IgnoreIsChangedYN::No);

		[[nodiscard]]
		const PropertyValue<String>& fontAssetName() const
		{
			return m_fontAssetName.propertyValue();
		}

		void setFontAssetName(const PropertyValue<String>& fontAssetName)
		{
			m_fontAssetName.setPropertyValue(fontAssetName);
		}

		[[nodiscard]]
		const PropertyValue<double>& fontSize() const
		{
			return m_fontSize.propertyValue();
		}

		void setFontSize(const PropertyValue<double>& fontSize)
		{
			m_fontSize.setPropertyValue(fontSize);
		}

		[[nodiscard]]
		const PropertyValue<ColorF>& color() const
		{
			return m_color.propertyValue();
		}

		void setColor(const PropertyValue<ColorF>& color)
		{
			m_color.setPropertyValue(color);
		}

		[[nodiscard]]
		const PropertyValue<Vec2>& horizontalPadding() const
		{
			return m_horizontalPadding.propertyValue();
		}

		void setHorizontalPadding(const PropertyValue<Vec2>& horizontalPadding)
		{
			m_horizontalPadding.setPropertyValue(horizontalPadding);
		}

		[[nodiscard]]
		const PropertyValue<Vec2>& verticalPadding() const
		{
			return m_verticalPadding.propertyValue();
		}

		void setVerticalPadding(const PropertyValue<Vec2>& verticalPadding)
		{
			m_verticalPadding.setPropertyValue(verticalPadding);
		}

		[[nodiscard]]
		const PropertyValue<ColorF>& cursorColor() const
		{
			return m_cursorColor.propertyValue();
		}

		void setCursorColor(const PropertyValue<ColorF>& cursorColor)
		{
			m_cursorColor.setPropertyValue(cursorColor);
		}

		[[nodiscard]]
		const PropertyValue<ColorF>& selectionColor() const
		{
			return m_selectionColor.propertyValue();
		}

		void setSelectionColor(const PropertyValue<ColorF>& selectionColor)
		{
			m_selectionColor.setPropertyValue(selectionColor);
		}

		[[nodiscard]]
		bool isChanged() const
		{
			return m_isChanged;
		}
	};
}
