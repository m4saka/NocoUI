#include "TextureFontLabelCellTrimEditDialog.hpp"
#include "Inspector.hpp"

namespace noco::editor
{
	CharTrimInputDialog::CharTrimInputDialog(
		const LRTB& initialTrim,
		std::function<void(char32, const LRTB&)> onComplete)
		: m_onComplete(std::move(onComplete))
		, m_initialChar(none)
		, m_initialTrim(initialTrim)
		, m_isEditMode(false)
	{
	}

	CharTrimInputDialog::CharTrimInputDialog(
		char32 character,
		const LRTB& initialTrim,
		std::function<void(char32, const LRTB&)> onComplete)
		: m_onComplete(std::move(onComplete))
		, m_initialChar(character)
		, m_initialTrim(initialTrim)
		, m_isEditMode(true)
	{
	}

	Array<DialogButtonDesc> CharTrimInputDialog::buttonDescs() const
	{
		return {
			DialogButtonDesc{ .text = U"OK", .isDefaultButton = IsDefaultButtonYN::Yes },
			DialogButtonDesc{ .text = U"キャンセル", .mnemonicInput = KeyC, .isCancelButton = IsCancelButtonYN::Yes }
		};
	}

	void CharTrimInputDialog::createDialogContent(
		const std::shared_ptr<Node>& contentRootNode,
		const std::shared_ptr<ContextMenu>&,
		std::function<void()>)
	{
		contentRootNode->setChildrenLayout(VerticalLayout{ .padding = LRTB{ 16 } });

		// タイトル
		const auto titleNode = contentRootNode->emplaceChild(
			U"Title",
			InlineRegion{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = Vec2{ 0, 0 },
				.margin = LRTB{ 0, 0, 8, 16 },
			});
		const auto titleLabel = titleNode->emplaceComponent<Label>(
			m_isEditMode ? U"トリミング編集" : U"トリミング追加",
			U"", 16, Palette::White,
			HorizontalAlign::Center, VerticalAlign::Middle)
			->setSizingMode(LabelSizingMode::AutoResizeHeight);
		titleLabel->refreshAutoResizeImmediately(titleNode);

		// 追加する文字
		String initialCharText = m_initialChar.has_value() ? String{ *m_initialChar } : U"";
		const auto charNode = contentRootNode->addChild(
			Inspector::CreatePropertyNode(
				U"追加する文字",
				initialCharText,
				[](StringView) {})); // UpdaterCallbackで処理するためコールバックは空

		// TextBoxノードとコンポーネントを取得
		const auto textBoxNode = charNode->findByName(U"TextBox", RecursiveYN::Yes);
		if (textBoxNode)
		{
			m_charTextBox = textBoxNode->getComponent<TextBox>();

			// 毎フレーム1文字に制限
			textBoxNode->emplaceComponent<UpdaterComponent>(
				[textBoxWeak = std::weak_ptr<TextBox>(m_charTextBox)](const std::shared_ptr<Node>&)
				{
					if (const auto textBox = textBoxWeak.lock())
					{
						const String currentText = textBox->text();
						if (currentText.length() > 1)
						{
							// 1文字に制限
							textBox->setText(String{ currentText[0] });
						}
					}
				});

			if (m_isEditMode)
			{
				// 編集モードでは文字変更不可
				textBoxNode->setInteractable(false);
			}
		}

		// トリミング量
		const auto lrtbNode = contentRootNode->addChild(
			Inspector::CreateLRTBPropertyNode(
				U"トリミング量",
				m_initialTrim,
				[this](const LRTB& value) {
					m_initialTrim = value;
				}));

		m_lrtbPropertyTextBox = lrtbNode->getComponent<LRTBPropertyTextBox>(RecursiveYN::Yes);

		TabStop::LinkAllTabStops(contentRootNode, true);

		if (!m_isEditMode && textBoxNode)
		{
			CurrentFrame::SetFocusedNode(textBoxNode);
		}
	}

	void CharTrimInputDialog::onResult(StringView resultButtonText)
	{
		if (resultButtonText == U"OK" && m_onComplete)
		{
			const String charText = m_charTextBox->text();
			if (charText.isEmpty())
			{
				return;
			}

			const char32 character = charText[0];
			const LRTB trim = m_lrtbPropertyTextBox ? m_lrtbPropertyTextBox->value() : m_initialTrim;

			m_onComplete(character, trim);
		}
	}

	TextureFontLabelCellTrimEditDialog::TextureFontLabelCellTrimEditDialog(
		const std::shared_ptr<TextureFontLabel>& textureFontLabel,
		std::function<void()> onComplete,
		const std::shared_ptr<DialogOpener>& dialogOpener)
		: m_textureFontLabel(textureFontLabel)
		, m_onComplete(std::move(onComplete))
		, m_dialogOpener(dialogOpener)
	{
		parseCurrentJSON();
	}

	Array<DialogButtonDesc> TextureFontLabelCellTrimEditDialog::buttonDescs() const
	{
		return {
			DialogButtonDesc{ .text = U"OK", .isDefaultButton = IsDefaultButtonYN::Yes },
			DialogButtonDesc{ .text = U"キャンセル", .mnemonicInput = KeyC, .isCancelButton = IsCancelButtonYN::Yes }
		};
	}

	void TextureFontLabelCellTrimEditDialog::createDialogContent(
		const std::shared_ptr<Node>& contentRootNode,
		const std::shared_ptr<ContextMenu>&,
		std::function<void()>)
	{
		contentRootNode->setChildrenLayout(VerticalLayout{ .padding = LRTB{ 16 } });

		// タイトル
		const auto titleNode = contentRootNode->emplaceChild(
			U"Title",
			InlineRegion{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = Vec2{ 0, 0 },
				.margin = LRTB{ 0, 0, 8, 4 },
			});
		const auto titleLabel = titleNode->emplaceComponent<Label>(
			U"文字毎のトリミング設定",
			U"", 16, Palette::White,
			HorizontalAlign::Center, VerticalAlign::Middle)
			->setSizingMode(LabelSizingMode::AutoResizeHeight);
		titleLabel->refreshAutoResizeImmediately(titleNode);

		// 説明文
		const auto descNode = contentRootNode->emplaceChild(
			U"Description",
			InlineRegion{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = Vec2{ 0, 0 },
				.margin = LRTB{ 0, 0, 4, 12 },
			});
		const auto descLabel = descNode->emplaceComponent<Label>(
			U"特定の文字に対するセルのトリミング量を個別に指定できます。",
			U"", 12, ColorF{ 0.7 },
			HorizontalAlign::Center, VerticalAlign::Middle)
			->setSizingMode(LabelSizingMode::AutoResizeHeight);
		descLabel->refreshAutoResizeImmediately(descNode);

		// リスト外枠
		const auto listContainer = contentRootNode->emplaceChild(
			U"ListContainer",
			InlineRegion{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = Vec2{ 0, 250 },
				.margin = LRTB{ 0, 0, 0, 8 },
			});
		listContainer->emplaceComponent<RectRenderer>(
			ColorF{ 0.1, 0.15 },
			ColorF{ 0.4, 0.5 },
			1.0, 0.0, 4.0);

		// リスト
		m_listNode = listContainer->emplaceChild(
			U"CharTrimList",
			InlineRegion{
				.sizeRatio = Vec2{ 1, 1 },
				.margin = LRTB{ 4, 4, 4, 4 },
			});
		m_listNode->setChildrenLayout(VerticalLayout{ .spacing = 4 });

		// リスト更新
		refreshList();

		// 追加ボタン
		const auto addButtonContainer = contentRootNode->emplaceChild(
			U"AddButtonContainer",
			InlineRegion{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = Vec2{ 0, 26 },
				.margin = LRTB{ 0, 0, 4, 20 },
			});
		addButtonContainer->setChildrenLayout(HorizontalLayout{
			.horizontalAlign = HorizontalAlign::Center,
		});
		const auto addButton = addButtonContainer->addChild(CreateButtonNode(
			U"＋ 追加",
			InlineRegion{
				.sizeDelta = Vec2{ 100, 26 },
			},
			[this](const std::shared_ptr<Node>&) { onAddCharTrim(); },
			IsDefaultButtonYN::No,
			12));

		TabStop::LinkAllTabStops(contentRootNode, true);
	}

	void TextureFontLabelCellTrimEditDialog::onResult(StringView resultButtonText)
	{
		if (resultButtonText == U"OK")
		{
			saveToComponent();

			if (m_onComplete)
			{
				m_onComplete();
			}
		}
	}

	void TextureFontLabelCellTrimEditDialog::parseCurrentJSON()
	{
		m_currentTrimMap.clear();
		const String& jsonString = m_textureFontLabel->textureCellTrimByCharacterJSON();

		if (jsonString.isEmpty() || jsonString == U"{}")
		{
			return;
		}

		try
		{
			const JSON json = JSON::Parse(jsonString);
			if (json.isObject())
			{
				for (const auto& item : json)
				{
					const String& key = item.key;
					if (key.length() >= 1)
					{
						const char32 ch = key[0];
						const auto& value = item.value;
						if (value.isArray() && value.size() == 4)
						{
							m_currentTrimMap[ch] = LRTB{
								value[0].get<double>(),
								value[1].get<double>(),
								value[2].get<double>(),
								value[3].get<double>()
							};
						}
					}
				}
			}
		}
		catch (const Error&)
		{
			// パースエラー時は無視
		}
	}

	void TextureFontLabelCellTrimEditDialog::refreshList()
	{
		if (!m_listNode)
		{
			return;
		}

		m_listNode->removeChildrenAll();
		m_charTrimInfos.clear();

		if (m_currentTrimMap.empty())
		{
			// 空の場合のメッセージ
			const auto emptyNode = m_listNode->emplaceChild(
				U"Empty",
				InlineRegion{
					.sizeRatio = Vec2{ 1, 1 },
				});
			emptyNode->emplaceComponent<Label>(
				U"(設定なし)",
				U"", 13, ColorF{ 0.6 },
				HorizontalAlign::Center, VerticalAlign::Middle);
		}
		else
		{
			Array<std::pair<char32, LRTB>> sortedItems;
			for (const auto& [ch, trim] : m_currentTrimMap)
			{
				sortedItems.emplace_back(ch, trim);
			}
			sortedItems.sort_by([](const auto& a, const auto& b) {
				return a.first < b.first;
			});

			for (size_t i = 0; i < sortedItems.size(); ++i)
			{
				CharTrimInfo info;
				info.character = sortedItems[i].first;
				info.trim = sortedItems[i].second;
				m_charTrimInfos.push_back(info);

				createCharTrimRow(m_listNode, i);
			}
		}

		m_listNode->setInlineRegionToFitToChildren(FitTarget::HeightOnly);
	}

	void TextureFontLabelCellTrimEditDialog::createCharTrimRow(const std::shared_ptr<Node>& parentNode, size_t index)
	{
		auto& info = m_charTrimInfos[index];

		const auto rowNode = parentNode->emplaceChild(
			U"CharTrimRow_{}"_fmt(index),
			InlineRegion{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = SizeF{ -20, 32 },
				.margin = LRTB{ 10, 10, 2, 2 },
			});
		rowNode->setChildrenLayout(HorizontalLayout{ .spacing = 6 });
		info.rowNode = rowNode;

		// 背景
		rowNode->emplaceComponent<RectRenderer>(
			ColorF{ 0.15, 0.3 },
			ColorF{ 0.4, 0.3 },
			1.0, 0.0, 3.0);

		// 文字
		const auto charNode = rowNode->emplaceChild(
			U"Char",
			InlineRegion{
				.sizeRatio = Vec2{ 0, 1 },
				.sizeDelta = Vec2{ 40, 0 },
			});
		charNode->emplaceComponent<Label>(
			U"[{}]"_fmt(String{ info.character }),
			U"", 14, Palette::White,
			HorizontalAlign::Center, VerticalAlign::Middle);

		// トリミング量表示
		const auto valuesNode = rowNode->emplaceChild(
			U"Values",
			InlineRegion{
				.sizeRatio = Vec2{ 0, 1 },
				.flexibleWeight = 1.0,
			});
		valuesNode->setChildrenLayout(HorizontalLayout{ .spacing = 12 });

		// L
		const auto lNode = valuesNode->emplaceChild(
			U"L",
			InlineRegion{ .sizeRatio = Vec2{ 0, 1 }, .sizeDelta = Vec2{ 50, 0 } });
		lNode->emplaceComponent<Label>(
			U"L:{}"_fmt(info.trim.left),
			U"", 12, ColorF{ 0.9 },
			HorizontalAlign::Left, VerticalAlign::Middle);

		// R
		const auto rNode = valuesNode->emplaceChild(
			U"R",
			InlineRegion{ .sizeRatio = Vec2{ 0, 1 }, .sizeDelta = Vec2{ 50, 0 } });
		rNode->emplaceComponent<Label>(
			U"R:{}"_fmt(info.trim.right),
			U"", 12, ColorF{ 0.9 },
			HorizontalAlign::Left, VerticalAlign::Middle);

		// T
		const auto tNode = valuesNode->emplaceChild(
			U"T",
			InlineRegion{ .sizeRatio = Vec2{ 0, 1 }, .sizeDelta = Vec2{ 50, 0 } });
		tNode->emplaceComponent<Label>(
			U"T:{}"_fmt(info.trim.top),
			U"", 12, ColorF{ 0.9 },
			HorizontalAlign::Left, VerticalAlign::Middle);

		// B
		const auto bNode = valuesNode->emplaceChild(
			U"B",
			InlineRegion{ .sizeRatio = Vec2{ 0, 1 }, .sizeDelta = Vec2{ 50, 0 } });
		bNode->emplaceComponent<Label>(
			U"B:{}"_fmt(info.trim.bottom),
			U"", 12, ColorF{ 0.9 },
			HorizontalAlign::Left, VerticalAlign::Middle);

		// 編集ボタン
		info.editButton = rowNode->addChild(CreateButtonNode(
			U"編集",
			InlineRegion{
				.sizeDelta = Vec2{ 55, 24 },
				.margin = LRTB{ 0, 4, 0, 0 },
			},
			[this, index](const std::shared_ptr<Node>&) {
				onEditCharTrim(index);
			},
			IsDefaultButtonYN::No,
			12));

		// 削除ボタン
		info.deleteButton = rowNode->addChild(CreateButtonNode(
			U"ー 削除",
			InlineRegion{
				.sizeDelta = Vec2{ 70, 24 },
				.margin = LRTB{ 0, 4, 0, 0 },
			},
			[this, index](const std::shared_ptr<Node>&) {
				onDeleteCharTrim(index);
			},
			IsDefaultButtonYN::No,
			12));
	}

	void TextureFontLabelCellTrimEditDialog::onAddCharTrim()
	{
		if (!m_dialogOpener)
		{
			return;
		}

		// textureCellTrimの値を初期値として使用
		const LRTB defaultTrim = m_textureFontLabel->textureCellTrim().defaultValue();

		m_dialogOpener->openDialog(
			std::make_shared<CharTrimInputDialog>(
				defaultTrim,
				[this](char32 character, const LRTB& trim)
				{
					m_currentTrimMap[character] = trim;
					refreshList();
				}));
	}

	void TextureFontLabelCellTrimEditDialog::onEditCharTrim(size_t index)
	{
		if (index >= m_charTrimInfos.size() || !m_dialogOpener)
		{
			return;
		}

		const auto& info = m_charTrimInfos[index];

		m_dialogOpener->openDialog(
			std::make_shared<CharTrimInputDialog>(
				info.character,
				info.trim,
				[this](char32 character, const LRTB& trim)
				{
					m_currentTrimMap[character] = trim;
					refreshList();
				}));
	}

	void TextureFontLabelCellTrimEditDialog::onDeleteCharTrim(size_t index)
	{
		if (index >= m_charTrimInfos.size())
		{
			return;
		}

		const char32 character = m_charTrimInfos[index].character;

		m_currentTrimMap.erase(character);
		refreshList();
	}

	void TextureFontLabelCellTrimEditDialog::saveToComponent()
	{
		if (!m_textureFontLabel)
		{
			return;
		}

		m_textureFontLabel->setTextureCellTrimByCharacter(m_currentTrimMap);
	}
}
