#pragma once
#include <NocoUI.hpp>

namespace noco::editor
{
	class PropertyTextBox : public ComponentBase
	{
	private:
		std::shared_ptr<TextBox> m_textBox;
		std::function<void(StringView)> m_fnSetValue;
		std::function<String()> m_fnGetValue;
		String m_prevExternalValue;

		void update(const std::shared_ptr<Node>&) override
		{
			// 外部からの値の変更をチェック
			if (m_fnGetValue)
			{
				const String currentExternalValue = String(m_fnGetValue());
				if (!m_textBox->isEditing() && currentExternalValue != m_prevExternalValue)
				{
					m_textBox->setText(currentExternalValue, IgnoreIsChangedYN::Yes);
					m_prevExternalValue = currentExternalValue;
				}
			}

			// ユーザーによる変更をチェック
			if (m_textBox->isChanged())
			{
				m_fnSetValue(m_textBox->text());
				if (m_fnGetValue)
				{
					m_prevExternalValue = String{ m_fnGetValue() };
				}
			}
		}

	public:
		explicit PropertyTextBox(const std::shared_ptr<TextBox>& textBox, std::function<void(StringView)> fnSetValue, std::function<String()> fnGetValue = nullptr)
			: ComponentBase{ {} }
			, m_textBox(textBox)
			, m_fnSetValue(std::move(fnSetValue))
			, m_fnGetValue(std::move(fnGetValue))
			, m_prevExternalValue(m_fnGetValue ? String{ m_fnGetValue() } : U"")
		{
		}
	};
}