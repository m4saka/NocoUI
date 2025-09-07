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
		SmoothProperty<ColorF> m_fillColor;
		SmoothProperty<ColorF> m_fillGradationColor1;
		SmoothProperty<ColorF> m_fillGradationColor2;
		Property<BlendMode> m_blendMode;
		SmoothProperty<ColorF> m_outlineColor;
		SmoothProperty<double> m_outlineThicknessInner;
		SmoothProperty<double> m_outlineThicknessOuter;
		SmoothProperty<double> m_cornerRadius;
		SmoothProperty<ColorF> m_shadowColor;
		SmoothProperty<Vec2> m_shadowOffset;
		SmoothProperty<double> m_shadowBlur;
		SmoothProperty<double> m_shadowSpread;

	public:
		explicit RectRenderer(
			const PropertyValue<ColorF>& fillColor = Palette::White,
			const PropertyValue<ColorF>& outlineColor = Palette::Black,
			const PropertyValue<double>& outlineThicknessInner = 0.0,
			const PropertyValue<double>& outlineThicknessOuter = 0.0,
			const PropertyValue<double>& cornerRadius = 0.0,
			const PropertyValue<ColorF>& shadowColor = ColorF{ 0.0, 0.0 },
			const PropertyValue<Vec2>& shadowOffset = Vec2{ 2.0, 2.0 },
			const PropertyValue<double>& shadowBlur = 0.0,
			const PropertyValue<double>& shadowSpread = 0.0)
			: SerializableComponentBase{ U"RectRenderer", { &m_fillGradationType, &m_fillColor, &m_fillGradationColor1, &m_fillGradationColor2, &m_blendMode, &m_outlineColor, &m_outlineThicknessInner, &m_outlineThicknessOuter, &m_cornerRadius, &m_shadowColor, &m_shadowOffset, &m_shadowBlur, &m_shadowSpread } }
			, m_fillGradationType{ U"fillGradationType", RectFillGradationType::None }
			, m_fillColor{ U"fillColor", fillColor }
			, m_fillGradationColor1{ U"fillGradationColor1", fillColor }
			, m_fillGradationColor2{ U"fillGradationColor2", fillColor }
			, m_blendMode{ U"blendMode", BlendMode::Normal }
			, m_outlineColor{ U"outlineColor", outlineColor }
			, m_outlineThicknessInner{ U"outlineThicknessInner", outlineThicknessInner }
			, m_outlineThicknessOuter{ U"outlineThicknessOuter", outlineThicknessOuter }
			, m_cornerRadius{ U"cornerRadius", cornerRadius }
			, m_shadowColor{ U"shadowColor", shadowColor }
			, m_shadowOffset{ U"shadowOffset", shadowOffset }
			, m_shadowBlur{ U"shadowBlur", shadowBlur }
			, m_shadowSpread{ U"shadowSpread", shadowSpread }
		{
		}

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
		const PropertyValue<ColorF>& fillColor() const
		{
			return m_fillColor.propertyValue();
		}

		std::shared_ptr<RectRenderer> setFillColor(const PropertyValue<ColorF>& fillColor)
		{
			m_fillColor.setPropertyValue(fillColor);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<ColorF>& fillGradationColor1() const
		{
			return m_fillGradationColor1.propertyValue();
		}

		std::shared_ptr<RectRenderer> setFillGradationColor1(const PropertyValue<ColorF>& fillGradationColor1)
		{
			m_fillGradationColor1.setPropertyValue(fillGradationColor1);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<ColorF>& fillGradationColor2() const
		{
			return m_fillGradationColor2.propertyValue();
		}

		std::shared_ptr<RectRenderer> setFillGradationColor2(const PropertyValue<ColorF>& fillGradationColor2)
		{
			m_fillGradationColor2.setPropertyValue(fillGradationColor2);
			return shared_from_this();
		}

		std::shared_ptr<RectRenderer> setFillGradationColors(const PropertyValue<ColorF>& color1, const PropertyValue<ColorF>& color2)
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
		const PropertyValue<ColorF>& outlineColor() const
		{
			return m_outlineColor.propertyValue();
		}

		std::shared_ptr<RectRenderer> setOutlineColor(const PropertyValue<ColorF>& outlineColor)
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
		const PropertyValue<ColorF>& shadowColor() const
		{
			return m_shadowColor.propertyValue();
		}

		std::shared_ptr<RectRenderer> setShadowColor(const PropertyValue<ColorF>& shadowColor)
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
