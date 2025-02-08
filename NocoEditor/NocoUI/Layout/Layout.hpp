#pragma once
#include "FlowLayout.hpp"
#include "HorizontalLayout.hpp"
#include "VerticalLayout.hpp"

namespace noco
{
	using LayoutVariant = std::variant<FlowLayout, HorizontalLayout, VerticalLayout>;
}
