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

		virtual void onActivated(const std::shared_ptr<Node>&)
		{
		}

		virtual void onDeactivated(const std::shared_ptr<Node>&)
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
		uint64 m_instanceId;

		// ライブラリレベルでのマルチスレッド対応はしないが、atomicにはしておく
		static inline std::atomic<uint64> s_nextInstanceId = 1;

	public:
		explicit SerializableComponentBase(StringView type, const Array<IProperty*>& properties)
			: ComponentBase{ properties }
			, m_type{ type }
			, m_instanceId{ s_nextInstanceId++ }
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
			return toJSONImpl(detail::WithInstanceIdYN::No);
		}

		[[nodiscard]]
		JSON toJSONImpl(detail::WithInstanceIdYN withInstanceId) const
		{
			JSON json;
			json[U"type"] = m_type;
			for (const IProperty* property : properties())
			{
				property->appendJSON(json);
			}
			if (withInstanceId)
			{
				json[U"_instanceId"] = m_instanceId;
			}
			return json;
		}

		bool tryReadFromJSON(const JSON& json)
		{
			return tryReadFromJSONImpl(json, detail::WithInstanceIdYN::No);
		}

		bool tryReadFromJSONImpl(const JSON& json, detail::WithInstanceIdYN withInstanceId)
		{
			if (!json.contains(U"type") || json[U"type"].getString() != m_type)
			{
				return false;
			}
			for (IProperty* property : properties())
			{
				property->readFromJSON(json);
			}
			if (withInstanceId && json.contains(U"_instanceId"))
			{
				m_instanceId = json[U"_instanceId"].get<uint64>();
			}
			return true;
		}

		[[nodiscard]]
		uint64 instanceId() const
		{
			return m_instanceId;
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
