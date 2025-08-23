#pragma once
#include <Siv3D.hpp>
#include "Component/ComponentBase.hpp"

namespace noco
{
	class ComponentFactory
	{
	public:
		using UnknownComponentHandler = std::function<std::shared_ptr<ComponentBase>(const String& type, const JSON& json, detail::WithInstanceIdYN withInstanceId)>;
		
	private:
		using ComponentFactoryFunc = std::function<std::shared_ptr<SerializableComponentBase>()>;
		
		HashTable<String, ComponentFactoryFunc> m_factories;
		UnknownComponentHandler m_unknownComponentHandler;
		
	public:
		ComponentFactory() = default;
		
		void setUnknownComponentHandler(const UnknownComponentHandler& handler)
		{
			m_unknownComponentHandler = handler;
		}
		
		template <typename TComponent>
		void registerComponentType(const String& typeName)
			requires std::derived_from<TComponent, SerializableComponentBase>
		{
			auto factory = []() -> std::shared_ptr<SerializableComponentBase>
			{
				return std::make_shared<TComponent>();
			};
			
			// 後に登録されたものを優先するため上書き
			m_factories.insert_or_assign(typeName, factory);
		}
		
		[[nodiscard]]
		std::shared_ptr<SerializableComponentBase> createComponent(StringView typeName) const
		{
			if (auto it = m_factories.find(typeName); it != m_factories.end())
			{
				return it->second();
			}
			return nullptr;
		}
		
		[[nodiscard]]
		std::shared_ptr<ComponentBase> createComponentFromJSON(const JSON& json, detail::WithInstanceIdYN withInstanceId = detail::WithInstanceIdYN::No) const
		{
			const auto type = json[U"type"].getOr<String>(U"");
			if (type.isEmpty())
			{
				return nullptr;
			}
			
			auto component = createComponent(type);
			if (component)
			{
				if (component->tryReadFromJSON(json, withInstanceId))
				{
					return component;
				}
				Logger << U"[NocoUI warning] Failed to read {} component from JSON"_fmt(type);
				return nullptr;
			}
			
			if (m_unknownComponentHandler)
			{
				return m_unknownComponentHandler(type, json, withInstanceId);
			}
			
			Logger << U"[NocoUI warning] Unknown component type: {}"_fmt(type);
			return nullptr;
		}
		
		[[nodiscard]]
		bool hasType(StringView typeName) const
		{
			return m_factories.contains(typeName);
		}
		
		[[nodiscard]]
		Array<String> getRegisteredTypes() const
		{
			Array<String> types;
			types.reserve(m_factories.size());
			for (const auto& [typeName, _] : m_factories)
			{
				types.push_back(typeName);
			}
			return types;
		}
		
		static ComponentFactory CreateWithBuiltinComponents();
		
		static const ComponentFactory& GetBuiltinFactory();
		
	};
}