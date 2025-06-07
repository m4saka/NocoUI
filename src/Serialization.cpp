#include "NocoUI/Serialization.hpp"
#include "NocoUI/Component/Component.hpp"

namespace noco
{
	std::shared_ptr<ComponentBase> CreateComponentFromJSON(const JSON& json)
	{
		const auto type = json[U"type"].getString();
		if (type == U"Label")
		{
			auto component = std::make_shared<Label>();
			if (!component->tryReadFromJSON(json))
			{
				Logger << U"[NocoUI warning] Failed to read Label component from JSON";
				return nullptr;
			}
			return component;
		}
		else if (type == U"Sprite")
		{
			auto component = std::make_shared<Sprite>();
			if (!component->tryReadFromJSON(json))
			{
				Logger << U"[NocoUI warning] Failed to read Sprite component from JSON";
				return nullptr;
			}
			return component;
		}
		else if (type == U"RectRenderer")
		{
			auto component = std::make_shared<RectRenderer>();
			if (!component->tryReadFromJSON(json))
			{
				Logger << U"[NocoUI warning] Failed to read RectRenderer component from JSON";
				return nullptr;
			}
			return component;
		}
		else if (type == U"TextBox")
		{
			auto component = std::make_shared<TextBox>();
			if (!component->tryReadFromJSON(json))
			{
				Logger << U"[NocoUI warning] Failed to read TextBox component from JSON";
				return nullptr;
			}
			return component;
		}
		else if (type == U"TextArea")
		{
			auto component = std::make_shared<TextArea>();
			if (!component->tryReadFromJSON(json))
			{
				Logger << U"[NocoUI warning] Failed to read TextArea component from JSON";
				return nullptr;
			}
			return component;
		}
		else if (type == U"InputBlocker")
		{
			auto component = std::make_shared<InputBlocker>();
			if (!component->tryReadFromJSON(json))
			{
				Logger << U"[NocoUI warning] Failed to read InputBlocker component from JSON";
				return nullptr;
			}
			return component;
		}
		else if (type == U"EventTrigger")
		{
			auto component = std::make_shared<EventTrigger>();
			if (!component->tryReadFromJSON(json))
			{
				Logger << U"[NocoUI warning] Failed to read EventTrigger component from JSON";
				return nullptr;
			}
			return component;
		}
		else if (type == U"Placeholder")
		{
			auto component = std::make_shared<Placeholder>();
			if (!component->tryReadFromJSON(json))
			{
				Logger << U"[NocoUI warning] Failed to read Placeholder component from JSON";
				return nullptr;
			}
			return component;
		}
		else
		{
			Logger << U"[NocoUI warning] Unknown component type: '{}'"_fmt(type);
			return nullptr;
		}
	}
}
