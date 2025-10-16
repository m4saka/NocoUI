#pragma once
#include <Siv3D.hpp>

namespace noco
{
	namespace detail
	{
		struct GlobalDefaultFontState
		{
			Optional<Font> defaultFont;
			String defaultFontAssetName;

			[[nodiscard]]
			Optional<Font> getFont() const
			{
				if (defaultFont)
				{
					return defaultFont;
				}

				if (!defaultFontAssetName.isEmpty() && FontAsset::IsRegistered(defaultFontAssetName))
				{
					return FontAsset(defaultFontAssetName);
				}

				return none;
			}
		};

		inline GlobalDefaultFontState s_globalDefaultFont;
	}

	inline void SetGlobalDefaultFont(const Font& font)
	{
		detail::s_globalDefaultFont.defaultFont = font;
		detail::s_globalDefaultFont.defaultFontAssetName.clear();
	}

	inline void SetGlobalDefaultFontAssetName(StringView assetName)
	{
		detail::s_globalDefaultFont.defaultFontAssetName = assetName;
		detail::s_globalDefaultFont.defaultFont.reset();
	}

	inline void ClearGlobalDefaultFont()
	{
		detail::s_globalDefaultFont.defaultFont.reset();
		detail::s_globalDefaultFont.defaultFontAssetName.clear();
	}

	namespace detail
	{
		[[nodiscard]]
		inline Optional<Font> GetGlobalDefaultFont()
		{
			return s_globalDefaultFont.getFont();
		}

		[[nodiscard]]
		inline const String& GetGlobalDefaultFontAssetName()
		{
			return s_globalDefaultFont.defaultFontAssetName;
		}
	}
}
