#pragma once
#include <Siv3D.hpp>
#include <NocoUI/PropertyValue.hpp>

namespace noco::editor
{
	namespace EditorColor
	{
		inline constexpr ColorF ControlBackgroundColor{ 0.1, 0.8 };

		[[nodiscard]]
		inline PropertyValue<Color> ControlBackgroundColorValue()
		{
			return PropertyValue<Color>{ ControlBackgroundColor }
				.withDisabled(ColorF{ 0.2, 0.8 })
				.withSmoothTime(0.05);
		}

		[[nodiscard]]
		inline PropertyValue<Color> ButtonBorderColorValue()
		{
			return PropertyValue<Color>{ ColorF{ 1.0, 0.4 } }
				.withHovered(ColorF{ 1.0, 0.6 })
				.withDisabled(ColorF{ 1.0, 0.2 })
				.withSmoothTime(0.05);
		}

		[[nodiscard]]
		inline PropertyValue<Color> DefaultButtonBorderColorValue()
		{
			return PropertyValue<Color>{ ColorF{ 1.0, 0.6 } }
				.withHovered(ColorF{ 1.0, 0.8 })
				.withSmoothTime(0.05);
		}

		[[nodiscard]]
		inline PropertyValue<Color> TextBoxBorderColorValue()
		{
			return PropertyValue<Color>{ ColorF{ 1.0, 0.4 } }
				.withHovered(Palette::Skyblue)
				.withStyleState(U"focused", Palette::Orange)
				.withSmoothTime(0.05);
		}

		inline constexpr ColorF TextSelectionColor{ Palette::Orange, 0.5 };
	}
}