﻿#include "EditorDialog.hpp"
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
			BoxConstraint
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
				BoxConstraint
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = SizeF{ -20, 0 },
					.margin = LRTB{ 0, 0, 0, 8 },
				});
			propertyNode->emplaceChild(
				U"Spacing",
				BoxConstraint
				{
					.sizeRatio = Vec2{ 0, 0 },
					.sizeDelta = SizeF{ 8, 0 },
				});
			propertyNode->setBoxChildrenLayout(HorizontalLayout{}, RefreshesLayoutYN::No);
			const auto currentValueString = std::make_shared<String>(m_pProperty->propertyValueStringOfFallback(interactionState, activeStyleStates));
			std::shared_ptr<Node> propertyValueNode;
			switch (m_pProperty->editType())
			{
			case PropertyEditType::Text:
				propertyValueNode = propertyNode->addChild(
					Inspector::CreatePropertyNode(
						headingText,
						m_pProperty->propertyValueStringOfFallback(interactionState, activeStyleStates),
						[this, interactionState, currentValueString](StringView value)
						{
							// 現在のactiveStyleStatesを動的に構築
							Array<String> currentActiveStyleStates;
							if (!m_currentStyleState.isEmpty())
							{
								currentActiveStyleStates.push_back(m_currentStyleState);
							}

							if (m_pProperty->trySetPropertyValueStringOf(value, interactionState, currentActiveStyleStates))
							{
								*currentValueString = value;
								if (m_onChange)
								{
									m_onChange();
								}
							}
						}),
					RefreshesLayoutYN::No);
				break;
			case PropertyEditType::Bool:
				propertyValueNode = propertyNode->addChild(
					Inspector::CreateBoolPropertyNode(
						headingText,
						ParseOr<bool>(m_pProperty->propertyValueStringOfFallback(interactionState, activeStyleStates), false),
						[this, interactionState, currentValueString](bool value)
						{
							// 現在のactiveStyleStatesを動的に構築
							Array<String> currentActiveStyleStates;
							if (!m_currentStyleState.isEmpty())
							{
								currentActiveStyleStates.push_back(m_currentStyleState);
							}

							const String formattedValue = Format(value);
							if (m_pProperty->trySetPropertyValueStringOf(formattedValue, interactionState, currentActiveStyleStates))
							{
								*currentValueString = formattedValue;
								if (m_onChange)
								{
									m_onChange();
								}
							}
						}),
					RefreshesLayoutYN::No);
				break;
			case PropertyEditType::Vec2:
				propertyValueNode = propertyNode->addChild(
					Inspector::CreateVec2PropertyNode(
						headingText,
						ParseOr<Vec2>(m_pProperty->propertyValueStringOfFallback(interactionState, activeStyleStates), Vec2{ 0, 0 }),
						[this, interactionState, currentValueString](const Vec2& value)
						{
							// 現在のactiveStyleStatesを動的に構築
							Array<String> currentActiveStyleStates;
							if (!m_currentStyleState.isEmpty())
							{
								currentActiveStyleStates.push_back(m_currentStyleState);
							}

							const String formattedValue = Format(value);
							if (m_pProperty->trySetPropertyValueStringOf(formattedValue, interactionState, currentActiveStyleStates))
							{
								*currentValueString = formattedValue;
								if (m_onChange)
								{
									m_onChange();
								}
							}
						}),
					RefreshesLayoutYN::No);
				break;
			case PropertyEditType::Color:
				propertyValueNode = propertyNode->addChild(
					Inspector::CreateColorPropertyNode(
						headingText,
						ParseOr<ColorF>(m_pProperty->propertyValueStringOfFallback(interactionState, activeStyleStates), ColorF{ 0, 0, 0, 1 }),
						[this, interactionState, currentValueString](const ColorF& value)
						{
							// 現在のactiveStyleStatesを動的に構築
							Array<String> currentActiveStyleStates;
							if (!m_currentStyleState.isEmpty())
							{
								currentActiveStyleStates.push_back(m_currentStyleState);
							}

							const String formattedValue = Format(value);
							if (m_pProperty->trySetPropertyValueStringOf(formattedValue, interactionState, currentActiveStyleStates))
							{
								*currentValueString = formattedValue;
								if (m_onChange)
								{
									m_onChange();
								}
							}
						}),
					RefreshesLayoutYN::No);
				break;
			case PropertyEditType::LRTB:
				propertyValueNode = propertyNode->addChild(
					Inspector::CreateLRTBPropertyNode(
						headingText,
						ParseOr<LRTB>(m_pProperty->propertyValueStringOfFallback(interactionState, activeStyleStates), LRTB{ 0, 0, 0, 0 }),
						[this, interactionState, currentValueString](const LRTB& value)
						{
							// 現在のactiveStyleStatesを動的に構築
							Array<String> currentActiveStyleStates;
							if (!m_currentStyleState.isEmpty())
							{
								currentActiveStyleStates.push_back(m_currentStyleState);
							}

							const String formattedValue = Format(value);
							if (m_pProperty->trySetPropertyValueStringOf(formattedValue, interactionState, currentActiveStyleStates))
							{
								*currentValueString = formattedValue;
								if (m_onChange)
								{
									m_onChange();
								}
							}
						}),
					RefreshesLayoutYN::No);
				break;
			case PropertyEditType::Enum:
				propertyValueNode = propertyNode->addChild(
					Inspector::CreateEnumPropertyNode(
						headingText,
						m_pProperty->propertyValueStringOfFallback(interactionState, activeStyleStates),
						[this, interactionState, currentValueString](StringView value)
						{
							// 現在のactiveStyleStatesを動的に構築
							Array<String> currentActiveStyleStates;
							if (!m_currentStyleState.isEmpty())
							{
								currentActiveStyleStates.push_back(m_currentStyleState);
							}

							if (m_pProperty->trySetPropertyValueStringOf(value, interactionState, currentActiveStyleStates))
							{
								*currentValueString = value;
								if (m_onChange)
								{
									m_onChange();
								}
							}
						},
						dialogContextMenu,
						m_pProperty->enumCandidates()),
					RefreshesLayoutYN::No);
				break;
			}
			if (!propertyValueNode)
			{
				throw Error{ U"Property value node is nullptr" };
			}

			const auto checkboxNode = propertyNode->addChildAtIndex(Inspector::CreateCheckboxNode(
				m_pProperty->hasPropertyValueOf(interactionState, activeStyleStates),
				[this, interactionState, propertyValueNode, currentValueString](bool value)
				{
					// 現在のactiveStyleStatesを動的に構築
					Array<String> currentActiveStyleStates;
					if (!m_currentStyleState.isEmpty())
					{
						currentActiveStyleStates.push_back(m_currentStyleState);
					}

					if (value)
					{
						if (m_pProperty->trySetPropertyValueStringOf(*currentValueString, interactionState, currentActiveStyleStates))
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
						if (m_pProperty->tryUnsetPropertyValueOf(interactionState, currentActiveStyleStates))
						{
							propertyValueNode->setInteractable(false);
							if (m_onChange)
							{
								m_onChange();
							}
						}
					}
				}),
				0,
				RefreshesLayoutYN::No);
			// Defaultは常に値が存在するのでチェックボックスは無効
			checkboxNode->setInteractable(interactionState != InteractionState::Default);
			propertyValueNode->setInteractable(m_pProperty->hasPropertyValueOf(interactionState, activeStyleStates));
			propertyNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly, RefreshesLayoutYN::No);

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
				BoxConstraint{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = SizeF{ 0, 1 },
					.margin = LRTB{ 0, 0, 0, 8 },
				});
			separatorNode2->emplaceComponent<RectRenderer>(ColorF{ 1.0, 0.3 });

			const auto propertyNode = contentRootNode->emplaceChild(
				U"Property",
				BoxConstraint
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = SizeF{ 0, 0 },
					.margin = LRTB{ 0, 0, 0, 8 },
				});
			propertyNode->addChild(
				Inspector::CreatePropertyNode(
					U"smoothTime [sec]",
					Format(m_pProperty->smoothTime()),
					[this](StringView value) { m_pProperty->trySetSmoothTime(ParseFloatOpt<double>(value).value_or(m_pProperty->smoothTime())); }),
				RefreshesLayoutYN::No);
			propertyNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly, RefreshesLayoutYN::No);
		}

		// 初期表示時に正しい値を反映
		refreshPropertyValues();

		contentRootNode->refreshContainedCanvasLayout();
	}
}
