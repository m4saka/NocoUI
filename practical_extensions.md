# NocoUI 実用的な拡張機能の提案

## 1. はじめに

本ドキュメントでは、NocoUIの既存アーキテクチャを活かしながら、比較的短期間で実装可能で、実用的な価値を提供する拡張機能を提案します。各機能は実装の難易度と期待される効果のバランスを考慮して選定しています。

## 2. フォームバリデーションシステム

### 2.1 概要

TextBoxやその他の入力コンポーネントに対する汎用的なバリデーション機能です。

### 2.2 実装設計

```cpp
// バリデーションルールの基底クラス
class ValidationRule
{
public:
    virtual bool validate(const String& value) const = 0;
    virtual String getErrorMessage() const = 0;
};

// 具体的なルール実装
class RequiredRule : public ValidationRule
{
    bool validate(const String& value) const override
    {
        return !value.isEmpty();
    }
    
    String getErrorMessage() const override
    {
        return U"このフィールドは必須です";
    }
};

class EmailRule : public ValidationRule
{
    bool validate(const String& value) const override
    {
        // 簡易的なメールアドレスチェック
        return value.contains(U'@') && value.contains(U'.');
    }
    
    String getErrorMessage() const override
    {
        return U"有効なメールアドレスを入力してください";
    }
};

// TextBoxの拡張
class ValidatedTextBox : public TextBox
{
    Array<SharedPtr<ValidationRule>> m_rules;
    bool m_isValid = true;
    String m_errorMessage;
    
public:
    void addRule(SharedPtr<ValidationRule> rule)
    {
        m_rules.push_back(rule);
    }
    
    bool validate()
    {
        const String currentText = getText();
        
        for (const auto& rule : m_rules)
        {
            if (!rule->validate(currentText))
            {
                m_isValid = false;
                m_errorMessage = rule->getErrorMessage();
                updateVisualState();
                return false;
            }
        }
        
        m_isValid = true;
        m_errorMessage.clear();
        updateVisualState();
        return true;
    }
    
    void onTextChanged() override
    {
        TextBox::onTextChanged();
        
        // リアルタイムバリデーション（オプション）
        if (m_validateOnChange)
        {
            validate();
        }
    }
};
```

### 2.3 実装難易度と価値

- **実装難易度**: 低〜中
- **開発期間**: 1週間程度
- **価値**: フォーム入力が必要なアプリケーションで即座に活用可能

## 3. レスポンシブレイアウトシステム

### 3.1 概要

画面サイズに応じて自動的にレイアウトを調整する機能です。

### 3.2 実装設計

```cpp
// ブレークポイント定義
struct Breakpoint
{
    double width;
    String name;  // "mobile", "tablet", "desktop"
};

// レスポンシブ値
template<typename T>
class ResponsiveValue
{
    HashMap<String, T> m_values;
    T m_defaultValue;
    
public:
    ResponsiveValue(T defaultValue) : m_defaultValue(defaultValue) {}
    
    void set(const String& breakpoint, const T& value)
    {
        m_values[breakpoint] = value;
    }
    
    T get(double screenWidth) const
    {
        // 現在の画面幅に応じた値を返す
        String currentBreakpoint = getBreakpoint(screenWidth);
        
        if (auto it = m_values.find(currentBreakpoint); it != m_values.end())
        {
            return it->second;
        }
        
        return m_defaultValue;
    }
};

// レスポンシブコンテナ
class ResponsiveContainer : public ComponentBase
{
    ResponsiveValue<LayoutVariant> m_layout;
    ResponsiveValue<double> m_spacing;
    ResponsiveValue<LRTB> m_padding;
    
public:
    ResponsiveContainer()
    {
        // デフォルト設定
        m_layout.set("mobile", VerticalLayout{});
        m_layout.set("desktop", HorizontalLayout{});
        
        m_spacing.set("mobile", 8.0);
        m_spacing.set("desktop", 16.0);
    }
    
    void update(double deltaTime) override
    {
        double screenWidth = Scene::Width();
        
        // 現在の画面幅に応じてレイアウトを更新
        m_node->setLayout(m_layout.get(screenWidth));
        
        // スペーシングも更新
        if (auto* hLayout = std::get_if<HorizontalLayout>(&m_node->layout()))
        {
            hLayout->spacing = m_spacing.get(screenWidth);
        }
    }
};
```

### 3.3 実装難易度と価値

- **実装難易度**: 中
- **開発期間**: 1〜2週間
- **価値**: マルチデバイス対応が必須の現代において高い実用性

## 4. データグリッドコンポーネント

### 4.1 概要

表形式のデータを効率的に表示・編集できるグリッドコンポーネントです。

### 4.2 実装設計

```cpp
// カラム定義
struct GridColumn
{
    String id;
    String header;
    double width = 100.0;
    bool sortable = true;
    bool resizable = true;
    std::function<String(const JSON&)> formatter;
};

// データグリッド本体
class DataGrid : public ComponentBase
{
    Array<GridColumn> m_columns;
    Array<JSON> m_data;
    size_t m_selectedRow = 0;
    Optional<String> m_sortColumn;
    bool m_sortAscending = true;
    
    // 仮想スクロール用
    size_t m_visibleStartIndex = 0;
    size_t m_visibleRowCount = 20;
    
public:
    void setColumns(const Array<GridColumn>& columns)
    {
        m_columns = columns;
        rebuildHeader();
    }
    
    void setData(const Array<JSON>& data)
    {
        m_data = data;
        updateVisibleRange();
    }
    
    void draw() const override
    {
        const double rowHeight = 30.0;
        const auto bounds = m_node->getBounds();
        
        // ヘッダーを描画
        drawHeader(bounds);
        
        // 可視範囲のデータのみ描画（仮想スクロール）
        for (size_t i = 0; i < m_visibleRowCount; ++i)
        {
            size_t dataIndex = m_visibleStartIndex + i;
            if (dataIndex >= m_data.size()) break;
            
            drawRow(dataIndex, bounds.movedBy(0, (i + 1) * rowHeight));
        }
    }
    
    void onScroll(const Vec2& delta) override
    {
        m_visibleStartIndex = Max(0.0, m_visibleStartIndex + delta.y / 30.0);
        updateVisibleRange();
    }
    
private:
    void sortData(const String& columnId)
    {
        if (m_sortColumn == columnId)
        {
            m_sortAscending = !m_sortAscending;
        }
        else
        {
            m_sortColumn = columnId;
            m_sortAscending = true;
        }
        
        std::sort(m_data.begin(), m_data.end(), 
            [&](const JSON& a, const JSON& b) {
                auto valueA = a[columnId];
                auto valueB = b[columnId];
                
                if (valueA.isNumber() && valueB.isNumber())
                {
                    return m_sortAscending ? 
                        valueA.get<double>() < valueB.get<double>() :
                        valueA.get<double>() > valueB.get<double>();
                }
                
                return m_sortAscending ?
                    valueA.getString() < valueB.getString() :
                    valueA.getString() > valueB.getString();
            });
    }
};
```

### 4.3 実装難易度と価値

- **実装難易度**: 中〜高
- **開発期間**: 2〜3週間
- **価値**: 業務アプリケーションで頻繁に必要とされる機能

## 5. ドラッグ＆ドロップの拡張

### 5.1 概要

既存のDragDropSource/Targetを拡張し、より高度なドラッグ＆ドロップ操作を可能にします。

### 5.2 実装設計

```cpp
// ドラッグプレビュー
class DragPreview : public ComponentBase
{
    SharedPtr<Node> m_sourceNode;
    Vec2 m_offset;
    
public:
    void startDrag(SharedPtr<Node> source, const Vec2& mousePos)
    {
        m_sourceNode = source;
        m_offset = mousePos - source->getGlobalPosition();
        
        // 半透明のプレビューを作成
        m_node->transformEffect().opacity = PropertyValue<double>{0.7};
    }
    
    void update(double deltaTime) override
    {
        if (m_sourceNode)
        {
            m_node->setPosition(Cursor::PosF() - m_offset);
        }
    }
};

// 並び替え可能なリスト
class SortableList : public ComponentBase
{
    struct DragState
    {
        size_t draggedIndex;
        size_t dropIndex;
        double insertIndicatorY;
    };
    
    Optional<DragState> m_dragState;
    
public:
    void onChildDragStart(size_t childIndex)
    {
        m_dragState = DragState{childIndex, childIndex, 0};
    }
    
    void onDragOver(const Vec2& pos)
    {
        if (!m_dragState) return;
        
        // ドロップ位置を計算
        size_t newDropIndex = calculateDropIndex(pos);
        
        if (newDropIndex != m_dragState->dropIndex)
        {
            m_dragState->dropIndex = newDropIndex;
            updateInsertIndicator();
        }
    }
    
    void onDrop()
    {
        if (!m_dragState) return;
        
        // 実際に並び替えを実行
        auto children = m_node->children();
        auto draggedNode = children[m_dragState->draggedIndex];
        
        children.erase(children.begin() + m_dragState->draggedIndex);
        children.insert(children.begin() + m_dragState->dropIndex, draggedNode);
        
        // 子ノードを再構築
        m_node->clearChildren();
        for (auto& child : children)
        {
            m_node->addChild(child);
        }
        
        m_dragState = none;
    }
};
```

### 5.3 実装難易度と価値

- **実装難易度**: 中
- **開発期間**: 1週間
- **価値**: 直感的なUI操作を実現し、ユーザビリティを大幅に向上

## 6. トースト通知システム

### 6.1 概要

非侵入的な通知を表示するトーストメッセージシステムです。

### 6.2 実装設計

```cpp
// トーストメッセージ
class ToastMessage
{
public:
    enum Type { Info, Success, Warning, Error };
    
    String message;
    Type type;
    double duration = 3.0;
    std::function<void()> onAction;
    String actionLabel;
};

// トーストマネージャー
class ToastManager : public ComponentBase
{
    struct ActiveToast
    {
        ToastMessage message;
        SharedPtr<Node> node;
        double elapsedTime = 0.0;
        double animationProgress = 0.0;
    };
    
    Array<ActiveToast> m_activeToasts;
    static constexpr double TOAST_HEIGHT = 60.0;
    static constexpr double TOAST_SPACING = 10.0;
    
public:
    void show(const ToastMessage& message)
    {
        // トーストノードを作成
        auto toastNode = Node::Create();
        setupToastNode(toastNode, message);
        
        m_activeToasts.push_back({message, toastNode, 0.0, 0.0});
        m_node->addChild(toastNode);
        
        updateToastPositions();
    }
    
    void update(double deltaTime) override
    {
        Array<size_t> toRemove;
        
        for (size_t i = 0; i < m_activeToasts.size(); ++i)
        {
            auto& toast = m_activeToasts[i];
            toast.elapsedTime += deltaTime;
            
            // アニメーション更新
            if (toast.animationProgress < 1.0)
            {
                toast.animationProgress = Min(1.0, toast.animationProgress + deltaTime * 3.0);
                updateToastAnimation(toast);
            }
            
            // 自動削除チェック
            if (toast.elapsedTime > toast.message.duration)
            {
                // フェードアウト
                double fadeOutProgress = (toast.elapsedTime - toast.message.duration) / 0.3;
                
                if (fadeOutProgress >= 1.0)
                {
                    toRemove.push_back(i);
                }
                else
                {
                    toast.node->transformEffect().opacity = PropertyValue<double>{1.0 - fadeOutProgress};
                }
            }
        }
        
        // 削除処理
        for (auto it = toRemove.rbegin(); it != toRemove.rend(); ++it)
        {
            m_node->removeChild(m_activeToasts[*it].node);
            m_activeToasts.erase(m_activeToasts.begin() + *it);
        }
        
        if (!toRemove.empty())
        {
            updateToastPositions();
        }
    }
    
private:
    void setupToastNode(SharedPtr<Node> node, const ToastMessage& message)
    {
        // 背景色を設定
        auto rect = node->emplaceComponent<RectRenderer>();
        rect->color = PropertyValue<ColorF>{getColorForType(message.type)};
        rect->cornerRadius = PropertyValue<Vec4>{8, 8, 8, 8};
        
        // メッセージテキスト
        auto label = node->emplaceComponent<Label>();
        label->text = message.message;
        label->color = PropertyValue<ColorF>{Palette::White};
        
        // アクションボタン（オプション）
        if (!message.actionLabel.isEmpty())
        {
            auto actionButton = Node::Create();
            // ボタン設定...
            node->addChild(actionButton);
        }
    }
};
```

### 6.3 実装難易度と価値

- **実装難易度**: 低〜中
- **開発期間**: 3〜5日
- **価値**: ユーザーフィードバックの標準的な方法として広く使用可能

## 7. キーボードナビゲーション

### 7.1 概要

Tab、矢印キー、Enterキーなどでの完全なキーボード操作を実現します。

### 7.2 実装設計

```cpp
// フォーカス管理
class FocusManager
{
    WeakPtr<Node> m_focusedNode;
    Array<WeakPtr<Node>> m_focusableNodes;
    
public:
    void registerFocusable(SharedPtr<Node> node)
    {
        m_focusableNodes.push_back(node);
    }
    
    void moveFocus(FocusDirection direction)
    {
        auto current = m_focusedNode.lock();
        if (!current) 
        {
            focusFirst();
            return;
        }
        
        switch (direction)
        {
        case FocusDirection::Next:
            focusNext();
            break;
        case FocusDirection::Previous:
            focusPrevious();
            break;
        case FocusDirection::Up:
        case FocusDirection::Down:
        case FocusDirection::Left:
        case FocusDirection::Right:
            focusSpatial(direction);
            break;
        }
    }
    
private:
    void focusSpatial(FocusDirection direction)
    {
        auto current = m_focusedNode.lock();
        if (!current) return;
        
        const Vec2 currentPos = current->getGlobalCenter();
        SharedPtr<Node> bestCandidate;
        double bestScore = Inf<double>;
        
        for (const auto& weakNode : m_focusableNodes)
        {
            auto node = weakNode.lock();
            if (!node || node == current) continue;
            
            const Vec2 candidatePos = node->getGlobalCenter();
            const Vec2 delta = candidatePos - currentPos;
            
            // 方向に応じたスコア計算
            if (isInDirection(delta, direction))
            {
                double score = calculateFocusScore(currentPos, candidatePos, direction);
                if (score < bestScore)
                {
                    bestScore = score;
                    bestCandidate = node;
                }
            }
        }
        
        if (bestCandidate)
        {
            setFocus(bestCandidate);
        }
    }
};

// キーボード対応コンポーネント
class KeyboardNavigable : public ComponentBase
{
    bool m_isFocused = false;
    
public:
    void onMount() override
    {
        getCanvas()->getFocusManager()->registerFocusable(m_node);
    }
    
    void onKeyDown(const KeyEvent& event) override
    {
        if (!m_isFocused) return;
        
        switch (event.key)
        {
        case Key::Tab:
            if (event.shift)
                getCanvas()->getFocusManager()->moveFocus(FocusDirection::Previous);
            else
                getCanvas()->getFocusManager()->moveFocus(FocusDirection::Next);
            event.consume();
            break;
            
        case Key::Enter:
        case Key::Space:
            // クリックイベントをトリガー
            m_node->triggerClick();
            event.consume();
            break;
        }
    }
    
    void onFocusChanged(bool focused)
    {
        m_isFocused = focused;
        
        // フォーカスインジケーターを表示
        if (focused)
        {
            m_node->setBorder(2, Palette::Blue);
        }
        else
        {
            m_node->setBorder(0, Palette::Transparent);
        }
    }
};
```

### 7.3 実装難易度と価値

- **実装難易度**: 中
- **開発期間**: 1〜2週間
- **価値**: アクセシビリティ向上と作業効率化

## 8. コンテキストメニューの拡張

### 8.1 概要

右クリックメニューをより柔軟で高機能にします。

### 8.2 実装設計

```cpp
// コンテキストメニュービルダー
class ContextMenuBuilder
{
    struct MenuItem
    {
        String label;
        Optional<String> icon;
        Optional<String> shortcut;
        std::function<void()> action;
        bool enabled = true;
        bool checked = false;
        Array<MenuItem> subItems;
    };
    
    Array<MenuItem> m_items;
    
public:
    ContextMenuBuilder& addItem(const String& label, std::function<void()> action)
    {
        m_items.push_back({label, none, none, action});
        return *this;
    }
    
    ContextMenuBuilder& addSeparator()
    {
        m_items.push_back({U"---", none, none, nullptr});
        return *this;
    }
    
    ContextMenuBuilder& addSubMenu(const String& label, 
                                   std::function<void(ContextMenuBuilder&)> builder)
    {
        MenuItem item{label};
        ContextMenuBuilder subBuilder;
        builder(subBuilder);
        item.subItems = subBuilder.m_items;
        m_items.push_back(item);
        return *this;
    }
    
    SharedPtr<Node> build()
    {
        auto menuNode = Node::Create();
        menuNode->setConstraint(AnchorConstraint{});
        
        double y = 0;
        for (const auto& item : m_items)
        {
            auto itemNode = createMenuItem(item);
            itemNode->setPosition({0, y});
            menuNode->addChild(itemNode);
            y += item.label == U"---" ? 1 : 30;
        }
        
        return menuNode;
    }
};

// 動的コンテキストメニュー
class DynamicContextMenu : public ComponentBase
{
    std::function<void(ContextMenuBuilder&)> m_builder;
    
public:
    void onRightClick(const Vec2& pos) override
    {
        ContextMenuBuilder builder;
        
        // コンテキストに応じてメニューを構築
        m_builder(builder);
        
        auto menu = builder.build();
        showContextMenu(menu, pos);
    }
};
```

### 8.3 実装難易度と価値

- **実装難易度**: 低〜中
- **開発期間**: 1週間
- **価値**: 一般的なデスクトップアプリケーションの標準的なUXパターン

## 9. アンドゥ・リドゥシステム

### 9.1 概要

ユーザーの操作を記録し、元に戻す・やり直すを可能にします。

### 9.2 実装設計

```cpp
// コマンドパターン
class Command
{
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual String getDescription() const = 0;
};

// プロパティ変更コマンド
template<typename T>
class PropertyChangeCommand : public Command
{
    Property<T>* m_property;
    T m_oldValue;
    T m_newValue;
    String m_description;
    
public:
    PropertyChangeCommand(Property<T>* prop, const T& newValue, const String& desc)
        : m_property(prop)
        , m_oldValue(prop->get())
        , m_newValue(newValue)
        , m_description(desc)
    {}
    
    void execute() override
    {
        m_property->set(m_newValue);
    }
    
    void undo() override
    {
        m_property->set(m_oldValue);
    }
    
    String getDescription() const override
    {
        return m_description;
    }
};

// アンドゥマネージャー
class UndoManager
{
    Array<SharedPtr<Command>> m_history;
    size_t m_currentIndex = 0;
    size_t m_maxHistorySize = 100;
    
public:
    void execute(SharedPtr<Command> command)
    {
        // 現在位置より後の履歴を削除
        m_history.erase(m_history.begin() + m_currentIndex, m_history.end());
        
        // コマンドを実行して履歴に追加
        command->execute();
        m_history.push_back(command);
        
        // 履歴サイズ制限
        if (m_history.size() > m_maxHistorySize)
        {
            m_history.erase(m_history.begin());
        }
        else
        {
            m_currentIndex++;
        }
    }
    
    bool canUndo() const
    {
        return m_currentIndex > 0;
    }
    
    void undo()
    {
        if (!canUndo()) return;
        
        m_currentIndex--;
        m_history[m_currentIndex]->undo();
    }
    
    bool canRedo() const
    {
        return m_currentIndex < m_history.size();
    }
    
    void redo()
    {
        if (!canRedo()) return;
        
        m_history[m_currentIndex]->execute();
        m_currentIndex++;
    }
};
```

### 9.3 実装難易度と価値

- **実装難易度**: 中
- **開発期間**: 1〜2週間
- **価値**: プロフェッショナルなアプリケーションには必須の機能

## 10. パフォーマンスモニター

### 10.1 概要

開発時にパフォーマンスを可視化するオーバーレイです。

### 10.2 実装設計

```cpp
class PerformanceMonitor : public ComponentBase
{
    struct Metrics
    {
        double frameTime = 0;
        double updateTime = 0;
        double drawTime = 0;
        size_t nodeCount = 0;
        size_t componentCount = 0;
        size_t drawCalls = 0;
        double memoryUsage = 0;
    };
    
    RingBuffer<Metrics> m_history{120};
    bool m_visible = false;
    
public:
    void update(double deltaTime) override
    {
        if (KeyCtrl.pressed() && KeyF12.down())
        {
            m_visible = !m_visible;
        }
        
        if (!m_visible) return;
        
        // メトリクスを収集
        Metrics current;
        current.frameTime = Scene::DeltaTime();
        current.nodeCount = countNodes(getCanvas()->rootNode());
        current.componentCount = countComponents(getCanvas()->rootNode());
        
        m_history.push(current);
    }
    
    void draw() const override
    {
        if (!m_visible) return;
        
        const auto bounds = RectF{10, 10, 300, 200};
        
        // 背景
        bounds.draw(ColorF{0, 0, 0, 0.7});
        
        // FPSグラフ
        drawGraph(bounds.movedBy(10, 30), m_history, 
                 [](const Metrics& m) { return 1.0 / m.frameTime; },
                 60.0, U"FPS");
        
        // テキスト情報
        const auto& latest = m_history.back();
        Font font{12};
        font(U"Nodes: {}"_fmt(latest.nodeCount)).draw(bounds.pos + Vec2{10, 10}, Palette::White);
        font(U"Components: {}"_fmt(latest.componentCount)).draw(bounds.pos + Vec2{10, 140}, Palette::White);
        font(U"Draw calls: {}"_fmt(latest.drawCalls)).draw(bounds.pos + Vec2{10, 160}, Palette::White);
    }
    
private:
    void drawGraph(const RectF& area, const RingBuffer<Metrics>& data,
                   std::function<double(const Metrics&)> getValue,
                   double maxValue, const String& label) const
    {
        // グラフ描画ロジック
        Array<Vec2> points;
        
        for (size_t i = 0; i < data.size(); ++i)
        {
            double x = area.x + (i / double(data.capacity())) * area.w;
            double y = area.y + area.h - (getValue(data[i]) / maxValue) * area.h;
            points.emplace_back(x, y);
        }
        
        if (points.size() >= 2)
        {
            LineString{points}.draw(2, Palette::Lime);
        }
    }
};
```

### 10.3 実装難易度と価値

- **実装難易度**: 低
- **開発期間**: 2〜3日
- **価値**: 開発時のパフォーマンス問題の早期発見

## 11. 実装優先順位の提案

実装の容易さと実用的価値のバランスを考慮した推奨順序：

1. **トースト通知システム** - 最も簡単で、即座に価値を提供
2. **フォームバリデーション** - 多くのアプリで必要とされる基本機能
3. **パフォーマンスモニター** - 開発効率を向上させる
4. **キーボードナビゲーション** - アクセシビリティの基本
5. **ドラッグ＆ドロップ拡張** - UXを大幅に向上
6. **コンテキストメニュー拡張** - デスクトップアプリの標準機能
7. **レスポンシブレイアウト** - モダンなUI要件
8. **アンドゥ・リドゥ** - プロフェッショナルな編集機能
9. **データグリッド** - 業務アプリケーションに必須

## 12. まとめ

これらの実用的な拡張機能は、NocoUIを本格的なアプリケーション開発フレームワークとして成熟させるために重要です。各機能は独立して実装可能で、段階的に追加していくことができます。既存のアーキテクチャを最大限に活用しながら、実際のアプリケーション開発で求められる機能を提供します。