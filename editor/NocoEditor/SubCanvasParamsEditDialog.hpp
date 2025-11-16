#pragma once
#include <NocoUI.hpp>
#include "EditorDialog.hpp"
#include "EditorColor.hpp"
#include "CheckboxToggler.hpp"
#include "TabStop.hpp"

namespace noco::editor
{
	class SubCanvasParamsEditDialog : public IDialog
	{
	private:
		std::shared_ptr<SubCanvas> m_subCanvas;
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

		struct ParamNodeInfo
		{
			String paramName;
			ParamType paramType;
			ParamValue defaultValue;
			std::shared_ptr<Node> rowNode;
			std::shared_ptr<Node> checkboxNode;
			std::shared_ptr<Node> valueInputNode;
			std::shared_ptr<String> currentValueString;
			bool isChecked = false;
		};
		Array<ParamNodeInfo> m_paramNodes;

		JSON m_currentParamsJSON;

	public:
		explicit SubCanvasParamsEditDialog(
			const std::shared_ptr<SubCanvas>& subCanvas,
			std::function<void()> onComplete,
			const std::shared_ptr<DialogOpener>& dialogOpener)
			: m_subCanvas(subCanvas)
			, m_onComplete(std::move(onComplete))
			, m_dialogOpener(dialogOpener)
		{
			// SubCanvasからcanvasPathを取得
			const String canvasPath = m_subCanvas->canvasPath().defaultValue();

			if (canvasPath.isEmpty())
			{
				m_dialogState = DialogState::CanvasLoadError;
				return;
			}

			// Canvasを読み込み
			const FilePath fullPath = FileSystem::PathAppend(noco::Asset::GetBaseDirectoryPath(), canvasPath);
			m_targetCanvas = Canvas::LoadFromFile(fullPath, AllowExceptions::No);

			if (!m_targetCanvas)
			{
				m_dialogState = DialogState::CanvasLoadError;
				return;
			}

			// パラメータを取得
			const auto& params = m_targetCanvas->params();

			if (params.empty())
			{
				m_dialogState = DialogState::NoParams;
				return;
			}

			// 現在のparamsJSONをパース
			const String paramsJSONString = m_subCanvas->paramsJSON().defaultValue();
			if (!paramsJSONString.isEmpty())
			{
				m_currentParamsJSON = JSON::Parse(paramsJSONString);
			}

			// パラメータ一覧を作成（ソート済み）
			Array<std::pair<String, ParamValue>> sortedParams;
			for (const auto& [name, value] : params)
			{
				sortedParams.emplace_back(name, value);
			}
			sortedParams.sort_by([](const auto& a, const auto& b) { return a.first < b.first; });

			// ParamNodeInfo配列を構築
			for (const auto& [name, value] : sortedParams)
			{
				ParamNodeInfo info;
				info.paramName = name;
				info.paramType = GetParamType(value);
				info.defaultValue = value;
				info.currentValueString = std::make_shared<String>();

				// 現在のJSONに値が存在するかチェック
				if (m_currentParamsJSON.isObject() && m_currentParamsJSON.hasElement(name))
				{
					info.isChecked = true;
					const auto& paramJSON = m_currentParamsJSON[name];
					if (auto paramValue = ParamValueFromJSONValue(paramJSON, info.paramType))
					{
						*info.currentValueString = ParamValueToString(*paramValue);
					}
					else
					{
						*info.currentValueString = ParamValueToString(value);
					}
				}
				else
				{
					info.isChecked = false;
					*info.currentValueString = ParamValueToString(value);
				}

				m_paramNodes.push_back(info);
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
				DialogButtonDesc{ .text = U"キャンセル", .mnemonicInput = KeyC, .isCancelButton = IsCancelButtonYN::Yes }
			};
		}

		void createDialogContent(const std::shared_ptr<Node>& contentRootNode, const std::shared_ptr<ContextMenu>&, std::function<void()>) override
		{
			if (m_dialogState == DialogState::CanvasLoadError)
			{
				const auto messageNode = contentRootNode->emplaceChild(
					U"Message",
					InlineRegion
					{
						.sizeRatio = Vec2{ 1, 0 },
						.sizeDelta = SizeF{ 0, 100 },
						.margin = LRTB{ 0, 0, 16, 16 },
					});
				messageNode->emplaceComponent<Label>(
					U"Canvasが存在しません",
					U"",
					16,
					Palette::White,
					HorizontalAlign::Center,
					VerticalAlign::Middle);
				return;
			}

			if (m_dialogState == DialogState::NoParams)
			{
				const auto messageNode = contentRootNode->emplaceChild(
					U"Message",
					InlineRegion
					{
						.sizeRatio = Vec2{ 1, 0 },
						.sizeDelta = SizeF{ 0, 100 },
						.margin = LRTB{ 0, 0, 16, 16 },
					});
				messageNode->emplaceComponent<Label>(
					U"Canvasにパラメータがありません",
					U"",
					16,
					Palette::White,
					HorizontalAlign::Center,
					VerticalAlign::Middle);
				return;
			}

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
				U"パラメータ編集",
				U"",
				16,
				Palette::White,
				HorizontalAlign::Center,
				VerticalAlign::Middle);

			// パラメータリストコンテナ
			const auto paramsListNode = contentRootNode->emplaceChild(
				U"ParamsList",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 0 },
					.margin = LRTB{ 0, 0, 0, 8 },
				});
			paramsListNode->setChildrenLayout(VerticalLayout{ .spacing = 4 });

			// 各パラメータの行を作成
			for (size_t i = 0; i < m_paramNodes.size(); ++i)
			{
				createParamRow(paramsListNode, i);
			}

			// 高さを子要素にフィット
			paramsListNode->setInlineRegionToFitToChildren(FitTarget::HeightOnly);

			TabStop::LinkAllTabStops(contentRootNode, true);
		}

		void createParamRow(const std::shared_ptr<Node>& parentNode, size_t index);

		std::shared_ptr<Node> createValueInputNode(const std::shared_ptr<Node>& parentNode, size_t index);

		std::shared_ptr<Node> createBoolInput(const std::shared_ptr<Node>& parentNode, size_t index);

		std::shared_ptr<Node> createNumberInput(const std::shared_ptr<Node>& parentNode, size_t index);

		std::shared_ptr<Node> createStringInput(const std::shared_ptr<Node>& parentNode, size_t index);

		std::shared_ptr<Node> createColorInput(const std::shared_ptr<Node>& parentNode, size_t index);

		std::shared_ptr<Node> createVec2Input(const std::shared_ptr<Node>& parentNode, size_t index);

		std::shared_ptr<Node> createLRTBInput(const std::shared_ptr<Node>& parentNode, size_t index);

		void updateParamRowInteractable(size_t index)
		{
			auto& info = m_paramNodes[index];
			if (info.valueInputNode)
			{
				info.valueInputNode->setInteractable(info.isChecked);
			}
		}

		void onResult(StringView resultButtonText) override
		{
			if (resultButtonText == U"OK")
			{
				if (m_dialogState != DialogState::Normal)
				{
					return;
				}

				// チェックされたパラメータのみをJSONに変換
				JSON paramsJSON = JSON::Parse(U"{}");

				for (const auto& info : m_paramNodes)
				{
					if (info.isChecked)
					{
						// 文字列から型に応じた値に変換してJSONに追加
						if (auto paramValue = ParamValueFromString(info.paramType, *info.currentValueString))
						{
							paramsJSON[info.paramName] = ParamValueToJSONValue(*paramValue);
						}
					}
				}

				// JSONを文字列化してSubCanvasに設定
				const String paramsJSONString = paramsJSON.formatMinimum();
				m_subCanvas->setParamsJSON(paramsJSONString);

				if (m_onComplete)
				{
					m_onComplete();
				}
			}
		}
	};
}
