#include "NocoUI/Component/AudioPlayer.hpp"
#include "NocoUI/Asset.hpp"
#include "NocoUI/Canvas.hpp"

namespace noco
{
	namespace
	{
		Audio GetAudio(const String& audioFilePath, const String& audioAssetName)
		{
#ifdef NOCO_EDITOR
			// �G�f�B�^�ł͏��O
			(void)audioAssetName;
#else
			// �C�t���Ȃ������Ƀt�@�C���p�X���g����̂�����邽�߁AAudioAsset�Ɏw�肳�ꂽ�L�[�����݂��Ȃ������t�@�C���p�X�ւ̃t�H�[���o�b�N�͂��Ȃ��d�l�Ƃ���
			if (!audioAssetName.empty())
			{
				return AudioAsset(audioAssetName);
			}
#endif // NOCO_EDITOR
			if (!audioFilePath.empty())
			{
				return noco::Asset::GetOrLoadAudio(audioFilePath);
			}
			return Audio{};
		}
	}

	void AudioPlayer::update(const std::shared_ptr<Node>& node)
	{
		const auto triggerType = m_triggerType.value();
		const bool recursive = m_recursive.value();

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
			if (node->isClicked(RecursiveYN{ recursive }))
			{
				play();
			}
			break;
		case TriggerType::RightClick:
			if (node->isRightClicked(RecursiveYN{ recursive }))
			{
				play();
			}
			break;
		case TriggerType::HoverStart:
			if (recursive)
			{
				if (node->isHovered(RecursiveYN::Yes))
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
				if (node->isHovered())
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
				if (!node->isHovered(RecursiveYN::Yes))
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
				if (!node->isHovered())
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
				if (node->isPressed(RecursiveYN::Yes))
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
				if (node->isPressed())
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
				if (!node->isPressed(RecursiveYN::Yes))
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
				if (!node->isPressed())
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
				if (node->isRightPressed(RecursiveYN::Yes))
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
				if (node->isRightPressed())
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
				if (!node->isRightPressed(RecursiveYN::Yes))
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
				if (!node->isRightPressed())
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
