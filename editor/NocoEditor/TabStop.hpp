#pragma once
#include <Siv3D.hpp>
#include <NocoUI/Component/ComponentBase.hpp>

namespace nocoeditor
{
	// タブキーでのフォーカス移動を可能にするコンポーネント（NocoEditor専用、非シリアライズ）
	class TabStop : public noco::ComponentBase
	{
	public:
		TabStop()
			: ComponentBase{ U"TabStop" }
		{
		}

		void update(const std::shared_ptr<noco::Node>& node) override;
	};
}