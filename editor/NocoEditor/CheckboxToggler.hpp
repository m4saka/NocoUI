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
		std::function<bool()> m_fnGetValue;
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
			HasParameterRefYN hasParamRef = HasParameterRefYN::No,
			std::function<bool()> fnGetValue = nullptr)
			: ComponentBase{ {} }
			, m_value(initialValue)
			, m_fnSetValue(std::move(fnSetValue))
			, m_fnGetValue(std::move(fnGetValue))
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
			// 外部から値が変更された場合の更新
			if (m_fnGetValue)
			{
				const bool externalValue = m_fnGetValue();
				if (externalValue != m_value)
				{
					m_value = externalValue;
					m_checkLabel->setText(m_value ? U"✓" : U"");
				}
			}

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
