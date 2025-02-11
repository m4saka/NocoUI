#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"

namespace noco
{
	class Sprite : public ComponentBase
	{
	private:
		Property<String> m_assetName;
		SmoothProperty<ColorF> m_color;
		Property<bool> m_preserveAspect;

	public:
		explicit Sprite(const PropertyValue<String>& assetName = String{}, const PropertyValue<ColorF>& color = Palette::White, bool preserveAspect = false)
			: ComponentBase{ U"Sprite", { &m_assetName, &m_color, &m_preserveAspect } }
			, m_assetName{ U"assetName", assetName }
			, m_color{ U"color", color }
			, m_preserveAspect{ U"preserveAspect", preserveAspect }
		{
		}

		void draw(const Node& node) const override;

		[[nodiscard]]
		const PropertyValue<String>& assetName() const
		{
			return m_assetName.propertyValue();
		}

		void setAssetName(const PropertyValue<String>& assetName)
		{
			m_assetName.setPropertyValue(assetName);
		}

		[[nodiscard]]
		const PropertyValue<ColorF>& color() const
		{
			return m_color.propertyValue();
		}

		void setColor(const PropertyValue<ColorF>& color)
		{
			m_color.setPropertyValue(color);
		}

		[[nodiscard]]
		const PropertyValue<bool>& preserveAspect() const
		{
			return m_preserveAspect.propertyValue();
		}

		void setPreserveAspect(const PropertyValue<bool>& preserveAspect)
		{
			m_preserveAspect.setPropertyValue(preserveAspect);
		}
	};
}
