#pragma once
#include <Siv3D.hpp>
#include "NocoUI.hpp"
#include "EditorTypes.hpp"
#include "Menu.hpp"

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

	virtual void createDialogContent(const std::shared_ptr<noco::Node>& contentRootNode, const std::shared_ptr<ContextMenu>& dialogContextMenu) = 0;

	virtual void onResult(StringView resultButtonText) = 0;
};

class DialogFrame
{
private:
	std::shared_ptr<noco::Canvas> m_dialogCanvas;
	std::shared_ptr<noco::Node> m_screenMaskNode;
	std::shared_ptr<noco::Node> m_dialogNode;
	std::shared_ptr<noco::Node> m_contentRootNode;
	std::shared_ptr<noco::Node> m_buttonRootNode;
	std::function<void(StringView)> m_onResult;

public:
	explicit DialogFrame(const std::shared_ptr<noco::Canvas>& dialogCanvas, double dialogWidth, const std::function<void(StringView)>& onResult, const Array<DialogButtonDesc>& buttonDescs);

	virtual ~DialogFrame() = default;

	std::shared_ptr<noco::Node> contentRootNode() const;

	void refreshLayoutForContent();
};

class DialogOpener
{
private:
	size_t m_nextDialogId = 1;
	std::shared_ptr<noco::Canvas> m_dialogCanvas;
	std::shared_ptr<ContextMenu> m_dialogContextMenu;
	HashTable<size_t, std::shared_ptr<DialogFrame>> m_openedDialogFrames;

public:
	explicit DialogOpener(const std::shared_ptr<noco::Canvas>& dialogCanvas, const std::shared_ptr<ContextMenu>& dialogContextMenu);

	void openDialog(const std::shared_ptr<IDialog>& dialog);

	bool anyDialogOpened() const;
};

class SimpleDialog : public IDialog
{
private:
	String m_text;
	std::function<void(StringView)> m_onResult;
	Array<DialogButtonDesc> m_buttonDescs;

public:
	SimpleDialog(StringView text, const std::function<void(StringView)>& onResult, const Array<DialogButtonDesc>& buttonDescs);

	double dialogWidth() const override;

	Array<DialogButtonDesc> buttonDescs() const override;

	void createDialogContent(const std::shared_ptr<noco::Node>& contentRootNode, const std::shared_ptr<ContextMenu>& dialogContextMenu) override;

	void onResult(StringView resultButtonText) override;
};

class InteractivePropertyValueDialog : public IDialog
{
private:
	noco::IProperty* m_pProperty;
	Array<String> m_buttonTexts;
	std::function<void()> m_onChange;

public:
	InteractivePropertyValueDialog(noco::IProperty* pProperty, std::function<void()> onChange);

	double dialogWidth() const override;

	Array<DialogButtonDesc> buttonDescs() const override;

	void createDialogContent(const std::shared_ptr<noco::Node>& contentRootNode, const std::shared_ptr<ContextMenu>& dialogContextMenu) override;

	void onResult(StringView) override;
};

