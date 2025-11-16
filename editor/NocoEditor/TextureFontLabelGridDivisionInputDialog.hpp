#pragma once
#include <NocoUI.hpp>
#include "EditorDialog.hpp"
#include "EditorColor.hpp"
#include "PropertyTextBox.hpp"
#include "TabStop.hpp"

namespace noco::editor
{
	class TextureFontLabelGridDivisionInputDialog : public IDialog
	{
	private:
		std::shared_ptr<TextureFontLabel> m_textureFontLabel;
		std::function<void()> m_onComplete;
		std::shared_ptr<Node> m_columnsTextBoxNode;
		std::shared_ptr<Node> m_rowsTextBoxNode;
		std::shared_ptr<Label> m_textureSizeLabel;
		std::shared_ptr<Label> m_cellSizeLabel;
		Size m_textureSize;

		void updateCellSizeDisplay()
		{
			int32 columns = 1;
			int32 rows = 1;

			if (const auto columnsTextBox = m_columnsTextBoxNode->getComponent<TextBox>())
			{
				columns = ParseOr<int32>(columnsTextBox->text(), 1);
			}
			if (const auto rowsTextBox = m_rowsTextBoxNode->getComponent<TextBox>())
			{
				rows = ParseOr<int32>(rowsTextBox->text(), 1);
			}

			if (columns <= 0) columns = 1;
			if (rows <= 0) rows = 1;

			const Vec2 cellSize{
				static_cast<double>(m_textureSize.x) / columns,
				static_cast<double>(m_textureSize.y) / rows
			};

			if (m_cellSizeLabel)
			{
				m_cellSizeLabel->setText(U"セルサイズ: {} x {}"_fmt(
					static_cast<int32>(cellSize.x),
					static_cast<int32>(cellSize.y)
				));
			}
		}


	public:
		explicit TextureFontLabelGridDivisionInputDialog(
			const std::shared_ptr<TextureFontLabel>& textureFontLabel,
			std::function<void()> onComplete)
			: m_textureFontLabel(textureFontLabel)
			, m_onComplete(std::move(onComplete))
			, m_textureSize(0, 0)
		{
			const String texturePath = m_textureFontLabel->textureFilePath().defaultValue();
			if (!texturePath.isEmpty())
			{
				Texture texture = noco::Asset::GetOrLoadTexture(texturePath);
				if (texture)
				{
					m_textureSize = texture.size();
				}
			}
		}

		double dialogWidth() const override
		{
			return 400;
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
			const auto descNode = contentRootNode->emplaceChild(
				U"Description",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 32 },
					.margin = LRTB{ 0, 0, 8, 8 },
				});
			descNode->emplaceComponent<Label>(
				U"テクスチャの縦横の分割数を入力してください",
				U"",
				14,
				Palette::White,
				HorizontalAlign::Center,
				VerticalAlign::Middle);

			const auto texSizeNode = contentRootNode->emplaceChild(
				U"TextureSize",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 24 },
					.margin = LRTB{ 16, 16, 4, 8 },
				});
			m_textureSizeLabel = texSizeNode->emplaceComponent<Label>(
				U"テクスチャサイズ: {} x {}"_fmt(m_textureSize.x, m_textureSize.y),
				U"",
				14,
				Palette::White,
				HorizontalAlign::Left,
				VerticalAlign::Middle);

			const auto columnsRow = contentRootNode->emplaceChild(
				U"ColumnsRow",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 32 },
					.margin = LRTB{ 0, 0, 4, 4 },
				});
			columnsRow->setChildrenLayout(HorizontalLayout{ .spacing = 8 });

			const auto columnsLabelNode = columnsRow->emplaceChild(
				U"ColumnsLabel",
				InlineRegion
				{
					.sizeDelta = Vec2{ 80, 32 },
				});
			columnsLabelNode->emplaceComponent<Label>(
				U"Columns:",
				U"",
				14,
				Palette::White,
				HorizontalAlign::Right,
				VerticalAlign::Middle);

			m_columnsTextBoxNode = columnsRow->emplaceChild(
				U"ColumnsTextBox",
				InlineRegion
				{
					.sizeRatio = Vec2{ 0, 0 },
					.sizeDelta = Vec2{ 0, 26 },
					.flexibleWeight = 1.0,
				});
			m_columnsTextBoxNode->emplaceComponent<RectRenderer>(
				PropertyValue<Color>{ Color{ 26, 26, 26, 204 } }.withDisabled(Color{ 51, 51, 51, 204 }).withSmoothTime(0.05),
				PropertyValue<Color>{ Color{ 255, 255, 255, 102 } }.withHovered(Palette::Skyblue).withStyleState(U"focused", Palette::Orange).withSmoothTime(0.05),
				1.0,
				0.0,
				4.0);
			const auto columnsTextBox = m_columnsTextBoxNode->emplaceComponent<TextBox>(
				U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 },
				HorizontalAlign::Left, VerticalAlign::Middle,
				Palette::White, Color{ 255, 165, 0, 128 });
			columnsTextBox->setText(Format(m_textureFontLabel->textureGridColumns().defaultValue()));
			m_columnsTextBoxNode->emplaceComponent<TabStop>();

			m_columnsTextBoxNode->emplaceComponent<PropertyTextBox>(
				columnsTextBox,
				[this](StringView) { updateCellSizeDisplay(); });

			const auto rowsRow = contentRootNode->emplaceChild(
				U"RowsRow",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 32 },
					.margin = LRTB{ 0, 0, 4, 4 },
				});
			rowsRow->setChildrenLayout(HorizontalLayout{ .spacing = 8 });

			const auto rowsLabelNode = rowsRow->emplaceChild(
				U"RowsLabel",
				InlineRegion
				{
					.sizeDelta = Vec2{ 80, 32 },
				});
			rowsLabelNode->emplaceComponent<Label>(
				U"Rows:",
				U"",
				14,
				Palette::White,
				HorizontalAlign::Right,
				VerticalAlign::Middle);

			m_rowsTextBoxNode = rowsRow->emplaceChild(
				U"RowsTextBox",
				InlineRegion
				{
					.sizeRatio = Vec2{ 0, 0 },
					.sizeDelta = Vec2{ 0, 26 },
					.flexibleWeight = 1.0,
				});
			m_rowsTextBoxNode->emplaceComponent<RectRenderer>(
				PropertyValue<Color>{ Color{ 26, 26, 26, 204 } }.withDisabled(Color{ 51, 51, 51, 204 }).withSmoothTime(0.05),
				PropertyValue<Color>{ Color{ 255, 255, 255, 102 } }.withHovered(Palette::Skyblue).withStyleState(U"focused", Palette::Orange).withSmoothTime(0.05),
				1.0,
				0.0,
				4.0);
			const auto rowsTextBox = m_rowsTextBoxNode->emplaceComponent<TextBox>(
				U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 },
				HorizontalAlign::Left, VerticalAlign::Middle,
				Palette::White, Color{ 255, 165, 0, 128 });
			rowsTextBox->setText(Format(m_textureFontLabel->textureGridRows().defaultValue()));
			m_rowsTextBoxNode->emplaceComponent<TabStop>();

			m_rowsTextBoxNode->emplaceComponent<PropertyTextBox>(
				rowsTextBox,
				[this](StringView) { updateCellSizeDisplay(); });

			const auto cellSizeNode = contentRootNode->emplaceChild(
				U"CellSize",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 24 },
					.margin = LRTB{ 16, 16, 8, 4 },
				});
			m_cellSizeLabel = cellSizeNode->emplaceComponent<Label>(
				U"セルサイズ: 0 x 0",
				U"",
				14,
				Palette::White,
				HorizontalAlign::Left,
				VerticalAlign::Middle);

			TabStop::LinkAllTabStops(contentRootNode, true);

			CurrentFrame::SetFocusedNode(m_columnsTextBoxNode);

			updateCellSizeDisplay();
		}

		void onResult(StringView resultButtonText) override
		{
			if (resultButtonText == U"OK")
			{
				int32 columns = 1;
				int32 rows = 1;

				if (const auto columnsTextBox = m_columnsTextBoxNode->getComponent<TextBox>())
				{
					columns = ParseOr<int32>(columnsTextBox->text(), 1);
				}
				if (const auto rowsTextBox = m_rowsTextBoxNode->getComponent<TextBox>())
				{
					rows = ParseOr<int32>(rowsTextBox->text(), 1);
				}

				if (columns <= 0 || rows <= 0)
				{
					return;
				}

				const Vec2 cellSize{
					static_cast<double>(m_textureSize.x) / columns,
					static_cast<double>(m_textureSize.y) / rows
				};

				m_textureFontLabel->setTextureCellSize(cellSize);
				m_textureFontLabel->setTextureGridColumns(columns);
				m_textureFontLabel->setTextureGridRows(rows);

				if (m_onComplete)
				{
					m_onComplete();
				}
			}
		}
	};
}
