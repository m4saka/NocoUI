#include "Dialog.hpp"
#include "EditorUtility.hpp"

using namespace noco;

DialogFrame::DialogFrame(const std::shared_ptr<Canvas>& dialogCanvas, double dialogWidth, const std::function<void(StringView)>& onResult, const Array<DialogButtonDesc>& buttonDescs)
	: m_dialogCanvas(dialogCanvas)
	, m_screenMaskNode(dialogCanvas->rootNode()->emplaceChild(
		U"Dialog_ScreenMask",
		AnchorConstraint
		{
			.anchorMin = Anchor::TopLeft,
			.anchorMax = Anchor::BottomRight,
			.posDelta = Vec2{ 0, 0 },
			.sizeDelta = Vec2{ 0, 0 },
			.sizeDeltaPivot = Anchor::TopLeft,
		}))
	, m_dialogNode(m_screenMaskNode->emplaceChild(
		U"Dialog",
		BoxConstraint
		{
			.sizeRatio = Vec2{ 0, 0 },
			.sizeDelta = Vec2{ dialogWidth, 0 },
			.margin = LRTB{ 0, 0, 0, 0 },
		}))
	, m_contentRootNode(m_dialogNode->emplaceChild(
		U"Dialog_ContentRoot",
		BoxConstraint
		{
			.sizeRatio = Vec2{ 1, 0 },
			.margin = LRTB{ 0, 0, 0, 0 },
		}))
	, m_buttonRootNode(m_dialogNode->emplaceChild(
		U"Dialog_ButtonRoot",
		BoxConstraint
		{
			.sizeRatio = Vec2{ 1, 0 },
			.margin = LRTB{ 0, 0, 0, 0 },
		}))
	, m_onResult(onResult)
{
	m_screenMaskNode->emplaceComponent<InputBlocker>();

	// ダイアログ背面を暗くする
	m_screenMaskNode->emplaceComponent<RectRenderer>(ColorF{ 0.0, 0.25 });
	m_screenMaskNode->setBoxChildrenLayout(FlowLayout{ .horizontalAlign = HorizontalAlign::Center, .verticalAlign = VerticalAlign::Middle }, RefreshesLayoutYN::No);

	m_dialogNode->setBoxChildrenLayout(VerticalLayout{ .padding = LRTB{ 8, 8, 8, 12 } }, RefreshesLayoutYN::No);
	m_dialogNode->emplaceComponent<RectRenderer>(ColorF{ 0.1, 0.8 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0, ColorF{ 0.0, 0.3 }, Vec2{ 2, 2 }, 8.0, 4.0);

	const auto buttonParentNode = m_dialogNode->emplaceChild(
		U"Dialog_ButtonParent",
		BoxConstraint
		{
			.sizeRatio = Vec2{ 1, 0 },
			.margin = LRTB{ 0, 0, 8, 0 },
		});
	buttonParentNode->setBoxChildrenLayout(HorizontalLayout{ .padding = LRTB{ 0, 0, 0, 0 }, .horizontalAlign = HorizontalAlign::Center }, RefreshesLayoutYN::No);
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
				BoxConstraint
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
				buttonDesc.isDefaultButton),
			RefreshesLayoutYN::No);

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
	buttonParentNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly, RefreshesLayoutYN::No);
	m_dialogNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly, RefreshesLayoutYN::No);

	m_dialogCanvas->refreshLayout();
}

std::shared_ptr<Node> DialogFrame::contentRootNode() const
{
	return m_contentRootNode;
}

void DialogFrame::refreshLayoutForContent()
{
	m_contentRootNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly, RefreshesLayoutYN::No);
	m_dialogNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly, RefreshesLayoutYN::No);
	m_dialogCanvas->refreshLayout();
}

DialogOpener::DialogOpener(const std::shared_ptr<Canvas>& dialogCanvas, const std::shared_ptr<ContextMenu>& dialogContextMenu)
	: m_dialogCanvas(dialogCanvas)
	, m_dialogContextMenu(dialogContextMenu)
{
}

void DialogOpener::openDialog(const std::shared_ptr<IDialog>& dialog)
{
	auto dialogFrame = std::make_shared<DialogFrame>(m_dialogCanvas, dialog->dialogWidth(), [this, dialogId = m_nextDialogId, dialog](StringView resultButtonText) { dialog->onResult(resultButtonText); m_openedDialogFrames.erase(dialogId); }, dialog->buttonDescs());
	dialog->createDialogContent(dialogFrame->contentRootNode(), m_dialogContextMenu);
	dialogFrame->refreshLayoutForContent();
	m_openedDialogFrames.emplace(m_nextDialogId, dialogFrame);
	++m_nextDialogId;
}

bool DialogOpener::anyDialogOpened() const
{
	return !m_openedDialogFrames.empty();
}

SimpleDialog::SimpleDialog(StringView text, const std::function<void(StringView)>& onResult, const Array<DialogButtonDesc>& buttonDescs)
	: m_text(text)
	, m_onResult(onResult)
	, m_buttonDescs(buttonDescs)
{
}

double SimpleDialog::dialogWidth() const
{
	return 400;
}

Array<DialogButtonDesc> SimpleDialog::buttonDescs() const
{
	return m_buttonDescs;
}

void SimpleDialog::createDialogContent(const std::shared_ptr<Node>& contentRootNode, const std::shared_ptr<ContextMenu>&)
{
	const auto labelNode = contentRootNode->emplaceChild(
		U"Label",
		BoxConstraint
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

void SimpleDialog::onResult(StringView resultButtonText)
{
	if (m_onResult)
	{
		m_onResult(resultButtonText);
	}
}

InteractivePropertyValueDialog::InteractivePropertyValueDialog(IProperty* pProperty, std::function<void()> onChange)
	: m_pProperty(pProperty)
	, m_onChange(std::move(onChange))
{
	if (!m_pProperty)
	{
		throw Error{ U"Property is nullptr" };
	}
	if (!m_pProperty->isInteractiveProperty())
	{
		throw Error{ U"Property is not interactive" };
	}
}

double InteractivePropertyValueDialog::dialogWidth() const
{
	return m_pProperty->editType() == PropertyEditType::LRTB ? 640 : 500;
}

Array<DialogButtonDesc> InteractivePropertyValueDialog::buttonDescs() const
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

void InteractivePropertyValueDialog::createDialogContent(const std::shared_ptr<Node>& contentRootNode, const std::shared_ptr<ContextMenu>& dialogContextMenu)
{
	// TODO: この実装はInspectorクラスに依存しているため、Inspectorクラスの抽出後に実装する
	// InteractivePropertyValueDialog::createDialogContent implementation will be added after Inspector class extraction
}

void InteractivePropertyValueDialog::onResult(StringView)
{
	if (m_onChange)
	{
		m_onChange();
	}
}