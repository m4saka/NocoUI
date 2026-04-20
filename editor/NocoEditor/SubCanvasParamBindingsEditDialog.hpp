#pragma once
#include <NocoUI.hpp>
#include "EditorDialog.hpp"
#include "EditorColor.hpp"
#include "AddParamDialog.hpp"

namespace noco::editor
{
	class SubCanvasParamBindingsEditDialog : public IDialog
	{
	private:
		std::shared_ptr<SubCanvas> m_subCanvas;
		std::shared_ptr<Canvas> m_parentCanvas;
		std::function<void()> m_onComplete;
		std::shared_ptr<DialogOpener> m_dialogOpener;
		std::shared_ptr<Canvas> m_targetCanvas;

		enum class DialogState
		{
			Normal,
			CanvasLoadError,
			NoParams,
		};
		DialogState m_dialogState = DialogState::Normal;

		struct BindingInfo
		{
			String subCanvasParamName;
			ParamType subCanvasParamType;
			std::shared_ptr<Node> rowNode;
			std::shared_ptr<Node> comboBoxNode;
			std::shared_ptr<Label> comboLabel;
			String selectedParentParamName;
			Array<std::pair<String, ParamValue>> availableParentParams;
		};
		Array<BindingInfo> m_bindings;

		JSON m_currentBindingsJSON;

	public:
		explicit SubCanvasParamBindingsEditDialog(
			const std::shared_ptr<SubCanvas>& subCanvas,
			const std::shared_ptr<Canvas>& parentCanvas,
			std::function<void()> onComplete,
			const std::shared_ptr<DialogOpener>& dialogOpener)
			: m_subCanvas(subCanvas)
			, m_parentCanvas(parentCanvas)
			, m_onComplete(std::move(onComplete))
			, m_dialogOpener(dialogOpener)
		{
			const String canvasPath = m_subCanvas->canvasPath().defaultValue();

			if (canvasPath.isEmpty())
			{
				m_dialogState = DialogState::CanvasLoadError;
				return;
			}

			const FilePath fullPath = noco::Asset::GetFullPath(canvasPath);
			m_targetCanvas = Canvas::LoadFromFile(fullPath, AllowExceptions::No);

			if (!m_targetCanvas)
			{
				m_dialogState = DialogState::CanvasLoadError;
				return;
			}

			const auto& subCanvasParams = m_targetCanvas->params();

			if (subCanvasParams.empty())
			{
				m_dialogState = DialogState::NoParams;
				return;
			}

			// 既存のserializedParamBindingsJSONをパース
			const String bindingsJSONString = m_subCanvas->serializedParamBindingsJSON();
			if (!bindingsJSONString.isEmpty())
			{
				m_currentBindingsJSON = JSON::Parse(bindingsJSONString);
			}

			// 子Canvasのパラメータ一覧をソート
			Array<std::pair<String, ParamValue>> sortedSubCanvasParams;
			for (const auto& [name, value] : subCanvasParams)
			{
				sortedSubCanvasParams.emplace_back(name, value);
			}
			sortedSubCanvasParams.sort_by([](const auto& a, const auto& b) { return a.first < b.first; });

			// 親Canvasのパラメータ
			const auto& parentParams = m_parentCanvas->params();

			for (const auto& [name, value] : sortedSubCanvasParams)
			{
				BindingInfo info;
				info.subCanvasParamName = name;
				info.subCanvasParamType = GetParamType(value);

				// 型フィルタリングで利用可能な親パラメータを収集
				for (const auto& [parentName, parentValue] : parentParams)
				{
					if (GetParamType(parentValue) == info.subCanvasParamType)
					{
						info.availableParentParams.push_back({ parentName, parentValue });
					}
				}
				info.availableParentParams.sort_by([](const auto& a, const auto& b) { return a.first < b.first; });

				// 既存のserializedParamBindingsJSONから初期値を取得
				if (m_currentBindingsJSON.isObject() && m_currentBindingsJSON.hasElement(name))
				{
					const auto& bindingValue = m_currentBindingsJSON[name];
					if (bindingValue.isString())
					{
						info.selectedParentParamName = bindingValue.getString();
					}
				}

				m_bindings.push_back(info);
			}

			m_dialogState = DialogState::Normal;
		}

		double dialogWidth() const override
		{
			return 640;
		}

		Array<DialogButtonDesc> buttonDescs() const override
		{
			return {
				DialogButtonDesc{ .text = U"OK", .isDefaultButton = IsDefaultButtonYN::Yes },
				DialogButtonDesc{ .text = U"キャンセル", .mnemonicInput = KeyC, .isCancelButton = IsCancelButtonYN::Yes },
			};
		}

		void createDialogContent(const std::shared_ptr<Node>& contentRootNode, const std::shared_ptr<ContextMenu>& dialogContextMenu, std::function<void()>) override
		{
			if (m_dialogState == DialogState::CanvasLoadError)
			{
				const auto messageNode = contentRootNode->emplaceChild(
					U"Message",
					InlineRegion
					{
						.sizeRatio = Vec2{ 1, 0 },
						.sizeDelta = SizeF{ 0, 0 },
						.margin = LRTB{ 0, 0, 16, 16 },
					});
				const auto msgLabel = messageNode->emplaceComponent<Label>(
					U"Canvasが存在しません",
					U"",
					16,
					Palette::White,
					HorizontalAlign::Center,
					VerticalAlign::Middle)
					->setSizingMode(LabelSizingMode::AutoResizeHeight);
				msgLabel->refreshAutoResizeImmediately(messageNode);
				return;
			}

			if (m_dialogState == DialogState::NoParams)
			{
				const auto messageNode = contentRootNode->emplaceChild(
					U"Message",
					InlineRegion
					{
						.sizeRatio = Vec2{ 1, 0 },
						.sizeDelta = SizeF{ 0, 0 },
						.margin = LRTB{ 0, 0, 16, 16 },
					});
				const auto msgLabel = messageNode->emplaceComponent<Label>(
					U"Canvasにパラメータがありません",
					U"",
					16,
					Palette::White,
					HorizontalAlign::Center,
					VerticalAlign::Middle)
					->setSizingMode(LabelSizingMode::AutoResizeHeight);
				msgLabel->refreshAutoResizeImmediately(messageNode);
				return;
			}

			// タイトル
			const auto titleNode = contentRootNode->emplaceChild(
				U"Title",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 0 },
					.margin = LRTB{ 0, 0, 8, 8 },
				});
			const auto titleLabel = titleNode->emplaceComponent<Label>(
				U"パラメータ紐付けの編集",
				U"",
				16,
				Palette::White,
				HorizontalAlign::Center,
				VerticalAlign::Middle)
				->setSizingMode(LabelSizingMode::AutoResizeHeight);
			titleLabel->refreshAutoResizeImmediately(titleNode);

			// 列見出し
			const auto headerRowNode = contentRootNode->emplaceChild(
				U"BindingsHeader",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = SizeF{ -20, 20 },
					.margin = LRTB{ 0, 0, 0, 4 },
				});
			headerRowNode->setChildrenLayout(HorizontalLayout{ .spacing = 8, .padding = LRTB{ 8, 0, 0, 0 } });

			const auto subCanvasHeaderNode = headerRowNode->emplaceChild(
				U"SubCanvasHeader",
				InlineRegion
				{
					.sizeRatio = Vec2{ 0, 1 },
					.sizeDelta = Vec2{ 200, 0 },
				},
				IsHitTargetYN::No);
			subCanvasHeaderNode->emplaceComponent<Label>(
				U"子Canvasのパラメータ",
				U"",
				13,
				ColorF{ 0.8, 0.8, 0.8 },
				HorizontalAlign::Center,
				VerticalAlign::Middle)
				->setSizingMode(LabelSizingMode::AutoShrink);

			const auto parentHeaderNode = headerRowNode->emplaceChild(
				U"ParentHeader",
				InlineRegion
				{
					.sizeRatio = Vec2{ 0, 1 },
					.sizeDelta = Vec2{ 0, 0 },
					.flexibleWeight = 1.0,
				},
				IsHitTargetYN::No);
			parentHeaderNode->emplaceComponent<Label>(
				U"紐付けるパラメータ",
				U"",
				13,
				ColorF{ 0.8, 0.8, 0.8 },
				HorizontalAlign::Center,
				VerticalAlign::Middle)
				->setSizingMode(LabelSizingMode::AutoShrink);

			// リストコンテナ
			const auto listNode = contentRootNode->emplaceChild(
				U"BindingsList",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 0 },
					.margin = LRTB{ 0, 0, 0, 8 },
				});
			listNode->setChildrenLayout(VerticalLayout{ .spacing = 4 });

			for (size_t i = 0; i < m_bindings.size(); ++i)
			{
				createBindingRow(listNode, i, dialogContextMenu);
			}

			listNode->setInlineRegionToFitToChildren(FitTarget::HeightOnly);
		}

		void createBindingRow(const std::shared_ptr<Node>& parentNode, size_t index, const std::shared_ptr<ContextMenu>& dialogContextMenu);

		void onComboBoxClick(size_t index, const std::shared_ptr<ContextMenu>& dialogContextMenu);

		void selectParentParam(size_t index, const String& paramName);

		void onResult(StringView resultButtonText) override
		{
			if (resultButtonText == U"OK")
			{
				if (m_dialogState != DialogState::Normal)
				{
					return;
				}

				JSON bindingsJSON = JSON::Parse(U"{}");

				for (const auto& info : m_bindings)
				{
					if (!info.selectedParentParamName.isEmpty())
					{
						bindingsJSON[info.subCanvasParamName] = info.selectedParentParamName;
					}
				}

				const String bindingsJSONString = bindingsJSON.formatMinimum();
				m_subCanvas->setSerializedParamBindingsJSON(bindingsJSONString);

				if (m_onComplete)
				{
					m_onComplete();
				}
			}
		}
	};
}
