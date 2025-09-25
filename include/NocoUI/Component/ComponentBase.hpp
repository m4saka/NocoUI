#pragma once
#include <Siv3D.hpp>
#include "../YN.hpp"
#include "../Property.hpp"
#include "../PropertyValue.hpp"

namespace noco
{
	class Node;

	struct CanvasUpdateContext;

	namespace detail
	{
		using WithInstanceIdYN = YesNo<struct WithInstanceIdYN_tag>;
	}

	class ComponentBase
	{
	private:
		Array<IProperty*> m_properties;

	protected:
		void setProperties(const Array<IProperty*>& properties)
		{
			m_properties = properties;
		}

	public:
		explicit ComponentBase(const Array<IProperty*>& properties)
			: m_properties{ properties }
		{
		}

		virtual ~ComponentBase() = 0;

		virtual void updateKeyInput(const std::shared_ptr<Node>&)
		{
		}

		virtual void update(const std::shared_ptr<Node>&)
		{
		}

		virtual void lateUpdate(const std::shared_ptr<Node>&)
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

		void updateProperties(InteractionState interactionState, const Array<String>& activeStyleStates, double deltaTime, const HashTable<String, ParamValue>& params, SkipSmoothingYN skipSmoothing);

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

		void clearCurrentFrameOverride()
		{
			for (IProperty* property : m_properties)
			{
				property->clearCurrentFrameOverride();
			}
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

	protected:
		[[nodiscard]]
		virtual String typeOverrideInternal() const
		{
			return m_type;
		}

		[[nodiscard]]
		virtual JSON toJSONOverrideInternal(detail::WithInstanceIdYN withInstanceId) const
		{
			JSON json;
			json[U"type"] = typeOverrideInternal();
			
			for (const IProperty* property : properties())
			{
				property->appendJSON(json);
			}
			
			if (withInstanceId)
			{
				json[U"_instanceId"] = instanceId();
			}
			
			return json;
		}

		virtual bool tryReadFromJSONOverrideInternal(const JSON& json, detail::WithInstanceIdYN withInstanceId)
		{
			if (!json.contains(U"type") || json[U"type"].getString() != typeOverrideInternal())
			{
				return false;
			}
			
			for (IProperty* property : properties())
			{
				property->readFromJSON(json);
			}
			
			if (withInstanceId && json.contains(U"_instanceId"))
			{
				setInstanceId(json[U"_instanceId"].get<uint64>());
				s_nextInstanceId = Max(s_nextInstanceId.load(), m_instanceId + 1);
			}
			
			return true;
		}

	public:
		explicit SerializableComponentBase(StringView type, const Array<IProperty*>& properties)
			: ComponentBase{ properties }
			, m_type{ type }
			, m_instanceId{ s_nextInstanceId++ }
		{
		}

		virtual ~SerializableComponentBase() = 0;

		[[nodiscard]]
		String type() const
		{
			return typeOverrideInternal();
		}

		[[nodiscard]]
		uint64 instanceId() const
		{
			return m_instanceId;
		}

		void setInstanceId(uint64 id)
		{
			m_instanceId = id;
		}

		[[nodiscard]]
		JSON toJSON(detail::WithInstanceIdYN withInstanceId = detail::WithInstanceIdYN::No) const
		{
			return toJSONOverrideInternal(withInstanceId);
		}

		bool tryReadFromJSON(const JSON& json, detail::WithInstanceIdYN withInstanceId = detail::WithInstanceIdYN::No)
		{
			return tryReadFromJSONOverrideInternal(json, withInstanceId);
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
