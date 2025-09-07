#include "EditorButton.hpp"

namespace noco::editor
{
	std::shared_ptr<Node> CreateButtonNode(StringView text, const RegionVariant& region, std::function<void(const std::shared_ptr<Node>&)> onClick, IsDefaultButtonYN isDefaultButton)
	{
		auto buttonNode = Node::Create(
			U"Button",
			region,
			IsHitTargetYN::Yes);
		buttonNode->setChildrenLayout(HorizontalLayout{ .horizontalAlign = HorizontalAlign::Center, .verticalAlign = VerticalAlign::Middle });
		buttonNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, isDefaultButton ? 0.6 : 0.4 } }.withHovered(ColorF{ 1.0, isDefaultButton ? 0.8 : 0.6 }).withSmoothTime(0.05), 1.0, 0.0, 4.0);
		buttonNode->addOnClick([onClick](const std::shared_ptr<Node>& node) { if (onClick) onClick(node); });
		const auto labelNode = buttonNode->emplaceChild(
			U"ButtonLabel",
			InlineRegion
			{
				.sizeRatio = Vec2{ 1, 1 },
				.margin = LRTB{ 0, 0, 0, 0 },
			},
			IsHitTargetYN::No);
		labelNode->emplaceComponent<Label>(
			text,
			U"",
			14,
			Palette::White,
			HorizontalAlign::Center,
			VerticalAlign::Middle,
			LRTB{ -2, -2, -2, -2 })
			->setSizingMode(LabelSizingMode::AutoShrink);
		return buttonNode;
	}
}
