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
			if (node->isClicked())
			{
				play();
			}
			break;
		case TriggerType::RightClick:
			if (node->isRightClicked())
			{
				play();
			}
			break;
		case TriggerType::HoverStart:
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
			break;
		case TriggerType::HoverEnd:
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
			break;
		case TriggerType::PressStart:
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
			break;
		case TriggerType::PressEnd:
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
			break;
		case TriggerType::RightPressStart:
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
			break;
		case TriggerType::RightPressEnd:
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
			break;
		}
	}
}
