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
		virtual double tweenTransitionTime() const { return 0.0; }
		virtual void setTweenTransitionTime(double) {}
		virtual Optional<String> tweenValueString(InteractionState, const Array<String>&) const { return none; }
		virtual void setTweenValueString(InteractionState, const Array<String>&, const Optional<String>&) {}
		virtual bool hasTweenOf(InteractionState, const Array<String>&) const { return false; }
		virtual bool hasAnyTween() const { return false; }
		virtual void requestResetTween() {}
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

		/*NonSerialized*/ ShouldResetTweenYN m_shouldResetTween = ShouldResetTweenYN::Yes;

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
		bool hasTweenOf(InteractionState interactionState, const Array<String>& activeStyleStates) const
		{
			return m_tweenValues.get(interactionState, activeStyleStates).has_value();
		}
		
		[[nodiscard]]
		bool hasAnyTween() const override
		{
			Array<String> normalStates{};
			Array<String> selectedStates{ U"selected" };
			for (const auto& activeStyleStates : { normalStates, selectedStates })
			{
				for (const auto state : { InteractionState::Default, InteractionState::Hovered, InteractionState::Pressed, InteractionState::Disabled })
				{
					if (hasTweenOf(state, activeStyleStates))
					{
						return true;
					}
				}
			}
			return false;
		}
		
		[[nodiscard]]
		Optional<InteractionState> tweenSourceOf(InteractionState interactionState, const Array<String>& activeStyleStates) const
		{
			// フォールバック検索
			switch (interactionState)
			{
			case InteractionState::Pressed:
				if (m_tweenValues.get(InteractionState::Pressed, activeStyleStates).has_value()) return InteractionState::Pressed;
				if (m_tweenValues.get(InteractionState::Hovered, activeStyleStates).has_value()) return InteractionState::Hovered;
				if (m_tweenValues.get(InteractionState::Default, activeStyleStates).has_value()) return InteractionState::Default;
				break;
			case InteractionState::Hovered:
				if (m_tweenValues.get(InteractionState::Hovered, activeStyleStates).has_value()) return InteractionState::Hovered;
				if (m_tweenValues.get(InteractionState::Default, activeStyleStates).has_value()) return InteractionState::Default;
				break;
			case InteractionState::Default:
				if (m_tweenValues.get(InteractionState::Default, activeStyleStates).has_value()) return InteractionState::Default;
				break;
			case InteractionState::Disabled:
				if (m_tweenValues.get(InteractionState::Disabled, activeStyleStates).has_value()) return InteractionState::Disabled;
				break;
			}
			return none;
		}
		
		[[nodiscard]]
		Optional<TweenValue<T>> getTweenWithFallback(InteractionState interactionState, const Array<String>& activeStyleStates) const
		{
			// 各状態を順番にチェック
			if (activeStyleStates.contains(U"selected"))
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
		
		void updateTweenWeights(InteractionState interactionState, const Array<String>& activeStyleStates, double deltaTime)
		{
			// 現在の状態に対応するTweenのみを1.0、他は0.0に設定
			double targetDefaultWeight = 0.0;
			double targetHoveredWeight = 0.0;
			double targetPressedWeight = 0.0;
			double targetDisabledWeight = 0.0;
			
			// フォールバックを考慮してweightを設定
			const Optional<InteractionState> tweenSource = tweenSourceOf(interactionState, activeStyleStates);
			if (tweenSource.has_value())
			{
				switch (*tweenSource)
				{
				case InteractionState::Default:
					targetDefaultWeight = 1.0;
					break;
				case InteractionState::Hovered:
					targetHoveredWeight = 1.0;
					break;
				case InteractionState::Pressed:
					targetPressedWeight = 1.0;
					break;
				case InteractionState::Disabled:
					targetDisabledWeight = 1.0;
					break;
				}
			}
			
			// weightを滑らかに更新
			m_tweenWeights.get(InteractionState::Default, activeStyleStates).update(targetDefaultWeight, m_tweenTransitionTime, deltaTime);
			m_tweenWeights.get(InteractionState::Hovered, activeStyleStates).update(targetHoveredWeight, m_tweenTransitionTime, deltaTime);
			m_tweenWeights.get(InteractionState::Pressed, activeStyleStates).update(targetPressedWeight, m_tweenTransitionTime, deltaTime);
			m_tweenWeights.get(InteractionState::Disabled, activeStyleStates).update(targetDisabledWeight, m_tweenTransitionTime, deltaTime);
		}
		
		[[nodiscard]]
		T calculateBlendedTweenValue() const
		{
			T result{};
			double totalWeight = 0.0;
			
			// 各状態のTweenを重み付きで加算
			auto addWeightedValue =
				[&](InteractionState state, const Array<String>& activeStyleStates)
				{
					if (m_tweenValues.get(state, activeStyleStates).has_value())
					{
						double weight = m_tweenWeights.get(state, activeStyleStates).currentValue();
						if (weight > 0.0)
						{
							const double tweenTime = m_tweenStopwatches.get(state, activeStyleStates).sF();
							T tweenValue = m_tweenValues.get(state, activeStyleStates)->calculateValue(tweenTime);
							
							if constexpr (std::is_arithmetic_v<T>)
							{
								result += static_cast<T>(tweenValue * weight);
							}
							else if constexpr (std::is_same_v<T, ColorF>)
							{
								// Siv3DのColorFのoperator+はアルファ値を無視するため、手動で加算が必要
								result.r += tweenValue.r * weight;
								result.g += tweenValue.g * weight;
								result.b += tweenValue.b * weight;
								result.a += tweenValue.a * weight;
							}
							else
							{
								// ベクトルの場合は要素ごとに重み付き加算
								result = result + tweenValue * weight;
							}
							totalWeight += weight;
						}
					}
				};

			Array<String> normalStates{};
			Array<String> selectedStates{ U"selected" };
			
			addWeightedValue(InteractionState::Default, normalStates);
			addWeightedValue(InteractionState::Hovered, normalStates);
			addWeightedValue(InteractionState::Pressed, normalStates);
			addWeightedValue(InteractionState::Disabled, normalStates);
			addWeightedValue(InteractionState::Default, selectedStates);
			addWeightedValue(InteractionState::Hovered, selectedStates);
			addWeightedValue(InteractionState::Pressed, selectedStates);
			addWeightedValue(InteractionState::Disabled, selectedStates);

			// 正規化はしない（重みの合計は1とは限らない）
			return result;
		}
		
		[[nodiscard]]
		bool hasTweenWithNonZeroWeight() const
		{
			auto hasNonZeroWeight =
				[&](InteractionState state, const Array<String>& activeStyleStates)
				{
					return m_tweenValues.get(state, activeStyleStates).has_value() && 
						   m_tweenWeights.get(state, activeStyleStates).currentValue() > 0.001;
				};
			
			Array<String> normalStates{};
			Array<String> selectedStates{ U"selected" };
			
			return hasNonZeroWeight(InteractionState::Default, normalStates) ||
				hasNonZeroWeight(InteractionState::Hovered, normalStates) ||
				hasNonZeroWeight(InteractionState::Pressed, normalStates) ||
				hasNonZeroWeight(InteractionState::Disabled, normalStates) ||
				hasNonZeroWeight(InteractionState::Default, selectedStates) ||
				hasNonZeroWeight(InteractionState::Hovered, selectedStates) ||
				hasNonZeroWeight(InteractionState::Pressed, selectedStates) ||
				hasNonZeroWeight(InteractionState::Disabled, selectedStates);
		}

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
			// 現在のTweenソースを取得
			const Optional<InteractionState> currentTweenSource = tweenSourceOf(interactionState, activeStyleStates);
			
			// Tweenリセット時は、全ての状態のStopwatchを再開始
			if (m_shouldResetTween)
			{
				m_tweenStopwatches.defaultValue.restart();
				m_tweenStopwatches.hoveredValue.restart();
				m_tweenStopwatches.pressedValue.restart();
				m_tweenStopwatches.disabledValue.restart();
				m_tweenStopwatches.selectedDefaultValue.restart();
				m_tweenStopwatches.selectedHoveredValue.restart();
				m_tweenStopwatches.selectedPressedValue.restart();
				m_tweenStopwatches.selectedDisabledValue.restart();
				m_shouldResetTween = ShouldResetTweenYN::No;
			}
			
			if (m_prevTweenSource != currentTweenSource)
			{
				// tweenSourceが変わった場合、新しいソースのrestartsOnEnter設定をチェック
				if (currentTweenSource && m_tweenValues.get(*currentTweenSource, activeStyleStates).has_value())
				{
					const auto& tween = m_tweenValues.get(*currentTweenSource, activeStyleStates);
					if (tween->restartsOnEnter)
					{
						m_tweenStopwatches.get(*currentTweenSource, activeStyleStates).restart();
					}
				}
			}
			
			m_prevTweenSource = currentTweenSource;
			
			// 各状態のweightを更新
			updateTweenWeights(interactionState, activeStyleStates, deltaTime);
			
			// Tweenがある状態の値を計算
			T tweenBlendedValue = calculateBlendedTweenValue();
			double tweenTotalWeight = 0.0;
			
			// 各状態のweightを合計
			Array<String> normalStates{};
			Array<String> selectedStates{ U"selected" };
			for (const auto& currentStates : { normalStates, selectedStates })
			{
				for (const auto state : { InteractionState::Default, InteractionState::Hovered, InteractionState::Pressed, InteractionState::Disabled })
				{
					if (m_tweenValues.get(state, currentStates).has_value())
					{
						tweenTotalWeight += m_tweenWeights.get(state, currentStates).currentValue();
					}
				}
			}
			
			// プロパティ値を取得
			T propertyValue = m_propertyValue.value(interactionState, activeStyleStates);
			
			// Tweenとプロパティ値を線形補間
			T blendedValue;
			if constexpr (std::is_arithmetic_v<T>)
			{
				blendedValue = static_cast<T>(tweenBlendedValue * tweenTotalWeight + propertyValue * (1.0 - tweenTotalWeight));
			}
			else if constexpr (std::is_same_v<T, ColorF>)
			{
				blendedValue.r = tweenBlendedValue.r * tweenTotalWeight + propertyValue.r * (1.0 - tweenTotalWeight);
				blendedValue.g = tweenBlendedValue.g * tweenTotalWeight + propertyValue.g * (1.0 - tweenTotalWeight);
				blendedValue.b = tweenBlendedValue.b * tweenTotalWeight + propertyValue.b * (1.0 - tweenTotalWeight);
				blendedValue.a = tweenBlendedValue.a * tweenTotalWeight + propertyValue.a * (1.0 - tweenTotalWeight);
			}
			else
			{
				blendedValue = tweenBlendedValue * tweenTotalWeight + propertyValue * (1.0 - tweenTotalWeight);
			}
			
			// smoothingに設定
			m_smoothing.setCurrentValue(blendedValue);
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
			m_smoothing = Smoothing<T>{ m_propertyValue.value(InteractionState::Default, Array<String>{}) };
			
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
		
		[[nodiscard]]
		double tweenTransitionTime() const override
		{
			return m_tweenTransitionTime;
		}
		
		void setTweenTransitionTime(double tweenTransitionTime) override
		{
			m_tweenTransitionTime = tweenTransitionTime;
		}
		
		[[nodiscard]]
		Optional<TweenValue<T>> getTweenValue(InteractionState interactionState, const Array<String>& activeStyleStates) const
		{
			return m_tweenValues.get(interactionState, activeStyleStates);
		}
		
		void setTweenValue(InteractionState interactionState, const Array<String>& activeStyleStates, const Optional<TweenValue<T>>& tweenValue)
		{
			m_tweenValues.get(interactionState, activeStyleStates) = tweenValue;
		}
		
		[[nodiscard]]
		Optional<String> tweenValueString(InteractionState interactionState, const Array<String>& activeStyleStates) const override
		{
			const auto& tweenValue = m_tweenValues.get(interactionState, activeStyleStates);
			if (tweenValue.has_value())
			{
				return tweenValue->toJSON().format();
			}
			return none;
		}
		
		void setTweenValueString(InteractionState interactionState, const Array<String>& activeStyleStates, const Optional<String>& jsonString) override
		{
			if (jsonString.has_value())
			{
				const JSON json = JSON::Parse(*jsonString);
				m_tweenValues.get(interactionState, activeStyleStates) = TweenValue<T>::fromJSON(json, T{}, T{});
			}
			else
			{
				m_tweenValues.get(interactionState, activeStyleStates) = none;
			}
		}

		void requestResetTween() override
		{
			m_shouldResetTween = ShouldResetTweenYN::Yes;
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
	};
}
