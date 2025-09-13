#pragma once
#include <NocoUI.hpp>
#include "EditorYN.hpp"

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
		std::function<void(const Color&)> m_fnOnValueChanged;
		Color m_value;
		std::weak_ptr<Label> m_propertyLabelWeak;
		HasInteractivePropertyValueYN m_hasInteractivePropertyValue = HasInteractivePropertyValueYN::No;
		HasParameterRefYN m_hasParamRef = HasParameterRefYN::No;

	public:
		ColorPropertyTextBox(
			const std::shared_ptr<TextBox>& r,
			const std::shared_ptr<TextBox>& g,
			const std::shared_ptr<TextBox>& b,
			const std::shared_ptr<TextBox>& a,
			const std::shared_ptr<RectRenderer>& previewRect,
			std::function<void(const Color&)> fnOnValueChanged,
			const Color& initialValue,
			std::weak_ptr<Label> propertyLabelWeak = {},
			HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No,
			HasParameterRefYN hasParamRef = HasParameterRefYN::No)
			: ComponentBase{ {} }
			, m_textBoxR(r)
			, m_textBoxG(g)
			, m_textBoxB(b)
			, m_textBoxA(a)
			, m_previewRect(previewRect)
			, m_fnOnValueChanged(std::move(fnOnValueChanged))
			, m_value(initialValue)
			, m_propertyLabelWeak(propertyLabelWeak)
			, m_hasInteractivePropertyValue(hasInteractivePropertyValue)
			, m_hasParamRef(hasParamRef)
		{
		}

		void update(const std::shared_ptr<Node>&) override
		{
			const int32 r = Clamp(ParseOpt<int32>(m_textBoxR->text()).value_or(m_value.r), 0, 255);
			const int32 g = Clamp(ParseOpt<int32>(m_textBoxG->text()).value_or(m_value.g), 0, 255);
			const int32 b = Clamp(ParseOpt<int32>(m_textBoxB->text()).value_or(m_value.b), 0, 255);
			const int32 a = Clamp(ParseOpt<int32>(m_textBoxA->text()).value_or(m_value.a), 0, 255);

			const Color newColor{ static_cast<uint8>(r), static_cast<uint8>(g), static_cast<uint8>(b), static_cast<uint8>(a) };
			if (newColor != m_value)
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
				m_value = newColor;
				m_previewRect->setFillColor(newColor);
				if (m_fnOnValueChanged)
				{
					m_fnOnValueChanged(newColor);
				}
			}
		}

		void setValue(const Color& value, bool callsOnValueChanged = false)
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

		const Color& value() const
		{
			return m_value;
		}
	};
}
