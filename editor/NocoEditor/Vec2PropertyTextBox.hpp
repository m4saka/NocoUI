#pragma once
#include <NocoUI.hpp>
#include "EditorYN.hpp"

namespace noco::editor
{
	class Vec2PropertyTextBox : public ComponentBase
	{
	private:
		std::shared_ptr<TextBox> m_textBoxX;
		std::shared_ptr<TextBox> m_textBoxY;
		std::function<void(const Vec2&)> m_fnOnValueChanged;
		Vec2 m_value;
		std::weak_ptr<Label> m_propertyLabelWeak;
		HasInteractivePropertyValueYN m_hasInteractivePropertyValue = HasInteractivePropertyValueYN::No;
		HasParameterRefYN m_hasParamRef = HasParameterRefYN::No;

	public:
		Vec2PropertyTextBox(
			const std::shared_ptr<TextBox>& textBoxX,
			const std::shared_ptr<TextBox>& textBoxY,
			std::function<void(const Vec2&)> fnOnValueChanged,
			const Vec2& initialValue,
			std::weak_ptr<Label> propertyLabelWeak = {},
			HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No,
			HasParameterRefYN hasParamRef = HasParameterRefYN::No)
			: ComponentBase{ {} }
			, m_textBoxX(textBoxX)
			, m_textBoxY(textBoxY)
			, m_fnOnValueChanged(std::move(fnOnValueChanged))
			, m_value(initialValue)
			, m_propertyLabelWeak(propertyLabelWeak)
			, m_hasInteractivePropertyValue(hasInteractivePropertyValue)
			, m_hasParamRef(hasParamRef)
		{
		}

		void update(const std::shared_ptr<Node>&) override
		{
			const double x = ParseOpt<double>(m_textBoxX->text()).value_or(m_value.x);
			const double y = ParseOpt<double>(m_textBoxY->text()).value_or(m_value.y);

			const Vec2 newValue{ x, y };
			if (newValue != m_value)
			{
				m_value = newValue;
				if (m_fnOnValueChanged)
				{
					m_fnOnValueChanged(newValue);
				}
				// ステート値がある状態で編集した場合、即時に黄色下線を消す（パラメータ参照がある場合は保持）
				if (m_hasInteractivePropertyValue && !m_hasParamRef)
				{
					if (const auto label = m_propertyLabelWeak.lock())
					{
						label->setUnderlineStyle(LabelUnderlineStyle::None);
					}
					m_hasInteractivePropertyValue = HasInteractivePropertyValueYN::No;
				}
			}
		}

		void draw(const Node&) const override
		{
		}

		void setValue(const Vec2& value, bool callsOnValueChanged = false)
		{
			m_value = value;

			m_textBoxX->setText(Format(value.x));
			m_textBoxY->setText(Format(value.y));

			if (callsOnValueChanged && m_fnOnValueChanged)
			{
				m_fnOnValueChanged(m_value);
			}
		}

		const Vec2& value() const
		{
			return m_value;
		}
	};
}
