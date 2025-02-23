#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"

namespace noco
{
	class UpdaterComponent : public ComponentBase
	{
	private:
		std::function<void(const std::shared_ptr<Node>&)> m_function;

	public:
		explicit UpdaterComponent(std::function<void(const std::shared_ptr<Node>&)> function)
			: ComponentBase{ {} }
			, m_function{ std::move(function) }
		{
		}

		void update(CanvasUpdateContext*, const std::shared_ptr<Node>& node) override
		{
			if (m_function)
			{
				m_function(node);
			}
		}
	};
}
