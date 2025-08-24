#pragma once
#include <Siv3D.hpp>
#include <NocoUI/Property.hpp>

namespace noco::editor
{
	struct PropertySchema
	{
		String name;
		String displayName;
		PropertyEditType editType;
		String defaultValue;
		String tooltip;
		String tooltipDetail;
		Array<String> enumCandidates;
		Optional<int32> numTextAreaLines;
		Optional<double> dragValueChangeStep;
		bool refreshInspectorOnChange = false;
	};

	struct ComponentSchema
	{
		String type;
		Array<PropertySchema> properties;
		/* NonSerialized */ Optional<Texture> thumbnailTexture;

		[[nodiscard]]
		Optional<PropertySchema> findProperty(const String& name) const
		{
			for (const auto& prop : properties)
			{
				if (prop.name == name)
				{
					return prop;
				}
			}
			return none;
		}
	};
}
