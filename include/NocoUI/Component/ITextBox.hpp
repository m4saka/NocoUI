#pragma once
#include <Siv3D.hpp>
#include "IFocusable.hpp"

namespace noco
{
	class ITextBox : public IFocusable
	{
	public:
		virtual ~ITextBox() = default;

		[[nodiscard]]
		virtual bool isEditing() const = 0;

		[[nodiscard]]
		virtual bool isChanged() const = 0;

		[[nodiscard]]
		virtual const String& text() const = 0;
	};
}
