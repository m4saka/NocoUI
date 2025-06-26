#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"
#include "../Node.hpp"
#include "../Enums.hpp"

namespace noco
{
	class AudioPlayer : public SerializableComponentBase, public std::enable_shared_from_this<AudioPlayer>
	{
	public:
		enum class TriggerType : uint8
		{
			Click,
			RightClick,
			HoverStart,
			HoverEnd,
			PressStart,
			PressEnd,
			RightPressStart,
			RightPressEnd,
		};

	private:
		Property<String> m_audioFilePath;
		Property<String> m_audioAssetName;
		PropertyNonInteractive<TriggerType> m_triggerType;
		Property<double> m_volume;

		/* NonSerialized */ Optional<bool> m_prevHovered = false;
		/* NonSerialized */ Optional<bool> m_prevPressed = none;
		/* NonSerialized */ Optional<bool> m_prevRightPressed = none;

	public:
		AudioPlayer(const PropertyValue<String>& audioFilePath = String{}, const PropertyValue<String>& audioAssetName = String{}, TriggerType triggerType = TriggerType::Click, const PropertyValue<double>& volume = 1.0)
			: SerializableComponentBase{ U"AudioPlayer", { &m_audioFilePath, &m_audioAssetName, &m_triggerType, &m_volume } }
			, m_audioFilePath{ U"audioFilePath", audioFilePath }
			, m_audioAssetName{ U"audioAssetName", audioAssetName }
			, m_triggerType{ U"triggerType", triggerType }
			, m_volume{ U"volume", volume }
		{
		}

		void update(const std::shared_ptr<Node>& node) override;

		const PropertyValue<String>& audioFilePath() const
		{
			return m_audioFilePath.propertyValue();
		}

		std::shared_ptr<AudioPlayer> setAudioFilePath(const PropertyValue<String>& path)
		{
			m_audioFilePath.setPropertyValue(path);
			return shared_from_this();
		}

		const PropertyValue<String>& audioAssetName() const
		{
			return m_audioAssetName.propertyValue();
		}

		std::shared_ptr<AudioPlayer> setAudioAssetName(const PropertyValue<String>& name)
		{
			m_audioAssetName.setPropertyValue(name);
			return shared_from_this();
		}

		TriggerType triggerType() const
		{
			return m_triggerType.value();
		}

		std::shared_ptr<AudioPlayer> setTriggerType(TriggerType type)
		{
			m_triggerType.setValue(type);
			return shared_from_this();
		}

		const PropertyValue<double>& volume() const
		{
			return m_volume.propertyValue();
		}

		std::shared_ptr<AudioPlayer> setVolume(const PropertyValue<double>& value)
		{
			m_volume.setPropertyValue(value);
			return shared_from_this();
		}
	};
}
