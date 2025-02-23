#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"

namespace noco
{
	class Sprite : public SerializableComponentBase
	{
	private:
		Property<String> m_textureAssetName;
		SmoothProperty<ColorF> m_color;
		Property<bool> m_preserveAspect;

	public:
		explicit Sprite(const PropertyValue<String>& textureAssetName = String{}, const PropertyValue<ColorF>& color = Palette::White, bool preserveAspect = false)
			: SerializableComponentBase{ U"Sprite", { &m_textureAssetName, &m_color, &m_preserveAspect } }
			, m_textureAssetName{ U"textureAssetName", textureAssetName }
			, m_color{ U"color", color }
			, m_preserveAspect{ U"preserveAspect", preserveAspect }
		{
		}

		void draw(const Node& node) const override;

		[[nodiscard]]
		const PropertyValue<String>& textureAssetName() const
		{
			return m_textureAssetName.propertyValue();
		}

		void setTextureAssetName(const PropertyValue<String>& textureAssetName)
		{
			m_textureAssetName.setPropertyValue(textureAssetName);
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
