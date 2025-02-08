#include "NocoUI/Component/Sprite.hpp"
#include "NocoUI/Node.hpp"

namespace noco
{
	void Sprite::draw(const Node& node) const
	{
		const RectF& rect = node.rect();
		const String& assetName = m_assetName.value();
		const ColorF& color = m_color.value();
		if (m_preserveAspect.value())
		{
			TextureAsset(assetName).fitted(rect.size).drawAt(rect.center(), color);
		}
		else
		{
			TextureAsset(assetName).resized(rect.size).draw(rect.pos, color);
		}
	}
}
