﻿#pragma once
#include <Siv3D.hpp>
#include "PropertyValue.hpp"
#include "InteractionState.hpp"
#include "YN.hpp"
#include "Smoothing.hpp"
#include "LRTB.hpp"

namespace noco
{
	enum class PropertyEditType
	{
		Text,
		Bool,
		Enum,
		Vec2,
		Color,
		LRTB,
	};

	class IProperty
	{
	public:
		virtual ~IProperty() = default;
		virtual StringView name() const = 0;
		virtual void update(InteractionState interactionState, const Array<String>& activeStyleStates, double deltaTime) = 0;
		virtual void appendJSON(JSON& json) const = 0;
		virtual void readFromJSON(const JSON& json) = 0;
		virtual String propertyValueStringOfDefault() const = 0;
		virtual Optional<String> propertyValueStringOf(InteractionState interactionState, const Array<String>& activeStyleStates) const = 0;
		virtual String propertyValueStringOfFallback(InteractionState interactionState, const Array<String>& activeStyleStates) const = 0;
		virtual bool trySetPropertyValueString(StringView value) = 0;
		virtual bool trySetPropertyValueStringOf(StringView value, InteractionState interactionState, const Array<String>& activeStyleStates) = 0;
		virtual bool tryUnsetPropertyValueOf(InteractionState interactionState, const Array<String>& activeStyleStates) = 0;
		virtual bool hasPropertyValueOf(InteractionState interactionState, const Array<String>& activeStyleStates) const = 0;
		virtual PropertyEditType editType() const = 0;
		virtual Array<String> enumCandidates() const
		{
			if (editType() != PropertyEditType::Enum)
			{
				throw Error{ U"enumCandidates() called for non-enum property" };
			}
			return {};
		}
		virtual bool isInteractiveProperty() const = 0;
		virtual bool hasInteractivePropertyValue() const = 0;
		virtual bool isSmoothProperty() const = 0;
		virtual double smoothTime() const = 0;
		virtual bool trySetSmoothTime(double smoothTime) = 0;
		virtual Array<String> styleStateKeys() const = 0;
	};

	template <typename T>
	PropertyEditType PropertyEditTypeOf()
	{
		if constexpr (std::same_as<T, bool>)
		{
			return PropertyEditType::Bool;
		}
		else if constexpr (std::is_enum_v<T>)
		{
			return PropertyEditType::Enum;
		}
		else if constexpr (std::same_as<T, Vec2>)
		{
			return PropertyEditType::Vec2;
		}
		else if constexpr (std::same_as<T, ColorF>)
		{
			return PropertyEditType::Color;
		}
		else if constexpr (std::same_as<T, LRTB>)
		{
			return PropertyEditType::LRTB;
		}
		else
		{
			return PropertyEditType::Text;
		}
	}

	template <class T>
	class Property : public IProperty
	{
	private:
		const char32_t* m_name; // 数が多く、基本的にリテラルのみのため、Stringではなくconst char32_t*で持つ
		PropertyValue<T> m_propertyValue;
		/*NonSerialized*/ InteractionState m_interactionState = InteractionState::Default;
		/*NonSerialized*/ Array<String> m_activeStyleStates{};

	public:
		Property(const char32_t* name, const PropertyValue<T>& propertyValue)
			: m_name{ name }
			, m_propertyValue{ propertyValue }
		{
		}

		template <class U>
		Property(const char32_t* name, const U& defaultValue) requires std::convertible_to<U, T>
			: m_name{ name }
			, m_propertyValue{ defaultValue }
		{
		}

		Property(const char32_t* name, StringView defaultValue) requires std::same_as<T, String>
			: m_name{ name }
			, m_propertyValue{ String{ defaultValue } }
		{
		}

		[[nodiscard]]
		StringView name() const override
		{
			return m_name;
		}

		[[nodiscard]]
		const PropertyValue<T>& propertyValue() const
		{
			return m_propertyValue;
		}

		[[nodiscard]]
		const T& propertyValue(InteractionState interactionState, const Array<String>& activeStyleStates) const
		{
			return m_propertyValue.value(interactionState, activeStyleStates);
		}

		void setPropertyValue(const PropertyValue<T>& propertyValue)
		{
			m_propertyValue = propertyValue;
		}

		[[nodiscard]]
		const T& value() const
		{
			return m_propertyValue.value(m_interactionState, m_activeStyleStates);
		}

		void update(InteractionState interactionState, const Array<String>& activeStyleStates, double) override
		{
			m_interactionState = interactionState;
			m_activeStyleStates = activeStyleStates;
		}

		void appendJSON(JSON& json) const override
		{
			json[m_name] = m_propertyValue.toJSON();
		}

		void readFromJSON(const JSON& json) override
		{
			if (!json.contains(m_name))
			{
				return;
			}
			m_propertyValue = PropertyValue<T>::fromJSON(json[m_name]);
		}

		[[nodiscard]]
		String propertyValueStringOfDefault() const override
		{
			return m_propertyValue.getValueStringOfDefault();
		}

		[[nodiscard]]
		Optional<String> propertyValueStringOf(InteractionState interactionState, const Array<String>& activeStyleStates) const override
		{
			return m_propertyValue.getValueStringOf(interactionState, activeStyleStates);
		}

		[[nodiscard]]
		String propertyValueStringOfFallback(InteractionState interactionState, const Array<String>& activeStyleStates) const override
		{
			return m_propertyValue.getValueStringOfFallback(interactionState, activeStyleStates);
		}

		bool trySetPropertyValueString(StringView value) override
		{
			return m_propertyValue.trySetValueString(value);
		}

		bool trySetPropertyValueStringOf(StringView value, InteractionState interactionState, const Array<String>& activeStyleStates) override
		{
			return m_propertyValue.trySetValueStringOf(value, interactionState, activeStyleStates);
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
			return PropertyEditTypeOf<T>();
		}

		[[nodiscard]]
		Array<String> enumCandidates() const override
		{
			if constexpr (std::is_enum_v<T>)
			{
				return EnumNames<T>();
			}
			else
			{
				throw Error{ U"enumCandidates() called for non-enum property" };
			}
		}

		[[nodiscard]]
		bool isInteractiveProperty() const override
		{
			return true;
		}

		[[nodiscard]]
		bool hasInteractivePropertyValue() const override
		{
			return m_propertyValue.hasInteractiveValue();
		}

		[[nodiscard]]
		bool isSmoothProperty() const override
		{
			return false;
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

		Array<String> styleStateKeys() const override
		{
			Array<String> result;
			
			// styleStateValuesから収集
			if (m_propertyValue.styleStateValues)
			{
				for (const auto& [state, value] : *m_propertyValue.styleStateValues)
				{
					if (!result.contains(state))
					{
						result.push_back(state);
					}
				}
			}
			
			
			return result;
		}
	};

	// InteractiveValue: 8つの状態（通常4状態 + selected4状態）を持つ値の構造体
	template <typename T>
	struct InteractiveValue
	{
		T defaultValue{};
		T hoveredValue{};
		T pressedValue{};
		T disabledValue{};
		T selectedDefaultValue{};
		T selectedHoveredValue{};
		T selectedPressedValue{};
		T selectedDisabledValue{};
		
		T& get(InteractionState state, const Array<String>& activeStyleStates)
		{
			if (activeStyleStates.contains(U"selected"))
			{
				switch (state)
				{
				case InteractionState::Default:
					return selectedDefaultValue;
				case InteractionState::Hovered:
					return selectedHoveredValue;
				case InteractionState::Pressed:
					return selectedPressedValue;
				case InteractionState::Disabled:
					return selectedDisabledValue;
				}
			}
			else
			{
				switch (state)
				{
				case InteractionState::Default:
					return defaultValue;
				case InteractionState::Hovered:
					return hoveredValue;
				case InteractionState::Pressed:
					return pressedValue;
				case InteractionState::Disabled:
					return disabledValue;
				}
			}
			return defaultValue;
		}
		
		const T& get(InteractionState state, const Array<String>& activeStyleStates) const
		{
			if (activeStyleStates.contains(U"selected"))
			{
				switch (state)
				{
				case InteractionState::Default:
					return selectedDefaultValue;
				case InteractionState::Hovered:
					return selectedHoveredValue;
				case InteractionState::Pressed:
					return selectedPressedValue;
				case InteractionState::Disabled:
					return selectedDisabledValue;
				}
			}
			else
			{
				switch (state)
				{
				case InteractionState::Default:
					return defaultValue;
				case InteractionState::Hovered:
					return hoveredValue;
				case InteractionState::Pressed:
					return pressedValue;
				case InteractionState::Disabled:
					return disabledValue;
				}
			}
			return defaultValue;
		}
		
	};


	template <class T>
	class SmoothProperty : public IProperty
	{
		static_assert(!std::is_same_v<T, bool>, "SmoothProperty<bool> is not allowed");
		static_assert(!std::is_enum_v<T>, "SmoothProperty does not support enum type");
		static_assert(HasSmoothDamp<T> || HasSmoothDampInner<T>, "T must have Math::SmoothDamp");

	private:
		const char32_t* m_name; // 数が多く、基本的にリテラルのみのため、Stringではなくconst char32_t*で持つ
		PropertyValue<T> m_propertyValue;
		/*NonSerialized*/ Smoothing<T> m_smoothing;

	public:
		SmoothProperty(const char32_t* name, const PropertyValue<T>& propertyValue)
			: m_name{ name }
			, m_propertyValue{ propertyValue }
			, m_smoothing{ propertyValue.value(InteractionState::Default, Array<String>{}) }
		{
		}

		template <class U>
		SmoothProperty(const char32_t* name, const U& defaultValue) requires std::convertible_to<U, T>
			: m_name{ name }
			, m_propertyValue{ defaultValue }
			, m_smoothing{ defaultValue }
		{
		}

		[[nodiscard]]
		StringView name() const override
		{
			return m_name;
		}

		[[nodiscard]]
		const PropertyValue<T>& propertyValue() const
		{
			return m_propertyValue;
		}

		[[nodiscard]]
		const T& propertyValue(InteractionState interactionState, const Array<String>& activeStyleStates) const
		{
			return m_propertyValue.value(interactionState, activeStyleStates);
		}

		void setPropertyValue(const PropertyValue<T>& propertyValue)
		{
			m_propertyValue = propertyValue;
		}

		[[nodiscard]]
		const T& value() const
		{
			return m_smoothing.currentValue();
		}

		void update(InteractionState interactionState, const Array<String>& activeStyleStates, double deltaTime) override
		{
			m_smoothing.update(m_propertyValue.value(interactionState, activeStyleStates), m_propertyValue.smoothTime, deltaTime);
		}

		void appendJSON(JSON& json) const override
		{
			json[m_name] = m_propertyValue.toJSON();
		}

		void readFromJSON(const JSON& json) override
		{
			if (!json.contains(m_name))
			{
				return;
			}
			m_propertyValue = PropertyValue<T>::fromJSON(json[m_name]);
			m_smoothing = Smoothing<T>{ m_propertyValue.value(InteractionState::Default, Array<String>{}) };
		}

		[[nodiscard]]
		String propertyValueStringOfDefault() const override
		{
			return m_propertyValue.getValueStringOfDefault();
		}

		[[nodiscard]]
		Optional<String> propertyValueStringOf(InteractionState interactionState, const Array<String>& activeStyleStates) const override
		{
			return m_propertyValue.getValueStringOf(interactionState, activeStyleStates);
		}

		[[nodiscard]]
		String propertyValueStringOfFallback(InteractionState interactionState, const Array<String>& activeStyleStates) const override
		{
			return m_propertyValue.getValueStringOfFallback(interactionState, activeStyleStates);
		}

		bool trySetPropertyValueString(StringView value) override
		{
			return m_propertyValue.trySetValueString(value);
		}

		bool trySetPropertyValueStringOf(StringView value, InteractionState interactionState, const Array<String>& activeStyleStates) override
		{
			return m_propertyValue.trySetValueStringOf(value, interactionState, activeStyleStates);
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
			if constexpr (std::same_as<T, Vec2>)
			{
				return PropertyEditType::Vec2;
			}
			else if constexpr (std::same_as<T, ColorF>)
			{
				return PropertyEditType::Color;
			}
			else if constexpr (std::same_as<T, LRTB>)
			{
				return PropertyEditType::LRTB;
			}
			else
			{
				return PropertyEditType::Text;
			}
		}

		[[nodiscard]]
		Array<String> enumCandidates() const override
		{
			throw Error{ U"enumCandidates() called for non-enum property" };
		}

		[[nodiscard]]
		bool isInteractiveProperty() const override
		{
			return true;
		}

		[[nodiscard]]
		bool hasInteractivePropertyValue() const override
		{
			return m_propertyValue.hasInteractiveValue();
		}

		[[nodiscard]]
		bool isSmoothProperty() const override
		{
			return true;
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

		Array<String> styleStateKeys() const override
		{
			Array<String> result;
			
			// styleStateValuesから収集
			if (m_propertyValue.styleStateValues)
			{
				for (const auto& [state, value] : *m_propertyValue.styleStateValues)
				{
					if (!result.contains(state))
					{
						result.push_back(state);
					}
				}
			}
			
			
			return result;
		}
	};

	template <class T>
	class PropertyNonInteractive : public IProperty
	{
	private:
		const char32_t* m_name; // 数が多く、基本的にリテラルのみのため、Stringではなくconst char32_t*で持つ
		T m_value;
		/*NonSerialized*/ InteractionState m_interactionState = InteractionState::Default;
		/*NonSerialized*/ Array<String> m_activeStyleStates{};

	public:
		template <class U>
		PropertyNonInteractive(const char32_t* name, const U& value) requires std::convertible_to<U, T>
			: m_name{ name }
			, m_value{ value }
		{
		}

		PropertyNonInteractive(const char32_t* name, StringView value) requires std::same_as<T, String>
			: m_name{ name }
			, m_value{ String{ value } }
		{
		}

		[[nodiscard]]
		StringView name() const override
		{
			return m_name;
		}

		template <typename U>
		void setValue(const U& value) requires std::convertible_to<U, T>
		{
			m_value = value;
		}

		void setValue(StringView value) requires std::same_as<T, String>
		{
			m_value = String{ value };
		}

		[[nodiscard]]
		const T& value() const
		{
			return m_value;
		}

		void update(InteractionState, const Array<String>&, double) override
		{
		}

		void appendJSON(JSON& json) const override
		{
			if constexpr (std::is_enum_v<T>)
			{
				json[m_name] = EnumToString(m_value);
			}
			else if constexpr (HasToJSON<T>)
			{
				json[m_name] = m_value.toJSON();
			}
			else
			{
				json[m_name] = m_value;
			}
		}

		void readFromJSON(const JSON& json) override
		{
			if (!json.contains(m_name))
			{
				return;
			}

			// Propertyが後からPropertyNonInteractiveに変更される場合を考慮して、PropertyValue<T>::fromJSONを使う
			m_value = PropertyValue<T>::fromJSON(json[m_name]).defaultValue;
		}

		[[nodiscard]]
		String propertyValueStringOfDefault() const override
		{
			if constexpr (std::is_enum_v<T>)
			{
				return EnumToString(m_value);
			}
			else
			{
				return Format(m_value);
			}
		}

		[[nodiscard]]
		Optional<String> propertyValueStringOf(InteractionState interactionState, const Array<String>& activeStyleStates) const override
		{
			if (interactionState == InteractionState::Default && activeStyleStates.empty())
			{
				return propertyValueStringOfDefault();
			}
			else
			{
				return none;
			}
		}

		[[nodiscard]]
		String propertyValueStringOfFallback(InteractionState, const Array<String>&) const override
		{
			return propertyValueStringOfDefault();
		}

		bool trySetPropertyValueString(StringView value) override
		{
			if (const auto valueOpt = StringToValueOpt<T>(value))
			{
				m_value = *valueOpt;
				return true;
			}
			else
			{
				return false;
			}
		}

		bool trySetPropertyValueStringOf(StringView, InteractionState, const Array<String>&) override
		{
			throw Error{ U"trySetPropertyValueStringOf() called for non-interactive property" };
		}

		bool tryUnsetPropertyValueOf(InteractionState, const Array<String>&) override
		{
			return false;
		}

		[[nodiscard]]
		bool hasPropertyValueOf(InteractionState interactionState, const Array<String>& activeStyleStates) const override
		{
			if (interactionState == InteractionState::Default && activeStyleStates.empty())
			{
				return true;
			}
			else
			{
				return false;
			}
		}

		[[nodiscard]]
		PropertyEditType editType() const override
		{
			if constexpr (std::same_as<T, bool>)
			{
				return PropertyEditType::Bool;
			}
			else if constexpr (std::is_enum_v<T>)
			{
				return PropertyEditType::Enum;
			}
			else if constexpr (std::same_as<T, Vec2>)
			{
				return PropertyEditType::Vec2;
			}
			else if constexpr (std::same_as<T, ColorF>)
			{
				return PropertyEditType::Color;
			}
			else if constexpr (std::same_as<T, LRTB>)
			{
				return PropertyEditType::LRTB;
			}
			else
			{
				return PropertyEditType::Text;
			}
		}

		[[nodiscard]]
		Array<String> enumCandidates() const override
		{
			if constexpr (std::is_enum_v<T>)
			{
				return EnumNames<T>();
			}
			else
			{
				throw Error{ U"enumCandidates() called for non-enum property" };
			}
		}

		[[nodiscard]]
		bool isInteractiveProperty() const override
		{
			return false;
		}

		[[nodiscard]]
		bool hasInteractivePropertyValue() const override
		{
			return false;
		}

		[[nodiscard]]
		bool isSmoothProperty() const override
		{
			return false;
		}

		[[nodiscard]]
		double smoothTime() const override
		{
			return 0.0;
		}

		bool trySetSmoothTime(double) override
		{
			return false;
		}

		Array<String> styleStateKeys() const override
		{
			// PropertyNonInteractiveはstyleStateをサポートしない
			return {};
		}
	};
}
