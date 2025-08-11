#pragma once
#include <NocoUI.hpp>
#include "EditorDialog.hpp"
#include "EnumPropertyComboBox.hpp"
#include "Vec2PropertyTextBox.hpp"
#include "ColorPropertyTextBox.hpp"
#include "LRTBPropertyTextBox.hpp"
#include "CheckboxToggler.hpp"
#include "TabStop.hpp"
#include "PropertyTextBox.hpp"

namespace noco::editor
{
	class AddParamDialog : public IDialog
	{
	private:
		std::shared_ptr<Canvas> m_canvas;
		std::function<void()> m_onComplete;
		
		// ダイアログ内のコントロール
		std::shared_ptr<TextBox> m_nameTextBox;
		std::shared_ptr<Node> m_typeComboBox;
		std::shared_ptr<Label> m_typeLabel;
		
		String m_selectedType = U"Number";
		std::variant<bool, double, String, ColorF, Vec2, LRTB> m_value = 0.0;
		
	public:
		explicit AddParamDialog(const std::shared_ptr<Canvas>& canvas, std::function<void()> onComplete)
			: m_canvas(canvas)
			, m_onComplete(std::move(onComplete))
		{
		}
		
		double dialogWidth() const override
		{
			return 400;
		}
		
		Array<DialogButtonDesc> buttonDescs() const override
		{
			return {
				DialogButtonDesc{ .text = U"作成", .mnemonicInput = KeyEnter, .isDefaultButton = IsDefaultButtonYN::Yes },
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
				U"新規パラメータ",
				U"",
				16,
				Palette::White,
				HorizontalAlign::Center,
				VerticalAlign::Middle);
			
			// パラメータ名入力
			const auto nameRow = contentRootNode->emplaceChild(
				U"NameRow",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 32 },
					.margin = LRTB{ 0, 0, 4, 4 },
				});
			nameRow->setChildrenLayout(HorizontalLayout{ .spacing = 8 });
			
			const auto nameLabelNode = nameRow->emplaceChild(
				U"NameLabel",
				InlineRegion
				{
					.sizeDelta = Vec2{ 80, 32 },
				});
			nameLabelNode->emplaceComponent<Label>(
				U"名前:",
				U"",
				14,
				Palette::White,
				HorizontalAlign::Right,
				VerticalAlign::Middle);
			
			const auto nameTextBoxNode = nameRow->emplaceChild(
				U"NameTextBox",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ -80, 26 },
				});
			nameTextBoxNode->emplaceComponent<RectRenderer>(ColorF{ 0.1, 0.8 }, ColorF{ 1.0, 0.4 }, 1.0, 4.0);
			m_nameTextBox = nameTextBoxNode->emplaceComponent<TextBox>(
				U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, 
				HorizontalAlign::Left, VerticalAlign::Middle, 
				Palette::White, ColorF{ Palette::Orange, 0.5 });
			m_nameTextBox->setText(U"param1");
			nameTextBoxNode->emplaceComponent<TabStop>();
			
			// 型選択
			const auto typeRow = contentRootNode->emplaceChild(
				U"TypeRow",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 32 },
					.margin = LRTB{ 0, 0, 4, 4 },
				});
			typeRow->setChildrenLayout(HorizontalLayout{ .spacing = 8 });
			
			const auto typeLabelNode = typeRow->emplaceChild(
				U"TypeLabel",
				InlineRegion
				{
					.sizeDelta = Vec2{ 80, 32 },
				});
			typeLabelNode->emplaceComponent<Label>(
				U"型:",
				U"",
				14,
				Palette::White,
				HorizontalAlign::Right,
				VerticalAlign::Middle);
			
			m_typeComboBox = typeRow->emplaceChild(
				U"TypeComboBox",
				InlineRegion
				{
					.sizeDelta = Vec2{ 120, 26 },
				});
			m_typeComboBox->emplaceComponent<RectRenderer>(
				PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withHovered(ColorF{ 0.15, 0.85 }),
				PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue),
				1.0, 4.0);
			m_typeLabel = m_typeComboBox->emplaceComponent<Label>(
				m_selectedType,
				U"",
				14,
				Palette::White,
				HorizontalAlign::Center,
				VerticalAlign::Middle);
			m_typeComboBox->emplaceComponent<UpdaterComponent>([this, dialogContextMenu](const std::shared_ptr<Node>& node) 
				{
					if (node->isClicked())
					{
						onTypeComboBoxClick(dialogContextMenu);
					}
				});
			
			// フォーカスを名前入力欄に設定
			CurrentFrame::SetFocusedNode(nameTextBoxNode);
		}
		
		void onTypeComboBoxClick(const std::shared_ptr<ContextMenu>& dialogContextMenu)
		{
			Array<MenuElement> menuElements;
			
			const Array<String> types = {
				U"Bool",
				U"Number",
				U"String",
				U"Color",
				U"Vec2",
				U"LRTB"
			};
			
			for (const auto& type : types)
			{
				menuElements.push_back(MenuItem{
					.text = type,
					.hotKeyText = U"",
					.mnemonicInput = none,
					.onClick = [this, type = type]() { selectType(type); }
				});
			}
			
			dialogContextMenu->show(m_typeComboBox->layoutAppliedRect().bl(), menuElements);
		}
		
		void selectType(const String& type)
		{
			m_selectedType = type;
			m_typeLabel->setText(type);
			
			// 型に応じてデフォルト値を設定
			if (type == U"Bool")
			{
				m_value = false;
			}
			else if (type == U"Number")
			{
				m_value = 0.0;
			}
			else if (type == U"String")
			{
				m_value = String{ U"" };
			}
			else if (type == U"Color")
			{
				m_value = ColorF{ 1.0, 1.0, 1.0, 1.0 };
			}
			else if (type == U"Vec2")
			{
				m_value = Vec2{ 0, 0 };
			}
			else if (type == U"LRTB")
			{
				m_value = LRTB{ 0, 0, 0, 0 };
			}
		}
		
		void onResult(StringView resultButtonText) override
		{
			if (resultButtonText == U"作成")
			{
				const String name = String(m_nameTextBox->text());
				if (name.isEmpty())
				{
					return;
				}
				
				if (m_canvas->getParam(name))
				{
					return;
				}
				
				std::shared_ptr<Param> newParam;
				if (m_selectedType == U"Bool")
				{
					newParam = std::make_shared<Param>(name, std::get<bool>(m_value));
				}
				else if (m_selectedType == U"Number")
				{
					newParam = std::make_shared<Param>(name, std::get<double>(m_value));
				}
				else if (m_selectedType == U"String")
				{
					newParam = std::make_shared<Param>(name, std::get<String>(m_value));
				}
				else if (m_selectedType == U"Color")
				{
					newParam = std::make_shared<Param>(name, std::get<ColorF>(m_value));
				}
				else if (m_selectedType == U"Vec2")
				{
					newParam = std::make_shared<Param>(name, std::get<Vec2>(m_value));
				}
				else if (m_selectedType == U"LRTB")
				{
					newParam = std::make_shared<Param>(name, std::get<LRTB>(m_value));
				}
				
				if (newParam)
				{
					m_canvas->setParam(newParam);
					if (m_onComplete)
					{
						m_onComplete();
					}
				}
			}
		}
	};
}