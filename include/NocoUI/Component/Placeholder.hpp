#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"

namespace noco
{
	class Placeholder : public SerializableComponentBase, public std::enable_shared_from_this<Placeholder>
	{
	private:
		PropertyNonInteractive<String> m_tag;
		PropertyNonInteractive<String> m_data;

	public:
		explicit Placeholder(StringView tag = U"", StringView data = U"")
			: SerializableComponentBase{ U"Placeholder", { &m_tag, &m_data } }
			, m_tag{ U"tag", tag }
			, m_data{ U"data", data }
		{
		}

		const String& tag() const
		{
			return m_tag.value();
		}

		std::shared_ptr<Placeholder> setTag(StringView tag)
		{
			m_tag.setValue(tag);
			return shared_from_this();
		}

		const String& data() const
		{
			return m_data.value();
		}

		std::shared_ptr<Placeholder> setData(StringView data)
		{
			m_data.setValue(data);
			return shared_from_this();
		}
	};
}
