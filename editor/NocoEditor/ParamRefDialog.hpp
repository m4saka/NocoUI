#pragma once
#include <NocoUI.hpp>
#include "AddParamDialog.hpp"
#include "TabStop.hpp"

namespace noco::editor
{
	class ParamRefDialog : public IDialog
	{
	private:
		IProperty* m_pProperty;
		std::shared_ptr<Canvas> m_canvas;
		std::function<void()> m_onComplete;
		
		// ダイアログ内のコントロール
		std::shared_ptr<Node> m_comboBox;
		std::shared_ptr<Label> m_comboLabel;
		std::shared_ptr<Label> m_valueLabel;
		
		String m_selectedParamName;
		Array<std::pair<String, ParamValue>> m_availableParams;
		
		// ダイアログ管理
		std::shared_ptr<DialogOpener> m_dialogOpener;
		
		Optional<ParamType> getPropertyParamType() const
		{
			if (!m_pProperty)
			{
				return none;
			}
			return GetRequiredParamType(m_pProperty);
		}
		
		void filterAvailableParams()
		{
			m_availableParams.clear();
			
			const auto propertyType = getPropertyParamType();
			if (!propertyType)
			{
				return;
			}
			
			for (const auto& [name, value] : m_canvas->params())
			{
				if (GetParamType(value) == *propertyType)
				{
					m_availableParams.push_back({name, value});
				}
			}
			
			m_availableParams.sort_by([](const auto& a, const auto& b) { return a.first < b.first; });
		}
		
		String getParamValueString(const ParamValue& value) const
		{
			return ParamValueToString(value);
		}
		
	public:
		explicit ParamRefDialog(IProperty* pProperty, const std::shared_ptr<Canvas>& canvas, std::function<void()> onComplete, const std::shared_ptr<DialogOpener>& dialogOpener)
			: m_pProperty(pProperty)
			, m_canvas(canvas)
			, m_onComplete(std::move(onComplete))
			, m_selectedParamName(m_pProperty->paramRef())
			, m_dialogOpener(dialogOpener)
		{
			filterAvailableParams();
		}
		
		double dialogWidth() const override
		{
			return 400;
		}
		
		Array<DialogButtonDesc> buttonDescs() const override
		{
			return {
				DialogButtonDesc{ .text = U"OK", .mnemonicInput = KeyO, .appendsMnemonicKeyText = AppendsMnemonicKeyTextYN::No, .isDefaultButton = IsDefaultButtonYN::Yes },
				DialogButtonDesc{ .text = U"キャンセル", .mnemonicInput = KeyC, .isCancelButton = IsCancelButtonYN::Yes }
			};
		}
		
		void createDialogContent(const std::shared_ptr<Node>& contentRootNode, const std::shared_ptr<ContextMenu>& dialogContextMenu) override
		{
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
				U"参照パラメータを選択",
				U"",
				16,
				Palette::White,
				HorizontalAlign::Center,
				VerticalAlign::Middle);
			
			// プロパティ名とタイプ表示
			const auto propInfoNode = contentRootNode->emplaceChild(
				U"PropInfo",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 24 },
					.margin = LRTB{ 0, 0, 4, 8 },
				});
			const auto typeStr = getPropertyParamType() ? ParamTypeToString(*getPropertyParamType()) : U"Unknown";
			propInfoNode->emplaceComponent<Label>(
				U"プロパティ: {} ({}型)"_fmt(m_pProperty->name(), typeStr),
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
					.sizeDelta = Vec2{ 100, 32 },
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
				PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05),
				PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(ColorF{ 1.0, 0.6 }).withSmoothTime(0.05),
				1.0, 4.0);
			
			// 現在選択されているパラメータ名を表示
			String displayText = m_selectedParamName.isEmpty() ? U"(なし)" : m_selectedParamName;
			m_comboLabel = m_comboBox->emplaceComponent<Label>(
				displayText,
				U"",
				14,
				Palette::White,
				HorizontalAlign::Left,
				VerticalAlign::Middle,
				LRTB{ 8, 25, 0, 0 });
			
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
			const auto newParamButton = comboRow->emplaceChild(
				U"NewParamButton",
				InlineRegion
				{
					.sizeDelta = Vec2{ 90, 26 },
				});
			newParamButton->emplaceComponent<RectRenderer>(
				PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05),
				PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(ColorF{ 1.0, 0.6 }).withSmoothTime(0.05),
				1.0, 4.0);
			newParamButton->emplaceComponent<Label>(
				U"＋ 新規",
				U"",
				12,
				Palette::White,
				HorizontalAlign::Center,
				VerticalAlign::Middle);
			newParamButton->emplaceComponent<UpdaterComponent>([this](const std::shared_ptr<Node>& node)
			{
				if (node->isClicked())
				{
					onCreateNewParamButtonClick();
				}
			});
			
			// 値表示
			const auto valueRow = contentRootNode->emplaceChild(
				U"ValueRow",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 32 },
					.margin = LRTB{ 0, 0, 4, 8 },
				});
			valueRow->setChildrenLayout(HorizontalLayout{ .spacing = 8 });
			
			const auto valueLabelNode = valueRow->emplaceChild(
				U"ValueLabel",
				InlineRegion
				{
					.sizeDelta = Vec2{ 100, 32 },
				});
			valueLabelNode->emplaceComponent<Label>(
				U"現在の値:",
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
			valueDisplayNode->emplaceComponent<RectRenderer>(ColorF{ 0.05, 0.8 }, ColorF{ 0.5, 0.4 }, 1.0, 4.0);
			
			// 現在選択されているパラメータの値を表示
			String valueText = U"";
			if (!m_selectedParamName.isEmpty())
			{
				if (const auto param = m_canvas->param(m_selectedParamName))
				{
					valueText = getParamValueString(*param);
				}
			}
			m_valueLabel = valueDisplayNode->emplaceComponent<Label>(
				valueText,
				U"",
				14,
				ColorF{ 0.9, 0.9, 0.9 },
				HorizontalAlign::Center,
				VerticalAlign::Middle);
			
			// 利用可能なパラメータがない場合の警告
			if (m_availableParams.empty())
			{
				const auto warningNode = contentRootNode->emplaceChild(
					U"Warning",
					InlineRegion
					{
						.sizeRatio = Vec2{ 1, 0 },
						.sizeDelta = Vec2{ 0, 32 },
						.margin = LRTB{ 0, 0, 8, 8 },
					});
				warningNode->emplaceComponent<Label>(
					U"※ この型に対応するパラメータがありません",
					U"",
					14,
					ColorF{ 1.0, 0.7, 0.7 },
					HorizontalAlign::Center,
					VerticalAlign::Middle);
			}
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
					
					menuElements.push_back(MenuItem{
						.text = displayText,
						.hotKeyText = U"",
						.mnemonicInput = none,
						.onClick = [this, paramName]() { selectParam(paramName); }
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
			
			// 値表示を更新
			String valueText = U"";
			if (!paramName.isEmpty())
			{
				if (const auto param = m_canvas->param(paramName))
				{
					valueText = getParamValueString(*param);
				}
			}
			m_valueLabel->setText(valueText);
		}
		
		void onCreateNewParamButtonClick()
		{
			const auto propertyType = getPropertyParamType();
			if (!propertyType)
			{
				return;
			}
			
			auto addParamDialog = std::make_shared<AddParamDialog>(
				m_canvas,
				[this]()
				{
					filterAvailableParams();
				});
			
			m_dialogOpener->openDialog(addParamDialog);
		}
		
		void onResult(StringView resultButtonText) override
		{
			if (resultButtonText == U"OK")
			{
				SetPropertyParamRef(m_pProperty, m_selectedParamName);
				
				if (m_onComplete)
				{
					m_onComplete();
				}
			}
		}
	};
}
