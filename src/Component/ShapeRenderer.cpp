#include "NocoUI/Component/ShapeRenderer.hpp"
#include "NocoUI/Node.hpp"

namespace noco
{
	bool ShapeRenderer::Cache::refreshIfDirty(
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
		const Vec2& center)
	{
		if (prevParams.has_value() && !prevParams->isDirty(
			shapeType, preserveAspect, thickness, sides, points, innerRatio,
			startPoint, endPoint, arrowHeadSize, targetPoint, tailRatio,
			stairCount, upStairs, squircleQuality, regionSize))
		{
			return false;
		}

		prevParams = CacheParams
		{
			.shapeType = shapeType,
			.preserveAspect = preserveAspect,
			.thickness = thickness,
			.sides = sides,
			.points = points,
			.innerRatio = innerRatio,
			.startPoint = startPoint,
			.endPoint = endPoint,
			.arrowHeadSize = arrowHeadSize,
			.targetPoint = targetPoint,
			.tailRatio = tailRatio,
			.stairCount = stairCount,
			.upStairs = upStairs,
			.squircleQuality = squircleQuality,
			.regionSize = regionSize,
		};

		const double minDimension = Min(regionSize.x, regionSize.y);
		const double radius = minDimension / 2.0;

		switch (shapeType)
		{
		case ShapeType::Cross:
			baseShape = Shape2D::Cross(radius, thickness, center, 0.0);
			break;
		case ShapeType::Plus:
			baseShape = Shape2D::Plus(radius, thickness, center, 0.0);
			break;
		case ShapeType::Pentagon:
			baseShape = Shape2D::Pentagon(radius, center, 0.0);
			break;
		case ShapeType::Hexagon:
			baseShape = Shape2D::Hexagon(radius, center, 0.0);
			break;
		case ShapeType::Ngon:
			{
				const auto actualSides = Max(sides, 0);
				baseShape = Shape2D::Ngon(actualSides, radius, center, 0.0);
			}
			break;
		case ShapeType::Star:
			baseShape = Shape2D::Star(radius, center, 0.0);
			break;
		case ShapeType::NStar:
			{
				const auto actualPoints = Max(points, 0);
				const double innerRadius = radius * innerRatio;
				baseShape = Shape2D::NStar(actualPoints, radius, innerRadius, center, 0.0);
			}
			break;
		case ShapeType::Arrow:
			{
				const Vec2 startPoint_px{
					regionSize.x * startPoint.x,
					regionSize.y * startPoint.y
				};
				const Vec2 endPoint_px{
					regionSize.x * endPoint.x,
					regionSize.y * endPoint.y
				};
				baseShape = Shape2D::Arrow(startPoint_px, endPoint_px, thickness, arrowHeadSize);
			}
			break;
		case ShapeType::DoubleHeadedArrow:
			{
				const Vec2 startPoint_px{
					regionSize.x * startPoint.x,
					regionSize.y * startPoint.y
				};
				const Vec2 endPoint_px{
					regionSize.x * endPoint.x,
					regionSize.y * endPoint.y
				};
				baseShape = Shape2D::DoubleHeadedArrow(startPoint_px, endPoint_px, thickness, arrowHeadSize);
			}
			break;
		case ShapeType::Rhombus:
			baseShape = Shape2D::Rhombus(regionSize.x, regionSize.y, center, 0.0);
			break;
		case ShapeType::RectBalloon:
			{
				const Vec2 targetPoint_px{
					regionSize.x * targetPoint.x,
					regionSize.y * targetPoint.y
				};
				const RectF rect{ center.x - regionSize.x / 2, center.y - regionSize.y / 2, regionSize.x, regionSize.y };
				baseShape = Shape2D::RectBalloon(rect, targetPoint_px, tailRatio);
			}
			break;
		case ShapeType::Stairs:
			{
				const auto actualStairCount = Max(stairCount, 0);
				const Vec2 stairsPos{ center.x - regionSize.x / 2 * (upStairs ? -1 : 1), center.y + regionSize.y / 2 };
				baseShape = Shape2D::Stairs(stairsPos, regionSize.x, regionSize.y, actualStairCount, upStairs);
			}
			break;
		case ShapeType::Heart:
			baseShape = Shape2D::Heart(radius, center, 0.0);
			break;
		case ShapeType::Squircle:
			{
				const auto actualQuality = Max(squircleQuality, 1);
				baseShape = Shape2D::Squircle(radius, center, actualQuality);
			}
			break;
		case ShapeType::Astroid:
			baseShape = Shape2D::Astroid(center, regionSize.x / 2, regionSize.y / 2, 0.0);
			break;
		}

		const bool isRadiusBasedShape =
			shapeType == ShapeType::Cross
			|| shapeType == ShapeType::Plus
			|| shapeType == ShapeType::Pentagon
			|| shapeType == ShapeType::Hexagon
			|| shapeType == ShapeType::Ngon
			|| shapeType == ShapeType::Star
			|| shapeType == ShapeType::NStar
			|| shapeType == ShapeType::Heart
			|| shapeType == ShapeType::Squircle;

		isScaled = !preserveAspect && regionSize.x != regionSize.y && isRadiusBasedShape && Abs(minDimension) > 0.0;

		if (isScaled)
		{
			const double scaleX = regionSize.x / minDimension;
			const double scaleY = regionSize.y / minDimension;

			Array<Float2> scaledVertices;
			scaledVertices.reserve(baseShape.vertices().size());

			for (const auto& vertex : baseShape.vertices())
			{
				const Float2 scaledVertex{
					(vertex.x - center.x) * scaleX + center.x,
					(vertex.y - center.y) * scaleY + center.y
				};
				scaledVertices.push_back(scaledVertex);
			}

			scaledShape = Shape2D{ scaledVertices, baseShape.indices() };
		}

		return true;
	}

	void ShapeRenderer::draw(const Node& node) const
	{
		const auto region = node.regionRect();
		const auto center = region.center();
		
		const Color& fillColor = m_fillColor.value();
		const Color& outlineColor = m_outlineColor.value();
		const double outlineThickness = m_outlineThickness.value();
		const BlendMode blendModeValue = m_blendMode.value();
		
		Optional<ScopedRenderStates2D> blendState;
		switch (blendModeValue)
		{
		case BlendMode::Additive:
			blendState.emplace(BlendState::Additive);
			break;
		case BlendMode::Subtractive:
			blendState.emplace(BlendState::Subtractive);
			break;
		case BlendMode::Multiply:
			blendState.emplace(BlendState::Multiplicative);
			break;
		default:
			break;
		}
		
		m_cache.refreshIfDirty(
			m_shapeType.value(),
			m_preserveAspect.value(),
			m_thickness.value(),
			m_sides.value(),
			m_points.value(),
			m_innerRatio.value(),
			m_startPoint.value(),
			m_endPoint.value(),
			m_arrowHeadSize.value(),
			m_targetPoint.value(),
			m_tailRatio.value(),
			m_stairCount.value(),
			m_upStairs.value(),
			m_squircleQuality.value(),
			region.size,
			center);
		
		const Shape2D& shapeToRender = m_cache.isScaled ? m_cache.scaledShape : m_cache.baseShape;
		shapeToRender.draw(fillColor);
		
		if (outlineThickness > 0.0)
		{
			shapeToRender.drawFrame(outlineThickness, outlineColor);
		}
	}
}
