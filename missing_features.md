# NocoUI 不足機能の分析と実装提案

## 1. はじめに

本ドキュメントでは、NocoUIに現在不足している基本的なUIコンポーネントと機能について分析し、その必要性と実装方法を考察します。これらの機能は、多くのUIフレームワークで標準的に提供されており、NocoUIの実用性を高めるために重要です。

## 2. 最重要：基本的な入力コンポーネント

### 2.1 Button（ボタン）

#### 現状の問題
現在、ボタンはRectRenderer + Label + EventTriggerの組み合わせで実現する必要があり、毎回同じ設定を繰り返す必要があります。

#### 実装提案

```cpp
class Button : public ComponentBase
{
    // 内部コンポーネント
    SharedPtr<RectRenderer> m_rect;
    SharedPtr<Label> m_label;
    SharedPtr<EventTrigger> m_trigger;
    
    // ボタン固有のプロパティ
    Property<String> text{U""};
    Property<ColorF> backgroundColor{Palette::Lightblue};
    Property<ColorF> textColor{Palette::Black};
    Property<bool> enabled{true};
    
    // 状態別の自動色変更
    PropertyValue<ColorF> m_stateColors{
        .defaultValue = Palette::Lightblue,
        .hoveredValue = Palette::Deepskyblue,
        .pressedValue = Palette::Dodgerblue,
        .disabledValue = Palette::Gray,
        .smoothTime = 0.1
    };
    
public:
    void onMount() override
    {
        // 自動的に必要なコンポーネントを設定
        m_rect = m_node->emplaceComponent<RectRenderer>();
        m_rect->color = m_stateColors;
        m_rect->cornerRadius = PropertyValue<Vec4>{Vec4{4, 4, 4, 4}};
        
        m_label = m_node->emplaceComponent<Label>();
        m_label->text = text;
        m_label->color = textColor;
        m_label->alignment = Vec2{0.5, 0.5};
        
        m_trigger = m_node->emplaceComponent<EventTrigger>();
        
        // デフォルトサイズ
        m_node->setConstraint(BoxConstraint{
            .minSize = {80, 32},
            .padding = LRTB{12, 12, 6, 6}
        });
    }
    
    // クリックイベントの簡易設定
    void onClick(std::function<void()> handler)
    {
        m_trigger->setOnClicked(handler);
    }
};
```

#### 必要性
- **高頻度使用**: ほぼすべてのUIで使用される最も基本的なコンポーネント
- **標準化**: 一貫したボタンの見た目と動作を保証
- **開発効率**: 毎回複数のコンポーネントを組み合わせる手間を削減

### 2.2 Checkbox（チェックボックス）

#### 実装提案

```cpp
class Checkbox : public ComponentBase
{
    Property<bool> checked{false};
    Property<String> label{U""};
    Property<bool> enabled{true};
    
    std::function<void(bool)> m_onValueChanged;
    
    SharedPtr<RectRenderer> m_box;
    SharedPtr<Label> m_checkmark;
    SharedPtr<Label> m_label;
    
public:
    void onMount() override
    {
        // チェックボックス本体
        auto boxNode = Node::Create();
        boxNode->setConstraint(BoxConstraint{.fixedSize = Vec2{20, 20}});
        
        m_box = boxNode->emplaceComponent<RectRenderer>();
        m_box->color = PropertyValue<ColorF>{Palette::White};
        m_box->borderColor = PropertyValue<ColorF>{Palette::Gray};
        m_box->borderWidth = PropertyValue<double>{2};
        
        // チェックマーク
        m_checkmark = boxNode->emplaceComponent<Label>();
        m_checkmark->text = U"✓";
        m_checkmark->fontSize = PropertyValue<double>{16};
        m_checkmark->alignment = Vec2{0.5, 0.5};
        
        // クリックイベント
        auto trigger = boxNode->emplaceComponent<EventTrigger>();
        trigger->setOnClicked([this] {
            if (enabled.get())
            {
                checked.set(!checked.get());
                if (m_onValueChanged)
                {
                    m_onValueChanged(checked.get());
                }
            }
        });
        
        m_node->addChild(boxNode);
        
        // ラベル
        if (!label.get().isEmpty())
        {
            auto labelNode = Node::Create();
            m_label = labelNode->emplaceComponent<Label>();
            m_label->text = label;
            m_node->addChild(labelNode);
        }
        
        // 水平レイアウト
        m_node->setLayout(HorizontalLayout{.spacing = 8});
    }
    
    void updateProperties() override
    {
        m_checkmark->color = PropertyValue<ColorF>{
            checked.get() ? Palette::Black : Palette::Transparent
        };
        
        m_box->backgroundColor = PropertyValue<ColorF>{
            enabled.get() ? Palette::White : Palette::Lightgray
        };
    }
};
```

#### 必要性
- **フォーム必須**: 複数選択や設定のON/OFFに不可欠
- **アクセシビリティ**: キーボード操作やスクリーンリーダー対応が必要

### 2.3 RadioButton（ラジオボタン）

#### 実装提案

```cpp
class RadioGroup
{
    Array<WeakPtr<RadioButton>> m_buttons;
    Optional<size_t> m_selectedIndex;
    
public:
    void registerButton(SharedPtr<RadioButton> button)
    {
        m_buttons.push_back(button);
        button->setGroup(this);
    }
    
    void select(RadioButton* button)
    {
        // 他のボタンの選択を解除
        for (auto& weakButton : m_buttons)
        {
            if (auto btn = weakButton.lock())
            {
                if (btn.get() != button)
                {
                    btn->setChecked(false);
                }
            }
        }
    }
};

class RadioButton : public ComponentBase
{
    Property<bool> checked{false};
    Property<String> label{U""};
    Property<String> value{U""};
    
    RadioGroup* m_group = nullptr;
    
    // 円形のラジオボタンを描画
    void drawRadioCircle(const Vec2& center, bool checked) const
    {
        Circle{center, 10}.drawFrame(2, Palette::Gray);
        
        if (checked)
        {
            Circle{center, 6}.draw(Palette::Blue);
        }
    }
};
```

#### 必要性
- **単一選択**: 複数の選択肢から1つを選ぶ標準的なUI
- **グループ管理**: RadioGroupによる排他制御が必要

### 2.4 Slider（スライダー）

#### 実装提案

```cpp
class Slider : public ComponentBase
{
    Property<double> value{0.0};
    Property<double> minValue{0.0};
    Property<double> maxValue{100.0};
    Property<double> step{1.0};
    Property<bool> showValue{true};
    Property<Orientation> orientation{Orientation::Horizontal};
    
    std::function<void(double)> m_onValueChanged;
    
    SharedPtr<RectRenderer> m_track;
    SharedPtr<RectRenderer> m_fill;
    SharedPtr<RectRenderer> m_thumb;
    SharedPtr<Label> m_valueLabel;
    
    bool m_isDragging = false;
    
public:
    void onMount() override
    {
        // トラック（背景）
        m_track = m_node->emplaceComponent<RectRenderer>();
        m_track->color = PropertyValue<ColorF>{Palette::Lightgray};
        m_track->cornerRadius = PropertyValue<Vec4>{Vec4{4, 4, 4, 4}};
        
        // フィル（進捗部分）
        auto fillNode = Node::Create();
        m_fill = fillNode->emplaceComponent<RectRenderer>();
        m_fill->color = PropertyValue<ColorF>{Palette::Blue};
        m_node->addChild(fillNode);
        
        // つまみ
        auto thumbNode = Node::Create();
        m_thumb = thumbNode->emplaceComponent<RectRenderer>();
        m_thumb->color = PropertyValue<ColorF>{
            .defaultValue = Palette::White,
            .hoveredValue = Palette::Lightblue,
            .pressedValue = Palette::Blue,
            .smoothTime = 0.1
        };
        m_thumb->borderColor = PropertyValue<ColorF>{Palette::Gray};
        m_thumb->borderWidth = PropertyValue<double>{2};
        m_thumb->cornerRadius = PropertyValue<Vec4>{Vec4{10, 10, 10, 10}};
        
        thumbNode->setConstraint(BoxConstraint{.fixedSize = Vec2{20, 20}});
        m_node->addChild(thumbNode);
        
        // ドラッグ処理
        auto trigger = thumbNode->emplaceComponent<EventTrigger>();
        trigger->setOnDragStart([this] { m_isDragging = true; });
        trigger->setOnDragEnd([this] { m_isDragging = false; });
    }
    
    void onDrag(const Vec2& delta) override
    {
        if (!m_isDragging) return;
        
        const double range = maxValue.get() - minValue.get();
        const double sliderLength = orientation.get() == Orientation::Horizontal ?
            m_node->getSize().x : m_node->getSize().y;
        
        const double deltaValue = (delta.x / sliderLength) * range;
        setValue(value.get() + deltaValue);
    }
    
    void setValue(double newValue)
    {
        // ステップに合わせて丸める
        newValue = std::round(newValue / step.get()) * step.get();
        newValue = std::clamp(newValue, minValue.get(), maxValue.get());
        
        if (newValue != value.get())
        {
            value.set(newValue);
            
            if (m_onValueChanged)
            {
                m_onValueChanged(newValue);
            }
            
            updateVisuals();
        }
    }
};
```

#### 必要性
- **直感的な値入力**: 範囲内の値を視覚的に選択
- **リアルタイムフィードバック**: 音量、明度、サイズなどの調整に最適

## 3. コンテナ・ナビゲーション系コンポーネント

### 3.1 ScrollView（スクロールビュー）

#### 現状の問題
Nodeにスクロール機能はあるが、スクロールバーの表示制御やスクロール領域の管理が手動で必要。

#### 実装提案

```cpp
class ScrollView : public ComponentBase
{
    Property<bool> showVerticalScrollbar{true};
    Property<bool> showHorizontalScrollbar{false};
    Property<ScrollbarVisibility> verticalScrollbarVisibility{ScrollbarVisibility::Auto};
    Property<ScrollbarVisibility> horizontalScrollbarVisibility{ScrollbarVisibility::Auto};
    
    SharedPtr<Node> m_contentNode;
    SharedPtr<Node> m_verticalScrollbar;
    SharedPtr<Node> m_horizontalScrollbar;
    
public:
    void onMount() override
    {
        // コンテンツ領域
        m_contentNode = Node::Create();
        m_contentNode->setScrollable(ScrollableAxisFlags::Both);
        m_node->addChild(m_contentNode);
        
        // 垂直スクロールバー
        if (showVerticalScrollbar.get())
        {
            m_verticalScrollbar = createScrollbar(Orientation::Vertical);
            m_node->addChild(m_verticalScrollbar);
        }
        
        // ビューポートのクリッピング
        m_node->setClipContent(true);
    }
    
    SharedPtr<Node> content() { return m_contentNode; }
    
    void scrollTo(const Vec2& position)
    {
        m_contentNode->setScrollOffset(position);
        updateScrollbars();
    }
    
    void scrollToBottom()
    {
        const double maxScroll = m_contentNode->getContentSize().y - m_node->getSize().y;
        scrollTo({0, maxScroll});
    }
};
```

#### 必要性
- **長いコンテンツの表示**: リストやドキュメントの表示に必須
- **モバイル対応**: タッチスクロールやモメンタムスクロールの実装基盤

### 3.2 TabView（タブビュー）

#### 実装提案

```cpp
class TabView : public ComponentBase
{
    struct Tab
    {
        String title;
        SharedPtr<Node> content;
        Optional<String> icon;
        bool closable = false;
    };
    
    Array<Tab> m_tabs;
    size_t m_selectedIndex = 0;
    
    SharedPtr<Node> m_tabBar;
    SharedPtr<Node> m_contentArea;
    
public:
    void addTab(const String& title, SharedPtr<Node> content)
    {
        m_tabs.push_back({title, content});
        
        // タブボタンを作成
        auto tabButton = createTabButton(title, m_tabs.size() - 1);
        m_tabBar->addChild(tabButton);
        
        // 最初のタブの場合は表示
        if (m_tabs.size() == 1)
        {
            selectTab(0);
        }
    }
    
    void selectTab(size_t index)
    {
        if (index >= m_tabs.size()) return;
        
        m_selectedIndex = index;
        
        // コンテンツを切り替え
        m_contentArea->clearChildren();
        m_contentArea->addChild(m_tabs[index].content);
        
        // タブボタンの状態を更新
        updateTabButtons();
    }
    
private:
    SharedPtr<Node> createTabButton(const String& title, size_t index)
    {
        auto button = Node::Create();
        auto bg = button->emplaceComponent<RectRenderer>();
        auto label = button->emplaceComponent<Label>();
        label->text = title;
        
        auto trigger = button->emplaceComponent<EventTrigger>();
        trigger->setOnClicked([this, index] {
            selectTab(index);
        });
        
        return button;
    }
};
```

#### 必要性
- **画面スペースの有効活用**: 複数のビューを切り替えて表示
- **組織化**: 関連する内容をグループ化

### 3.3 Dropdown（ドロップダウン）

#### 実装提案

```cpp
class Dropdown : public ComponentBase
{
    struct Option
    {
        String text;
        String value;
        Optional<Texture> icon;
        bool enabled = true;
    };
    
    Property<size_t> selectedIndex{0};
    Property<String> placeholder{U"選択してください"};
    
    Array<Option> m_options;
    bool m_isOpen = false;
    
    SharedPtr<Node> m_button;
    SharedPtr<Node> m_dropdownList;
    SharedPtr<Label> m_selectedLabel;
    
    std::function<void(size_t, const String&)> m_onSelectionChanged;
    
public:
    void setOptions(const Array<String>& options)
    {
        m_options.clear();
        for (const auto& text : options)
        {
            m_options.push_back({text, text});
        }
        updateDisplay();
    }
    
    void onMount() override
    {
        // メインボタン
        m_button = Node::Create();
        m_selectedLabel = m_button->emplaceComponent<Label>();
        
        // ドロップダウンアイコン
        auto icon = m_button->emplaceComponent<Label>();
        icon->text = U"▼";
        icon->alignment = Vec2{1.0, 0.5};
        
        auto trigger = m_button->emplaceComponent<EventTrigger>();
        trigger->setOnClicked([this] { toggleDropdown(); });
        
        m_node->addChild(m_button);
        
        // ドロップダウンリスト（初期は非表示）
        m_dropdownList = createDropdownList();
        m_dropdownList->setVisible(false);
    }
    
    void toggleDropdown()
    {
        m_isOpen = !m_isOpen;
        m_dropdownList->setVisible(m_isOpen);
        
        if (m_isOpen)
        {
            // クリック外で閉じる処理を登録
            getCanvas()->setModalOverlay(m_dropdownList, [this] {
                m_isOpen = false;
                m_dropdownList->setVisible(false);
            });
        }
    }
};
```

#### 必要性
- **選択UI**: 多数の選択肢から1つを選ぶ標準的な方法
- **スペース効率**: 選択時のみ選択肢を表示

## 4. フィードバック・情報表示系

### 4.1 ProgressBar（プログレスバー）

#### 実装提案

```cpp
class ProgressBar : public ComponentBase
{
    Property<double> value{0.0};  // 0.0 ~ 1.0
    Property<ProgressStyle> style{ProgressStyle::Linear};
    Property<bool> indeterminate{false};
    Property<bool> showPercentage{false};
    
    SharedPtr<RectRenderer> m_background;
    SharedPtr<RectRenderer> m_fill;
    SharedPtr<Label> m_percentageLabel;
    
    // インデターミネートアニメーション用
    double m_animationTime = 0.0;
    
public:
    void update(double deltaTime) override
    {
        if (indeterminate.get())
        {
            m_animationTime += deltaTime;
            updateIndeterminateAnimation();
        }
        else
        {
            updateDeterminateProgress();
        }
        
        if (showPercentage.get() && m_percentageLabel)
        {
            m_percentageLabel->text = Format(U"{}%", 
                static_cast<int>(value.get() * 100));
        }
    }
    
private:
    void updateDeterminateProgress()
    {
        const double clampedValue = std::clamp(value.get(), 0.0, 1.0);
        const auto barSize = m_node->getSize();
        
        if (style.get() == ProgressStyle::Linear)
        {
            m_fill->node()->setSize({barSize.x * clampedValue, barSize.y});
        }
        else if (style.get() == ProgressStyle::Circular)
        {
            // 円形プログレスバーの描画
            updateCircularProgress(clampedValue);
        }
    }
};
```

#### 必要性
- **進捗表示**: 長時間の処理やダウンロードの進行状況を表示
- **ユーザー体験**: 処理が進行中であることを明確に示す

### 4.2 Tooltip（ツールチップ）

#### 実装提案

```cpp
class Tooltip : public ComponentBase
{
    Property<String> text{U""};
    Property<TooltipPlacement> placement{TooltipPlacement::Auto};
    Property<double> delay{0.5};  // 表示までの遅延（秒）
    
    SharedPtr<Node> m_tooltipNode;
    double m_hoverTime = 0.0;
    bool m_isShowing = false;
    
public:
    void onHoverChanged(bool isHovered) override
    {
        if (isHovered)
        {
            m_hoverTime = 0.0;
        }
        else
        {
            hideTooltip();
        }
    }
    
    void update(double deltaTime) override
    {
        if (m_node->isHovered() && !m_isShowing)
        {
            m_hoverTime += deltaTime;
            
            if (m_hoverTime >= delay.get())
            {
                showTooltip();
            }
        }
    }
    
private:
    void showTooltip()
    {
        if (text.get().isEmpty()) return;
        
        m_tooltipNode = createTooltipNode();
        
        // 位置を計算
        const Vec2 position = calculateTooltipPosition();
        m_tooltipNode->setPosition(position);
        
        // オーバーレイレイヤーに追加
        getCanvas()->addToOverlay(m_tooltipNode);
        
        m_isShowing = true;
    }
    
    Vec2 calculateTooltipPosition()
    {
        const auto nodeRect = m_node->getGlobalBounds();
        const auto tooltipSize = m_tooltipNode->getSize();
        
        switch (placement.get())
        {
        case TooltipPlacement::Top:
            return nodeRect.center().movedBy(-tooltipSize.x / 2, 
                                            -nodeRect.h / 2 - tooltipSize.y - 5);
        case TooltipPlacement::Bottom:
            return nodeRect.center().movedBy(-tooltipSize.x / 2, 
                                            nodeRect.h / 2 + 5);
        // ... 他の配置オプション
        }
    }
};
```

#### 必要性
- **追加情報の提供**: UIを圧迫せずに補足説明を表示
- **アクセシビリティ**: アイコンボタンなどの説明に必須

### 4.3 Modal/Dialog（モーダルダイアログ）

#### 実装提案

```cpp
class Modal : public ComponentBase
{
    Property<String> title{U""};
    Property<bool> closeOnBackdropClick{true};
    Property<bool> showCloseButton{true};
    
    SharedPtr<Node> m_backdrop;
    SharedPtr<Node> m_dialog;
    SharedPtr<Node> m_contentArea;
    
    std::function<void()> m_onClose;
    
public:
    static SharedPtr<Modal> Show(SharedPtr<Node> content, 
                                const ModalOptions& options = {})
    {
        auto modal = std::make_shared<Modal>();
        modal->configure(options);
        modal->setContent(content);
        
        // キャンバスのモーダルレイヤーに追加
        Scene::GetCanvas()->showModal(modal);
        
        return modal;
    }
    
    void onMount() override
    {
        // 背景（半透明オーバーレイ）
        m_backdrop = Node::Create();
        auto backdropRect = m_backdrop->emplaceComponent<RectRenderer>();
        backdropRect->color = PropertyValue<ColorF>{ColorF{0, 0, 0, 0.5}};
        
        if (closeOnBackdropClick.get())
        {
            auto trigger = m_backdrop->emplaceComponent<EventTrigger>();
            trigger->setOnClicked([this] { close(); });
        }
        
        // ダイアログ本体
        m_dialog = Node::Create();
        m_dialog->setConstraint(BoxConstraint{
            .maxSize = {600, 400},
            .padding = LRTB{20}
        });
        
        setupDialogStructure();
    }
    
    void close()
    {
        if (m_onClose)
        {
            m_onClose();
        }
        
        // アニメーション後に削除
        animateOut([this] {
            getCanvas()->removeModal(shared_from_this());
        });
    }
};
```

#### 必要性
- **重要な操作の確認**: 削除確認などのクリティカルな操作
- **フォーカス管理**: ユーザーの注意を特定のタスクに集中

## 5. 実装優先順位の提案

### 第1段階（最優先）
1. **Button** - 最も基本的で使用頻度が高い
2. **Checkbox** - フォームの基本要素
3. **TextBox/TextAreaの改善** - 現在あるが機能不足（プレースホルダー、検証など）
4. **ScrollView** - 長いコンテンツ表示に必須

### 第2段階（高優先度）
5. **Dropdown** - 選択UIの標準
6. **Slider** - 数値入力の直感的な方法
7. **ProgressBar** - 非同期処理のフィードバック
8. **Tooltip** - UXの向上

### 第3段階（中優先度）
9. **TabView** - 複雑なUIの整理
10. **Modal/Dialog** - 重要な操作の確認
11. **RadioButton** - 単一選択UI
12. **Toggle/Switch** - ON/OFF設定

## 6. 共通機能の不足

### 6.1 フォーカス管理システム

```cpp
class FocusScope
{
    Array<WeakPtr<Node>> m_focusableNodes;
    WeakPtr<Node> m_currentFocus;
    
    void moveFocus(FocusDirection direction);
    void setFocus(SharedPtr<Node> node);
    SharedPtr<Node> findNextFocusable(FocusDirection direction);
};
```

### 6.2 テーマシステム

```cpp
class Theme
{
    HashMap<String, ColorF> m_colors;
    HashMap<String, double> m_dimensions;
    HashMap<String, Font> m_fonts;
    
    static Theme& Light();
    static Theme& Dark();
    
    void apply(SharedPtr<Node> root);
};
```

### 6.3 アニメーション統合

```cpp
class Transition
{
    static void FadeIn(SharedPtr<Node> node, double duration = 0.3);
    static void SlideIn(SharedPtr<Node> node, Direction from, double duration = 0.3);
    static void ScaleIn(SharedPtr<Node> node, double duration = 0.3);
};
```

## 7. まとめ

NocoUIは強力な基盤を持っていますが、一般的なアプリケーション開発に必要な標準的なUIコンポーネントが不足しています。これらのコンポーネントを段階的に実装することで、以下の効果が期待できます：

1. **開発効率の向上**: 基本的なUIを構築する時間を大幅に削減
2. **一貫性の確保**: 標準化されたコンポーネントによる統一感のあるUI
3. **アクセシビリティ**: キーボード操作やスクリーンリーダー対応の標準化
4. **学習曲線の改善**: 他のUIフレームワークと同様のコンポーネント名と動作

特に、Button、Checkbox、ScrollView、Dropdownなどの基本コンポーネントは、ほぼすべてのアプリケーションで使用されるため、最優先で実装すべきです。