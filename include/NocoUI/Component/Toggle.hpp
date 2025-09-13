#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"
#include "../Property.hpp"

namespace noco
{
	class Toggle : public SerializableComponentBase, public std::enable_shared_from_this<Toggle>
	{
	private:
		PropertyNonInteractive<bool> m_value;
		PropertyNonInteractive<String> m_tag;

	public:
		explicit Toggle(
			bool initialValue = false)
			: SerializableComponentBase{ U"Toggle", { &m_value, &m_tag } }
			, m_value{ U"value", initialValue }
			, m_tag{ U"tag", U"" }
		{
		}

		void update(const std::shared_ptr<Node>& node) override;

		[[nodiscard]]
		bool value() const
		{
			return m_value.value();
		}

		std::shared_ptr<Toggle> setValue(bool value)
		{
			m_value.setValue(value);
			return shared_from_this();
		}

		[[nodiscard]]
		const String& tag() const
		{
			return m_tag.value();
		}

		std::shared_ptr<Toggle> setTag(const String& tag)
		{
			m_tag.setValue(tag);
			return shared_from_this();
		}
	};
}