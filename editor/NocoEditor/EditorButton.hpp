#pragma once
#include <NocoUI.hpp>
#include "EditorYN.hpp"

namespace noco::editor
{
	[[nodiscard]]
	std::shared_ptr<Node> CreateButtonNode(StringView text, const ConstraintVariant& constraint, std::function<void(const std::shared_ptr<Node>&)> onClick, IsDefaultButtonYN isDefaultButton = IsDefaultButtonYN::No);
}
