#pragma once
#include <Siv3D.hpp>
#include "YN.hpp"
#include "InteractionState.hpp"

namespace noco
{
	class MouseTracker
	{
	private:
		Input m_input;
		InteractableYN m_interactable;
		bool m_mouseOverForHovered = false;
		bool m_mouseOverForPressed = false;
		bool m_isPressed = false;
		bool m_isClicked = false;
		Optional<int32> m_lastUpdateFrame = none;

	public:
		explicit MouseTracker(const Input& input, InteractableYN interactable)
			: m_input(input)
			, m_interactable(interactable)
		{
		}

		// 1フレームに複数回呼び出した場合、最初の呼び出しのみ実行される
		void update(bool mouseOverForHovered, bool mouseOverForPressed, IsScrollingYN isScrolling = IsScrollingYN::No)
		{
			const int32 currentFrame = Scene::FrameCount();
			if (currentFrame == m_lastUpdateFrame)
			{
				return;
			}

			m_mouseOverForHovered = mouseOverForHovered;
			m_mouseOverForPressed = mouseOverForPressed;
			if (m_mouseOverForPressed)
			{
				if (m_input.down() && !m_isPressed)
				{
					// 領域内でクリック開始
					m_isPressed = true;
				}
			}
			else
			{
				if (m_input.down())
				{
					// 領域外でクリックが開始
					m_isPressed = false;
				}
			}

			if (m_input.up())
			{
				if (m_isPressed && m_mouseOverForPressed && !isScrolling)
				{
					// 領域内でクリック終了(スクロール中でない場合のみ)
					m_isClicked = true;
				}
				m_isPressed = false;
			}
			else
			{
				m_isClicked = false;
			}

			m_lastUpdateFrame = currentFrame;
		}

		[[nodiscard]]
		InteractableYN interactable() const
		{
			return m_interactable;
		}

		void setInteractable(InteractableYN interactable)
		{
			m_interactable = interactable;
			if (!interactable)
			{
				m_mouseOverForHovered = false;
				m_mouseOverForPressed = false;
				m_isPressed = false;
				m_isClicked = false;
			}
		}

		[[nodiscard]]
		bool isHovered(IncludingDisabledYN includingDisabled = IncludingDisabledYN::No) const
		{
			if (!m_interactable && !includingDisabled)
			{
				return false;
			}

			if (m_mouseOverForHovered)
			{
				if (m_isPressed)
				{
					return true;
				}
				else if (m_input.pressed())
				{
					// 領域外でクリック開始された場合のマウスオーバーはマウスオーバー扱いにしない
					return false;
				}
				return true;
			}
			return false;
		}

		[[nodiscard]]
		bool isPressed(IncludingDisabledYN includingDisabled = IncludingDisabledYN::No) const
		{
			if (!m_interactable && !includingDisabled)
			{
				return false;
			}
			return m_isPressed;
		}

		[[nodiscard]]
		bool isPressedHover(IncludingDisabledYN includingDisabled = IncludingDisabledYN::No) const
		{
			if (!m_interactable && !includingDisabled)
			{
				return false;
			}
			return m_isPressed && isHovered(includingDisabled);
		}

		[[nodiscard]]
		bool isClicked(IncludingDisabledYN includingDisabled = IncludingDisabledYN::No) const
		{
			if (!m_interactable && !includingDisabled)
			{
				return false;
			}
			return m_isClicked;
		}

		[[nodiscard]]
		InteractionState interactionStateSelf() const
		{
			if (!m_interactable)
			{
				return InteractionState::Disabled;
			}

			if (m_mouseOverForHovered)
			{
				if (m_isPressed)
				{
					return InteractionState::Pressed;
				}
				else if (m_input.pressed())
				{
					// 領域外でクリック開始された場合のマウスオーバーはマウスオーバー扱いにしない
					return InteractionState::Default;
				}
				return InteractionState::Hovered;
			}

			return InteractionState::Default;
		}
	};
}
