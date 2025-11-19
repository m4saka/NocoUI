#pragma once
#include <NocoUI.hpp>
#include "EditorDialog.hpp"
#include "EditorColor.hpp"
#include "CheckboxToggler.hpp"
#include "TabStop.hpp"

namespace noco::editor
{
	class Inspector;

	class CharTrimInputDialog : public IDialog
	{
	private:
		std::shared_ptr<TextBox> m_charTextBox;
		std::shared_ptr<LRTBPropertyTextBox> m_lrtbPropertyTextBox;
		std::function<void(char32, const LRTB&)> m_onComplete;

		Optional<char32> m_initialChar;
		LRTB m_initialTrim;
		bool m_isEditMode;

	public:
		// 新規追加ダイアログの場合
		explicit CharTrimInputDialog(
			const LRTB& initialTrim,
			std::function<void(char32, const LRTB&)> onComplete);

		// 編集ダイアログの場合
		explicit CharTrimInputDialog(
			char32 character,
			const LRTB& initialTrim,
			std::function<void(char32, const LRTB&)> onComplete);

		double dialogWidth() const override { return 300; }

		Array<DialogButtonDesc> buttonDescs() const override;

		void createDialogContent(
			const std::shared_ptr<Node>& contentRootNode,
			const std::shared_ptr<ContextMenu>& dialogContextMenu,
			std::function<void()> fnRefreshLayoutForContent) override;

		void onResult(StringView resultButtonText) override;
	};

	class TextureFontLabelCellTrimEditDialog : public IDialog
	{
	private:
		std::shared_ptr<TextureFontLabel> m_textureFontLabel;
		std::function<void()> m_onComplete;
		std::shared_ptr<DialogOpener> m_dialogOpener;

		struct CharTrimInfo
		{
			char32 character;
			LRTB trim;
			std::shared_ptr<Node> rowNode;
			std::shared_ptr<Node> editButton;
			std::shared_ptr<Node> deleteButton;
		};

		Array<CharTrimInfo> m_charTrimInfos;
		std::shared_ptr<Node> m_listNode;
		HashTable<char32, LRTB> m_currentTrimMap;

	public:
		explicit TextureFontLabelCellTrimEditDialog(
			const std::shared_ptr<TextureFontLabel>& textureFontLabel,
			std::function<void()> onComplete,
			const std::shared_ptr<DialogOpener>& dialogOpener);

		double dialogWidth() const override { return 500; }

		Array<DialogButtonDesc> buttonDescs() const override;

		void createDialogContent(
			const std::shared_ptr<Node>& contentRootNode,
			const std::shared_ptr<ContextMenu>& dialogContextMenu,
			std::function<void()> fnRefreshLayoutForContent) override;

		void onResult(StringView resultButtonText) override;

	private:
		void parseCurrentJSON();
		void refreshList();
		void createCharTrimRow(const std::shared_ptr<Node>& parentNode, size_t index);
		void onAddCharTrim();
		void onEditCharTrim(size_t index);
		void onDeleteCharTrim(size_t index);
		void saveToComponent();
	};
}
