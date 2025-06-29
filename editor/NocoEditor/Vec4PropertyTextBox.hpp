#pragma once
#include <NocoUI.hpp>

namespace noco::editor
{
	class Vec4PropertyTextBox : public ComponentBase
	{
	private:
		std::shared_ptr<TextBox> m_textBoxX;
		std::shared_ptr<TextBox> m_textBoxY;
		std::shared_ptr<TextBox> m_textBoxZ;
		std::shared_ptr<TextBox> m_textBoxW;
		std::function<void(const Vec4&)> m_fnOnValueChanged;
		Vec4 m_value;

	public:
		Vec4PropertyTextBox(
			const std::shared_ptr<TextBox>& textBoxX,
			const std::shared_ptr<TextBox>& textBoxY,
			const std::shared_ptr<TextBox>& textBoxZ,
			const std::shared_ptr<TextBox>& textBoxW,
			std::function<void(const Vec4&)> fnOnValueChanged,
			const Vec4& initialValue)
			: ComponentBase{ {} }
			, m_textBoxX(textBoxX)
			, m_textBoxY(textBoxY)
			, m_textBoxZ(textBoxZ)
			, m_textBoxW(textBoxW)
			, m_fnOnValueChanged(std::move(fnOnValueChanged))
			, m_value(initialValue)
		{
		}

		void update(const std::shared_ptr<Node>&) override
		{
			const double x = ParseOpt<double>(m_textBoxX->text()).value_or(m_value.x);
			const double y = ParseOpt<double>(m_textBoxY->text()).value_or(m_value.y);
			const double z = ParseOpt<double>(m_textBoxZ->text()).value_or(m_value.z);
			const double w = ParseOpt<double>(m_textBoxW->text()).value_or(m_value.w);

			const Vec4 newValue{ x, y, z, w };
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

		void setValue(const Vec4& value, bool callsOnValueChanged = false)
		{
			m_value = value;

			m_textBoxX->setText(Format(value.x));
			m_textBoxY->setText(Format(value.y));
			m_textBoxZ->setText(Format(value.z));
			m_textBoxW->setText(Format(value.w));

			if (callsOnValueChanged && m_fnOnValueChanged)
			{
				m_fnOnValueChanged(m_value);
			}
		}

		const Vec4& value() const
		{
			return m_value;
		}
	};
}
