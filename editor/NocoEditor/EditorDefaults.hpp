#pragma once
#include <Siv3D.hpp>
#include "NocoUI.hpp"
#include "EditorTypes.hpp"

struct Defaults
{
	ConstraintType constraintType;

	noco::ConstraintVariant defaultConstraint()
	{
		switch (constraintType)
		{
		case ConstraintType::AnchorConstraint:
			return noco::AnchorConstraint
			{
				.sizeDelta = { 100, 100 },
			};

		case ConstraintType::BoxConstraint:
			return noco::BoxConstraint
			{
				.sizeDelta = { 100, 100 },
			};

		default:
			throw Error{ U"Unknown constraint type: {}"_fmt(static_cast<uint8>(constraintType)) };
		}
	}
};