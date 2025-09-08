#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"
#include "../Enums.hpp"

namespace noco
{
	enum class ShapeType : uint8
	{
		Cross,
		Plus,
		Pentagon,
		Hexagon,
		Ngon,
		Star,
		NStar,
		Arrow,
		DoubleHeadedArrow,
		Rhombus,
		RectBalloon,
		Stairs,
		Heart,
		Squircle,
		Astroid,
	};

	class ShapeRenderer : public SerializableComponentBase, public std::enable_shared_from_this<ShapeRenderer>
	{
	private:
		PropertyNonInteractive<ShapeType> m_shapeType;
		Property<bool> m_preserveAspect;
		SmoothProperty<double> m_thickness;
		Property<int32> m_sides;
		Property<int32> m_points;
		SmoothProperty<double> m_innerRatio;
		SmoothProperty<Vec2> m_startPoint;
		SmoothProperty<Vec2> m_endPoint;
		SmoothProperty<Vec2> m_arrowHeadSize;
		SmoothProperty<Vec2> m_targetPoint;
		SmoothProperty<double> m_tailRatio;
		Property<int32> m_stairCount;
		Property<bool> m_upStairs;
		Property<int32> m_squircleQuality;
		SmoothProperty<ColorF> m_fillColor;
		SmoothProperty<ColorF> m_outlineColor;
		SmoothProperty<double> m_outlineThickness;
		Property<BlendMode> m_blendMode;

		struct CacheParams
		{
			ShapeType shapeType;
			bool preserveAspect;
			double thickness;
			int32 sides;
			int32 points;
			double innerRatio;
			Vec2 startPoint;
			Vec2 endPoint;
			Vec2 arrowHeadSize;
			Vec2 targetPoint;
			double tailRatio;
			int32 stairCount;
			bool upStairs;
			int32 squircleQuality;
			SizeF regionSize;

			[[nodiscard]]
			bool isDirty(
				ShapeType newShapeType,
				bool newPreserveAspect,
				double newThickness,
				int32 newSides,
				int32 newPoints,
				double newInnerRatio,
				const Vec2& newStartPoint,
				const Vec2& newEndPoint,
				const Vec2& newArrowHeadSize,
				const Vec2& newTargetPoint,
				double newTailRatio,
				int32 newStairCount,
				bool newUpStairs,
				int32 newSquircleQuality,
				const SizeF& newRegionSize) const
			{
				return shapeType != newShapeType
					|| preserveAspect != newPreserveAspect
					|| thickness != newThickness
					|| sides != newSides
					|| points != newPoints
					|| innerRatio != newInnerRatio
					|| startPoint != newStartPoint
					|| endPoint != newEndPoint
					|| arrowHeadSize != newArrowHeadSize
					|| targetPoint != newTargetPoint
					|| tailRatio != newTailRatio
					|| stairCount != newStairCount
					|| upStairs != newUpStairs
					|| squircleQuality != newSquircleQuality
					|| regionSize != newRegionSize;
			}
		};

		struct Cache
		{
			Shape2D baseShape;
			Shape2D scaledShape;
			Optional<CacheParams> prevParams;
			bool isScaled = false;

			bool refreshIfDirty(
				ShapeType shapeType,
				bool preserveAspect,
				double thickness,
				int32 sides,
				int32 points,
				double innerRatio,
				const Vec2& startPoint,
				const Vec2& endPoint,
				const Vec2& arrowHeadSize,
				const Vec2& targetPoint,
				double tailRatio,
				int32 stairCount,
				bool upStairs,
				int32 squircleQuality,
				const SizeF& regionSize,
				const Vec2& center);
		};

		/* NonSerialized */ mutable Cache m_cache;

	public:
		explicit ShapeRenderer(
			ShapeType shapeType = ShapeType::Star,
			const PropertyValue<ColorF>& fillColor = Palette::White,
			const PropertyValue<ColorF>& outlineColor = Palette::Black,
			const PropertyValue<double>& outlineThickness = 0.0)
			: SerializableComponentBase{ U"ShapeRenderer", { 
				&m_shapeType, &m_preserveAspect, &m_thickness, &m_sides, &m_points, &m_innerRatio,
				&m_startPoint, &m_endPoint, &m_arrowHeadSize, &m_targetPoint,
				&m_tailRatio, &m_stairCount, &m_upStairs, &m_squircleQuality,
				&m_fillColor, &m_outlineColor, &m_outlineThickness, &m_blendMode, } }
			, m_shapeType{ U"shapeType", shapeType }
			, m_preserveAspect{ U"preserveAspect", true }
			, m_thickness{ U"thickness", 10.0 }
			, m_sides{ U"sides", 6 }
			, m_points{ U"points", 5 }
			, m_innerRatio{ U"innerRatio", 0.5 }
			, m_startPoint{ U"startPoint", Vec2{ 0.0, 0.0 } }
			, m_endPoint{ U"endPoint", Vec2{ 1.0, 1.0 } }
			, m_arrowHeadSize{ U"arrowHeadSize", Vec2{ 16.0, 16.0 } }
			, m_targetPoint{ U"targetPoint", Vec2{ 0.5, 1.2 } }
			, m_tailRatio{ U"tailRatio", 0.5 }
			, m_stairCount{ U"stairCount", 5 }
			, m_upStairs{ U"upStairs", true }
			, m_squircleQuality{ U"squircleQuality", 64 }
			, m_fillColor{ U"fillColor", fillColor }
			, m_outlineColor{ U"outlineColor", outlineColor }
			, m_outlineThickness{ U"outlineThickness", outlineThickness }
			, m_blendMode{ U"blendMode", BlendMode::Normal }
		{
		}

		void draw(const Node& node) const override;

		[[nodiscard]]
		ShapeType shapeType() const
		{
			return m_shapeType.value();
		}

		std::shared_ptr<ShapeRenderer> setShapeType(ShapeType shapeType)
		{
			m_shapeType.setValue(shapeType);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<bool>& preserveAspect() const
		{
			return m_preserveAspect.propertyValue();
		}

		std::shared_ptr<ShapeRenderer> setPreserveAspect(const PropertyValue<bool>& preserveAspect)
		{
			m_preserveAspect.setPropertyValue(preserveAspect);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<double>& thickness() const
		{
			return m_thickness.propertyValue();
		}

		std::shared_ptr<ShapeRenderer> setThickness(const PropertyValue<double>& thickness)
		{
			m_thickness.setPropertyValue(thickness);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<int32>& sides() const
		{
			return m_sides.propertyValue();
		}

		std::shared_ptr<ShapeRenderer> setSides(const PropertyValue<int32>& sides)
		{
			m_sides.setPropertyValue(sides);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<int32>& points() const
		{
			return m_points.propertyValue();
		}

		std::shared_ptr<ShapeRenderer> setPoints(const PropertyValue<int32>& points)
		{
			m_points.setPropertyValue(points);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<double>& innerRatio() const
		{
			return m_innerRatio.propertyValue();
		}

		std::shared_ptr<ShapeRenderer> setInnerRatio(const PropertyValue<double>& innerRatio)
		{
			m_innerRatio.setPropertyValue(innerRatio);
			return shared_from_this();
		}


		[[nodiscard]]
		const PropertyValue<Vec2>& startPoint() const
		{
			return m_startPoint.propertyValue();
		}

		std::shared_ptr<ShapeRenderer> setStartPoint(const PropertyValue<Vec2>& startPoint)
		{
			m_startPoint.setPropertyValue(startPoint);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<Vec2>& endPoint() const
		{
			return m_endPoint.propertyValue();
		}

		std::shared_ptr<ShapeRenderer> setEndPoint(const PropertyValue<Vec2>& endPoint)
		{
			m_endPoint.setPropertyValue(endPoint);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<Vec2>& arrowHeadSize() const
		{
			return m_arrowHeadSize.propertyValue();
		}

		std::shared_ptr<ShapeRenderer> setArrowHeadSize(const PropertyValue<Vec2>& arrowHeadSize)
		{
			m_arrowHeadSize.setPropertyValue(arrowHeadSize);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<Vec2>& targetPoint() const
		{
			return m_targetPoint.propertyValue();
		}

		std::shared_ptr<ShapeRenderer> setTargetPoint(const PropertyValue<Vec2>& targetPoint)
		{
			m_targetPoint.setPropertyValue(targetPoint);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<double>& tailRatio() const
		{
			return m_tailRatio.propertyValue();
		}

		std::shared_ptr<ShapeRenderer> setTailRatio(const PropertyValue<double>& tailRatio)
		{
			m_tailRatio.setPropertyValue(tailRatio);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<int32>& stairCount() const
		{
			return m_stairCount.propertyValue();
		}

		std::shared_ptr<ShapeRenderer> setStairCount(const PropertyValue<int32>& stairCount)
		{
			m_stairCount.setPropertyValue(stairCount);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<bool>& upStairs() const
		{
			return m_upStairs.propertyValue();
		}

		std::shared_ptr<ShapeRenderer> setUpStairs(const PropertyValue<bool>& upStairs)
		{
			m_upStairs.setPropertyValue(upStairs);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<int32>& squircleQuality() const
		{
			return m_squircleQuality.propertyValue();
		}

		std::shared_ptr<ShapeRenderer> setSquircleQuality(const PropertyValue<int32>& squircleQuality)
		{
			m_squircleQuality.setPropertyValue(squircleQuality);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<ColorF>& fillColor() const
		{
			return m_fillColor.propertyValue();
		}

		std::shared_ptr<ShapeRenderer> setFillColor(const PropertyValue<ColorF>& fillColor)
		{
			m_fillColor.setPropertyValue(fillColor);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<ColorF>& outlineColor() const
		{
			return m_outlineColor.propertyValue();
		}

		std::shared_ptr<ShapeRenderer> setOutlineColor(const PropertyValue<ColorF>& outlineColor)
		{
			m_outlineColor.setPropertyValue(outlineColor);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<double>& outlineThickness() const
		{
			return m_outlineThickness.propertyValue();
		}

		std::shared_ptr<ShapeRenderer> setOutlineThickness(const PropertyValue<double>& outlineThickness)
		{
			m_outlineThickness.setPropertyValue(outlineThickness);
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<BlendMode>& blendMode() const
		{
			return m_blendMode.propertyValue();
		}

		std::shared_ptr<ShapeRenderer> setBlendMode(const PropertyValue<BlendMode>& blendMode)
		{
			m_blendMode.setPropertyValue(blendMode);
			return shared_from_this();
		}
	};
}
