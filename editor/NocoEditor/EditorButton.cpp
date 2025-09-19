#include "EditorButton.hpp"
#include "EditorColor.hpp"

namespace noco::editor
{
	std::shared_ptr<Node> CreateButtonNode(StringView text, const RegionVariant& region, std::function<void(const std::shared_ptr<Node>&)> onClick, IsDefaultButtonYN isDefaultButton, double fontSize)
	{
		auto buttonNode = Node::Create(
			U"Button",
			region,
			IsHitTargetYN::Yes);
		buttonNode->setChildrenLayout(HorizontalLayout{ .horizontalAlign = HorizontalAlign::Center, .verticalAlign = VerticalAlign::Middle });
		buttonNode->emplaceComponent<RectRenderer>(EditorColor::ControlBackgroundColorValue(),
			isDefaultButton ? EditorColor::DefaultButtonBorderColorValue() : EditorColor::ButtonBorderColorValue(),
			1.0, 0.0, 4.0);
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
			fontSize,
			Palette::White,
			HorizontalAlign::Center,
			VerticalAlign::Middle,
			LRTB{ -2, -2, -2, -2 })
			->setSizingMode(LabelSizingMode::AutoShrink);
		return buttonNode;
	}
}
