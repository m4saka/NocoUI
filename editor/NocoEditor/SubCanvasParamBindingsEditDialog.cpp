#include "SubCanvasParamBindingsEditDialog.hpp"
#include "Inspector.hpp"

namespace noco::editor
{
	void SubCanvasParamBindingsEditDialog::createBindingRow(const std::shared_ptr<Node>& parentNode, size_t index, const std::shared_ptr<ContextMenu>& dialogContextMenu)
	{
		auto& info = m_bindings[index];

		const auto rowNode = parentNode->emplaceChild(
			U"BindingRow_{}"_fmt(index),
			InlineRegion
			{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = SizeF{ -20, 32 },
				.margin = LRTB{ 0, 0, 0, 4 },
			},
			IsHitTargetYN::Yes,
			InheritChildrenStateFlags::Hovered);
		rowNode->setChildrenLayout(HorizontalLayout{ .spacing = 8, .padding = LRTB{ 8, 0, 0, 0 } });
		rowNode->emplaceComponent<RectRenderer>(
			PropertyValue<Color>{ ColorF{ 1.0, 0.0 } }.withHovered(ColorF{ 1.0, 0.1 }),
			Palette::Black,
			0.0,
			0.0,
			3.0);
		info.rowNode = rowNode;

		// パラメータ名+型ラベル
		const auto labelNode = rowNode->emplaceChild(
			U"Label",
			InlineRegion
			{
				.sizeRatio = Vec2{ 0, 1 },
				.sizeDelta = Vec2{ 200, 0 },
			},
			IsHitTargetYN::No);
		labelNode->emplaceComponent<Label>(
			U"{} [{}]"_fmt(info.subCanvasParamName, ParamTypeToString(info.subCanvasParamType)),
			U"",
			14,
			Palette::White,
			HorizontalAlign::Left,
			VerticalAlign::Middle)
			->setSizingMode(LabelSizingMode::AutoShrink);

		// コンボボックス
		info.comboBoxNode = rowNode->emplaceChild(
			U"ComboBox",
			InlineRegion
			{
				.sizeRatio = Vec2{ 0, 1 },
				.sizeDelta = Vec2{ 0, 0 },
				.flexibleWeight = 1.0,
			});
		info.comboBoxNode->emplaceComponent<RectRenderer>(
			EditorColor::ControlBackgroundColorValue(),
			EditorColor::ButtonBorderColorValue(),
			1.0,
			0.0,
			4.0);

		const String displayText = info.selectedParentParamName.isEmpty() ? U"(なし)" : info.selectedParentParamName;
		info.comboLabel = info.comboBoxNode->emplaceComponent<Label>(
			displayText,
			U"",
			14,
			Palette::White,
			HorizontalAlign::Left,
			VerticalAlign::Middle,
			LRTB{ 8, 25, 0, 0 })
			->setSizingMode(LabelSizingMode::AutoShrink);

		// 下三角アイコン
		info.comboBoxNode->emplaceComponent<Label>(
			U"▼",
			U"",
			10,
			Palette::White,
			HorizontalAlign::Right,
			VerticalAlign::Middle,
			LRTB{ 5, 7, 5, 5 });

		info.comboBoxNode->emplaceComponent<UpdaterComponent>([this, index, dialogContextMenu](const std::shared_ptr<Node>& node)
		{
			if (node->isClicked())
			{
				onComboBoxClick(index, dialogContextMenu);
			}
		});
	}

	void SubCanvasParamBindingsEditDialog::onComboBoxClick(size_t index, const std::shared_ptr<ContextMenu>& dialogContextMenu)
	{
		auto& info = m_bindings[index];

		Array<MenuElement> menuElements;

		menuElements.push_back(MenuItem{
			.text = U"(なし)",
			.hotKeyText = U"",
			.mnemonicInput = none,
			.onClick = [this, index]() { selectParentParam(index, U""); },
		});

		if (!info.availableParentParams.empty())
		{
			menuElements.push_back(MenuSeparator{});

			for (const auto& [paramName, value] : info.availableParentParams)
			{
				const String valueStr = ParamValueToString(value);
				const String displayText = U"{} = {}"_fmt(paramName, valueStr);

				menuElements.push_back(MenuItem{
					.text = displayText,
					.hotKeyText = U"",
					.mnemonicInput = none,
					.onClick = [this, index, name = paramName]() { selectParentParam(index, name); },
				});
			}
		}

		menuElements.push_back(MenuSeparator{});

		menuElements.push_back(MenuItem{
			.text = U"新規パラメータ...",
			.hotKeyText = U"",
			.mnemonicInput = none,
			.onClick = [this, index]()
			{
				auto& bindingInfo = m_bindings[index];

				m_dialogOpener->openDialog(
					std::make_shared<AddParamDialog>(
						m_parentCanvas,
						std::function<void()>{},
						bindingInfo.subCanvasParamType,
						U"",
						[this, index](const String& createdParamName)
						{
							auto& bi = m_bindings[index];

							// 親Canvasから作成されたパラメータの値を取得して選択肢に追加
							if (auto paramValue = m_parentCanvas->paramValueOpt(createdParamName))
							{
								bi.availableParentParams.push_back({ createdParamName, *paramValue });
								bi.availableParentParams.sort_by([](const auto& a, const auto& b) { return a.first < b.first; });
							}

							selectParentParam(index, createdParamName);
						},
						m_dialogOpener,
						bindingInfo.subCanvasParamName));
			},
		});

		dialogContextMenu->show(info.comboBoxNode->regionRect().bl(), menuElements);
	}

	void SubCanvasParamBindingsEditDialog::selectParentParam(size_t index, const String& paramName)
	{
		auto& info = m_bindings[index];
		info.selectedParentParamName = paramName;

		const String displayText = paramName.isEmpty() ? U"(なし)" : paramName;
		info.comboLabel->setText(displayText);
	}
}
