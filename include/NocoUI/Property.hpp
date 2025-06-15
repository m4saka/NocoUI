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
		double m_tweenTransitionTime = 0.0;
		/*NonSerialized*/ InteractiveValue<Stopwatch> m_tweenStopwatches;
		/*NonSerialized*/ InteractiveValue<Smoothing<double>> m_tweenWeights{
			Smoothing<double>{ 1.0 }, // defaultValue
			Smoothing<double>{ 0.0 }, // hoveredValue
			Smoothing<double>{ 0.0 }, // pressedValue
			Smoothing<double>{ 0.0 }, // disabledValue
			Smoothing<double>{ 0.0 }, // selectedDefaultValue
			Smoothing<double>{ 0.0 }, // selectedHoveredValue
			Smoothing<double>{ 0.0 }, // selectedPressedValue
			Smoothing<double>{ 0.0 }  // selectedDisabledValue
		};
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
		
		void updateTweenWeights(InteractionState interactionState, SelectedYN selected, double deltaTime)
		{
			// 現在の状態に対応するTweenのみを1.0、他は0.0に設定
			double targetDefaultWeight = 0.0;
			double targetHoveredWeight = 0.0;
			double targetPressedWeight = 0.0;
			double targetDisabledWeight = 0.0;
			
			// 現在の状態に基づいてweightを設定
			switch (interactionState)
			{
			case InteractionState::Default:
				if (m_tweenValues.get(InteractionState::Default, selected).has_value())
					targetDefaultWeight = 1.0;
				break;
			case InteractionState::Hovered:
				if (m_tweenValues.get(InteractionState::Hovered, selected).has_value())
					targetHoveredWeight = 1.0;
				break;
			case InteractionState::Pressed:
				if (m_tweenValues.get(InteractionState::Pressed, selected).has_value())
					targetPressedWeight = 1.0;
				break;
			case InteractionState::Disabled:
				if (m_tweenValues.get(InteractionState::Disabled, selected).has_value())
					targetDisabledWeight = 1.0;
				break;
			}
			
			// weightを滑らかに更新
			m_tweenWeights.get(InteractionState::Default, selected).update(targetDefaultWeight, m_tweenTransitionTime, deltaTime);
			m_tweenWeights.get(InteractionState::Hovered, selected).update(targetHoveredWeight, m_tweenTransitionTime, deltaTime);
			m_tweenWeights.get(InteractionState::Pressed, selected).update(targetPressedWeight, m_tweenTransitionTime, deltaTime);
			m_tweenWeights.get(InteractionState::Disabled, selected).update(targetDisabledWeight, m_tweenTransitionTime, deltaTime);
		}
		
		[[nodiscard]]
		T calculateBlendedTweenValue() const
		{
			T result{};
			double totalWeight = 0.0;
			
			// 各状態のTweenを重み付きで加算
			auto addWeightedValue =
				[&](InteractionState state, SelectedYN selected)
				{
					if (m_tweenValues.get(state, selected).has_value())
					{
						double weight = m_tweenWeights.get(state, selected).currentValue();
						if (weight > 0.0)
						{
							const double tweenTime = m_tweenStopwatches.get(state, selected).sF();
							T tweenValue = m_tweenValues.get(state, selected)->calculateValue(tweenTime);
							
							if constexpr (std::is_arithmetic_v<T>)
							{
								result += static_cast<T>(tweenValue * weight);
							}
							else
							{
								// ベクトルや色などの場合は要素ごとに重み付き加算
								result = result + tweenValue * weight;
							}
							totalWeight += weight;
						}
					}
				};

			addWeightedValue(InteractionState::Default, SelectedYN::No);
			addWeightedValue(InteractionState::Hovered, SelectedYN::No);
			addWeightedValue(InteractionState::Pressed, SelectedYN::No);
			addWeightedValue(InteractionState::Disabled, SelectedYN::No);
			addWeightedValue(InteractionState::Default, SelectedYN::Yes);
			addWeightedValue(InteractionState::Hovered, SelectedYN::Yes);
			addWeightedValue(InteractionState::Pressed, SelectedYN::Yes);
			addWeightedValue(InteractionState::Disabled, SelectedYN::Yes);

			// 正規化はしない（重みの合計は1とは限らない）
			return result;
		}
		
		[[nodiscard]]
		bool hasTweenWithNonZeroWeight() const
		{
			auto hasNonZeroWeight =
				[&](InteractionState state, SelectedYN selected)
				{
					return m_tweenValues.get(state, selected).has_value() && 
						   m_tweenWeights.get(state, selected).currentValue() > 0.001;
				};
			
			return hasNonZeroWeight(InteractionState::Default, SelectedYN::No) ||
				hasNonZeroWeight(InteractionState::Hovered, SelectedYN::No) ||
				hasNonZeroWeight(InteractionState::Pressed, SelectedYN::No) ||
				hasNonZeroWeight(InteractionState::Disabled, SelectedYN::No) ||
				hasNonZeroWeight(InteractionState::Default, SelectedYN::Yes) ||
				hasNonZeroWeight(InteractionState::Hovered, SelectedYN::Yes) ||
				hasNonZeroWeight(InteractionState::Pressed, SelectedYN::Yes) ||
				hasNonZeroWeight(InteractionState::Disabled, SelectedYN::Yes);
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
			// 現在のTweenソースを取得
			const Optional<InteractionState> currentTweenSource = tweenSourceOf(interactionState, selected);
			
			// 初回update時は、全ての状態のStopwatchを開始
			if (m_prevTweenSource == none)
			{
				m_tweenStopwatches.defaultValue.start();
				m_tweenStopwatches.hoveredValue.start();
				m_tweenStopwatches.pressedValue.start();
				m_tweenStopwatches.disabledValue.start();
				m_tweenStopwatches.selectedDefaultValue.start();
				m_tweenStopwatches.selectedHoveredValue.start();
				m_tweenStopwatches.selectedPressedValue.start();
				m_tweenStopwatches.selectedDisabledValue.start();
			}
			else if (m_prevTweenSource != currentTweenSource)
			{
				// tweenSourceが変わった場合、新しいソースのretrigger設定をチェック
				if (currentTweenSource && m_tweenValues.get(*currentTweenSource, selected).has_value())
				{
					const auto& tween = m_tweenValues.get(*currentTweenSource, selected);
					if (tween->retrigger)
					{
						m_tweenStopwatches.get(*currentTweenSource, selected).restart();
					}
				}
			}
			
			m_prevTweenSource = currentTweenSource;
			
			// 各状態のweightを更新
			updateTweenWeights(interactionState, selected, deltaTime);
			
			// 各状態のTweenを計算してブレンド
			T blendedValue = calculateBlendedTweenValue();
			
			// ブレンドされたTween値があるかどうかチェック
			if (hasTweenWithNonZeroWeight())
			{
				// Tweenの値を直接smoothingに設定
				m_smoothing.setCurrentValue(blendedValue);
			}
			else
			{
				// Tweenがない場合は通常のsmoothing処理
				T targetValue = m_propertyValue.value(interactionState, selected);
				m_smoothing.update(targetValue, m_propertyValue.smoothTime, deltaTime);
			}
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
			
			// tweenTransitionTimeがデフォルト値でない場合はJSONに含める
			if (m_tweenTransitionTime != 0.0)
			{
				// propertyJsonが単純な値の場合、オブジェクトに変換
				if (!propertyJson.isObject())
				{
					JSON newJson;
					newJson[U"default"] = propertyJson;
					propertyJson = newJson;
				}
				
				propertyJson[U"tweenTransitionTime"] = m_tweenTransitionTime;
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
			
			// tweenTransitionTimeの読み込み
			if (propertyJson.contains(U"tweenTransitionTime"))
			{
				m_tweenTransitionTime = propertyJson[U"tweenTransitionTime"].get<double>();
			}
			
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
		
		[[nodiscard]]
		double tweenTransitionTime() const
		{
			return m_tweenTransitionTime;
		}
		
		void setTweenTransitionTime(double tweenTransitionTime)
		{
			m_tweenTransitionTime = tweenTransitionTime;
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
