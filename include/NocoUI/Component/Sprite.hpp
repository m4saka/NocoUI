#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"
#include "../LRTB.hpp"

namespace noco
{
	class Sprite : public SerializableComponentBase
	{
	private:
		Property<String> m_textureFilePath;
		Property<String> m_textureAssetName;
		SmoothProperty<ColorF> m_color;
		Property<bool> m_preserveAspect;
		Property<bool> m_nineSliceEnabled;
		Property<LRTB> m_nineSliceMargin;
		Property<Vec2> m_nineSliceScale;
		Property<bool> m_nineSliceCenterTiled;
		Property<bool> m_nineSliceLeftTiled;
		Property<bool> m_nineSliceRightTiled;
		Property<bool> m_nineSliceTopTiled;
		Property<bool> m_nineSliceBottomTiled;
		Property<bool> m_nineSliceFallback;
		
		/* NonSerialized */ Optional<Texture> m_textureOpt;

		void drawNineSlice(const Texture& texture, const RectF& rect, const Vec2& effectScale, const ColorF& color) const;

	public:
		explicit Sprite(const PropertyValue<String>& textureFilePath = String{}, const PropertyValue<String>& textureAssetName = String{}, const PropertyValue<ColorF>& color = Palette::White, bool preserveAspect = false)
			: SerializableComponentBase{ U"Sprite", { &m_textureFilePath, &m_textureAssetName, &m_color, &m_preserveAspect, &m_nineSliceEnabled, &m_nineSliceMargin, &m_nineSliceScale, &m_nineSliceCenterTiled, &m_nineSliceLeftTiled, &m_nineSliceRightTiled, &m_nineSliceTopTiled, &m_nineSliceBottomTiled, &m_nineSliceFallback } }
			, m_textureFilePath{ U"textureFilePath", textureFilePath }
			, m_textureAssetName{ U"textureAssetName", textureAssetName }
			, m_color{ U"color", color }
			, m_preserveAspect{ U"preserveAspect", preserveAspect }
			, m_nineSliceEnabled{ U"nineSliceEnabled", false }
			, m_nineSliceMargin{ U"nineSliceMargin", LRTB::Zero() }
			, m_nineSliceScale{ U"nineSliceScale", Vec2::One() }
			, m_nineSliceCenterTiled{ U"nineSliceCenterTiled", false }
			, m_nineSliceLeftTiled{ U"nineSliceLeftTiled", false }
			, m_nineSliceRightTiled{ U"nineSliceRightTiled", false }
			, m_nineSliceTopTiled{ U"nineSliceTopTiled", false }
			, m_nineSliceBottomTiled{ U"nineSliceBottomTiled", false }
			, m_nineSliceFallback{ U"nineSliceFallback", true }
		{
		}

		void draw(const Node& node) const override;

		[[nodiscard]]
		const PropertyValue<String>& textureFilePath() const
		{
			return m_textureFilePath.propertyValue();
		}

		void setTextureFilePath(const PropertyValue<String>& textureFilePath)
		{
			m_textureFilePath.setPropertyValue(textureFilePath);
		}

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
		
		[[nodiscard]]
		const PropertyValue<bool>& nineSliceEnabled() const
		{
			return m_nineSliceEnabled.propertyValue();
		}
		
		void setNineSliceEnabled(const PropertyValue<bool>& nineSliceEnabled)
		{
			m_nineSliceEnabled.setPropertyValue(nineSliceEnabled);
		}
		
		[[nodiscard]]
		const PropertyValue<LRTB>& nineSliceMargin() const
		{
			return m_nineSliceMargin.propertyValue();
		}
		
		void setNineSliceMargin(const PropertyValue<LRTB>& nineSliceMargin)
		{
			m_nineSliceMargin.setPropertyValue(nineSliceMargin);
		}
		
		[[nodiscard]]
		const PropertyValue<Vec2>& nineSliceScale() const
		{
			return m_nineSliceScale.propertyValue();
		}
		
		void setNineSliceScale(const PropertyValue<Vec2>& nineSliceScale)
		{
			m_nineSliceScale.setPropertyValue(nineSliceScale);
		}
		
		[[nodiscard]]
		const PropertyValue<bool>& nineSliceCenterTiled() const
		{
			return m_nineSliceCenterTiled.propertyValue();
		}
		
		void setNineSliceCenterTiled(const PropertyValue<bool>& nineSliceCenterTiled)
		{
			m_nineSliceCenterTiled.setPropertyValue(nineSliceCenterTiled);
		}
		
		[[nodiscard]]
		const PropertyValue<bool>& nineSliceLeftTiled() const
		{
			return m_nineSliceLeftTiled.propertyValue();
		}
		
		void setNineSliceLeftTiled(const PropertyValue<bool>& nineSliceLeftTiled)
		{
			m_nineSliceLeftTiled.setPropertyValue(nineSliceLeftTiled);
		}
		
		[[nodiscard]]
		const PropertyValue<bool>& nineSliceRightTiled() const
		{
			return m_nineSliceRightTiled.propertyValue();
		}
		
		void setNineSliceRightTiled(const PropertyValue<bool>& nineSliceRightTiled)
		{
			m_nineSliceRightTiled.setPropertyValue(nineSliceRightTiled);
		}
		
		[[nodiscard]]
		const PropertyValue<bool>& nineSliceTopTiled() const
		{
			return m_nineSliceTopTiled.propertyValue();
		}
		
		void setNineSliceTopTiled(const PropertyValue<bool>& nineSliceTopTiled)
		{
			m_nineSliceTopTiled.setPropertyValue(nineSliceTopTiled);
		}
		
		[[nodiscard]]
		const PropertyValue<bool>& nineSliceBottomTiled() const
		{
			return m_nineSliceBottomTiled.propertyValue();
		}
		
		void setNineSliceBottomTiled(const PropertyValue<bool>& nineSliceBottomTiled)
		{
			m_nineSliceBottomTiled.setPropertyValue(nineSliceBottomTiled);
		}
		
		[[nodiscard]]
		const PropertyValue<bool>& nineSliceFallback() const
		{
			return m_nineSliceFallback.propertyValue();
		}
		
		void setNineSliceFallback(const PropertyValue<bool>& nineSliceFallback)
		{
			m_nineSliceFallback.setPropertyValue(nineSliceFallback);
		}
		
		void setTexture(const Texture& texture)
		{
			m_textureOpt = texture;
		}
		
		void clearTexture()
		{
			m_textureOpt.reset();
		}
	};
}
