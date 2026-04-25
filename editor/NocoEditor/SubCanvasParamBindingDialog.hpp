#pragma once
#include <NocoUI.hpp>
#include "AddParamDialog.hpp"
#include "EditorButton.hpp"
#include "EditorColor.hpp"

namespace noco::editor
{
	class SubCanvasParamBindingDialog : public IDialog
	{
	private:
		std::shared_ptr<SubCanvas> m_subCanvas;
		String m_childParamName;
		ParamType m_childParamType;
		ParamValue m_childParamDefaultValue;
		std::shared_ptr<Canvas> m_parentCanvas;
		std::function<void()> m_onComplete;

		// ダイアログ内のコントロール
		std::shared_ptr<Node> m_comboBox;
		std::shared_ptr<Label> m_comboLabel;
		std::shared_ptr<Label> m_valueLabel;
		std::shared_ptr<Label> m_appliedValueLabel;
		std::shared_ptr<Node> m_warningNode;
		std::shared_ptr<Node> m_modeComboBox;
		std::shared_ptr<Label> m_modeComboLabel;

		String m_selectedParamName;
		ParamRefMode m_selectedMode = ParamRefMode::Normal;
		Array<std::pair<String, ParamValue>> m_availableParams;
		Array<ParamRefMode> m_availableModes;

		// ダイアログ管理
		std::shared_ptr<DialogOpener> m_dialogOpener;

		std::function<void()> m_fnRefreshLayoutForContent = nullptr;

		void filterAvailableParams()
		{
			m_availableParams.clear();

			for (const auto& [name, value] : m_parentCanvas->params())
			{
				if (IsParamTypeCompatibleWith(GetParamType(value), m_childParamType))
				{
					m_availableParams.push_back({ name, value });
				}
			}

			m_availableParams.sort_by([](const auto& a, const auto& b) { return a.first < b.first; });

			if (m_warningNode)
			{
				m_warningNode->setActive(m_availableParams.empty());
				if (m_fnRefreshLayoutForContent)
				{
					m_fnRefreshLayoutForContent();
				}
			}
		}

		String getParamValueString(const ParamValue& value) const
		{
			return ParamValueToString(value);
		}

		ParamValue resolveBaseValue() const
		{
			const String paramsJSONString = m_subCanvas->serializedParamsJSON();
			if (!paramsJSONString.isEmpty() && paramsJSONString != U"{}")
			{
				const JSON paramsJSON = JSON::Parse(paramsJSONString);
				if (paramsJSON.isObject() && paramsJSON.hasElement(m_childParamName))
				{
					if (auto pv = ParamValueFromJSONValue(paramsJSON[m_childParamName], m_childParamType))
					{
						return *pv;
					}
				}
			}
			return m_childParamDefaultValue;
		}

	public:
		explicit SubCanvasParamBindingDialog(
			const std::shared_ptr<SubCanvas>& subCanvas,
			const String& childParamName,
			ParamType childParamType,
			const ParamValue& childParamDefaultValue,
			const std::shared_ptr<Canvas>& parentCanvas,
			std::function<void()> onComplete,
			const std::shared_ptr<DialogOpener>& dialogOpener)
			: m_subCanvas(subCanvas)
			, m_childParamName(childParamName)
			, m_childParamType(childParamType)
			, m_childParamDefaultValue(childParamDefaultValue)
			, m_parentCanvas(parentCanvas)
			, m_onComplete(std::move(onComplete))
			, m_dialogOpener(dialogOpener)
		{
			m_availableModes = AvailableParamRefModesFor(m_childParamType);

			// 既存の紐付けから初期値を取得
			const String bindingsJSONString = m_subCanvas->serializedParamBindingsJSON();
			if (!bindingsJSONString.isEmpty() && bindingsJSONString != U"{}")
			{
				const JSON bindingsJSON = JSON::Parse(bindingsJSONString);
				if (bindingsJSON.isObject() && bindingsJSON.hasElement(m_childParamName))
				{
					const auto& v = bindingsJSON[m_childParamName];
					if (v.isString())
					{
						m_selectedParamName = v.getString();
					}
				}
			}
			const String modesJSONString = m_subCanvas->serializedParamBindingModesJSON();
			if (!modesJSONString.isEmpty() && modesJSONString != U"{}")
			{
				const JSON modesJSON = JSON::Parse(modesJSONString);
				if (modesJSON.isObject() && modesJSON.hasElement(m_childParamName))
				{
					const auto& v = modesJSON[m_childParamName];
					if (v.isString())
					{
						if (auto opt = StringToEnumOpt<ParamRefMode>(v.getString()))
						{
							if (m_availableModes.contains(*opt))
							{
								m_selectedMode = *opt;
							}
						}
					}
				}
			}
		}

		double dialogWidth() const override
		{
			return 480;
		}

		Array<DialogButtonDesc> buttonDescs() const override
		{
			return
			{
				DialogButtonDesc
				{
					.text = U"OK",
					.mnemonicInput = KeyO,
					.appendsMnemonicKeyText = AppendsMnemonicKeyTextYN::No,
					.isDefaultButton = IsDefaultButtonYN::Yes,
				},
				DialogButtonDesc
				{
					.text = U"キャンセル",
					.mnemonicInput = KeyC,
					.isCancelButton = IsCancelButtonYN::Yes,
				},
			};
		}

		void createDialogContent(const std::shared_ptr<Node>& contentRootNode, const std::shared_ptr<ContextMenu>& dialogContextMenu, std::function<void()> fnRefreshLayoutForContent) override
		{
			m_fnRefreshLayoutForContent = std::move(fnRefreshLayoutForContent);

			// タイトル
			const auto titleNode = contentRootNode->emplaceChild(
				U"Title",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 32 },
					.margin = LRTB{ 0, 0, 8, 8 },
				});
			titleNode->emplaceComponent<Label>(
				U"紐付けるパラメータを選択",
				U"",
				16,
				Palette::White,
				HorizontalAlign::Center,
				VerticalAlign::Middle);

			// 子Canvasパラメータ情報
			const auto propInfoNode = contentRootNode->emplaceChild(
				U"PropInfo",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 24 },
					.margin = LRTB{ 0, 0, 4, 8 },
				});
			propInfoNode->emplaceComponent<Label>(
				U"子Canvasパラメータ: {} ({}型)"_fmt(m_childParamName, ParamTypeToString(m_childParamType)),
				U"",
				14,
				ColorF{ 0.7, 0.7, 0.7 },
				HorizontalAlign::Center,
				VerticalAlign::Middle);

			// パラメータ選択コンボボックス
			const auto comboRow = contentRootNode->emplaceChild(
				U"ComboRow",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 32 },
					.margin = LRTB{ 0, 0, 4, 4 },
				});
			comboRow->setChildrenLayout(HorizontalLayout{ .spacing = 8 });

			const auto comboLabelNode = comboRow->emplaceChild(
				U"ComboLabel",
				InlineRegion
				{
					.sizeDelta = Vec2{ 130, 32 },
				});
			comboLabelNode->emplaceComponent<Label>(
				U"パラメータ:",
				U"",
				14,
				Palette::White,
				HorizontalAlign::Right,
				VerticalAlign::Middle);

			m_comboBox = comboRow->emplaceChild(
				U"ComboBox",
				InlineRegion
				{
					.sizeRatio = Vec2{ 0, 0 },
					.sizeDelta = Vec2{ 0, 26 },
					.flexibleWeight = 1.0,
				});
			m_comboBox->emplaceComponent<RectRenderer>(
				EditorColor::ControlBackgroundColorValue(),
				EditorColor::ButtonBorderColorValue(),
				1.0, 0.0, 4.0);

			// 現在選択されているパラメータ名を表示
			String displayText = m_selectedParamName.isEmpty() ? U"(なし)" : m_selectedParamName;
			m_comboLabel = m_comboBox->emplaceComponent<Label>(
				displayText,
				U"",
				14,
				Palette::White,
				HorizontalAlign::Left,
				VerticalAlign::Middle,
				LRTB{ 8, 25, 0, 0 })
				->setSizingMode(LabelSizingMode::AutoShrink);

			// 下三角アイコンを追加
			m_comboBox->emplaceComponent<Label>(
				U"▼",
				U"",
				10,
				Palette::White,
				HorizontalAlign::Right,
				VerticalAlign::Middle,
				LRTB{ 5, 7, 5, 5 });

			m_comboBox->emplaceComponent<UpdaterComponent>([this, dialogContextMenu](const std::shared_ptr<Node>& node)
			{
				if (node->isClicked())
				{
					onComboBoxClick(dialogContextMenu);
				}
			});

			// 新規パラメータ作成ボタン
			const auto newParamButton = comboRow->addChild(CreateButtonNode(
				U"＋ 新規",
				InlineRegion
				{
					.sizeDelta = Vec2{ 90, 26 },
				},
				[this](const std::shared_ptr<Node>&) { onCreateNewParamButtonClick(); },
				IsDefaultButtonYN::No,
				12));

			// モード選択コンボボックス
			const auto modeRow = contentRootNode->emplaceChild(
				U"ModeRow",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 32 },
					.margin = LRTB{ 0, 0, 4, 4 },
				});
			modeRow->setChildrenLayout(HorizontalLayout{ .spacing = 8 });

			const auto modeLabelNode = modeRow->emplaceChild(
				U"ModeLabel",
				InlineRegion
				{
					.sizeDelta = Vec2{ 130, 32 },
				});
			modeLabelNode->emplaceComponent<Label>(
				U"反映モード:",
				U"",
				14,
				Palette::White,
				HorizontalAlign::Right,
				VerticalAlign::Middle);

			m_modeComboBox = modeRow->emplaceChild(
				U"ModeComboBox",
				InlineRegion
				{
					.sizeRatio = Vec2{ 0, 0 },
					.sizeDelta = Vec2{ 0, 26 },
					.flexibleWeight = 1.0,
				});

			// 選択肢が1つ以下の場合は変更不可なのでグレーアウト
			const bool modeSelectable = m_availableModes.size() > 1;
			if (!modeSelectable)
			{
				m_modeComboBox->setInteractable(false);
			}

			m_modeComboBox->emplaceComponent<RectRenderer>(
				PropertyValue<Color>{ Color{ 26, 26, 26, 204 } }.withDisabled(Color{ 51, 51, 51, 204 }).withSmoothTime(0.05),
				PropertyValue<Color>{ Color{ 255, 255, 255, 102 } }.withHovered(Color{ 255, 255, 255, 153 }).withSmoothTime(0.05),
				1.0, 0.0, 4.0);

			m_modeComboLabel = m_modeComboBox->emplaceComponent<Label>(
				String{ ParamRefModeToDisplayString(m_selectedMode) },
				U"",
				14,
				PropertyValue<Color>{ Palette::White }.withDisabled(Color{ 153, 153, 153 }),
				HorizontalAlign::Left,
				VerticalAlign::Middle,
				LRTB{ 8, 25, 0, 0 })
				->setSizingMode(LabelSizingMode::AutoShrink);

			if (modeSelectable)
			{
				m_modeComboBox->emplaceComponent<Label>(
					U"▼",
					U"",
					10,
					Palette::White,
					HorizontalAlign::Right,
					VerticalAlign::Middle,
					LRTB{ 5, 7, 5, 5 });

				m_modeComboBox->emplaceComponent<UpdaterComponent>([this, dialogContextMenu](const std::shared_ptr<Node>& node)
				{
					if (node->isClicked())
					{
						onModeComboBoxClick(dialogContextMenu);
					}
				});
			}

			// パラメータの値表示
			const auto valueRow = contentRootNode->emplaceChild(
				U"ValueRow",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 32 },
					.margin = LRTB{ 0, 0, 4, 4 },
				});
			valueRow->setChildrenLayout(HorizontalLayout{ .spacing = 8 });

			const auto valueLabelNode = valueRow->emplaceChild(
				U"ValueLabel",
				InlineRegion
				{
					.sizeDelta = Vec2{ 130, 32 },
				});
			valueLabelNode->emplaceComponent<Label>(
				U"パラメータの値:",
				U"",
				14,
				Palette::White,
				HorizontalAlign::Right,
				VerticalAlign::Middle);

			const auto valueDisplayNode = valueRow->emplaceChild(
				U"ValueDisplay",
				InlineRegion
				{
					.sizeRatio = Vec2{ 0, 0 },
					.sizeDelta = Vec2{ 0, 26 },
					.flexibleWeight = 1.0,
				});
			valueDisplayNode->emplaceComponent<RectRenderer>(ColorF{ 0.05, 0.8 }, ColorF{ 0.5, 0.4 }, 1.0, 0.0, 4.0);

			m_valueLabel = valueDisplayNode->emplaceComponent<Label>(
				U"",
				U"",
				14,
				ColorF{ 0.9, 0.9, 0.9 },
				HorizontalAlign::Center,
				VerticalAlign::Middle,
				LRTB{ 4, 4, 0, 0 })
				->setSizingMode(LabelSizingMode::AutoShrink);

			// 反映後の値表示
			const auto appliedValueRow = contentRootNode->emplaceChild(
				U"AppliedValueRow",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 32 },
					.margin = LRTB{ 0, 0, 4, 8 },
				});
			appliedValueRow->setChildrenLayout(HorizontalLayout{ .spacing = 8 });

			const auto appliedValueLabelNode = appliedValueRow->emplaceChild(
				U"AppliedValueLabel",
				InlineRegion
				{
					.sizeDelta = Vec2{ 130, 32 },
				});
			appliedValueLabelNode->emplaceComponent<Label>(
				U"反映後の値:",
				U"",
				14,
				Palette::White,
				HorizontalAlign::Right,
				VerticalAlign::Middle);

			const auto appliedValueDisplayNode = appliedValueRow->emplaceChild(
				U"AppliedValueDisplay",
				InlineRegion
				{
					.sizeRatio = Vec2{ 0, 0 },
					.sizeDelta = Vec2{ 0, 26 },
					.flexibleWeight = 1.0,
				});
			appliedValueDisplayNode->emplaceComponent<RectRenderer>(ColorF{ 0.05, 0.8 }, ColorF{ 0.5, 0.4 }, 1.0, 0.0, 4.0);

			m_appliedValueLabel = appliedValueDisplayNode->emplaceComponent<Label>(
				U"",
				U"",
				14,
				ColorF{ 0.9, 0.9, 0.9 },
				HorizontalAlign::Center,
				VerticalAlign::Middle,
				LRTB{ 4, 4, 0, 0 })
				->setSizingMode(LabelSizingMode::AutoShrink);

			updateValueLabels();

			// 利用可能なパラメータがない場合の警告
			m_warningNode = contentRootNode->emplaceChild(
				U"Warning",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 32 },
					.margin = LRTB{ 0, 0, 8, 8 },
				});
			m_warningNode->emplaceComponent<Label>(
				U"※ この型に対応するパラメータはまだ作成されていません。\n　 「＋ 新規」ボタンからパラメータを作成できます。",
				U"",
				12,
				ColorF{ 1.0, 1.0, 0.7 },
				HorizontalAlign::Left,
				VerticalAlign::Middle)
				->setPadding(LRTB{ 24, 24, 0, 0 });

			filterAvailableParams();
		}

		void onComboBoxClick(const std::shared_ptr<ContextMenu>& dialogContextMenu)
		{
			Array<MenuElement> menuElements;

			// "(なし)" オプション
			menuElements.push_back(MenuItem{
				.text = U"(なし)",
				.hotKeyText = U"",
				.mnemonicInput = none,
				.onClick = [this]() { selectParam(U""); }
			});

			if (!m_availableParams.empty())
			{
				menuElements.push_back(MenuSeparator{});

				// 利用可能なパラメータを追加
				for (const auto& [paramName, value] : m_availableParams)
				{
					const String valueStr = getParamValueString(value);
					const String displayText = U"{} = {}"_fmt(paramName, valueStr);

					// 構造化束縛の変数を明示的にコピー
					const String paramNameCopy = paramName;
					menuElements.push_back(MenuItem{
						.text = displayText,
						.hotKeyText = U"",
						.mnemonicInput = none,
						.onClick = [this, paramNameCopy]() { selectParam(paramNameCopy); }
					});
				}
			}

			dialogContextMenu->show(m_comboBox->regionRect().bl(), menuElements);
		}

		void selectParam(const String& paramName)
		{
			m_selectedParamName = paramName;

			// コンボボックスの表示を更新
			const String displayText = paramName.isEmpty() ? U"(なし)" : paramName;
			m_comboLabel->setText(displayText);

			updateValueLabels();
		}

		void updateValueLabels()
		{
			if (m_selectedParamName.isEmpty())
			{
				m_valueLabel->setText(U"");
				m_appliedValueLabel->setText(U"");
				return;
			}
			const auto param = m_parentCanvas->paramValueOpt(m_selectedParamName);
			if (!param)
			{
				m_valueLabel->setText(U"");
				m_appliedValueLabel->setText(U"");
				return;
			}
			m_valueLabel->setText(getParamValueString(*param));

			// モード適用後の値を計算
			const ParamValue base = resolveBaseValue();
			if (auto applied = ApplyParamRefMode(base, *param, m_selectedMode, m_childParamType))
			{
				m_appliedValueLabel->setText(ParamValueToString(*applied));
			}
			else
			{
				m_appliedValueLabel->setText(U"(適用不可)");
			}
		}

		void onModeComboBoxClick(const std::shared_ptr<ContextMenu>& dialogContextMenu)
		{
			Array<MenuElement> menuElements;
			for (const auto mode : m_availableModes)
			{
				const ParamRefMode modeCopy = mode;
				menuElements.push_back(MenuItem{
					.text = String{ ParamRefModeToDisplayString(mode) },
					.hotKeyText = U"",
					.mnemonicInput = none,
					.onClick = [this, modeCopy]() { selectMode(modeCopy); }
				});
			}
			dialogContextMenu->show(m_modeComboBox->regionRect().bl(), menuElements);
		}

		void selectMode(ParamRefMode mode)
		{
			m_selectedMode = mode;
			m_modeComboLabel->setText(String{ ParamRefModeToDisplayString(mode) });
			updateValueLabels();
		}

		void onCreateNewParamButtonClick()
		{
			Array<ParamType> compatibleTypes = CompatibleParamTypes(m_childParamType);
			if (compatibleTypes.isEmpty())
			{
				return;
			}

			auto addParamDialog = std::make_shared<AddParamDialog>(
				m_parentCanvas,
				[this]()
				{
					filterAvailableParams();
				},
				std::move(compatibleTypes),
				m_childParamType,
				ParamValueToString(m_childParamDefaultValue),
				[this](const String& createdParamName)
				{
					// 作成されたパラメータを自動選択
					selectParam(createdParamName);
				},
				m_dialogOpener,
				m_childParamName);

			m_dialogOpener->openDialog(addParamDialog);
		}

		void onResult(StringView resultButtonText) override
		{
			if (resultButtonText != U"OK")
			{
				return;
			}

			// serializedParamBindingsJSONを更新
			JSON bindingsJSON;
			{
				const String s = m_subCanvas->serializedParamBindingsJSON();
				bindingsJSON = (s.isEmpty() || s == U"{}") ? JSON::Parse(U"{}") : JSON::Parse(s);
				if (!bindingsJSON.isObject())
				{
					bindingsJSON = JSON::Parse(U"{}");
				}
			}
			if (m_selectedParamName.isEmpty())
			{
				bindingsJSON.erase(m_childParamName);
			}
			else
			{
				bindingsJSON[m_childParamName] = m_selectedParamName;
			}
			m_subCanvas->setSerializedParamBindingsJSON(bindingsJSON.formatMinimum());

			// serializedParamBindingModesJSONを更新
			JSON modesJSON;
			{
				const String s = m_subCanvas->serializedParamBindingModesJSON();
				modesJSON = s.isEmpty() || s == U"{}" ? JSON::Parse(U"{}") : JSON::Parse(s);
				if (!modesJSON.isObject())
				{
					modesJSON = JSON::Parse(U"{}");
				}
			}
			if (m_selectedParamName.isEmpty())
			{
				modesJSON.erase(m_childParamName);
			}
			else
			{
				modesJSON[m_childParamName] = EnumToString(m_selectedMode);
			}
			m_subCanvas->setSerializedParamBindingModesJSON(modesJSON.formatMinimum());

			if (m_onComplete)
			{
				m_onComplete();
			}
		}
	};
}
