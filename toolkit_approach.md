# NocoUI ツールキットアプローチの提案

## 1. 概要

本ドキュメントでは、新しいコンポーネントクラスを追加する代わりに、既存のコンポーネントの組み合わせをパラメータ化されたテンプレート（ツールキット）として提供するアプローチについて詳しく考察します。

## 2. 基本コンセプト

### 2.1 問題認識

現在のNocoUIで「ボタン」を作成するには、以下のような手順が必要です：

```cpp
auto buttonNode = Node::Create();

// 背景
auto rect = buttonNode->emplaceComponent<RectRenderer>();
rect->color = PropertyValue<ColorF>{
    .defaultValue = Palette::Lightblue,
    .hoveredValue = Palette::Deepskyblue,
    .pressedValue = Palette::Dodgerblue
};
rect->cornerRadius = PropertyValue<Vec4>{Vec4{4, 4, 4, 4}};

// ラベル
auto label = buttonNode->emplaceComponent<Label>();
label->text = U"Click Me";
label->alignment = Vec2{0.5, 0.5};

// イベント
auto trigger = buttonNode->emplaceComponent<EventTrigger>();
trigger->setOnClicked([]{ /* ... */ });

// サイズ設定
buttonNode->setConstraint(BoxConstraint{.padding = LRTB{12, 12, 6, 6}});
```

これは毎回繰り返す必要があり、非効率的です。

### 2.2 ツールキットアプローチの解決策

既存のシリアライズ機能と変数システムを活用して、再利用可能なNode構造のテンプレートを定義します：

```json
// Button.toolkit.json
{
  "name": "Button",
  "description": "標準的なボタンのテンプレート",
  "parameters": {
    "text": {
      "type": "String",
      "default": "Button",
      "description": "ボタンに表示するテキスト"
    },
    "width": {
      "type": "double",
      "default": 80,
      "min": 40,
      "description": "ボタンの幅"
    },
    "height": {
      "type": "double", 
      "default": 32,
      "min": 20,
      "description": "ボタンの高さ"
    },
    "primaryColor": {
      "type": "ColorF",
      "default": [0.2, 0.6, 0.9, 1.0],
      "description": "ボタンの基本色"
    },
    "onClick": {
      "type": "callback",
      "description": "クリック時のコールバック"
    }
  },
  "structure": {
    "name": "$text",
    "constraint": {
      "type": "BoxConstraint",
      "minSize": ["$width", "$height"],
      "padding": [12, 12, 6, 6]
    },
    "components": [
      {
        "type": "RectRenderer",
        "color": {
          "variable": "$primaryColor",
          "fallback": {
            "default": "$primaryColor",
            "hovered": "@lighten($primaryColor, 0.1)",
            "pressed": "@darken($primaryColor, 0.1)",
            "smoothTime": 0.1
          }
        },
        "cornerRadius": [4, 4, 4, 4]
      },
      {
        "type": "Label",
        "text": "$text",
        "alignment": [0.5, 0.5],
        "fontSize": {
          "variable": "$fontSize",
          "fallback": 14
        }
      },
      {
        "type": "EventTrigger",
        "bindings": {
          "clicked": "$onClick"
        }
      }
    ]
  }
}
```

### 2.3 使用方法

```cpp
// C++コードでの使用
auto button = Toolkit::Create("Button", {
    {"text", U"Submit"},
    {"primaryColor", ColorF{0.2, 0.8, 0.2}},
    {"onClick", [this] { submitForm(); }}
});

// または、より型安全な方法
auto button = Toolkit::Button()
    .text(U"Submit")
    .primaryColor(ColorF{0.2, 0.8, 0.2})
    .onClick([this] { submitForm(); })
    .create();
```

## 3. ツールキットシステムの設計

### 3.1 ツールキットローダー

```cpp
class ToolkitLoader
{
    HashMap<String, ToolkitDefinition> m_toolkits;
    
public:
    void loadFromFile(const FilePath& path)
    {
        JSON json = JSON::Load(path);
        auto toolkit = ToolkitDefinition::FromJSON(json);
        m_toolkits[toolkit.name] = toolkit;
    }
    
    void loadFromDirectory(const FilePath& directory)
    {
        for (const auto& file : FileSystem::DirectoryContents(directory))
        {
            if (file.extension() == U".toolkit.json")
            {
                loadFromFile(file);
            }
        }
    }
    
    SharedPtr<Node> instantiate(const String& toolkitName, 
                               const HashMap<String, std::any>& parameters)
    {
        auto it = m_toolkits.find(toolkitName);
        if (it == m_toolkits.end())
        {
            throw Error{U"Toolkit '{}' not found", toolkitName};
        }
        
        return it->second.instantiate(parameters);
    }
};
```

### 3.2 パラメータ解決システム

```cpp
class ParameterResolver
{
    HashMap<String, std::any> m_parameters;
    HashMap<String, std::function<std::any(const Array<std::any>&)>> m_functions;
    
public:
    ParameterResolver()
    {
        // 組み込み関数の登録
        registerFunction(U"lighten", [](const Array<std::any>& args) {
            auto color = std::any_cast<ColorF>(args[0]);
            double amount = std::any_cast<double>(args[1]);
            return color.lerp(Palette::White, amount);
        });
        
        registerFunction(U"darken", [](const Array<std::any>& args) {
            auto color = std::any_cast<ColorF>(args[0]);
            double amount = std::any_cast<double>(args[1]);
            return color.lerp(Palette::Black, amount);
        });
    }
    
    std::any resolve(const String& expression)
    {
        if (expression.starts_with(U'$'))
        {
            // パラメータ参照
            return resolveParameter(expression.substr(1));
        }
        else if (expression.starts_with(U'@'))
        {
            // 関数呼び出し
            return resolveFunction(expression.substr(1));
        }
        else
        {
            // リテラル値
            return parseLiteral(expression);
        }
    }
};
```

### 3.3 ツールキット定義

```cpp
struct ToolkitDefinition
{
    String name;
    String description;
    HashMap<String, ParameterDefinition> parameters;
    JSON structure;
    
    SharedPtr<Node> instantiate(const HashMap<String, std::any>& userParams) const
    {
        // パラメータのマージ（ユーザー指定 + デフォルト値）
        auto resolver = std::make_shared<ParameterResolver>();
        
        for (const auto& [name, def] : parameters)
        {
            if (auto it = userParams.find(name); it != userParams.end())
            {
                resolver->setParameter(name, it->second);
            }
            else if (def.hasDefault)
            {
                resolver->setParameter(name, def.defaultValue);
            }
            else if (def.required)
            {
                throw Error{U"Required parameter '{}' not provided", name};
            }
        }
        
        // Node構造の構築
        return buildNodeFromJSON(structure, resolver);
    }
    
private:
    SharedPtr<Node> buildNodeFromJSON(const JSON& json, 
                                     SharedPtr<ParameterResolver> resolver) const
    {
        auto node = Node::Create();
        
        // 名前の設定
        if (json.hasElement(U"name"))
        {
            String name = json[U"name"].getString();
            node->setName(resolver->resolveString(name));
        }
        
        // 制約の設定
        if (json.hasElement(U"constraint"))
        {
            node->setConstraint(buildConstraintFromJSON(
                json[U"constraint"], resolver));
        }
        
        // コンポーネントの追加
        if (json.hasElement(U"components"))
        {
            for (const auto& compJSON : json[U"components"].arrayView())
            {
                addComponentFromJSON(node, compJSON, resolver);
            }
        }
        
        // 子ノードの追加
        if (json.hasElement(U"children"))
        {
            for (const auto& childJSON : json[U"children"].arrayView())
            {
                auto child = buildNodeFromJSON(childJSON, resolver);
                node->addChild(child);
            }
        }
        
        return node;
    }
};
```

## 4. 高度なツールキット機能

### 4.1 条件付き構造

```json
{
  "name": "IconButton",
  "parameters": {
    "icon": {
      "type": "String",
      "optional": true
    },
    "text": {
      "type": "String",
      "optional": true
    },
    "iconPosition": {
      "type": "String",
      "default": "left",
      "enum": ["left", "right", "top", "bottom"]
    }
  },
  "structure": {
    "constraint": {
      "type": "BoxConstraint",
      "padding": [8, 8, 8, 8]
    },
    "layout": {
      "@if": "$iconPosition == 'left' || $iconPosition == 'right'",
      "@then": {
        "type": "HorizontalLayout",
        "spacing": 8
      },
      "@else": {
        "type": "VerticalLayout",
        "spacing": 4
      }
    },
    "children": [
      {
        "@if": "$icon",
        "components": [{
          "type": "Label",
          "text": "$icon",
          "font": "FontAwesome"
        }]
      },
      {
        "@if": "$text",
        "components": [{
          "type": "Label",
          "text": "$text"
        }]
      }
    ]
  }
}
```

### 4.2 ツールキットの合成

```json
{
  "name": "DialogBox",
  "parameters": {
    "title": { "type": "String", "required": true },
    "content": { "type": "Node", "required": true },
    "buttons": { "type": "Array<ButtonDef>", "default": [] }
  },
  "structure": {
    "components": [{
      "type": "RectRenderer",
      "color": "$backgroundColor",
      "cornerRadius": [8, 8, 8, 8]
    }],
    "layout": {
      "type": "VerticalLayout",
      "spacing": 16
    },
    "children": [
      {
        "@use": "DialogHeader",
        "@params": {
          "title": "$title",
          "onClose": "$onClose"
        }
      },
      {
        "@slot": "content"
      },
      {
        "layout": {
          "type": "HorizontalLayout",
          "spacing": 8,
          "alignment": [1.0, 0.5]
        },
        "children": {
          "@foreach": "$buttons",
          "@as": "buttonDef",
          "@template": {
            "@use": "Button",
            "@params": "$buttonDef"
          }
        }
      }
    ]
  }
}
```

### 4.3 スタイルバリアント

```json
{
  "name": "Button",
  "variants": {
    "primary": {
      "primaryColor": [0.2, 0.6, 0.9, 1.0],
      "textColor": [1.0, 1.0, 1.0, 1.0]
    },
    "secondary": {
      "primaryColor": [0.7, 0.7, 0.7, 1.0],
      "textColor": [0.2, 0.2, 0.2, 1.0]
    },
    "danger": {
      "primaryColor": [0.9, 0.2, 0.2, 1.0],
      "textColor": [1.0, 1.0, 1.0, 1.0]
    }
  },
  "parameters": {
    "variant": {
      "type": "String",
      "default": "primary",
      "enum": ["primary", "secondary", "danger"]
    }
  }
}
```

## 5. ツールキットの配布と管理

### 5.1 パッケージシステム

```cpp
// toolkit-manifest.json
{
  "name": "NocoUI-StandardKit",
  "version": "1.0.0",
  "description": "NocoUI標準UIツールキット",
  "author": "NocoUI Team",
  "toolkits": [
    "Button.toolkit.json",
    "Checkbox.toolkit.json",
    "Dropdown.toolkit.json",
    "Dialog.toolkit.json"
  ],
  "dependencies": {
    "NocoUI": ">=0.1.0"
  }
}
```

### 5.2 ツールキットマネージャー

```cpp
class ToolkitManager
{
    FilePath m_toolkitDirectory;
    ToolkitLoader m_loader;
    HashMap<String, ToolkitPackage> m_packages;
    
public:
    void installPackage(const URL& packageUrl)
    {
        // パッケージをダウンロード
        auto packageData = downloadPackage(packageUrl);
        
        // マニフェストを読み込み
        auto manifest = parseManifest(packageData);
        
        // ツールキットファイルを展開
        extractToolkits(packageData, m_toolkitDirectory / manifest.name);
        
        // ローダーに登録
        for (const auto& toolkit : manifest.toolkits)
        {
            m_loader.loadFromFile(m_toolkitDirectory / manifest.name / toolkit);
        }
        
        m_packages[manifest.name] = manifest;
    }
    
    Array<String> listAvailableToolkits() const
    {
        return m_loader.getToolkitNames();
    }
    
    void generateTypeScriptDefinitions()
    {
        // 型定義ファイルを生成（エディタ用）
        for (const auto& [name, toolkit] : m_loader.getToolkits())
        {
            generateToolkitTypeDef(name, toolkit);
        }
    }
};
```

## 6. エディタ統合

### 6.1 ツールキットパレット

NocoEditorに「ツールキットパレット」を追加し、ドラッグ＆ドロップでツールキットからNodeを作成できるようにします。

```cpp
class ToolkitPalette : public ComponentBase
{
    SharedPtr<ToolkitManager> m_toolkitManager;
    
    void draw() const override
    {
        for (const auto& toolkitName : m_toolkitManager->listAvailableToolkits())
        {
            drawToolkitItem(toolkitName);
        }
    }
    
    void drawToolkitItem(const String& name) const
    {
        // アイコンとラベルを表示
        auto itemNode = createPaletteItem(name);
        
        // ドラッグ可能に
        auto dragSource = itemNode->emplaceComponent<DragDropSource>();
        dragSource->setData(U"toolkit", name);
    }
};
```

### 6.2 プロパティパネル統合

ツールキットから作成されたNodeの場合、元のパラメータを編集可能にします。

```cpp
class ToolkitPropertyEditor : public ComponentBase
{
    SharedPtr<Node> m_targetNode;
    ToolkitInstance m_instance;  // ツールキット情報を保持
    
    void buildUI()
    {
        for (const auto& [paramName, paramDef] : m_instance.parameters)
        {
            auto editor = createParameterEditor(paramDef);
            editor->setValue(m_instance.getCurrentValue(paramName));
            
            editor->onChange([this, paramName](const std::any& newValue) {
                // パラメータを更新して再構築
                m_instance.setParameter(paramName, newValue);
                rebuildNode();
            });
        }
    }
};
```

## 7. 利点と課題

### 7.1 利点

1. **拡張性**: 新しいコンポーネントクラスを追加せずに、UIパターンを追加可能
2. **共有可能性**: JSONファイルなので、コミュニティでの共有が容易
3. **カスタマイズ性**: ユーザーが独自のツールキットを作成可能
4. **保守性**: UIパターンの更新が容易
5. **学習曲線**: 既存のコンポーネントの知識で理解可能

### 7.2 課題と解決策

#### 型安全性
**課題**: JSONベースのため、コンパイル時の型チェックが効かない
**解決策**: 
- ツールキット定義にJSON Schemaを使用
- コード生成ツールで型安全なラッパーを生成
- エディタでのリアルタイム検証

#### パフォーマンス
**課題**: 実行時のパラメータ解決によるオーバーヘッド
**解決策**:
- パラメータ解決結果のキャッシング
- 頻繁に使用されるツールキットの事前コンパイル
- 静的なツールキットはC++コードに変換

#### デバッグ
**課題**: ツールキットから生成されたNodeのデバッグが困難
**解決策**:
- デバッグモードでツールキット情報を保持
- エディタでツールキットソースへのリンク表示
- ツールキットプロファイラーの実装

## 8. 実装ロードマップ

### フェーズ1: 基本実装（2-3週間）
1. ParameterResolverの実装
2. ToolkitLoaderの基本機能
3. 基本的なツールキット（Button、Checkbox）の作成

### フェーズ2: エディタ統合（2-3週間）
4. ツールキットパレットの実装
5. プロパティエディタの拡張
6. ドラッグ＆ドロップ対応

### フェーズ3: 高度な機能（3-4週間）
7. 条件付き構造のサポート
8. ツールキット合成機能
9. パッケージマネージャー

### フェーズ4: 最適化と公開（2週間）
10. パフォーマンス最適化
11. ドキュメント作成
12. サンプルツールキットの充実

## 9. サンプルツールキット集

### 9.1 フォーム要素

```json
// TextInput.toolkit.json
{
  "name": "TextInput",
  "parameters": {
    "placeholder": { "type": "String", "default": "" },
    "value": { "type": "String", "default": "" },
    "maxLength": { "type": "int", "optional": true },
    "validation": { "type": "String", "optional": true }
  },
  "structure": {
    "constraint": {
      "type": "BoxConstraint",
      "minSize": [200, 32],
      "padding": [8, 8, 4, 4]
    },
    "components": [
      {
        "type": "RectRenderer",
        "color": "$backgroundColor",
        "borderColor": {
          "default": "$borderColor",
          "focused": "$focusColor"
        },
        "borderWidth": 1
      },
      {
        "type": "TextBox",
        "text": "$value",
        "placeholder": "$placeholder",
        "maxLength": "$maxLength"
      }
    ]
  }
}
```

### 9.2 レイアウト要素

```json
// Card.toolkit.json
{
  "name": "Card",
  "parameters": {
    "title": { "type": "String", "optional": true },
    "elevation": { "type": "int", "default": 2 },
    "padding": { "type": "LRTB", "default": [16, 16, 16, 16] }
  },
  "structure": {
    "components": [
      {
        "type": "RectRenderer",
        "color": "$cardBackground",
        "cornerRadius": [8, 8, 8, 8],
        "shadow": {
          "@compute": "getShadowForElevation($elevation)"
        }
      }
    ],
    "layout": {
      "type": "VerticalLayout",
      "spacing": 12
    },
    "children": [
      {
        "@if": "$title",
        "components": [{
          "type": "Label",
          "text": "$title",
          "fontSize": 18,
          "fontWeight": "Bold"
        }]
      },
      {
        "@slot": "content"
      }
    ]
  }
}
```

## 10. まとめ

ツールキットアプローチは、NocoUIの既存の強力な基盤を活かしながら、より高レベルのUI構築を可能にする実用的な解決策です。このアプローチにより：

1. **即座の価値提供**: 新しいコンポーネントクラスの実装を待たずに、標準的なUIパターンを提供
2. **コミュニティ駆動**: ユーザーが独自のツールキットを作成・共有
3. **段階的な移行**: 既存のコードと共存可能
4. **エディタとの親和性**: ビジュアルな編集が可能

このシステムは、NocoUIを「低レベルのUIフレームワーク」から「包括的なUI構築プラットフォーム」へと進化させる重要なステップとなります。