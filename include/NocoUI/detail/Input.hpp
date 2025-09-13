#pragma once
#include <Siv3D.hpp>

namespace noco::detail
{
#ifdef __APPLE__
	// macOS: Commandキーを使用
	inline constexpr Input KeyCommandControl{ InputDeviceType::Keyboard, 0xD8 };
#else
	// Windows/Linux: Controlキーを使用
	inline constexpr Input KeyCommandControl{ InputDeviceType::Keyboard, 0x11 };
#endif
}
