#pragma once
#include <Siv3D.hpp>
#include "../YN.hpp"
#include "ComponentBase.hpp"

namespace noco
{
	enum class HotKeyTarget
	{
		None,
		Click,
		RightClick,
	};

	class HotKeyInputHandler : public ComponentBase
	{
	private:
		Input m_input;
		HotKeyTarget m_target;
		EnabledWhileTextEditingYN m_enabledWhileTextEditing;
		ClearsInputYN m_clearsInput;

	public:
		explicit HotKeyInputHandler(
			const Input& input,
			HotKeyTarget target,
			EnabledWhileTextEditingYN enabledWhileTextEditing = EnabledWhileTextEditingYN::No,
			ClearsInputYN clearsInput = ClearsInputYN::Yes)
			: ComponentBase{ {} }
			, m_input{ input }
			, m_target{ target }
			, m_enabledWhileTextEditing{ enabledWhileTextEditing }
			, m_clearsInput{ clearsInput }
		{
		}

		void updateInput(const std::shared_ptr<Node>& node) override
		{
			if (!m_enabledWhileTextEditing && IsEditingTextBox())
			{
				// テキストボックス編集中はキーを無視
				return;
			}

			if (m_target == HotKeyTarget::Click && m_input.down())
			{
				node->requestClick();
			}

			if (m_target == HotKeyTarget::RightClick && m_input.down())
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
