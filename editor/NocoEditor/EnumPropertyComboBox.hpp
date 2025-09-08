#pragma once
#include <NocoUI.hpp>
#include "EditorYN.hpp"

namespace noco::editor
{
	class EnumPropertyComboBox : public ComponentBase
	{
	private:
		String m_value;
		std::function<void(StringView)> m_fnOnValueChanged;
		std::shared_ptr<Label> m_label;
		std::shared_ptr<ContextMenu> m_contextMenu;
		Array<String> m_enumCandidates;
		std::weak_ptr<Label> m_propertyLabelWeak;
		HasInteractivePropertyValueYN m_hasInteractivePropertyValue = HasInteractivePropertyValueYN::No;
		HasParameterRefYN m_hasParamRef = HasParameterRefYN::No;

	public:
		EnumPropertyComboBox(
			StringView initialValue,
			std::function<void(StringView)> onValueChanged,
			const std::shared_ptr<Label>& label,
			const std::shared_ptr<ContextMenu>& contextMenu,
			const Array<String>& enumCandidates,
			std::weak_ptr<Label> propertyLabelWeak = {},
			HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No,
			HasParameterRefYN hasParamRef = HasParameterRefYN::No)
			: ComponentBase{ {} }
			, m_value(initialValue)
			, m_fnOnValueChanged(std::move(onValueChanged))
			, m_label(label)
			, m_contextMenu(contextMenu)
			, m_enumCandidates(enumCandidates)
			, m_propertyLabelWeak(propertyLabelWeak)
			, m_hasInteractivePropertyValue(hasInteractivePropertyValue)
			, m_hasParamRef(hasParamRef)
		{
		}

		void update(const std::shared_ptr<Node>& node) override
		{
			if (node->isClicked())
			{
				Array<MenuElement> menuElements;
				for (const auto& name : m_enumCandidates)
				{
					menuElements.push_back(
						MenuItem
						{
							name,
							U"",
							none,
							[this, name]
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
								m_value = name;
								m_label->setText(name);
								if (m_fnOnValueChanged)
								{
									m_fnOnValueChanged(m_value);
								}
							}
						});
				}
				m_contextMenu->show(node->regionRect().bl(), menuElements);
			}
		}

		void setValue(StringView value, bool callsOnValueChanged = false)
		{
			m_value = value;
			m_label->setText(m_value);
			if (callsOnValueChanged && m_fnOnValueChanged)
			{
				m_fnOnValueChanged(m_value);
			}
		}

		const String& value() const
		{
			return m_value;
		}
	};
}
