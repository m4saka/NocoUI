#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"
#include "../YN.hpp"

namespace noco
{
	class InputBlocker : public SerializableComponentBase
	{
	public:
		explicit InputBlocker()
			: SerializableComponentBase{ U"InputBlocker", { } }
		{
		}

		void updateInput(const std::shared_ptr<Node>&) override;
	};
}
