#pragma once
#include <NocoUI.hpp>

namespace noco::editor
{
	// メタデータのキー
	struct PropertyKey
	{
		String componentName;
		String propertyName;

		bool operator==(const PropertyKey& other) const
		{
			return componentName == other.componentName && propertyName == other.propertyName;
		}
	};

	// PropertyValueのいずれかの状態の値がtrueかをチェック
	[[nodiscard]]
	inline bool HasAnyTrueState(const PropertyValue<bool>& propertyValue)
	{
		return propertyValue.hasAnyStateEqualTo(true);
	}

	struct PropertyMetadata
	{
		Optional<String> tooltip;
		Optional<String> tooltipDetail;
		std::function<bool(const ComponentBase&)> visibilityCondition;  // 表示条件
		bool refreshInspectorOnChange = false;  // 変更時にInspectorを更新するかどうか
		Optional<int32> numTextAreaLines;  // テキストエリアとして表示する場合の行数(未設定の場合はテキストボックス)
		bool refreshesEveryFrame = false;  // 毎フレームInspectorの値の更新が必要かどうか(テキストボックスなどユーザー編集を伴うコンポーネントで使用。現状コンポーネントのStringプロパティのみ対応)
	};

	struct PropertyVisibilityData
	{
		bool isVisibleByCondition = true;
	};

	[[nodiscard]]
	HashTable<PropertyKey, PropertyMetadata> InitPropertyMetadata();
}

template <>
struct std::hash<noco::editor::PropertyKey>
{
	size_t operator()(const noco::editor::PropertyKey& key) const
	{
		return std::hash<String>{}(key.componentName) ^ (std::hash<String>{}(key.propertyName) << 1);
	}
};
