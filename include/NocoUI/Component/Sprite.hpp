#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"
#include "../LRTB.hpp"
#include "../Enums.hpp"

namespace noco
{
	class Sprite : public SerializableComponentBase, public std::enable_shared_from_this<Sprite>
	{
	private:
		Property<String> m_textureFilePath;
		Property<String> m_textureAssetName;
		SmoothProperty<Color> m_color;
		SmoothProperty<Color> m_addColor;
		Property<BlendMode> m_blendMode;
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
		Property<TextureRegionMode> m_textureRegionMode;
		Property<Vec2> m_textureOffset;
		Property<Vec2> m_textureSize;
		Property<Vec2> m_textureGridCellSize;
		Property<int32> m_textureGridColumns;
		Property<int32> m_textureGridRows;
		Property<int32> m_textureGridIndex;
		Property<SpriteGridAnimationType> m_gridAnimationType;
		PropertyNonInteractive<double> m_gridAnimationFPS;
		PropertyNonInteractive<int32> m_gridAnimationStartIndex;
		PropertyNonInteractive<int32> m_gridAnimationEndIndex;
		Property<SpriteOffsetAnimationType> m_offsetAnimationType;
		PropertyNonInteractive<Vec2> m_offsetAnimationSpeed;
		Property<SpriteTextureFilter> m_textureFilter;
		Property<SpriteTextureAddressMode> m_textureAddressMode;

		/* NonSerialized */ Optional<Texture> m_textureOpt;
		/* NonSerialized */ Stopwatch m_animationStopwatch;
		/* NonSerialized */ int32 m_currentGridAnimationIndex = 0;
		/* NonSerialized */ bool m_gridAnimationFinished = false;
		/* NonSerialized */ Vec2 m_currentOffsetAnimation = Vec2::Zero();

		void drawNineSlice(const Texture& texture, const RectF& rect, const Color& color) const;
		void drawNineSliceFromRegion(const Texture& texture, const RectF& sourceRect, const RectF& rect, const Color& color) const;

	public:
		explicit Sprite(const PropertyValue<String>& textureFilePath = String{}, const PropertyValue<String>& textureAssetName = String{}, const PropertyValue<Color>& color = Palette::White, const PropertyValue<bool>& preserveAspect = false)
			: SerializableComponentBase{ U"Sprite", { &m_textureFilePath, &m_textureAssetName, &m_color, &m_addColor, &m_blendMode, &m_preserveAspect, &m_nineSliceEnabled, &m_nineSliceMargin, &m_nineSliceScale, &m_nineSliceCenterTiled, &m_nineSliceLeftTiled, &m_nineSliceRightTiled, &m_nineSliceTopTiled, &m_nineSliceBottomTiled, &m_nineSliceFallback, &m_textureRegionMode, &m_textureOffset, &m_textureSize, &m_textureGridCellSize, &m_textureGridColumns, &m_textureGridRows, &m_textureGridIndex, &m_gridAnimationType, &m_gridAnimationFPS, &m_gridAnimationStartIndex, &m_gridAnimationEndIndex, &m_offsetAnimationType, &m_offsetAnimationSpeed, &m_textureFilter, &m_textureAddressMode } }
			, m_textureFilePath{ U"textureFilePath", textureFilePath }
			, m_textureAssetName{ U"textureAssetName", textureAssetName }
			, m_color{ U"color", color }
			, m_addColor{ U"addColor", Color{ 0, 0, 0, 0 } }
			, m_blendMode{ U"blendMode", BlendMode::Normal }
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
			, m_textureRegionMode{ U"textureRegionMode", TextureRegionMode::Full }
			, m_textureOffset{ U"textureOffset", Vec2::Zero() }
			, m_textureSize{ U"textureSize", Vec2{ 100, 100 } }
			, m_textureGridCellSize{ U"textureGridCellSize", Vec2{ 32, 32 } }
			, m_textureGridColumns{ U"textureGridColumns", 1 }
			, m_textureGridRows{ U"textureGridRows", 1 }
			, m_textureGridIndex{ U"textureGridIndex", 0 }
			, m_gridAnimationType{ U"gridAnimationType", SpriteGridAnimationType::None }
			, m_gridAnimationFPS{ U"gridAnimationFPS", 10.0 }
			, m_gridAnimationStartIndex{ U"gridAnimationStartIndex", 0 }
			, m_gridAnimationEndIndex{ U"gridAnimationEndIndex", 0 }
			, m_offsetAnimationType{ U"offsetAnimationType", SpriteOffsetAnimationType::None }
			, m_offsetAnimationSpeed{ U"offsetAnimationSpeed", Vec2{ 0.0, 0.0 } }
			, m_textureFilter{ U"textureFilter", SpriteTextureFilter::Default }
			, m_textureAddressMode{ U"textureAddressMode", SpriteTextureAddressMode::Default }
		{
		}

		void onActivated(const std::shared_ptr<Node>& node) override;
		void update(const std::shared_ptr<Node>& node) override;
		void draw(const Node& node) const override;

		[[nodiscard]]
		const PropertyValue<String>& textureFilePath() const
		{
			return m_textureFilePath.propertyValue();
		}

		std::shared_ptr<Sprite> setTextureFilePath(const PropertyValue<String>& textureFilePath)
		{
			m_textureFilePath.setPropertyValue(textureFilePath);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<String>& textureAssetName() const
		{
			return m_textureAssetName.propertyValue();
		}

		std::shared_ptr<Sprite> setTextureAssetName(const PropertyValue<String>& textureAssetName)
		{
			m_textureAssetName.setPropertyValue(textureAssetName);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<Color>& color() const
		{
			return m_color.propertyValue();
		}

		std::shared_ptr<Sprite> setColor(const PropertyValue<Color>& color)
		{
			m_color.setPropertyValue(color);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<Color>& addColor() const
		{
			return m_addColor.propertyValue();
		}

		std::shared_ptr<Sprite> setAddColor(const PropertyValue<Color>& addColor)
		{
			m_addColor.setPropertyValue(addColor);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<BlendMode>& blendMode() const
		{
			return m_blendMode.propertyValue();
		}

		std::shared_ptr<Sprite> setBlendMode(const PropertyValue<BlendMode>& blendMode)
		{
			m_blendMode.setPropertyValue(blendMode);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<bool>& preserveAspect() const
		{
			return m_preserveAspect.propertyValue();
		}

		std::shared_ptr<Sprite> setPreserveAspect(const PropertyValue<bool>& preserveAspect)
		{
			m_preserveAspect.setPropertyValue(preserveAspect);
			return shared_from_this();
		}
		
		[[nodiscard]]
		const PropertyValue<bool>& nineSliceEnabled() const
		{
			return m_nineSliceEnabled.propertyValue();
		}
		
		std::shared_ptr<Sprite> setNineSliceEnabled(const PropertyValue<bool>& nineSliceEnabled)
		{
			m_nineSliceEnabled.setPropertyValue(nineSliceEnabled);
			return shared_from_this();
		}
		
		[[nodiscard]]
		const PropertyValue<LRTB>& nineSliceMargin() const
		{
			return m_nineSliceMargin.propertyValue();
		}
		
		std::shared_ptr<Sprite> setNineSliceMargin(const PropertyValue<LRTB>& nineSliceMargin)
		{
			m_nineSliceMargin.setPropertyValue(nineSliceMargin);
			return shared_from_this();
		}
		
		[[nodiscard]]
		const PropertyValue<Vec2>& nineSliceScale() const
		{
			return m_nineSliceScale.propertyValue();
		}
		
		std::shared_ptr<Sprite> setNineSliceScale(const PropertyValue<Vec2>& nineSliceScale)
		{
			m_nineSliceScale.setPropertyValue(nineSliceScale);
			return shared_from_this();
		}
		
		[[nodiscard]]
		const PropertyValue<bool>& nineSliceCenterTiled() const
		{
			return m_nineSliceCenterTiled.propertyValue();
		}
		
		std::shared_ptr<Sprite> setNineSliceCenterTiled(const PropertyValue<bool>& nineSliceCenterTiled)
		{
			m_nineSliceCenterTiled.setPropertyValue(nineSliceCenterTiled);
			return shared_from_this();
		}
		
		[[nodiscard]]
		const PropertyValue<bool>& nineSliceLeftTiled() const
		{
			return m_nineSliceLeftTiled.propertyValue();
		}
		
		std::shared_ptr<Sprite> setNineSliceLeftTiled(const PropertyValue<bool>& nineSliceLeftTiled)
		{
			m_nineSliceLeftTiled.setPropertyValue(nineSliceLeftTiled);
			return shared_from_this();
		}
		
		[[nodiscard]]
		const PropertyValue<bool>& nineSliceRightTiled() const
		{
			return m_nineSliceRightTiled.propertyValue();
		}
		
		std::shared_ptr<Sprite> setNineSliceRightTiled(const PropertyValue<bool>& nineSliceRightTiled)
		{
			m_nineSliceRightTiled.setPropertyValue(nineSliceRightTiled);
			return shared_from_this();
		}
		
		[[nodiscard]]
		const PropertyValue<bool>& nineSliceTopTiled() const
		{
			return m_nineSliceTopTiled.propertyValue();
		}
		
		std::shared_ptr<Sprite> setNineSliceTopTiled(const PropertyValue<bool>& nineSliceTopTiled)
		{
			m_nineSliceTopTiled.setPropertyValue(nineSliceTopTiled);
			return shared_from_this();
		}
		
		[[nodiscard]]
		const PropertyValue<bool>& nineSliceBottomTiled() const
		{
			return m_nineSliceBottomTiled.propertyValue();
		}
		
		std::shared_ptr<Sprite> setNineSliceBottomTiled(const PropertyValue<bool>& nineSliceBottomTiled)
		{
			m_nineSliceBottomTiled.setPropertyValue(nineSliceBottomTiled);
			return shared_from_this();
		}
		
		[[nodiscard]]
		const PropertyValue<bool>& nineSliceFallback() const
		{
			return m_nineSliceFallback.propertyValue();
		}
		
		std::shared_ptr<Sprite> setNineSliceFallback(const PropertyValue<bool>& nineSliceFallback)
		{
			m_nineSliceFallback.setPropertyValue(nineSliceFallback);
			return shared_from_this();
		}
		
		[[nodiscard]]
		const PropertyValue<TextureRegionMode>& textureRegionMode() const
		{
			return m_textureRegionMode.propertyValue();
		}
		
		std::shared_ptr<Sprite> setTextureRegionMode(const PropertyValue<TextureRegionMode>& textureRegionMode)
		{
			m_textureRegionMode.setPropertyValue(textureRegionMode);
			return shared_from_this();
		}
		
		[[nodiscard]]
		const PropertyValue<Vec2>& textureOffset() const
		{
			return m_textureOffset.propertyValue();
		}
		
		std::shared_ptr<Sprite> setTextureOffset(const PropertyValue<Vec2>& textureOffset)
		{
			m_textureOffset.setPropertyValue(textureOffset);
			return shared_from_this();
		}
		
		[[nodiscard]]
		const PropertyValue<Vec2>& textureSize() const
		{
			return m_textureSize.propertyValue();
		}
		
		std::shared_ptr<Sprite> setTextureSize(const PropertyValue<Vec2>& textureSize)
		{
			m_textureSize.setPropertyValue(textureSize);
			return shared_from_this();
		}
		
		[[nodiscard]]
		const PropertyValue<Vec2>& textureGridCellSize() const
		{
			return m_textureGridCellSize.propertyValue();
		}
		
		std::shared_ptr<Sprite> setTextureGridCellSize(const PropertyValue<Vec2>& textureGridCellSize)
		{
			m_textureGridCellSize.setPropertyValue(textureGridCellSize);
			return shared_from_this();
		}
		
		[[nodiscard]]
		const PropertyValue<int32>& textureGridColumns() const
		{
			return m_textureGridColumns.propertyValue();
		}
		
		std::shared_ptr<Sprite> setTextureGridColumns(const PropertyValue<int32>& textureGridColumns)
		{
			m_textureGridColumns.setPropertyValue(textureGridColumns);
			return shared_from_this();
		}
		
		[[nodiscard]]
		const PropertyValue<int32>& textureGridRows() const
		{
			return m_textureGridRows.propertyValue();
		}
		
		std::shared_ptr<Sprite> setTextureGridRows(const PropertyValue<int32>& textureGridRows)
		{
			m_textureGridRows.setPropertyValue(textureGridRows);
			return shared_from_this();
		}
		
		[[nodiscard]]
		const PropertyValue<int32>& textureGridIndex() const
		{
			return m_textureGridIndex.propertyValue();
		}
		
		std::shared_ptr<Sprite> setTextureGridIndex(const PropertyValue<int32>& textureGridIndex)
		{
			m_textureGridIndex.setPropertyValue(textureGridIndex);
			return shared_from_this();
		}
		
		std::shared_ptr<Sprite> setTexture(const Texture& texture)
		{
			m_textureOpt = texture;
			return shared_from_this();
		}
		
		std::shared_ptr<Sprite> clearTexture()
		{
			m_textureOpt.reset();
			return shared_from_this();
		}
		
		[[nodiscard]]
		const PropertyValue<SpriteGridAnimationType>& gridAnimationType() const
		{
			return m_gridAnimationType.propertyValue();
		}
		
		std::shared_ptr<Sprite> setGridAnimationType(const PropertyValue<SpriteGridAnimationType>& gridAnimationType)
		{
			m_gridAnimationType.setPropertyValue(gridAnimationType);
			return shared_from_this();
		}
		
		[[nodiscard]]
		double gridAnimationFPS() const
		{
			return m_gridAnimationFPS.value();
		}
		
		std::shared_ptr<Sprite> setGridAnimationFPS(double gridAnimationFPS)
		{
			m_gridAnimationFPS.setValue(gridAnimationFPS);
			return shared_from_this();
		}
		
		[[nodiscard]]
		int32 gridAnimationStartIndex() const
		{
			return m_gridAnimationStartIndex.value();
		}
		
		std::shared_ptr<Sprite> setGridAnimationStartIndex(int32 gridAnimationStartIndex)
		{
			m_gridAnimationStartIndex.setValue(gridAnimationStartIndex);
			return shared_from_this();
		}
		
		[[nodiscard]]
		int32 gridAnimationEndIndex() const
		{
			return m_gridAnimationEndIndex.value();
		}
		
		std::shared_ptr<Sprite> setGridAnimationEndIndex(int32 gridAnimationEndIndex)
		{
			m_gridAnimationEndIndex.setValue(gridAnimationEndIndex);
			return shared_from_this();
		}
		
		std::shared_ptr<Sprite> restartAnimation()
		{
			m_animationStopwatch.restart();
			m_currentGridAnimationIndex = 0;
			m_gridAnimationFinished = false;
			m_currentOffsetAnimation = Vec2::Zero();
			return shared_from_this();
		}
		
		[[nodiscard]]
		const PropertyValue<SpriteOffsetAnimationType>& offsetAnimationType() const
		{
			return m_offsetAnimationType.propertyValue();
		}
		
		std::shared_ptr<Sprite> setOffsetAnimationType(const PropertyValue<SpriteOffsetAnimationType>& offsetAnimationType)
		{
			m_offsetAnimationType.setPropertyValue(offsetAnimationType);
			return shared_from_this();
		}
		
		[[nodiscard]]
		Vec2 offsetAnimationSpeed() const
		{
			return m_offsetAnimationSpeed.value();
		}
		
		std::shared_ptr<Sprite> setOffsetAnimationSpeed(const Vec2& offsetAnimationSpeed)
		{
			m_offsetAnimationSpeed.setValue(offsetAnimationSpeed);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<SpriteTextureFilter>& textureFilter() const
		{
			return m_textureFilter.propertyValue();
		}

		std::shared_ptr<Sprite> setTextureFilter(const PropertyValue<SpriteTextureFilter>& textureFilter)
		{
			m_textureFilter.setPropertyValue(textureFilter);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<SpriteTextureAddressMode>& textureAddressMode() const
		{
			return m_textureAddressMode.propertyValue();
		}

		std::shared_ptr<Sprite> setTextureAddressMode(const PropertyValue<SpriteTextureAddressMode>& textureAddressMode)
		{
			m_textureAddressMode.setPropertyValue(textureAddressMode);
			return shared_from_this();
		}
	};
}
