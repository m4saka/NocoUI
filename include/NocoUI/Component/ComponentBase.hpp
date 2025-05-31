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
		Array<IProperty*> m_properties;

	public:
		explicit ComponentBase(const Array<IProperty*>& properties)
			: m_properties{ properties }
		{
		}

		virtual ~ComponentBase() = 0;

		virtual void updateInput(const std::shared_ptr<Node>&)
		{
		}

		virtual void updateInputInactive(const std::shared_ptr<Node>&)
		{
		}

		virtual void update(const std::shared_ptr<Node>&)
		{
		}

		virtual void updateInactive(const std::shared_ptr<Node>&)
		{
		}

		virtual void lateUpdate(const std::shared_ptr<Node>&)
		{
		}

		virtual void lateUpdateInactive(const std::shared_ptr<Node>&)
		{
		}

		virtual void draw(const Node&) const
		{
		}

		void updateProperties(InteractState interactState, SelectedYN selected, double deltaTime)
		{
			for (IProperty* property : m_properties)
			{
				property->update(interactState, selected, deltaTime);
			}
		}

		[[nodiscard]]
		const Array<IProperty*>& properties() const
		{
			return m_properties;
		}
	};

	inline ComponentBase::~ComponentBase() = default;

	class SerializableComponentBase : public ComponentBase
	{
	private:
		String m_type;

	public:
		explicit SerializableComponentBase(StringView type, const Array<IProperty*>& properties)
			: ComponentBase{ properties }
			, m_type{ type }
		{
		}

		virtual ~SerializableComponentBase() = 0;

		[[nodiscard]]
		const String& type() const
		{
			return m_type;
		}

		[[nodiscard]]
		JSON toJSON() const
		{
			JSON json;
			json[U"type"] = m_type;
			for (const IProperty* property : properties())
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
			for (IProperty* property : properties())
			{
				property->readFromJSON(json);
			}
			return true;
		}
	};

	inline SerializableComponentBase::~SerializableComponentBase() = default;
}
