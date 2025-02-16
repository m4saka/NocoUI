#include "NocoUI/Component/Sprite.hpp"
#include "NocoUI/Node.hpp"

namespace noco
{
	void Sprite::draw(const Node& node) const
	{
		const RectF& rect = node.rect();
		const String& textureAssetName = m_textureAssetName.value();
		const ColorF& color = m_color.value();
		if (m_preserveAspect.value())
		{
			TextureAsset(textureAssetName).fitted(rect.size).drawAt(rect.center(), color);
		}
		else
		{
			TextureAsset(textureAssetName).resized(rect.size).draw(rect.pos, color);
		}
	}
}
