#pragma once
#include <NocoUI.hpp>

namespace noco::editor
{
	class Vec2PropertyTextBox : public ComponentBase
	{
	private:
		std::shared_ptr<TextBox> m_textBoxX;
		std::shared_ptr<TextBox> m_textBoxY;
		std::function<void(const Vec2&)> m_fnOnValueChanged;
		Vec2 m_value;

	public:
		Vec2PropertyTextBox(
			const std::shared_ptr<TextBox>& textBoxX,
			const std::shared_ptr<TextBox>& textBoxY,
			std::function<void(const Vec2&)> fnOnValueChanged,
			const Vec2& initialValue)
			: ComponentBase{ {} }
			, m_textBoxX(textBoxX)
			, m_textBoxY(textBoxY)
			, m_fnOnValueChanged(std::move(fnOnValueChanged))
			, m_value(initialValue)
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
