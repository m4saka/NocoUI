#include "SubCanvasParamsEditDialog.hpp"
#include "Inspector.hpp"

namespace noco::editor
{
	void SubCanvasParamsEditDialog::createParamRow(const std::shared_ptr<Node>& parentNode, size_t index)
	{
		auto& info = m_paramNodes[index];

		const auto rowNode = parentNode->emplaceChild(
			U"ParamRow_{}"_fmt(index),
			InlineRegion
			{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = SizeF{ -20, 0 },
				.margin = LRTB{ 0, 0, 0, 8 },
			});
		rowNode->setChildrenLayout(HorizontalLayout{ .padding = LRTB{ 8, 0, 0, 0 } });
		info.rowNode = rowNode;

		// 型に応じた入力コントロール
		info.valueInputNode = rowNode->addChild(createValueInputNode(index));

		// チェックボックスを先頭に挿入
		info.checkboxNode = rowNode->addChildAtIndex(
			Inspector::CreateCheckboxNode(
				info.isChecked,
				[this, index](bool checked)
				{
					m_paramNodes[index].isChecked = checked;
					updateParamRowInteractable(index);
				}),
			0);

		// 高さを子要素に合わせる
		rowNode->setInlineRegionToFitToChildren(FitTarget::HeightOnly);

		// 初期状態の有効/無効を設定
		updateParamRowInteractable(index);
	}

	std::shared_ptr<Node> SubCanvasParamsEditDialog::createValueInputNode(size_t index)
	{
		auto& info = m_paramNodes[index];

		switch (info.paramType)
		{
		case ParamType::Bool:
			return createBoolInput(index);
		case ParamType::Number:
			return createNumberInput(index);
		case ParamType::String:
			return createStringInput(index);
		case ParamType::Color:
			return createColorInput(index);
		case ParamType::Vec2:
			return createVec2Input(index);
		case ParamType::LRTB:
			return createLRTBInput(index);
		default:
			return createStringInput(index);
		}
	}

	std::shared_ptr<Node> SubCanvasParamsEditDialog::createBoolInput(size_t index)
	{
		auto& info = m_paramNodes[index];

		const bool defaultValue = GetParamValueAs<bool>(info.defaultValue).value_or(false);
		const bool initialValue = ParseOr<bool>(*info.currentValueString, defaultValue);

		return Inspector::CreateBoolPropertyNode(
			U"{} [{}]"_fmt(info.paramName, ParamTypeToString(info.paramType)),
			initialValue,
			[this, index](bool value)
			{
				*m_paramNodes[index].currentValueString = value ? U"true" : U"false";
			});
	}

	std::shared_ptr<Node> SubCanvasParamsEditDialog::createNumberInput(size_t index)
	{
		auto& info = m_paramNodes[index];

		return Inspector::CreatePropertyNode(
			U"{} [{}]"_fmt(info.paramName, ParamTypeToString(info.paramType)),
			*info.currentValueString,
			[this, index](StringView text)
			{
				*m_paramNodes[index].currentValueString = String{ text };
			});
	}

	std::shared_ptr<Node> SubCanvasParamsEditDialog::createStringInput(size_t index)
	{
		auto& info = m_paramNodes[index];

		return Inspector::CreatePropertyNodeWithTextArea(
			U"{} [{}]"_fmt(info.paramName, ParamTypeToString(info.paramType)),
			*info.currentValueString,
			[this, index](StringView text)
			{
				*m_paramNodes[index].currentValueString = String{ text };
			},
			HasInteractivePropertyValueYN::No,
			3);
	}

	std::shared_ptr<Node> SubCanvasParamsEditDialog::createColorInput(size_t index)
	{
		auto& info = m_paramNodes[index];

		const Color defaultValue = GetParamValueAs<Color>(info.defaultValue).value_or(Palette::White);
		const Color initialValue = ParseOr<Color>(*info.currentValueString, defaultValue);

		return Inspector::CreateColorPropertyNode(
			U"{} [{}]"_fmt(info.paramName, ParamTypeToString(info.paramType)),
			initialValue,
			[this, index](const Color& color)
			{
				*m_paramNodes[index].currentValueString = ValueToString(color);
			});
	}

	std::shared_ptr<Node> SubCanvasParamsEditDialog::createVec2Input(size_t index)
	{
		auto& info = m_paramNodes[index];

		const Vec2 defaultValue = GetParamValueAs<Vec2>(info.defaultValue).value_or(Vec2::Zero());
		const Vec2 initialValue = ParseOr<Vec2>(*info.currentValueString, defaultValue);

		return Inspector::CreateVec2PropertyNode(
			U"{} [{}]"_fmt(info.paramName, ParamTypeToString(info.paramType)),
			initialValue,
			[this, index](const Vec2& vec)
			{
				*m_paramNodes[index].currentValueString = ValueToString(vec);
			});
	}

	std::shared_ptr<Node> SubCanvasParamsEditDialog::createLRTBInput(size_t index)
	{
		auto& info = m_paramNodes[index];

		const LRTB defaultValue = GetParamValueAs<LRTB>(info.defaultValue).value_or(LRTB::Zero());
		const LRTB initialValue = ParseOr<LRTB>(*info.currentValueString, defaultValue);

		return Inspector::CreateLRTBPropertyNode(
			U"{} [{}]"_fmt(info.paramName, ParamTypeToString(info.paramType)),
			initialValue,
			[this, index](const LRTB& lrtb)
			{
				*m_paramNodes[index].currentValueString = ValueToString(lrtb);
			});
	}
}
