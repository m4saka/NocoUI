#pragma once
#include <Siv3D.hpp>

namespace noco
{
	constexpr StringView NocoUIVersion = U"0.4.3";

	constexpr int32 CurrentSerializedVersion = 10;

	/// @brief Number型を廃止してInt/Double型を導入したserializedVersion
	constexpr int32 SerializedVersion_IntDoubleParamType = 8;
}
