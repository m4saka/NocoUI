#include "NocoUI/Component/CursorChanger.hpp"

namespace noco
{
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
