#include "NocoUI/Utility.hpp"

namespace noco
{
	std::shared_ptr<Node> LoadNode(FilePathView path, AllowExceptions allowExceptions)
	{
		const auto json = JSON::Load(path, allowExceptions);
		if (!json)
		{
			if (allowExceptions)
			{
				throw Error{ U"Node::LoadFile: Failed to load file '{}'"_fmt(path) };
			}
			return nullptr;
		}
		return Node::CreateFromJSON(json);
	}

	std::shared_ptr<Canvas> LoadCanvas(FilePathView path, AllowExceptions allowExceptions)
	{
		// JSONファイルを直接読み込んでCanvasとして解釈
		const JSON json = JSON::Load(path, allowExceptions);
		if (!json)
		{
			return nullptr;
		}
		return Canvas::CreateFromJSON(json);
	}
}
