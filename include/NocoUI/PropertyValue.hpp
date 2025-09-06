#pragma once
#include <Siv3D.hpp>
#include "YN.hpp"
#include "InteractionState.hpp"
#include "Serialization.hpp"
#include "LRTB.hpp"

// std::pair<String, InteractionState>用のハッシュ関数
template <>
struct std::hash<std::pair<s3d::String, noco::InteractionState>>
{
	size_t operator()(const std::pair<s3d::String, noco::InteractionState>& p) const noexcept
	{
		size_t h1 = std::hash<s3d::String>{}(p.first);
		size_t h2 = std::hash<noco::InteractionState>{}(p.second);
		return h1 ^ (h2 << 1);
	}
};

namespace noco
{
	template <class T>
	struct PropertyStyleStateValue
	{
		T defaultValue;
		Optional<T> hoveredValue = none;
		Optional<T> pressedValue = none;
		Optional<T> disabledValue = none;
		
		/*implicit*/ PropertyStyleStateValue(const T& defaultVal)
			: defaultValue{ static_cast<T>(defaultVal) }
		{
		}
		
		PropertyStyleStateValue() = default;
		PropertyStyleStateValue(const PropertyStyleStateValue&) = default;
		PropertyStyleStateValue& operator=(const PropertyStyleStateValue&) = default;
		PropertyStyleStateValue(PropertyStyleStateValue&&) noexcept = default;
		PropertyStyleStateValue& operator=(PropertyStyleStateValue&&) noexcept = default;
	};

	template <class T>
	struct PropertyInteractionValues
	{
		Optional<T> hoveredValue = none;
		Optional<T> pressedValue = none;
		Optional<T> disabledValue = none;

		PropertyInteractionValues() = default;
		PropertyInteractionValues(const Optional<T>& hoveredVal, const Optional<T>& pressedVal, const Optional<T>& disabledVal)
			: hoveredValue{ hoveredVal }
			, pressedValue{ pressedVal }
			, disabledValue{ disabledVal }
		{
		}
		PropertyInteractionValues(const PropertyInteractionValues&) = default;
		PropertyInteractionValues& operator=(const PropertyInteractionValues&) = default;
		PropertyInteractionValues(PropertyInteractionValues&&) noexcept = default;
		PropertyInteractionValues& operator=(PropertyInteractionValues&&) noexcept = default;

		[[nodiscard]]
		bool hasAny() const noexcept
		{
			return hoveredValue.has_value() || pressedValue.has_value() || disabledValue.has_value();
		}
	};
	
	template <class T>
	struct PropertyValue
	{
		T defaultValue;
		std::unique_ptr<PropertyInteractionValues<T>> interactionValues;
		double smoothTime = 0.0;
		
		std::unique_ptr<HashTable<String, PropertyStyleStateValue<T>>> styleStateValues;

		/*implicit*/ PropertyValue(const T& defaultVal)
			: defaultValue{ static_cast<T>(defaultVal) }
		{
		}
		
		PropertyValue(const PropertyValue& other)
			: defaultValue{ other.defaultValue }
			, smoothTime{ other.smoothTime }
		{
			if (other.interactionValues)
			{
				interactionValues = std::make_unique<PropertyInteractionValues<T>>(*other.interactionValues);
			}
			if (other.styleStateValues)
			{
				styleStateValues = std::make_unique<HashTable<String, PropertyStyleStateValue<T>>>(*other.styleStateValues);
			}
		}
		
		PropertyValue& operator=(const PropertyValue& other)
		{
			if (this != &other)
			{
				defaultValue = other.defaultValue;
				smoothTime = other.smoothTime;

				if (other.interactionValues)
				{
					interactionValues = std::make_unique<PropertyInteractionValues<T>>(*other.interactionValues);
				}
				else
				{
					interactionValues.reset();
				}
				
				if (other.styleStateValues)
				{
					styleStateValues = std::make_unique<HashTable<String, PropertyStyleStateValue<T>>>(*other.styleStateValues);
				}
				else
				{
					styleStateValues.reset();
				}
			}
			return *this;
		}
		
		PropertyValue(PropertyValue&& other) noexcept = default;
		PropertyValue& operator=(PropertyValue&& other) noexcept = default;

		template <class U>
		/*implicit*/ PropertyValue(const U& defaultVal) requires std::convertible_to<U, T>
			: defaultValue{ static_cast<T>(defaultVal) }
		{
		}

		/*implicit*/ PropertyValue(StringView defaultVal) requires std::same_as<T, String>
			: defaultValue{ static_cast<T>(String{ defaultVal }) }
		{
		}

		PropertyValue(const T& defaultVal, const Optional<T>& hoveredVal, const Optional<T>& pressedVal, const Optional<T>& disabledVal, double smoothTimeVal = 0.0)
			: defaultValue{ defaultVal }
			, interactionValues{ hoveredVal || pressedVal || disabledVal ?
				std::make_unique<PropertyInteractionValues<T>>(PropertyInteractionValues<T>{ hoveredVal, pressedVal, disabledVal }) : nullptr }
			, smoothTime{ smoothTimeVal }
		{
		}

		[[nodiscard]]
		const T& value(InteractionState interactionState, const Array<String>& activeStyleStates) const
		{
			// activeStyleStatesがある場合は、優先度の高い順にstyleStateの一致を調べる
			if (!activeStyleStates.empty() && styleStateValues)
			{
				// 優先度の高い順にチェック(配列の末尾から)
				for (auto styleStateIt = activeStyleStates.rbegin(); styleStateIt != activeStyleStates.rend(); ++styleStateIt)
				{
					const String& currentStyleState = *styleStateIt;
					
					// styleStateのPropertyStyleStateValueを取得
					if (auto it = styleStateValues->find(currentStyleState); it != styleStateValues->end())
					{
						const PropertyStyleStateValue<T>& styleStateValue = it->second;
						
						// そのstyleState内でInteractionStateのフォールバックを行う
						switch (interactionState)
						{
						case InteractionState::Default:
							return styleStateValue.defaultValue;
						case InteractionState::Hovered:
							// Hovered → Default
							if (styleStateValue.hoveredValue)
							{
								return *styleStateValue.hoveredValue;
							}
							return styleStateValue.defaultValue;
						case InteractionState::Pressed:
							// Pressed → Hovered → Default
							if (styleStateValue.pressedValue)
							{
								return *styleStateValue.pressedValue;
							}
							if (styleStateValue.hoveredValue)
							{
								return *styleStateValue.hoveredValue;
							}
							return styleStateValue.defaultValue;
						case InteractionState::Disabled:
							// Disabled → Default
							if (styleStateValue.disabledValue)
							{
								return *styleStateValue.disabledValue;
							}
							return styleStateValue.defaultValue;
						}
					}
				}
			}
			
			// activeStyleStatesがない場合、またはstyleStateの値が見つからない場合は通常の値を返す
			switch (interactionState)
			{
			case InteractionState::Default:
				break;
			case InteractionState::Hovered:
				if (const auto& hoveredVal = hoveredValue())
				{
					return *hoveredVal;
				}
				break;
			case InteractionState::Pressed:
				if (const auto& pressedVal = pressedValue())
				{
					return *pressedVal;
				}
				if (const auto& hoveredVal = hoveredValue())
				{
					return *hoveredVal;
				}
				break;
			case InteractionState::Disabled:
				if (const auto& disabledVal = disabledValue())
				{
					return *disabledVal;
				}
				break;
			}
			
			return defaultValue;
		}

		[[nodiscard]]
		bool hasHovered() const noexcept
		{
			return interactionValues && interactionValues->hoveredValue.has_value();
		}

		[[nodiscard]]
		const Optional<T>& hoveredValue() const
		{
			if (!interactionValues)
			{
				static const Optional<T> noneValue = none;
				return noneValue;
			}
			return interactionValues->hoveredValue;
		}

		[[nodiscard]]
		const Optional<T>& pressedValue() const
		{
			if (!interactionValues)
			{
				static const Optional<T> noneValue = none;
				return noneValue;
			}
			return interactionValues->pressedValue;
		}

		[[nodiscard]]
		const Optional<T>& disabledValue() const
		{
			if (!interactionValues)
			{
				static const Optional<T> noneValue = none;
				return noneValue;
			}
			return interactionValues->disabledValue;
		}

		[[nodiscard]]
		bool hasPressed() const noexcept
		{
			return interactionValues && interactionValues->pressedValue.has_value();
		}

		[[nodiscard]]
		bool hasDisabled() const noexcept
		{
			return interactionValues && interactionValues->disabledValue.has_value();
		}

		[[nodiscard]]
		JSON toJSON() const
		{
			const auto& hoveredVal = hoveredValue();
			const auto& pressedVal = pressedValue();
			const auto& disabledVal = disabledValue();

			if constexpr (std::is_enum_v<T>)
			{
				if (!hoveredVal && !pressedVal && !disabledVal && smoothTime == 0.0 && 
				    (!styleStateValues || styleStateValues->empty()))
				{
					return EnumToString(defaultValue);
				}
				JSON json;
				json[U"default"] = EnumToString(defaultValue);
				if (hoveredVal)
				{
					json[U"hovered"] = EnumToString(*hoveredVal);
				}
				if (pressedVal)
				{
					json[U"pressed"] = EnumToString(*pressedVal);
				}
				if (disabledVal)
				{
					json[U"disabled"] = EnumToString(*disabledVal);
				}
				if (smoothTime != 0.0)
				{
					json[U"smoothTime"] = smoothTime;
				}
				
				if (styleStateValues && !styleStateValues->empty())
				{
					JSON styleStatesJson;
					
					for (const auto& [state, values] : *styleStateValues)
					{
						bool hasOnlyDefault = !values.hoveredValue && !values.pressedValue && !values.disabledValue;
						
						if (hasOnlyDefault)
						{
							// Defaultのみの場合は文字列で保存
							styleStatesJson[state] = EnumToString(values.defaultValue);
						}
						else
						{
							// interactionState毎の値がある場合はオブジェクトで保存
							JSON stateJson;
							stateJson[U"default"] = EnumToString(values.defaultValue);
							if (values.hoveredValue)
							{
								stateJson[U"hovered"] = EnumToString(*values.hoveredValue);
							}
							if (values.pressedValue)
							{
								stateJson[U"pressed"] = EnumToString(*values.pressedValue);
							}
							if (values.disabledValue)
							{
								stateJson[U"disabled"] = EnumToString(*values.disabledValue);
							}
							styleStatesJson[state] = stateJson;
						}
					}
					
					json[U"styleStates"] = styleStatesJson;
				}
				
				return json;
			}
			else if constexpr (HasToJSON<T>)
			{
				if (!hoveredVal && !pressedVal && !disabledVal && smoothTime == 0.0 && 
				    (!styleStateValues || styleStateValues->empty()))
				{
					return defaultValue.toJSON();
				}
				JSON json;
				json[U"default"] = defaultValue.toJSON();
				if (hoveredVal)
				{
					json[U"hovered"] = hoveredVal->toJSON();
				}
				if (pressedVal)
				{
					json[U"pressed"] = pressedVal->toJSON();
				}
				if (disabledVal)
				{
					json[U"disabled"] = disabledVal->toJSON();
				}
				if (smoothTime != 0.0)
				{
					json[U"smoothTime"] = smoothTime;
				}
				
				if (styleStateValues && !styleStateValues->empty())
				{
					JSON styleStatesJson;
					
					for (const auto& [state, values] : *styleStateValues)
					{
						bool hasOnlyDefault = !values.hoveredValue && !values.pressedValue && !values.disabledValue;
						
						if (hasOnlyDefault)
						{
							// Defaultのみの場合は文字列で保存
							styleStatesJson[state] = values.defaultValue.toJSON();
						}
						else
						{
							// interactionState毎の値がある場合はオブジェクトで保存
							JSON stateJson;
							stateJson[U"default"] = values.defaultValue.toJSON();
							if (values.hoveredValue)
							{
								stateJson[U"hovered"] = values.hoveredValue->toJSON();
							}
							if (values.pressedValue)
							{
								stateJson[U"pressed"] = values.pressedValue->toJSON();
							}
							if (values.disabledValue)
							{
								stateJson[U"disabled"] = values.disabledValue->toJSON();
							}
							styleStatesJson[state] = stateJson;
						}
					}
					
					json[U"styleStates"] = styleStatesJson;
				}
				
				return json;
			}
			else
			{
				if (!hoveredVal && !pressedVal && !disabledVal && smoothTime == 0.0 && 
				    (!styleStateValues || styleStateValues->empty()))
				{
					return defaultValue;
				}
				JSON json;
				json[U"default"] = defaultValue;
				if (hoveredVal)
				{
					json[U"hovered"] = *hoveredVal;
				}
				if (pressedVal)
				{
					json[U"pressed"] = *pressedVal;
				}
				if (disabledVal)
				{
					json[U"disabled"] = *disabledVal;
				}
				if (smoothTime != 0.0)
				{
					json[U"smoothTime"] = smoothTime;
				}
				
				if (styleStateValues && !styleStateValues->empty())
				{
					JSON styleStatesJson;
					
					for (const auto& [state, values] : *styleStateValues)
					{
						bool hasOnlyDefault = !values.hoveredValue && !values.pressedValue && !values.disabledValue;
						
						if (hasOnlyDefault)
						{
							// Defaultのみの場合は文字列で保存
							styleStatesJson[state] = values.defaultValue;
						}
						else
						{
							// interactionState毎の値がある場合はオブジェクトで保存
							JSON stateJson;
							stateJson[U"default"] = values.defaultValue;
							if (values.hoveredValue)
							{
								stateJson[U"hovered"] = *values.hoveredValue;
							}
							if (values.pressedValue)
							{
								stateJson[U"pressed"] = *values.pressedValue;
							}
							if (values.disabledValue)
							{
								stateJson[U"disabled"] = *values.disabledValue;
							}
							styleStatesJson[state] = stateJson;
						}
					}
					
					json[U"styleStates"] = styleStatesJson;
				}
				
				return json;
			}
		}

		[[nodiscard]]
		static PropertyValue<T> fromJSON(const JSON& json, const T& defaultValue = T{})
		{
			if constexpr (std::is_enum_v<T>)
			{
				if (json.isString())
				{
					return PropertyValue<T>{ StringToEnum(json.getString(), defaultValue) };
				}
				else if (json.isObject() && json.contains(U"default"))
				{
					auto propertyValue = PropertyValue<T>
					{
						GetFromJSONOr(json, U"default", defaultValue),
						GetFromJSONOpt<T>(json, U"hovered"),
						GetFromJSONOpt<T>(json, U"pressed"),
						GetFromJSONOpt<T>(json, U"disabled"),
						GetFromJSONOr(json, U"smoothTime", 0.0),
					};
					
					if (json.contains(U"styleStates"))
					{
						const JSON& styleStatesJson = json[U"styleStates"];
						if (styleStatesJson.isObject())
						{
							propertyValue.styleStateValues = std::make_unique<HashTable<String, PropertyStyleStateValue<T>>>();
							
							for (const auto& [state, valueJson] : styleStatesJson)
							{
								if (valueJson.isString())
								{
									// 文字列の場合はDefaultのみ
									(*propertyValue.styleStateValues)[state] = PropertyStyleStateValue<T>{ StringToEnum<T>(valueJson.getString(), defaultValue) };
								}
								else if (valueJson.isObject())
								{
									// オブジェクトの場合はInteractionStateごとの値がある
									PropertyStyleStateValue<T> styleStateValue{ defaultValue };
									
									if (valueJson.contains(U"default"))
									{
										styleStateValue.defaultValue = StringToEnum<T>(valueJson[U"default"].getString(), defaultValue);
									}
									if (valueJson.contains(U"hovered"))
									{
										styleStateValue.hoveredValue = StringToEnum<T>(valueJson[U"hovered"].getString(), defaultValue);
									}
									if (valueJson.contains(U"pressed"))
									{
										styleStateValue.pressedValue = StringToEnum<T>(valueJson[U"pressed"].getString(), defaultValue);
									}
									if (valueJson.contains(U"disabled"))
									{
										styleStateValue.disabledValue = StringToEnum<T>(valueJson[U"disabled"].getString(), defaultValue);
									}
									
									(*propertyValue.styleStateValues)[state] = styleStateValue;
								}
							}
						}
					}
					
					return propertyValue;
				}
				return PropertyValue<T>{ defaultValue };
			}
			else if constexpr (HasFromJSON<T>)
			{
				if (json.isObject() && json.contains(U"default"))
				{
					auto propertyValue = PropertyValue<T>
					{
						T::fromJSON(json[U"default"], defaultValue),
						json.contains(U"hovered") ? T::fromJSON(json[U"hovered"], defaultValue) : Optional<T>{ none },
						json.contains(U"pressed") ? T::fromJSON(json[U"pressed"], defaultValue) : Optional<T>{ none },
						json.contains(U"disabled") ? T::fromJSON(json[U"disabled"], defaultValue) : Optional<T>{ none },
						json.contains(U"smoothTime") ? json[U"smoothTime"].getOr<double>(0.0) : 0.0,
					};
					
					if (json.contains(U"styleStates"))
					{
						const JSON& styleStatesJson = json[U"styleStates"];
						if (styleStatesJson.isObject())
						{
							if (!propertyValue.styleStateValues)
							{
								propertyValue.styleStateValues = std::make_unique<HashTable<String, PropertyStyleStateValue<T>>>();
							}
							
							for (const auto& [state, valueJson] : styleStatesJson)
							{
								if (valueJson.isObject() && valueJson.contains(U"default"))
								{
									// オブジェクトの場合はInteractionStateごとの値がある
									PropertyStyleStateValue<T> styleStateValues{ T::fromJSON(valueJson[U"default"], defaultValue) };
									
									if (valueJson.contains(U"hovered"))
									{
										styleStateValues.hoveredValue = T::fromJSON(valueJson[U"hovered"], defaultValue);
									}
									if (valueJson.contains(U"pressed"))
									{
										styleStateValues.pressedValue = T::fromJSON(valueJson[U"pressed"], defaultValue);
									}
									if (valueJson.contains(U"disabled"))
									{
										styleStateValues.disabledValue = T::fromJSON(valueJson[U"disabled"], defaultValue);
									}
									
									(*propertyValue.styleStateValues)[state] = styleStateValues;
								}
								else
								{
									// 文字列の場合はDefaultのみの値
									(*propertyValue.styleStateValues)[state] = PropertyStyleStateValue<T>{ T::fromJSON(valueJson, defaultValue) };
								}
							}
						}
					}
					
					return propertyValue;
				}
				return PropertyValue<T>{ T::fromJSON(json, defaultValue) };
			}
			else
			{
				if (json.isObject() && json.contains(U"default"))
				{
					auto propertyValue = PropertyValue<T>
					{
						GetFromJSONOr(json, U"default", defaultValue),
						GetFromJSONOpt<T>(json, U"hovered"),
						GetFromJSONOpt<T>(json, U"pressed"),
						GetFromJSONOpt<T>(json, U"disabled"),
						GetFromJSONOr(json, U"smoothTime", 0.0),
					};
					
					if (json.contains(U"styleStates"))
					{
						const JSON& styleStatesJson = json[U"styleStates"];
						if (styleStatesJson.isObject())
						{
							if (!propertyValue.styleStateValues)
							{
								propertyValue.styleStateValues = std::make_unique<HashTable<String, PropertyStyleStateValue<T>>>();
							}
							
							for (const auto& [state, valueJson] : styleStatesJson)
							{
								if (valueJson.isObject() && valueJson.contains(U"default"))
								{
									// オブジェクトの場合はInteractionStateごとの値がある
									PropertyStyleStateValue<T> styleStateValues{ valueJson[U"default"].get<T>() };
									
									if (valueJson.contains(U"hovered"))
									{
										styleStateValues.hoveredValue = valueJson[U"hovered"].get<T>();
									}
									if (valueJson.contains(U"pressed"))
									{
										styleStateValues.pressedValue = valueJson[U"pressed"].get<T>();
									}
									if (valueJson.contains(U"disabled"))
									{
										styleStateValues.disabledValue = valueJson[U"disabled"].get<T>();
									}
									
									(*propertyValue.styleStateValues)[state] = styleStateValues;
								}
								else
								{
									// 文字列の場合はDefaultのみの値
									(*propertyValue.styleStateValues)[state] = PropertyStyleStateValue<T>{ valueJson.get<T>() };
								}
							}
						}
					}
					
					return propertyValue;
				}
				return PropertyValue<T>{ json.getOr<T>(defaultValue) };
			}
		}

		[[nodiscard]]
		PropertyValue<T> withDefault(const T& newDefaultValue) const
		{
			auto value = *this;
			value.defaultValue = newDefaultValue;
			return value;
		}

		template <class U>
		[[nodiscard]]
		PropertyValue<T> withDefault(const U& newDefaultValue) const requires std::convertible_to<U, T>
		{
			auto value = *this;
			value.defaultValue = static_cast<T>(newDefaultValue);
			return value;
		}

		[[nodiscard]]
		PropertyValue<T> withHovered(const T& newHoveredValue) const
		{
			auto value = *this;
			if (value.interactionValues == nullptr)
			{
				value.interactionValues = std::make_unique<PropertyInteractionValues<T>>();
			}
			value.interactionValues->hoveredValue = newHoveredValue;
			return value;
		}

		template <class U>
		[[nodiscard]]
		PropertyValue<T> withHovered(const U& newHoveredValue) const requires std::convertible_to<U, T>
		{
			auto value = *this;
			if (value.interactionValues == nullptr)
			{
				value.interactionValues = std::make_unique<PropertyInteractionValues<T>>();
			}
			value.interactionValues->hoveredValue = static_cast<T>(newHoveredValue);
			return value;
		}

		[[nodiscard]]
		PropertyValue<T> withPressed(const T& newPressedValue) const
		{
			auto value = *this;
			if (value.interactionValues == nullptr)
			{
				value.interactionValues = std::make_unique<PropertyInteractionValues<T>>();
			}
			value.interactionValues->pressedValue = newPressedValue;
			return value;
		}

		template <class U>
		[[nodiscard]]
		PropertyValue<T> withPressed(const U& newPressedValue) const requires std::convertible_to<U, T>
		{
			auto value = *this;
			if (value.interactionValues == nullptr)
			{
				value.interactionValues = std::make_unique<PropertyInteractionValues<T>>();
			}
			value.interactionValues->pressedValue = static_cast<T>(newPressedValue);
			return value;
		}

		[[nodiscard]]
		PropertyValue<T> withDisabled(const T& newDisabledValue) const
		{
			auto value = *this;
			if (value.interactionValues == nullptr)
			{
				value.interactionValues = std::make_unique<PropertyInteractionValues<T>>();
			}
			value.interactionValues->disabledValue = newDisabledValue;
			return value;
		}

		template <class U>
		[[nodiscard]]
		PropertyValue<T> withDisabled(const U& newDisabledValue) const requires std::convertible_to<U, T>
		{
			auto value = *this;
			if (value.interactionValues == nullptr)
			{
				value.interactionValues = std::make_unique<PropertyInteractionValues<T>>();
			}
			value.interactionValues->disabledValue = static_cast<T>(newDisabledValue);
			return value;
		}

		[[nodiscard]]
		PropertyValue<T> withStyleState(const String& styleState, const T& newValue) const
		{
			auto value = *this;
			if (!value.styleStateValues)
			{
				value.styleStateValues = std::make_unique<HashTable<String, PropertyStyleStateValue<T>>>();
			}
			(*value.styleStateValues)[styleState] = PropertyStyleStateValue<T>{ newValue };
			return value;
		}

		template <class U>
		[[nodiscard]]
		PropertyValue<T> withStyleState(const String& styleState, const U& newValue) const requires std::convertible_to<U, T>
		{
			auto value = *this;
			if (!value.styleStateValues)
			{
				value.styleStateValues = std::make_unique<HashTable<String, PropertyStyleStateValue<T>>>();
			}
			(*value.styleStateValues)[styleState] = PropertyStyleStateValue<T>{ static_cast<T>(newValue) };
			return value;
		}

		[[nodiscard]]
		PropertyValue<T> withStyleStateInteraction(const String& styleState, InteractionState interactionState, const T& newValue) const
		{
			auto value = *this;
			if (!value.styleStateValues)
			{
				value.styleStateValues = std::make_unique<HashTable<String, PropertyStyleStateValue<T>>>();
			}
			
			auto it = value.styleStateValues->find(styleState);
			if (it == value.styleStateValues->end())
			{
				// styleStateが存在しない場合、デフォルト値で初期化
				(*value.styleStateValues)[styleState] = PropertyStyleStateValue<T>{ defaultValue };
				it = value.styleStateValues->find(styleState);
			}
			
			switch (interactionState)
			{
			case InteractionState::Default:
				it->second.defaultValue = newValue;
				break;
			case InteractionState::Hovered:
				it->second.hoveredValue = newValue;
				break;
			case InteractionState::Pressed:
				it->second.pressedValue = newValue;
				break;
			case InteractionState::Disabled:
				it->second.disabledValue = newValue;
				break;
			}
			
			return value;
		}

		template <class U>
		[[nodiscard]]
		PropertyValue<T> withStyleStateInteraction(const String& styleState, InteractionState interactionState, const U& newValue) const requires std::convertible_to<U, T>
		{
			auto value = *this;
			if (!value.styleStateValues)
			{
				value.styleStateValues = std::make_unique<HashTable<String, PropertyStyleStateValue<T>>>();
			}
			
			auto it = value.styleStateValues->find(styleState);
			if (it == value.styleStateValues->end())
			{
				// styleStateが存在しない場合、デフォルト値で初期化
				(*value.styleStateValues)[styleState] = PropertyStyleStateValue<T>{ defaultValue };
				it = value.styleStateValues->find(styleState);
			}
			
			switch (interactionState)
			{
			case InteractionState::Default:
				it->second.defaultValue = static_cast<T>(newValue);
				break;
			case InteractionState::Hovered:
				it->second.hoveredValue = static_cast<T>(newValue);
				break;
			case InteractionState::Pressed:
				it->second.pressedValue = static_cast<T>(newValue);
				break;
			case InteractionState::Disabled:
				it->second.disabledValue = static_cast<T>(newValue);
				break;
			}
			
			return value;
		}

		[[nodiscard]]
		PropertyValue<T> withSmoothTime(double newSmoothTime) const
		{
			auto value = *this;
			value.smoothTime = newSmoothTime;
			return value;
		}

		[[nodiscard]]
		String getValueStringOfDefault() const
		{
			if constexpr (std::is_enum_v<T>)
			{
				return EnumToString(defaultValue);
			}
			else
			{
				return Format(defaultValue);
			}
		}

		[[nodiscard]]
		Optional<String> getValueStringOf(InteractionState interactionState, const Array<String>& activeStyleStates) const
		{
			const auto fnGetStr = [](const T& value)
				{
					if constexpr (std::is_enum_v<T>)
					{
						return EnumToString(value);
					}
					else
					{
						return Format(value);
					}
				};

			// styleStateの値をチェック
			if (styleStateValues)
			{
				for (int32 i = static_cast<int32>(activeStyleStates.size()) - 1; i >= 0; --i)
				{
					if (auto it = styleStateValues->find(activeStyleStates[i]); it != styleStateValues->end())
					{
						const PropertyStyleStateValue<T>& styleStateValue = it->second;
						
						// そのstyleState内でInteractionStateのフォールバックを行う
						switch (interactionState)
						{
						case InteractionState::Default:
							return fnGetStr(styleStateValue.defaultValue);
						case InteractionState::Hovered:
							if (styleStateValue.hoveredValue)
							{
								return fnGetStr(*styleStateValue.hoveredValue);
							}
							return fnGetStr(styleStateValue.defaultValue);
						case InteractionState::Pressed:
							if (styleStateValue.pressedValue)
							{
								return fnGetStr(*styleStateValue.pressedValue);
							}
							if (styleStateValue.hoveredValue)
							{
								return fnGetStr(*styleStateValue.hoveredValue);
							}
							return fnGetStr(styleStateValue.defaultValue);
						case InteractionState::Disabled:
							if (styleStateValue.disabledValue)
							{
								return fnGetStr(*styleStateValue.disabledValue);
							}
							return fnGetStr(styleStateValue.defaultValue);
						}
					}
				}
			}
			
			switch (interactionState)
			{
			case InteractionState::Default:
				return fnGetStr(defaultValue);
			case InteractionState::Hovered:
				if (const auto& hoveredVal = hoveredValue())
				{
					return fnGetStr(*hoveredVal);
				}
				break;
			case InteractionState::Pressed:
				if (const auto& pressedVal = pressedValue())
				{
					return fnGetStr(*pressedVal);
				}
				break;
			case InteractionState::Disabled:
				if (const auto& disabledVal = disabledValue())
				{
					return fnGetStr(*disabledVal);
				}
				break;
			}
			
			return none;
		}

		[[nodiscard]]
		String getValueStringOfFallback(InteractionState interactionState, const Array<String>& activeStyleStates) const
		{
			const T v = value(interactionState, activeStyleStates);
			if constexpr (std::is_enum_v<T>)
			{
				return EnumToString(v);
			}
			else
			{
				return Format(v);
			}
		}

		bool trySetValueString(StringView value)
		{
			const Optional<T> parsedValue = StringToValueOpt<T>(value);
			if (!parsedValue)
			{
				return false;
			}
			defaultValue = *parsedValue;
			interactionValues.reset();
			styleStateValues.reset();
			smoothTime = 0.0;
			return true;
		}

		bool trySetValueStringOf(StringView value, InteractionState interactionState, StringView styleState = U"")
		{
			const Optional<T> parsedValue = StringToValueOpt<T>(value);
			if (!parsedValue)
			{
				return false;
			}
			
			if (!styleState.isEmpty())
			{
				if (!styleStateValues)
				{
					styleStateValues = std::make_unique<HashTable<String, PropertyStyleStateValue<T>>>();
				}
				
				auto it = styleStateValues->find(styleState);
				if (it == styleStateValues->end())
				{
					// styleStateが存在しない場合、デフォルト値で初期化
					(*styleStateValues)[styleState] = PropertyStyleStateValue<T>{ defaultValue };
					it = styleStateValues->find(styleState);
				}
				
				switch (interactionState)
				{
				case InteractionState::Default:
					it->second.defaultValue = *parsedValue;
					break;
				case InteractionState::Hovered:
					it->second.hoveredValue = *parsedValue;
					break;
				case InteractionState::Pressed:
					it->second.pressedValue = *parsedValue;
					break;
				case InteractionState::Disabled:
					it->second.disabledValue = *parsedValue;
					break;
				}
				
				return true;
			}
			
			switch (interactionState)
			{
			case InteractionState::Default:
				defaultValue = *parsedValue;
				return true;
			case InteractionState::Hovered:
				if (interactionValues == nullptr)
				{
					interactionValues = std::make_unique<PropertyInteractionValues<T>>();
				}
				interactionValues->hoveredValue = *parsedValue;
				return true;
			case InteractionState::Pressed:
				if (interactionValues == nullptr)
				{
					interactionValues = std::make_unique<PropertyInteractionValues<T>>();
				}
				interactionValues->pressedValue = *parsedValue;
				return true;
			case InteractionState::Disabled:
				if (interactionValues == nullptr)
				{
					interactionValues = std::make_unique<PropertyInteractionValues<T>>();
				}
				interactionValues->disabledValue = *parsedValue;
				return true;
			}
			return false;
		}

		void unsetValueOf(InteractionState interactionState, StringView styleState = U"")
		{
			if (interactionState == InteractionState::Default)
			{
				return; // defaultValueは削除できない
			}

			if (!styleState.isEmpty())
			{
				if (styleStateValues)
				{
					if (auto it = styleStateValues->find(styleState); it != styleStateValues->end())
					{
						switch (interactionState)
						{
						case InteractionState::Default:
							break; // defaultValueは削除できない
						case InteractionState::Hovered:
							it->second.hoveredValue = none;
							break;
						case InteractionState::Pressed:
							it->second.pressedValue = none;
							break;
						case InteractionState::Disabled:
							it->second.disabledValue = none;
							break;
						}
					}
				}
				return;
			}
			
			if (interactionValues != nullptr)
			{
				switch (interactionState)
				{
				case InteractionState::Default:
					break; // defaultValueは削除できない
				case InteractionState::Hovered:
					interactionValues->hoveredValue = none;
					break;
				case InteractionState::Pressed:
					interactionValues->pressedValue = none;
					break;
				case InteractionState::Disabled:
					interactionValues->disabledValue = none;
					break;
				}
			}
		}

		[[nodiscard]]
		bool hasValueOf(InteractionState interactionState, StringView styleState = U"") const
		{
			if (interactionState == InteractionState::Default)
			{
				// defaultValueは常にある
				return true;
			}

			if (!styleState.isEmpty())
			{
				if (styleStateValues)
				{
					if (auto it = styleStateValues->find(styleState); it != styleStateValues->end())
					{
						const PropertyStyleStateValue<T>& styleStateValue = it->second;
						switch (interactionState)
						{
						case InteractionState::Hovered:
							return styleStateValue.hoveredValue.has_value();
						case InteractionState::Pressed:
							return styleStateValue.pressedValue.has_value();
						case InteractionState::Disabled:
							return styleStateValue.disabledValue.has_value();
						default:
							return false;
						}
					}
				}
			}
			
			switch (interactionState)
			{
			case InteractionState::Hovered:
				if (interactionValues != nullptr)
				{
					return interactionValues->hoveredValue.has_value();
				}
				return false;
			case InteractionState::Pressed:
				if (interactionValues != nullptr)
				{
					return interactionValues->pressedValue.has_value();
				}
				return false;
			case InteractionState::Disabled:
				if (interactionValues != nullptr)
				{
					return interactionValues->disabledValue.has_value();
				}
				return false;
			default:
				return false;
			}
		}

		[[nodiscard]]
		bool hasInteractiveValue() const
		{
			return (interactionValues != nullptr && (interactionValues->hoveredValue.has_value() || interactionValues->pressedValue.has_value() || interactionValues->disabledValue.has_value())) ||
				(styleStateValues && !styleStateValues->empty());
		}

		[[nodiscard]]
		bool hasAnyStateEqualTo(const T& value) const
		{
			if (defaultValue == value)
			{
				return true;
			}
			if (interactionValues)
			{
				if ((interactionValues->hoveredValue && *interactionValues->hoveredValue == value) ||
					(interactionValues->pressedValue && *interactionValues->pressedValue == value) ||
					(interactionValues->disabledValue && *interactionValues->disabledValue == value))
				{
					return true;
				}
			}
			if (styleStateValues)
			{
				for (const auto& [key, val] : *styleStateValues)
				{
					if (val.defaultValue == value ||
						(val.hoveredValue && *val.hoveredValue == value) ||
						(val.pressedValue && *val.pressedValue == value) ||
						(val.disabledValue && *val.disabledValue == value))
					{
						return true;
					}
				}
			}
			return false;
		}
	};
}
