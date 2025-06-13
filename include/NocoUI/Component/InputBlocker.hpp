#pragma once
// #include <Siv3D.hpp> // pch.hppに移動
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
