#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"

namespace noco
{
	class IFocusable
	{
	public:
		virtual ~IFocusable() = default;

		virtual bool isTabStopEnabled() const = 0;

		virtual bool isFocused() const = 0;

		virtual void setFocused(bool focused) = 0;
	};
}