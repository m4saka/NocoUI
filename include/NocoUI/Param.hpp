#pragma once
#include <Siv3D.hpp>
#include <variant>
#include "Serialization.hpp"
#include "LRTB.hpp"

namespace noco
{
	enum class ParamType : uint8
	{
		Bool,
		Number,  // double値（すべての算術型を内部的にdoubleで保持）
		String,
		Color,
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
		default:
			return U"Unknown";
		}
	}

	class Param
	{
	private:
		String m_name;
		ParamType m_type;
		
		std::variant<bool, double, String, ColorF, Vec2, LRTB> m_value;
		
		template<typename T>
		static auto convertToVariantType(const T& value)
		{
			if constexpr (std::is_arithmetic_v<T> && !std::is_same_v<T, bool>)
			{
				return static_cast<double>(value);  // 全ての数値型はdoubleとして保持
			}
			else if constexpr (std::is_same_v<T, Color>)
			{
				return ColorF(value);  // ColorはColorFとして保持
			}
			else
			{
				return value;
			}
		}
		
		template<typename T>
		static constexpr ParamType paramTypeOf()
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
			else if constexpr (std::is_same_v<T, ColorF>)
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

	public:
		// デフォルトコンストラクタ（空のString、Number型、値0.0）
		Param()
			: m_name(U"")
			, m_type(ParamType::Number)
			, m_value(0.0)
		{
		}
		
		template<typename T>
		static constexpr bool isSupportedType()
		{
			return std::is_same_v<T, bool> || 
			       std::is_arithmetic_v<T> || 
			       std::is_same_v<T, String> || 
			       std::is_same_v<T, Color> ||
			       std::is_same_v<T, ColorF> || 
			       std::is_same_v<T, Vec2> || 
			       std::is_same_v<T, LRTB>;
		}
		
		template<typename T>
		explicit Param(const String& name, const T& value)
			: m_name(name)
			, m_type(paramTypeOf<T>())
			, m_value(convertToVariantType(value))
		{
		}
		
		// 文字列リテラル用の特殊化
		explicit Param(const String& name, const char32_t* value)
			: m_name(name)
			, m_type(ParamType::String)
			, m_value(String(value))
		{
		}
		
		template<typename T>
		[[nodiscard]]
		Optional<T> valueAsOpt() const
		{
			if constexpr (std::is_arithmetic_v<T> && !std::is_same_v<T, bool>)
			{
				// 全ての数値型はdoubleで保持している
				if (auto* ptr = std::get_if<double>(&m_value))
				{
					if constexpr (std::is_unsigned_v<T>)
					{
						if (*ptr < 0)
						{
							// unsignedの場合、負の値は0に丸める
							return static_cast<T>(0);
						}
					}
					return static_cast<T>(*ptr);
				}
			}
			else if constexpr (std::is_same_v<T, Color>)
			{
				// ColorはColorFで保持している
				if (auto* ptr = std::get_if<ColorF>(&m_value))
				{
					return Color{ *ptr };
				}
			}
			else if (auto* ptr = std::get_if<T>(&m_value))
			{
				return *ptr;
			}
			return none;
		}
		
		template<typename T>
		[[nodiscard]]
		T valueAs(const T& fallbackValue = T{}) const
		{
			if (auto opt = valueAsOpt<T>())
			{
				return *opt;
			}
			return fallbackValue;
		}
		
		template<typename T>
		void setValue(const T& value)
		{
			m_value = convertToVariantType(value);
		}
		
		[[nodiscard]]
		const String& name() const
		{
			return m_name;
		}
		
		[[nodiscard]]
		ParamType type() const
		{
			return m_type;
		}
		
		[[nodiscard]]
		bool isBool() const
		{
			return std::holds_alternative<bool>(m_value);
		}
		
		[[nodiscard]]
		bool isNumber() const
		{
			return std::holds_alternative<double>(m_value);
		}
		
		[[nodiscard]]
		bool isString() const
		{
			return std::holds_alternative<String>(m_value);
		}
		
		[[nodiscard]]
		bool isColor() const
		{
			return std::holds_alternative<ColorF>(m_value);
		}
		
		[[nodiscard]]
		bool isVec2() const
		{
			return std::holds_alternative<Vec2>(m_value);
		}
		
		[[nodiscard]]
		bool isLRTB() const
		{
			return std::holds_alternative<LRTB>(m_value);
		}
		
		[[nodiscard]]
		JSON toJSON() const
		{
			return std::visit([this](const auto& val)
			{
				return JSON{
					{U"name", m_name},
					{U"type", ParamTypeToString(m_type)},
					{U"value", ValueToString(val)}
				};
			}, m_value);
		}
		
		static Optional<Param> fromJSON(const JSON& json)
		{
			if (!json.contains(U"name") || !json.contains(U"type") || !json.contains(U"value"))
			{
				return none;
			}
			
			const String name = json[U"name"].getString();
			const String typeStr = json[U"type"].getString();
			const auto& valueJson = json[U"value"];
			
			if (typeStr == U"Bool")
			{
				if (valueJson.isString())
				{
					if (auto opt = StringToValueOpt<bool>(valueJson.getString()))
					{
						return Param{name, *opt};
					}
				}
				else
				{
					Logger << U"[NocoUI warning] Bool param '" << name << U"' is ignored because value is not string format";
				}
			}
			else if (typeStr == U"Number")
			{
				if (valueJson.isString())
				{
					if (auto opt = StringToValueOpt<double>(valueJson.getString()))
					{
						return Param{name, *opt};
					}
				}
				else
				{
					Logger << U"[NocoUI warning] Number param '" << name << U"' is ignored because value is not string format";
				}
			}
			else if (typeStr == U"String")
			{
				if (valueJson.isString())
				{
					return Param{name, valueJson.getString()};
				}
				else
				{
					Logger << U"[NocoUI warning] String param '" << name << U"' is ignored because value is not string format";
				}
			}
			else if (typeStr == U"Color")
			{
				if (valueJson.isString())
				{
					if (auto opt = StringToValueOpt<ColorF>(valueJson.getString()))
					{
						return Param{name, *opt};
					}
				}
				else
				{
					Logger << U"[NocoUI warning] Color param '" << name << U"' is ignored because value is not string format";
				}
			}
			else if (typeStr == U"Vec2")
			{
				if (valueJson.isString())
				{
					if (auto opt = StringToValueOpt<Vec2>(valueJson.getString()))
					{
						return Param{name, *opt};
					}
				}
				else
				{
					Logger << U"[NocoUI warning] Vec2 param '" << name << U"' is ignored because value is not string format";
				}
			}
			else if (typeStr == U"LRTB")
			{
				if (valueJson.isString())
				{
					if (auto opt = StringToValueOpt<LRTB>(valueJson.getString()))
					{
						return Param{name, *opt};
					}
				}
				else
				{
					Logger << U"[NocoUI warning] LRTB param '" << name << U"' is ignored because value is not string format";
				}
			}
			
			return none;
		}
	};
}
