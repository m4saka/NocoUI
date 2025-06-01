#pragma once
#include <Siv3D.hpp>
#include "NocoUI.hpp"
#include "EditorTypes.hpp"

// ボタンノードを作成するユーティリティ関数
std::shared_ptr<noco::Node> CreateButtonNode(StringView text, const noco::ConstraintVariant& constraint, std::function<void(const std::shared_ptr<noco::Node>&)> onClick, IsDefaultButtonYN isDefaultButton = IsDefaultButtonYN::No);