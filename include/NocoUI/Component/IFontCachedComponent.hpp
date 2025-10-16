#pragma once

namespace noco::detail
{
	class IFontCachedComponent
	{
	public:
		virtual ~IFontCachedComponent() = default;

		virtual void clearFontCache() = 0;
	};
}
