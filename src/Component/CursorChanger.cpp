#include "NocoUI/Component/CursorChanger.hpp"

namespace noco
{
	CursorChanger::CursorChanger(CursorStyle cursorStyle, RecursiveYN recursive)
		: SerializableComponentBase{ U"CursorChanger", { &m_cursorStyle, &m_recursive, &m_includingDisabled } }
		, m_cursorStyle{ U"cursorStyle", cursorStyle }
		, m_recursive{ U"recursive", recursive.getBool() }
		, m_includingDisabled{ U"includingDisabled", false }
	{
	}

	void CursorChanger::update(const std::shared_ptr<Node>& node)
	{
		const bool recursive = m_recursive.value();
		const bool includingDisabled = m_includingDisabled.value();
		if (node->isHovered(RecursiveYN{ recursive }, IncludingDisabledYN{ includingDisabled }))
		{
			const CursorStyle style = m_cursorStyle.value();
			Cursor::RequestStyle(style);
		}
	}
}
