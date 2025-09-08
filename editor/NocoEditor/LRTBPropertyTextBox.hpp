#pragma once
#include <NocoUI.hpp>
#include "EditorYN.hpp"

namespace noco::editor
{
	class LRTBPropertyTextBox : public ComponentBase
	{
	private:
		std::shared_ptr<TextBox> m_textBoxL;
		std::shared_ptr<TextBox> m_textBoxR;
		std::shared_ptr<TextBox> m_textBoxT;
		std::shared_ptr<TextBox> m_textBoxB;
		std::function<void(const LRTB&)> m_fnOnValueChanged;
		LRTB m_value;
		std::weak_ptr<Label> m_propertyLabelWeak;
		HasInteractivePropertyValueYN m_hasInteractivePropertyValue = HasInteractivePropertyValueYN::No;
		HasParameterRefYN m_hasParamRef = HasParameterRefYN::No;

	public:
		LRTBPropertyTextBox(
			const std::shared_ptr<TextBox>& textBoxL,
			const std::shared_ptr<TextBox>& textBoxR,
			const std::shared_ptr<TextBox>& textBoxT,
			const std::shared_ptr<TextBox>& textBoxB,
			std::function<void(const LRTB&)> fnOnValueChanged,
			const LRTB& initialValue,
			std::weak_ptr<Label> propertyLabelWeak = {},
			HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No,
			HasParameterRefYN hasParamRef = HasParameterRefYN::No)
			: ComponentBase{ {} }
			, m_textBoxL(textBoxL)
			, m_textBoxR(textBoxR)
			, m_textBoxT(textBoxT)
			, m_textBoxB(textBoxB)
			, m_fnOnValueChanged(std::move(fnOnValueChanged))
			, m_value(initialValue)
			, m_propertyLabelWeak(propertyLabelWeak)
			, m_hasInteractivePropertyValue(hasInteractivePropertyValue)
			, m_hasParamRef(hasParamRef)
		{
		}

		void update(const std::shared_ptr<Node>&) override
		{
			const double l = ParseOpt<double>(m_textBoxL->text()).value_or(m_value.left);
			const double r = ParseOpt<double>(m_textBoxR->text()).value_or(m_value.right);
			const double t = ParseOpt<double>(m_textBoxT->text()).value_or(m_value.top);
			const double b = ParseOpt<double>(m_textBoxB->text()).value_or(m_value.bottom);
			const LRTB newValue{ l, r, t, b };
			if (newValue != m_value)
			{
				// ステート値がある状態で編集した場合、即時に黄色下線を消す（パラメータ参照がある場合は保持）
				if (m_hasInteractivePropertyValue && !m_hasParamRef)
				{
					if (const auto label = m_propertyLabelWeak.lock())
					{
						label->setUnderlineStyle(LabelUnderlineStyle::None);
					}
					m_hasInteractivePropertyValue = HasInteractivePropertyValueYN::No;
				}
				
				// コールバック内でInspectorを再構成する場合があるためコールバックは最後に実行
				m_value = newValue;
				if (m_fnOnValueChanged)
				{
					m_fnOnValueChanged(newValue);
				}
			}
		}

		void draw(const Node&) const override
		{
		}

		void setValue(const LRTB& value, bool callsOnValueChanged = false)
		{
			m_value = value;

			m_textBoxL->setText(Format(value.left));
			m_textBoxR->setText(Format(value.right));
			m_textBoxT->setText(Format(value.top));
			m_textBoxB->setText(Format(value.bottom));

			if (callsOnValueChanged && m_fnOnValueChanged)
			{
				m_fnOnValueChanged(m_value);
			}
		}

		const LRTB& value() const
		{
			return m_value;
		}
	};
}
