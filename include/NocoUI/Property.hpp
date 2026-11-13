#pragma once
#include <Siv3D.hpp>
#include "PropertyValue.hpp"
#include "InteractionState.hpp"
#include "YN.hpp"
#include "Smoothing.hpp"
#include "LRTB.hpp"
#include "Param.hpp"

namespace noco
{
	enum class PropertyEditType
	{
		Text,
		Bool,
		Int,
		Double,
		Enum,
		Vec2,
		Color,
		LRTB,
	};

	namespace detail
	{
		/// @brief パラメータ参照名のハッシュ値を保持してパラメータを取得
		[[nodiscard]]
		inline HashTable<String, ParamValue>::const_iterator FindParam(const HashTable<String, ParamValue>& params, const String& paramRef, Optional<size_t>& paramRefHash)
		{
			if (!paramRefHash.has_value())
			{
				paramRefHash = params.hash(paramRef);
			}
			return params.find(paramRef, *paramRefHash);
		}

		// パラメータ参照・currentFrameOverrideの値
		template <class T>
		struct PropertyDetail
		{
			String paramRef; // パラメータ参照名
			ParamRefMode paramRefMode = ParamRefMode::Normal;
			Optional<T> paramRefValue; // OverrideIfTrue/OverrideIfFalseで上書きする値
			/*NonSerialized*/ Optional<size_t> paramRefHash; // パラメータ参照名のハッシュ値
			/*NonSerialized*/ Optional<T> paramRefOverride; // パラメータ参照による上書き
			/*NonSerialized*/ Optional<T> currentFrameOverride;
			/*NonSerialized*/ int32 currentFrameOverrideFrameCount = 0;
		};
	}

	class IProperty
	{
	public:
		virtual ~IProperty() = default;
		virtual StringView name() const = 0;
		virtual void update(InteractionState interactionState, const Array<String>& activeStyleStates, double deltaTime, const HashTable<String, ParamValue>& params, SkipSmoothingYN skipSmoothing) = 0;
		virtual void appendJSON(JSON& json) const = 0;
		virtual void readFromJSON(const JSON& json) = 0;
		virtual String propertyValueStringOfDefault() const = 0;
		virtual Optional<String> propertyValueStringOf(InteractionState interactionState, const Array<String>& activeStyleStates) const = 0;
		virtual String propertyValueStringOfFallback(InteractionState interactionState, const Array<String>& activeStyleStates) const = 0;
		virtual bool trySetPropertyValueString(StringView value) = 0;
		virtual bool trySetPropertyValueStringOf(StringView value, InteractionState interactionState, StringView styleState = U"") = 0;
		virtual void unsetPropertyValueOf(InteractionState interactionState, StringView styleState = U"") = 0;
		virtual bool hasPropertyValueOf(InteractionState interactionState, StringView styleState = U"") const = 0;
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
		virtual const String& paramRef() const = 0;
		virtual void setParamRef(const String& paramRef) = 0;
		virtual bool hasParamRef() const = 0;
		virtual ParamRefMode paramRefMode() const = 0;
		virtual void setParamRefMode(ParamRefMode mode) = 0;
		virtual Array<ParamRefMode> availableParamRefModes() const = 0;
		/// @brief OverrideIfTrue/OverrideIfFalseで上書きする値の文字列を取得(未設定ならnone)
		virtual Optional<String> paramRefValueString() const = 0;
		virtual bool trySetParamRefValueString(StringView value) = 0;
		/// @brief 反映後の値の文字列プレビューを取得(現在のデフォルト値に対して指定モードで指定パラメータ値を適用)。適用不可ならnone
		virtual Optional<String> previewParamRefAppliedString(const ParamValue& paramValue, ParamRefMode mode) const = 0;
		virtual void clearParamRefIfInvalid(const HashTable<String, ParamValue>& validParams, HashSet<String>& clearedParams) = 0;
		virtual void clearCurrentFrameOverride() = 0;
	};

	template <typename T>
	constexpr PropertyEditType PropertyEditTypeOf()
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
		else if constexpr (std::same_as<T, Color> || std::same_as<T, ColorF>)
		{
			return PropertyEditType::Color;
		}
		else if constexpr (std::same_as<T, LRTB>)
		{
			return PropertyEditType::LRTB;
		}
		else if constexpr (std::is_integral_v<T> && !std::same_as<T, bool>)
		{
			// int32, uint32等の整数型
			return PropertyEditType::Int;
		}
		else if constexpr (std::is_floating_point_v<T>)
		{
			// float, double等の浮動小数点型
			return PropertyEditType::Double;
		}
		else
		{
			// String等のテキスト型
			return PropertyEditType::Text;
		}
	}

	/// @brief paramTypeがeditTypeのプロパティからパラメータ参照可能か
	/// @param paramType 参照元のParamType
	/// @param editType 参照先プロパティのPropertyEditType
	[[nodiscard]]
	inline bool IsParamTypeCompatibleWith(ParamType paramType, PropertyEditType editType)
	{
		switch (editType)
		{
		case PropertyEditType::Bool:
			return paramType == ParamType::Bool;
		case PropertyEditType::Int:
		case PropertyEditType::Double:
			// Int/Doubleプロパティは相互に受け付ける
			return paramType == ParamType::Int || paramType == ParamType::Double;
		case PropertyEditType::Text:
			// Stringプロパティは任意の型をFormat()で文字列化して受け入れる
			return paramType != ParamType::Unknown;
		case PropertyEditType::Enum:
			return paramType == ParamType::String;
		case PropertyEditType::Color:
			return paramType == ParamType::Color;
		case PropertyEditType::Vec2:
			return paramType == ParamType::Vec2;
		case PropertyEditType::LRTB:
			return paramType == ParamType::LRTB;
		}
		return false;
	}

	/// @brief paramTypeがeditTypeのプロパティから指定paramRefモードでパラメータ参照可能か
	/// @param paramType 参照元のParamType
	/// @param editType 参照先プロパティのPropertyEditType
	/// @param mode paramRefモード
	[[nodiscard]]
	inline bool IsParamTypeCompatibleWith(ParamType paramType, PropertyEditType editType, ParamRefMode mode)
	{
		if (ParamRefModeRequiresBoolParam(mode))
		{
			return paramType == ParamType::Bool;
		}
		return IsParamTypeCompatibleWith(paramType, editType);
	}

	/// @brief 型Tのプロパティで利用可能なparamRefモード一覧
	template <typename T>
	[[nodiscard]]
	inline Array<ParamRefMode> AvailableParamRefModesForProperty()
	{
		Array<ParamRefMode> modes = AvailableParamRefModesFor<T>();
		if constexpr (!std::is_same_v<T, bool>)
		{
			// Boolプロパティでは通常反映・反転して反映と同じ結果になるため対象外
			modes.push_back(ParamRefMode::OverrideIfTrue);
			modes.push_back(ParamRefMode::OverrideIfFalse);
		}
		return modes;
	}

	/// @brief editTypeのプロパティがパラメータ参照可能な全ParamTypeのリストを返す
	[[nodiscard]]
	inline Array<ParamType> CompatibleParamTypes(PropertyEditType editType)
	{
		constexpr ParamType AllTypes[] = {
			ParamType::Bool,
			ParamType::Int,
			ParamType::Double,
			ParamType::String,
			ParamType::Color,
			ParamType::Vec2,
			ParamType::LRTB,
		};
		Array<ParamType> result;
		for (const auto t : AllTypes)
		{
			if (IsParamTypeCompatibleWith(t, editType))
			{
				result.push_back(t);
			}
		}
		return result;
	}

	template <class T>
	class Property : public IProperty
	{
	private:
		const char32_t* m_name; // 数が多く、基本的にリテラルのみのため、Stringではなくconst char32_t*で持つ
		PropertyValue<T> m_propertyValue;
		/*NonSerialized*/ InteractionState m_interactionState = InteractionState::Default;
		/*NonSerialized*/ Array<String> m_activeStyleStates{};
		std::unique_ptr<detail::PropertyDetail<T>> m_propertyDetail; // パラメータ参照・currentFrameOverrideを使用していない場合はnullptr

		[[nodiscard]]
		detail::PropertyDetail<T>& ensurePropertyDetail()
		{
			if (!m_propertyDetail)
			{
				m_propertyDetail = std::make_unique<detail::PropertyDetail<T>>();
			}
			return *m_propertyDetail;
		}

		// OverrideIfTrue/OverrideIfFalseの場合は上書きする値、それ以外の場合はvalueを返す
		[[nodiscard]]
		const T& paramRefBaseValue(const T& value, ParamRefMode mode) const
		{
			return (ParamRefModeRequiresBoolParam(mode) && m_propertyDetail && m_propertyDetail->paramRefValue) ? *m_propertyDetail->paramRefValue : value;
		}

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
		const String& paramRef() const override
		{
			static const String EmptyString;
			return m_propertyDetail ? m_propertyDetail->paramRef : EmptyString;
		}

		void setParamRef(const String& paramRef) override
		{
			if (!m_propertyDetail && paramRef.isEmpty())
			{
				return;
			}
			auto& propertyDetail = ensurePropertyDetail();
			propertyDetail.paramRef = paramRef;
			propertyDetail.paramRefHash.reset();
		}

		[[nodiscard]]
		bool hasParamRef() const override
		{
			return m_propertyDetail && !m_propertyDetail->paramRef.isEmpty();
		}

		[[nodiscard]]
		ParamRefMode paramRefMode() const override
		{
			return m_propertyDetail ? m_propertyDetail->paramRefMode : ParamRefMode::Normal;
		}

		void setParamRefMode(ParamRefMode mode) override
		{
			if (!m_propertyDetail && mode == ParamRefMode::Normal)
			{
				return;
			}
			ensurePropertyDetail().paramRefMode = mode;
		}

		[[nodiscard]]
		const Optional<T>& paramRefValue() const
		{
			static const Optional<T> NoneValue = none;
			return m_propertyDetail ? m_propertyDetail->paramRefValue : NoneValue;
		}

		void setParamRefValue(const Optional<T>& paramRefValue)
		{
			if (!m_propertyDetail && !paramRefValue)
			{
				return;
			}
			ensurePropertyDetail().paramRefValue = paramRefValue;
		}

		[[nodiscard]]
		Optional<String> paramRefValueString() const override
		{
			if (!m_propertyDetail || !m_propertyDetail->paramRefValue)
			{
				return none;
			}
			if constexpr (std::is_enum_v<T>)
			{
				return EnumToString(*m_propertyDetail->paramRefValue);
			}
			else
			{
				return Format(*m_propertyDetail->paramRefValue);
			}
		}

		bool trySetParamRefValueString(StringView value) override
		{
			const Optional<T> parsedValue = StringToValueOpt<T>(value);
			if (!parsedValue)
			{
				return false;
			}
			ensurePropertyDetail().paramRefValue = *parsedValue;
			return true;
		}

		[[nodiscard]]
		Array<ParamRefMode> availableParamRefModes() const override
		{
			return AvailableParamRefModesForProperty<T>();
		}

		[[nodiscard]]
		Optional<String> previewParamRefAppliedString(const ParamValue& paramValue, ParamRefMode mode) const override
		{
			if constexpr (IsParamSupportedType<T>())
			{
				const T& base = m_propertyValue.value(InteractionState::Default, Array<String>{});
				if (auto resolved = ApplyParamRefMode<T>(paramRefBaseValue(base, mode), paramValue, mode))
				{
					if constexpr (std::is_enum_v<T>)
					{
						return EnumToString(*resolved);
					}
					else
					{
						return Format(*resolved);
					}
				}
			}
			return none;
		}

		void clearParamRefIfInvalid(const HashTable<String, ParamValue>& validParams, HashSet<String>& clearedParams) override
		{
			if (!m_propertyDetail || m_propertyDetail->paramRef.isEmpty())
			{
				return;
			}

			const auto it = validParams.find(m_propertyDetail->paramRef);
			if (it == validParams.end() || !IsParamTypeCompatibleWith(GetParamType(it->second), editType(), m_propertyDetail->paramRefMode))
			{
				clearedParams.insert(m_propertyDetail->paramRef);
				m_propertyDetail->paramRef = U"";
				m_propertyDetail->paramRefHash.reset();
				m_propertyDetail->paramRefMode = ParamRefMode::Normal;
				m_propertyDetail->paramRefValue.reset();
			}
		}

		[[nodiscard]]
		const T& value() const
		{
			if (m_propertyDetail)
			{
				if (m_propertyDetail->currentFrameOverride.has_value() && m_propertyDetail->currentFrameOverrideFrameCount == Scene::FrameCount())
				{
					return *m_propertyDetail->currentFrameOverride;
				}
				if (m_propertyDetail->paramRefOverride.has_value())
				{
					return *m_propertyDetail->paramRefOverride;
				}
			}
			return m_propertyValue.value(m_interactionState, m_activeStyleStates);
		}

		const Optional<T>& currentFrameOverride() const
		{
			static const Optional<T> NoneValue = none;
			if (!hasCurrentFrameOverride())
			{
				return NoneValue;
			}
			return m_propertyDetail->currentFrameOverride;
		}
		
		void setCurrentFrameOverride(const T& value)
		{
			auto& propertyDetail = ensurePropertyDetail();
			propertyDetail.currentFrameOverride = value;
			propertyDetail.currentFrameOverrideFrameCount = Scene::FrameCount();
		}
		
		void clearCurrentFrameOverride() override
		{
			if (m_propertyDetail)
			{
				m_propertyDetail->currentFrameOverride.reset();
			}
		}
		
		[[nodiscard]]
		bool hasCurrentFrameOverride() const
		{
			return m_propertyDetail && m_propertyDetail->currentFrameOverride.has_value() && m_propertyDetail->currentFrameOverrideFrameCount == Scene::FrameCount();
		}

		void update(InteractionState interactionState, const Array<String>& activeStyleStates, double, const HashTable<String, ParamValue>& params, SkipSmoothingYN) override
		{
			m_interactionState = interactionState;
			if (m_activeStyleStates != activeStyleStates)
			{
				m_activeStyleStates = activeStyleStates;
			}

			// パラメータ参照を反映
			if constexpr (IsParamSupportedType<T>())
			{
				if (m_propertyDetail && !m_propertyDetail->paramRef.isEmpty())
				{
					if (auto it = detail::FindParam(params, m_propertyDetail->paramRef, m_propertyDetail->paramRefHash); it != params.end())
					{
						const T& base = m_propertyValue.value(interactionState, activeStyleStates);
						if (auto resolved = ApplyParamRefMode<T>(paramRefBaseValue(base, m_propertyDetail->paramRefMode), it->second, m_propertyDetail->paramRefMode))
						{
							m_propertyDetail->paramRefOverride = std::move(*resolved);
						}
						else
						{
							// パラメータの型が合わない場合はクリア
							m_propertyDetail->paramRefOverride.reset();
						}
					}
					else
					{
						// パラメータが見つからない場合はクリア
						m_propertyDetail->paramRefOverride.reset();
					}
				}
				else if (m_propertyDetail)
				{
					// パラメータ参照がない場合はクリア
					m_propertyDetail->paramRefOverride.reset();
				}
			}
		}

		void appendJSON(JSON& json) const override
		{
			json[m_name] = m_propertyValue.toJSON();
			if (m_propertyDetail && !m_propertyDetail->paramRef.isEmpty())
			{
				json[String(m_name) + U"_paramRef"] = m_propertyDetail->paramRef;
				if (m_propertyDetail->paramRefMode != ParamRefMode::Normal)
				{
					json[String(m_name) + U"_paramRefMode"] = EnumToString(m_propertyDetail->paramRefMode);
				}
				if (ParamRefModeRequiresBoolParam(m_propertyDetail->paramRefMode) && m_propertyDetail->paramRefValue)
				{
					json[String(m_name) + U"_paramRefValue"] = PropertyValue<T>{ *m_propertyDetail->paramRefValue }.toJSON();
				}
			}
		}

		void readFromJSON(const JSON& json) override
		{
			if (!json.contains(m_name))
			{
				return;
			}
			m_propertyValue = PropertyValue<T>::FromJSON(json[m_name]);

			const String paramRefKey = String(m_name) + U"_paramRef";
			if (json.contains(paramRefKey))
			{
				setParamRef(json[paramRefKey].getString());
			}
			const String paramRefValueKey = String(m_name) + U"_paramRefValue";
			if (json.contains(paramRefValueKey))
			{
				ensurePropertyDetail().paramRefValue = PropertyValue<T>::FromJSON(json[paramRefValueKey]).defaultValue();
			}
			else if (m_propertyDetail)
			{
				m_propertyDetail->paramRefValue.reset();
			}
			const String paramRefModeKey = String(m_name) + U"_paramRefMode";
			if (json.contains(paramRefModeKey))
			{
				if (auto opt = StringToEnumOpt<ParamRefMode>(json[paramRefModeKey].getString()))
				{
					setParamRefMode(ValidateParamRefModeFromJSON(*opt, AvailableParamRefModesForProperty<T>(), name()));
				}
			}
			else
			{
				setParamRefMode(ParamRefMode::Normal);
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

		bool trySetPropertyValueStringOf(StringView value, InteractionState interactionState, StringView styleState = U"") override
		{
			return m_propertyValue.trySetValueStringOf(value, interactionState, styleState);
		}

		void unsetPropertyValueOf(InteractionState interactionState, StringView styleState = U"") override
		{
			m_propertyValue.unsetValueOf(interactionState, styleState);
		}

		[[nodiscard]]
		bool hasPropertyValueOf(InteractionState interactionState, StringView styleState = U"") const override
		{
			return m_propertyValue.hasValueOf(interactionState, styleState);
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
			return m_propertyValue.smoothTime();
		}

		bool trySetSmoothTime(double smoothTime) override
		{
			m_propertyValue.setSmoothTime(smoothTime);
			return true;
		}

		Array<String> styleStateKeys() const override
		{
			Array<String> result;
			
			// styleStateValuesから収集
			if (m_propertyValue.styleStateValues())
			{
				for (const auto& [state, value] : *m_propertyValue.styleStateValues())
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
	class SmoothProperty : public IProperty
	{
		static_assert(!std::is_same_v<T, bool>, "SmoothProperty<bool> is not allowed");
		static_assert(!std::is_enum_v<T>, "SmoothProperty does not support enum type");
		static_assert(HasSmoothDamp<T> || HasSmoothDampInner<T>, "T must have Math::SmoothDamp");

	private:
		const char32_t* m_name; // 数が多く、基本的にリテラルのみのため、Stringではなくconst char32_t*で持つ
		PropertyValue<T> m_propertyValue;
		/*NonSerialized*/ InteractionState m_interactionState = InteractionState::Default;
		/*NonSerialized*/ Array<String> m_activeStyleStates{};
		std::unique_ptr<detail::PropertyDetail<T>> m_propertyDetail; // パラメータ参照・currentFrameOverrideを使用していない場合はnullptr

		[[nodiscard]]
		detail::PropertyDetail<T>& ensurePropertyDetail()
		{
			if (!m_propertyDetail)
			{
				m_propertyDetail = std::make_unique<detail::PropertyDetail<T>>();
			}
			return *m_propertyDetail;
		}
		/*NonSerialized*/ Smoothing<T> m_smoothing;

		// OverrideIfTrue/OverrideIfFalseの場合は上書きする値、それ以外の場合はvalueを返す
		[[nodiscard]]
		const T& paramRefBaseValue(const T& value, ParamRefMode mode) const
		{
			return (ParamRefModeRequiresBoolParam(mode) && m_propertyDetail && m_propertyDetail->paramRefValue) ? *m_propertyDetail->paramRefValue : value;
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
			if (m_propertyValue.smoothTime() <= 0.0)
			{
				// Canvas更新より後にsetPropertyValueされた場合に1フレーム遅れが発生しないよう、smoothTimeが0なら即座に反映
				m_smoothing.setCurrentValue(m_propertyValue.value(m_interactionState, m_activeStyleStates));
			}
		}

		[[nodiscard]]
		const String& paramRef() const override
		{
			static const String EmptyString;
			return m_propertyDetail ? m_propertyDetail->paramRef : EmptyString;
		}

		void setParamRef(const String& paramRef) override
		{
			if (!m_propertyDetail && paramRef.isEmpty())
			{
				return;
			}
			auto& propertyDetail = ensurePropertyDetail();
			propertyDetail.paramRef = paramRef;
			propertyDetail.paramRefHash.reset();
		}

		[[nodiscard]]
		bool hasParamRef() const override
		{
			return m_propertyDetail && !m_propertyDetail->paramRef.isEmpty();
		}

		[[nodiscard]]
		ParamRefMode paramRefMode() const override
		{
			return m_propertyDetail ? m_propertyDetail->paramRefMode : ParamRefMode::Normal;
		}

		void setParamRefMode(ParamRefMode mode) override
		{
			if (!m_propertyDetail && mode == ParamRefMode::Normal)
			{
				return;
			}
			ensurePropertyDetail().paramRefMode = mode;
		}

		[[nodiscard]]
		const Optional<T>& paramRefValue() const
		{
			static const Optional<T> NoneValue = none;
			return m_propertyDetail ? m_propertyDetail->paramRefValue : NoneValue;
		}

		void setParamRefValue(const Optional<T>& paramRefValue)
		{
			if (!m_propertyDetail && !paramRefValue)
			{
				return;
			}
			ensurePropertyDetail().paramRefValue = paramRefValue;
		}

		[[nodiscard]]
		Optional<String> paramRefValueString() const override
		{
			if (!m_propertyDetail || !m_propertyDetail->paramRefValue)
			{
				return none;
			}
			return Format(*m_propertyDetail->paramRefValue);
		}

		bool trySetParamRefValueString(StringView value) override
		{
			const Optional<T> parsedValue = StringToValueOpt<T>(value);
			if (!parsedValue)
			{
				return false;
			}
			ensurePropertyDetail().paramRefValue = *parsedValue;
			return true;
		}

		[[nodiscard]]
		Array<ParamRefMode> availableParamRefModes() const override
		{
			return AvailableParamRefModesForProperty<T>();
		}

		[[nodiscard]]
		Optional<String> previewParamRefAppliedString(const ParamValue& paramValue, ParamRefMode mode) const override
		{
			if constexpr (IsParamSupportedType<T>())
			{
				const T& base = m_propertyValue.value(InteractionState::Default, Array<String>{});
				if (auto resolved = ApplyParamRefMode<T>(paramRefBaseValue(base, mode), paramValue, mode))
				{
					return Format(*resolved);
				}
			}
			return none;
		}

		void clearParamRefIfInvalid(const HashTable<String, ParamValue>& validParams, HashSet<String>& clearedParams) override
		{
			if (!m_propertyDetail || m_propertyDetail->paramRef.isEmpty())
			{
				return;
			}

			const auto it = validParams.find(m_propertyDetail->paramRef);
			if (it == validParams.end() || !IsParamTypeCompatibleWith(GetParamType(it->second), editType(), m_propertyDetail->paramRefMode))
			{
				clearedParams.insert(m_propertyDetail->paramRef);
				m_propertyDetail->paramRef = U"";
				m_propertyDetail->paramRefHash.reset();
				m_propertyDetail->paramRefMode = ParamRefMode::Normal;
				m_propertyDetail->paramRefValue.reset();
			}
		}

		[[nodiscard]]
		const T& value() const
		{
			if (m_propertyDetail)
			{
				if (m_propertyDetail->currentFrameOverride.has_value() && m_propertyDetail->currentFrameOverrideFrameCount == Scene::FrameCount())
				{
					return *m_propertyDetail->currentFrameOverride;
				}
				if (m_propertyDetail->paramRefOverride.has_value())
				{
					return *m_propertyDetail->paramRefOverride;
				}
			}
			return m_smoothing.currentValue();
		}

		void update(InteractionState interactionState, const Array<String>& activeStyleStates, double deltaTime, const HashTable<String, ParamValue>& params, SkipSmoothingYN skipSmoothing) override
		{
			m_interactionState = interactionState;
			if (m_activeStyleStates != activeStyleStates)
			{
				m_activeStyleStates = activeStyleStates;
			}

			// スムージング適用
			if (skipSmoothing)
			{
				m_smoothing.setCurrentValue(m_propertyValue.value(interactionState, activeStyleStates));
			}
			else
			{
				m_smoothing.update(m_propertyValue.value(interactionState, activeStyleStates), m_propertyValue.smoothTime(), deltaTime);
			}

			// パラメータ参照適用
			if constexpr (IsParamSupportedType<T>())
			{
				if (m_propertyDetail && !m_propertyDetail->paramRef.isEmpty())
				{
					if (auto it = detail::FindParam(params, m_propertyDetail->paramRef, m_propertyDetail->paramRefHash); it != params.end())
					{
						if (auto resolved = ApplyParamRefMode<T>(paramRefBaseValue(m_smoothing.currentValue(), m_propertyDetail->paramRefMode), it->second, m_propertyDetail->paramRefMode))
						{
							m_propertyDetail->paramRefOverride = *resolved;
						}
						else
						{
							// パラメータの型が合わない場合はクリア
							m_propertyDetail->paramRefOverride.reset();
						}
					}
					else
					{
						// パラメータが見つからない場合はクリア
						m_propertyDetail->paramRefOverride.reset();
					}
				}
				else if (m_propertyDetail)
				{
					// パラメータ参照がない場合はクリア
					m_propertyDetail->paramRefOverride.reset();
				}
			}
		}

		const Optional<T>& currentFrameOverride() const
		{
			static const Optional<T> NoneValue = none;
			if (!hasCurrentFrameOverride())
			{
				return NoneValue;
			}
			return m_propertyDetail->currentFrameOverride;
		}

		void setCurrentFrameOverride(const T& value)
		{
			auto& propertyDetail = ensurePropertyDetail();
			propertyDetail.currentFrameOverride = value;
			propertyDetail.currentFrameOverrideFrameCount = Scene::FrameCount();
			m_smoothing.setCurrentValue(value);
		}
		
		void clearCurrentFrameOverride() override
		{
			if (m_propertyDetail)
			{
				m_propertyDetail->currentFrameOverride.reset();
			}
		}
		
		[[nodiscard]]
		bool hasCurrentFrameOverride() const
		{
			return m_propertyDetail && m_propertyDetail->currentFrameOverride.has_value() && m_propertyDetail->currentFrameOverrideFrameCount == Scene::FrameCount();
		}

		void appendJSON(JSON& json) const override
		{
			json[m_name] = m_propertyValue.toJSON();
			if (m_propertyDetail && !m_propertyDetail->paramRef.isEmpty())
			{
				json[String(m_name) + U"_paramRef"] = m_propertyDetail->paramRef;
				if (m_propertyDetail->paramRefMode != ParamRefMode::Normal)
				{
					json[String(m_name) + U"_paramRefMode"] = EnumToString(m_propertyDetail->paramRefMode);
				}
				if (ParamRefModeRequiresBoolParam(m_propertyDetail->paramRefMode) && m_propertyDetail->paramRefValue)
				{
					json[String(m_name) + U"_paramRefValue"] = PropertyValue<T>{ *m_propertyDetail->paramRefValue }.toJSON();
				}
			}
		}

		void readFromJSON(const JSON& json) override
		{
			if (!json.contains(m_name))
			{
				return;
			}
			m_propertyValue = PropertyValue<T>::FromJSON(json[m_name]);
			m_smoothing = Smoothing<T>{ m_propertyValue.value(InteractionState::Default, Array<String>{}) };

			const String paramRefKey = String(m_name) + U"_paramRef";
			if (json.contains(paramRefKey))
			{
				setParamRef(json[paramRefKey].getString());
			}
			const String paramRefValueKey = String(m_name) + U"_paramRefValue";
			if (json.contains(paramRefValueKey))
			{
				ensurePropertyDetail().paramRefValue = PropertyValue<T>::FromJSON(json[paramRefValueKey]).defaultValue();
			}
			else if (m_propertyDetail)
			{
				m_propertyDetail->paramRefValue.reset();
			}
			const String paramRefModeKey = String(m_name) + U"_paramRefMode";
			if (json.contains(paramRefModeKey))
			{
				if (auto opt = StringToEnumOpt<ParamRefMode>(json[paramRefModeKey].getString()))
				{
					setParamRefMode(ValidateParamRefModeFromJSON(*opt, AvailableParamRefModesForProperty<T>(), name()));
				}
			}
			else
			{
				setParamRefMode(ParamRefMode::Normal);
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

		bool trySetPropertyValueStringOf(StringView value, InteractionState interactionState, StringView styleState = U"") override
		{
			return m_propertyValue.trySetValueStringOf(value, interactionState, styleState);
		}

		void unsetPropertyValueOf(InteractionState interactionState, StringView styleState = U"") override
		{
			m_propertyValue.unsetValueOf(interactionState, styleState);
		}

		[[nodiscard]]
		bool hasPropertyValueOf(InteractionState interactionState, StringView styleState = U"") const override
		{
			return m_propertyValue.hasValueOf(interactionState, styleState);
		}

		[[nodiscard]]
		PropertyEditType editType() const override
		{
			return PropertyEditTypeOf<T>();
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
			return m_propertyValue.smoothTime();
		}

		bool trySetSmoothTime(double smoothTime) override
		{
			m_propertyValue.setSmoothTime(smoothTime);
			return true;
		}

		Array<String> styleStateKeys() const override
		{
			Array<String> result;
			
			// styleStateValuesから収集
			if (m_propertyValue.styleStateValues())
			{
				for (const auto& [state, value] : *m_propertyValue.styleStateValues())
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
		std::unique_ptr<detail::PropertyDetail<T>> m_propertyDetail; // パラメータ参照・currentFrameOverrideを使用していない場合はnullptr

		[[nodiscard]]
		detail::PropertyDetail<T>& ensurePropertyDetail()
		{
			if (!m_propertyDetail)
			{
				m_propertyDetail = std::make_unique<detail::PropertyDetail<T>>();
			}
			return *m_propertyDetail;
		}

		// OverrideIfTrue/OverrideIfFalseの場合は上書きする値、それ以外の場合はvalueを返す
		[[nodiscard]]
		const T& paramRefBaseValue(const T& value, ParamRefMode mode) const
		{
			return (ParamRefModeRequiresBoolParam(mode) && m_propertyDetail && m_propertyDetail->paramRefValue) ? *m_propertyDetail->paramRefValue : value;
		}

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
		const String& paramRef() const override
		{
			static const String EmptyString;
			return m_propertyDetail ? m_propertyDetail->paramRef : EmptyString;
		}

		void setParamRef(const String& paramRef) override
		{
			if (!m_propertyDetail && paramRef.isEmpty())
			{
				return;
			}
			auto& propertyDetail = ensurePropertyDetail();
			propertyDetail.paramRef = paramRef;
			propertyDetail.paramRefHash.reset();
		}

		[[nodiscard]]
		bool hasParamRef() const override
		{
			return m_propertyDetail && !m_propertyDetail->paramRef.isEmpty();
		}

		[[nodiscard]]
		ParamRefMode paramRefMode() const override
		{
			return m_propertyDetail ? m_propertyDetail->paramRefMode : ParamRefMode::Normal;
		}

		void setParamRefMode(ParamRefMode mode) override
		{
			if (!m_propertyDetail && mode == ParamRefMode::Normal)
			{
				return;
			}
			ensurePropertyDetail().paramRefMode = mode;
		}

		[[nodiscard]]
		const Optional<T>& paramRefValue() const
		{
			static const Optional<T> NoneValue = none;
			return m_propertyDetail ? m_propertyDetail->paramRefValue : NoneValue;
		}

		void setParamRefValue(const Optional<T>& paramRefValue)
		{
			if (!m_propertyDetail && !paramRefValue)
			{
				return;
			}
			ensurePropertyDetail().paramRefValue = paramRefValue;
		}

		[[nodiscard]]
		Optional<String> paramRefValueString() const override
		{
			if (!m_propertyDetail || !m_propertyDetail->paramRefValue)
			{
				return none;
			}
			if constexpr (std::is_enum_v<T>)
			{
				return EnumToString(*m_propertyDetail->paramRefValue);
			}
			else
			{
				return Format(*m_propertyDetail->paramRefValue);
			}
		}

		bool trySetParamRefValueString(StringView value) override
		{
			const Optional<T> parsedValue = StringToValueOpt<T>(value);
			if (!parsedValue)
			{
				return false;
			}
			ensurePropertyDetail().paramRefValue = *parsedValue;
			return true;
		}

		[[nodiscard]]
		Array<ParamRefMode> availableParamRefModes() const override
		{
			return AvailableParamRefModesForProperty<T>();
		}

		[[nodiscard]]
		Optional<String> previewParamRefAppliedString(const ParamValue& paramValue, ParamRefMode mode) const override
		{
			if constexpr (IsParamSupportedType<T>())
			{
				if (auto resolved = ApplyParamRefMode<T>(paramRefBaseValue(m_value, mode), paramValue, mode))
				{
					if constexpr (std::is_enum_v<T>)
					{
						return EnumToString(*resolved);
					}
					else
					{
						return Format(*resolved);
					}
				}
			}
			return none;
		}

		void clearParamRefIfInvalid(const HashTable<String, ParamValue>& validParams, HashSet<String>& clearedParams) override
		{
			if (!m_propertyDetail || m_propertyDetail->paramRef.isEmpty())
			{
				return;
			}

			const auto it = validParams.find(m_propertyDetail->paramRef);
			if (it == validParams.end() || !IsParamTypeCompatibleWith(GetParamType(it->second), editType(), m_propertyDetail->paramRefMode))
			{
				clearedParams.insert(m_propertyDetail->paramRef);
				m_propertyDetail->paramRef = U"";
				m_propertyDetail->paramRefHash.reset();
				m_propertyDetail->paramRefMode = ParamRefMode::Normal;
				m_propertyDetail->paramRefValue.reset();
			}
		}

		[[nodiscard]]
		const T& value() const
		{
			if (m_propertyDetail)
			{
				if (m_propertyDetail->currentFrameOverride.has_value() && m_propertyDetail->currentFrameOverrideFrameCount == Scene::FrameCount())
				{
					return *m_propertyDetail->currentFrameOverride;
				}
				if (m_propertyDetail->paramRefOverride.has_value())
				{
					return *m_propertyDetail->paramRefOverride;
				}
			}
			return m_value;
		}

		[[nodiscard]]
		const T& propertyValue() const
		{
			return m_value;
		}

		const Optional<T>& currentFrameOverride() const
		{
			static const Optional<T> NoneValue = none;
			if (!hasCurrentFrameOverride())
			{
				return NoneValue;
			}
			return m_propertyDetail->currentFrameOverride;
		}

		void setCurrentFrameOverride(const T& value)
		{
			auto& propertyDetail = ensurePropertyDetail();
			propertyDetail.currentFrameOverride = value;
			propertyDetail.currentFrameOverrideFrameCount = Scene::FrameCount();
		}
		
		void clearCurrentFrameOverride() override
		{
			if (m_propertyDetail)
			{
				m_propertyDetail->currentFrameOverride.reset();
			}
		}
		
		[[nodiscard]]
		bool hasCurrentFrameOverride() const
		{
			return m_propertyDetail && m_propertyDetail->currentFrameOverride.has_value() && m_propertyDetail->currentFrameOverrideFrameCount == Scene::FrameCount();
		}

		void update(InteractionState interactionState, const Array<String>& activeStyleStates, double, const HashTable<String, ParamValue>& params, SkipSmoothingYN) override
		{
			m_interactionState = interactionState;
			if (m_activeStyleStates != activeStyleStates)
			{
				m_activeStyleStates = activeStyleStates;
			}

			// パラメータ参照を反映
			if constexpr (IsParamSupportedType<T>())
			{
				if (m_propertyDetail && !m_propertyDetail->paramRef.isEmpty())
				{
					if (auto it = detail::FindParam(params, m_propertyDetail->paramRef, m_propertyDetail->paramRefHash); it != params.end())
					{
						if (auto resolved = ApplyParamRefMode<T>(paramRefBaseValue(m_value, m_propertyDetail->paramRefMode), it->second, m_propertyDetail->paramRefMode))
						{
							m_propertyDetail->paramRefOverride = std::move(*resolved);
						}
						else
						{
							// パラメータの型が合わない場合はクリア
							m_propertyDetail->paramRefOverride.reset();
						}
					}
					else
					{
						// パラメータが見つからない場合はクリア
						m_propertyDetail->paramRefOverride.reset();
					}
				}
				else if (m_propertyDetail)
				{
					// パラメータ参照がない場合はクリア
					m_propertyDetail->paramRefOverride.reset();
				}
			}
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
			else if constexpr (std::same_as<T, Vec2> || std::same_as<T, Color>)
			{
				json[m_name] = ToArrayJSON<T>(m_value);
			}
			else
			{
				json[m_name] = m_value;
			}
			if (m_propertyDetail && !m_propertyDetail->paramRef.isEmpty())
			{
				json[String(m_name) + U"_paramRef"] = m_propertyDetail->paramRef;
				if (m_propertyDetail->paramRefMode != ParamRefMode::Normal)
				{
					json[String(m_name) + U"_paramRefMode"] = EnumToString(m_propertyDetail->paramRefMode);
				}
				if (ParamRefModeRequiresBoolParam(m_propertyDetail->paramRefMode) && m_propertyDetail->paramRefValue)
				{
					json[String(m_name) + U"_paramRefValue"] = PropertyValue<T>{ *m_propertyDetail->paramRefValue }.toJSON();
				}
			}
		}

		void readFromJSON(const JSON& json) override
		{
			if (!json.contains(m_name))
			{
				return;
			}

			// Propertyが後からPropertyNonInteractiveに変更される場合を考慮して、PropertyValue<T>::FromJSONを使う
			m_value = PropertyValue<T>::FromJSON(json[m_name]).defaultValue();

			const String paramRefKey = String(m_name) + U"_paramRef";
			if (json.contains(paramRefKey))
			{
				setParamRef(json[paramRefKey].getString());
			}
			const String paramRefValueKey = String(m_name) + U"_paramRefValue";
			if (json.contains(paramRefValueKey))
			{
				ensurePropertyDetail().paramRefValue = PropertyValue<T>::FromJSON(json[paramRefValueKey]).defaultValue();
			}
			else if (m_propertyDetail)
			{
				m_propertyDetail->paramRefValue.reset();
			}
			const String paramRefModeKey = String(m_name) + U"_paramRefMode";
			if (json.contains(paramRefModeKey))
			{
				if (auto opt = StringToEnumOpt<ParamRefMode>(json[paramRefModeKey].getString()))
				{
					setParamRefMode(ValidateParamRefModeFromJSON(*opt, AvailableParamRefModesForProperty<T>(), name()));
				}
			}
			else
			{
				setParamRefMode(ParamRefMode::Normal);
			}
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

		bool trySetPropertyValueStringOf(StringView, InteractionState, StringView = U"") override
		{
			throw Error{ U"trySetPropertyValueStringOf() called for non-interactive property" };
		}

		void unsetPropertyValueOf(InteractionState, StringView = U"") override
		{
			// NonInteractiveプロパティでは何もしない
		}

		[[nodiscard]]
		bool hasPropertyValueOf(InteractionState interactionState, StringView styleState = U"") const override
		{
			if (interactionState == InteractionState::Default && styleState.isEmpty())
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

	// SmoothProperty<Color>の特殊化
	// 内部的にColorFで補間計算を行い、外部インターフェースはColorで統一
	template <>
	class SmoothProperty<Color> : public IProperty
	{
	private:
		const char32_t* m_name;
		PropertyValue<Color> m_propertyValue;
		/*NonSerialized*/ InteractionState m_interactionState = InteractionState::Default;
		/*NonSerialized*/ Array<String> m_activeStyleStates{};
		std::unique_ptr<detail::PropertyDetail<Color>> m_propertyDetail; // パラメータ参照・currentFrameOverrideを使用していない場合はnullptr

		[[nodiscard]]
		detail::PropertyDetail<Color>& ensurePropertyDetail()
		{
			if (!m_propertyDetail)
			{
				m_propertyDetail = std::make_unique<detail::PropertyDetail<Color>>();
			}
			return *m_propertyDetail;
		}
		/*NonSerialized*/ Smoothing<ColorF> m_smoothing; // ColorFで補間

		// OverrideIfTrue/OverrideIfFalseの場合は上書きする値、それ以外の場合はvalueを返す
		[[nodiscard]]
		const Color& paramRefBaseValue(const Color& value, ParamRefMode mode) const
		{
			return (ParamRefModeRequiresBoolParam(mode) && m_propertyDetail && m_propertyDetail->paramRefValue) ? *m_propertyDetail->paramRefValue : value;
		}

	public:
		SmoothProperty(const char32_t* name, const PropertyValue<Color>& propertyValue)
			: m_name{ name }
			, m_propertyValue{ propertyValue }
			, m_smoothing{ ColorF{ propertyValue.value(InteractionState::Default, Array<String>{}) } }
		{
		}

		template <class U>
		SmoothProperty(const char32_t* name, const U& defaultValue) requires std::convertible_to<U, Color>
			: m_name{ name }
			, m_propertyValue{ defaultValue }
			, m_smoothing{ ColorF{ Color{ defaultValue } } }
		{
		}

		[[nodiscard]]
		StringView name() const override
		{
			return m_name;
		}

		[[nodiscard]]
		const PropertyValue<Color>& propertyValue() const
		{
			return m_propertyValue;
		}

		[[nodiscard]]
		const Color& propertyValue(InteractionState interactionState, const Array<String>& activeStyleStates) const
		{
			return m_propertyValue.value(interactionState, activeStyleStates);
		}

		void setPropertyValue(const PropertyValue<Color>& propertyValue)
		{
			m_propertyValue = propertyValue;
			if (m_propertyValue.smoothTime() <= 0.0)
			{
				m_smoothing.setCurrentValue(ColorF{ m_propertyValue.value(m_interactionState, m_activeStyleStates) });
			}
		}

		[[nodiscard]]
		const String& paramRef() const override
		{
			static const String EmptyString;
			return m_propertyDetail ? m_propertyDetail->paramRef : EmptyString;
		}

		void setParamRef(const String& paramRef) override
		{
			if (!m_propertyDetail && paramRef.isEmpty())
			{
				return;
			}
			auto& propertyDetail = ensurePropertyDetail();
			propertyDetail.paramRef = paramRef;
			propertyDetail.paramRefHash.reset();
		}

		[[nodiscard]]
		bool hasParamRef() const override
		{
			return m_propertyDetail && !m_propertyDetail->paramRef.isEmpty();
		}

		[[nodiscard]]
		ParamRefMode paramRefMode() const override
		{
			return m_propertyDetail ? m_propertyDetail->paramRefMode : ParamRefMode::Normal;
		}

		void setParamRefMode(ParamRefMode mode) override
		{
			if (!m_propertyDetail && mode == ParamRefMode::Normal)
			{
				return;
			}
			ensurePropertyDetail().paramRefMode = mode;
		}

		[[nodiscard]]
		const Optional<Color>& paramRefValue() const
		{
			static const Optional<Color> NoneValue = none;
			return m_propertyDetail ? m_propertyDetail->paramRefValue : NoneValue;
		}

		void setParamRefValue(const Optional<Color>& paramRefValue)
		{
			if (!m_propertyDetail && !paramRefValue)
			{
				return;
			}
			ensurePropertyDetail().paramRefValue = paramRefValue;
		}

		[[nodiscard]]
		Optional<String> paramRefValueString() const override
		{
			if (!m_propertyDetail || !m_propertyDetail->paramRefValue)
			{
				return none;
			}
			return Format(*m_propertyDetail->paramRefValue);
		}

		bool trySetParamRefValueString(StringView value) override
		{
			const Optional<Color> parsedValue = StringToValueOpt<Color>(value);
			if (!parsedValue)
			{
				return false;
			}
			ensurePropertyDetail().paramRefValue = *parsedValue;
			return true;
		}

		[[nodiscard]]
		Array<ParamRefMode> availableParamRefModes() const override
		{
			return AvailableParamRefModesForProperty<Color>();
		}

		[[nodiscard]]
		Optional<String> previewParamRefAppliedString(const ParamValue& paramValue, ParamRefMode mode) const override
		{
			const Color base = m_propertyValue.value(InteractionState::Default, Array<String>{});
			if (auto resolved = ApplyParamRefMode<Color>(paramRefBaseValue(base, mode), paramValue, mode))
			{
				return Format(*resolved);
			}
			return none;
		}

		void clearParamRefIfInvalid(const HashTable<String, ParamValue>& validParams, HashSet<String>& clearedParams) override
		{
			if (!m_propertyDetail || m_propertyDetail->paramRef.isEmpty())
			{
				return;
			}

			const auto it = validParams.find(m_propertyDetail->paramRef);
			if (it == validParams.end() || !IsParamTypeCompatibleWith(GetParamType(it->second), editType(), m_propertyDetail->paramRefMode))
			{
				clearedParams.insert(m_propertyDetail->paramRef);
				m_propertyDetail->paramRef = U"";
				m_propertyDetail->paramRefHash.reset();
				m_propertyDetail->paramRefMode = ParamRefMode::Normal;
				m_propertyDetail->paramRefValue.reset();
			}
		}

		[[nodiscard]]
		Color value() const
		{
			if (m_propertyDetail)
			{
				if (m_propertyDetail->currentFrameOverride.has_value() && m_propertyDetail->currentFrameOverrideFrameCount == Scene::FrameCount())
				{
					return *m_propertyDetail->currentFrameOverride;
				}
				if (m_propertyDetail->paramRefOverride.has_value())
				{
					return *m_propertyDetail->paramRefOverride;
				}
			}
			return Color{ m_smoothing.currentValue() };
		}

		void update(InteractionState interactionState, const Array<String>& activeStyleStates, double deltaTime, const HashTable<String, ParamValue>& params, SkipSmoothingYN skipSmoothing) override
		{
			m_interactionState = interactionState;
			if (m_activeStyleStates != activeStyleStates)
			{
				m_activeStyleStates = activeStyleStates;
			}

			// スムージング適用
			const Color targetColor = m_propertyValue.value(interactionState, activeStyleStates);
			const ColorF targetColorF{ targetColor };
			if (skipSmoothing)
			{
				m_smoothing.setCurrentValue(targetColorF);
			}
			else
			{
				m_smoothing.update(targetColorF, m_propertyValue.smoothTime(), deltaTime);
			}

			// パラメータ参照適用
			if (m_propertyDetail && !m_propertyDetail->paramRef.isEmpty())
			{
				if (auto it = detail::FindParam(params, m_propertyDetail->paramRef, m_propertyDetail->paramRefHash); it != params.end())
				{
					if (auto applied = ApplyParamRefMode<Color>(paramRefBaseValue(Color{ m_smoothing.currentValue() }, m_propertyDetail->paramRefMode), it->second, m_propertyDetail->paramRefMode))
					{
						m_propertyDetail->paramRefOverride = *applied;
					}
					else
					{
						// パラメータの型が合わない場合はクリア
						m_propertyDetail->paramRefOverride.reset();
					}
				}
				else
				{
					// パラメータが見つからない場合はクリア
					m_propertyDetail->paramRefOverride.reset();
				}
			}
			else if (m_propertyDetail)
			{
				// パラメータ参照がない場合はクリア
				m_propertyDetail->paramRefOverride.reset();
			}
		}

		const Optional<Color>& currentFrameOverride() const
		{
			static const Optional<Color> NoneValue = none;
			if (!hasCurrentFrameOverride())
			{
				return NoneValue;
			}
			return m_propertyDetail->currentFrameOverride;
		}

		void setCurrentFrameOverride(const Color& value)
		{
			auto& propertyDetail = ensurePropertyDetail();
			propertyDetail.currentFrameOverride = value;
			propertyDetail.currentFrameOverrideFrameCount = Scene::FrameCount();
		}

		[[nodiscard]]
		bool hasCurrentFrameOverride() const
		{
			return m_propertyDetail && m_propertyDetail->currentFrameOverride.has_value() && m_propertyDetail->currentFrameOverrideFrameCount == Scene::FrameCount();
		}

		void clearCurrentFrameOverride() override
		{
			if (m_propertyDetail)
			{
				m_propertyDetail->currentFrameOverride.reset();
			}
		}

		void appendJSON(JSON& json) const override
		{
			json[m_name] = m_propertyValue.toJSON();
			if (m_propertyDetail && !m_propertyDetail->paramRef.isEmpty())
			{
				json[String{ m_name } + U"_paramRef"] = m_propertyDetail->paramRef;
				if (m_propertyDetail->paramRefMode != ParamRefMode::Normal)
				{
					json[String{ m_name } + U"_paramRefMode"] = EnumToString(m_propertyDetail->paramRefMode);
				}
				if (ParamRefModeRequiresBoolParam(m_propertyDetail->paramRefMode) && m_propertyDetail->paramRefValue)
				{
					json[String{ m_name } + U"_paramRefValue"] = PropertyValue<Color>{ *m_propertyDetail->paramRefValue }.toJSON();
				}
			}
		}

		void readFromJSON(const JSON& json) override
		{
			if (!json.contains(m_name))
			{
				return;
			}
			m_propertyValue = PropertyValue<Color>::FromJSON(json[m_name]);
			m_smoothing = Smoothing<ColorF>{ ColorF{ m_propertyValue.value(InteractionState::Default, Array<String>{}) } };

			const String paramRefKey = String{ m_name } + U"_paramRef";
			if (json.contains(paramRefKey))
			{
				setParamRef(json[paramRefKey].getString());
			}
			const String paramRefValueKey = String{ m_name } + U"_paramRefValue";
			if (json.contains(paramRefValueKey))
			{
				ensurePropertyDetail().paramRefValue = PropertyValue<Color>::FromJSON(json[paramRefValueKey]).defaultValue();
			}
			else if (m_propertyDetail)
			{
				m_propertyDetail->paramRefValue.reset();
			}
			const String paramRefModeKey = String{ m_name } + U"_paramRefMode";
			if (json.contains(paramRefModeKey))
			{
				if (auto opt = StringToEnumOpt<ParamRefMode>(json[paramRefModeKey].getString()))
				{
					setParamRefMode(ValidateParamRefModeFromJSON(*opt, AvailableParamRefModesForProperty<Color>(), name()));
				}
			}
			else
			{
				setParamRefMode(ParamRefMode::Normal);
			}
		}

		String propertyValueStringOfDefault() const override
		{
			return m_propertyValue.getValueStringOfDefault();
		}

		Optional<String> propertyValueStringOf(InteractionState interactionState, const Array<String>& activeStyleStates) const override
		{
			return m_propertyValue.getValueStringOf(interactionState, activeStyleStates);
		}

		String propertyValueStringOfFallback(InteractionState interactionState, const Array<String>& activeStyleStates) const override
		{
			return m_propertyValue.getValueStringOfFallback(interactionState, activeStyleStates);
		}

		bool trySetPropertyValueString(StringView value) override
		{
			return m_propertyValue.trySetValueString(value);
		}

		bool trySetPropertyValueStringOf(StringView value, InteractionState interactionState, StringView styleState = U"") override
		{
			return m_propertyValue.trySetValueStringOf(value, interactionState, styleState);
		}

		void unsetPropertyValueOf(InteractionState interactionState, StringView styleState = U"") override
		{
			m_propertyValue.unsetValueOf(interactionState, styleState);
		}

		bool hasPropertyValueOf(InteractionState interactionState, StringView styleState = U"") const override
		{
			return m_propertyValue.hasValueOf(interactionState, styleState);
		}

		PropertyEditType editType() const override
		{
			return PropertyEditType::Color;
		}

		bool isInteractiveProperty() const override
		{
			return true;
		}

		bool hasInteractivePropertyValue() const override
		{
			return m_propertyValue.hasInteractiveValue();
		}

		bool isSmoothProperty() const override
		{
			return true;
		}

		double smoothTime() const override
		{
			return m_propertyValue.smoothTime();
		}

		bool trySetSmoothTime(double smoothTime) override
		{
			m_propertyValue.setSmoothTime(smoothTime);
			return true;
		}

		Array<String> styleStateKeys() const override
		{
			Array<String> result;

			// styleStateValuesから収集
			if (m_propertyValue.styleStateValues())
			{
				for (const auto& [state, value] : *m_propertyValue.styleStateValues())
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
}
