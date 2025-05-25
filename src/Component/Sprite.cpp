#include "NocoUI/Component/Sprite.hpp"
#include "NocoUI/Node.hpp"
#include "NocoUI/Asset.hpp"

namespace noco
{
	namespace
	{
		String FormatTextureAssetName(const String& textureFilePath)
		{
			return U"noco::{}"_fmt(textureFilePath);
		}

		Texture GetTexture(const String& textureFilePath, const String& textureAssetName)
		{
			if (!textureAssetName.empty() && TextureAsset::IsRegistered(textureAssetName))
			{
				return TextureAsset(textureAssetName);
			}
			else if (!textureFilePath.empty())
			{
				return noco::Asset::GetOrLoadTexture(textureFilePath);
			}
			else
			{
				return Texture{};
			}
		}
	}

	void Sprite::draw(const Node& node) const
	{
		const String& textureFilePath = m_textureFilePath.value();
		const String& textureAssetName = m_textureAssetName.value();
		const Texture texture = GetTexture(textureFilePath, textureAssetName);
		const RectF& rect = node.rect();
		const ColorF& color = m_color.value();
		if (m_preserveAspect.value())
		{
			texture.fitted(rect.size).drawAt(rect.center(), color);
		}
		else
		{
			texture.resized(rect.size).draw(rect.pos, color);
		}
	}
}
