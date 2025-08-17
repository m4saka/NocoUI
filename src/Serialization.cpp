#include "NocoUI/Serialization.hpp"
#include "NocoUI/Component/Component.hpp"

namespace noco
{
	namespace detail
	{
		std::shared_ptr<ComponentBase> CreateComponentFromJSONImpl(const JSON& json, IncludesInstanceIdYN includesInstanceId)
		{
			const auto type = json[U"type"].getOr<String>(U"");
			if (type == U"Label")
			{
				auto component = std::make_shared<Label>();
				if (!component->tryReadFromJSONImpl(json, includesInstanceId))
				{
					Logger << U"[NocoUI warning] Failed to read Label component from JSON";
					return nullptr;
				}
				return component;
			}
			else if (type == U"Sprite")
			{
				auto component = std::make_shared<Sprite>();
				if (!component->tryReadFromJSONImpl(json, includesInstanceId))
				{
					Logger << U"[NocoUI warning] Failed to read Sprite component from JSON";
					return nullptr;
				}
				return component;
			}
			else if (type == U"RectRenderer")
			{
				auto component = std::make_shared<RectRenderer>();
				if (!component->tryReadFromJSONImpl(json, includesInstanceId))
				{
					Logger << U"[NocoUI warning] Failed to read RectRenderer component from JSON";
					return nullptr;
				}
				return component;
			}
			else if (type == U"TextBox")
			{
				auto component = std::make_shared<TextBox>();
				if (!component->tryReadFromJSONImpl(json, includesInstanceId))
				{
					Logger << U"[NocoUI warning] Failed to read TextBox component from JSON";
					return nullptr;
				}
				return component;
			}
			else if (type == U"TextArea")
			{
				auto component = std::make_shared<TextArea>();
				if (!component->tryReadFromJSONImpl(json, includesInstanceId))
				{
					Logger << U"[NocoUI warning] Failed to read TextArea component from JSON";
					return nullptr;
				}
				return component;
			}
			else if (type == U"EventTrigger")
			{
				auto component = std::make_shared<EventTrigger>();
				if (!component->tryReadFromJSONImpl(json, includesInstanceId))
				{
					Logger << U"[NocoUI warning] Failed to read EventTrigger component from JSON";
					return nullptr;
				}
				return component;
			}
			else if (type == U"Placeholder")
			{
				auto component = std::make_shared<Placeholder>();
				if (!component->tryReadFromJSONImpl(json, includesInstanceId))
				{
					Logger << U"[NocoUI warning] Failed to read Placeholder component from JSON";
					return nullptr;
				}
				return component;
			}
			else if (type == U"CursorChanger")
			{
				auto component = std::make_shared<CursorChanger>();
				if (!component->tryReadFromJSONImpl(json, includesInstanceId))
				{
					Logger << U"[NocoUI warning] Failed to read CursorChanger component from JSON";
					return nullptr;
				}
				return component;
			}
			else if (type == U"UISound")
			{
				auto component = std::make_shared<UISound>();
				if (!component->tryReadFromJSONImpl(json, includesInstanceId))
				{
					Logger << U"[NocoUI warning] Failed to read UISound component from JSON";
					return nullptr;
				}
				return component;
			}
			else if (type == U"Tween")
			{
				auto component = std::make_shared<Tween>();
				if (!component->tryReadFromJSONImpl(json, includesInstanceId))
				{
					Logger << U"[NocoUI warning] Failed to read Tween component from JSON";
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

	[[nodiscard]]
	std::shared_ptr<ComponentBase> CreateComponentFromJSON(const JSON& json)
	{
		return detail::CreateComponentFromJSONImpl(json, detail::IncludesInstanceIdYN::No);
	}
}
