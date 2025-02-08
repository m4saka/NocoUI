#pragma once
#include "BoxConstraint.hpp"
#include "AnchorConstraint.hpp"

namespace noco
{
	using ConstraintVariant = std::variant<BoxConstraint, AnchorConstraint>;
}
