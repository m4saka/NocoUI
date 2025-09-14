# NocoUI for Siv3D

> [!WARNING]
> NocoUIは現在プレビュー版として試験的に公開されています。  
> 今後のアップデートで大規模な仕様変更が発生する可能性があるため、利用される場合はご注意ください。

NocoUIは、Siv3D向けのUIライブラリです。

ゲーム上で利用する複雑なUIのレイアウトをビジュアルエディタ(NocoEditor)上で編集できます。
作成したUIは、数行のソースコードを書くだけで簡単に利用できます。

![スクリーンショット(エディタ)](./docs/readme_example_editor.png)
![スクリーンショット(実行画面)](./docs/readme_example_runtime.gif)

※ 上記スクリーンショットのMain.cppおよび.nocoファイルのサンプルは[examples/PlayerNameDialog](./examples/PlayerNameDialog)内にあります。

## 基本的な使い方

NocoEditor上で.nocoファイルを作成し、それを下記のようなソースコードで読み込んで利用します。

```cpp
#include <Siv3D.hpp>
#include <NocoUI.hpp>

void Main()
{
    // NocoUIを初期化
    noco::Init();

    // ファイルからCanvasを読み込み
    const auto canvas = noco::Canvas::LoadFromFile(U"canvas.noco");

    while (System::Update())
    {
        // 更新処理
        canvas->update();

        // 描画
        canvas->draw();
    }
}
```

## NocoUIの特徴

NocoUIの"Noco"という名前は、下記の2つの特徴が由来となっています。

- ノーコード (**No** **Co**de)
    - UIの見た目をビジュアルエディタ上で編集できます
    - 見た目のための値(座標、色、画像ファイル名、テキストなど)を、.nocoファイル(JSON形式)として保存できます
        - ソースコードをシンプルに保つことができます

- ノード＆コンポーネントUI (**No**de & **Co**mponent-based UI)
    - ノード(`Node`): 座標とサイズを持つ要素。コンポーネントを複数持つことができます
    - コンポーネント(`Component`): ノードへ追加することで、各種処理や描画ができます

## プロジェクトへの導入方法

### Windows(Visual Studio)の場合

下記手順で、ソリューション内にNocoUI.vcxprojを追加し、プロジェクトの依存関係を設定してください。  
例として、「MySiv3DProject」という名前で既存のSiv3Dプロジェクトを作成しているとします。

1. NocoUIをダウンロードし、ソリューション内の任意のディレクトリにNocoUIのフォルダを配置します

2. Visual Studioのソリューション エクスプローラー上で、ソリューション(先頭の項目)を右クリックし、「追加＞既存のプロジェクト」をクリックします  
    ![](./docs/readme_setup_1.png)

3. 先ほど配置したNocoUIのフォルダ内のNocoUI.vcxprojを選択します

4. ソリューション内に「NocoUI」プロジェクトが追加されます  
    ![](./docs/readme_setup_2.png)

5. 「MySiv3DProject」プロジェクトを右クリックし、「ビルドの依存関係＞プロジェクトの依存関係」をクリックします  
    ![](./docs/readme_setup_3.png)

6. ウィンドウ上にある「NocoUI」にチェックを入れてOKで閉じます  
    ![](./docs/readme_setup_4.png)

7. 「MySiv3DProject」プロジェクトの「参照」を右クリックし、「参照の追加」をクリックします  
    ![](./docs/readme_setup_5.png)

8. ウィンドウ上にある「NocoUI」にチェックを入れてOKで閉じます  
    ![](./docs/readme_setup_6.png)

9. 「MySiv3DProject」プロジェクト内のstdafx.hに「`# include <NocoUI.hpp>`」を追記します  
    (※必須ではないですが、ビルド速度向上のため推奨)

10. これで、プロジェクト上でNocoUIを利用できるようになりました

> [!NOTE]
> プロジェクト内に直接NocoUIのcppファイル・hppファイルを追加しても利用可能ですが、上記手順で導入した方が今後のNocoUIのアップデートをNocoUIフォルダ全体の上書きのみで完結できるため便利です。

### macOS/Linuxの場合

プロジェクト内の任意のディレクトリにNocoUIフォルダを配置してください。

その上で、NocoUIのincludeフォルダをインクルードディレクトリに追加し、srcフォルダ内のcppファイルがビルド対象となるようプロジェクトに手動で追加してください。

※ Linux向けに、現在CMakeを利用した導入方法を準備中です

## コンポーネントの種類

下記のコンポーネントが標準搭載されています。  
各コンポーネントの詳しい機能は、エディタ上にコンポーネントを挿入し、プロパティ名にマウスカーソルを当てることで説明が表示されます。

### 主要なコンポーネント

- `Sprite`: 画像を描画します。9スライス・フレームアニメーション・スクロールアニメーションなどに対応しています
- `RectRenderer`: 長方形を描画します。角丸四角形や、枠線描画、グラデーション色などに対応しています
- `TextBox`: 1行のテキストを入力できます。`TextBox`自体は背景を描画しないため、`RectRenderer`と組み合わせて利用します
- `Label`: 文字を描画します。フォント指定や下線、左揃え/中央揃え/右揃え、上揃え/中央揃え/下揃え、フォントサイズの自動縮小、アウトライン・影などに対応しています
- `Tween`: 位置・スケール・回転・色をアニメーションします

### その他のコンポーネント

- `ShapeRenderer`: Siv3DのShape2Dに定義されている各種図形を描画します
- `TextArea`: 複数行のテキストを入力できます。`TextArea`自体は背景を描画しないため、`RectRenderer`と組み合わせて利用します
- `Toggle`: クリックの度にvalueプロパティ(bool)の値が切り替わります。チェックボックスの実装に利用できます
- `TextureFontLabel`: あらかじめ用意したフォントテクスチャ画像をもとに、文字を描画します
- `EventTrigger`: マウスイベントに応じてイベントを発火します。イベントはプログラム上から取得できます(`Canvas::isEventFiredWithTag`, `Canvas::getFiredEvent(s)WithTag`, `Canvas::getFiredEventsAll`)
- `CursorChanger`: ホバー中のマウスカーソルの見た目を変更します
- `UISound`: マウスイベントに応じて効果音を鳴らします

## 子レイアウト(Children Layout)

子レイアウトは、`InlineRegion`が指定された子ノードの並べ方を決めるものです。

下記の3種類があります。

- `FlowLayout`
    - 子ノードを左上から順番に配置します
    - 右端まで到達した場合、次の行へ折り返します

- `HorizontalLayout`
    - 子ノードを左から右の方向に並べて配置します
    - 右端に到達しても、行を折り返しません

- `VerticalLayout`
    - 子ノードを上から下の方向に並べて配置します
    - 下端に到達しても、行を折り返しません

## リージョン(Region)

Regionは、自身のノードの領域を指定するものです。

下記の2種類があります。

- `AnchorRegion`
    - 親ノードの領域を基準に、ノードの位置とサイズを決定します
    - レイアウトは無視されます

- `InlineRegion`
    - レイアウトに従って、順番にノードを配置します

> [!WARNING]
> リージョンは`Node::setRegion()`で動的に変更が可能ですが、変更がある度にフレームの最後にCanvas全体のレイアウトの再配置が発生するため、頻繁に切り替えることは推奨されません。  
> アニメーションなど、プログラム上から頻繁に変更が必要な場合は、リージョンではなくトランスフォームを変更してください。

## トランスフォーム(Transform)

トランスフォームは、ノードの位置・サイズ・回転・スケール・色を変化させることができます。

- `translate`: ノードの位置を変化させます(Vec2)

- `scale`: ノードのスケールを変化させます(Vec2)

- `rotation`: ノードの回転を変化させます(double)

- `pivot`: ノードの回転・スケールの基準点を0～1の割合で指定します(Vec2)

- `color`: ノードの色を変化させます(Color)

- `hitTestAffected`: ヒットテストの領域にも適用するかどうか(bool)

トランスフォームはノードのレイアウトに影響しないため、レイアウトの再配置が発生せず、高速に変更できます。

## ヒットテスト

ヒットテストは、マウスカーソルがどのノードに当たっているかどうかを判定する仕組みです。

ノード毎に、下記のプロパティでヒットテストの挙動を制御できます。

- `isHitTarget`: ヒットテストの対象にするかどうか(bool)
    - falseに設定すると、親ノードの結果を継承します

- `hitPadding`: ヒットテストの領域の拡縮(LRTB)
    - ノードのヒットテスト領域を指定ピクセル分だけ拡大・縮小します

## ステート

各ノードは、インタラクションステートとスタイルステートの2種類の組み合わせで構成されるステートを持ちます。

プロパティ名の右クリックメニューから「ステート毎に値を変更...」を選択すると、ステート毎のプロパティ値を設定できます。  
ステート毎の値が設定されているプロパティには、プロパティ名に黄色い下線が表示されます。

![ステート毎に値を変更](./docs/readme_state_editor.png)

### インタラクションステート(Interaction State)

マウス操作に応じて自動的に切り替わるステートです。

下記の4種類があり、いずれかの状態を持ちます。

- `Default`: 通常時の状態
- `Hovered`: マウスホバー中
- `Pressed`: マウス押下中
- `Disabled`: 無効

ノードの`interactable`プロパティをfalseに設定すると、ノードが無効化され、`Disabled`ステートになります。

子ノードのインタラクションステートを親に継承するには、下記のプロパティを有効にします。

- `inheritChildrenHover`: 子ノードのホバー状態を親に継承します
- `inheritChildrenPress`: 子ノードの押下状態を親に継承します

### スタイルステート(Style State)

スタイルステートは、ノードの見た目を切り替えるために自由に利用できるステートです。  
文字列で指定でき、1つのノードに対して1つのスタイルステートを持ちます。

祖先ノードのスタイルステートが再帰的に継承されます。自身に近いノードが優先して適用されます。

※ 子ノードのスタイルステートは、親に継承されません。

#### コンポーネントによるスタイルステートの上書き

コンポーネントによって、スタイルステートが上書きされる場合があります。下記のコンポーネントが該当します。

- `TextBox`/`TextArea`コンポーネント
    - "`focused`": フォーカス中に上書きされます
    - "`unfocused`": フォーカスされていない時に上書きされます
- `Toggle`コンポーネント
    - "`on`": valueプロパティがtrueの時に上書きされます
    - "`off`": valueプロパティがfalseの時に上書きされます

これらのスタイルステートに応じた値をプロパティに設定しておくことで、コンポーネントの状態に応じて見た目を切り替えられます。

## パラメータ(Params)

パラメータは、各コンポーネントのプロパティ値を外部から動的に変更できる仕組みです。
テキストのみ異なるUIなど、似たようなUIを使い回す際に便利です。

![パラメータの例](./docs/readme_params.png)

プロパティ名の右クリックメニューから「参照パラメータを選択...」を選択すると、パラメータ参照の設定ができます。

![参照パラメータを選択](./docs/readme_param_editor.png)

パラメータ参照を指定しておくことで、下記のようにソースコード上から簡単に値を設定できるようになります。

```cpp
// Before: パラメータ不使用の場合、ノードからコンポーネントを取得する必要がある
const auto titleNode = canvas->findByName(U"Title");
const auto titleLabel = titleNode->getComponent<noco::Label>();
titleLabel->setText(U"新たな仲間が参戦！");

const auto messageNode = canvas->findByName(U"Message");
const auto messageLabel = messageNode->getComponent<noco::Label>();
messageLabel->setText(U"仲間の名前を入力してください");
```
　　↓
```cpp
// After: パラメータ参照を利用することで、簡単に値をセットできる
canvas->setParamValue(U"dialogTitle", U"新たな仲間が参戦！");
canvas->setParamValue(U"messageText", U"仲間の名前を入力してください");
```

## 独自コンポーネントの利用(高度な使い方)

独自コンポーネントを作成するには、下記の2通りの方法があります。

### `SerializableComponentBase`を継承してエディタ上で使用

独自コンポーネントをエディタ上で使用するには、`noco::SerializableComponentBase`を継承してコンポーネントを作成します。

```cpp
// enumも使用可能
enum class IconType
{
    None,
    Home,
    Settings,
    Search,
};

// 独自コンポーネント
class CustomButton : public noco::SerializableComponentBase
{
private:
    Property<String> m_text{ U"text", U"" };
    SmoothProperty<Color> m_backgroundColor{ U"backgroundColor", Palette::White };
    SmoothProperty<Color> m_textColor{ U"textColor", Palette::Black };
    SmoothProperty<double> m_fontSize{ U"fontSize", 16.0 };
    Property<bool> m_enabled{ U"enabled", true };
    Property<IconType> m_iconType{ U"iconType", IconType::None };

public:
    CustomButton()
        : noco::SerializableComponentBase{
            {
                m_text,
                m_backgroundColor,
                m_textColor,
                m_fontSize,
                m_enabled,
                m_iconType
            } }
    {
    }

    void update() override
    {
        // 更新処理
    }

    void draw() const override
    {
        // 描画処理
    }
};
```

その上で、NocoEditorの実行ファイルから見て`Custom/Components`ディレクトリへコンポーネントスキーマ定義をJSON形式で記述したものを配置します。
例えば、`Custom/Components/CustomButton.json`に下記の内容を配置します。

```json
{
	"type": "CustomButton",
	"properties": [
		{
			"name": "text",
			"editType": "Text",
			"defaultValue": "ボタン",
			"tooltip": "ボタンに表示するテキスト"
		},
		{
			"name": "backgroundColor",
			"editType": "Color",
			"defaultValue": [255, 255, 255, 255],
			"tooltip": "ボタンの背景色"
		},
		{
			"name": "textColor",
			"editType": "Color",
			"defaultValue": [0, 0, 0, 255],
			"tooltip": "テキストの色"
		},
		{
			"name": "padding",
			"editType": "LRTB",
			"defaultValue": [2, 2, 2, 2],
			"tooltip": "内側の余白"
		},
		{
			"name": "fontSize",
			"editType": "Number",
			"defaultValue": 16,
			"tooltip": "テキストのフォントサイズ",
			"dragValueChangeStep": 1.0
		},
		{
			"name": "enabled",
			"editType": "Bool",
			"defaultValue": true,
			"tooltip": "ボタンが有効かどうか"
		},
		{
			"name": "iconType",
			"editType": "Enum",
			"defaultValue": "None",
			"enumCandidates": ["None", "Home", "Settings", "Search"],
			"tooltip": "アイコンの種類"
		}
	]
}
```

すると、下記のようにNocoEditor上で独自コンポーネントを追加できるようになります。

![独自コンポーネントの追加](./docs/readme_custom_component_editor.png)

プログラム上で独自コンポーネントを利用するには、下記のようにComponentFactoryを取得して独自コンポーネントを登録し、`Canvas::LoadFromFile`の第2引数に渡します。
```cpp
// 標準コンポーネントを含むComponentFactoryを取得
ComponentFactory factory = ComponentFactory::GetBuiltinFactory();

// 独自コンポーネントを登録
factory.registerComponentType<CustomButton>(U"CustomButton");

// Canvasを読み込む際、factoryを渡す
const auto canvas = noco::Canvas::LoadFromFile(U"canvas.noco", factory);
```

### `ComponentBase`を継承してプログラム上で使用

独自コンポーネントをプログラム上でのみ使用する場合は、`noco::ComponentBase`を継承してコンポーネントを作成します。

なお、この方法の場合は、ステートに応じた値変化が必要なければ`Property`や`SmoothProperty`を利用せずに実装しても構いません。

```cpp
// 独自コンポーネント
class CustomButton : public noco::ComponentBase
{
private:
    String m_text = U"";
    Color m_backgroundColor = Palette::White;

public:
    CustomButton(StringView text, const Color& backgroundColor)
        : noco::ComponentBase{ {} } // プロパティが必要なければ空の配列を渡す
        , m_text{ text }
        , m_backgroundColor{ backgroundColor }
    {
    }

    void update() override
    {
        // 更新処理
    }

    void draw() const override
    {
        // 描画処理
    }
};
```

プログラム上では、emplaceComponentまたはaddComponentでノードへ追加して利用します。

```cpp
node->emplaceComponent<CustomButton>(U"OK", Palette::White);
// または
node->addComponent(std::make_shared<CustomButton>(U"OK", Palette::White));
```

## Editor上での独自フォントのプレビュー(高度な使い方)

NocoEditor上で独自フォントをプレビューするには、`Custom/FontAssets`ディレクトリへフォントアセット定義(.json)を配置します。  
Labelコンポーネント等の`fontAssetName`プロパティに指定することで、エディタ上でフォントをプレビューできます。

なお、これはエディタプレビュー用のアセット定義であり、ユーザープログラム上では自前でSiv3Dの`FontAsset::Register()`を実行してフォントアセットを登録する必要があります。

### フォントファイル(.ttf/.otf)の場合

例えば、`Custom/FontAssets/MyFont.json`と`Custom/FontAssets/MyFont.ttf`を配置し、フォントアセット定義(.json)に下記の内容を記述します。

```json
{
  "fontAssetName": "MyFont",
  "fontSize": 32,
  "method": "MSDF",
  "style": "Bold",
  "source": {
    "type": "File",
    "path": "MyFont.ttf"
  }
}
```

### Siv3Dの組み込みフォントの場合

Siv3Dの組み込みフォントを利用する場合は、"`type`"に"`Typeface`"を指定し、"`typeface`"にSiv3DのTypefaceの列挙子名を指定します。

```json
{
  "fontAssetName": "MyFont",
  "fontSize": 32,
  "method": "MSDF",
  "style": "Default",
  "source": {
    "type": "Typeface",
    "typeface": "Bold"
  }
}
```

## ライセンス・外部依存ライブラリ

本ライブラリは MIT License で提供されます。  
また、ライブラリ内に [magic_enum](https://github.com/Neargye/magic_enum) (MIT License)を含んでいます。

`noco::Init()`を実行することで、Siv3DのLicenseManagerへ NocoUI および magic_enum のライセンス表記が自動登録されます。  
そのため、Siv3DのLicenseManager(F1キー)を無効化していなければ、利用時にライセンス表記に関して特別な対応は不要です。
