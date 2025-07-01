#pragma once
#include <NocoUI.hpp>

namespace noco::editor
{
	class CheckboxToggler : public noco::ComponentBase
	{
	private:
		bool m_value;
		std::function<void(bool)> m_fnSetValue;
		std::shared_ptr<Label> m_checkLabel;
		bool m_useParentHoverState;

	public:
		CheckboxToggler(bool initialValue,
			std::function<void(bool)> fnSetValue,
			const std::shared_ptr<Label>& checkLabel,
			bool useParentHoverState)
			: ComponentBase{ {} }
			, m_value(initialValue)
			, m_fnSetValue(std::move(fnSetValue))
			, m_checkLabel(checkLabel)
			, m_useParentHoverState(useParentHoverState)
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
			}
		}
	};
}
