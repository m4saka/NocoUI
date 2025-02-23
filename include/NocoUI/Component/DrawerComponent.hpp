#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"

namespace noco
{
	class DrawerComponent : public ComponentBase
	{
	private:
		std::function<void(const Node&)> m_function;

	public:
		explicit DrawerComponent(std::function<void(const Node&)> function)
			: ComponentBase{ {} }
			, m_function{ std::move(function) }
		{
		}

		void draw(const Node& node) const override
		{
			if (m_function)
			{
				m_function(node);
			}
		}
	};
}
