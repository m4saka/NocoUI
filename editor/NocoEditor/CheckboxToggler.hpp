#pragma once
#include <NocoUI.hpp>
#include "EditorYN.hpp"

namespace noco::editor
{
	class CheckboxToggler : public noco::ComponentBase
	{
	private:
		bool m_value;
		std::function<void(bool)> m_fnSetValue;
		std::shared_ptr<Label> m_checkLabel;
		bool m_useParentHoverState;
		std::weak_ptr<Label> m_propertyLabelWeak;
		HasInteractivePropertyValueYN m_hasInteractivePropertyValue = HasInteractivePropertyValueYN::No;
		HasParameterRefYN m_hasParamRef = HasParameterRefYN::No;

	public:
		CheckboxToggler(
			bool initialValue,
			std::function<void(bool)> fnSetValue,
			const std::shared_ptr<Label>& checkLabel,
			bool useParentHoverState,
			std::weak_ptr<Label> propertyLabelWeak = {},
			HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No,
			HasParameterRefYN hasParamRef = HasParameterRefYN::No)
			: ComponentBase{ {} }
			, m_value(initialValue)
			, m_fnSetValue(std::move(fnSetValue))
			, m_checkLabel(checkLabel)
			, m_useParentHoverState(useParentHoverState)
			, m_propertyLabelWeak(propertyLabelWeak)
			, m_hasInteractivePropertyValue(hasInteractivePropertyValue)
			, m_hasParamRef(hasParamRef)
		{
		}

		void setValue(bool value)
		{
			m_value = value;
			m_checkLabel->setText(m_value ? U"✓" : U"");
		}

		bool value() const
		{
			return m_value;
		}

		void update(const std::shared_ptr<Node>& node) override
		{
			// クリックでON/OFFをトグル
			bool isClicked = false;
			if (m_useParentHoverState)
			{
				if (const auto parent = node->findHoverTargetParent())
				{
					isClicked = parent->isClicked();
				}
			}
			else
			{
				isClicked = node->isClicked();
			}
			if (isClicked)
			{
				m_value = !m_value;
				m_checkLabel->setText(m_value ? U"✓" : U"");
				if (m_fnSetValue)
				{
					m_fnSetValue(m_value);
				}
				// ステート値がある状態で編集した場合、即時に黄色下線を消す（パラメータ参照がある場合は保持）
				if (m_hasInteractivePropertyValue && !m_hasParamRef)
				{
					if (const auto pl = m_propertyLabelWeak.lock())
					{
						pl->setUnderlineStyle(LabelUnderlineStyle::None);
					}
					m_hasInteractivePropertyValue = HasInteractivePropertyValueYN::No;
				}
			}
		}
	};
}
