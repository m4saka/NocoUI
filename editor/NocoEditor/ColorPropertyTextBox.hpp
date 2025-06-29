#pragma once
#include <NocoUI.hpp>

namespace noco::editor
{
	class ColorPropertyTextBox : public ComponentBase
	{
	private:
		std::shared_ptr<TextBox> m_textBoxR;
		std::shared_ptr<TextBox> m_textBoxG;
		std::shared_ptr<TextBox> m_textBoxB;
		std::shared_ptr<TextBox> m_textBoxA;
		std::shared_ptr<RectRenderer> m_previewRect;
		std::function<void(const ColorF&)> m_fnOnValueChanged;
		ColorF m_value;

	public:
		ColorPropertyTextBox(
			const std::shared_ptr<TextBox>& r,
			const std::shared_ptr<TextBox>& g,
			const std::shared_ptr<TextBox>& b,
			const std::shared_ptr<TextBox>& a,
			const std::shared_ptr<RectRenderer>& previewRect,
			std::function<void(const ColorF&)> fnOnValueChanged,
			const ColorF& initialValue)
			: ComponentBase{ {} }
			, m_textBoxR(r)
			, m_textBoxG(g)
			, m_textBoxB(b)
			, m_textBoxA(a)
			, m_previewRect(previewRect)
			, m_fnOnValueChanged(std::move(fnOnValueChanged))
			, m_value(initialValue)
		{
		}

		void update(const std::shared_ptr<Node>&) override
		{
			const double r = Clamp(ParseOpt<double>(m_textBoxR->text()).value_or(m_value.r), 0.0, 1.0);
			const double g = Clamp(ParseOpt<double>(m_textBoxG->text()).value_or(m_value.g), 0.0, 1.0);
			const double b = Clamp(ParseOpt<double>(m_textBoxB->text()).value_or(m_value.b), 0.0, 1.0);
			const double a = Clamp(ParseOpt<double>(m_textBoxA->text()).value_or(m_value.a), 0.0, 1.0);

			const ColorF newColor{ r, g, b, a };
			if (newColor != m_value)
			{
				m_value = newColor;
				if (m_fnOnValueChanged)
				{
					m_fnOnValueChanged(newColor);
				}
				m_previewRect->setFillColor(newColor);
			}
		}

		void setValue(const ColorF& value, bool callsOnValueChanged = false)
		{
			m_value = value;
			
			m_textBoxR->setText(Format(value.r));
			m_textBoxG->setText(Format(value.g));
			m_textBoxB->setText(Format(value.b));
			m_textBoxA->setText(Format(value.a));
			m_previewRect->setFillColor(value);

			if (callsOnValueChanged && m_fnOnValueChanged)
			{
				m_fnOnValueChanged(m_value);
			}
		}

		const ColorF& value() const
		{
			return m_value;
		}
	};
}
