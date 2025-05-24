#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"
#include "../Node.hpp"
#include "../Canvas.hpp"

namespace noco
{
	class Placeholder : public SerializableComponentBase
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

		void setTag(StringView tag)
		{
			m_tag.setValue(tag);
		}

		const String& data() const
		{
			return m_data.value();
		}

		void setData(StringView data)
		{
			m_data.setValue(data);
		}
	};
}
