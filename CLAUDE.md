# CLAUDE.md

このファイルは、リポジトリ内のコードを扱う際にClaudeCode(claude.ai/code)が参照するガイドラインです。

# NocoUI開発ガイド

## ビルドコマンド
- **ビルド** 
  `cmake -B editor/NocoEditor/build -S editor/NocoEditor && cmake --build editor/NocoEditor/build`
- **実行** 
  `cd editor/NocoEditor && ./NocoEditor`
- **デバッグビルド** 
  `cmake -DCMAKE_BUILD_TYPE=Debug`でビルドタイプを`Debug`に設定してからビルド

## コーディング規約
- C++20準拠
- インデントはタブ(幅:4)
- メンバ関数/変数は**camelCase**、クラス/フリー関数/staticメンバ関数は**PascalCase**
- 関数オブジェクトのprefixとして`fn`を付ける(例:`fnOpenFile`)
- 引数1つのコンストラクタには基本的に`explicit`を付ける
- ブロックの開始(`{`)の前で改行すること(いわゆるAllmanスタイル)
- メンバ変数には`m_`プレフィックス(例:`m_canvas`)
- 可能な限りSiv3Dの型と慣例を使用
- 真偽値にはYesNoテンプレートを採用(例:`ActiveYN`,`RefreshesLayoutYN`)
- プログラム上のコメントについて
  - コメントは日本語で記述する。ただし、NodeやComponentなどのプログラム上のキーワードは基本的にカタカナにせずそのまま記述する。例: `// Componentが持つ各Propertyを初期化`
  - アルファベットと日本語の間にスペースを入れないこと。

## ディレクトリ構成
- `include/NocoUI`: 公開APIヘッダ
- `src`: 実装コード
- `editor/NocoEditor`: UIエディタアプリ

## コアアーキテクチャ

### Node/Component
NocoUIは、階層化された**Node**に複数の**Component**を付与する構造を採用しています。これはUnityのGameObject–Componentパターンに近い設計です。

- **Node** 
  変換情報・制約・子ノード・インタラクション状態を保持する基礎エンティティ。
- **Components** 
 `RectRenderer`,`Label`,`Sprite`,`TextBox`などの機能モジュール。
- **shared_ptrの使用** 
  ノードとコンポーネントは`std::shared_ptr`でメモリ管理。ただし、`std::weak_ptr`を使用して循環参照を防止。
- **生成方法** 
 `node->emplaceComponent<T>(args)`を使用して追加。
- **拡張** 
  新規コンポーネントは`ComponentBase`または`SerializableComponentBase`を継承して実装。

### Property
NocoUIでは、各Componentに対して複数の**Property**が存在し、各々に対して**PropertyValue**型の値を設定します。
- **Property** 
  コンポーネントのプロパティを定義するための型。  
 `Property<T>`は型`T`の値を保持し、`PropertyValue<T>`を通じて動的に変化可能。  
  プロパティはコンポーネントの状態や外観を制御するために使用されます。
- **PropertyValue<T>** 
  `interactState`(Normal/Hovered/Pressed/Disabled/Selected)の各状態毎の値を保持し、実際の値はそれをもとに毎フレーム更新される。  
  平滑化時間`smoothTime`を設定することで、`interactState`の変化時になめらかに値を推移可能。

### BoxChildrenLayout
`LayoutVariant`で次の3種類から選択できます。

1. **FlowLayout**— 折り返し対応のフローレイアウト。  
2. **HorizontalLayout**— 単一行の水平配置。間隔指定可。  
3. **VerticalLayout**— 単一列の垂直配置。間隔指定可。  

### Constraint
`ConstraintVariant`で次の2種類から選択できます。

1. **BoxConstraint**— 余白とウェイトを指定できる固定/可変サイズ。親Nodeが持つ`boxChildrenLayout`に応じて整列される。  
2. **AnchorConstraint**— 親領域を基準にしたアンカー配置。  

### Canvas
- **Canvas** 
  ノード階層全体を管理するルートレンダリングコンテキスト。  
  ホバー中ノード、スクロール対象ノード、テキスト編集状態などのグローバル状態を保持。  
  イベント処理と座標変換を提供し、描画・更新ループのエントリポイントとなる。  

## NocoEditor
NocoEditorは、NocoUIのノードとコンポーネントを視覚的に編集するためのビジュアルエディタです。  
NocoEditor自体もNocoUIを使用して実装されています。  
Unityや他のゲームエンジンのエディタに似た設計で、UIの階層構造を直感的に構築・編集できます。

### 使用しているCanvas
- `m_canvas`: メインのUIプレビューキャンバス  
- `m_editorCanvas`: エディタUI(Hierarchy、Inspectorなど)  
- `m_editorOverlayCanvas`: コンテキストメニュー等のオーバーレイ  
- `m_dialogCanvas`/`m_dialogOverlayCanvas`: ダイアログ表示用

### 主要コンポーネント

#### Hierarchy(階層ビュー)
ノード階層をツリー形式で表示・操作するパネル
- **ノード管理** 
  - ツリー形式でのノード階層表示
  - ノードの追加・削除・複製
  - ドラッグ＆ドロップによる階層の再編成
  - 折りたたみ可能なツリー構造(▶/▼ アイコン)
- **選択機能** 
  - 単一/複数選択(Shiftキーで範囲選択)
  - 選択ノードのハイライト表示(オレンジ色)
- **コンテキストメニュー** 
  - 新規ノード作成(兄弟/子として)
  - カット・コピー・ペースト・複製
  - ノードの上下移動

#### Inspector(インスペクタ)
選択中のノードの詳細プロパティを編集するパネル
- **プロパティ編集** 
  - ノード名の編集
  - Constraintの設定
  - レイアウト設定(Flow/Horizontal/Vertical)
  - TransformEffectの編集
- **コンポーネント管理** 
  - コンポーネントの追加・削除
  - 各コンポーネントのプロパティ編集
  - インタラクティブなプロパティ値(Normal/Hovered/Pressed等)の設定
- **折りたたみ可能なUI** 
  各セクション(Constraint、Layout、Component等)は個別に折りたたみ可能

#### MenuBar(メニューバー)
エディタ上部の標準的なメニューバー(ファイル/編集/表示/ツール)

#### ContextMenu(コンテキストメニュー)
右クリックやボタンで表示されるポップアップメニュー
- スクリーンマスク機能(メニュー外クリックで閉じる)
- メニュー項目の種類：通常項目、チェック可能項目、セパレータ
- ホットキー表示とニーモニックキー対応

## プロパティ実装手順

新しいプロパティを追加する際は、以下の手順に従って実装してください。

### 1. ヘッダファイルでのプロパティ定義
対象のクラス(Component、TransformEffect、NodeSetting等)のヘッダファイルで`Property<T>`を定義します。
```cpp
// 例: RectRenderer.hpp
Property<ColorF> gradientTopColor{ ColorF{ 1.0, 1.0, 1.0, 1.0 } };
Property<ColorF> gradientBottomColor{ ColorF{ 1.0, 1.0, 1.0, 1.0 } };
Property<YesNo<ActiveYN>> gradientEnabled{ No };
```

### 2. コンストラクタでの初期化
SerializableComponentBaseを継承している場合は、コンストラクタで`initializeProperties`を呼び出します。
```cpp
// 例: RectRenderer.cpp
RectRenderer::RectRenderer()
{
    initializeProperties();
}
```

### 3. Main.cppでのツールチップ追加
NocoEditorのMain.cppで、新しいプロパティのツールチップ文言を追加します。
```cpp
// 例: Main.cpp内のsetToolTip関数付近
setToolTip(U"gradientEnabled", U"グラデーションを有効にするかどうか");
setToolTip(U"gradientTopColor", U"グラデーションの上部の色");
setToolTip(U"gradientBottomColor", U"グラデーションの下部の色");
```

### 4. シリアライズ実装(Component以外の場合)
TransformEffectやNodeSetting等、Component以外でプロパティを追加した場合は、対応するクラスの`toJSON()`と`readFromJSON()`メソッドでシリアライズ/デシリアライズ処理を実装する必要があります。

```cpp
// 例: TransformEffect.hpp内のtoJSON()メソッド
JSON toJSON() const
{
    JSON json;
    m_position.appendJSON(json);
    m_scale.appendJSON(json);
    m_pivot.appendJSON(json);
    m_appliesToHitTest.appendJSON(json);
    // 新しいプロパティの追加
    m_rotation.appendJSON(json);
    return json;
}

// readFromJSON()メソッド
void readFromJSON(const JSON& json)
{
    m_position.readFromJSON(json);
    m_scale.readFromJSON(json);
    m_pivot.readFromJSON(json);
    m_appliesToHitTest.readFromJSON(json);
    // 新しいプロパティの読み込み
    m_rotation.readFromJSON(json);
}
```

### 5. Inspector表示の調整(必要に応じて)
特定の条件でプロパティを表示/非表示にしたい場合は、Main.cppのInspector描画部分で制御します。
```cpp
// 例: 9スライスが無効時は関連プロパティを非表示
if (sprite->nineSliceEnabled.get() == Yes)
{
    // 9スライス関連のプロパティを表示
}
```

### プロパティ型の使い分け

#### Property<T>
- インタラクション状態(Normal/Hovered/Pressed/Disabled/Selected)に応じて値が変化するプロパティに使用
- 離散的な値の変化（スムーズな補間なし）
- bool、enum、String型などに適している
```cpp
Property<bool> visible{ true };
Property<String> fontAssetName{ U"Mplus1-Regular" };
```

#### SmoothProperty<T>
- 値が時間経過とともにスムーズに補間されるプロパティに使用
- `Math::SmoothDamp`をサポートする型（Vec2、ColorF、double、LRTB等）でのみ使用可能
- インタラクション状態の遷移をアニメーション化したい場合に使用
```cpp
SmoothProperty<ColorF> fillColor{ ColorF{ 1.0, 1.0, 1.0, 1.0 } };
SmoothProperty<double> fontSize{ 16.0 };
```

#### PropertyNonInteractive<T>
- インタラクション状態に関わらず値が一定のプロパティに使用
- メモリ効率が良く、シンプルな実装
- テキスト内容など、状態によって変化しない値に適している
```cpp
PropertyNonInteractive<String> text{ U"" };
```

### 注意事項
- プロパティ名は**camelCase**で記述
- 真偽値プロパティには`Property<bool>`を使用（`YesNo<T>`はコンストラクタ引数等でのみ使用）
- ツールチップは日本語で簡潔に記述
- Component以外の場合はシリアライズ実装を忘れずに

## コンポーネント実装手順

新しいコンポーネントを追加する際は、以下の手順に従って実装してください。

### 1. ヘッダファイルの作成(`include/NocoUI/Component/ComponentName.hpp`)
```cpp
#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"

namespace noco
{
    class ComponentName : public SerializableComponentBase
    {
    private:
        Property<CursorStyle> m_cursorStyle;
        PropertyNonInteractive<bool> m_enabled;

    public:
        explicit ComponentName(CursorStyle cursorStyle = CursorStyle::Default, EnabledYN enabled = EnabledYN::Yes)
            : SerializableComponentBase{ U"ComponentName", { &m_cursorStyle, &m_enabled } }
            , m_cursorStyle{ U"cursorStyle", cursorStyle }
            , m_enabled{ U"enabled", enabled.getBool() }
        {
        }

        void update(const std::shared_ptr<Node>& node) override;
    };
}
```

### 2. 実装ファイルの作成(`src/Component/ComponentName.cpp`)
```cpp
#include "NocoUI/Component/ComponentName.hpp"

namespace noco
{
    void ComponentName::update(const std::shared_ptr<Node>& node)
    {
        // 実装内容
    }
}
```

### 3. Component.hppへの追加
`include/NocoUI/Component/Component.hpp`の最後に以下を追加：
```cpp
#include "ComponentName.hpp"
```

### 4. シリアライズファクトリへの追加
`src/Serialization.cpp`の`CreateComponentFromJSON`関数に以下を追加：
```cpp
else if (type == U"ComponentName")
{
    auto component = std::make_shared<ComponentName>();
    if (!component->tryReadFromJSON(json))
    {
        Logger << U"[NocoUI warning] Failed to read ComponentName component from JSON";
        return nullptr;
    }
    return component;
}
```

### 5. NocoEditorへの統合(`editor/NocoEditor/Main.cpp`)

#### プロパティメタデータの追加
`InitPropertyMetadata()`関数内に追加：
```cpp
// ComponentName
metadata[PropertyKey{ U"ComponentName", U"propertyName" }] = PropertyMetadata{
    .tooltip = U"プロパティの説明",
    .tooltipDetail = U"詳細な説明（任意）",
};
```

#### コンポーネント追加メニューへの登録
`InspectorView`クラスの`m_componentButtonMenu`初期化部分に追加：
```cpp
MenuItem{ U"ComponentName を追加", U"", Key未使用キー, [this] { onClickAddComponent<ComponentName>(); } },
```

### 6. プロジェクトファイルの更新
プロジェクトファイル(`.vcxproj`、`.vcxproj.filters`)の更新は、ユーザーが別途手動で行います。

### 7. YN型の追加(必要に応じて)
新しいYesNoテンプレート型が必要な場合は`include/NocoUI/YN.hpp`に追加：
```cpp
using EnabledYN = YesNo<struct EnabledYN_tag>;
```

### オーバーライドメソッドの選択
- `update()`: 毎フレームの更新処理（奥の要素から手前に向かって実行）
- `updateInput()`: 入力処理（手前の要素から奥に向かって実行、入力のブロックが可能）
- `draw()`: 描画処理

### 実行順序の重要性
- **updateInput()**: 画面上の手前（上層）の要素から順に実行されます。これにより、手前の要素が入力をブロック（消費）した場合、奥の要素には入力が届きません。例えば、ダイアログやポップアップが入力を遮断する仕組みを実現できます。
- **update()**: 画面上の奥（下層）の要素から順に実行されます。これにより、親ノードの状態更新が子ノードより先に行われ、階層構造に沿った適切な更新順序が保証されます。