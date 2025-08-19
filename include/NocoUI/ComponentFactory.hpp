#pragma once
#include <Siv3D.hpp>
#include "Component/ComponentBase.hpp"
#include "Component/PlaceholderComponent.hpp"

namespace noco
{
	enum class UnknownComponentBehavior : uint8
	{
		Skip,
		CreatePlaceholder,
		ThrowError,
	};

	class ComponentFactory
	{
	private:
		using ComponentFactoryFunc = std::function<std::shared_ptr<SerializableComponentBase>()>;
		
		HashTable<String, ComponentFactoryFunc> m_factories;
		UnknownComponentBehavior m_unknownBehavior = UnknownComponentBehavior::Skip;
		
	public:
		ComponentFactory() = default;
		
		void setUnknownComponentBehavior(UnknownComponentBehavior behavior)
		{
			m_unknownBehavior = behavior;
		}
		
		[[nodiscard]]
		UnknownComponentBehavior unknownComponentBehavior() const
		{
			return m_unknownBehavior;
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
			
			switch (m_unknownBehavior)
			{
			case UnknownComponentBehavior::Skip:
				return nullptr;
				
			case UnknownComponentBehavior::CreatePlaceholder:
				return PlaceholderComponent::Create(type, json, withInstanceId);
				
			case UnknownComponentBehavior::ThrowError:
				throw Error{ U"Unknown component type: {}"_fmt(type) };
			}
			
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