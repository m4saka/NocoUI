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
	struct PropertyValue
	{
		T defaultValue;
		Optional<T> hoveredValue = none;
		Optional<T> pressedValue = none;
		Optional<T> disabledValue = none;
		double smoothTime = 0.0;
		
		// styleStateベースのストレージ
		std::unique_ptr<HashTable<String, T>> styleStateValues;
		std::unique_ptr<HashTable<std::pair<String, InteractionState>, T>> styleStateInteractionValues;

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
				styleStateValues = std::make_unique<HashTable<String, T>>(*other.styleStateValues);
			}
			if (other.styleStateInteractionValues)
			{
				styleStateInteractionValues = std::make_unique<HashTable<std::pair<String, InteractionState>, T>>(*other.styleStateInteractionValues);
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
					styleStateValues = std::make_unique<HashTable<String, T>>(*other.styleStateValues);
				}
				else
				{
					styleStateValues.reset();
				}
				
				if (other.styleStateInteractionValues)
				{
					styleStateInteractionValues = std::make_unique<HashTable<std::pair<String, InteractionState>, T>>(*other.styleStateInteractionValues);
				}
				else
				{
					styleStateInteractionValues.reset();
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
			// 優先順位: styleState×InteractionState → styleState → InteractionState → default
			
			// styleState×InteractionStateの組み合わせをチェック（優先順位は配列の後ろから）
			if (styleStateInteractionValues)
			{
				for (int32 i = static_cast<int32>(activeStyleStates.size()) - 1; i >= 0; --i)
				{
					const auto key = std::make_pair(activeStyleStates[i], interactionState);
					if (auto it = styleStateInteractionValues->find(key); it != styleStateInteractionValues->end())
					{
						return it->second;
					}
				}
			}
			
			// styleStateのみの値をチェック（優先順位は配列の後ろから）
			if (styleStateValues)
			{
				for (int32 i = static_cast<int32>(activeStyleStates.size()) - 1; i >= 0; --i)
				{
					if (auto it = styleStateValues->find(activeStyleStates[i]); it != styleStateValues->end())
					{
						return it->second;
					}
				}
			}
			
			// InteractionStateのみの値をチェック
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
			
			// デフォルト値を返す
			return defaultValue;
		}

		[[nodiscard]]
		JSON toJSON() const
		{
			if constexpr (std::is_enum_v<T>)
			{
				if (!hoveredValue && !pressedValue && !disabledValue && smoothTime == 0.0 && 
				    (!styleStateValues || styleStateValues->empty()) && 
				    (!styleStateInteractionValues || styleStateInteractionValues->empty()))
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
				
				// styleState関連の値をシリアライズ
				if ((styleStateValues && !styleStateValues->empty()) || 
				    (styleStateInteractionValues && !styleStateInteractionValues->empty()))
				{
					JSON styleStatesJson;
					
					// 各styleStateごとにグループ化
					HashSet<String> allStates;
					if (styleStateValues)
					{
						for (const auto& [state, value] : *styleStateValues)
						{
							allStates.insert(state);
						}
					}
					if (styleStateInteractionValues)
					{
						for (const auto& [stateInteraction, value] : *styleStateInteractionValues)
						{
							allStates.insert(stateInteraction.first);
						}
					}
					
					// 各stateについて、InteractionStateごとの値を構築
					for (const String& state : allStates)
					{
						bool hasMultipleValues = false;
						JSON stateJson;
						
						// InteractionStateごとの値をチェック
						if (styleStateInteractionValues)
						{
							for (const auto& [stateInteraction, value] : *styleStateInteractionValues)
							{
								if (stateInteraction.first == state)
								{
									hasMultipleValues = true;
									stateJson[EnumToString(stateInteraction.second)] = EnumToString(value);
								}
							}
						}
						
						// stateのみの値（defaultとして扱う）
						if (styleStateValues)
						{
							if (auto it = styleStateValues->find(state); it != styleStateValues->end())
							{
								if (hasMultipleValues)
								{
									stateJson[U"Default"] = EnumToString(it->second);
								}
								else
								{
									// Defaultのみの場合は省略記法として直接保存
									styleStatesJson[state] = EnumToString(it->second);
									continue;
								}
							}
						}
						
						// 複数値がある場合はオブジェクトとして保存
						if (hasMultipleValues)
						{
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
				    (!styleStateValues || styleStateValues->empty()) && 
				    (!styleStateInteractionValues || styleStateInteractionValues->empty()))
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
				
				// styleState関連の値をシリアライズ
				if ((styleStateValues && !styleStateValues->empty()) || 
				    (styleStateInteractionValues && !styleStateInteractionValues->empty()))
				{
					JSON styleStatesJson;
					
					// 各styleStateごとにグループ化
					HashSet<String> allStates;
					if (styleStateValues)
					{
						for (const auto& [state, value] : *styleStateValues)
						{
							allStates.insert(state);
						}
					}
					if (styleStateInteractionValues)
					{
						for (const auto& [stateInteraction, value] : *styleStateInteractionValues)
						{
							allStates.insert(stateInteraction.first);
						}
					}
					
					// 各stateについて、InteractionStateごとの値を構築
					for (const String& state : allStates)
					{
						bool hasOnlyDefault = true;
						JSON stateJson;
						
						// InteractionStateごとの値をチェック
						if (styleStateInteractionValues)
						{
							for (const auto& [stateInteraction, value] : *styleStateInteractionValues)
							{
								if (stateInteraction.first == state)
								{
									hasOnlyDefault = false;
									stateJson[EnumToString(stateInteraction.second)] = value.toJSON();
								}
							}
						}
						
						// stateのみの値（defaultとして扱う）
						if (styleStateValues)
						{
							if (auto it = styleStateValues->find(state); it != styleStateValues->end())
							{
								if (!hasOnlyDefault)
								{
									stateJson[U"Default"] = it->second.toJSON();
								}
								else
								{
									// Defaultのみの場合は省略記法として直接保存
									styleStatesJson[state] = it->second.toJSON();
									continue;
								}
							}
						}
						
						// Default以外の値がある場合はオブジェクトとして保存
						if (!hasOnlyDefault)
						{
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
				    (!styleStateValues || styleStateValues->empty()) && 
				    (!styleStateInteractionValues || styleStateInteractionValues->empty()))
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
				
				// styleState関連の値をシリアライズ
				if ((styleStateValues && !styleStateValues->empty()) || 
				    (styleStateInteractionValues && !styleStateInteractionValues->empty()))
				{
					JSON styleStatesJson;
					
					// 各styleStateごとにグループ化
					HashSet<String> allStates;
					if (styleStateValues)
					{
						for (const auto& [state, value] : *styleStateValues)
						{
							allStates.insert(state);
						}
					}
					if (styleStateInteractionValues)
					{
						for (const auto& [stateInteraction, value] : *styleStateInteractionValues)
						{
							allStates.insert(stateInteraction.first);
						}
					}
					
					// 各stateについて、InteractionStateごとの値を構築
					for (const String& state : allStates)
					{
						bool hasOnlyDefault = true;
						JSON stateJson;
						
						// InteractionStateごとの値をチェック
						if (styleStateInteractionValues)
						{
							for (const auto& [stateInteraction, value] : *styleStateInteractionValues)
							{
								if (stateInteraction.first == state)
								{
									hasOnlyDefault = false;
									stateJson[EnumToString(stateInteraction.second)] = value;
								}
							}
						}
						
						// stateのみの値（defaultとして扱う）
						if (styleStateValues)
						{
							if (auto it = styleStateValues->find(state); it != styleStateValues->end())
							{
								if (!hasOnlyDefault)
								{
									stateJson[U"Default"] = it->second;
								}
								else
								{
									// Defaultのみの場合は省略記法として直接保存
									styleStatesJson[state] = it->second;
									continue;
								}
							}
						}
						
						// Default以外の値がある場合はオブジェクトとして保存
						if (!hasOnlyDefault)
						{
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
					
					// styleState関連の値をデシリアライズ
					if (json.contains(U"styleStates"))
					{
						const JSON& styleStatesJson = json[U"styleStates"];
						if (styleStatesJson.isObject())
						{
							for (const auto& [state, valueJson] : styleStatesJson)
							{
								if (valueJson.isString())
								{
									// 省略記法: 文字列の場合はdefaultの値として扱う
									if (!propertyValue.styleStateValues)
									{
										propertyValue.styleStateValues = std::make_unique<HashTable<String, T>>();
									}
									(*propertyValue.styleStateValues)[state] = StringToEnum<T>(valueJson.getString(), defaultValue);
								}
								else if (valueJson.isObject())
								{
									// 完全記法: InteractionStateごとの値
									for (const auto& [interactionStr, interactionValueJson] : valueJson)
									{
										if (interactionStr == U"Default")
										{
											// stateのみの値
											if (!propertyValue.styleStateValues)
											{
												propertyValue.styleStateValues = std::make_unique<HashTable<String, T>>();
											}
											if (interactionValueJson.isString())
											{
												(*propertyValue.styleStateValues)[state] = StringToEnum<T>(interactionValueJson.getString(), defaultValue);
											}
										}
										else
										{
											// state×InteractionStateの値
											if (!propertyValue.styleStateInteractionValues)
											{
												propertyValue.styleStateInteractionValues = std::make_unique<HashTable<std::pair<String, InteractionState>, T>>();
											}
											const InteractionState interaction = StringToEnum<InteractionState>(interactionStr, InteractionState::Default);
											if (interactionValueJson.isString())
											{
												(*propertyValue.styleStateInteractionValues)[std::make_pair(state, interaction)] = StringToEnum<T>(interactionValueJson.getString(), defaultValue);
											}
										}
									}
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
					
					// styleState関連の値をデシリアライズ
					if (json.contains(U"styleStates"))
					{
						const JSON& styleStatesJson = json[U"styleStates"];
						if (styleStatesJson.isObject())
						{
							for (const auto& [state, valueJson] : styleStatesJson)
							{
								if (valueJson.isObject() && valueJson.contains(U"Default"))
								{
									// 完全記法: InteractionStateごとの値
									for (const auto& [interactionStr, interactionValueJson] : valueJson)
									{
										if (interactionStr == U"Default")
										{
											// stateのみの値
											if (!propertyValue.styleStateValues)
											{
												propertyValue.styleStateValues = std::make_unique<HashTable<String, T>>();
											}
											(*propertyValue.styleStateValues)[state] = T::fromJSON(interactionValueJson, defaultValue);
										}
										else
										{
											// state×InteractionStateの値
											if (!propertyValue.styleStateInteractionValues)
											{
												propertyValue.styleStateInteractionValues = std::make_unique<HashTable<std::pair<String, InteractionState>, T>>();
											}
											const InteractionState interaction = StringToEnum<InteractionState>(interactionStr, InteractionState::Default);
											(*propertyValue.styleStateInteractionValues)[std::make_pair(state, interaction)] = T::fromJSON(interactionValueJson, defaultValue);
										}
									}
								}
								else
								{
									// 省略記法: 値を直接defaultとして扱う
									if (!propertyValue.styleStateValues)
									{
										propertyValue.styleStateValues = std::make_unique<HashTable<String, T>>();
									}
									(*propertyValue.styleStateValues)[state] = T::fromJSON(valueJson, defaultValue);
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
					
					// styleState関連の値をデシリアライズ
					if (json.contains(U"styleStates"))
					{
						const JSON& styleStatesJson = json[U"styleStates"];
						if (styleStatesJson.isObject())
						{
							for (const auto& [state, valueJson] : styleStatesJson)
							{
								if (valueJson.isObject())
								{
									// 完全記法: InteractionStateごとの値
									for (const auto& [interactionStr, interactionValueJson] : valueJson)
									{
										if (interactionStr == U"Default")
										{
											// stateのみの値
											if (!propertyValue.styleStateValues)
											{
												propertyValue.styleStateValues = std::make_unique<HashTable<String, T>>();
											}
											(*propertyValue.styleStateValues)[state] = interactionValueJson.get<T>();
										}
										else
										{
											// state×InteractionStateの値
											if (!propertyValue.styleStateInteractionValues)
											{
												propertyValue.styleStateInteractionValues = std::make_unique<HashTable<std::pair<String, InteractionState>, T>>();
											}
											const InteractionState interaction = StringToEnum<InteractionState>(interactionStr, InteractionState::Default);
											(*propertyValue.styleStateInteractionValues)[std::make_pair(state, interaction)] = interactionValueJson.get<T>();
										}
									}
								}
								else
								{
									// 省略記法: Defaultのみの値として扱う
									if (!propertyValue.styleStateValues)
									{
										propertyValue.styleStateValues = std::make_unique<HashTable<String, T>>();
									}
									(*propertyValue.styleStateValues)[state] = valueJson.get<T>();
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
				value.styleStateValues = std::make_unique<HashTable<String, T>>();
			}
			(*value.styleStateValues)[styleState] = newValue;
			return value;
		}

		template <class U>
		[[nodiscard]]
		PropertyValue<T> withStyleState(const String& styleState, const U& newValue) const requires std::convertible_to<U, T>
		{
			auto value = *this;
			if (!value.styleStateValues)
			{
				value.styleStateValues = std::make_unique<HashTable<String, T>>();
			}
			(*value.styleStateValues)[styleState] = static_cast<T>(newValue);
			return value;
		}

		[[nodiscard]]
		PropertyValue<T> withStyleStateInteraction(const String& styleState, InteractionState interactionState, const T& newValue) const
		{
			auto value = *this;
			if (!value.styleStateInteractionValues)
			{
				value.styleStateInteractionValues = std::make_unique<HashTable<std::pair<String, InteractionState>, T>>();
			}
			(*value.styleStateInteractionValues)[std::make_pair(styleState, interactionState)] = newValue;
			return value;
		}

		template <class U>
		[[nodiscard]]
		PropertyValue<T> withStyleStateInteraction(const String& styleState, InteractionState interactionState, const U& newValue) const requires std::convertible_to<U, T>
		{
			auto value = *this;
			if (!value.styleStateInteractionValues)
			{
				value.styleStateInteractionValues = std::make_unique<HashTable<std::pair<String, InteractionState>, T>>();
			}
			(*value.styleStateInteractionValues)[std::make_pair(styleState, interactionState)] = static_cast<T>(newValue);
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

			// styleState×InteractionStateの組み合わせをチェック
			if (styleStateInteractionValues)
			{
				for (int32 i = static_cast<int32>(activeStyleStates.size()) - 1; i >= 0; --i)
				{
					const auto key = std::make_pair(activeStyleStates[i], interactionState);
					if (auto it = styleStateInteractionValues->find(key); it != styleStateInteractionValues->end())
					{
						return fnGetStr(it->second);
					}
				}
			}
			
			// styleStateのみの値をチェック
			if (styleStateValues)
			{
				for (int32 i = static_cast<int32>(activeStyleStates.size()) - 1; i >= 0; --i)
				{
					if (auto it = styleStateValues->find(activeStyleStates[i]); it != styleStateValues->end())
					{
						return fnGetStr(it->second);
					}
				}
			}
			
			// InteractionStateのみの値をチェック
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
			styleStateInteractionValues.reset();
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
			
			// activeStyleStatesがある場合はstyleState対応の値として設定
			if (!activeStyleStates.empty())
			{
				const String& styleState = activeStyleStates.back(); // 最も優先度の高いstyleState
				
				if (!styleStateInteractionValues)
				{
					styleStateInteractionValues = std::make_unique<HashTable<std::pair<String, InteractionState>, T>>();
				}
				(*styleStateInteractionValues)[std::make_pair(styleState, interactionState)] = *parsedValue;
				return true;
			}
			
			// activeStyleStatesがない場合は通常のInteractionState値として設定
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
			// activeStyleStatesがある場合はstyleState対応の値を削除
			if (!activeStyleStates.empty())
			{
				const String& styleState = activeStyleStates.back();
				
				if (styleStateInteractionValues)
				{
					const auto key = std::make_pair(styleState, interactionState);
					if (auto it = styleStateInteractionValues->find(key); it != styleStateInteractionValues->end())
					{
						styleStateInteractionValues->erase(it);
						return true;
					}
				}
				return false;
			}
			
			// activeStyleStatesがない場合は通常のInteractionState値を削除
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
			// activeStyleStatesがある場合はstyleState対応の値をチェック
			if (!activeStyleStates.empty())
			{
				const String& styleState = activeStyleStates.back();
				
				if (styleStateInteractionValues)
				{
					const auto key = std::make_pair(styleState, interactionState);
					if (styleStateInteractionValues->contains(key))
					{
						return true;
					}
				}
			}
			
			// 通常のInteractionState値をチェック
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

		bool hasInteractiveValue() const
		{
			return hoveredValue.has_value() ||
				pressedValue.has_value() ||
				disabledValue.has_value() ||
				(styleStateValues && !styleStateValues->empty()) ||
				(styleStateInteractionValues && !styleStateInteractionValues->empty());
		}
	};
}
