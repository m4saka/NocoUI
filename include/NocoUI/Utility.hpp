#pragma once
#include "Canvas.hpp"

namespace noco
{
    [[nodiscard]]
    std::shared_ptr<Node> LoadNode(FilePathView path, AllowExceptions allowExceptions = AllowExceptions::No);

    [[nodiscard]]
    std::shared_ptr<Canvas> LoadCanvas(FilePathView path, RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes, AllowExceptions allowExceptions = AllowExceptions::No);
}
