#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"
#include "../Property.hpp"

namespace noco
{
	class Canvas;

	/// @brief 入れ子でCanvas利用できるようにするためのコンポーネント
	class SubCanvas : public SerializableComponentBase, public std::enable_shared_from_this<SubCanvas>
	{
	private:
		Property<String> m_canvasPath;
		Property<bool> m_propagateEvents;
		PropertyNonInteractive<String> m_paramsJSON;
		PropertyNonInteractive<String> m_tag;

		/* NonSerialized */ std::shared_ptr<Canvas> m_canvas;
		/* NonSerialized */ String m_loadedPath;
		/* NonSerialized */ String m_appliedParamsJSON;

		/// @brief Canvasファイルを読み込む
		void loadCanvasInternal();

		bool tryReadFromJSONOverrideInternal(const JSON& json, detail::WithInstanceIdYN withInstanceId) override
		{
			if (!SerializableComponentBase::tryReadFromJSONOverrideInternal(json, withInstanceId))
			{
				return false;
			}
			loadCanvasInternal();
			return true;
		}

	public:
		explicit SubCanvas(const PropertyValue<String>& canvasPath = U"", const PropertyValue<bool>& propagateEvents = true, StringView paramsJSON = U"{}", StringView tag = U"")
			: SerializableComponentBase{ U"SubCanvas", { &m_canvasPath, &m_propagateEvents, &m_paramsJSON, &m_tag } }
			, m_canvasPath{ U"canvasPath", canvasPath }
			, m_propagateEvents{ U"propagateEvents", propagateEvents }
			, m_paramsJSON{ U"paramsJSON", paramsJSON }
			, m_tag{ U"tag", tag }
		{
			loadCanvasInternal();
		}

		void update(const std::shared_ptr<Node>& node) override;

		void draw(const Node& node) const override;

		/// @brief Canvasファイルを再読み込み
		void reloadCanvasFile();

		[[nodiscard]]
		const PropertyValue<String>& canvasPath() const
		{
			return m_canvasPath.propertyValue();
		}

		std::shared_ptr<SubCanvas> setCanvasPath(const PropertyValue<String>& canvasPath)
		{
			m_canvasPath.setPropertyValue(canvasPath);
			loadCanvasInternal();
			return shared_from_this();
		}

		[[nodiscard]]
		const PropertyValue<bool>& propagateEvents() const
		{
			return m_propagateEvents.propertyValue();
		}

		std::shared_ptr<SubCanvas> setPropagateEvents(const PropertyValue<bool>& propagateEvents)
		{
			m_propagateEvents.setPropertyValue(propagateEvents);
			return shared_from_this();
		}

		[[nodiscard]]
		const String& paramsJSON() const
		{
			return m_paramsJSON.value();
		}

		std::shared_ptr<SubCanvas> setParamsJSON(const String& paramsJSON)
		{
			m_paramsJSON.setValue(paramsJSON);
			return shared_from_this();
		}

		[[nodiscard]]
		const String& tag() const
		{
			return m_tag.value();
		}

		std::shared_ptr<SubCanvas> setTag(const String& tag)
		{
			m_tag.setValue(tag);
			return shared_from_this();
		}

		/// @brief 内部のCanvasにアクセス
		/// @return Canvasのshared_ptr(読み込まれていない場合はnullptr)
		[[nodiscard]]
		std::shared_ptr<Canvas> canvas() const
		{
			return m_canvas;
		}

		/// @brief 読み込まれたCanvasのパスを取得
		/// @return 読み込まれたパス(読み込まれていない場合は空文字列)
		[[nodiscard]]
		const String& loadedPath() const
		{
			return m_loadedPath;
		}
	};
}
