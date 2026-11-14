#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"
#include "../Enums.hpp"

namespace noco
{
	enum class RectFillGradationType : uint8
	{
		None,
		TopBottom,
		LeftRight,
	};

	class RectRenderer : public SerializableComponentBase, public std::enable_shared_from_this<RectRenderer>
	{
	private:
		Property<RectFillGradationType> m_fillGradationType;
		SmoothProperty<Color> m_fillColor;
		SmoothProperty<Color> m_fillGradationColor1;
		SmoothProperty<Color> m_fillGradationColor2;
		Property<BlendMode> m_blendMode;
		SmoothProperty<Color> m_outlineColor;
		SmoothProperty<double> m_outlineThicknessInner;
		SmoothProperty<double> m_outlineThicknessOuter;
		SmoothProperty<double> m_cornerRadius;
		SmoothProperty<Color> m_shadowColor;
		SmoothProperty<Vec2> m_shadowOffset;
		SmoothProperty<double> m_shadowBlur;
		SmoothProperty<double> m_shadowSpread;

	public:
		explicit RectRenderer(
			const PropertyValue<Color>& fillColor = Palette::White,
			const PropertyValue<Color>& outlineColor = Palette::Black,
			const PropertyValue<double>& outlineThicknessInner = 0.0,
			const PropertyValue<double>& outlineThicknessOuter = 0.0,
			const PropertyValue<double>& cornerRadius = 0.0,
			const PropertyValue<Color>& shadowColor = Color{ 0, 0, 0, 0 },
			const PropertyValue<Vec2>& shadowOffset = Vec2{ 2.0, 2.0 },
			const PropertyValue<double>& shadowBlur = 0.0,
			const PropertyValue<double>& shadowSpread = 0.0);

		void draw(const Node& node) const override;

		[[nodiscard]]
		const PropertyValue<RectFillGradationType>& fillGradationType() const
		{
			return m_fillGradationType.propertyValue();
		}

		std::shared_ptr<RectRenderer> setFillGradationType(const PropertyValue<RectFillGradationType>& fillGradationType)
		{
			m_fillGradationType.setPropertyValue(fillGradationType);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<Color>& fillColor() const
		{
			return m_fillColor.propertyValue();
		}

		std::shared_ptr<RectRenderer> setFillColor(const PropertyValue<Color>& fillColor)
		{
			m_fillColor.setPropertyValue(fillColor);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<Color>& fillGradationColor1() const
		{
			return m_fillGradationColor1.propertyValue();
		}

		std::shared_ptr<RectRenderer> setFillGradationColor1(const PropertyValue<Color>& fillGradationColor1)
		{
			m_fillGradationColor1.setPropertyValue(fillGradationColor1);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<Color>& fillGradationColor2() const
		{
			return m_fillGradationColor2.propertyValue();
		}

		std::shared_ptr<RectRenderer> setFillGradationColor2(const PropertyValue<Color>& fillGradationColor2)
		{
			m_fillGradationColor2.setPropertyValue(fillGradationColor2);
			return shared_from_this();
		}

		std::shared_ptr<RectRenderer> setFillGradationColors(const PropertyValue<Color>& color1, const PropertyValue<Color>& color2)
		{
			m_fillGradationColor1.setPropertyValue(color1);
			m_fillGradationColor2.setPropertyValue(color2);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<BlendMode>& blendMode() const
		{
			return m_blendMode.propertyValue();
		}

		std::shared_ptr<RectRenderer> setBlendMode(const PropertyValue<BlendMode>& blendMode)
		{
			m_blendMode.setPropertyValue(blendMode);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<Color>& outlineColor() const
		{
			return m_outlineColor.propertyValue();
		}

		std::shared_ptr<RectRenderer> setOutlineColor(const PropertyValue<Color>& outlineColor)
		{
			m_outlineColor.setPropertyValue(outlineColor);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<double>& outlineThicknessInner() const
		{
			return m_outlineThicknessInner.propertyValue();
		}

		std::shared_ptr<RectRenderer> setOutlineThicknessInner(const PropertyValue<double>& outlineThicknessInner)
		{
			m_outlineThicknessInner.setPropertyValue(outlineThicknessInner);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<double>& outlineThicknessOuter() const
		{
			return m_outlineThicknessOuter.propertyValue();
		}

		std::shared_ptr<RectRenderer> setOutlineThicknessOuter(const PropertyValue<double>& outlineThicknessOuter)
		{
			m_outlineThicknessOuter.setPropertyValue(outlineThicknessOuter);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<double>& cornerRadius() const
		{
			return m_cornerRadius.propertyValue();
		}

		std::shared_ptr<RectRenderer> setCornerRadius(const PropertyValue<double>& cornerRadius)
		{
			m_cornerRadius.setPropertyValue(cornerRadius);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<Color>& shadowColor() const
		{
			return m_shadowColor.propertyValue();
		}

		std::shared_ptr<RectRenderer> setShadowColor(const PropertyValue<Color>& shadowColor)
		{
			m_shadowColor.setPropertyValue(shadowColor);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<Vec2>& shadowOffset() const
		{
			return m_shadowOffset.propertyValue();
		}

		std::shared_ptr<RectRenderer> setShadowOffset(const PropertyValue<Vec2>& shadowOffset)
		{
			m_shadowOffset.setPropertyValue(shadowOffset);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<double>& shadowBlur() const
		{
			return m_shadowBlur.propertyValue();
		}

		std::shared_ptr<RectRenderer> setShadowBlur(const PropertyValue<double>& shadowBlur)
		{
			m_shadowBlur.setPropertyValue(shadowBlur);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<double>& shadowSpread() const
		{
			return m_shadowSpread.propertyValue();
		}

		std::shared_ptr<RectRenderer> setShadowSpread(const PropertyValue<double>& shadowSpread)
		{
			m_shadowSpread.setPropertyValue(shadowSpread);
			return shared_from_this();
		}
	};
}
