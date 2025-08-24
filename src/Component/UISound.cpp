#include "NocoUI/Component/UISound.hpp"
#include "NocoUI/Asset.hpp"
#include "NocoUI/Canvas.hpp"

namespace noco
{
	namespace
	{
		Audio GetAudio(const String& audioFilePath, const String& audioAssetName)
		{
			if (detail::IsEditorMode())
			{
				// エディタモードではアセット名は無視してファイル名のみを使用
				if (!audioFilePath.empty())
				{
					return noco::Asset::GetOrLoadAudio(audioFilePath);
				}
			}
			else
			{
				// 通常モードではアセット名を優先
				// 気付かないうちにファイルパスが使われるのを避けるため、AudioAssetに指定されたキーが存在しない時もファイルパスへのフォールバックはしない仕様とする
				if (!audioAssetName.empty())
				{
					return AudioAsset(audioAssetName);
				}
				if (!audioFilePath.empty())
				{
					return noco::Asset::GetOrLoadAudio(audioFilePath);
				}
			}
			return Audio{};
		}
	}

	void UISound::update(const std::shared_ptr<Node>& node)
	{
		const auto triggerType = m_triggerType.value();
		const bool recursive = m_recursive.value();
		const bool includingDisabled = m_includingDisabled.value();

		auto play = [&]() {
			const String& audioFilePath = m_audioFilePath.value();
			const String& audioAssetName = m_audioAssetName.value();
			Audio audio = GetAudio(audioFilePath, audioAssetName);
			if (audio)
			{
				audio.playOneShot(m_volume.value());
			}
		};

		switch (triggerType)
		{
		case TriggerType::Click:
			if (node->isClicked(RecursiveYN{ recursive }, IncludingDisabledYN{ includingDisabled }))
			{
				play();
			}
			break;
		case TriggerType::RightClick:
			if (node->isRightClicked(RecursiveYN{ recursive }, IncludingDisabledYN{ includingDisabled }))
			{
				play();
			}
			break;
		case TriggerType::HoverStart:
			if (recursive)
			{
				if (node->isHovered(RecursiveYN::Yes, IncludingDisabledYN{ includingDisabled }))
				{
					if (m_prevHoveredRecursive.has_value() && !m_prevHoveredRecursive.value())
					{
						play();
					}
					m_prevHoveredRecursive = true;
				}
				else
				{
					m_prevHoveredRecursive = false;
				}
			}
			else
			{
				if (node->isHovered(RecursiveYN::No, IncludingDisabledYN{ includingDisabled }))
				{
					if (m_prevHovered.has_value() && !m_prevHovered.value())
					{
						play();
					}
					m_prevHovered = true;
				}
				else
				{
					m_prevHovered = false;
				}
			}
			break;
		case TriggerType::HoverEnd:
			if (recursive)
			{
				if (!node->isHovered(RecursiveYN::Yes, IncludingDisabledYN{ includingDisabled }))
				{
					if (m_prevHoveredRecursive.has_value() && m_prevHoveredRecursive.value())
					{
						play();
					}
					m_prevHoveredRecursive = false;
				}
				else
				{
					m_prevHoveredRecursive = true;
				}
			}
			else
			{
				if (!node->isHovered(RecursiveYN::No, IncludingDisabledYN{ includingDisabled }))
				{
					if (m_prevHovered.has_value() && m_prevHovered.value())
					{
						play();
					}
					m_prevHovered = false;
				}
				else
				{
					m_prevHovered = true;
				}
			}
			break;
		case TriggerType::PressStart:
			if (recursive)
			{
				if (node->isPressed(RecursiveYN::Yes, IncludingDisabledYN{ includingDisabled }))
				{
					if (m_prevPressedRecursive.has_value() && !m_prevPressedRecursive.value())
					{
						play();
					}
					m_prevPressedRecursive = true;
				}
				else
				{
					m_prevPressedRecursive = false;
				}
			}
			else
			{
				if (node->isPressed(RecursiveYN::No, IncludingDisabledYN{ includingDisabled }))
				{
					if (m_prevPressed.has_value() && !m_prevPressed.value())
					{
						play();
					}
					m_prevPressed = true;
				}
				else
				{
					m_prevPressed = false;
				}
			}
			break;
		case TriggerType::PressEnd:
			if (recursive)
			{
				if (!node->isPressed(RecursiveYN::Yes, IncludingDisabledYN{ includingDisabled }))
				{
					if (m_prevPressedRecursive.has_value() && m_prevPressedRecursive.value())
					{
						play();
					}
					m_prevPressedRecursive = false;
				}
				else
				{
					m_prevPressedRecursive = true;
				}
			}
			else
			{
				if (!node->isPressed(RecursiveYN::No, IncludingDisabledYN{ includingDisabled }))
				{
					if (m_prevPressed.has_value() && m_prevPressed.value())
					{
						play();
					}
					m_prevPressed = false;
				}
				else
				{
					m_prevPressed = true;
				}
			}
			break;
		case TriggerType::RightPressStart:
			if (recursive)
			{
				if (node->isRightPressed(RecursiveYN::Yes, IncludingDisabledYN{ includingDisabled }))
				{
					if (m_prevRightPressedRecursive.has_value() && !m_prevRightPressedRecursive.value())
					{
						play();
					}
					m_prevRightPressedRecursive = true;
				}
				else
				{
					m_prevRightPressedRecursive = false;
				}
			}
			else
			{
				if (node->isRightPressed(RecursiveYN::No, IncludingDisabledYN{ includingDisabled }))
				{
					if (m_prevRightPressed.has_value() && !m_prevRightPressed.value())
					{
						play();
					}
					m_prevRightPressed = true;
				}
				else
				{
					m_prevRightPressed = false;
				}
			}
			break;
		case TriggerType::RightPressEnd:
			if (recursive)
			{
				if (!node->isRightPressed(RecursiveYN::Yes, IncludingDisabledYN{ includingDisabled }))
				{
					if (m_prevRightPressedRecursive.has_value() && m_prevRightPressedRecursive.value())
					{
						play();
					}
					m_prevRightPressedRecursive = false;
				}
				else
				{
					m_prevRightPressedRecursive = true;
				}
			}
			else
			{
				if (!node->isRightPressed(RecursiveYN::No, IncludingDisabledYN{ includingDisabled }))
				{
					if (m_prevRightPressed.has_value() && m_prevRightPressed.value())
					{
						play();
					}
					m_prevRightPressed = false;
				}
				else
				{
					m_prevRightPressed = true;
				}
			}
			break;
		}
	}
}
