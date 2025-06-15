#pragma once
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
		virtual void update(InteractionState interactionState, SelectedYN selected, double deltaTime) = 0;
		virtual void appendJSON(JSON& json) const = 0;
		virtual void readFromJSON(const JSON& json) = 0;
		virtual String propertyValueStringOfDefault() const = 0;
		virtual Optional<String> propertyValueStringOf(InteractionState interactionState, SelectedYN selected) const = 0;
		virtual String propertyValueStringOfFallback(InteractionState interactionState, SelectedYN selected) const = 0;
		virtual bool trySetPropertyValueString(StringView value) = 0;
		virtual bool trySetPropertyValueStringOf(StringView value, InteractionState interactionState, SelectedYN selected) = 0;
		virtual bool tryUnsetPropertyValueOf(InteractionState interactionState, SelectedYN selected) = 0;
		virtual bool hasPropertyValueOf(InteractionState interactionState, SelectedYN selected) const = 0;
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
		/*NonSerialized*/ InteractionState m_interactionState = InteractionState::Default;
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
		const T& propertyValue(InteractionState interactionState, SelectedYN selected) const
		{
			return m_propertyValue.value(interactionState, selected);
		}

		void setPropertyValue(const PropertyValue<T>& propertyValue)
		{
			m_propertyValue = propertyValue;
		}

		[[nodiscard]]
		const T& value() const
		{
			return m_propertyValue.value(m_interactionState, m_selected);
		}

		void update(InteractionState interactionState, SelectedYN selected, double) override
		{
			m_interactionState = interactionState;
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
		String propertyValueStringOfDefault() const override
		{
			return m_propertyValue.getValueStringOfDefault();
		}

		[[nodiscard]]
		Optional<String> propertyValueStringOf(InteractionState interactionState, SelectedYN selected) const override
		{
			return m_propertyValue.getValueStringOf(interactionState, selected);
		}

		[[nodiscard]]
		String propertyValueStringOfFallback(InteractionState interactionState, SelectedYN selected) const override
		{
			return m_propertyValue.getValueStringOfFallback(interactionState, selected);
		}

		bool trySetPropertyValueString(StringView value) override
		{
			return m_propertyValue.trySetValueString(value);
		}

		bool trySetPropertyValueStringOf(StringView value, InteractionState interactionState, SelectedYN selected) override
		{
			return m_propertyValue.trySetValueStringOf(value, interactionState, selected);
		}

		bool tryUnsetPropertyValueOf(InteractionState interactionState, SelectedYN selected) override
		{
			return m_propertyValue.tryUnsetValueOf(interactionState, selected);
		}

		[[nodiscard]]
		bool hasPropertyValueOf(InteractionState interactionState, SelectedYN selected) const override
		{
			return m_propertyValue.hasValueOf(interactionState, selected);
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
		
		T& get(InteractionState state, SelectedYN selected)
		{
			if (selected)
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
		
		const T& get(InteractionState state, SelectedYN selected) const
		{
			if (selected)
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
		
		InteractiveValue<Optional<TweenValue<T>>> m_tweenValues;
		/*NonSerialized*/ InteractiveValue<Optional<Stopwatch>> m_tweenStopwatches;
		/*NonSerialized*/ Optional<InteractionState> m_prevTweenSource = none;

	private:
		[[nodiscard]]
		JSON tweenValuesToJSON() const
		{
			JSON json;
			if (m_tweenValues.defaultValue.has_value()) json[U"default"] = m_tweenValues.defaultValue->toJSON();
			if (m_tweenValues.hoveredValue.has_value()) json[U"hovered"] = m_tweenValues.hoveredValue->toJSON();
			if (m_tweenValues.pressedValue.has_value()) json[U"pressed"] = m_tweenValues.pressedValue->toJSON();
			if (m_tweenValues.disabledValue.has_value()) json[U"disabled"] = m_tweenValues.disabledValue->toJSON();
			if (m_tweenValues.selectedDefaultValue.has_value()) json[U"selectedDefault"] = m_tweenValues.selectedDefaultValue->toJSON();
			if (m_tweenValues.selectedHoveredValue.has_value()) json[U"selectedHovered"] = m_tweenValues.selectedHoveredValue->toJSON();
			if (m_tweenValues.selectedPressedValue.has_value()) json[U"selectedPressed"] = m_tweenValues.selectedPressedValue->toJSON();
			if (m_tweenValues.selectedDisabledValue.has_value()) json[U"selectedDisabled"] = m_tweenValues.selectedDisabledValue->toJSON();
			return json;
		}
		
		void tweenValuesFromJSON(const JSON& json)
		{
			m_tweenValues = InteractiveValue<Optional<TweenValue<T>>>{};
			if (json.contains(U"default")) m_tweenValues.defaultValue = TweenValue<T>::fromJSON(json[U"default"], T{}, T{});
			if (json.contains(U"hovered")) m_tweenValues.hoveredValue = TweenValue<T>::fromJSON(json[U"hovered"], T{}, T{});
			if (json.contains(U"pressed")) m_tweenValues.pressedValue = TweenValue<T>::fromJSON(json[U"pressed"], T{}, T{});
			if (json.contains(U"disabled")) m_tweenValues.disabledValue = TweenValue<T>::fromJSON(json[U"disabled"], T{}, T{});
			if (json.contains(U"selectedDefault")) m_tweenValues.selectedDefaultValue = TweenValue<T>::fromJSON(json[U"selectedDefault"], T{}, T{});
			if (json.contains(U"selectedHovered")) m_tweenValues.selectedHoveredValue = TweenValue<T>::fromJSON(json[U"selectedHovered"], T{}, T{});
			if (json.contains(U"selectedPressed")) m_tweenValues.selectedPressedValue = TweenValue<T>::fromJSON(json[U"selectedPressed"], T{}, T{});
			if (json.contains(U"selectedDisabled")) m_tweenValues.selectedDisabledValue = TweenValue<T>::fromJSON(json[U"selectedDisabled"], T{}, T{});
		}

		[[nodiscard]]
		bool hasTweenOf(InteractionState interactionState, SelectedYN selected) const
		{
			return m_tweenValues.get(interactionState, selected).has_value();
		}
		
		[[nodiscard]]
		Optional<InteractionState> tweenSourceOf(InteractionState interactionState, SelectedYN selected) const
		{
			// フォールバック検索
			switch (interactionState)
			{
			case InteractionState::Pressed:
				if (m_tweenValues.get(InteractionState::Pressed, selected).has_value()) return InteractionState::Pressed;
				if (m_tweenValues.get(InteractionState::Hovered, selected).has_value()) return InteractionState::Hovered;
				if (m_tweenValues.get(InteractionState::Default, selected).has_value()) return InteractionState::Default;
				break;
			case InteractionState::Hovered:
				if (m_tweenValues.get(InteractionState::Hovered, selected).has_value()) return InteractionState::Hovered;
				if (m_tweenValues.get(InteractionState::Default, selected).has_value()) return InteractionState::Default;
				break;
			case InteractionState::Default:
				if (m_tweenValues.get(InteractionState::Default, selected).has_value()) return InteractionState::Default;
				break;
			case InteractionState::Disabled:
				if (m_tweenValues.get(InteractionState::Disabled, selected).has_value()) return InteractionState::Disabled;
				break;
			}
			return none;
		}
		
		[[nodiscard]]
		Optional<TweenValue<T>> getTweenWithFallback(InteractionState interactionState, SelectedYN selected) const
		{
			// 各状態を順番にチェック
			if (selected)
			{
				switch (interactionState)
				{
				case InteractionState::Pressed:
					if (m_tweenValues.selectedPressedValue.has_value()) return m_tweenValues.selectedPressedValue;
					if (m_tweenValues.selectedHoveredValue.has_value()) return m_tweenValues.selectedHoveredValue;
					if (m_tweenValues.selectedDefaultValue.has_value()) return m_tweenValues.selectedDefaultValue;
					break;
				case InteractionState::Hovered:
					if (m_tweenValues.selectedHoveredValue.has_value()) return m_tweenValues.selectedHoveredValue;
					if (m_tweenValues.selectedDefaultValue.has_value()) return m_tweenValues.selectedDefaultValue;
					break;
				case InteractionState::Default:
					if (m_tweenValues.selectedDefaultValue.has_value()) return m_tweenValues.selectedDefaultValue;
					break;
				case InteractionState::Disabled:
					if (m_tweenValues.selectedDisabledValue.has_value()) return m_tweenValues.selectedDisabledValue;
					break;
				}
			}
			
			// 通常の状態をチェック
			switch (interactionState)
			{
			case InteractionState::Pressed:
				if (m_tweenValues.pressedValue.has_value()) return m_tweenValues.pressedValue;
				if (m_tweenValues.hoveredValue.has_value()) return m_tweenValues.hoveredValue;
				break;
			case InteractionState::Hovered:
				if (m_tweenValues.hoveredValue.has_value()) return m_tweenValues.hoveredValue;
				break;
			case InteractionState::Default:
				if (m_tweenValues.defaultValue.has_value()) return m_tweenValues.defaultValue;
				break;
			case InteractionState::Disabled:
				if (m_tweenValues.disabledValue.has_value()) return m_tweenValues.disabledValue;
				break;
			}
			return none;
		}

	public:
		SmoothProperty(const char32_t* name, const PropertyValue<T>& propertyValue)
			: m_name{ name }
			, m_propertyValue{ propertyValue }
			, m_smoothing{ propertyValue.value(InteractionState::Default, SelectedYN::No) }
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
		const T& propertyValue(InteractionState interactionState, SelectedYN selected) const
		{
			return m_propertyValue.value(interactionState, selected);
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

		void update(InteractionState interactionState, SelectedYN selected, double deltaTime) override
		{
			T targetValue = m_propertyValue.value(interactionState, selected);
			
			// 現在の状態のTweenを取得（フォールバック含む）
			Optional<TweenValue<T>> tween = getTweenWithFallback(interactionState, selected);
			
			if (tween)
			{
				// 初回update時は、DefaultのTweenのvalue1を初期値に設定
				if (m_prevTweenSource == none)
				{
					Optional<TweenValue<T>> defaultTween = getTweenWithFallback(InteractionState::Default, selected);
					if (defaultTween)
					{
						m_smoothing.setCurrentValue(defaultTween->value1);
					}
				}
				
				// Tweenソースの変更をチェック
				Optional<InteractionState> currentTweenSource = tweenSourceOf(interactionState, selected);
				if (m_prevTweenSource != currentTweenSource)
				{
					// 遷移先のTweenのretrigger設定に従ってStopwatchをリセットするかどうか決める
					if (tween->retrigger && currentTweenSource)
					{
						m_tweenStopwatches.get(*currentTweenSource, selected) = none;
					}
					// retriggerが無効の場合はStopwatchをリセットしない
					// （既存のStopwatchが保持され、アニメーションが継続）
					m_prevTweenSource = currentTweenSource;
				}
				
				// Stopwatchを開始（フォールバック元のStateから）
				if (currentTweenSource)
				{
					auto& stopwatch = m_tweenStopwatches.get(*currentTweenSource, selected);
					if (!stopwatch)
					{
						stopwatch = Stopwatch{ StartImmediately::Yes };
					}
					
					const double tweenTime = stopwatch->sF();
					targetValue = tween->calculateValue(tweenTime);
				}
			}
			
			m_smoothing.update(targetValue, m_propertyValue.smoothTime, deltaTime);
		}

		void appendJSON(JSON& json) const override
		{
			// 既存のプロパティ値をJSONに追加
			JSON propertyJson = m_propertyValue.toJSON();
			
			// Tween設定がある場合はJSONに含める
			JSON tweenJson = tweenValuesToJSON();
			if (!tweenJson.isEmpty())
			{
				// propertyJsonが単純な値の場合、オブジェクトに変換
				if (!propertyJson.isObject())
				{
					JSON newJson;
					newJson[U"default"] = propertyJson;
					propertyJson = newJson;
				}
				
				propertyJson[U"tweens"] = tweenJson;
			}
			
			json[m_name] = propertyJson;
		}

		void readFromJSON(const JSON& json) override
		{
			if (!json.contains(m_name))
			{
				return;
			}
			const JSON& propertyJson = json[m_name];
			m_propertyValue = PropertyValue<T>::fromJSON(propertyJson);
			m_smoothing = Smoothing<T>{ m_propertyValue.value(InteractionState::Default, SelectedYN::No) };
			
			// Tween設定の読み込み
			if (propertyJson.contains(U"tweens"))
			{
				const JSON& tweenJson = propertyJson[U"tweens"];
				tweenValuesFromJSON(tweenJson);
			}
		}

		[[nodiscard]]
		String propertyValueStringOfDefault() const override
		{
			return m_propertyValue.getValueStringOfDefault();
		}

		[[nodiscard]]
		Optional<String> propertyValueStringOf(InteractionState interactionState, SelectedYN selected) const override
		{
			return m_propertyValue.getValueStringOf(interactionState, selected);
		}

		[[nodiscard]]
		String propertyValueStringOfFallback(InteractionState interactionState, SelectedYN selected) const override
		{
			return m_propertyValue.getValueStringOfFallback(interactionState, selected);
		}

		bool trySetPropertyValueString(StringView value) override
		{
			return m_propertyValue.trySetValueString(value);
		}

		bool trySetPropertyValueStringOf(StringView value, InteractionState interactionState, SelectedYN selected) override
		{
			return m_propertyValue.trySetValueStringOf(value, interactionState, selected);
		}

		bool tryUnsetPropertyValueOf(InteractionState interactionState, SelectedYN selected) override
		{
			return m_propertyValue.tryUnsetValueOf(interactionState, selected);
		}

		[[nodiscard]]
		bool hasPropertyValueOf(InteractionState interactionState, SelectedYN selected) const override
		{
			return m_propertyValue.hasValueOf(interactionState, selected);
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
		/*NonSerialized*/ InteractionState m_interactionState = InteractionState::Default;
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

		void update(InteractionState, SelectedYN, double) override
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
		Optional<String> propertyValueStringOf(InteractionState interactionState, SelectedYN selected) const override
		{
			if (interactionState == InteractionState::Default && selected == SelectedYN::No)
			{
				return propertyValueStringOfDefault();
			}
			else
			{
				return none;
			}
		}

		[[nodiscard]]
		String propertyValueStringOfFallback(InteractionState, SelectedYN) const override
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

		bool trySetPropertyValueStringOf(StringView, InteractionState, SelectedYN) override
		{
			throw Error{ U"trySetPropertyValueStringOf() called for non-interactive property" };
		}

		bool tryUnsetPropertyValueOf(InteractionState, SelectedYN) override
		{
			return false;
		}

		[[nodiscard]]
		bool hasPropertyValueOf(InteractionState interactionState, SelectedYN selected) const override
		{
			if (interactionState == InteractionState::Default && selected == SelectedYN::No)
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
