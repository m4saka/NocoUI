#pragma once
#include <Siv3D.hpp>
#include "YN.hpp"
#include "InteractionState.hpp"
#include "Serialization.hpp"
#include "LRTB.hpp"

namespace noco
{
	enum class TweenType
	{
		Linear,
		Sine0_1,
		Triangle,
		Square,
		SawTooth,
		EaseIn,
		EaseOut,
		EaseInOut
	};


	template <class T>
	struct TweenValue
	{
		T value1{};
		T value2{};
		TweenType type = TweenType::Linear;
		double duration = 1.0;
		double delay = 0.0;  // Tween開始までの遅延
		bool loop = false;   // ループするかどうか
		bool retrigger = true;  // 状態変更時に再開始するかどうか

		[[nodiscard]]
		T calculateValue(double time) const
		{

			// 遅延時間中はvalue1を返す
			if (time < delay)
			{
				return value1;
			}

			// 遅延時間を考慮した経過時間
			const double elapsedTime = time - delay;
			
			double normalizedTime;
			if (loop)
			{
				// ループモード: durationで剰余を取る
				normalizedTime = Math::Fmod(elapsedTime, duration) / duration;
			}
			else
			{
				// 非ループモード: 0.0〜1.0でクランプ
				normalizedTime = Math::Clamp(elapsedTime / duration, 0.0, 1.0);
			}

			double progress = 0.0;

			switch (type)
			{
			case TweenType::Linear:
				progress = normalizedTime;
				break;
			case TweenType::Sine0_1:
				progress = (Math::Sin(normalizedTime * Math::TwoPi - Math::HalfPi) + 1.0) * 0.5;
				break;
			case TweenType::Triangle:
				progress = normalizedTime < 0.5 ? normalizedTime * 2.0 : (1.0 - normalizedTime) * 2.0;
				break;
			case TweenType::Square:
				progress = normalizedTime < 0.5 ? 0.0 : 1.0;
				break;
			case TweenType::SawTooth:
				progress = normalizedTime;
				break;
			case TweenType::EaseIn:
				progress = normalizedTime * normalizedTime;
				break;
			case TweenType::EaseOut:
				progress = 1.0 - (1.0 - normalizedTime) * (1.0 - normalizedTime);
				break;
			case TweenType::EaseInOut:
				progress = normalizedTime < 0.5 
					? 2.0 * normalizedTime * normalizedTime 
					: 1.0 - Math::Pow(-2.0 * normalizedTime + 2.0, 2) / 2.0;
				break;
			}

			if constexpr (std::is_arithmetic_v<T>)
			{
				return static_cast<T>(value1 + (value2 - value1) * progress);
			}
			else if constexpr (std::same_as<T, Vec2>)
			{
				return value1.lerp(value2, progress);
			}
			else if constexpr (std::same_as<T, ColorF>)
			{
				return value1.lerp(value2, progress);
			}
			else if constexpr (std::same_as<T, LRTB>)
			{
				return LRTB{
					value1.left + (value2.left - value1.left) * progress,
					value1.right + (value2.right - value1.right) * progress,
					value1.top + (value2.top - value1.top) * progress,
					value1.bottom + (value2.bottom - value1.bottom) * progress
				};
			}
			else
			{
				return progress < 0.5 ? value1 : value2;
			}
		}

		[[nodiscard]]
		JSON toJSON() const
		{
			JSON json;
			// value1とvalue2は必ず文字列として保存
			json[U"value1"] = ValueToString(value1);
			json[U"value2"] = ValueToString(value2);
			json[U"type"] = EnumToString(type);
			json[U"duration"] = duration;
			json[U"delay"] = delay;
			json[U"loop"] = loop;
			json[U"retrigger"] = retrigger;
			return json;
		}

		[[nodiscard]]
		static TweenValue<T> fromJSON(const JSON& json, const T& defaultValue1 = T{}, const T& defaultValue2 = T{})
		{
			TweenValue<T> result;
			
			// value1とvalue2は文字列として保存される
			if (json.contains(U"value1"))
			{
				result.value1 = StringToValueOpt<T>(json[U"value1"].getOr<String>(U"")).value_or(defaultValue1);
			}
			else
			{
				result.value1 = defaultValue1;
			}
			
			if (json.contains(U"value2"))
			{
				result.value2 = StringToValueOpt<T>(json[U"value2"].getOr<String>(U"")).value_or(defaultValue2);
			}
			else
			{
				result.value2 = defaultValue2;
			}
			
			result.type = GetFromJSONOr(json, U"type", TweenType::Linear);
			result.duration = GetFromJSONOr(json, U"duration", 1.0);
			result.delay = GetFromJSONOr(json, U"delay", 0.0);
			result.loop = GetFromJSONOr(json, U"loop", false);
			result.retrigger = GetFromJSONOr(json, U"retrigger", true);
			return result;
		}
	};

	template <class T>
	struct PropertyValue
	{
		T defaultValue;
		Optional<T> hoveredValue = none;
		Optional<T> pressedValue = none;
		Optional<T> disabledValue = none;
		Optional<T> selectedDefaultValue = none;
		Optional<T> selectedHoveredValue = none;
		Optional<T> selectedPressedValue = none;
		Optional<T> selectedDisabledValue = none;
		double smoothTime = 0.0;

		/*implicit*/ PropertyValue(const T& defaultValue)
			: defaultValue{ static_cast<T>(defaultValue) }
		{
		}

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
		const T& value(InteractionState interactionState, SelectedYN selected) const
		{
			if (selected)
			{
				switch (interactionState)
				{
				case InteractionState::Default:
					if (selectedDefaultValue)
					{
						return *selectedDefaultValue;
					}
					break;
				case InteractionState::Hovered:
					if (selectedHoveredValue)
					{
						return *selectedHoveredValue;
					}
					if (selectedDefaultValue)
					{
						return *selectedDefaultValue;
					}
					break;
				case InteractionState::Pressed:
					if (selectedPressedValue)
					{
						return *selectedPressedValue;
					}
					if (selectedHoveredValue)
					{
						return *selectedHoveredValue;
					}
					if (selectedDefaultValue)
					{
						return *selectedDefaultValue;
					}
					break;
				case InteractionState::Disabled:
					if (selectedDisabledValue)
					{
						return *selectedDisabledValue;
					}
					break;
				}
			}

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
				if (!hoveredValue && !pressedValue && !disabledValue && smoothTime == 0.0)
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
				if (selectedDefaultValue)
				{
					json[U"selectedDefault"] = EnumToString(*selectedDefaultValue);
				}
				if (selectedHoveredValue)
				{
					json[U"selectedHovered"] = EnumToString(*selectedHoveredValue);
				}
				if (selectedPressedValue)
				{
					json[U"selectedPressed"] = EnumToString(*selectedPressedValue);
				}
				if (selectedDisabledValue)
				{
					json[U"selectedDisabled"] = EnumToString(*selectedDisabledValue);
				}
				if (smoothTime != 0.0)
				{
					json[U"smoothTime"] = smoothTime;
				}
				return json;
			}
			else if constexpr (HasToJSON<T>)
			{
				if (!hoveredValue && !pressedValue && !disabledValue && smoothTime == 0.0)
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
				if (selectedDefaultValue)
				{
					json[U"selectedDefault"] = selectedDefaultValue->toJSON();
				}
				if (selectedHoveredValue)
				{
					json[U"selectedHovered"] = selectedHoveredValue->toJSON();
				}
				if (selectedPressedValue)
				{
					json[U"selectedPressed"] = selectedPressedValue->toJSON();
				}
				if (selectedDisabledValue)
				{
					json[U"selectedDisabled"] = selectedDisabledValue->toJSON();
				}
				if (smoothTime != 0.0)
				{
					json[U"smoothTime"] = smoothTime;
				}
				return json;
			}
			else
			{
				if (!hoveredValue && !pressedValue && !disabledValue && smoothTime == 0.0)
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
				if (selectedDefaultValue)
				{
					json[U"selectedDefault"] = *selectedDefaultValue;
				}
				if (selectedHoveredValue)
				{
					json[U"selectedHovered"] = *selectedHoveredValue;
				}
				if (selectedPressedValue)
				{
					json[U"selectedPressed"] = *selectedPressedValue;
				}
				if (selectedDisabledValue)
				{
					json[U"selectedDisabled"] = *selectedDisabledValue;
				}
				if (smoothTime != 0.0)
				{
					json[U"smoothTime"] = smoothTime;
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
					propertyValue.selectedDefaultValue = GetFromJSONOpt<T>(json, U"selectedDefault");
					propertyValue.selectedHoveredValue = GetFromJSONOpt<T>(json, U"selectedHovered");
					propertyValue.selectedPressedValue = GetFromJSONOpt<T>(json, U"selectedPressed");
					propertyValue.selectedDisabledValue = GetFromJSONOpt<T>(json, U"selectedDisabled");
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
					if (json.contains(U"selectedDefault"))
					{
						propertyValue.selectedDefaultValue = T::fromJSON(json[U"selectedDefault"], defaultValue);
					}
					if (json.contains(U"selectedHovered"))
					{
						propertyValue.selectedHoveredValue = T::fromJSON(json[U"selectedHovered"], defaultValue);
					}
					if (json.contains(U"selectedPressed"))
					{
						propertyValue.selectedPressedValue = T::fromJSON(json[U"selectedPressed"], defaultValue);
					}
					if (json.contains(U"selectedDisabled"))
					{
						propertyValue.selectedDisabledValue = T::fromJSON(json[U"selectedDisabled"], defaultValue);
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
					propertyValue.selectedDefaultValue = GetFromJSONOpt<T>(json, U"selectedDefault");
					propertyValue.selectedHoveredValue = GetFromJSONOpt<T>(json, U"selectedHovered");
					propertyValue.selectedPressedValue = GetFromJSONOpt<T>(json, U"selectedPressed");
					propertyValue.selectedDisabledValue = GetFromJSONOpt<T>(json, U"selectedDisabled");
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
		PropertyValue<T> withSelectedDefault(const T& newSelectedDefaultValue) const
		{
			auto value = *this;
			value.selectedDefaultValue = newSelectedDefaultValue;
			return value;
		}

		template <class U>
		[[nodiscard]]
		PropertyValue<T> withSelectedDefault(const U& newSelectedDefaultValue) const requires std::convertible_to<U, T>
		{
			auto value = *this;
			value.selectedDefaultValue = static_cast<T>(newSelectedDefaultValue);
			return value;
		}

		[[nodiscard]]
		PropertyValue<T> withSelectedHovered(const T& newSelectedHoveredValue) const
		{
			auto value = *this;
			value.selectedHoveredValue = newSelectedHoveredValue;
			return value;
		}

		template <class U>
		[[nodiscard]]
		PropertyValue<T> withSelectedHovered(const U& newSelectedHoveredValue) const requires std::convertible_to<U, T>
		{
			auto value = *this;
			value.selectedHoveredValue = static_cast<T>(newSelectedHoveredValue);
			return value;
		}

		[[nodiscard]]
		PropertyValue<T> withSelectedPressed(const T& newSelectedPressedValue) const
		{
			auto value = *this;
			value.selectedPressedValue = newSelectedPressedValue;
			return value;
		}

		template <class U>
		[[nodiscard]]
		PropertyValue<T> withSelectedPressed(const U& newSelectedPressedValue) const requires std::convertible_to<U, T>
		{
			auto value = *this;
			value.selectedPressedValue = static_cast<T>(newSelectedPressedValue);
			return value;
		}

		[[nodiscard]]
		PropertyValue<T> withSelectedDisabled(const T& newSelectedDisabledValue) const
		{
			auto value = *this;
			value.selectedDisabledValue = newSelectedDisabledValue;
			return value;
		}

		template <class U>
		[[nodiscard]]
		PropertyValue<T> withSelectedDisabled(const U& newSelectedDisabledValue) const requires std::convertible_to<U, T>
		{
			auto value = *this;
			value.selectedDisabledValue = static_cast<T>(newSelectedDisabledValue);
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
		Optional<String> getValueStringOf(InteractionState interactionState, SelectedYN selected) const
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

			if (selected)
			{
				switch (interactionState)
				{
				case InteractionState::Default:
					if (selectedDefaultValue)
					{
						return fnGetStr(*selectedDefaultValue);
					}
					break;
				case InteractionState::Hovered:
					if (selectedHoveredValue)
					{
						return fnGetStr(*selectedHoveredValue);
					}
					break;
				case InteractionState::Pressed:
					if (selectedPressedValue)
					{
						return fnGetStr(*selectedPressedValue);
					}
					break;
				case InteractionState::Disabled:
					if (selectedDisabledValue)
					{
						return fnGetStr(*selectedDisabledValue);
					}
					break;
				}
			}
			else
			{
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
			}
			return none;
		}

		[[nodiscard]]
		String getValueStringOfFallback(InteractionState interactionState, SelectedYN selected) const
		{
			const T v = value(interactionState, selected);
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
			selectedDefaultValue = none;
			selectedHoveredValue = none;
			selectedPressedValue = none;
			selectedDisabledValue = none;
			smoothTime = 0.0;
			return true;
		}

		bool trySetValueStringOf(StringView value, InteractionState interactionState, SelectedYN selected)
		{
			if (selected)
			{
				switch (interactionState)
				{
				case InteractionState::Default:
					if (const Optional<T> parsedValue = StringToValueOpt<T>(value))
					{
						selectedDefaultValue = *parsedValue;
						return true;
					}
					break;
				case InteractionState::Hovered:
					if (const Optional<T> parsedValue = StringToValueOpt<T>(value))
					{
						selectedHoveredValue = *parsedValue;
						return true;
					}
					break;
				case InteractionState::Pressed:
					if (const Optional<T> parsedValue = StringToValueOpt<T>(value))
					{
						selectedPressedValue = *parsedValue;
						return true;
					}
					break;
				case InteractionState::Disabled:
					if (const Optional<T> parsedValue = StringToValueOpt<T>(value))
					{
						selectedDisabledValue = *parsedValue;
						return true;
					}
					break;
				}
			}
			else
			{
				switch (interactionState)
				{
				case InteractionState::Default:
					if (const Optional<T> parsedValue = StringToValueOpt<T>(value))
					{
						defaultValue = *parsedValue;
						return true;
					}
					break;
				case InteractionState::Hovered:
					if (const Optional<T> parsedValue = StringToValueOpt<T>(value))
					{
						hoveredValue = *parsedValue;
						return true;
					}
					break;
				case InteractionState::Pressed:
					if (const Optional<T> parsedValue = StringToValueOpt<T>(value))
					{
						pressedValue = *parsedValue;
						return true;
					}
					break;
				case InteractionState::Disabled:
					if (const Optional<T> parsedValue = StringToValueOpt<T>(value))
					{
						disabledValue = *parsedValue;
						return true;
					}
					break;
				}
			}
			return false;
		}

		bool tryUnsetValueOf(InteractionState interactionState, SelectedYN selected)
		{
			if (selected)
			{
				switch (interactionState)
				{
				case InteractionState::Default:
					selectedDefaultValue = none;
					return true;
				case InteractionState::Hovered:
					selectedHoveredValue = none;
					return true;
				case InteractionState::Pressed:
					selectedPressedValue = none;
					return true;
				case InteractionState::Disabled:
					selectedDisabledValue = none;
					return true;
				}
			}
			else
			{
				switch (interactionState)
				{
				case InteractionState::Default:
					return false;
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
			}
			return false;
		}

		[[nodiscard]]
		bool hasValueOf(InteractionState interactionState, SelectedYN selected) const
		{
			if (selected)
			{
				switch (interactionState)
				{
				case InteractionState::Default:
					return selectedDefaultValue.has_value();
				case InteractionState::Hovered:
					return selectedHoveredValue.has_value();
				case InteractionState::Pressed:
					return selectedPressedValue.has_value();
				case InteractionState::Disabled:
					return selectedDisabledValue.has_value();
				}
			}
			else
			{
				switch (interactionState)
				{
				case InteractionState::Default:
					return true;
				case InteractionState::Hovered:
					return hoveredValue.has_value();
				case InteractionState::Pressed:
					return pressedValue.has_value();
				case InteractionState::Disabled:
					return disabledValue.has_value();
				}
			}
			return false;
		}

		bool hasInteractiveValue() const
		{
			return hoveredValue.has_value() ||
				pressedValue.has_value() ||
				disabledValue.has_value() ||
				selectedDefaultValue.has_value() ||
				selectedHoveredValue.has_value() ||
				selectedPressedValue.has_value() ||
				selectedDisabledValue.has_value();
		}
	};
}
