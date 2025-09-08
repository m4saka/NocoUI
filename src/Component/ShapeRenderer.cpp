#include "NocoUI/Component/ShapeRenderer.hpp"
#include "NocoUI/Node.hpp"

namespace noco
{
	void ShapeRenderer::draw(const Node& node) const
	{
		const auto region = node.regionRect();
		const auto center = region.center();
		
		const ShapeType shapeType = m_shapeType.value();
		const bool preserveAspect = m_preserveAspect.value();
		const double thickness = m_thickness.value();
		const ColorF& fillColor = m_fillColor.value();
		const ColorF& outlineColor = m_outlineColor.value();
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
		
		// ノードサイズを基準に半径を計算
		const double minDimension = Min(region.w, region.h);
		const double radius = minDimension / 2.0;
		
		Shape2D shape;
		
		switch (shapeType)
		{
		case ShapeType::Cross:
			shape = Shape2D::Cross(radius, thickness, center, 0.0);
			break;
			
		case ShapeType::Plus:
			shape = Shape2D::Plus(radius, thickness, center, 0.0);
			break;
			
		case ShapeType::Pentagon:
			shape = Shape2D::Pentagon(radius, center, 0.0);
			break;
			
		case ShapeType::Hexagon:
			shape = Shape2D::Hexagon(radius, center, 0.0);
			break;
			
		case ShapeType::Ngon:
			{
				const auto sides = Max(m_sides.value(), 0);
				shape = Shape2D::Ngon(sides, radius, center, 0.0);
			}
			break;
			
		case ShapeType::Star:
			shape = Shape2D::Star(radius, center, 0.0);
			break;
			
		case ShapeType::NStar:
			{
				const auto points = Max(m_points.value(), 0);
				const auto innerRatio = m_innerRatio.value();
				const double innerRadius = radius * innerRatio;
				shape = Shape2D::NStar(points, radius, innerRadius, center, 0.0);
			}
			break;
			
		case ShapeType::Arrow:
			{
				const auto startPointRatio = m_startPoint.value();
				const auto endPointRatio = m_endPoint.value();
				const Vec2 arrowHeadSizePx = m_arrowHeadSize.value();
				
				// 0〜1の比率で指定（0,0が左上）
				const Vec2 startPoint{
					region.pos.x + startPointRatio.x * region.w,
					region.pos.y + startPointRatio.y * region.h
				};
				const Vec2 endPoint{
					region.pos.x + endPointRatio.x * region.w,
					region.pos.y + endPointRatio.y * region.h
				};
				
				shape = Shape2D::Arrow(startPoint, endPoint, thickness, arrowHeadSizePx);
			}
			break;
			
		case ShapeType::DoubleHeadedArrow:
			{
				const auto startPointRatio = m_startPoint.value();
				const auto endPointRatio = m_endPoint.value();
				const Vec2 arrowHeadSizePx = m_arrowHeadSize.value();
				
				// 0〜1の比率で指定（0,0が左上）
				const Vec2 startPoint{
					region.pos.x + startPointRatio.x * region.w,
					region.pos.y + startPointRatio.y * region.h
				};
				const Vec2 endPoint{
					region.pos.x + endPointRatio.x * region.w,
					region.pos.y + endPointRatio.y * region.h
				};
				
				shape = Shape2D::DoubleHeadedArrow(startPoint, endPoint, thickness, arrowHeadSizePx);
			}
			break;
			
		case ShapeType::Rhombus:
			{
				shape = Shape2D::Rhombus(region.w, region.h, center, 0.0);
			}
			break;
			
		case ShapeType::RectBalloon:
			{
				const auto targetPointRatio = m_targetPoint.value();
				const auto tailRatio = m_tailRatio.value();
				
				// 0〜1の比率で指定（0,0が左上）
				const Vec2 targetPoint{ 
					region.pos.x + targetPointRatio.x * region.w,
					region.pos.y + targetPointRatio.y * region.h 
				};
				
				shape = Shape2D::RectBalloon(region, targetPoint, tailRatio);
			}
			break;
			
		case ShapeType::Stairs:
			{
				const auto stairCount = Max(m_stairCount.value(), 0);
				const auto upStairs = m_upStairs.value();
				
				const Vec2 stairsPos{ center.x - region.w / 2 * (upStairs ? -1 : 1), center.y + region.h / 2 };
				shape = Shape2D::Stairs(stairsPos, region.w, region.h, stairCount, upStairs);
			}
			break;
			
		case ShapeType::Heart:
			shape = Shape2D::Heart(radius, center, 0.0);
			break;
			
		case ShapeType::Squircle:
			{
				const auto squircleQuality = Max(m_squircleQuality.value(), 1);
				shape = Shape2D::Squircle(radius, center, squircleQuality);
			}
			break;
			
		case ShapeType::Astroid:
			{
				shape = Shape2D::Astroid(center, region.w / 2, region.h / 2, 0.0);
			}
			break;
		}
		
		// 縦横比が1:1でない場合のスケーリング（半径ベース形状のみ適用）
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
		const bool needsScaling = !preserveAspect && region.w != region.h && isRadiusBasedShape && Abs(minDimension) > 0.0;
		
		if (needsScaling)
		{
			const double scaleX = region.w / minDimension;
			const double scaleY = region.h / minDimension;
			const Transformer2D transformer{ Mat3x2::Scale(scaleX, scaleY, center) };
			shape.draw(fillColor);
			if (outlineThickness > 0.0)
			{
				shape.drawFrame(outlineThickness, outlineColor);
			}
		}
		else
		{
			shape.draw(fillColor);
			if (outlineThickness > 0.0)
			{
				shape.drawFrame(outlineThickness, outlineColor);
			}
		}
	}
}
