#pragma once
#include "Canvas.hpp"

namespace noco
{
	class ComponentFactory;
	
	[[nodiscard]]
	std::shared_ptr<Node> LoadNode(FilePathView path, AllowExceptions allowExceptions = AllowExceptions::No);

	[[nodiscard]]
	std::shared_ptr<Canvas> LoadCanvas(FilePathView path, AllowExceptions allowExceptions = AllowExceptions::No);
	
	[[nodiscard]]
	std::shared_ptr<Canvas> LoadCanvas(FilePathView path, const ComponentFactory& factory, AllowExceptions allowExceptions = AllowExceptions::No);
}
