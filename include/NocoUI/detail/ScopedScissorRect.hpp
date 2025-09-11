#pragma once
#include <Siv3D.hpp>

namespace noco::detail
{
	class ScopedScissorRect
	{
	private:
		// ライブラリレベルでのマルチスレッド対応はしないが、atomicにはしておく
		static inline std::atomic<size_t> s_nestLevel = 0;
		Rect m_prevScissorRect;
		ScopedRenderStates2D m_renderStates;

	public:
		explicit ScopedScissorRect(const Rect& rect)
			: m_prevScissorRect(Graphics2D::GetScissorRect())
			, m_renderStates(RasterizerState::SolidCullNoneScissor)
		{
			if (s_nestLevel == 0) // ライブラリ外部で設定されたScissorRectとのネストはここでは考慮しないことにする
			{
				Graphics2D::SetScissorRect(rect);
			}
			else
			{
				Graphics2D::SetScissorRect(rect.getOverlap(m_prevScissorRect));
			}
			++s_nestLevel;
		}

		~ScopedScissorRect()
		{
			Graphics2D::SetScissorRect(m_prevScissorRect);
			--s_nestLevel;
		}
	};
}
