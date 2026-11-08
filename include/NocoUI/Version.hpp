#pragma once
#include <Siv3D.hpp>

namespace noco
{
	constexpr StringView NocoUIVersion = U"0.5.2-beta1";

	constexpr int32 CurrentSerializedVersion = 13;

	/// @brief Number型を廃止してInt/Double型を導入したserializedVersion
	constexpr int32 SerializedVersion_IntDoubleParamType = 8;
}
