#pragma once
#include <NocoUI.hpp>
#include "ContextMenu.hpp"
#include "EnumPropertyComboBox.hpp"
#include "Vec2PropertyTextBox.hpp"
#include "ColorPropertyTextBox.hpp"
#include "LRTBPropertyTextBox.hpp"
#include "CheckboxToggler.hpp"
#include "EditorButton.hpp"
#include "EditorColor.hpp"
#include "EditorYN.hpp"
#include "KeyInputBlocker.hpp"

namespace noco::editor
{
	struct DialogButtonDesc
	{
		String text = U""; // TODO: 現状は単一表示言語想定でダイアログ結果にテキストを流用しているが、将来的にはダイアログ毎の結果型として任意の型を指定可能にして、シンプルな用途のためにYes/No/Cancel用の関数を用意したい
		Optional<Input> mnemonicInput = none;
		AppendsMnemonicKeyTextYN appendsMnemonicKeyText = AppendsMnemonicKeyTextYN::Yes; // TODO: 現状は日本語想定でカッコで追加する形にしているが、将来的には「&File」「ファイル(&F)」など&を前につけるとニーモニック扱いされるようにしたい
		IsDefaultButtonYN isDefaultButton = IsDefaultButtonYN::No;
		IsCancelButtonYN isCancelButton = IsCancelButtonYN::No;
	};

	class IDialog
	{
	public:
		virtual ~IDialog() = default;

		virtual double dialogWidth() const = 0;

		virtual Array<DialogButtonDesc> buttonDescs() const = 0;

		virtual void createDialogContent(const std::shared_ptr<Node>& contentRootNode, const std::shared_ptr<ContextMenu>& dialogContextMenu, std::function<void()> fnRefreshLayoutForContent) = 0;

		virtual void onResult(StringView resultButtonText) = 0;
	};

	class DialogFrame
	{
	private:
		std::shared_ptr<Canvas> m_dialogCanvas;
		std::shared_ptr<Node> m_screenMaskNode;
		std::shared_ptr<Node> m_dialogNode;
		std::shared_ptr<Node> m_contentRootNode;
		std::shared_ptr<Node> m_buttonRootNode;
		std::function<void(StringView)> m_onResult;

	public:
		explicit DialogFrame(const std::shared_ptr<Canvas>& dialogCanvas, double dialogWidth, const std::function<void(StringView)>& onResult, const Array<DialogButtonDesc>& buttonDescs)
			: m_dialogCanvas(dialogCanvas)
			, m_screenMaskNode(dialogCanvas->emplaceChild(
				U"Dialog_ScreenMask",
				AnchorRegion
				{
					.anchorMin = Anchor::TopLeft,
					.anchorMax = Anchor::BottomRight,
					.posDelta = Vec2{ 0, 0 },
					.sizeDelta = Vec2{ 0, 0 },
					.sizeDeltaPivot = Anchor::TopLeft,
				}))
			, m_dialogNode(m_screenMaskNode->emplaceChild(
				U"Dialog",
				InlineRegion
				{
					.sizeRatio = Vec2{ 0, 0 },
					.sizeDelta = Vec2{ dialogWidth, 0 },
					.margin = LRTB{ 0, 0, 0, 0 },
				}))
			, m_contentRootNode(m_dialogNode->emplaceChild(
				U"Dialog_ContentRoot",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.margin = LRTB{ 0, 0, 0, 0 },
					.maxHeight = 600,
				}))
			, m_buttonRootNode(m_dialogNode->emplaceChild(
				U"Dialog_ButtonRoot",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.margin = LRTB{ 0, 0, 0, 0 },
				}))
			, m_onResult(onResult)
		{
			m_screenMaskNode->emplaceComponent<KeyInputBlocker>();

			// ダイアログ背面を暗くする
			m_screenMaskNode->emplaceComponent<RectRenderer>(ColorF{ 0.0, 0.25 });
			m_screenMaskNode->setChildrenLayout(FlowLayout{ .horizontalAlign = HorizontalAlign::Center, .verticalAlign = VerticalAlign::Middle });

			m_dialogNode->setChildrenLayout(VerticalLayout{ .padding = LRTB{ 8, 8, 8, 12 } });
			m_dialogNode->emplaceComponent<RectRenderer>(EditorColor::ControlBackgroundColor, ColorF{ 1.0, 0.3 }, 1.0, 0.0, 3.0, ColorF{ 0.0, 0.3 }, Vec2{ 2, 2 }, 8.0, 4.0);

			const auto buttonParentNode = m_dialogNode->emplaceChild(
				U"Dialog_ButtonParent",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.margin = LRTB{ 0, 0, 8, 0 },
				});
			buttonParentNode->setChildrenLayout(HorizontalLayout{ .padding = LRTB{ 0, 0, 0, 0 }, .horizontalAlign = HorizontalAlign::Center });
			for (const DialogButtonDesc& buttonDesc : buttonDescs)
			{
				const String buttonText = [](const DialogButtonDesc& buttonDesc) -> String
					{
						if (buttonDesc.mnemonicInput.has_value() && buttonDesc.appendsMnemonicKeyText)
						{
							return U"{}({})"_fmt(buttonDesc.text, buttonDesc.mnemonicInput->name());
						}
						else
						{
							return buttonDesc.text;
						}
					}(buttonDesc);

				const auto buttonNode = buttonParentNode->addChild(
					CreateButtonNode(
						buttonText,
						InlineRegion
						{
							.sizeDelta = Vec2{ 100, 24 },
							.margin = LRTB{ 4, 4, 0, 0 },
						},
						[this, buttonDesc](const std::shared_ptr<Node>&)
						{
							m_screenMaskNode->removeFromParent();
							if (m_onResult)
							{
								m_onResult(buttonDesc.text);
							}
						},
						buttonDesc.isDefaultButton));

				if (buttonDesc.mnemonicInput.has_value())
				{
					buttonNode->addClickHotKey(*buttonDesc.mnemonicInput);
				}

				if (buttonDesc.isDefaultButton)
				{
					buttonNode->addClickHotKey(KeyEnter, EnabledWhileTextEditingYN::Yes);
				}

				if (buttonDesc.isCancelButton)
				{
					buttonNode->addClickHotKey(KeyEscape, EnabledWhileTextEditingYN::Yes);
				}
			}
			buttonParentNode->setInlineRegionToFitToChildren(FitTarget::HeightOnly);
			m_dialogNode->setInlineRegionToFitToChildren(FitTarget::HeightOnly);

			// ダイアログの中身が大きすぎる場合用にスクロール可能にする
			m_contentRootNode->setVerticalScrollable(true);
			m_contentRootNode->setClippingEnabled(true);

			m_dialogCanvas->refreshLayoutImmediately();
		}

		virtual ~DialogFrame() = default;

		std::shared_ptr<Node> contentRootNode() const
		{
			return m_contentRootNode;
		}

		void refreshLayoutForContent()
		{
			m_contentRootNode->setInlineRegionToFitToChildren(FitTarget::HeightOnly);
			m_dialogNode->setInlineRegionToFitToChildren(FitTarget::HeightOnly);
			m_dialogCanvas->refreshLayoutImmediately();
		}
	};

	class DialogOpener
	{
	private:
		size_t m_nextDialogId = 1;
		std::shared_ptr<Canvas> m_dialogCanvas;
		std::shared_ptr<ContextMenu> m_dialogContextMenu;
		HashTable<size_t, std::shared_ptr<DialogFrame>> m_openedDialogFrames;

	public:
		explicit DialogOpener(const std::shared_ptr<Canvas>& dialogCanvas, const std::shared_ptr<ContextMenu>& dialogContextMenu)
			: m_dialogCanvas(dialogCanvas)
			, m_dialogContextMenu(dialogContextMenu)
		{
		}

		void openDialog(const std::shared_ptr<IDialog>& dialog)
		{
			auto dialogFrame = std::make_shared<DialogFrame>(m_dialogCanvas, dialog->dialogWidth(), [this, dialogId = m_nextDialogId, dialog](StringView resultButtonText) { dialog->onResult(resultButtonText); m_openedDialogFrames.erase(dialogId); }, dialog->buttonDescs());
			dialog->createDialogContent(dialogFrame->contentRootNode(), m_dialogContextMenu,
				[dialogFrameWeak = std::weak_ptr<DialogFrame>{ dialogFrame }]
				{
					if (const auto dialogFrame = dialogFrameWeak.lock())
					{
						dialogFrame->refreshLayoutForContent();
					}
				});
			dialogFrame->refreshLayoutForContent();
			m_openedDialogFrames.emplace(m_nextDialogId, dialogFrame);
			++m_nextDialogId;
		}

		bool anyDialogOpened() const
		{
			return !m_openedDialogFrames.empty();
		}

		void openDialogOK(const String& text, const std::function<void()>& onComplete = nullptr);

		void openDialogOKMultiple(const Array<String>& messages, const std::function<void()>& onComplete = nullptr);

	private:
		void openDialogOKMultipleRecursive(const Array<String>& messages, size_t index, const std::function<void()>& onComplete);

	public:
	};

	class SimpleDialog : public IDialog
	{
	private:
		String m_text;
		std::function<void(StringView)> m_onResult;
		Array<DialogButtonDesc> m_buttonDescs;

	public:
		SimpleDialog(StringView text, const std::function<void(StringView)>& onResult, const Array<DialogButtonDesc>& buttonDescs)
			: m_text(text)
			, m_onResult(onResult)
			, m_buttonDescs(buttonDescs)
		{
		}

		double dialogWidth() const override
		{
			return 400;
		}

		Array<DialogButtonDesc> buttonDescs() const override
		{
			return m_buttonDescs;
		}

		void createDialogContent(const std::shared_ptr<Node>& contentRootNode, const std::shared_ptr<ContextMenu>&, std::function<void()>) override
		{
			const auto labelNode = contentRootNode->emplaceChild(
				U"Label",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = SizeF{ 0, 48 },
					.margin = LRTB{ 0, 0, 16, 16 },
				});
			labelNode->emplaceComponent<Label>(
				m_text,
				U"",
				14,
				Palette::White,
				HorizontalAlign::Center,
				VerticalAlign::Middle);
		}

		void onResult(StringView resultButtonText) override
		{
			if (m_onResult)
			{
				m_onResult(resultButtonText);
			}
		}
	};

	class SimpleInputDialog : public IDialog
	{
	private:
		String m_labelText;
		String m_defaultValue;
		std::function<void(StringView, StringView)> m_onResult;
		Array<DialogButtonDesc> m_buttonDescs;
		std::shared_ptr<Node> m_textBoxNode;

	public:
		SimpleInputDialog(StringView labelText, StringView defaultValue, const std::function<void(StringView, StringView)>& onResult, const Array<DialogButtonDesc>& buttonDescs)
			: m_labelText(labelText)
			, m_defaultValue(defaultValue)
			, m_onResult(onResult)
			, m_buttonDescs(buttonDescs)
		{
		}

		double dialogWidth() const override
		{
			return 400;
		}

		Array<DialogButtonDesc> buttonDescs() const override
		{
			return m_buttonDescs;
		}

		void createDialogContent(const std::shared_ptr<Node>& contentRootNode, const std::shared_ptr<ContextMenu>&, std::function<void()>) override
		{
			const auto labelNode = contentRootNode->emplaceChild(
				U"Label",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = SizeF{ 0, 24 },
					.margin = LRTB{ 16, 16, 16, 8 },
				});
			labelNode->emplaceComponent<Label>(
				m_labelText,
				U"",
				14,
				Palette::White,
				HorizontalAlign::Left,
				VerticalAlign::Middle);

			m_textBoxNode = contentRootNode->emplaceChild(
				U"TextBox",
				InlineRegion
				{
					.sizeDelta = SizeF{ 0, 26 },
					.flexibleWeight = 1,
					.margin = LRTB{ 16, 16, 8, 16 },
				});
			m_textBoxNode->emplaceComponent<RectRenderer>(
				EditorColor::ControlBackgroundColorValue(),
				EditorColor::TextBoxBorderColorValue(),
				1.0,
				0.0,
				4.0);
			const auto textBox = m_textBoxNode->emplaceComponent<TextBox>(
				U"",
				14,
				Palette::White,
				Vec2{ 4, 4 },
				Vec2{ 2, 2 },
				HorizontalAlign::Left,
				VerticalAlign::Middle,
				Palette::White,
				EditorColor::TextSelectionColor);
			textBox->setText(m_defaultValue);
		
			// テキストボックスにフォーカスを設定
			CurrentFrame::SetFocusedNode(m_textBoxNode);
		}

		void onResult(StringView resultButtonText) override
		{
			if (m_onResult && m_textBoxNode)
			{
				const auto textBox = m_textBoxNode->getComponent<TextBox>();
				if (textBox)
				{
					m_onResult(resultButtonText, textBox->text());
				}
			}
		}
	};

	class InteractivePropertyValueDialog : public IDialog
	{
	private:
		IProperty* m_pProperty;
		Array<String> m_buttonTexts;
		std::function<void()> m_onChange;
		std::shared_ptr<DialogOpener> m_dialogOpener;
	
		// styleState選択用
		String m_currentStyleState;
		Array<String> m_availableStyleStates;
		std::shared_ptr<Node> m_styleStateComboBox;
		std::shared_ptr<Label> m_styleStateLabel;
		std::shared_ptr<Node> m_removeButton;
	
		// プロパティ値表示用ノード
		struct PropertyValueNodeInfo
		{
			std::shared_ptr<Node> propertyNode;
			std::shared_ptr<Node> propertyValueNode;
			std::shared_ptr<Node> checkboxNode;
			std::shared_ptr<String> currentValueString;
		};
		HashTable<InteractionState, PropertyValueNodeInfo> m_propertyValueNodes;

	public:
		InteractivePropertyValueDialog(IProperty* pProperty, std::function<void()> onChange, const std::shared_ptr<DialogOpener>& dialogOpener)
			: m_pProperty(pProperty)
			, m_onChange(std::move(onChange))
			, m_dialogOpener(dialogOpener)
		{
			if (!m_pProperty)
			{
				throw Error{ U"Property is nullptr" };
			}
			if (!m_pProperty->isInteractiveProperty())
			{
				throw Error{ U"Property is not interactive" };
			}
		
			// 既存のstyleStateを収集
			collectExistingStyleStates();
		}

		double dialogWidth() const override
		{
			return m_pProperty->editType() == PropertyEditType::LRTB ? 640 : 500;
		}

		Array<DialogButtonDesc> buttonDescs() const override
		{
			return Array<DialogButtonDesc>
			{
				DialogButtonDesc
				{
					.text = U"OK",
					.isDefaultButton = IsDefaultButtonYN::Yes,
				}
			};
		}

		void createStyleStateSection(const std::shared_ptr<Node>& parentNode, const std::shared_ptr<ContextMenu>& dialogContextMenu)
		{
			const auto styleStateNode = parentNode->emplaceChild(
				U"StyleStateSection",
				InlineRegion{
					.sizeRatio = Vec2{1, 0},
					.sizeDelta = SizeF{0, 36},
					.margin = LRTB{0, 0, 0, 8},
				});
			styleStateNode->setChildrenLayout(HorizontalLayout{.spacing = 4});
		
			// ラベル
			const auto labelNode = styleStateNode->emplaceChild(
				U"Label",
				InlineRegion{
					.sizeRatio = Vec2{0, 1},
					.sizeDelta = Vec2{80, 0},
				});
			labelNode->emplaceComponent<Label>(
				U"styleState:",
				U"",
				14,
				Palette::White,
				HorizontalAlign::Left,
				VerticalAlign::Middle,
				LRTB{ 0, 0, 0, 0 });
		
			// コンボボックス
			m_styleStateComboBox = styleStateNode->emplaceChild(
				U"ComboBox",
				InlineRegion{
					.sizeDelta = Vec2{0, 26},
					.flexibleWeight = 1,
				});
			m_styleStateComboBox->emplaceComponent<RectRenderer>(
				EditorColor::ControlBackgroundColorValue(),
				EditorColor::ButtonBorderColorValue(),
				1.0,
				0.0,
				4.0);
		
			m_styleStateLabel = m_styleStateComboBox->emplaceComponent<Label>(
				U"(styleStateなし)",
				U"",
				14,
				Palette::White,
				HorizontalAlign::Left,
				VerticalAlign::Middle,
				LRTB{ 3, 18, 3, 3 })
				->setSizingMode(LabelSizingMode::AutoShrink);
		
			// ▼アイコン
			m_styleStateComboBox->emplaceComponent<Label>(
				U"▼",
				U"",
				10,
				Palette::White,
				HorizontalAlign::Right,
				VerticalAlign::Middle,
				LRTB{ 5, 7, 5, 5 });
		
			// コンボボックスのクリックイベント
			m_styleStateComboBox->addOnClick([this, dialogContextMenu](const std::shared_ptr<Node>&) { onStyleStateComboBoxClick(dialogContextMenu); });
		
			// ＋追加ボタン
			const auto addButton = styleStateNode->addChild(CreateButtonNode(
				U"＋ 追加",
				InlineRegion{
					.sizeDelta = Vec2{60, 26},
				},
				[this](const std::shared_ptr<Node>&) { onAddStyleState(); },
				IsDefaultButtonYN::No,
				12));
		
			// －削除ボタン
			m_removeButton = styleStateNode->addChild(CreateButtonNode(
				U"－ 削除",
				InlineRegion{
					.sizeDelta = Vec2{60, 26},
				},
				[this](const std::shared_ptr<Node>&) { onRemoveStyleState(); },
				IsDefaultButtonYN::No,
				12));
			m_removeButton->setInteractable(false); // 初期状態では無効
		
			// 区切り線
			const auto separatorNode = parentNode->emplaceChild(
				U"Separator",
				InlineRegion{
					.sizeRatio = Vec2{1, 0},
					.sizeDelta = SizeF{0, 1},
					.margin = LRTB{0, 0, 0, 8},
				});
			separatorNode->emplaceComponent<RectRenderer>(ColorF{ 1.0, 0.3 });
		}
	
		void onStyleStateComboBoxClick(const std::shared_ptr<ContextMenu>& dialogContextMenu)
		{
			Array<MenuElement> menuElements;

			menuElements.push_back(MenuItem{
				.text = U"(styleStateなし)",
				.hotKeyText = U"",
				.mnemonicInput = none,
				.onClick = [this]() { selectStyleState(U""); }
			});
		
			// 既に設定されているstyleStateを選択肢として追加
			for (const auto& state : m_availableStyleStates)
			{
				menuElements.push_back(MenuItem{
					.text = state,
					.hotKeyText = U"",
					.mnemonicInput = none,
					.onClick = [this, state = state]() { selectStyleState(state); }
				});
			}
		
			// コンテキストメニュー表示
			dialogContextMenu->show(m_styleStateComboBox->regionRect().bl(), menuElements);
		}
	
		void selectStyleState(const String& styleState)
		{
			m_currentStyleState = styleState;
			updateStyleStateUI();
			refreshPropertyValues();
		}
	
		void updateStyleStateUI()
		{
			if (m_currentStyleState.isEmpty())
			{
				m_styleStateLabel->setText(U"(styleStateなし)");
				m_removeButton->setInteractable(false);
			}
			else
			{
				m_styleStateLabel->setText(m_currentStyleState);
				m_removeButton->setInteractable(true);
			}
		}
	
		void refreshPropertyValues()
		{
			// 現在のstyleStateに基づいてactiveStyleStatesを構築
			Array<String> activeStyleStates;
			if (!m_currentStyleState.isEmpty())
			{
				activeStyleStates.push_back(m_currentStyleState);
			}

			// 各InteractionStateの値を更新
			for (const auto& [interactionState, nodeInfo] : m_propertyValueNodes)
			{
				// 現在の値を取得
				const String currentValue = m_pProperty->propertyValueStringOfFallback(interactionState, activeStyleStates);
				*nodeInfo.currentValueString = currentValue;

				// UIを更新
				updatePropertyValueNode(interactionState, nodeInfo, currentValue);

				// チェックボックスの状態を更新
				const bool hasValue = m_pProperty->hasPropertyValueOf(interactionState, m_currentStyleState);
				if (const auto toggler = nodeInfo.checkboxNode->getComponent<CheckboxToggler>())
				{
					toggler->setValue(hasValue);
				}

				// Defaultは常に値が存在するのでチェックボックスは無効
				nodeInfo.checkboxNode->setInteractable(interactionState != InteractionState::Default);
			}
		}
	
		void updatePropertyValueNode(InteractionState interactionState, const PropertyValueNodeInfo& nodeInfo, const String& value)
		{
			// 各プロパティタイプに応じて表示を更新
			switch (m_pProperty->editType())
			{
			case PropertyEditType::Number:
			case PropertyEditType::Text:
				if (const auto textBox = nodeInfo.propertyValueNode->getComponent<TextBox>(RecursiveYN::Yes))
				{
					textBox->setText(value);
				}
				else if (const auto textArea = nodeInfo.propertyValueNode->getComponent<TextArea>(RecursiveYN::Yes))
				{
					textArea->setText(value);
				}
				else
				{
					Logger << U"[NocoEditor warning] TextBox or TextArea not found";
				}
				break;
			case PropertyEditType::Bool:
				if (const auto checkboxToggler = nodeInfo.propertyValueNode->getComponent<CheckboxToggler>(RecursiveYN::Yes))
				{
					checkboxToggler->setValue(StringToValueOpt<bool>(value).value_or(false));
				}
				else
				{
					Logger << U"[NocoEditor warning] CheckboxToggler not found";
				}
				break;
			case PropertyEditType::Vec2:
				if (const auto vec2PropertyTextBox = nodeInfo.propertyValueNode->getComponent<Vec2PropertyTextBox>(RecursiveYN::Yes))
				{
					vec2PropertyTextBox->setValue(StringToValueOpt<Vec2>(value).value_or(Vec2::Zero()));
				}
				else
				{
					Logger << U"[NocoEditor warning] Vec2PropertyTextBox not found";
				}
				break;
			case PropertyEditType::Color:
				if (const auto colorPropertyTextBox = nodeInfo.propertyValueNode->getComponent<ColorPropertyTextBox>(RecursiveYN::Yes))
				{
					colorPropertyTextBox->setValue(StringToValueOpt<Color>(value).value_or(Color{}));
				}
				else
				{
					Logger << U"[NocoEditor warning] ColorPropertyTextBox not found";
				}
				break;
			case PropertyEditType::LRTB:
				if (const auto lrtbPropertyTextBox = nodeInfo.propertyValueNode->getComponent<LRTBPropertyTextBox>(RecursiveYN::Yes))
				{
					lrtbPropertyTextBox->setValue(StringToValueOpt<LRTB>(value).value_or(LRTB::Zero()));
				}
				else
				{
					Logger << U"[NocoEditor warning] LRTBPropertyTextBox not found";
				}
				break;
			case PropertyEditType::Enum:
				if (const auto comboBox = nodeInfo.propertyValueNode->getComponent<EnumPropertyComboBox>(RecursiveYN::Yes))
				{
					comboBox->setValue(value);
				}
				else
				{
					Logger << U"[NocoEditor warning] EnumPropertyComboBox not found";
				}
				break;
			}
		
			// プロパティ値ノードの有効/無効を設定
			nodeInfo.propertyValueNode->setInteractable(m_pProperty->hasPropertyValueOf(interactionState, m_currentStyleState));
		}
	
		void onAddStyleState()
		{
			if (m_dialogOpener)
			{
				m_dialogOpener->openDialog(
					std::make_unique<SimpleInputDialog>(
						U"styleStateを入力",
						U"",
						[this](StringView buttonText, StringView inputValue)
						{
							if (buttonText == U"OK" && !inputValue.isEmpty())
							{
								// 重複チェック
								String newState = String{ inputValue };
								if (!m_availableStyleStates.contains(newState))
								{
									// 現在のstyleStateから値をコピー
									Array<String> currentActiveStyleStates;
									if (!m_currentStyleState.isEmpty())
									{
										currentActiveStyleStates.push_back(m_currentStyleState);
									}
								
									// 各InteractionStateの値をコピー
									for (const auto interactionState : { InteractionState::Default, InteractionState::Hovered, InteractionState::Pressed, InteractionState::Disabled })
									{
										if (m_pProperty->hasPropertyValueOf(interactionState, m_currentStyleState))
										{
											const String value = m_pProperty->propertyValueStringOfFallback(interactionState, currentActiveStyleStates);
											m_pProperty->trySetPropertyValueStringOf(value, interactionState, newState);
										}
									}
								
									m_availableStyleStates.push_back(newState);
									selectStyleState(newState);
								}
							}
						},
						Array<DialogButtonDesc>{
							DialogButtonDesc{
								.text = U"OK",
								.isDefaultButton = IsDefaultButtonYN::Yes
							},
							DialogButtonDesc{
								.text = U"キャンセル",
								.isCancelButton = IsCancelButtonYN::Yes
							}
						}
					)
				);
			}
		}
	
		void onRemoveStyleState()
		{
			if (m_currentStyleState == U"")
			{
				// styleStateなしは削除不可
				return;
			}
		
			// 確認ダイアログを表示
			String styleStateToRemove = m_currentStyleState;
			m_dialogOpener->openDialog(std::make_unique<SimpleDialog>(
				U"styleState「{}」を削除しますか？"_fmt(styleStateToRemove),
				[this, styleStateToRemove](StringView resultButtonText)
				{
					if (resultButtonText == U"削除")
					{
						// 削除処理
						removeStyleStateFromAll(styleStateToRemove);
					
						// styleStateなしに戻す
						m_currentStyleState = U"";
					
						// availableStyleStatesから削除
						m_availableStyleStates.remove_if([&](const String& state) { return state == styleStateToRemove; });
					
						updateStyleStateUI();
					
						// プロパティ値を更新
						refreshPropertyValues();
					}
				},
				Array<DialogButtonDesc>
				{
					DialogButtonDesc
					{
						.text = U"キャンセル",
						.isCancelButton = IsCancelButtonYN::Yes
					},
					DialogButtonDesc
					{
						.text = U"削除",
						.isDefaultButton = IsDefaultButtonYN::Yes
					}
				}));
		}

		void removeStyleStateFromAll(const String& styleStateToRemove)
		{
			// プロパティ値から指定されたstyleStateを削除
			if (m_pProperty && m_pProperty->isInteractiveProperty())
			{
				// 指定されたstyleStateのすべてのInteractionStateを削除
				for (auto interactionState : { InteractionState::Hovered, InteractionState::Pressed, InteractionState::Disabled })
				{
					m_pProperty->unsetPropertyValueOf(interactionState, styleStateToRemove);
				}
			}
		}

		void collectExistingStyleStates()
		{
			m_availableStyleStates.clear();
		
			if (!m_pProperty || !m_pProperty->isInteractiveProperty())
			{
				return;
			}
		
			m_availableStyleStates = m_pProperty->styleStateKeys();
		}

		void createDialogContent(const std::shared_ptr<Node>& contentRootNode, const std::shared_ptr<ContextMenu>& dialogContextMenu, std::function<void()> fnRefreshLayoutForContent) override;

		void onResult(StringView) override
		{
			if (m_onChange)
			{
				m_onChange();
			}
		}
	};

	inline void DialogOpener::openDialogOK(const String& text, const std::function<void()>& onComplete)
	{
		openDialog(std::make_shared<SimpleDialog>(
			text,
			[onComplete](StringView) { if (onComplete) onComplete(); },
			Array<DialogButtonDesc>
			{
				DialogButtonDesc
				{
					.text = U"OK",
					.mnemonicInput = KeyO,
					.appendsMnemonicKeyText = AppendsMnemonicKeyTextYN::No,
					.isDefaultButton = IsDefaultButtonYN::Yes,
				},
			}));
	}

	inline void DialogOpener::openDialogOKMultiple(const Array<String>& messages, const std::function<void()>& onComplete)
	{
		if (messages.empty())
		{
			if (onComplete)
			{
				onComplete();
			}
			return;
		}

		openDialogOKMultipleRecursive(messages, 0, onComplete);
	}

	inline void DialogOpener::openDialogOKMultipleRecursive(const Array<String>& messages, size_t index, const std::function<void()>& onComplete)
	{
		if (index >= messages.size())
		{
			if (onComplete)
			{
				onComplete();
			}
			return;
		}

		openDialogOK(messages[index], [this, messages, index, onComplete]()
		{
			openDialogOKMultipleRecursive(messages, index + 1, onComplete);
		});
	}
}
