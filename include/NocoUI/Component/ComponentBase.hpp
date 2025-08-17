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

		virtual void updateKeyInput(const std::shared_ptr<Node>&)
		{
		}

		virtual void updateKeyInputInactive(const std::shared_ptr<Node>&)
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

		void updateProperties(InteractionState interactionState, const Array<String>& activeStyleStates, double deltaTime, const HashTable<String, ParamValue>& params);

		[[nodiscard]]
		const Array<IProperty*>& properties() const
		{
			return m_properties;
		}


		[[nodiscard]]
		IProperty* getPropertyByName(StringView name) const
		{
			for (IProperty* property : m_properties)
			{
				if (property->name() == name)
				{
					return property;
				}
			}
			return nullptr;
		}
	};

	inline ComponentBase::~ComponentBase() = default;

	class SerializableComponentBase : public ComponentBase
	{
	private:
		String m_type;
		uint64 m_internalId;

		// ライブラリレベルでのマルチスレッド対応はしないが、atomicにはしておく
		static inline std::atomic<uint64> s_nextInternalId = 1;

	public:
		explicit SerializableComponentBase(StringView type, const Array<IProperty*>& properties)
			: ComponentBase{ properties }
			, m_type{ type }
			, m_internalId{ s_nextInternalId++ }
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
			return toJSONImpl(detail::IncludesInternalIdYN::No);
		}

		[[nodiscard]]
		JSON toJSONImpl(detail::IncludesInternalIdYN includesInternalId) const
		{
			JSON json;
			json[U"type"] = m_type;
			for (const IProperty* property : properties())
			{
				property->appendJSON(json);
			}
			if (includesInternalId)
			{
				json[U"_internalId"] = m_internalId;
			}
			return json;
		}

		bool tryReadFromJSON(const JSON& json)
		{
			return tryReadFromJSONImpl(json, detail::IncludesInternalIdYN::No);
		}

		bool tryReadFromJSONImpl(const JSON& json, detail::IncludesInternalIdYN includesInternalId)
		{
			if (!json.contains(U"type") || json[U"type"].getString() != m_type)
			{
				return false;
			}
			for (IProperty* property : properties())
			{
				property->readFromJSON(json);
			}
			if (includesInternalId && json.contains(U"_internalId"))
			{
				m_internalId = json[U"_internalId"].get<uint64>();
			}
			return true;
		}

		[[nodiscard]]
		uint64 internalId() const
		{
			return m_internalId;
		}

		void replaceParamRefs(StringView oldName, StringView newName)
		{
			for (IProperty* property : properties())
			{
				if (property->paramRef() == oldName)
				{
					property->setParamRef(String{ newName });
				}
			}
		}
	};

	inline SerializableComponentBase::~SerializableComponentBase() = default;
}
