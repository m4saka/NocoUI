# NocoUI 名前付きプロパティシステム 代替実装案の詳細考察

## 1. はじめに

本ドキュメントでは、NocoUIにおける名前付きプロパティシステム（変数システム）の実装について、props.mdで提案した方式以外の代替案を詳しく検討します。各案について、実装の詳細、技術的課題、メリット・デメリットを深く分析します。

## 2. 代替案1: プロパティプロキシパターン

### 2.1 概要

プロパティへの直接的なバインディングではなく、プロキシオブジェクトを介して値を管理する方式です。

### 2.2 実装詳細

```cpp
template<typename T>
class PropertyProxy
{
    std::variant<T, String> m_valueOrVariable;
    WeakPtr<Canvas> m_canvas;
    std::function<void(const T&)> m_onChange;
    
public:
    T get() const
    {
        if (std::holds_alternative<String>(m_valueOrVariable))
        {
            const String& varName = std::get<String>(m_valueOrVariable);
            if (auto canvas = m_canvas.lock())
            {
                return canvas->getVariable<T>(varName).value_or(T{});
            }
        }
        return std::get<T>(m_valueOrVariable);
    }
    
    void set(const T& value)
    {
        m_valueOrVariable = value;
        if (m_onChange)
        {
            m_onChange(value);
        }
    }
    
    void bindToVariable(const String& varName)
    {
        m_valueOrVariable = varName;
    }
    
    bool isBoundToVariable() const
    {
        return std::holds_alternative<String>(m_valueOrVariable);
    }
};

// コンポーネントでの使用例
class RectRenderer : public ComponentBase
{
    PropertyProxy<ColorF> m_colorProxy;
    
public:
    Property<ColorF>& color()
    {
        return m_colorProxy;
    }
    
    void draw() const override
    {
        const ColorF currentColor = m_colorProxy.get();
        // 描画処理
    }
};
```

### 2.3 メリット

1. **既存コードの変更が最小限**: Property<T>をPropertyProxy<T>に置き換えるだけ
2. **実行時の柔軟性**: 値と変数参照を動的に切り替え可能
3. **デバッグの容易さ**: プロキシ層で値の変更を監視可能
4. **メモリ効率**: 変数バインディングがない場合のオーバーヘッドが小さい

### 2.4 デメリット

1. **間接参照のコスト**: 毎回のアクセスでstd::variantのチェックが必要
2. **型安全性の低下**: コンパイル時の型チェックが弱い
3. **複雑な状態管理**: PropertyValueの状態管理との統合が複雑

### 2.5 シリアライズ

```json
{
    "color": {
        "type": "proxy",
        "value": "$panelColor"  // 変数参照
    }
}
// または
{
    "color": {
        "type": "proxy",
        "value": {
            "direct": [0.5, 0.2, 0.8, 1.0]  // 直接値
        }
    }
}
```

## 3. 代替案2: スタイルシートベースアプローチ

### 3.1 概要

CSS/SASSに似たスタイルシート言語を導入し、セレクタベースでプロパティを設定する方式です。

### 3.2 実装詳細

```cpp
// スタイルシート定義
class StyleSheet
{
    struct Rule
    {
        String selector;  // "#button", ".panel", "Node[name='header']"
        HashMap<String, PropertyValueVariant> properties;
        int specificity;  // CSSのような詳細度計算
    };
    
    Array<Rule> m_rules;
    HashMap<String, PropertyValueVariant> m_variables;
    
public:
    void parseFromString(const String& stylesheet)
    {
        // パーサーの実装
        // 例: "$primaryColor: #0066cc;"
        //     ".button { color: $primaryColor; }"
    }
    
    void applyToNode(SharedPtr<Node> node) const
    {
        Array<const Rule*> matchingRules = findMatchingRules(node);
        sortBySpecificity(matchingRules);
        
        for (const auto* rule : matchingRules)
        {
            applyRuleToNode(rule, node);
        }
    }
};

// セレクタエンジン
class SelectorEngine
{
    static bool matches(const String& selector, SharedPtr<Node> node)
    {
        if (selector.starts_with(U'#'))
        {
            // IDセレクタ
            return node->name() == selector.substr(1);
        }
        else if (selector.starts_with(U'.'))
        {
            // クラスセレクタ
            return node->hasClass(selector.substr(1));
        }
        else if (selector.contains(U'['))
        {
            // 属性セレクタ
            return matchesAttributeSelector(selector, node);
        }
        // 他のセレクタタイプ...
    }
};
```

### 3.3 スタイルシート記法

```scss
// variables.nss (NocoUI Style Sheet)
$primaryColor: #0066cc;
$secondaryColor: #6633cc;
$baseFontSize: 16px;
$spacing: 8px;

// アニメーション定義
@keyframes fadeIn {
    0% { opacity: 0; scale: 0.8; }
    100% { opacity: 1; scale: 1.0; }
}

// ミックスイン
@mixin button-style($bg-color) {
    background-color: $bg-color;
    border-radius: 4px;
    padding: $spacing;
    
    &:hover {
        background-color: lighten($bg-color, 10%);
    }
    
    &:pressed {
        background-color: darken($bg-color, 10%);
    }
}

// スタイル定義
.button {
    @include button-style($primaryColor);
    font-size: $baseFontSize;
    animation: fadeIn 0.3s ease-out;
}

.button.secondary {
    @include button-style($secondaryColor);
}

// 条件付きスタイル
@media (theme: dark) {
    .panel {
        background-color: #1a1a1a;
        color: #ffffff;
    }
}
```

### 3.4 実行時スタイル適用

```cpp
class StyledCanvas : public Canvas
{
    Array<SharedPtr<StyleSheet>> m_styleSheets;
    StyleCache m_cache;
    
public:
    void addStyleSheet(SharedPtr<StyleSheet> sheet)
    {
        m_styleSheets.push_back(sheet);
        invalidateCache();
    }
    
    void update(double deltaTime) override
    {
        // スタイルの継承と計算
        updateComputedStyles(m_rootNode);
        
        Canvas::update(deltaTime);
    }
    
private:
    void updateComputedStyles(SharedPtr<Node> node)
    {
        ComputedStyle computed = m_cache.getOrCompute(node, [&] {
            return computeStyleForNode(node);
        });
        
        applyComputedStyle(node, computed);
        
        for (auto& child : node->children())
        {
            updateComputedStyles(child);
        }
    }
};
```

### 3.5 メリット

1. **Web開発者に馴染みやすい**: CSS/SASSの知識を活用可能
2. **強力な表現力**: セレクタ、継承、カスケードなど
3. **テーマの管理が容易**: スタイルシートの切り替えだけ
4. **ホットリロード対応**: スタイルファイルの変更を即座に反映

### 3.6 デメリット

1. **実装の複雑さ**: パーサー、セレクタエンジン、詳細度計算など
2. **パフォーマンスオーバーヘッド**: セレクタマッチングのコスト
3. **学習コスト**: 独自のスタイル言語の習得が必要
4. **デバッグの困難さ**: どのルールが適用されたか追跡が難しい

## 4. 代替案3: データバインディングフレームワーク

### 4.1 概要

MVVMパターンに基づき、ViewModelとViewの双方向バインディングを実現する方式です。

### 4.2 実装詳細

```cpp
// ViewModelの基底クラス
class ViewModel : public std::enable_shared_from_this<ViewModel>
{
    HashMap<String, std::any> m_properties;
    HashMap<String, Array<std::function<void()>>> m_propertyChangedHandlers;
    
public:
    template<typename T>
    void setProperty(const String& name, const T& value)
    {
        if (auto it = m_properties.find(name); it != m_properties.end())
        {
            if (std::any_cast<T>(it->second) == value)
            {
                return;  // 値が変わっていない
            }
        }
        
        m_properties[name] = value;
        notifyPropertyChanged(name);
    }
    
    template<typename T>
    Optional<T> getProperty(const String& name) const
    {
        if (auto it = m_properties.find(name); it != m_properties.end())
        {
            try
            {
                return std::any_cast<T>(it->second);
            }
            catch (...) {}
        }
        return none;
    }
    
protected:
    void notifyPropertyChanged(const String& propertyName)
    {
        if (auto it = m_propertyChangedHandlers.find(propertyName); 
            it != m_propertyChangedHandlers.end())
        {
            for (const auto& handler : it->second)
            {
                handler();
            }
        }
    }
};

// バインディング式の評価
class BindingExpression
{
    String m_expression;  // "{Binding Path=BackgroundColor}"
    WeakPtr<ViewModel> m_dataContext;
    
public:
    std::any evaluate() const
    {
        if (auto vm = m_dataContext.lock())
        {
            // パス解析とプロパティ取得
            return evaluatePath(vm, parsePath(m_expression));
        }
        return {};
    }
    
private:
    Array<String> parsePath(const String& expr) const
    {
        // "Parent.Settings.BackgroundColor" -> ["Parent", "Settings", "BackgroundColor"]
        return expr.split(U'.');
    }
};

// バインディング可能なプロパティ
template<typename T>
class BindableProperty
{
    Optional<T> m_localValue;
    Optional<BindingExpression> m_binding;
    std::function<void(const T&)> m_onChanged;
    
public:
    void bind(const String& expression, SharedPtr<ViewModel> dataContext)
    {
        m_binding = BindingExpression{expression, dataContext};
        m_localValue = none;
        
        // プロパティ変更の監視を設定
        dataContext->subscribeToPropertyChange(
            extractPropertyName(expression),
            [this] { updateFromBinding(); }
        );
    }
    
    T get() const
    {
        if (m_binding)
        {
            if (auto value = m_binding->evaluate())
            {
                return std::any_cast<T>(value);
            }
        }
        return m_localValue.value_or(T{});
    }
};
```

### 4.3 使用例

```cpp
// ViewModelの定義
class ThemeViewModel : public ViewModel
{
public:
    ThemeViewModel()
    {
        setProperty("BackgroundColor", ColorF{0.1, 0.1, 0.1});
        setProperty("TextColor", ColorF{0.9, 0.9, 0.9});
        setProperty("FontSize", 16.0);
    }
    
    void switchToLightTheme()
    {
        setProperty("BackgroundColor", ColorF{0.95, 0.95, 0.95});
        setProperty("TextColor", ColorF{0.1, 0.1, 0.1});
    }
};

// Viewでの使用
auto panel = node->emplaceComponent<RectRenderer>();
panel->color.bind("{Binding Path=BackgroundColor}", themeViewModel);

auto label = node->emplaceComponent<Label>();
label->color.bind("{Binding Path=TextColor}", themeViewModel);
label->fontSize.bind("{Binding Path=FontSize}", themeViewModel);
```

### 4.4 高度なバインディング機能

```cpp
// コンバータ
class IValueConverter
{
public:
    virtual std::any convert(const std::any& value) const = 0;
    virtual std::any convertBack(const std::any& value) const = 0;
};

class BoolToVisibilityConverter : public IValueConverter
{
    std::any convert(const std::any& value) const override
    {
        bool b = std::any_cast<bool>(value);
        return b ? Visibility::Visible : Visibility::Collapsed;
    }
};

// マルチバインディング
class MultiBinding
{
    Array<BindingExpression> m_bindings;
    std::function<std::any(const Array<std::any>&)> m_converter;
    
public:
    std::any evaluate() const
    {
        Array<std::any> values;
        for (const auto& binding : m_bindings)
        {
            values.push_back(binding.evaluate());
        }
        return m_converter(values);
    }
};

// 使用例
colorBinding.bindMulti(
    {"{Binding Path=IsSelected}", "{Binding Path=IsHovered}"},
    [](const Array<std::any>& values) {
        bool isSelected = std::any_cast<bool>(values[0]);
        bool isHovered = std::any_cast<bool>(values[1]);
        
        if (isSelected) return ColorF{0.2, 0.5, 0.9};
        if (isHovered) return ColorF{0.3, 0.3, 0.3};
        return ColorF{0.1, 0.1, 0.1};
    }
);
```

### 4.5 メリット

1. **双方向バインディング**: UIとデータの同期が自動
2. **テスタビリティ**: ViewModelの単体テストが容易
3. **分離の明確化**: ビジネスロジックとUIの分離
4. **複雑な変換**: コンバータによる柔軟な値変換

### 4.6 デメリット

1. **過度な複雑性**: 小規模なUIには過剰な仕組み
2. **メモリ使用量**: ViewModelとバインディング情報の保持
3. **デバッグの困難さ**: バインディングエラーの原因特定が難しい
4. **パフォーマンス**: プロパティ変更の伝播コスト

## 5. 代替案4: コンパイル時プロパティ生成

### 5.1 概要

コンパイル時にプロパティ定義から最適化されたコードを生成する方式です。

### 5.2 実装詳細

```cpp
// プロパティ定義ファイル (properties.def)
DEFINE_PROPERTY_SET(UITheme,
    PROPERTY(ColorF, primaryColor, ColorF(0.0, 0.4, 0.8))
    PROPERTY(ColorF, secondaryColor, ColorF(0.4, 0.0, 0.8))
    PROPERTY(double, fontSize, 16.0)
    PROPERTY(Vec2, spacing, Vec2(8.0, 8.0))
)

// コード生成ツールによって生成されるコード
class UIThemeProperties
{
    static constexpr uint32_t PROPERTY_COUNT = 4;
    
    enum PropertyIndex : uint32_t
    {
        PrimaryColor = 0,
        SecondaryColor = 1,
        FontSize = 2,
        Spacing = 3
    };
    
    // プロパティストレージ（キャッシュライン最適化）
    alignas(64) std::array<ColorF, 2> m_colorProperties;
    alignas(64) double m_fontSize;
    alignas(64) Vec2 m_spacing;
    
    // 高速アクセス用のプロパティマップ
    static inline const HashMap<String, PropertyIndex> s_propertyMap = {
        {U"primaryColor", PrimaryColor},
        {U"secondaryColor", SecondaryColor},
        {U"fontSize", FontSize},
        {U"spacing", Spacing}
    };
    
public:
    template<typename T>
    Optional<T> getProperty(const String& name) const
    {
        auto it = s_propertyMap.find(name);
        if (it == s_propertyMap.end()) return none;
        
        switch (it->second)
        {
        case PrimaryColor:
            if constexpr (std::is_same_v<T, ColorF>)
                return m_colorProperties[0];
            break;
        case SecondaryColor:
            if constexpr (std::is_same_v<T, ColorF>)
                return m_colorProperties[1];
            break;
        case FontSize:
            if constexpr (std::is_same_v<T, double>)
                return m_fontSize;
            break;
        case Spacing:
            if constexpr (std::is_same_v<T, Vec2>)
                return m_spacing;
            break;
        }
        return none;
    }
    
    // コンパイル時プロパティアクセス
    template<PropertyIndex Index>
    constexpr auto& get()
    {
        if constexpr (Index == PrimaryColor || Index == SecondaryColor)
            return m_colorProperties[Index];
        else if constexpr (Index == FontSize)
            return m_fontSize;
        else if constexpr (Index == Spacing)
            return m_spacing;
    }
};

// 使用例（コンパイル時に解決）
auto color = theme.get<UIThemeProperties::PrimaryColor>();
```

### 5.3 プロパティ参照の最適化

```cpp
// プロパティ参照をコンパイル時に解決
template<typename PropertySet, auto PropertyIndex>
class CompiledPropertyRef
{
    SharedPtr<PropertySet> m_propertySet;
    
public:
    auto get() const
    {
        return m_propertySet->template get<PropertyIndex>();
    }
    
    void bindTo(SharedPtr<PropertySet> propertySet)
    {
        m_propertySet = propertySet;
    }
};

// マクロでの簡潔な記述
#define BIND_PROPERTY(component, property, propertySet, propertyName) \
    component->property.bindTo<propertySet, propertySet::propertyName>()

// 使用例
BIND_PROPERTY(rect, color, UIThemeProperties, PrimaryColor);
```

### 5.4 メリット

1. **最高のパフォーマンス**: 実行時のオーバーヘッドがほぼゼロ
2. **型安全性**: コンパイル時にすべての型チェック
3. **メモリ効率**: 最適化されたメモリレイアウト
4. **インライン化**: コンパイラによる積極的な最適化

### 5.5 デメリット

1. **柔軟性の欠如**: 実行時の動的なプロパティ追加が不可能
2. **ビルド時間**: コード生成によるビルド時間の増加
3. **複雑なビルドプロセス**: コード生成ツールの統合が必要
4. **エディタ統合の困難さ**: 動的な編集には不向き

## 6. 代替案5: アスペクト指向プロパティシステム

### 6.1 概要

アスペクト指向プログラミング（AOP）の概念を用いて、プロパティの振る舞いを横断的に管理する方式です。

### 6.2 実装詳細

```cpp
// プロパティアスペクト
class PropertyAspect
{
public:
    virtual void beforeGet(const String& propertyName, void* component) {}
    virtual void afterGet(const String& propertyName, void* component) {}
    virtual void beforeSet(const String& propertyName, const std::any& value, void* component) {}
    virtual void afterSet(const String& propertyName, const std::any& value, void* component) {}
};

// 変数解決アスペクト
class VariableResolutionAspect : public PropertyAspect
{
    SharedPtr<Canvas> m_canvas;
    HashMap<String, String> m_variableBindings;
    
public:
    void beforeGet(const String& propertyName, void* component) override
    {
        if (auto it = m_variableBindings.find(propertyName); 
            it != m_variableBindings.end())
        {
            // 変数値を解決してプロパティにインジェクト
            if (auto value = m_canvas->getVariableAny(it->second))
            {
                injectValue(component, propertyName, value);
            }
        }
    }
};

// ロギングアスペクト
class LoggingAspect : public PropertyAspect
{
    void afterSet(const String& propertyName, const std::any& value, void* component) override
    {
        Console() << U"Property changed: " << propertyName 
                  << U" in " << typeid(*component).name();
    }
};

// アスペクト適用可能なプロパティ
template<typename T>
class AspectProperty
{
    T m_value;
    Array<SharedPtr<PropertyAspect>> m_aspects;
    String m_name;
    void* m_owner;
    
public:
    T get() const
    {
        for (auto& aspect : m_aspects)
        {
            aspect->beforeGet(m_name, m_owner);
        }
        
        T result = m_value;
        
        for (auto& aspect : m_aspects)
        {
            aspect->afterGet(m_name, m_owner);
        }
        
        return result;
    }
    
    void set(const T& value)
    {
        for (auto& aspect : m_aspects)
        {
            aspect->beforeSet(m_name, std::any(value), m_owner);
        }
        
        m_value = value;
        
        for (auto& aspect : m_aspects)
        {
            aspect->afterSet(m_name, std::any(value), m_owner);
        }
    }
    
    void addAspect(SharedPtr<PropertyAspect> aspect)
    {
        m_aspects.push_back(aspect);
    }
};
```

### 6.3 アスペクトの合成

```cpp
// 複数のアスペクトを組み合わせる
class CompositeAspect : public PropertyAspect
{
    Array<SharedPtr<PropertyAspect>> m_aspects;
    
public:
    void addAspect(SharedPtr<PropertyAspect> aspect)
    {
        m_aspects.push_back(aspect);
    }
    
    void beforeGet(const String& propertyName, void* component) override
    {
        for (auto& aspect : m_aspects)
        {
            aspect->beforeGet(propertyName, component);
        }
    }
    // 他のメソッドも同様
};

// 条件付きアスペクト
class ConditionalAspect : public PropertyAspect
{
    std::function<bool()> m_condition;
    SharedPtr<PropertyAspect> m_innerAspect;
    
public:
    void beforeGet(const String& propertyName, void* component) override
    {
        if (m_condition())
        {
            m_innerAspect->beforeGet(propertyName, component);
        }
    }
};
```

### 6.4 メリット

1. **横断的関心事の分離**: ロギング、検証、変換などを独立管理
2. **動的な振る舞い変更**: 実行時にアスペクトを追加・削除
3. **再利用性**: アスペクトを異なるプロパティで再利用
4. **拡張性**: 新しい機能をアスペクトとして追加

### 6.5 デメリット

1. **パフォーマンスオーバーヘッド**: 各アクセスでの関数呼び出し
2. **デバッグの困難さ**: アスペクトのチェーンが複雑
3. **理解の困難さ**: AOPの概念が必要
4. **予測困難な振る舞い**: アスペクトの相互作用

## 7. 比較分析

### 7.1 パフォーマンス比較

| 方式 | 読み取り性能 | 書き込み性能 | メモリ使用量 | 初期化コスト |
|------|------------|------------|------------|------------|
| プロパティプロキシ | 中 | 中 | 小 | 小 |
| スタイルシート | 低 | 低 | 大 | 大 |
| データバインディング | 中 | 低 | 大 | 中 |
| コンパイル時生成 | 高 | 高 | 小 | なし |
| アスペクト指向 | 低 | 低 | 中 | 中 |

### 7.2 機能比較

| 方式 | 動的変更 | 型安全性 | エディタ統合 | 学習コスト |
|------|---------|---------|-------------|----------|
| プロパティプロキシ | ○ | △ | ○ | 低 |
| スタイルシート | ◎ | × | ◎ | 高 |
| データバインディング | ◎ | ○ | △ | 高 |
| コンパイル時生成 | × | ◎ | × | 中 |
| アスペクト指向 | ◎ | △ | △ | 高 |

### 7.3 使用シーン別推奨

1. **シンプルなテーマ切り替え**: プロパティプロキシ
2. **複雑なスタイル管理**: スタイルシート
3. **データ駆動UI**: データバインディング
4. **高性能要求**: コンパイル時生成
5. **プラグイン機能**: アスペクト指向

## 8. ハイブリッドアプローチの提案

### 8.1 段階的実装戦略

```cpp
// Phase 1: 基本的な変数システム（プロパティプロキシ）
class Phase1Property : public PropertyProxy<T> { /* ... */ };

// Phase 2: スタイルシート統合
class Phase2Property : public Phase1Property
{
    Optional<StyleRule> m_styleRule;
    // スタイルシートからの値も考慮
};

// Phase 3: バインディング対応
class Phase3Property : public Phase2Property
{
    Optional<BindingExpression> m_binding;
    // データバインディングも追加
};
```

### 8.2 プラガブルアーキテクチャ

```cpp
class PropertyResolver
{
    Array<SharedPtr<IPropertySource>> m_sources;
    
public:
    template<typename T>
    Optional<T> resolve(const String& propertyName)
    {
        for (auto& source : m_sources)
        {
            if (auto value = source->getValue<T>(propertyName))
            {
                return value;
            }
        }
        return none;
    }
};

// 各ソースの実装
class VariableSource : public IPropertySource { /* ... */ };
class StyleSheetSource : public IPropertySource { /* ... */ };
class BindingSource : public IPropertySource { /* ... */ };
```

## 9. 実装の技術的課題と解決策

### 9.1 循環参照の検出

```cpp
class DependencyGraph
{
    struct Node
    {
        String name;
        HashSet<String> dependencies;
        bool visiting = false;
        bool visited = false;
    };
    
    HashMap<String, Node> m_nodes;
    
public:
    bool hasCycle() const
    {
        for (auto& [name, node] : m_nodes)
        {
            if (!node.visited && hasCycleDFS(name))
            {
                return true;
            }
        }
        return false;
    }
    
private:
    bool hasCycleDFS(const String& nodeName) const
    {
        auto& node = m_nodes[nodeName];
        
        if (node.visiting) return true;  // 循環を検出
        if (node.visited) return false;
        
        node.visiting = true;
        
        for (const auto& dep : node.dependencies)
        {
            if (hasCycleDFS(dep)) return true;
        }
        
        node.visiting = false;
        node.visited = true;
        return false;
    }
};
```

### 9.2 メモリリークの防止

```cpp
class WeakPropertyBinding
{
    WeakPtr<ComponentBase> m_component;
    String m_propertyName;
    std::function<void()> m_updateCallback;
    
public:
    void update()
    {
        if (auto component = m_component.lock())
        {
            m_updateCallback();
        }
        else
        {
            // コンポーネントが削除されたら自動的にバインディングも削除
            markForRemoval();
        }
    }
};
```

### 9.3 スレッドセーフティ

```cpp
template<typename T>
class ThreadSafeProperty
{
    mutable std::shared_mutex m_mutex;
    T m_value;
    
public:
    T get() const
    {
        std::shared_lock lock(m_mutex);
        return m_value;
    }
    
    void set(const T& value)
    {
        std::unique_lock lock(m_mutex);
        m_value = value;
    }
    
    template<typename Func>
    void update(Func&& updater)
    {
        std::unique_lock lock(m_mutex);
        m_value = updater(m_value);
    }
};
```

## 10. まとめ

各代替案にはそれぞれ固有の利点と課題があります：

1. **プロパティプロキシ**: 実装が簡単で既存システムとの統合が容易
2. **スタイルシート**: 表現力が高くデザイナーフレンドリー
3. **データバインディング**: アプリケーション規模での状態管理に優れる
4. **コンパイル時生成**: 最高のパフォーマンスを実現
5. **アスペクト指向**: 柔軟性と拡張性に優れる

最適な選択は、プロジェクトの要件、チームのスキルセット、パフォーマンス要求によって異なります。多くの場合、これらのアプローチを組み合わせたハイブリッド実装が最も実用的な解決策となるでしょう。