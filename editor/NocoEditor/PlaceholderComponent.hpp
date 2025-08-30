#pragma once
#include <Siv3D.hpp>
#include <NocoUI/Component/ComponentBase.hpp>
#include <NocoUI/Node.hpp>
#include <NocoUI/Property.hpp>

namespace noco
{
	// PlaceholderComponentの動的プロパティ管理用。型情報なしでも任意のプロパティを扱えるよう全てString型として保持
	class PlaceholderProperty : public IProperty
	{
	private:
		String m_name;
		PropertyValue<String> m_propertyValue;
		String m_paramRef;
		/* NonSerialized */ InteractionState m_interactionState = InteractionState::Default;
		/* NonSerialized */ Array<String> m_activeStyleStates{};
		/* NonSerialized */ Optional<String> m_currentFrameOverride;
		/* NonSerialized */ int32 m_currentFrameOverrideFrameCount = 0;

	public:
		PlaceholderProperty(const String& name, const PropertyValue<String>& propertyValue = U"")
			: m_name{ name }
			, m_propertyValue{ propertyValue }
		{
		}

		[[nodiscard]]
		StringView name() const override
		{
			return m_name;
		}
		
		[[nodiscard]]
		const PropertyValue<String>& propertyValue() const
		{
			return m_propertyValue;
		}
		
		void setPropertyValue(const PropertyValue<String>& value)
		{
			m_propertyValue = value;
		}

		void update(InteractionState interactionState, const Array<String>& activeStyleStates, double, const HashTable<String, ParamValue>& params, SkipsSmoothingYN) override
		{
			m_interactionState = interactionState;
			m_activeStyleStates = activeStyleStates;
			
			if (!m_paramRef.isEmpty())
			{
				if (auto it = params.find(m_paramRef); it != params.end())
				{
					if (auto val = GetParamValueAs<String>(it->second))
					{
						setCurrentFrameOverride(*val);
					}
				}
			}
			
			if (m_currentFrameOverrideFrameCount > 0)
			{
				--m_currentFrameOverrideFrameCount;
				if (m_currentFrameOverrideFrameCount == 0)
				{
					m_currentFrameOverride = none;
				}
			}
		}

		void appendJSON(JSON& json) const override
		{
			json[m_name] = m_propertyValue.toJSON();
			if (!m_paramRef.isEmpty())
			{
				json[U"{}_paramRef"_fmt(m_name)] = m_paramRef;
			}
		}

		void readFromJSON(const JSON& json) override
		{
			if (!json.contains(m_name))
			{
				return;
			}
			m_propertyValue = PropertyValue<String>::fromJSON(json[m_name]);
			
			const String paramRefKey = U"{}_paramRef"_fmt(m_name);
			if (json.contains(paramRefKey))
			{
				m_paramRef = json[paramRefKey].getString();
			}
		}

		[[nodiscard]]
		String propertyValueStringOfDefault() const override
		{
			return m_propertyValue.defaultValue;
		}
		
		[[nodiscard]]
		Optional<String> propertyValueStringOf(InteractionState interactionState, const Array<String>& activeStyleStates) const override
		{
			if (m_currentFrameOverride)
			{
				return *m_currentFrameOverride;
			}
			
			return m_propertyValue.getValueStringOf(interactionState, activeStyleStates);
		}
		
		[[nodiscard]]
		String propertyValueStringOfFallback(InteractionState interactionState, const Array<String>& activeStyleStates) const override
		{
			if (m_currentFrameOverride)
			{
				return *m_currentFrameOverride;
			}
			
			return m_propertyValue.getValueStringOfFallback(interactionState, activeStyleStates);
		}

		bool trySetPropertyValueString(StringView value) override
		{
			m_propertyValue.defaultValue = String{ value };
			return true;
		}
		
		bool trySetPropertyValueStringOf(StringView value, InteractionState interactionState, const Array<String>& activeStyleStates) override
		{
			return m_propertyValue.trySetValueStringOf(String{ value }, interactionState, activeStyleStates);
		}
		
		bool tryUnsetPropertyValueOf(InteractionState interactionState, const Array<String>& activeStyleStates) override
		{
			return m_propertyValue.tryUnsetValueOf(interactionState, activeStyleStates);
		}
		
		[[nodiscard]]
		bool hasPropertyValueOf(InteractionState interactionState, const Array<String>& activeStyleStates) const override
		{
			return m_propertyValue.hasValueOf(interactionState, activeStyleStates);
		}

		[[nodiscard]]
		PropertyEditType editType() const override
		{
			return PropertyEditType::Text;
		}
		
		[[nodiscard]]
		bool isInteractiveProperty() const override
		{
			return true;
		}
		
		[[nodiscard]]
		bool hasInteractivePropertyValue() const override
		{
			return m_propertyValue.hoveredValue.has_value() ||
			       m_propertyValue.pressedValue.has_value() ||
			       m_propertyValue.disabledValue.has_value();
		}
		
		[[nodiscard]]
		bool isSmoothProperty() const override
		{
			return m_propertyValue.smoothTime > 0.0;
		}
		
		[[nodiscard]]
		double smoothTime() const override
		{
			return m_propertyValue.smoothTime;
		}
		
		bool trySetSmoothTime(double smoothTime) override
		{
			m_propertyValue.smoothTime = smoothTime;
			return true;
		}
		
		[[nodiscard]]
		Array<String> styleStateKeys() const override
		{
			return {};
		}
		
		[[nodiscard]]
		const String& paramRef() const override
		{
			return m_paramRef;
		}
		
		void setParamRef(const String& paramRef) override
		{
			m_paramRef = paramRef;
		}
		
		[[nodiscard]]
		bool hasParamRef() const override
		{
			return !m_paramRef.isEmpty();
		}
		
		void clearParamRefIfInvalid(const HashTable<String, ParamValue>& validParams, HashSet<String>& clearedParams) override
		{
			if (!m_paramRef.isEmpty() && !validParams.contains(m_paramRef))
			{
				clearedParams.insert(m_paramRef);
				m_paramRef.clear();
			}
		}
		
		void setCurrentFrameOverride(const String& value)
		{
			m_currentFrameOverride = value;
			m_currentFrameOverrideFrameCount = Scene::FrameCount();
		}
		
		[[nodiscard]]
		const String& value() const
		{
			if (m_currentFrameOverride.has_value() && m_currentFrameOverrideFrameCount == Scene::FrameCount())
			{
				return *m_currentFrameOverride;
			}
			return m_propertyValue.value(m_interactionState, m_activeStyleStates);
		}
	};

	class PlaceholderComponent : public SerializableComponentBase, public std::enable_shared_from_this<PlaceholderComponent>
	{
	private:
		String m_originalType;
		HashTable<String, std::unique_ptr<PlaceholderProperty>> m_properties;

		/* NonSerialized */ const void* m_schema = nullptr;
		/* NonSerialized */ Optional<Texture> m_thumbnailTexture;

	public:
		PlaceholderComponent(const String& originalType, const JSON& originalData, detail::WithInstanceIdYN withInstanceId = detail::WithInstanceIdYN::No)
			: SerializableComponentBase{ U"Placeholder", {} }  // 空の配列で初期化、後でupdatePropertyListInternalを呼ぶ
			, m_originalType{ originalType }
		{
			if (originalData.isObject())
			{
				HashSet<String> processedKeys;
				
				for (const auto& [key, value] : originalData)
				{
					const String keyStr = key;
					
					if (keyStr == U"type" || keyStr == U"_instanceId")
					{
						// メタデータはプロパティ以外で管理されるためスキップ
						continue;
					}
					
					if (keyStr.ends_with(U"_paramRef"))
					{
						// パラメータ参照はreadFromJSON側で処理されるためスキップ
						continue;
					}
					
					if (processedKeys.contains(keyStr))
					{
						continue;
					}
					
					auto property = std::make_unique<PlaceholderProperty>(keyStr);
					property->readFromJSON(originalData);
					m_properties[keyStr] = std::move(property);
					processedKeys.insert(keyStr);
				}
			}
			
			if (withInstanceId && originalData.contains(U"_instanceId"))
			{
				setInstanceId(originalData[U"_instanceId"].get<uint64>());
			}
			
			updatePropertyListInternal();
		}

		[[nodiscard]]
		static std::shared_ptr<PlaceholderComponent> Create(const String& originalType, const JSON& originalData, detail::WithInstanceIdYN withInstanceId = detail::WithInstanceIdYN::No)
		{
			return std::make_shared<PlaceholderComponent>(originalType, originalData, withInstanceId);
		}

		[[nodiscard]]
		const String& originalType() const
		{
			return m_originalType;
		}

		[[nodiscard]]
		JSON originalData() const
		{
			JSON result;
			result[U"type"] = m_originalType;
			
			for (const auto& [name, property] : m_properties)
			{
				JSON temp;
				property->appendJSON(temp);
				
				for (const auto& [key, value] : temp)
				{
					result[key] = value;
				}
			}
			
			return result;
		}

		JSON toJSONOverrideInternal(detail::WithInstanceIdYN withInstanceId) const override
		{
			JSON result;
			result[U"type"] = m_originalType;
			
			for (const auto& [name, property] : m_properties)
			{
				property->appendJSON(result);
			}
			
			if (withInstanceId)
			{
				result[U"_instanceId"] = instanceId();
			}
			return result;
		}

		bool tryReadFromJSONOverrideInternal(const JSON& json, detail::WithInstanceIdYN withInstanceId) override
		{
			if (!json.contains(U"type"))
			{
				return false;
			}
			
			m_originalType = json[U"type"].getString();
			
			m_properties.clear();
			
			if (json.isObject())
			{
				HashSet<String> processedKeys;
				
				for (const auto& [key, value] : json)
				{
					const String keyStr = key;
					
					if (keyStr == U"type" || keyStr == U"_instanceId")
					{
						// メタデータはプロパティ以外で管理されるためスキップ
						continue;
					}
					
					if (keyStr.ends_with(U"_paramRef"))
					{
						// パラメータ参照はreadFromJSON側で処理されるためスキップ
						continue;
					}
					
					if (processedKeys.contains(keyStr))
					{
						continue;
					}
					
					auto property = std::make_unique<PlaceholderProperty>(keyStr);
					property->readFromJSON(json);
					m_properties[keyStr] = std::move(property);
					processedKeys.insert(keyStr);
				}
			}
			
			if (withInstanceId && json.contains(U"_instanceId"))
			{
				setInstanceId(json[U"_instanceId"].get<uint64>());
			}
			
			updatePropertyListInternal();
			
			return true;
		}

		void draw(const Node& node) const override
		{
			if (m_thumbnailTexture)
			{
				const RectF rect = node.regionRect();

				const Vec2 textureSize{ m_thumbnailTexture->size() };
				const Vec2 rectSize = rect.size;

				const double scaleX = rectSize.x / textureSize.x;
				const double scaleY = rectSize.y / textureSize.y;
				const double scale = Min(Min(scaleX, scaleY), 1.0);

				const Vec2 drawSize = textureSize * scale;
				const Vec2 drawPos = rect.center() - drawSize / 2;

				m_thumbnailTexture->resized(drawSize).draw(drawPos);
			}
		}
		
		void setSchema(const void* schema) { m_schema = schema; }
		const void* schema() const { return m_schema; }
		void setThumbnailTexture(const Optional<Texture>& texture) { m_thumbnailTexture = texture; }
		const Optional<Texture>& thumbnailTexture() const { return m_thumbnailTexture; }

		[[nodiscard]]
		String getPropertyValueString(const String& propertyName) const
		{
			if (auto it = m_properties.find(propertyName); it != m_properties.end())
			{
				return it->second->value();
			}
			return U"";
		}
		
		void setPropertyValueString(const String& propertyName, const String& value)
		{
			if (auto it = m_properties.find(propertyName); it != m_properties.end())
			{
				// 既存プロパティの場合は値のみ更新
				it->second->trySetPropertyValueString(value);
			}
			else
			{
				// 新規プロパティの場合は作成してプロパティリストを更新
				auto property = std::make_unique<PlaceholderProperty>(propertyName, PropertyValue<String>{ value });
				m_properties[propertyName] = std::move(property);
				updatePropertyListInternal();
			}
		}
		
		
		[[nodiscard]]
		bool hasProperty(const String& propertyName) const
		{
			return m_properties.contains(propertyName);
		}
		
		[[nodiscard]]
		Array<String> getPropertyNames() const
		{
			Array<String> names;
			for (const auto& [name, property] : m_properties)
			{
				names.push_back(name);
			}
			return names;
		}
		
		[[nodiscard]]
		PlaceholderProperty* getProperty(const String& propertyName)
		{
			if (auto it = m_properties.find(propertyName); it != m_properties.end())
			{
				return it->second.get();
			}
			return nullptr;
		}
		
		[[nodiscard]]
		const PlaceholderProperty* getProperty(const String& propertyName) const
		{
			if (auto it = m_properties.find(propertyName); it != m_properties.end())
			{
				return it->second.get();
			}
			return nullptr;
		}

	private:
		void updatePropertyListInternal()
		{
			Array<IProperty*> properties;
			for (auto& [name, property] : m_properties)
			{
				properties.push_back(property.get());
			}
			ComponentBase::setProperties(properties);
		}
	};
}
