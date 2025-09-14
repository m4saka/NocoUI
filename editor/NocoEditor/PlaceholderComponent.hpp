#pragma once
#include <Siv3D.hpp>
#include <NocoUI/Component/ComponentBase.hpp>
#include <NocoUI/Node.hpp>
#include <NocoUI/Property.hpp>
#include <NocoUI/LRTB.hpp>
#include "ComponentSchema.hpp"
#include "ComponentSchemaLoader.hpp"
#include <variant>

namespace noco
{
	// JSON値の型に対応するPropertyEditTypeを取得
	[[nodiscard]]
	inline PropertyEditType GetEditTypeFromJSONType(const JSON& value)
	{
		if (value.isBool())
		{
			return PropertyEditType::Bool;
		}
		else if (value.isNumber())
		{
			return PropertyEditType::Number;
		}
		else if (value.isString())
		{
			return PropertyEditType::Text;
		}
		else if (value.isArray() && value.size() == 2)
		{
			return PropertyEditType::Vec2;
		}
		else if (value.isArray() && value.size() == 4)
		{
			// Colorもあるが、スキーマがなく特定できないため値制限のないLRTB扱いとする
			return PropertyEditType::LRTB;
		}
		Logger << U"[NocoUI warning] Unknown JSON type for property, defaulting to Text";
		return PropertyEditType::Text;
	}
	
	class PlaceholderProperty : public IProperty
	{
	private:
		using PropertyVariant = std::variant<
			PropertyValue<bool>,
			PropertyValue<double>,
			PropertyValue<String>,
			PropertyValue<Color>,
			PropertyValue<Vec2>,
			PropertyValue<LRTB>
		>;
		
		String m_name;
		PropertyEditType m_editType;
		PropertyVariant m_propertyValue;
		String m_paramRef;

	public:
		PlaceholderProperty(const String& name, PropertyEditType editType = PropertyEditType::Text)
			: m_name{ name }
			, m_editType{ editType }
			, m_propertyValue{ MakePropertyValueOfEditType(editType) }
		{
		}

		[[nodiscard]]
		StringView name() const override
		{
			return m_name;
		}
		

		void update(InteractionState, const Array<String>&, double, const HashTable<String, ParamValue>&, SkipSmoothingYN) override
		{
			// エディタ専用型なのでupdate処理は不要
		}

		void appendJSON(JSON& json) const override
		{
			std::visit([&](const auto& propValue) {
				using T = std::decay_t<decltype(propValue)>;
				json[m_name] = propValue.toJSON();
			}, m_propertyValue);
			
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
			
			const JSON& valueJson = json[m_name];
			
			// PropertyValueをJSONから読み込み
			switch (m_editType)
			{
			case PropertyEditType::Bool:
				m_propertyValue = PropertyValue<bool>::fromJSON(valueJson, false);
				break;
				
			case PropertyEditType::Number:
				m_propertyValue = PropertyValue<double>::fromJSON(valueJson, 0.0);
				break;
				
			case PropertyEditType::Text:
			case PropertyEditType::Enum:
				m_propertyValue = PropertyValue<String>::fromJSON(valueJson, String{});
				break;
				
			case PropertyEditType::Vec2:
				m_propertyValue = PropertyValue<Vec2>::fromJSON(valueJson, Vec2::Zero());
				break;
				
			case PropertyEditType::Color:
				m_propertyValue = PropertyValue<Color>::fromJSON(valueJson, Color{});
				break;
				
			case PropertyEditType::LRTB:
				m_propertyValue = PropertyValue<LRTB>::fromJSON(valueJson, LRTB::Zero());
				break;
			}
			
			const String paramRefKey = U"{}_paramRef"_fmt(m_name);
			if (json.contains(paramRefKey))
			{
				m_paramRef = json[paramRefKey].getString();
			}
		}

		[[nodiscard]]
		String propertyValueStringOfDefault() const override
		{
			return std::visit([](const auto& propValue) {
				return propValue.getValueStringOfDefault();
			}, m_propertyValue);
		}
		
		[[nodiscard]]
		Optional<String> propertyValueStringOf(InteractionState interactionState, const Array<String>& activeStyleStates) const override
		{
			return std::visit([&](const auto& propValue) {
				return propValue.getValueStringOf(interactionState, activeStyleStates);
			}, m_propertyValue);
		}
		
		[[nodiscard]]
		String propertyValueStringOfFallback(InteractionState interactionState, const Array<String>& activeStyleStates) const override
		{
			return std::visit([&](const auto& propValue) {
				return propValue.getValueStringOfFallback(interactionState, activeStyleStates);
			}, m_propertyValue);
		}

		bool trySetPropertyValueString(StringView valueStr) override
		{
			return std::visit([&](auto& propValue) -> bool {
				return propValue.trySetValueString(valueStr);
			}, m_propertyValue);
		}
		
		bool trySetPropertyValueStringOf(StringView value, InteractionState interactionState, StringView styleState = U"") override
		{
			return std::visit([&](auto& propValue) -> bool {
				return propValue.trySetValueStringOf(value, interactionState, styleState);
			}, m_propertyValue);
		}
		
		void unsetPropertyValueOf(InteractionState interactionState, StringView styleState = U"") override
		{
			std::visit([&](auto& propValue) {
				propValue.unsetValueOf(interactionState, styleState);
			}, m_propertyValue);
		}
		
		[[nodiscard]]
		bool hasPropertyValueOf(InteractionState interactionState, StringView styleState = U"") const override
		{
			return std::visit([&](const auto& propValue) -> bool {
				return propValue.hasValueOf(interactionState, styleState);
			}, m_propertyValue);
		}

		[[nodiscard]]
		PropertyEditType editType() const override
		{
			return m_editType;
		}
		
		[[nodiscard]]
		bool isInteractiveProperty() const override
		{
			return true;
		}
		
		[[nodiscard]]
		bool hasInteractivePropertyValue() const override
		{
			return std::visit([](const auto& propValue) -> bool {
				return propValue.hasInteractiveValue();
			}, m_propertyValue);
		}
		
		[[nodiscard]]
		bool isSmoothProperty() const override
		{
			return std::visit([](const auto& propValue) -> bool {
				return propValue.smoothTime() > 0.0;
			}, m_propertyValue);
		}
		
		[[nodiscard]]
		double smoothTime() const override
		{
			return std::visit([](const auto& propValue) -> double {
				return propValue.smoothTime();
			}, m_propertyValue);
		}
		
		bool trySetSmoothTime(double smoothTime) override
		{
			std::visit([&](auto& propValue) {
				propValue.setSmoothTime(smoothTime);
			}, m_propertyValue);
			return true;
		}
		
		[[nodiscard]]
		Array<String> styleStateKeys() const override
		{
			return std::visit([](const auto& propValue) -> Array<String> {
				if (propValue.styleStateValues())
				{
					Array<String> keys;
					for (const auto& [key, value] : *propValue.styleStateValues())
					{
						keys.push_back(key);
					}
					return keys;
				}
				return {};
			}, m_propertyValue);
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

		void clearCurrentFrameOverride() override
		{
			// エディタ専用型なので処理不要
		}
		
	private:
		[[nodiscard]]
		static PropertyVariant MakePropertyValueOfEditType(PropertyEditType editType)
		{
			switch (editType)
			{
			case PropertyEditType::Bool:
				return PropertyValue<bool>{ false };
			case PropertyEditType::Number:
				return PropertyValue<double>{ 0.0 };
			case PropertyEditType::Text:
			case PropertyEditType::Enum:
				return PropertyValue<String>{ String{} };
			case PropertyEditType::Vec2:
				return PropertyValue<Vec2>{ Vec2::Zero() };
			case PropertyEditType::Color:
				return PropertyValue<Color>{ Color{} };
			case PropertyEditType::LRTB:
				return PropertyValue<LRTB>{ LRTB::Zero() };
			}
			return PropertyValue<String>{ String{} };  // デフォルト
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
			// スキーマから型情報を取得
			const editor::ComponentSchema* schema = editor::ComponentSchemaLoader::GetSchema(originalType);
			
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
					
					// 型情報を決定
					PropertyEditType editType = PropertyEditType::Text;  // デフォルト
					
					if (schema)
					{
						// スキーマがある場合はスキーマから型情報を取得
						if (auto propSchema = schema->findProperty(keyStr))
						{
							editType = propSchema->editType;
						}
						else
						{
							// スキーマにないプロパティの場合はJSON値から型を取得
							editType = GetEditTypeFromJSONType(value);
						}
					}
					else
					{
						// スキーマがない場合はJSON値から型を取得
						editType = GetEditTypeFromJSONType(value);
					}
					
					auto property = std::make_unique<PlaceholderProperty>(keyStr, editType);
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

		[[nodiscard]]
		String typeOverrideInternal() const override
		{
			return m_originalType;
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
			
			// スキーマから型情報を取得
			const editor::ComponentSchema* schema = editor::ComponentSchemaLoader::GetSchema(m_originalType);
			
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
					
					// 型情報を決定
					PropertyEditType editType = PropertyEditType::Text;  // デフォルト
					
					if (schema)
					{
						// スキーマがある場合はスキーマから型情報を取得
						if (auto propSchema = schema->findProperty(keyStr))
						{
							editType = propSchema->editType;
						}
						else
						{
							// スキーマにないプロパティの場合はJSON値から型を取得
							editType = GetEditTypeFromJSONType(value);
						}
					}
					else
					{
						// スキーマがない場合はJSON値から型を取得
						editType = GetEditTypeFromJSONType(value);
					}
					
					auto property = std::make_unique<PlaceholderProperty>(keyStr, editType);
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
				return it->second->propertyValueStringOfDefault();
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
				Logger << U"[NocoEditor warning] Property '{}' not found in PlaceholderComponent. Ignored."_fmt(propertyName);
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
