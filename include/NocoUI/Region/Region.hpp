#pragma once
#include "InlineRegion.hpp"
#include "AnchorRegion.hpp"

namespace noco
{
	using RegionVariant = std::variant<InlineRegion, AnchorRegion>;
}
