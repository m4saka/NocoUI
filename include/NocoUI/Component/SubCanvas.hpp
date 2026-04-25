#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"
#include "../YN.hpp"
#include "../Param.hpp"
#include "../Property.hpp"

namespace noco
{
	class Canvas;
	class Node;

	/// @brief 入れ子でCanvas利用できるようにするためのコンポーネント
	class SubCanvas : public SerializableComponentBase, public std::enable_shared_from_this<SubCanvas>
	{
	private:
		Property<String> m_canvasPath;
		Property<bool> m_propagateEvents;
		PropertyNonInteractive<String> m_serializedParamsJSON;
		PropertyNonInteractive<String> m_serializedParamBindingsJSON;
		PropertyNonInteractive<String> m_serializedParamBindingModesJSON;
		PropertyNonInteractive<String> m_tag;

		/* NonSerialized */ std::shared_ptr<Canvas> m_canvas;
		/* NonSerialized */ String m_loadedPath;
		/* NonSerialized */ String m_loadedAssetBasePath;
		/* NonSerialized */ String m_appliedSerializedParamsJSON;
		/* NonSerialized */ String m_appliedSerializedParamBindingsJSON;
		/* NonSerialized */ String m_appliedSerializedParamBindingModesJSON;
		/* NonSerialized */ HashTable<String, String> m_paramBindingMappingCache;
		/* NonSerialized */ HashTable<String, ParamRefMode> m_paramBindingModeCache;
		/* NonSerialized */ HashTable<String, ParamValue> m_paramOverrideCache;

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

	protected:
		void replaceAdditionalParamRefsInternal(StringView oldName, StringView newName) override;

	public:
		explicit SubCanvas(const PropertyValue<String>& canvasPath = U"")
			: SerializableComponentBase{ U"SubCanvas", { &m_canvasPath, &m_propagateEvents, &m_serializedParamsJSON, &m_serializedParamBindingsJSON, &m_serializedParamBindingModesJSON, &m_tag } }
			, m_canvasPath{ U"canvasPath", canvasPath }
			, m_propagateEvents{ U"propagateEvents", true }
			, m_serializedParamsJSON{ U"serializedParamsJSON", U"{}" }
			, m_serializedParamBindingsJSON{ U"serializedParamBindingsJSON", U"{}" }
			, m_serializedParamBindingModesJSON{ U"serializedParamBindingModesJSON", U"{}" }
			, m_tag{ U"tag", U"" }
		{
			loadCanvasInternal();
		}

		void update(const std::shared_ptr<Node>& node) override;

		void draw(const Node& node) const override;

		std::shared_ptr<Node> hitTest(
			const std::shared_ptr<Node>& node,
			const Vec2& point,
			OnlyScrollableYN onlyScrollable,
			detail::UsePrevZOrderInSiblingsYN usePrevZOrderInSiblings) const override;

		[[nodiscard]]
		const Array<std::shared_ptr<Node>>& subCanvasChildren() const override;

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
		const String& serializedParamsJSON() const
		{
			return m_serializedParamsJSON.value();
		}

		std::shared_ptr<SubCanvas> setSerializedParamsJSON(const String& serializedParamsJSON)
		{
			m_serializedParamsJSON.setValue(serializedParamsJSON);
			return shared_from_this();
		}

		[[nodiscard]]
		const String& serializedParamBindingsJSON() const
		{
			return m_serializedParamBindingsJSON.value();
		}

		std::shared_ptr<SubCanvas> setSerializedParamBindingsJSON(const String& serializedParamBindingsJSON)
		{
			m_serializedParamBindingsJSON.setValue(serializedParamBindingsJSON);
			return shared_from_this();
		}

		[[nodiscard]]
		const String& serializedParamBindingModesJSON() const
		{
			return m_serializedParamBindingModesJSON.value();
		}

		std::shared_ptr<SubCanvas> setSerializedParamBindingModesJSON(const String& serializedParamBindingModesJSON)
		{
			m_serializedParamBindingModesJSON.setValue(serializedParamBindingModesJSON);
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

		/// @brief 指定したパラメータ名を参照している紐付け数を取得
		/// @param paramName パラメータ名
		/// @return 参照している数
		[[nodiscard]]
		size_t countParamBindingRefs(StringView paramName) const;

		/// @brief 指定したパラメータ名を参照している紐付けをすべて削除
		/// @param paramName パラメータ名
		void clearParamBindingRefs(StringView paramName);

		/// @brief 紐付けで参照しているパラメータ名を置換
		/// @param oldName 置換対象のパラメータ参照名
		/// @param newName 新しいパラメータ参照名
		void replaceParamBindingRefs(StringView oldName, StringView newName);

		/// @brief 有効なパラメータに含まれないパラメータ名を参照している紐付けをすべて削除
		/// @param node このコンポーネントが所属するノード(親Canvasのパラメータ取得に使用)
		/// @return 削除したパラメータ参照の名前の配列
		Array<String> removeInvalidParamBindingRefs(const Node& node);

		/// @brief パラメータ紐付けの反映モード指定を正規化(紐付けがあるがモード指定が空のものをNormalで埋める)
		void normalizeParamBindingModes();

		/// @brief 紐付けで参照しているパラメータ名を列挙
		/// @param pParamRefs 挿入先のハッシュセットのポインタ
		void populateParamBindingRefs(HashSet<String>* pParamRefs) const;

		/// @brief 指定したパラメータ名を参照している子Canvasのパラメータ名一覧を取得
		/// @param parentParamName 親Canvasのパラメータ名
		/// @return 紐付けで参照している子Canvasのパラメータ名の配列
		[[nodiscard]]
		Array<String> getSubCanvasParamsBoundTo(StringView parentParamName) const;
	};
}
