#pragma once
#include <Siv3D.hpp>
#include "../YN.hpp"
#include "ComponentBase.hpp"

namespace noco
{
	enum class ShortcutInputTarget
	{
		None,
		Click,
		RightClick,
	};

	class ShortcutInputHandler : public ComponentBase
	{
	private:
		Input m_input;
		ShortcutInputTarget m_target;
		ClearsInputYN m_clearsInput;
		EnabledWhileTextEditingYN m_enabledWhileTextEditing;

	public:
		explicit ShortcutInputHandler(
			const Input& input,
			ShortcutInputTarget target,
			ClearsInputYN clearsInput = ClearsInputYN::Yes,
			EnabledWhileTextEditingYN enabledWhileTextEditing = EnabledWhileTextEditingYN::No)
			: ComponentBase{ {} }
			, m_input{ input }
			, m_target{ target }
			, m_clearsInput{ clearsInput }
			, m_enabledWhileTextEditing{ enabledWhileTextEditing }
		{
		}

		void updateInput(const std::shared_ptr<Node>& node) override
		{
			if (!m_enabledWhileTextEditing && IsEditingTextBox())
			{
				// テキストボックス編集中はキーを無視
				return;
			}

			if (m_target == ShortcutInputTarget::Click && m_input.down())
			{
				node->requestClick();
			}

			if (m_target == ShortcutInputTarget::RightClick && m_input.down())
			{
				node->requestRightClick();
			}

			// down以外もクリアするため常に実行
			if (m_clearsInput)
			{
				m_input.clearInput();
			}
		}
	};
}
