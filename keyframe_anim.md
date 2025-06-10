# NocoUI キーフレームアニメーション機能 調査・考察レポート

## 1. 既存のアニメーション機能の調査結果

### 1.1 現在のアニメーションシステムの概要

NocoUIには既に洗練されたアニメーションシステムが実装されています。主な特徴は以下の通りです：

#### PropertyValue<T>システム
- 各プロパティが8つの状態値を保持（通常、ホバー、押下、無効、選択時の各状態）
- `smoothTime`パラメータによる状態遷移アニメーション
- インタラクション状態に基づく自動的な値の切り替え

#### Smoothing<T>クラス
- `Math::SmoothDamp`を使用した物理ベースの補間
- 現在値と速度を保持し、滑らかな遷移を実現
- Vec2、ColorF、LRTBなど様々な型をサポート

#### Property<T>とSmoothProperty<T>
- `Property<T>`: 即座に値が変わる通常プロパティ
- `SmoothProperty<T>`: Smoothingを内部で使用し、滑らかに遷移するプロパティ

#### TransformEffect
- Nodeの視覚的な変換を制御
- position、scale、pivotのSmoothPropertyを持つ
- 毎フレームupdate()で更新

### 1.2 アニメーション更新フロー

1. **Canvas::update()** → ホバー状態の検出
2. **Node::updateInteractionState()** → インタラクション状態の更新と伝播
3. **Node::update()** → TransformEffectとコンポーネントの更新
4. **Node::postLateUpdate()** → 全プロパティのSmoothing更新

### 1.3 既存システムの強み

- **宣言的**: PropertyValueで状態ごとの値を宣言的に定義
- **自動化**: インタラクション状態に応じて自動的にアニメーション
- **統一性**: 全てのプロパティで同じ仕組みを使用
- **パフォーマンス**: 必要なプロパティのみ更新

## 2. キーフレームアニメーションの要件分析

### 2.1 基本要件

1. **時間軸ベースのアニメーション**: 特定の時間にプロパティ値を設定
2. **補間方法の選択**: 線形、イージング、ベジェなど
3. **複数プロパティの同時制御**: 位置、色、スケールなどを同時にアニメーション
4. **ループ・リピート機能**: アニメーションの繰り返し再生
5. **エディタ統合**: NocoEditorでの視覚的な編集

### 2.2 既存システムとの統合要件

1. **PropertyValueとの共存**: インタラクション状態アニメーションと併用可能
2. **Smoothingとの調和**: 既存の補間システムとの整合性
3. **Componentベース**: 既存のアーキテクチャに準拠

## 3. 実装方針の考察（複数パターン）

### パターン1: AnimationComponentベース

#### 概要
専用のAnimationComponentを作成し、キーフレームアニメーションを管理する方式。

```cpp
class AnimationComponent : public ComponentBase
{
public:
    struct Keyframe
    {
        double time;
        PropertyValueVariant value;  // 様々な型の値を保持
        EasingType easing;
    };

    struct Track
    {
        String targetPropertyPath;  // "transform.position.x"のようなパス
        Array<Keyframe> keyframes;
    };

private:
    Array<Track> m_tracks;
    double m_duration;
    double m_currentTime;
    AnimationState m_state;  // Playing, Paused, Stopped
    LoopType m_loopType;
};
```

#### メリット
- 既存のComponentシステムに自然に統合
- エディタでの管理が容易
- 複数のアニメーションを1つのNodeに付与可能

#### デメリット
- プロパティへのアクセスにリフレクション的な仕組みが必要
- パフォーマンスオーバーヘッドの懸念

#### 実装詳細
1. PropertyPathResolver: 文字列パスから実際のプロパティにアクセス
2. KeyframeInterpolator: キーフレーム間の補間を担当
3. AnimationClip: アニメーションデータの保存・読み込み

### パターン2: PropertyAnimatorベース

#### 概要
各PropertyにAnimator機能を組み込む方式。

```cpp
template<typename T>
class AnimatedProperty : public SmoothProperty<T>
{
    struct AnimationData
    {
        Array<std::pair<double, T>> keyframes;
        Array<EasingType> easings;
        double duration;
        bool isPlaying;
        double currentTime;
    };

private:
    Optional<AnimationData> m_animation;

public:
    void setKeyframes(const Array<std::pair<double, T>>& keyframes);
    void play();
    void stop();
    void update(double deltaTime) override;
};
```

#### メリット
- 型安全性が高い
- 既存のPropertyシステムを自然に拡張
- パフォーマンスが良好

#### デメリット
- 各プロパティタイプごとに実装が必要
- 複数プロパティの同期が複雑

#### 実装詳細
1. AnimatedPropertyインターフェース: 共通の制御メソッド
2. AnimationController: 複数のAnimatedPropertyを同期制御
3. KeyframeEditor: プロパティごとのキーフレーム編集UI

### パターン3: TimelineSystemベース

#### 概要
独立したTimelineシステムを構築し、Nodeとは別に管理する方式。

```cpp
class Timeline
{
public:
    struct AnimationTarget
    {
        WeakPtr<Node> node;
        String componentType;
        String propertyName;
    };

    struct AnimationTrack
    {
        AnimationTarget target;
        Array<KeyframeVariant> keyframes;
    };

private:
    Array<AnimationTrack> m_tracks;
    double m_duration;
    double m_currentTime;
    TimelineState m_state;
};

class TimelineManager
{
private:
    HashMap<String, SharedPtr<Timeline>> m_timelines;
    Array<ActiveTimeline> m_activeTimelines;

public:
    void playTimeline(const String& name, SharedPtr<Node> targetRoot);
    void update(double deltaTime);
};
```

#### メリット
- 複雑なシーケンスアニメーションに対応
- リソース管理が明確
- 再利用性が高い

#### デメリット
- 既存システムとの統合が複雑
- メモリ使用量が増加

#### 実装詳細
1. TimelineResource: アニメーションデータの外部ファイル化
2. TimelineEditor: 専用のタイムライン編集UI
3. TimelineBlender: 複数タイムラインのブレンド機能

### パターン4: ハイブリッドアプローチ

#### 概要
上記パターンの良い点を組み合わせた統合的なアプローチ。

```cpp
// 基本的なキーフレームアニメーションはPropertyレベルで実装
template<typename T>
class KeyframableProperty : public SmoothProperty<T>
{
    // パターン2の実装
};

// 複雑なアニメーションはComponentで管理
class AnimationControllerComponent : public ComponentBase
{
    // アニメーショングループの管理
    // トリガー条件の設定
    // ブレンド機能
};

// エディタ用のTimelineシステム
class EditorTimeline
{
    // 視覚的な編集のためのデータ構造
    // AnimationControllerComponentへのエクスポート
};
```

#### メリット
- 柔軟性が高い
- 用途に応じて最適な方法を選択可能
- 段階的な実装が可能

#### デメリット
- システムが複雑化
- 学習コストが高い

## 4. 推奨実装方針

### 第1フェーズ: PropertyAnimatorベース（パターン2）の実装

理由：
1. 既存システムとの親和性が最も高い
2. 型安全性を保ちながら実装可能
3. パフォーマンスへの影響が最小限

実装手順：
1. `KeyframableProperty<T>`テンプレートクラスの作成
2. 主要な型（double、Vec2、ColorF）への対応
3. AnimationControllerの実装
4. 基本的なイージング関数の実装

### 第2フェーズ: AnimationComponentの追加

理由：
1. より複雑なアニメーションニーズに対応
2. エディタでの管理を容易に

実装手順：
1. AnimationComponentクラスの作成
2. PropertyPathResolverの実装
3. NocoEditorへの統合

### 第3フェーズ: TimelineSystemの検討

必要に応じて、より高度なアニメーション機能として実装を検討。

## 5. 技術的課題と解決策

### 5.1 プロパティアクセスの問題

**課題**: 文字列ベースのプロパティパスから実際のプロパティへのアクセス

**解決策**:
```cpp
class PropertyRegistry
{
    template<typename T>
    void registerProperty(const String& path, Property<T>* prop);
    
    PropertyBase* getProperty(const String& path);
};
```

### 5.2 型の多様性への対応

**課題**: 様々な型のプロパティを統一的に扱う

**解決策**:
```cpp
using PropertyValueVariant = std::variant<
    double, Vec2, Vec3, Vec4,
    ColorF, LRTB, String
>;
```

### 5.3 パフォーマンスの最適化

**課題**: 大量のアニメーションプロパティの更新コスト

**解決策**:
1. アクティブなアニメーションのみ更新
2. 空間的局所性を考慮したメモリレイアウト
3. SIMDを使用した補間計算の最適化

### 5.4 エディタ統合

**課題**: NocoEditorでの直感的な編集インターフェース

**解決策**:
1. タイムラインUIコンポーネントの開発
2. ドラッグ&ドロップでのキーフレーム編集
3. リアルタイムプレビュー機能

## 6. API設計案

### 基本的な使用例

```cpp
// コードでの使用
auto node = Node::Create();
auto rect = node->emplaceComponent<RectRenderer>();

// アニメーションの設定
rect->color.setKeyframes({
    {0.0, ColorF{1, 0, 0}},    // 0秒: 赤
    {1.0, ColorF{0, 1, 0}},    // 1秒: 緑
    {2.0, ColorF{0, 0, 1}}     // 2秒: 青
});
rect->color.setEasing(EasingType::EaseInOutCubic);
rect->color.setLoopType(LoopType::PingPong);
rect->color.play();

// より複雑なアニメーション
auto animCtrl = node->emplaceComponent<AnimationController>();
animCtrl->addAnimation("fadeIn", {
    {"opacity", {{0.0, 0.0}, {1.0, 1.0}}},
    {"scale", {{0.0, Vec2{0.5, 0.5}}, {1.0, Vec2{1.0, 1.0}}}}
});
animCtrl->play("fadeIn");
```

### エディタでの設定

1. InspectorでのAnimationセクション追加
2. タイムラインエディタウィンドウ
3. プリセットアニメーションライブラリ

## 7. 実装優先順位

1. **高優先度**
   - KeyframablePropertyの基本実装
   - 線形補間とイージング関数
   - play/stop/pauseの基本制御

2. **中優先度**
   - AnimationComponentの実装
   - NocoEditorへの基本的な統合
   - ループ・リバース機能

3. **低優先度**
   - 高度なイージング関数
   - タイムラインシステム
   - アニメーションのブレンド機能

## 8. まとめ

NocoUIの既存のアニメーションシステムは、インタラクション状態に基づく優れた設計となっています。キーフレームアニメーション機能は、この既存システムを拡張する形で実装することで、統一感のあるAPIと高いパフォーマンスを実現できます。

推奨される実装アプローチは、まずPropertyレベルでのキーフレーム機能を追加し、その後必要に応じてより高度な機能を段階的に実装していくことです。これにより、既存のコードベースへの影響を最小限に抑えながら、強力なアニメーション機能を提供できます。