#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"
#include "../Node.hpp"
#include "../Canvas.hpp"
#include "../YN.hpp"

namespace noco
{
	class CursorChanger : public SerializableComponentBase
	{
	private:
		Property<CursorStyle> m_cursorStyle;
		PropertyNonInteractive<bool> m_recursive;
		PropertyNonInteractive<bool> m_includingDisabled;

	public:
		explicit CursorChanger(CursorStyle cursorStyle = CursorStyle::Hand, RecursiveYN recursive = RecursiveYN::No)
			: SerializableComponentBase{ U"CursorChanger", { &m_cursorStyle, &m_recursive, &m_includingDisabled } }
			, m_cursorStyle{ U"cursorStyle", cursorStyle }
			, m_recursive{ U"recursive", recursive.getBool() }
			, m_includingDisabled{ U"includingDisabled", false }
		{
		}

		void update(const std::shared_ptr<Node>& node) override;
	};
}
