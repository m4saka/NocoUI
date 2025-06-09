#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"

namespace noco
{
	template <typename TData>
	class DataStore : public ComponentBase, public std::enable_shared_from_this<DataStore<TData>>
	{
	private:
		TData m_value;

	public:
		DataStore() requires std::is_default_constructible_v<TData>
			: ComponentBase{ {} }
		{
		}

		explicit DataStore(const TData& value)
			: ComponentBase{ {} }
			, m_value(value)
		{
		}

		explicit DataStore(TData&& value)
			: ComponentBase{ {} }
			, m_value(std::move(value))
		{
		}

		TData& value()
		{
			return m_value;
		}

		const TData& value() const
		{
			return m_value;
		}

		std::shared_ptr<DataStore> setValue(const TData& value)
		{
			m_value = value;
			return this->shared_from_this();
		}

		std::shared_ptr<DataStore> setValue(TData&& data)
		{
			m_value = std::move(data);
			return this->shared_from_this();
		}
	};
}
