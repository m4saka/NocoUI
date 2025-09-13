﻿#pragma once
#include <Siv3D.hpp>
#include "../YN.hpp"
#include "ComponentBase.hpp"

namespace noco
{
	enum class HotKeyTarget : uint8
	{
		None,
		Click,
		RightClick,
	};

	class HotKeyInputHandler : public ComponentBase
	{
	private:
		Input m_input;
		CtrlYN m_ctrl;
		AltYN m_alt;
		ShiftYN m_shift;
		HotKeyTarget m_target;
		EnabledWhileTextEditingYN m_enabledWhileTextEditing;
		ClearInputYN m_clearInput;

		[[nodiscard]]
		bool getModifiersPressed() const
		{
			if (m_ctrl.getBool() != KeyControl.pressed())
			{
				return false;
			}
			if (m_alt.getBool() != KeyAlt.pressed())
			{
				return false;
			}
			if (m_shift.getBool() != KeyShift.pressed())
			{
				return false;
			}
			return true;
		}

	public:
		explicit HotKeyInputHandler(
			const Input& input,
			CtrlYN ctrl = CtrlYN::No,
			AltYN alt = AltYN::No,
			ShiftYN shift = ShiftYN::No,
			HotKeyTarget target = HotKeyTarget::Click,
			EnabledWhileTextEditingYN enabledWhileTextEditing = EnabledWhileTextEditingYN::No,
			ClearInputYN clearInput = ClearInputYN::Yes)
			: ComponentBase{ {} }
			, m_input{ input }
			, m_ctrl{ ctrl }
			, m_alt{ alt }
			, m_shift{ shift }
			, m_target{ target }
			, m_enabledWhileTextEditing{ enabledWhileTextEditing }
			, m_clearInput{ clearInput }
		{
		}

		void updateKeyInput(const std::shared_ptr<Node>& node) override
		{
			if (!m_enabledWhileTextEditing && IsEditingTextBox())
			{
				// テキストボックス編集中はキーを無視
				return;
			}

			const bool modifiersPressed = getModifiersPressed();
			const bool inputDown = m_input.down();

			if (m_target == HotKeyTarget::Click && inputDown && modifiersPressed)
			{
				node->requestClick();
				if (m_clearInput)
				{
					m_input.clearInput();
				}
			}

			if (m_target == HotKeyTarget::RightClick && inputDown && modifiersPressed)
			{
				node->requestRightClick();
				if (m_clearInput)
				{
					m_input.clearInput();
				}
			}
		}
	};
}
