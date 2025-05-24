#pragma once
#include <Siv3D.hpp>
#include "PropertyValue.hpp"
#include "InteractState.hpp"
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
		virtual void update(InteractState interactState, SelectedYN selected, double deltaTime) = 0;
		virtual void appendJSON(JSON& json) const = 0;
		virtual void readFromJSON(const JSON& json) = 0;
		virtual String propertyValueString() const = 0;
		virtual Optional<String> propertyValueStringOf(InteractState interactState, SelectedYN selected) = 0;
		virtual String propertyValueStringOfFallback(InteractState interactState, SelectedYN selected) = 0;
		virtual bool trySetPropertyValueString(StringView value) = 0;
		virtual bool trySetPropertyValueStringOf(StringView value, InteractState interactState, SelectedYN selected) = 0;
		virtual bool tryUnsetPropertyValueOf(InteractState interactState, SelectedYN selected) = 0;
		virtual bool hasPropertyValueOf(InteractState interactState, SelectedYN selected) const = 0;
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
		/*NonSerialized*/ InteractState m_interactState = InteractState::Default;
		/*NonSerialized*/ SelectedYN m_selected = SelectedYN::No;

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
		const T& propertyValue(InteractState interactState, SelectedYN selected) const
		{
			return m_propertyValue.value(interactState, selected);
		}

		void setPropertyValue(const PropertyValue<T>& propertyValue)
		{
			m_propertyValue = propertyValue;
		}

		[[nodiscard]]
		const T& value() const
		{
			return m_propertyValue.value(m_interactState, m_selected);
		}

		void update(InteractState interactState, SelectedYN selected, double) override
		{
			m_interactState = interactState;
			m_selected = selected;
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
		String propertyValueString() const override
		{
			return m_propertyValue.getValueString();
		}

		[[nodiscard]]
		Optional<String> propertyValueStringOf(InteractState interactState, SelectedYN selected) override
		{
			return m_propertyValue.getValueStringOf(interactState, selected);
		}

		[[nodiscard]]
		String propertyValueStringOfFallback(InteractState interactState, SelectedYN selected) override
		{
			return m_propertyValue.getValueStringOfFallback(interactState, selected);
		}

		bool trySetPropertyValueString(StringView value) override
		{
			return m_propertyValue.trySetValueString(value);
		}

		bool trySetPropertyValueStringOf(StringView value, InteractState interactState, SelectedYN selected) override
		{
			return m_propertyValue.trySetValueStringOf(value, interactState, selected);
		}

		bool tryUnsetPropertyValueOf(InteractState interactState, SelectedYN selected) override
		{
			return m_propertyValue.tryUnsetValueOf(interactState, selected);
		}

		[[nodiscard]]
		bool hasPropertyValueOf(InteractState interactState, SelectedYN selected) const override
		{
			return m_propertyValue.hasValueOf(interactState, selected);
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
			, m_smoothing{ propertyValue.value(InteractState::Default, SelectedYN::No) }
		{
		}

		template <class U>
		SmoothProperty(const char32_t* name, const U& defaultValue) requires std::convertible_to<U, T>
			: m_name{ name }
			, m_propertyValue{ defaultValue }
			, m_smoothing{ 0.0, defaultValue }
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
		const T& propertyValue(InteractState interactState) const
		{
			return m_propertyValue.value(interactState);
		}

		[[nodiscard]]
		const T& value() const
		{
			return m_smoothing.currentValue();
		}

		void setPropertyValue(const PropertyValue<T>& propertyValue)
		{
			m_propertyValue = propertyValue;
		}

		void update(InteractState interactState, SelectedYN selected, double deltaTime) override
		{
			m_smoothing.update(m_propertyValue.value(interactState, selected), m_propertyValue.smoothTime, deltaTime);
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
			m_smoothing = Smoothing<T>{ m_propertyValue.value(InteractState::Default, SelectedYN::No) };
		}

		[[nodiscard]]
		String propertyValueString() const override
		{
			return m_propertyValue.getValueString();
		}

		[[nodiscard]]
		Optional<String> propertyValueStringOf(InteractState interactState, SelectedYN selected) override
		{
			return m_propertyValue.getValueStringOf(interactState, selected);
		}

		[[nodiscard]]
		String propertyValueStringOfFallback(InteractState interactState, SelectedYN selected) override
		{
			return m_propertyValue.getValueStringOfFallback(interactState, selected);
		}

		bool trySetPropertyValueString(StringView value) override
		{
			return m_propertyValue.trySetValueString(value);
		}

		bool trySetPropertyValueStringOf(StringView value, InteractState interactState, SelectedYN selected) override
		{
			return m_propertyValue.trySetValueStringOf(value, interactState, selected);
		}

		bool tryUnsetPropertyValueOf(InteractState interactState, SelectedYN selected) override
		{
			return m_propertyValue.tryUnsetValueOf(interactState, selected);
		}

		[[nodiscard]]
		bool hasPropertyValueOf(InteractState interactState, SelectedYN selected) const override
		{
			return m_propertyValue.hasValueOf(interactState, selected);
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
	};

	template <class T>
	class PropertyNonInteractive : public IProperty
	{
	private:
		const char32_t* m_name; // 数が多く、基本的にリテラルのみのため、Stringではなくconst char32_t*で持つ
		T m_value;
		/*NonSerialized*/ InteractState m_interactState = InteractState::Default;
		/*NonSerialized*/ SelectedYN m_selected = SelectedYN::No;

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

		[[nodiscard]]
		const T& value() const
		{
			return m_value;
		}

		void update(InteractState, SelectedYN, double) override
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
		String propertyValueString() const override
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
		Optional<String> propertyValueStringOf(InteractState interactState, SelectedYN selected) override
		{
			if (interactState == InteractState::Default && selected == SelectedYN::No)
			{
				return propertyValueString();
			}
			else
			{
				return none;
			}
		}

		[[nodiscard]]
		String propertyValueStringOfFallback(InteractState, SelectedYN) override
		{
			return propertyValueString();
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

		bool trySetPropertyValueStringOf(StringView, InteractState, SelectedYN) override
		{
			throw Error{ U"trySetPropertyValueStringOf() called for non-interactive property" };
		}

		bool tryUnsetPropertyValueOf(InteractState, SelectedYN) override
		{
			return false;
		}

		[[nodiscard]]
		bool hasPropertyValueOf(InteractState interactState, SelectedYN selected) const override
		{
			if (interactState == InteractState::Default && selected == SelectedYN::No)
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
	};
}
