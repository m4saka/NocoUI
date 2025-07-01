#pragma once
#include <Siv3D.hpp>
#include <NocoUI.hpp>

namespace noco::editor
{
	class TooltipOpener : public ComponentBase
	{
	private:
		std::shared_ptr<Canvas> m_overlayCanvas;
		std::shared_ptr<Node> m_tooltipNode;
		String m_tooltipText;
		String m_tooltipDetailText;
		double m_hoverTime = 0.0;
		bool m_isShowing = false;
		static constexpr double ShowDelay = 0.5; // ツールチップが表示されるまでの遅延時間(秒)

		void createTooltip();
		void destroyTooltip();
		void updateTooltipSize();

	public:
		explicit TooltipOpener(const std::shared_ptr<Canvas>& overlayCanvas, StringView tooltipText, StringView tooltipDetailText = U"");
		~TooltipOpener();

		void updateInput(const std::shared_ptr<Node>& node) override;
		void setTooltipText(StringView text, StringView detailText = U"");
	};
}
