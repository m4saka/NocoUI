# NocoUI 名前付きプロパティシステム（Props）考察

## 1. 概要

NocoUIにおいて、プロパティに名前（例：`$panelColor`）を付けて、Canvas利用時に値を動的に設定できる仕組みの導入を検討します。これはReactのpropsやCSSカスタムプロパティ（CSS変数）に似た概念で、UIの再利用性と柔軟性を大幅に向上させます。

## 2. ユースケース

### 2.1 基本的な使用例

```cpp
// UIテンプレート定義時
auto panel = node->emplaceComponent<RectRenderer>();
panel->color.bindToVariable("$panelColor");  // 変数名をバインド

// Canvas利用時
canvas->setVariable("$panelColor", ColorF{0.2, 0.3, 0.8});  // 値を設定
// 全ての$panelColorにバインドされたプロパティが自動的に更新される
```

### 2.2 テーマシステムへの応用

```cpp
// ダークテーマ
canvas->setVariables({
    {"$backgroundColor", ColorF{0.1, 0.1, 0.1}},
    {"$textColor", ColorF{0.9, 0.9, 0.9}},
    {"$accentColor", ColorF{0.3, 0.5, 0.9}}
});

// ライトテーマ
canvas->setVariables({
    {"$backgroundColor", ColorF{0.95, 0.95, 0.95}},
    {"$textColor", ColorF{0.1, 0.1, 0.1}},
    {"$accentColor", ColorF{0.2, 0.4, 0.8}}
});
```

### 2.3 コンポーネントの再利用

```cpp
// ボタンコンポーネントの定義
void createButton(SharedPtr<Node> node)
{
    auto rect = node->emplaceComponent<RectRenderer>();
    rect->color.bindToVariable("$buttonColor");
    rect->borderColor.bindToVariable("$buttonBorderColor");
    
    auto label = node->emplaceComponent<Label>();
    label->color.bindToVariable("$buttonTextColor");
}

// 異なるスタイルでの利用
canvas->setVariable("$buttonColor", ColorF{0.2, 0.6, 0.2});  // 緑のボタン
canvas->setVariable("$buttonColor", ColorF{0.8, 0.2, 0.2});  // 赤のボタン
```

## 3. 既存システムとの統合

### 3.1 PropertyValueシステムとの関係

現在のPropertyValueシステムは、インタラクション状態（Normal/Hovered/Pressed/Disabled）に基づく値の切り替えを提供しています。名前付きプロパティシステムは、これと直交する形で実装可能です：

```cpp
template<typename T>
class PropertyValue
{
    // 既存のフィールド
    T m_defaultValue;
    Optional<T> m_hoveredValue;
    Optional<T> m_pressedValue;
    Optional<T> m_disabledValue;
    
    // 新規追加
    Optional<String> m_variableName;  // バインドされた変数名
    
public:
    void bindToVariable(const String& varName)
    {
        m_variableName = varName;
    }
    
    T getValue(InteractionState state, Canvas* canvas) const
    {
        // 変数がバインドされている場合は、その値を使用
        if (m_variableName && canvas->hasVariable(*m_variableName))
        {
            return canvas->getVariable<T>(*m_variableName);
        }
        
        // 通常の状態ベースの値取得
        return getValueForState(state);
    }
};
```

### 3.2 Property/SmoothPropertyとの統合

```cpp
template<typename T>
class Property
{
    PropertyValue<T> m_value;
    Canvas* m_canvas;  // 所属するCanvas
    
public:
    void bindToVariable(const String& varName)
    {
        m_value.bindToVariable(varName);
    }
    
    T get() const
    {
        return m_value.getValue(getCurrentState(), m_canvas);
    }
};
```

## 4. 実装方針

### 4.1 Canvas側の変数管理

```cpp
class Canvas
{
private:
    // 変数名と値のマッピング
    HashMap<String, std::any> m_variables;
    
    // 変数が変更されたときに通知を受けるプロパティのリスト
    HashMap<String, Array<WeakPtr<PropertyBase>>> m_variableBindings;
    
public:
    template<typename T>
    void setVariable(const String& name, const T& value)
    {
        m_variables[name] = value;
        
        // バインドされているプロパティに通知
        if (auto it = m_variableBindings.find(name); it != m_variableBindings.end())
        {
            for (auto& weakProp : it->second)
            {
                if (auto prop = weakProp.lock())
                {
                    prop->notifyVariableChanged();
                }
            }
        }
    }
    
    template<typename T>
    Optional<T> getVariable(const String& name) const
    {
        if (auto it = m_variables.find(name); it != m_variables.end())
        {
            try
            {
                return std::any_cast<T>(it->second);
            }
            catch (const std::bad_any_cast&)
            {
                // 型が一致しない場合
                return none;
            }
        }
        return none;
    }
};
```

### 4.2 型安全性の確保

型の不一致を防ぐため、変数登録時に型情報を保持：

```cpp
class TypedVariable
{
    std::any m_value;
    std::type_index m_typeIndex;
    
public:
    template<typename T>
    TypedVariable(const T& value)
        : m_value(value)
        , m_typeIndex(typeid(T))
    {}
    
    template<typename T>
    Optional<T> get() const
    {
        if (m_typeIndex == typeid(T))
        {
            return std::any_cast<T>(m_value);
        }
        return none;
    }
};
```

### 4.3 変数のスコープ

変数のスコープを制御するための階層的な変数管理：

```cpp
class VariableScope
{
    HashMap<String, TypedVariable> m_localVariables;
    VariableScope* m_parent;
    
public:
    template<typename T>
    Optional<T> getVariable(const String& name) const
    {
        // ローカルスコープを優先
        if (auto it = m_localVariables.find(name); it != m_localVariables.end())
        {
            return it->second.get<T>();
        }
        
        // 親スコープを検索
        if (m_parent)
        {
            return m_parent->getVariable<T>(name);
        }
        
        return none;
    }
};
```

## 5. アニメーションシステムとの統合

### 5.1 変数値のアニメーション

キーフレームアニメーションシステムと統合して、変数値をアニメーション可能に：

```cpp
// 変数値のアニメーション定義
canvas->animateVariable("$panelColor", {
    {0.0, ColorF{1, 0, 0}},    // 0秒: 赤
    {1.0, ColorF{0, 1, 0}},    // 1秒: 緑
    {2.0, ColorF{0, 0, 1}}     // 2秒: 青
});

// 全ての$panelColorにバインドされたプロパティが
// 自動的にアニメーションされる
```

### 5.2 実装方法

```cpp
class Canvas
{
private:
    struct AnimatedVariable
    {
        String name;
        AnimationData animation;
        double currentTime;
    };
    
    Array<AnimatedVariable> m_animatedVariables;
    
public:
    template<typename T>
    void animateVariable(const String& name, const Array<Keyframe<T>>& keyframes)
    {
        m_animatedVariables.push_back({
            name,
            AnimationData{keyframes},
            0.0
        });
    }
    
    void updateAnimations(double deltaTime)
    {
        for (auto& animVar : m_animatedVariables)
        {
            animVar.currentTime += deltaTime;
            auto value = interpolateKeyframes(animVar.animation, animVar.currentTime);
            setVariable(animVar.name, value);
        }
    }
};
```

### 5.3 トランジション効果

変数値が変更されたときの自動トランジション：

```cpp
canvas->setVariableTransition("$panelColor", 0.3);  // 0.3秒でトランジション
canvas->setVariable("$panelColor", newColor);  // 滑らかに色が変化
```

## 6. エディタ統合

### 6.1 Inspector UI

NocoEditorのInspectorで変数バインディングを設定：

```
[Color Property]
Value: ■ (0.5, 0.2, 0.8)
Variable: [$panelColor    ] [Bind]
```

### 6.2 変数マネージャー

エディタ内で定義された変数の一覧管理：

```
Variables:
- $panelColor     : ColorF(0.2, 0.3, 0.8)
- $textColor      : ColorF(0.9, 0.9, 0.9)
- $buttonRadius   : 8.0
- $animDuration   : 0.5
```

## 7. 高度な機能

### 7.1 計算プロパティ

変数値を使った計算式のサポート：

```cpp
// 他の変数を参照した計算
canvas->setComputedVariable("$hoverColor", 
    [](Canvas* c) { 
        auto base = c->getVariable<ColorF>("$panelColor").value();
        return base * 1.2;  // 20%明るく
    }
);
```

### 7.2 条件付き値

```cpp
// 条件に応じた値の切り替え
panel->color.bindToConditional(
    "$isDarkMode",
    "$darkPanelColor",   // trueの場合
    "$lightPanelColor"   // falseの場合
);
```

### 7.3 名前空間

変数名の衝突を防ぐための名前空間：

```cpp
canvas->setVariable("theme.$backgroundColor", color1);
canvas->setVariable("button.$backgroundColor", color2);
```

## 8. パフォーマンス考慮事項

### 8.1 変更通知の最適化

- 変数が実際に変更された場合のみ通知
- バッチ更新で複数の変数を一度に更新
- WeakPtrを使用して不要な参照を回避

### 8.2 キャッシング

```cpp
class Property
{
    mutable Optional<T> m_cachedValue;
    mutable bool m_needsUpdate = true;
    
    void notifyVariableChanged()
    {
        m_needsUpdate = true;
    }
    
    T get() const
    {
        if (m_needsUpdate)
        {
            m_cachedValue = computeValue();
            m_needsUpdate = false;
        }
        return *m_cachedValue;
    }
};
```

## 9. エラーハンドリング

### 9.1 型の不一致

```cpp
// 警告ログを出力
if (!canvas->getVariable<ColorF>("$panelColor"))
{
    Console() << U"Warning: Variable '$panelColor' is not of type ColorF";
}
```

### 9.2 未定義変数

```cpp
// デフォルト値へのフォールバック
auto color = canvas->getVariable<ColorF>("$undefinedColor")
    .value_or(ColorF{0.5, 0.5, 0.5});
```

## 10. 実装優先順位

1. **フェーズ1: 基本実装**
   - 変数の設定・取得API
   - PropertyValueへの変数バインディング
   - Canvas側の変数管理

2. **フェーズ2: エディタ統合**
   - InspectorでのバインディングUI
   - 変数マネージャーパネル

3. **フェーズ3: アニメーション統合**
   - 変数値のアニメーション
   - トランジション効果

4. **フェーズ4: 高度な機能**
   - 計算プロパティ
   - 条件付き値
   - 名前空間

## 11. シリアライズシステムの詳細設計

### 11.1 変数バインディングのシリアライズ

#### PropertyValueのシリアライズ拡張

既存のPropertyValueシリアライズ形式を拡張して、変数バインディング情報を含める：

```json
// 単純な値の場合（従来通り）
"fontSize": 24.0

// 変数バインディングのみの場合
"fontSize": "$baseFontSize"

// 変数バインディングとフォールバック値を持つ場合
"fontSize": {
    "variable": "$baseFontSize",
    "fallback": 24.0
}

// 状態別の値と変数バインディングの組み合わせ
"color": {
    "variable": "$primaryColor",
    "fallback": {
        "default": "#0066cc",
        "hovered": "$hoverColor",  // 状態ごとに異なる変数も可能
        "pressed": "#003366",
        "smoothTime": 0.2
    }
}
```

#### 実装方法

```cpp
template<typename T>
JSON PropertyValue<T>::toJSON() const
{
    // 変数バインディングがある場合
    if (m_variableName)
    {
        // フォールバック値がない単純なケース
        if (!hasStateValues())
        {
            return JSON{*m_variableName};
        }
        
        // フォールバック値がある複雑なケース
        JSON result;
        result[U"variable"] = *m_variableName;
        result[U"fallback"] = serializeStateValues();
        return result;
    }
    
    // 従来のシリアライズ
    return serializeStateValues();
}

template<typename T>
void PropertyValue<T>::fromJSON(const JSON& json, Canvas* canvas)
{
    // 文字列の場合は変数名として解釈
    if (json.isString())
    {
        const String str = json.getString();
        if (str.starts_with(U'$'))
        {
            m_variableName = str;
            return;
        }
    }
    
    // オブジェクトの場合
    if (json.isObject())
    {
        // 変数バインディングのチェック
        if (json.hasElement(U"variable"))
        {
            m_variableName = json[U"variable"].getString();
            
            // フォールバック値の読み込み
            if (json.hasElement(U"fallback"))
            {
                deserializeStateValues(json[U"fallback"]);
            }
            return;
        }
    }
    
    // 従来の読み込み処理
    deserializeStateValues(json);
}
```

### 11.2 変数定義のシリアライズ

#### Canvas側の変数定義

変数定義をCanvasまたは専用のファイルにシリアライズ：

```json
{
    "variables": {
        "$primaryColor": {
            "type": "ColorF",
            "value": [0.0, 0.4, 0.8, 1.0],
            "description": "Main brand color"
        },
        "$fontSize": {
            "type": "double",
            "value": 16.0,
            "constraints": {
                "min": 8.0,
                "max": 72.0
            }
        },
        "$spacing": {
            "type": "Vec2",
            "value": [8.0, 8.0]
        }
    },
    "variableGroups": {
        "theme": {
            "$backgroundColor": "#ffffff",
            "$textColor": "#333333"
        },
        "animation": {
            "$duration": 0.3,
            "$easing": "easeInOutCubic"
        }
    }
}
```

#### 変数定義クラス

```cpp
class VariableDefinition
{
    String m_name;
    std::type_index m_typeIndex;
    std::any m_defaultValue;
    Optional<String> m_description;
    JSON m_constraints;  // 型固有の制約
    
public:
    JSON toJSON() const
    {
        JSON result;
        result[U"type"] = getTypeName(m_typeIndex);
        result[U"value"] = serializeValue(m_defaultValue);
        
        if (m_description)
        {
            result[U"description"] = *m_description;
        }
        
        if (!m_constraints.isEmpty())
        {
            result[U"constraints"] = m_constraints;
        }
        
        return result;
    }
    
    static VariableDefinition fromJSON(const String& name, const JSON& json)
    {
        const String typeName = json[U"type"].getString();
        const std::type_index typeIndex = getTypeIndex(typeName);
        
        VariableDefinition def;
        def.m_name = name;
        def.m_typeIndex = typeIndex;
        def.m_defaultValue = deserializeValue(typeName, json[U"value"]);
        def.m_description = json[U"description"].getOpt<String>();
        def.m_constraints = json[U"constraints"];
        
        return def;
    }
};
```

### 11.3 読み込み時の変数解決戦略

#### 遅延解決方式

変数が未定義の場合の処理方法：

```cpp
class DeferredVariableBinding
{
    String m_variableName;
    std::function<void(const std::any&)> m_setter;
    std::type_index m_expectedType;
    
public:
    void resolve(Canvas* canvas)
    {
        if (auto value = canvas->getVariableAny(m_variableName))
        {
            if (value->type() == m_expectedType)
            {
                m_setter(*value);
            }
            else
            {
                // 型の不一致をログ出力
                logTypeMismatch(m_variableName, m_expectedType, value->type());
            }
        }
    }
};

class Canvas
{
    Array<DeferredVariableBinding> m_deferredBindings;
    
public:
    void resolveDeferredBindings()
    {
        for (auto& binding : m_deferredBindings)
        {
            binding.resolve(this);
        }
        m_deferredBindings.clear();
    }
};
```

#### 読み込み順序の管理

```cpp
class SceneLoader
{
public:
    SharedPtr<Node> loadScene(const FilePath& path)
    {
        // 1. 変数定義ファイルを先に読み込み
        if (FileSystem::Exists(path.replaced_extension(U".vars")))
        {
            loadVariableDefinitions(path.replaced_extension(U".vars"));
        }
        
        // 2. シーンファイルを読み込み
        JSON sceneJSON = JSON::Load(path);
        
        // 3. グローバル変数を設定
        if (sceneJSON.hasElement(U"variables"))
        {
            loadVariables(sceneJSON[U"variables"]);
        }
        
        // 4. ノード階層を構築
        auto rootNode = Node::CreateFromJSON(sceneJSON[U"root"]);
        
        // 5. 遅延バインディングを解決
        m_canvas->resolveDeferredBindings();
        
        return rootNode;
    }
};
```

### 11.4 循環参照の防止

#### 変数依存グラフの構築

```cpp
class VariableDependencyGraph
{
    HashMap<String, HashSet<String>> m_dependencies;
    
public:
    bool addDependency(const String& variable, const String& dependsOn)
    {
        // 循環参照チェック
        if (hasCyclicDependency(variable, dependsOn))
        {
            return false;
        }
        
        m_dependencies[variable].insert(dependsOn);
        return true;
    }
    
private:
    bool hasCyclicDependency(const String& from, const String& to) const
    {
        // DFSで循環を検出
        HashSet<String> visited;
        return hasCyclicDependencyDFS(to, from, visited);
    }
};
```

#### 計算プロパティの安全な評価

```cpp
class ComputedVariable
{
    String m_name;
    std::function<std::any(Canvas*)> m_compute;
    HashSet<String> m_dependencies;
    mutable Optional<std::any> m_cachedValue;
    mutable bool m_computing = false;
    
public:
    std::any evaluate(Canvas* canvas) const
    {
        // 再帰的な評価を防ぐ
        if (m_computing)
        {
            throw std::runtime_error(
                Format(U"Circular dependency detected in variable: {}", m_name));
        }
        
        // キャッシュがあれば使用
        if (m_cachedValue)
        {
            return *m_cachedValue;
        }
        
        // 計算を実行
        m_computing = true;
        ScopeGuard guard{[this] { m_computing = false; }};
        
        m_cachedValue = m_compute(canvas);
        return *m_cachedValue;
    }
};
```

### 11.5 バージョン互換性

#### スキーマバージョン管理

```json
{
    "schemaVersion": "1.0",
    "compatibleVersions": ["0.9", "1.0"],
    "variables": {
        // ...
    }
}
```

#### マイグレーション戦略

```cpp
class VariableSchemaUpgrader
{
    static JSON upgrade(const JSON& oldData, const String& fromVersion)
    {
        if (fromVersion == U"0.9")
        {
            return upgradeFrom09To10(oldData);
        }
        return oldData;
    }
    
private:
    static JSON upgradeFrom09To10(const JSON& oldData)
    {
        JSON newData = oldData;
        
        // 旧形式の変数名を新形式に変換
        // 例: "panel-color" -> "$panelColor"
        for (auto& [key, value] : newData[U"variables"].objectView())
        {
            if (!key.starts_with(U'$'))
            {
                String newKey = U"$" + toCamelCase(key);
                newData[U"variables"][newKey] = value;
                newData[U"variables"].erase(key);
            }
        }
        
        return newData;
    }
};
```

### 11.6 エディタプレビューと実行時の分離

#### デザイン時変数とランタイム変数

```cpp
class EditorCanvas : public Canvas
{
    HashMap<String, std::any> m_designTimeVariables;
    HashMap<String, std::any> m_runtimeVariables;
    bool m_isPreviewMode = false;
    
public:
    template<typename T>
    Optional<T> getVariable(const String& name) const override
    {
        // プレビューモードではランタイム変数を優先
        if (m_isPreviewMode)
        {
            if (auto value = getRuntimeVariable<T>(name))
            {
                return value;
            }
        }
        
        // デザイン時変数を返す
        return getDesignTimeVariable<T>(name);
    }
    
    void exportForRuntime(const FilePath& path) const
    {
        JSON result;
        
        // デザイン時変数のみをエクスポート
        for (const auto& [name, value] : m_designTimeVariables)
        {
            result[U"variables"][name] = serializeVariable(value);
        }
        
        result.save(path);
    }
};
```

### 11.7 パフォーマンス最適化

#### 変数アクセスの高速化

```cpp
class OptimizedVariableStorage
{
    // 型別に専用ストレージを持つ
    HashMap<String, double> m_doubleVariables;
    HashMap<String, ColorF> m_colorVariables;
    HashMap<String, Vec2> m_vec2Variables;
    HashMap<String, std::any> m_otherVariables;
    
    // 頻繁にアクセスされる変数のキャッシュ
    mutable LRUCache<String, std::any> m_accessCache;
    
public:
    template<typename T>
    Optional<T> getVariable(const String& name) const
    {
        // キャッシュをチェック
        if (auto cached = m_accessCache.get(name))
        {
            return std::any_cast<T>(*cached);
        }
        
        // 型別ストレージから取得
        Optional<T> result;
        if constexpr (std::is_same_v<T, double>)
        {
            if (auto it = m_doubleVariables.find(name); 
                it != m_doubleVariables.end())
            {
                result = it->second;
            }
        }
        // ... 他の型も同様
        
        // キャッシュに追加
        if (result)
        {
            m_accessCache.put(name, std::any{*result});
        }
        
        return result;
    }
};
```

#### バッチ更新の実装

```cpp
class Canvas
{
    struct PendingVariableUpdate
    {
        String name;
        std::any value;
    };
    
    Array<PendingVariableUpdate> m_pendingUpdates;
    bool m_batchUpdateMode = false;
    
public:
    void beginBatchUpdate()
    {
        m_batchUpdateMode = true;
    }
    
    void endBatchUpdate()
    {
        m_batchUpdateMode = false;
        
        // 依存関係を考慮して更新順序を決定
        auto sortedUpdates = topologicalSort(m_pendingUpdates);
        
        // 一括更新
        for (const auto& update : sortedUpdates)
        {
            applyVariableUpdate(update.name, update.value);
        }
        
        // 一度だけ通知
        notifyAllBindings();
        
        m_pendingUpdates.clear();
    }
};
```

### 11.8 デバッグ支援機能

#### 変数トレース機能

```cpp
class VariableDebugger
{
    struct VariableAccess
    {
        String variableName;
        String accessorPath;  // "Node1/Component2/Property3"
        TimePoint timestamp;
        bool wasResolved;
    };
    
    Array<VariableAccess> m_accessLog;
    bool m_enabled = false;
    
public:
    void logAccess(const String& varName, const String& accessor, bool resolved)
    {
        if (!m_enabled) return;
        
        m_accessLog.push_back({
            varName,
            accessor,
            std::chrono::steady_clock::now(),
            resolved
        });
    }
    
    void printUnresolvedVariables() const
    {
        HashSet<String> unresolved;
        for (const auto& access : m_accessLog)
        {
            if (!access.wasResolved)
            {
                unresolved.insert(access.variableName);
            }
        }
        
        for (const auto& var : unresolved)
        {
            Console() << U"Unresolved variable: " << var;
        }
    }
};
```

## 12. まとめ

名前付きプロパティシステムの導入により、NocoUIは以下の利点を得られます：

1. **再利用性の向上**: 同じUIコンポーネントを異なるスタイルで使い回せる
2. **テーマ対応**: 変数値を変更するだけで全体のテーマを切り替え可能
3. **保守性の向上**: スタイル値を一元管理
4. **アニメーションとの統合**: 変数値をアニメーションすることで複雑な演出が可能
5. **エディタでの使いやすさ**: GUIで直感的に変数をバインド・管理

シリアライズシステムは既存のNocoUIのシリアライズ機構を自然に拡張し、以下の特徴を持ちます：

- **後方互換性**: 既存のシリアライズ形式との互換性を保持
- **型安全性**: 変数の型情報を保持し、実行時の型チェックを実施
- **柔軟性**: 変数バインディング、フォールバック値、計算プロパティなど多様な機能
- **パフォーマンス**: 型別ストレージとキャッシングによる高速アクセス
- **デバッグ性**: 変数アクセスのトレースと未解決変数の検出

既存のPropertyValueシステムと自然に統合でき、段階的な実装が可能な設計となっています。