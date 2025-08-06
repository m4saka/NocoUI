#pragma once
#include <NocoUI.hpp>

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

	public:
		EnumPropertyComboBox(
			StringView initialValue,
			std::function<void(StringView)> onValueChanged,
			const std::shared_ptr<Label>& label,
			const std::shared_ptr<ContextMenu>& contextMenu,
			const Array<String>& enumCandidates)
			: ComponentBase{ {} }
			, m_value(initialValue)
			, m_fnOnValueChanged(std::move(onValueChanged))
			, m_label(label)
			, m_contextMenu(contextMenu)
			, m_enumCandidates(enumCandidates)
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
								m_value = name;
								m_label->setText(name);
								m_fnOnValueChanged(m_value);
							}
						});
				}
				m_contextMenu->show(node->layoutAppliedRect().bl(), menuElements);
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
