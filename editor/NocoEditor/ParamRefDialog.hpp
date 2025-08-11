#pragma once
#include <NocoUI.hpp>
#include "EditorDialog.hpp"
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
		Array<std::shared_ptr<Param>> m_availableParams;
		
		Optional<ParamType> getPropertyParamType() const
		{
			const String propName = String(m_pProperty->name());
			
			if (dynamic_cast<Property<bool>*>(m_pProperty))
			{
				return ParamType::Bool;
			}
			else if (dynamic_cast<Property<double>*>(m_pProperty) ||
					 dynamic_cast<SmoothProperty<double>*>(m_pProperty) ||
					 dynamic_cast<Property<int32>*>(m_pProperty) ||
					 dynamic_cast<Property<uint32>*>(m_pProperty))
			{
				return ParamType::Number;
			}
			else if (dynamic_cast<PropertyNonInteractive<String>*>(m_pProperty))
			{
				return ParamType::String;
			}
			else if (dynamic_cast<Property<ColorF>*>(m_pProperty) ||
					 dynamic_cast<SmoothProperty<ColorF>*>(m_pProperty))
			{
				return ParamType::Color;
			}
			else if (dynamic_cast<Property<Vec2>*>(m_pProperty) ||
					 dynamic_cast<SmoothProperty<Vec2>*>(m_pProperty))
			{
				return ParamType::Vec2;
			}
			else if (dynamic_cast<Property<LRTB>*>(m_pProperty) ||
					 dynamic_cast<SmoothProperty<LRTB>*>(m_pProperty))
			{
				return ParamType::LRTB;
			}
			
			return none;
		}
		
		void filterAvailableParams()
		{
			m_availableParams.clear();
			
			const auto propertyType = getPropertyParamType();
			if (!propertyType)
			{
				return;
			}
			
			for (const auto& [name, param] : m_canvas->params())
			{
				if (param && param->type() == *propertyType)
				{
					m_availableParams.push_back(param);
				}
			}
			
			m_availableParams.sort_by([](const auto& a, const auto& b) { return a->name() < b->name(); });
		}
		
		String getParamValueString(const std::shared_ptr<Param>& param) const
		{
			if (!param)
			{
				return U"";
			}
			
			switch (param->type())
			{
			case ParamType::Bool:
				return ValueToString(param->valueAs<bool>());
			case ParamType::Number:
				return ValueToString(param->valueAs<double>());
			case ParamType::String:
				return param->valueAs<String>();
			case ParamType::Color:
				return ValueToString(param->valueAs<ColorF>());
			case ParamType::Vec2:
				return ValueToString(param->valueAs<Vec2>());
			case ParamType::LRTB:
				return ValueToString(param->valueAs<LRTB>());
			default:
				return U"";
			}
		}
		
	public:
		explicit ParamRefDialog(IProperty* pProperty, const std::shared_ptr<Canvas>& canvas, std::function<void()> onComplete)
			: m_pProperty(pProperty)
			, m_canvas(canvas)
			, m_onComplete(std::move(onComplete))
		{
			filterAvailableParams();
			
			// 現在のparamRefを取得
			if (auto* pProp = dynamic_cast<Property<bool>*>(m_pProperty))
			{
				m_selectedParamName = pProp->paramRef();
			}
			else if (auto* pProp = dynamic_cast<Property<double>*>(m_pProperty))
			{
				m_selectedParamName = pProp->paramRef();
			}
			else if (auto* pProp = dynamic_cast<SmoothProperty<double>*>(m_pProperty))
			{
				m_selectedParamName = pProp->paramRef();
			}
			else if (auto* pProp = dynamic_cast<PropertyNonInteractive<String>*>(m_pProperty))
			{
				m_selectedParamName = pProp->paramRef();
			}
			else if (auto* pProp = dynamic_cast<Property<ColorF>*>(m_pProperty))
			{
				m_selectedParamName = pProp->paramRef();
			}
			else if (auto* pProp = dynamic_cast<SmoothProperty<ColorF>*>(m_pProperty))
			{
				m_selectedParamName = pProp->paramRef();
			}
			else if (auto* pProp = dynamic_cast<Property<Vec2>*>(m_pProperty))
			{
				m_selectedParamName = pProp->paramRef();
			}
			else if (auto* pProp = dynamic_cast<SmoothProperty<Vec2>*>(m_pProperty))
			{
				m_selectedParamName = pProp->paramRef();
			}
			else if (auto* pProp = dynamic_cast<Property<LRTB>*>(m_pProperty))
			{
				m_selectedParamName = pProp->paramRef();
			}
			else if (auto* pProp = dynamic_cast<SmoothProperty<LRTB>*>(m_pProperty))
			{
				m_selectedParamName = pProp->paramRef();
			}
		}
		
		double dialogWidth() const override
		{
			return 400;
		}
		
		Array<DialogButtonDesc> buttonDescs() const override
		{
			return {
				DialogButtonDesc{ .text = U"OK", .mnemonicInput = KeyEnter, .isDefaultButton = IsDefaultButtonYN::Yes },
				DialogButtonDesc{ .text = U"キャンセル", .mnemonicInput = KeyEscape, .isCancelButton = IsCancelButtonYN::Yes }
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
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ -100, 26 },
				});
			m_comboBox->emplaceComponent<RectRenderer>(
				PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withHovered(ColorF{ 0.15, 0.85 }),
				PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue),
				1.0, 4.0);
			
			// 現在選択されているパラメータ名を表示
			String displayText = m_selectedParamName.isEmpty() ? U"(なし)" : m_selectedParamName;
			m_comboLabel = m_comboBox->emplaceComponent<Label>(
				displayText,
				U"",
				14,
				Palette::White,
				HorizontalAlign::Center,
				VerticalAlign::Middle);
			
			m_comboBox->emplaceComponent<UpdaterComponent>([this, dialogContextMenu](const std::shared_ptr<Node>& node)
			{
				if (node->isClicked())
				{
					onComboBoxClick(dialogContextMenu);
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
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ -100, 26 },
				});
			valueDisplayNode->emplaceComponent<RectRenderer>(ColorF{ 0.15, 0.8 }, ColorF{ 0.4, 0.4 }, 1.0, 4.0);
			
			// 現在選択されているパラメータの値を表示
			String valueText = U"";
			if (!m_selectedParamName.isEmpty())
			{
				if (const auto param = m_canvas->getParam(m_selectedParamName))
				{
					valueText = getParamValueString(param);
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
				for (const auto& param : m_availableParams)
				{
					const String valueStr = getParamValueString(param);
					const String displayText = U"{} = {}"_fmt(param->name(), valueStr);
					
					menuElements.push_back(MenuItem{
						.text = displayText,
						.hotKeyText = U"",
						.mnemonicInput = none,
						.onClick = [this, name = param->name()]() { selectParam(name); }
					});
				}
			}
			
			dialogContextMenu->show(m_comboBox->layoutAppliedRect().bl(), menuElements);
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
				if (const auto param = m_canvas->getParam(paramName))
				{
					valueText = getParamValueString(param);
				}
			}
			m_valueLabel->setText(valueText);
		}
		
		void onResult(StringView resultButtonText) override
		{
			if (resultButtonText == U"OK")
			{
				// プロパティにparamRefを設定
				if (auto* pProp = dynamic_cast<Property<bool>*>(m_pProperty))
				{
					pProp->setParamRef(m_selectedParamName);
				}
				else if (auto* pProp = dynamic_cast<Property<double>*>(m_pProperty))
				{
					pProp->setParamRef(m_selectedParamName);
				}
				else if (auto* pProp = dynamic_cast<SmoothProperty<double>*>(m_pProperty))
				{
					pProp->setParamRef(m_selectedParamName);
				}
				else if (auto* pProp = dynamic_cast<PropertyNonInteractive<String>*>(m_pProperty))
				{
					pProp->setParamRef(m_selectedParamName);
				}
				else if (auto* pProp = dynamic_cast<Property<ColorF>*>(m_pProperty))
				{
					pProp->setParamRef(m_selectedParamName);
				}
				else if (auto* pProp = dynamic_cast<SmoothProperty<ColorF>*>(m_pProperty))
				{
					pProp->setParamRef(m_selectedParamName);
				}
				else if (auto* pProp = dynamic_cast<Property<Vec2>*>(m_pProperty))
				{
					pProp->setParamRef(m_selectedParamName);
				}
				else if (auto* pProp = dynamic_cast<SmoothProperty<Vec2>*>(m_pProperty))
				{
					pProp->setParamRef(m_selectedParamName);
				}
				else if (auto* pProp = dynamic_cast<Property<LRTB>*>(m_pProperty))
				{
					pProp->setParamRef(m_selectedParamName);
				}
				else if (auto* pProp = dynamic_cast<SmoothProperty<LRTB>*>(m_pProperty))
				{
					pProp->setParamRef(m_selectedParamName);
				}
				
				if (m_onComplete)
				{
					m_onComplete();
				}
			}
		}
	};
}