#pragma once
#include <NocoUI.hpp>
#include "EditorEnums.hpp"

namespace noco::editor
{
	struct Defaults
	{
		ConstraintType constraintType;

		ConstraintVariant defaultConstraint() const
		{
			switch (constraintType)
			{
			case ConstraintType::AnchorConstraint:
				return AnchorConstraint
				{
					.sizeDelta = { 100, 100 },
				};

			case ConstraintType::BoxConstraint:
				return BoxConstraint
				{
					.sizeDelta = { 100, 100 },
				};

			default:
				throw Error{ U"Unknown constraint type: {}"_fmt(static_cast<uint8>(constraintType)) };
			}
		}
	};
}
