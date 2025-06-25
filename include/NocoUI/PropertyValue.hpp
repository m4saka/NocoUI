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
	struct InteractionValues
	{
		T defaultValue;
		Optional<T> hoveredValue = none;
		Optional<T> pressedValue = none;
		Optional<T> disabledValue = none;
		
		/*implicit*/ InteractionValues(const T& defaultValue)
			: defaultValue{ static_cast<T>(defaultValue) }
		{
		}
		
		InteractionValues() = default;
		InteractionValues(const InteractionValues&) = default;
		InteractionValues& operator=(const InteractionValues&) = default;
		InteractionValues(InteractionValues&&) noexcept = default;
		InteractionValues& operator=(InteractionValues&&) noexcept = default;
	};
	
	template <class T>
	struct PropertyValue
	{
		T defaultValue;
		Optional<T> hoveredValue = none;
		Optional<T> pressedValue = none;
		Optional<T> disabledValue = none;
		double smoothTime = 0.0;
		
		std::unique_ptr<HashTable<String, InteractionValues<T>>> styleStateValues;

		/*implicit*/ PropertyValue(const T& defaultValue)
			: defaultValue{ static_cast<T>(defaultValue) }
		{
		}
		
		PropertyValue(const PropertyValue& other)
			: defaultValue{ other.defaultValue }
			, hoveredValue{ other.hoveredValue }
			, pressedValue{ other.pressedValue }
			, disabledValue{ other.disabledValue }
			, smoothTime{ other.smoothTime }
		{
			if (other.styleStateValues)
			{
				styleStateValues = std::make_unique<HashTable<String, InteractionValues<T>>>(*other.styleStateValues);
			}
		}
		
		PropertyValue& operator=(const PropertyValue& other)
		{
			if (this != &other)
			{
				defaultValue = other.defaultValue;
				hoveredValue = other.hoveredValue;
				pressedValue = other.pressedValue;
				disabledValue = other.disabledValue;
				smoothTime = other.smoothTime;
				
				if (other.styleStateValues)
				{
					styleStateValues = std::make_unique<HashTable<String, InteractionValues<T>>>(*other.styleStateValues);
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
		/*implicit*/ PropertyValue(const U& defaultValue) requires std::convertible_to<U, T>
			: defaultValue{ static_cast<T>(defaultValue) }
		{
		}

		/*implicit*/ PropertyValue(StringView defaultValue) requires std::same_as<T, String>
			: defaultValue{ static_cast<T>(String{ defaultValue }) }
		{
		}

		PropertyValue(const T& defaultValue, const Optional<T>& hoveredValue, const Optional<T>& pressedValue, const Optional<T>& disabledValue, double smoothTime = 0.0)
			: defaultValue{ defaultValue }
			, hoveredValue{ hoveredValue }
			, pressedValue{ pressedValue }
			, disabledValue{ disabledValue }
			, smoothTime{ smoothTime }
		{
		}

		[[nodiscard]]
		const T& value(InteractionState interactionState, const Array<String>& activeStyleStates) const
		{
			// activeStyleStatesがある場合は、優先度の高い順にstyleStateの一致を調べる
			if (!activeStyleStates.empty() && styleStateValues)
			{
				// 優先度の高い順にチェック(配列の末尾から)
				for (int32 i = static_cast<int32>(activeStyleStates.size()) - 1; i >= 0; --i)
				{
					const String& currentStyleState = activeStyleStates[i];
					
					// styleStateのInteractionValuesを取得
					if (auto it = styleStateValues->find(currentStyleState); it != styleStateValues->end())
					{
						const InteractionValues<T>& values = it->second;
						
						// そのstyleState内でInteractionStateのフォールバックを行う
						switch (interactionState)
						{
						case InteractionState::Default:
							return values.defaultValue;
						case InteractionState::Hovered:
							// Hovered → Default
							if (values.hoveredValue)
							{
								return *values.hoveredValue;
							}
							return values.defaultValue;
						case InteractionState::Pressed:
							// Pressed → Hovered → Default
							if (values.pressedValue)
							{
								return *values.pressedValue;
							}
							if (values.hoveredValue)
							{
								return *values.hoveredValue;
							}
							return values.defaultValue;
						case InteractionState::Disabled:
							// Disabled → Default
							if (values.disabledValue)
							{
								return *values.disabledValue;
							}
							return values.defaultValue;
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
				if (hoveredValue)
				{
					return *hoveredValue;
				}
				break;
			case InteractionState::Pressed:
				if (pressedValue)
				{
					return *pressedValue;
				}
				if (hoveredValue)
				{
					return *hoveredValue;
				}
				break;
			case InteractionState::Disabled:
				if (disabledValue)
				{
					return *disabledValue;
				}
				break;
			}
			
			return defaultValue;
		}

		[[nodiscard]]
		JSON toJSON() const
		{
			if constexpr (std::is_enum_v<T>)
			{
				if (!hoveredValue && !pressedValue && !disabledValue && smoothTime == 0.0 && 
				    (!styleStateValues || styleStateValues->empty()))
				{
					return EnumToString(defaultValue);
				}
				JSON json;
				json[U"default"] = EnumToString(defaultValue);
				if (hoveredValue)
				{
					json[U"hovered"] = EnumToString(*hoveredValue);
				}
				if (pressedValue)
				{
					json[U"pressed"] = EnumToString(*pressedValue);
				}
				if (disabledValue)
				{
					json[U"disabled"] = EnumToString(*disabledValue);
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
							stateJson[U"Default"] = EnumToString(values.defaultValue);
							if (values.hoveredValue)
							{
								stateJson[U"Hovered"] = EnumToString(*values.hoveredValue);
							}
							if (values.pressedValue)
							{
								stateJson[U"Pressed"] = EnumToString(*values.pressedValue);
							}
							if (values.disabledValue)
							{
								stateJson[U"Disabled"] = EnumToString(*values.disabledValue);
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
				if (!hoveredValue && !pressedValue && !disabledValue && smoothTime == 0.0 && 
				    (!styleStateValues || styleStateValues->empty()))
				{
					return defaultValue.toJSON();
				}
				JSON json;
				json[U"default"] = defaultValue.toJSON();
				if (hoveredValue)
				{
					json[U"hovered"] = hoveredValue->toJSON();
				}
				if (pressedValue)
				{
					json[U"pressed"] = pressedValue->toJSON();
				}
				if (disabledValue)
				{
					json[U"disabled"] = disabledValue->toJSON();
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
							stateJson[U"Default"] = values.defaultValue.toJSON();
							if (values.hoveredValue)
							{
								stateJson[U"Hovered"] = values.hoveredValue->toJSON();
							}
							if (values.pressedValue)
							{
								stateJson[U"Pressed"] = values.pressedValue->toJSON();
							}
							if (values.disabledValue)
							{
								stateJson[U"Disabled"] = values.disabledValue->toJSON();
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
				if (!hoveredValue && !pressedValue && !disabledValue && smoothTime == 0.0 && 
				    (!styleStateValues || styleStateValues->empty()))
				{
					return defaultValue;
				}
				JSON json;
				json[U"default"] = defaultValue;
				if (hoveredValue)
				{
					json[U"hovered"] = *hoveredValue;
				}
				if (pressedValue)
				{
					json[U"pressed"] = *pressedValue;
				}
				if (disabledValue)
				{
					json[U"disabled"] = *disabledValue;
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
							stateJson[U"Default"] = values.defaultValue;
							if (values.hoveredValue)
							{
								stateJson[U"Hovered"] = *values.hoveredValue;
							}
							if (values.pressedValue)
							{
								stateJson[U"Pressed"] = *values.pressedValue;
							}
							if (values.disabledValue)
							{
								stateJson[U"Disabled"] = *values.disabledValue;
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
							propertyValue.styleStateValues = std::make_unique<HashTable<String, InteractionValues<T>>>();
							
							for (const auto& [state, valueJson] : styleStatesJson)
							{
								if (valueJson.isString())
								{
									// 文字列の場合はDefaultのみ
									InteractionValues<T> values{ StringToEnum<T>(valueJson.getString(), defaultValue) };
									(*propertyValue.styleStateValues)[state] = values;
								}
								else if (valueJson.isObject())
								{
									// オブジェクトの場合はInteractionStateごとの値がある
									InteractionValues<T> values{ defaultValue };
									
									if (valueJson.contains(U"Default"))
									{
										values.defaultValue = StringToEnum<T>(valueJson[U"Default"].getString(), defaultValue);
									}
									if (valueJson.contains(U"Hovered"))
									{
										values.hoveredValue = StringToEnum<T>(valueJson[U"Hovered"].getString(), defaultValue);
									}
									if (valueJson.contains(U"Pressed"))
									{
										values.pressedValue = StringToEnum<T>(valueJson[U"Pressed"].getString(), defaultValue);
									}
									if (valueJson.contains(U"Disabled"))
									{
										values.disabledValue = StringToEnum<T>(valueJson[U"Disabled"].getString(), defaultValue);
									}
									
									(*propertyValue.styleStateValues)[state] = values;
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
								propertyValue.styleStateValues = std::make_unique<HashTable<String, InteractionValues<T>>>();
							}
							
							for (const auto& [state, valueJson] : styleStatesJson)
							{
								if (valueJson.isObject() && valueJson.contains(U"Default"))
								{
									// オブジェクトの場合はInteractionStateごとの値がある
									InteractionValues<T> interactionValues{ T::fromJSON(valueJson[U"Default"], defaultValue) };
									
									if (valueJson.contains(U"Hovered"))
									{
										interactionValues.hoveredValue = T::fromJSON(valueJson[U"Hovered"], defaultValue);
									}
									if (valueJson.contains(U"Pressed"))
									{
										interactionValues.pressedValue = T::fromJSON(valueJson[U"Pressed"], defaultValue);
									}
									if (valueJson.contains(U"Disabled"))
									{
										interactionValues.disabledValue = T::fromJSON(valueJson[U"Disabled"], defaultValue);
									}
									
									(*propertyValue.styleStateValues)[state] = interactionValues;
								}
								else
								{
									// 文字列の場合はDefaultのみの値
									(*propertyValue.styleStateValues)[state] = InteractionValues<T>{ T::fromJSON(valueJson, defaultValue) };
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
								propertyValue.styleStateValues = std::make_unique<HashTable<String, InteractionValues<T>>>();
							}
							
							for (const auto& [state, valueJson] : styleStatesJson)
							{
								if (valueJson.isObject() && valueJson.contains(U"Default"))
								{
									// オブジェクトの場合はInteractionStateごとの値がある
									InteractionValues<T> interactionValues{ valueJson[U"Default"].get<T>() };
									
									if (valueJson.contains(U"Hovered"))
									{
										interactionValues.hoveredValue = valueJson[U"Hovered"].get<T>();
									}
									if (valueJson.contains(U"Pressed"))
									{
										interactionValues.pressedValue = valueJson[U"Pressed"].get<T>();
									}
									if (valueJson.contains(U"Disabled"))
									{
										interactionValues.disabledValue = valueJson[U"Disabled"].get<T>();
									}
									
									(*propertyValue.styleStateValues)[state] = interactionValues;
								}
								else
								{
									// 文字列の場合はDefaultのみの値
									(*propertyValue.styleStateValues)[state] = InteractionValues<T>{ valueJson.get<T>() };
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
			value.hoveredValue = newHoveredValue;
			return value;
		}

		template <class U>
		[[nodiscard]]
		PropertyValue<T> withHovered(const U& newHoveredValue) const requires std::convertible_to<U, T>
		{
			auto value = *this;
			value.hoveredValue = static_cast<T>(newHoveredValue);
			return value;
		}

		[[nodiscard]]
		PropertyValue<T> withPressed(const T& newPressedValue) const
		{
			auto value = *this;
			value.pressedValue = newPressedValue;
			return value;
		}

		template <class U>
		[[nodiscard]]
		PropertyValue<T> withPressed(const U& newPressedValue) const requires std::convertible_to<U, T>
		{
			auto value = *this;
			value.pressedValue = static_cast<T>(newPressedValue);
			return value;
		}

		[[nodiscard]]
		PropertyValue<T> withDisabled(const T& newDisabledValue) const
		{
			auto value = *this;
			value.disabledValue = newDisabledValue;
			return value;
		}

		template <class U>
		[[nodiscard]]
		PropertyValue<T> withDisabled(const U& newDisabledValue) const requires std::convertible_to<U, T>
		{
			auto value = *this;
			value.disabledValue = static_cast<T>(newDisabledValue);
			return value;
		}

		[[nodiscard]]
		PropertyValue<T> withStyleState(const String& styleState, const T& newValue) const
		{
			auto value = *this;
			if (!value.styleStateValues)
			{
				value.styleStateValues = std::make_unique<HashTable<String, InteractionValues<T>>>();
			}
			(*value.styleStateValues)[styleState] = InteractionValues<T>{ newValue };
			return value;
		}

		template <class U>
		[[nodiscard]]
		PropertyValue<T> withStyleState(const String& styleState, const U& newValue) const requires std::convertible_to<U, T>
		{
			auto value = *this;
			if (!value.styleStateValues)
			{
				value.styleStateValues = std::make_unique<HashTable<String, InteractionValues<T>>>();
			}
			(*value.styleStateValues)[styleState] = InteractionValues<T>{ static_cast<T>(newValue) };
			return value;
		}

		[[nodiscard]]
		PropertyValue<T> withStyleStateInteraction(const String& styleState, InteractionState interactionState, const T& newValue) const
		{
			auto value = *this;
			if (!value.styleStateValues)
			{
				value.styleStateValues = std::make_unique<HashTable<String, InteractionValues<T>>>();
			}
			
			auto it = value.styleStateValues->find(styleState);
			if (it == value.styleStateValues->end())
			{
				// styleStateが存在しない場合、デフォルト値で初期化
				(*value.styleStateValues)[styleState] = InteractionValues<T>{ defaultValue };
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
				value.styleStateValues = std::make_unique<HashTable<String, InteractionValues<T>>>();
			}
			
			auto it = value.styleStateValues->find(styleState);
			if (it == value.styleStateValues->end())
			{
				// styleStateが存在しない場合、デフォルト値で初期化
				(*value.styleStateValues)[styleState] = InteractionValues<T>{ defaultValue };
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
						const InteractionValues<T>& values = it->second;
						
						// そのstyleState内でInteractionStateのフォールバックを行う
						switch (interactionState)
						{
						case InteractionState::Default:
							return fnGetStr(values.defaultValue);
						case InteractionState::Hovered:
							if (values.hoveredValue)
							{
								return fnGetStr(*values.hoveredValue);
							}
							return fnGetStr(values.defaultValue);
						case InteractionState::Pressed:
							if (values.pressedValue)
							{
								return fnGetStr(*values.pressedValue);
							}
							if (values.hoveredValue)
							{
								return fnGetStr(*values.hoveredValue);
							}
							return fnGetStr(values.defaultValue);
						case InteractionState::Disabled:
							if (values.disabledValue)
							{
								return fnGetStr(*values.disabledValue);
							}
							return fnGetStr(values.defaultValue);
						}
					}
				}
			}
			
			switch (interactionState)
			{
			case InteractionState::Default:
				return fnGetStr(defaultValue);
			case InteractionState::Hovered:
				if (hoveredValue)
				{
					return fnGetStr(*hoveredValue);
				}
				break;
			case InteractionState::Pressed:
				if (pressedValue)
				{
					return fnGetStr(*pressedValue);
				}
				break;
			case InteractionState::Disabled:
				if (disabledValue)
				{
					return fnGetStr(*disabledValue);
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
			hoveredValue = none;
			pressedValue = none;
			disabledValue = none;
			styleStateValues.reset();
			smoothTime = 0.0;
			return true;
		}

		bool trySetValueStringOf(StringView value, InteractionState interactionState, const Array<String>& activeStyleStates)
		{
			const Optional<T> parsedValue = StringToValueOpt<T>(value);
			if (!parsedValue)
			{
				return false;
			}
			
			if (!activeStyleStates.empty())
			{
				const String& styleState = activeStyleStates.back(); // 最も優先度の高いstyleState
				
				if (!styleStateValues)
				{
					styleStateValues = std::make_unique<HashTable<String, InteractionValues<T>>>();
				}
				
				auto it = styleStateValues->find(styleState);
				if (it == styleStateValues->end())
				{
					// styleStateが存在しない場合、デフォルト値で初期化
					(*styleStateValues)[styleState] = InteractionValues<T>{ defaultValue };
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
				hoveredValue = *parsedValue;
				return true;
			case InteractionState::Pressed:
				pressedValue = *parsedValue;
				return true;
			case InteractionState::Disabled:
				disabledValue = *parsedValue;
				return true;
			}
			return false;
		}

		bool tryUnsetValueOf(InteractionState interactionState, const Array<String>& activeStyleStates)
		{
			if (!activeStyleStates.empty())
			{
				const String& styleState = activeStyleStates.back();
				
				if (styleStateValues)
				{
					if (auto it = styleStateValues->find(styleState); it != styleStateValues->end())
					{
						switch (interactionState)
						{
						case InteractionState::Default:
							return false; // defaultValueは削除できない
						case InteractionState::Hovered:
							it->second.hoveredValue = none;
							return true;
						case InteractionState::Pressed:
							it->second.pressedValue = none;
							return true;
						case InteractionState::Disabled:
							it->second.disabledValue = none;
							return true;
						}
					}
				}
				return false;
			}
			
			switch (interactionState)
			{
			case InteractionState::Default:
				return false; // defaultValueは削除できない
			case InteractionState::Hovered:
				hoveredValue = none;
				return true;
			case InteractionState::Pressed:
				pressedValue = none;
				return true;
			case InteractionState::Disabled:
				disabledValue = none;
				return true;
			}
			return false;
		}

		[[nodiscard]]
		bool hasValueOf(InteractionState interactionState, const Array<String>& activeStyleStates) const
		{
			if (!activeStyleStates.empty())
			{
				const String& styleState = activeStyleStates.back();
				
				if (styleStateValues)
				{
					if (auto it = styleStateValues->find(styleState); it != styleStateValues->end())
					{
						const InteractionValues<T>& values = it->second;
						switch (interactionState)
						{
						case InteractionState::Default:
							return true; // defaultValueは常にある
						case InteractionState::Hovered:
							return values.hoveredValue.has_value();
						case InteractionState::Pressed:
							return values.pressedValue.has_value();
						case InteractionState::Disabled:
							return values.disabledValue.has_value();
						}
					}
				}
			}
			
			switch (interactionState)
			{
			case InteractionState::Default:
				return true; // defaultValueは常にある
			case InteractionState::Hovered:
				return hoveredValue.has_value();
			case InteractionState::Pressed:
				return pressedValue.has_value();
			case InteractionState::Disabled:
				return disabledValue.has_value();
			}
			return false;
		}

		[[nodiscard]]
		bool hasInteractiveValue() const
		{
			return hoveredValue.has_value() ||
				pressedValue.has_value() ||
				disabledValue.has_value() ||
				(styleStateValues && !styleStateValues->empty());
		}

		[[nodiscard]]
		bool hasAnyStateEqualTo(const T& value) const
		{
			if (defaultValue == value)
			{
				return true;
			}
			if (hoveredValue && *hoveredValue == value)
			{
				return true;
			}
			if (pressedValue && *pressedValue == value)
			{
				return true;
			}
			if (disabledValue && *disabledValue == value)
			{
				return true;
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
