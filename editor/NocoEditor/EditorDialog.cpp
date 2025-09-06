#include "EditorDialog.hpp"
#include "Inspector.hpp"

namespace noco::editor
{
	void editor::InteractivePropertyValueDialog::createDialogContent(const std::shared_ptr<Node>& contentRootNode, const std::shared_ptr<ContextMenu>& dialogContextMenu)
	{
		if (!m_pProperty)
		{
			throw Error{ U"Property is nullptr" };
		}

		const auto labelNode = contentRootNode->emplaceChild(
			U"Label",
			InlineRegion
			{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = SizeF{ 0, 36 },
				.margin = LRTB{ 0, 0, 0, 8 },
			});
		labelNode->emplaceComponent<Label>(
			m_pProperty->name(),
			U"",
			14,
			Palette::White,
			HorizontalAlign::Center,
			VerticalAlign::Middle);

		createStyleStateSection(contentRootNode, dialogContextMenu);

		// 現在のstyleStateに基づいてactiveStyleStatesを構築
		Array<String> activeStyleStates{};
		if (!m_currentStyleState.isEmpty())
		{
			activeStyleStates.push_back(m_currentStyleState);
		}
		for (const auto interactionState : { InteractionState::Default, InteractionState::Hovered, InteractionState::Pressed, InteractionState::Disabled })
		{
			const String headingText = EnumToString(interactionState);

			const auto propertyNode = contentRootNode->emplaceChild(
				U"Property",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = SizeF{ -20, 0 },
					.margin = LRTB{ 0, 0, 0, 8 },
				});
			propertyNode->setChildrenLayout(HorizontalLayout{ .padding = LRTB{ 8, 0, 0, 0 } });
			const auto currentValueString = std::make_shared<String>(m_pProperty->propertyValueStringOfFallback(interactionState, activeStyleStates));
			std::shared_ptr<Node> propertyValueNode;
			switch (m_pProperty->editType())
			{
			case PropertyEditType::Number:
			case PropertyEditType::Text:
				propertyValueNode = propertyNode->addChild(
					Inspector::CreatePropertyNode(
						headingText,
						m_pProperty->propertyValueStringOfFallback(interactionState, activeStyleStates),
						[this, interactionState, currentValueString](StringView value)
						{
							if (m_pProperty->trySetPropertyValueStringOf(value, interactionState, m_currentStyleState))
							{
								*currentValueString = value;
								if (m_onChange)
								{
									m_onChange();
								}
							}
						}));
				break;
			case PropertyEditType::Bool:
				propertyValueNode = propertyNode->addChild(
					Inspector::CreateBoolPropertyNode(
						headingText,
						ParseOr<bool>(m_pProperty->propertyValueStringOfFallback(interactionState, activeStyleStates), false),
						[this, interactionState, currentValueString](bool value)
						{
							const String formattedValue = Format(value);
							if (m_pProperty->trySetPropertyValueStringOf(formattedValue, interactionState, m_currentStyleState))
							{
								*currentValueString = formattedValue;
								if (m_onChange)
								{
									m_onChange();
								}
							}
						}));
				break;
			case PropertyEditType::Vec2:
				propertyValueNode = propertyNode->addChild(
					Inspector::CreateVec2PropertyNode(
						headingText,
						ParseOr<Vec2>(m_pProperty->propertyValueStringOfFallback(interactionState, activeStyleStates), Vec2{ 0, 0 }),
						[this, interactionState, currentValueString](const Vec2& value)
						{
							const String formattedValue = Format(value);
							if (m_pProperty->trySetPropertyValueStringOf(formattedValue, interactionState, m_currentStyleState))
							{
								*currentValueString = formattedValue;
								if (m_onChange)
								{
									m_onChange();
								}
							}
						}));
				break;
			case PropertyEditType::Color:
				propertyValueNode = propertyNode->addChild(
					Inspector::CreateColorPropertyNode(
						headingText,
						ParseOr<ColorF>(m_pProperty->propertyValueStringOfFallback(interactionState, activeStyleStates), ColorF{}),
						[this, interactionState, currentValueString](const ColorF& value)
						{
							const String formattedValue = Format(value);
							if (m_pProperty->trySetPropertyValueStringOf(formattedValue, interactionState, m_currentStyleState))
							{
								*currentValueString = formattedValue;
								if (m_onChange)
								{
									m_onChange();
								}
							}
						}));
				break;
			case PropertyEditType::LRTB:
				propertyValueNode = propertyNode->addChild(
					Inspector::CreateLRTBPropertyNode(
						headingText,
						ParseOr<LRTB>(m_pProperty->propertyValueStringOfFallback(interactionState, activeStyleStates), LRTB{ 0, 0, 0, 0 }),
						[this, interactionState, currentValueString](const LRTB& value)
						{
							const String formattedValue = Format(value);
							if (m_pProperty->trySetPropertyValueStringOf(formattedValue, interactionState, m_currentStyleState))
							{
								*currentValueString = formattedValue;
								if (m_onChange)
								{
									m_onChange();
								}
							}
						}));
				break;
			case PropertyEditType::Enum:
				propertyValueNode = propertyNode->addChild(
					Inspector::CreateEnumPropertyNode(
						headingText,
						m_pProperty->propertyValueStringOfFallback(interactionState, activeStyleStates),
						[this, interactionState, currentValueString](StringView value)
						{
							if (m_pProperty->trySetPropertyValueStringOf(value, interactionState, m_currentStyleState))
							{
								*currentValueString = value;
								if (m_onChange)
								{
									m_onChange();
								}
							}
						},
						dialogContextMenu,
						m_pProperty->enumCandidates()));
				break;
			}
			if (!propertyValueNode)
			{
				throw Error{ U"Property value node is nullptr" };
			}

			const auto checkboxNode = propertyNode->addChildAtIndex(Inspector::CreateCheckboxNode(
				m_pProperty->hasPropertyValueOf(interactionState, m_currentStyleState),
				[this, interactionState, propertyValueNode, currentValueString](bool value)
				{
					if (value)
					{
						if (m_pProperty->trySetPropertyValueStringOf(*currentValueString, interactionState, m_currentStyleState))
						{
							propertyValueNode->setInteractable(true);
							if (m_onChange)
							{
								m_onChange();
							}
						}
					}
					else
					{
						m_pProperty->unsetPropertyValueOf(interactionState, m_currentStyleState);
						propertyValueNode->setInteractable(false);
						if (m_onChange)
						{
							m_onChange();
						}
					}
				}),
				0);
			// Defaultは常に値が存在するのでチェックボックスは無効
			checkboxNode->setInteractable(interactionState != InteractionState::Default);
			propertyValueNode->setInteractable(m_pProperty->hasPropertyValueOf(interactionState, m_currentStyleState));
			propertyNode->setInlineRegionToFitToChildren(FitTarget::HeightOnly);

			// PropertyValueNodeInfoを保存
			m_propertyValueNodes[interactionState] = PropertyValueNodeInfo{
				.propertyNode = propertyNode,
				.propertyValueNode = propertyValueNode,
				.checkboxNode = checkboxNode,
				.currentValueString = currentValueString
			};
		}

		// SmoothPropertyの場合はsmoothTimeの項目を追加
		if (m_pProperty->isSmoothProperty())
		{
			// 区切り線
			const auto separatorNode2 = contentRootNode->emplaceChild(
				U"Separator",
				InlineRegion{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = SizeF{ 0, 1 },
					.margin = LRTB{ 0, 0, 0, 8 },
				});
			separatorNode2->emplaceComponent<RectRenderer>(ColorF{ 1.0, 0.3 });

			const auto propertyNode = contentRootNode->emplaceChild(
				U"Property",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = SizeF{ 0, 0 },
					.margin = LRTB{ 0, 0, 0, 8 },
				});
			propertyNode->addChild(
				Inspector::CreatePropertyNode(
					U"smoothTime [sec]",
					Format(m_pProperty->smoothTime()),
					[this](StringView value) { m_pProperty->trySetSmoothTime(ParseFloatOpt<double>(value).value_or(m_pProperty->smoothTime())); }));
			propertyNode->setInlineRegionToFitToChildren(FitTarget::HeightOnly);
		}

		// 初期表示時に正しい値を反映
		refreshPropertyValues();
	}
}
