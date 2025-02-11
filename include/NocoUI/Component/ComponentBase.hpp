#pragma once
#include <Siv3D.hpp>
#include "../YN.hpp"
#include "../Property.hpp"
#include "../PropertyValue.hpp"

namespace noco
{
	class Node;

	struct CanvasUpdateContext;

	class ComponentBase
	{
	private:
		String m_type;
		Array<IProperty*> m_properties;

	public:
		explicit ComponentBase(StringView type, const Array<IProperty*>& properties)
			: m_type{ type }
			, m_properties{ properties }
		{
		}
		virtual ~ComponentBase() = 0;

		virtual void onActivated(CanvasUpdateContext*, const std::shared_ptr<Node>&)
		{
		}

		virtual void onDeactivated(CanvasUpdateContext*, const std::shared_ptr<Node>&)
		{
		}

		virtual void update(CanvasUpdateContext*, const std::shared_ptr<Node>&)
		{
		}

		virtual void updateInactive(CanvasUpdateContext*, const std::shared_ptr<Node>&)
		{
		}

		virtual void draw(const Node&) const
		{
		}

		[[nodiscard]]
		JSON toJSON() const
		{
			JSON json;
			json[U"type"] = m_type;
			for (const auto property : m_properties)
			{
				property->appendJSON(json);
			}
			return json;
		}

		bool tryReadFromJSON(const JSON& json)
		{
			if (!json.contains(U"type") || json[U"type"].getString() != m_type)
			{
				return false;
			}
			for (auto* property : m_properties)
			{
				property->readFromJSON(json);
			}
			return true;
		}

		void updateProperties(InteractState interactState, SelectedYN selected, double deltaTime)
		{
			for (auto* property : m_properties)
			{
				property->update(interactState, selected, deltaTime);
			}
		}

		[[nodiscard]]
		const String& type() const
		{
			return m_type;
		}

		[[nodiscard]]
		const Array<IProperty*>& properties() const
		{
			return m_properties;
		}
	};

	inline ComponentBase::~ComponentBase() = default;
}
