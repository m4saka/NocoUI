#include "ParamRefDialog.hpp"
#include "Inspector.hpp"

namespace noco::editor
{
	void ParamRefDialog::createParamRefValueRow(const std::shared_ptr<Node>& contentRootNode, const std::shared_ptr<ContextMenu>& dialogContextMenu)
	{
		m_paramRefValueRow = contentRootNode->emplaceChild(
			U"ParamRefValueRow",
			InlineRegion
			{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = Vec2{ 0, 32 },
				.margin = LRTB{ 0, 0, 4, 4 },
			});
		m_paramRefValueRow->setChildrenLayout(HorizontalLayout{ .spacing = 8 });

		const auto labelNode = m_paramRefValueRow->emplaceChild(
			U"ParamRefValueLabel",
			InlineRegion
			{
				.sizeDelta = Vec2{ 130, 32 },
			});
		labelNode->emplaceComponent<Label>(
			U"上書きする値:",
			U"",
			14,
			Palette::White,
			HorizontalAlign::Right,
			VerticalAlign::Middle);

		// ラベルは別のノードで表示するため、プロパティ名は空にする
		const StringView headingText = U"";
		std::shared_ptr<Node> inputNode;
		switch (m_pProperty->editType())
		{
		case PropertyEditType::Int:
		case PropertyEditType::Double:
		case PropertyEditType::Text:
			inputNode = m_paramRefValueRow->addChild(
				Inspector::CreatePropertyNode(
					headingText,
					m_paramRefValueString,
					[this](StringView value) { setParamRefValueString(value); }));
			break;
		case PropertyEditType::Bool:
			inputNode = m_paramRefValueRow->addChild(
				Inspector::CreateBoolPropertyNode(
					headingText,
					ParseOr<bool>(m_paramRefValueString, false),
					[this](bool value) { setParamRefValueString(Format(value)); }));
			break;
		case PropertyEditType::Vec2:
			inputNode = m_paramRefValueRow->addChild(
				Inspector::CreateVec2PropertyNode(
					headingText,
					ParseOr<Vec2>(m_paramRefValueString, Vec2{ 0, 0 }),
					[this](const Vec2& value) { setParamRefValueString(Format(value)); }));
			break;
		case PropertyEditType::Color:
			inputNode = m_paramRefValueRow->addChild(
				Inspector::CreateColorPropertyNode(
					headingText,
					ParseOr<Color>(m_paramRefValueString, Color{}),
					[this](const Color& value) { setParamRefValueString(Format(value)); }));
			break;
		case PropertyEditType::LRTB:
			inputNode = m_paramRefValueRow->addChild(
				Inspector::CreateLRTBPropertyNode(
					headingText,
					ParseOr<LRTB>(m_paramRefValueString, LRTB{ 0, 0, 0, 0 }),
					[this](const LRTB& value) { setParamRefValueString(Format(value)); }));
			break;
		case PropertyEditType::Enum:
			inputNode = m_paramRefValueRow->addChild(
				Inspector::CreateEnumPropertyNode(
					headingText,
					m_paramRefValueString,
					[this](StringView value) { setParamRefValueString(value); },
					dialogContextMenu,
					m_pProperty->enumCandidates()));
			break;
		}
		if (!inputNode)
		{
			throw Error{ U"Input node is nullptr" };
		}
		// 複数行の場合は行毎にLabelノードがあるため、全て非表示にする
		const std::function<void(const std::shared_ptr<Node>&)> fnHideLabelNodes = [&fnHideLabelNodes](const std::shared_ptr<Node>& node)
		{
			for (const auto& child : node->children())
			{
				if (child->name() == U"Label")
				{
					child->setActive(false);
				}
				else
				{
					fnHideLabelNodes(child);
				}
			}
		};
		fnHideLabelNodes(inputNode);

		// ラベルの右側の残りの幅に配置
		if (const InlineRegion* pInlineRegion = inputNode->inlineRegion())
		{
			InlineRegion inlineRegion = *pInlineRegion;
			inlineRegion.sizeRatio.x = 0.0;
			inlineRegion.flexibleWeight = 1.0;
			inputNode->setRegion(inlineRegion);
		}
		m_paramRefValueRow->setInlineRegionToFitToChildren(FitTarget::HeightOnly);
	}
}
