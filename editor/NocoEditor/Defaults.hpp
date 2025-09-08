#pragma once
#include <NocoUI.hpp>
#include "EditorEnums.hpp"

namespace noco::editor
{
	struct Defaults
	{
		RegionType regionType;

		RegionVariant defaultRegion() const
		{
			switch (regionType)
			{
			case RegionType::AnchorRegion:
				return AnchorRegion
				{
					.sizeDelta = { 100, 100 },
				};

			case RegionType::InlineRegion:
				return InlineRegion
				{
					.sizeDelta = { 100, 100 },
				};

			default:
				throw Error{ U"Unknown region type: {}"_fmt(static_cast<uint8>(regionType)) };
			}
		}
	};
}
