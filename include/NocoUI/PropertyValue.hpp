#pragma once
#include <Siv3D.hpp>
#include "YN.hpp"
#include "InteractState.hpp"
#include "Serialization.hpp"

namespace noco
{
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
		const T& value(InteractState interactState, SelectedYN selected) const
		{
			if (selected)
			{
				switch (interactState)
				{
				case InteractState::Default:
					if (selectedDefaultValue)
					{
						return *selectedDefaultValue;
					}
					break;
				case InteractState::Hovered:
					if (selectedHoveredValue)
					{
						return *selectedHoveredValue;
					}
					if (selectedDefaultValue)
					{
						return *selectedDefaultValue;
					}
					break;
				case InteractState::Pressed:
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
				case InteractState::Disabled:
					if (selectedDisabledValue)
					{
						return *selectedDisabledValue;
					}
					break;
				}
			}

			switch (interactState)
			{
			case InteractState::Default:
				break;
			case InteractState::Hovered:
				if (hoveredValue)
				{
					return *hoveredValue;
				}
				break;
			case InteractState::Pressed:
				if (pressedValue)
				{
					return *pressedValue;
				}
				if (hoveredValue)
				{
					return *hoveredValue;
				}
				break;
			case InteractState::Disabled:
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
		Optional<String> getValueStringOf(InteractState interactState, SelectedYN selected) const
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
				switch (interactState)
				{
				case InteractState::Default:
					if (selectedDefaultValue)
					{
						return fnGetStr(*selectedDefaultValue);
					}
					break;
				case InteractState::Hovered:
					if (selectedHoveredValue)
					{
						return fnGetStr(*selectedHoveredValue);
					}
					break;
				case InteractState::Pressed:
					if (selectedPressedValue)
					{
						return fnGetStr(*selectedPressedValue);
					}
					break;
				case InteractState::Disabled:
					if (selectedDisabledValue)
					{
						return fnGetStr(*selectedDisabledValue);
					}
					break;
				}
			}
			else
			{
				switch (interactState)
				{
				case InteractState::Default:
					return fnGetStr(defaultValue);
				case InteractState::Hovered:
					if (hoveredValue)
					{
						return fnGetStr(*hoveredValue);
					}
					break;
				case InteractState::Pressed:
					if (pressedValue)
					{
						return fnGetStr(*pressedValue);
					}
					break;
				case InteractState::Disabled:
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
		String getValueStringOfFallback(InteractState interactState, SelectedYN selected) const
		{
			const T v = value(interactState, selected);
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

		bool trySetValueStringOf(StringView value, InteractState interactState, SelectedYN selected)
		{
			if (selected)
			{
				switch (interactState)
				{
				case InteractState::Default:
					if (const Optional<T> parsedValue = StringToValueOpt<T>(value))
					{
						selectedDefaultValue = *parsedValue;
						return true;
					}
					break;
				case InteractState::Hovered:
					if (const Optional<T> parsedValue = StringToValueOpt<T>(value))
					{
						selectedHoveredValue = *parsedValue;
						return true;
					}
					break;
				case InteractState::Pressed:
					if (const Optional<T> parsedValue = StringToValueOpt<T>(value))
					{
						selectedPressedValue = *parsedValue;
						return true;
					}
					break;
				case InteractState::Disabled:
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
				switch (interactState)
				{
				case InteractState::Default:
					if (const Optional<T> parsedValue = StringToValueOpt<T>(value))
					{
						defaultValue = *parsedValue;
						return true;
					}
					break;
				case InteractState::Hovered:
					if (const Optional<T> parsedValue = StringToValueOpt<T>(value))
					{
						hoveredValue = *parsedValue;
						return true;
					}
					break;
				case InteractState::Pressed:
					if (const Optional<T> parsedValue = StringToValueOpt<T>(value))
					{
						pressedValue = *parsedValue;
						return true;
					}
					break;
				case InteractState::Disabled:
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

		bool tryUnsetValueOf(InteractState interactState, SelectedYN selected)
		{
			if (selected)
			{
				switch (interactState)
				{
				case InteractState::Default:
					selectedDefaultValue = none;
					return true;
				case InteractState::Hovered:
					selectedHoveredValue = none;
					return true;
				case InteractState::Pressed:
					selectedPressedValue = none;
					return true;
				case InteractState::Disabled:
					selectedDisabledValue = none;
					return true;
				}
			}
			else
			{
				switch (interactState)
				{
				case InteractState::Default:
					return false;
				case InteractState::Hovered:
					hoveredValue = none;
					return true;
				case InteractState::Pressed:
					pressedValue = none;
					return true;
				case InteractState::Disabled:
					disabledValue = none;
					return true;
				}
			}
			return false;
		}

		[[nodiscard]]
		bool hasValueOf(InteractState interactState, SelectedYN selected) const
		{
			if (selected)
			{
				switch (interactState)
				{
				case InteractState::Default:
					return selectedDefaultValue.has_value();
				case InteractState::Hovered:
					return selectedHoveredValue.has_value();
				case InteractState::Pressed:
					return selectedPressedValue.has_value();
				case InteractState::Disabled:
					return selectedDisabledValue.has_value();
				}
			}
			else
			{
				switch (interactState)
				{
				case InteractState::Default:
					return true;
				case InteractState::Hovered:
					return hoveredValue.has_value();
				case InteractState::Pressed:
					return pressedValue.has_value();
				case InteractState::Disabled:
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
