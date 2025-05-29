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
		IgnoresWhileTextEditingYN m_ignoresWhileTextEditing;

	public:
		explicit ShortcutInputHandler(
			const Input& input,
			ShortcutInputTarget target,
			ClearsInputYN clearsInput = ClearsInputYN::Yes,
			IgnoresWhileTextEditingYN ignoresWhileTextEditing = IgnoresWhileTextEditingYN::Yes)
			: ComponentBase{ {} }
			, m_input{ input }
			, m_target{ target }
			, m_clearsInput{ clearsInput }
			, m_ignoresWhileTextEditing{ ignoresWhileTextEditing }
		{
		}

		void updateInput(CanvasUpdateContext* pContext, const std::shared_ptr<Node>& node) override
		{
			if (m_ignoresWhileTextEditing && pContext && pContext->editingTextBox.lock()) // TODO: ←updateInputがupdateより先なので、これだとテキスト編集中か判定できない…
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
