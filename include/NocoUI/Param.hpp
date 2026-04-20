#pragma once
#include <Siv3D.hpp>
#include <variant>
#include "Serialization.hpp"
#include "LRTB.hpp"

namespace noco
{
	// パラメータ値の型定義
	using ParamValue = std::variant<bool, double, String, Color, Vec2, LRTB>;
	
	// UI表示用の型列挙
	enum class ParamType : uint8
	{
		Unknown,
		Bool,
		Number, // 数値は全てdouble型で保持
		String,
		Color, // 色は全てColor型で保持
		Vec2,
		LRTB,
	};

	[[nodiscard]]
	inline String ParamTypeToString(ParamType type)
	{
		switch (type)
		{
		case ParamType::Bool:
			return U"Bool";
		case ParamType::Number:
			return U"Number";
		case ParamType::String:
			return U"String";
		case ParamType::Color:
			return U"Color";
		case ParamType::Vec2:
			return U"Vec2";
		case ParamType::LRTB:
			return U"LRTB";
		case ParamType::Unknown:
		default:
			return U"Unknown";
		}
	}
	
	// ParamValueから型情報を取得
	[[nodiscard]]
	inline ParamType GetParamType(const ParamValue& value)
	{
		return std::visit([](const auto& v) -> ParamType
			{
				using T = std::decay_t<decltype(v)>;
				if constexpr (std::is_same_v<T, bool>)
				{
					return ParamType::Bool;
				}
				else if constexpr (std::is_arithmetic_v<T> && !std::is_same_v<T, bool>)
				{
					return ParamType::Number;
				}
				else if constexpr (std::is_same_v<T, String>)
				{
					return ParamType::String;
				}
				else if constexpr (std::is_same_v<T, Color>)
				{
					return ParamType::Color;
				}
				else if constexpr (std::is_same_v<T, Vec2>)
				{
					return ParamType::Vec2;
				}
				else if constexpr (std::is_same_v<T, LRTB>)
				{
					return ParamType::LRTB;
				}
				else
				{
					return ParamType::Unknown;
				}
			}, value);
	}
	
	// 型からParamTypeを取得
	template<typename T>
	constexpr ParamType GetParamTypeOf()
	{
		if constexpr (std::is_same_v<T, bool>)
		{
			return ParamType::Bool;
		}
		else if constexpr (std::is_arithmetic_v<T>)
		{
			return ParamType::Number;
		}
		else if constexpr (std::is_same_v<T, String>)
		{
			return ParamType::String;
		}
		else if constexpr (std::is_same_v<T, Color>)
		{
			return ParamType::Color;
		}
		else if constexpr (std::is_same_v<T, Vec2>)
		{
			return ParamType::Vec2;
		}
		else if constexpr (std::is_same_v<T, LRTB>)
		{
			return ParamType::LRTB;
		}
		else
		{
			static_assert(!std::is_same_v<T, T>, "Unsupported parameter type");
		}
	}
	
	// 型がパラメータ参照をサポートしているか返す
	template<typename T>
	constexpr bool IsParamSupportedType()
	{
		return std::is_same_v<T, bool> ||
			std::is_same_v<T, String> ||
			std::is_same_v<T, Color> ||
			std::is_same_v<T, Vec2> ||
			std::is_same_v<T, LRTB> ||
			std::is_arithmetic_v<T> ||
			std::is_enum_v<T>;
	}
	
	// 値をParamValue用の型に変換
	template<typename T>
	inline ParamValue MakeParamValue(const T& value)
	{
		if constexpr (std::is_arithmetic_v<T> && !std::is_same_v<T, bool>)
		{
			return static_cast<double>(value); // 全ての数値型はdoubleとして保持
		}
		// Colorは直接保持するため変換不要
		else if constexpr (std::is_same_v<T, const char32_t*>)
		{
			return String{ value }; // 文字列リテラル
		}
		else
		{
			return value;
		}
	}

	// ParamValue自体を受け取る場合はそのまま返す
	inline ParamValue MakeParamValue(const ParamValue& value)
	{
		return value;
	}

	// ParamValueから型安全に値を取得
	template<typename T>
	[[nodiscard]]
	inline Optional<T> GetParamValueAs(const ParamValue& value)
	{
		if constexpr (std::is_arithmetic_v<T> && !std::is_same_v<T, bool>)
		{
			// 全ての数値型はdoubleで保持している
			if (auto* ptr = std::get_if<double>(&value))
			{
				if constexpr (std::is_unsigned_v<T>)
				{
					if (*ptr < 0)
					{
						// unsignedの場合、負の値は0に丸める
						return T{ 0 };
					}
				}
				return static_cast<T>(*ptr);
			}
		}
		// Colorは直接保持しているため変換不要
		else if constexpr (std::is_enum_v<T>)
		{
			// enum型の場合、String型パラメータからenum値に変換
			if (auto* ptr = std::get_if<String>(&value))
			{
				return StringToEnum<T>(*ptr, T{});
			}
		}
		else if (auto* ptr = std::get_if<T>(&value))
		{
			return *ptr;
		}
		return none;
	}
	
	// ParamValueを文字列に変換
	[[nodiscard]]
	inline String ParamValueToString(const ParamValue& value)
	{
		return std::visit([](const auto& v) {
			return ValueToString(v);
		}, value);
	}
	
	// 文字列からParamValueを作成
	[[nodiscard]]
	inline Optional<ParamValue> ParamValueFromString(ParamType type, const String& str)
	{
		switch (type)
		{
		case ParamType::Bool:
			if (auto opt = StringToValueOpt<bool>(str))
			{
				return ParamValue{ *opt };
			}
			break;
		case ParamType::Number:
			if (auto opt = StringToValueOpt<double>(str))
			{
				return ParamValue{ *opt };
			}
			break;
		case ParamType::String:
			return ParamValue{ str };
		case ParamType::Color:
			if (auto opt = StringToValueOpt<Color>(str))
			{
				return ParamValue{ *opt };
			}
			break;
		case ParamType::Vec2:
			if (auto opt = StringToValueOpt<Vec2>(str))
			{
				return ParamValue{ *opt };
			}
			break;
		case ParamType::LRTB:
			if (auto opt = StringToValueOpt<LRTB>(str))
			{
				return ParamValue{ *opt };
			}
			break;
		case ParamType::Unknown:
		default:
			break;
		}
		return none;
	}
	
	// ParamValueを{"type": "...", "value": ...}形式のJSONに変換
	[[nodiscard]]
	JSON ParamValueToParamObjectJSON(const ParamValue& value);

	// ParamValueをJSON値に変換
	[[nodiscard]]
	JSON ParamValueToJSONValue(const ParamValue& value);

	// 型を指定してJSONの値をParamValueに変換
	[[nodiscard]]
	Optional<ParamValue> ParamValueFromJSONValue(const JSON& json, ParamType type);

	// {"type": "...", "value": ...}形式のJSONからParamValueを作成
	[[nodiscard]]
	Optional<ParamValue> ParamValueFromParamObjectJSON(const JSON& json);

	/// @brief パラメータ参照モード(全型共通。型ごとに有効なモードが異なり、AvailableParamRefModesFor<T>()で取得可能)
	enum class ParamRefMode : uint8
	{
		Normal,
		Inverted, // Bool用
		Add, // Number/Color/Vec2/LRTB用
		Subtract, // Number/Color/Vec2/LRTB用
		Multiply, // Number/Color/Vec2/LRTB用
		Format, // String用
	};

	/// @brief paramRefモードのUI表示用文字列
	[[nodiscard]]
	inline StringView ParamRefModeToDisplayString(ParamRefMode mode)
	{
		switch (mode)
		{
		case ParamRefMode::Normal:
			return U"通常反映(値を上書き)";
		case ParamRefMode::Inverted:
			return U"反転して反映(値を上書き)";
		case ParamRefMode::Add:
			return U"加算値として反映";
		case ParamRefMode::Subtract:
			return U"減算値として反映";
		case ParamRefMode::Multiply:
			return U"乗算値として反映";
		case ParamRefMode::Format:
			return U"フォーマット文字列に適用して反映";
		}
		return U"";
	}

	/// @brief 型Tで利用可能なparamRefモード一覧
	template<typename T>
	[[nodiscard]]
	inline Array<ParamRefMode> AvailableParamRefModesFor()
	{
		if constexpr (std::is_same_v<T, bool>)
		{
			return { ParamRefMode::Normal, ParamRefMode::Inverted };
		}
		else if constexpr (std::is_enum_v<T>)
		{
			return { ParamRefMode::Normal };
		}
		else if constexpr (std::is_same_v<T, String>)
		{
			return { ParamRefMode::Normal, ParamRefMode::Format };
		}
		else
		{
			// Number(arithmetic)/Color/Vec2/LRTB
			return { ParamRefMode::Normal, ParamRefMode::Add, ParamRefMode::Subtract, ParamRefMode::Multiply };
		}
	}

	/// @brief 指定paramRefモードが型に対応しない場合、警告を出してNormalを返す(JSON読込時用)
	inline ParamRefMode ValidateParamRefModeFromJSON(ParamRefMode mode, const Array<ParamRefMode>& available, StringView propertyName)
	{
		if (!available.contains(mode))
		{
			Logger << U"[NocoUI warning] Unknown paramRef mode '{}' for property '{}'. Using default Normal."_fmt(EnumToString(mode), propertyName);
			return ParamRefMode::Normal;
		}
		return mode;
	}

	/// @brief paramRef値にモードを適用
	template<typename T>
	[[nodiscard]]
	inline Optional<T> ApplyParamMode(const T& base, const ParamValue& paramValue, ParamRefMode mode)
	{
		if constexpr (std::same_as<T, bool>)
		{
			auto v = GetParamValueAs<bool>(paramValue);
			if (!v)
			{
				return none;
			}
			switch (mode)
			{
			case ParamRefMode::Inverted:
				return !*v;
			case ParamRefMode::Normal:
			default:
				return *v;
			}
		}
		else if constexpr (std::same_as<T, String>)
		{
			if (mode == ParamRefMode::Format)
			{
				return std::visit([&](const auto& v) -> Optional<String>
					{
						using V = std::decay_t<decltype(v)>;
						try
						{
							if constexpr (std::same_as<V, bool> || std::same_as<V, double> || std::same_as<V, String>)
							{
								return Fmt(base)(v);
							}
							else
							{
								return Fmt(base)(Format(v));
							}
						}
						catch (const std::exception&)
						{
							return base;
						}
					}, paramValue);
			}
			return std::visit([](const auto& v) -> Optional<String>
				{
					using V = std::decay_t<decltype(v)>;
					if constexpr (std::same_as<V, String>)
					{
						return v;
					}
					else
					{
						return Format(v);
					}
				}, paramValue);
		}
		else if constexpr (std::is_enum_v<T>)
		{
			if (auto v = GetParamValueAs<T>(paramValue))
			{
				return *v;
			}
			return none;
		}
		else if constexpr (std::same_as<T, Color>)
		{
			auto v = GetParamValueAs<Color>(paramValue);
			if (!v)
			{
				return none;
			}
			if (mode == ParamRefMode::Normal)
			{
				return *v;
			}
			const ColorF bf{ base };
			const ColorF vf{ *v };
			ColorF result = bf;
			switch (mode)
			{
			case ParamRefMode::Add:
				result = ColorF{ bf.r + vf.r, bf.g + vf.g, bf.b + vf.b, bf.a + vf.a };
				break;
			case ParamRefMode::Subtract:
				result = ColorF{ bf.r - vf.r, bf.g - vf.g, bf.b - vf.b, bf.a - vf.a };
				break;
			case ParamRefMode::Multiply:
				result = ColorF{ bf.r * vf.r, bf.g * vf.g, bf.b * vf.b, bf.a * vf.a };
				break;
			default:
				return *v;
			}
			return ColorF{
				Math::Saturate(result.r),
				Math::Saturate(result.g),
				Math::Saturate(result.b),
				Math::Saturate(result.a),
			}.toColor();
		}
		else if constexpr (std::is_arithmetic_v<T> || std::same_as<T, Vec2> || std::same_as<T, LRTB>)
		{
			auto v = GetParamValueAs<T>(paramValue);
			if (!v)
			{
				return none;
			}
			switch (mode)
			{
			case ParamRefMode::Add:
				return static_cast<T>(base + *v);
			case ParamRefMode::Subtract:
				return static_cast<T>(base - *v);
			case ParamRefMode::Multiply:
				return static_cast<T>(base * *v);
			case ParamRefMode::Normal:
			default:
				return *v;
			}
		}
		else
		{
			return none;
		}
	}
}
