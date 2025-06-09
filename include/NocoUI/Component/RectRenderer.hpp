#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"

namespace noco
{
	class RectRenderer : public SerializableComponentBase, public std::enable_shared_from_this<RectRenderer>
	{
	private:
		SmoothProperty<ColorF> m_fillColor;
		SmoothProperty<ColorF> m_outlineColor;
		SmoothProperty<double> m_outlineThickness;
		SmoothProperty<double> m_cornerRadius;
		SmoothProperty<ColorF> m_shadowColor;
		SmoothProperty<Vec2> m_shadowOffset;
		SmoothProperty<double> m_shadowBlur;
		SmoothProperty<double> m_shadowSpread;

	public:
		explicit RectRenderer(
			const PropertyValue<ColorF>& fillColor = Palette::White,
			const PropertyValue<ColorF>& outlineColor = Palette::Black,
			const PropertyValue<double>& outlineThickness = 0.0,
			const PropertyValue<double>& cornerRadius = 0.0,
			const PropertyValue<ColorF>& shadowColor = ColorF{ 0.0, 0.0 },
			const PropertyValue<Vec2>& shadowOffset = Vec2{ 2.0, 2.0 },
			const PropertyValue<double>& shadowBlur = 0.0,
			const PropertyValue<double>& shadowSpread = 0.0)
			: SerializableComponentBase{ U"RectRenderer", { &m_fillColor, &m_outlineColor, &m_outlineThickness, &m_cornerRadius, &m_shadowColor, &m_shadowOffset, &m_shadowBlur, &m_shadowSpread } }
			, m_fillColor{ U"fillColor", fillColor }
			, m_outlineColor{ U"outlineColor", outlineColor }
			, m_outlineThickness{ U"outlineThickness", outlineThickness }
			, m_cornerRadius{ U"cornerRadius", cornerRadius }
			, m_shadowColor{ U"shadowColor", shadowColor }
			, m_shadowOffset{ U"shadowOffset", shadowOffset }
			, m_shadowBlur{ U"shadowBlur", shadowBlur }
			, m_shadowSpread{ U"shadowSpread", shadowSpread }
		{
		}

		void draw(const Node& node) const override;

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
		const PropertyValue<double>& outlineThickness() const
		{
			return m_outlineThickness.propertyValue();
		}

		std::shared_ptr<RectRenderer> setOutlineThickness(const PropertyValue<double>& outlineThickness)
		{
			m_outlineThickness.setPropertyValue(outlineThickness);
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
