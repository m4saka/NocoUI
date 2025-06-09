#include "NocoUI/Component/CursorChanger.hpp"

namespace noco
{
	// 手前のものを後で実行する必要があるため、updateInputではなくupdateで実行
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
