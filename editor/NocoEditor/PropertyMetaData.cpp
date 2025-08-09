#include "PropertyMetaData.hpp"

namespace noco::editor
{
	HashTable<PropertyKey, PropertyMetadata> InitPropertyMetadata()
	{
		HashTable<PropertyKey, PropertyMetadata> metadata;
		
		// 9スライス関連プロパティの表示条件
		const auto nineSliceVisibilityCondition = [](const ComponentBase& component) -> bool
		{
			if (const auto* sprite = dynamic_cast<const Sprite*>(&component))
			{
				return HasAnyTrueState(sprite->nineSliceEnabled());
			}
			return true;
		};
		
		// Nodeのプロパティ
		metadata[PropertyKey{ U"Node", U"activeSelf" }] = PropertyMetadata{
			.tooltip = U"Nodeの有効/無効",
			.tooltipDetail = U"このNodeとその子要素の表示を制御します\n無効の場合、updateの代わりにupdateInactiveが実行され、drawは実行されません",
		};
		metadata[PropertyKey{ U"Node", U"isHitTarget" }] = PropertyMetadata{
			.tooltip = U"ヒットテストの対象にするどうか",
			.tooltipDetail = U"無効にすると、この要素はヒットテスト(要素にマウスカーソルがホバーしているかどうかの判定)の対象外となり、親要素のInteractionStateを受け継ぎます\n※無効の場合、ヒットテストでは要素の存在自体が無視されるため、背面にある要素にホバーが可能となります\n※無効の場合、TextBox等のマウス操作を利用するコンポーネントも入力を受け付けなくなります",
		};
		metadata[PropertyKey{ U"Node", U"hitTestPadding" }] = PropertyMetadata{
			.tooltip = U"ヒットテスト領域の拡縮 (左、右、上、下)",
			.tooltipDetail = U"ヒットテスト(要素にマウスカーソルがホバーしているかどうかの判定)に使用する領域を、指定されたピクセル数だけ拡大・縮小します\n正の値で領域を拡大、負の値で領域を縮小します\n実際の見た目よりもずれた位置にマウスカーソルがあっても反応させたい場合に使用できます",
		};
		metadata[PropertyKey{ U"Node", U"inheritsChildrenHoveredState" }] = PropertyMetadata{
			.tooltip = U"子要素のホバー状態(Hovered)を継承するかどうか",
			.tooltipDetail = U"有効にすると、子要素のInteractionStateがHoveredの場合に、このNodeのInteractionStateがHoveredになります\n※このNodeのInteractionStateがPressed・Disabledの場合は影響を受けません",
		};
		metadata[PropertyKey{ U"Node", U"inheritsChildrenPressedState" }] = PropertyMetadata{
			.tooltip = U"子要素の押下状態(Pressed)を継承するかどうか",
			.tooltipDetail = U"有効にすると、子要素のInteractionStateがPressedの場合に、このNodeのInteractionStateがPressedになります\n※このNodeのInteractionStateがDisabledの場合は影響を受けません",
		};
		metadata[PropertyKey{ U"Node", U"interactable" }] = PropertyMetadata{
			.tooltip = U"インタラクション可能かどうか",
			.tooltipDetail = U"無効にすると、InteractionStateがDisabledになり、マウスホバーやクリックイベントが無効になります\n※interactableを無効にしても、updateやdrawは実行されます",
		};
		metadata[PropertyKey{ U"Node", U"horizontalScrollable" }] = PropertyMetadata{
			.tooltip = U"水平方向のスクロール可能",
		};
		metadata[PropertyKey{ U"Node", U"verticalScrollable" }] = PropertyMetadata{
			.tooltip = U"垂直方向のスクロール可能",
		};
		metadata[PropertyKey{ U"Node", U"wheelScrollEnabled" }] = PropertyMetadata{
			.tooltip = U"ホイールスクロールの有効/無効",
			.tooltipDetail = U"有効にすると、マウスホイールでスクロールできます",
		};
		metadata[PropertyKey{ U"Node", U"dragScrollEnabled" }] = PropertyMetadata{
			.tooltip = U"ドラッグスクロールの有効/無効",
			.tooltipDetail = U"有効にすると、ドラッグ操作でスクロールできます",
		};
		metadata[PropertyKey{ U"Node", U"decelerationRate" }] = PropertyMetadata{
			.tooltip = U"慣性スクロールの減衰率",
			.tooltipDetail = U"1秒あたりの速度減衰率(0.0~1.0)。値が小さいほど早く停止します",
			.dragValueChangeStep = 0.01,
		};
		metadata[PropertyKey{ U"Node", U"rubberBandScrollEnabled" }] = PropertyMetadata{
			.tooltip = U"ラバーバンドスクロールの有効/無効",
			.tooltipDetail = U"有効にすると、スクロール範囲外でも一時的にドラッグでき、離すと自動的に範囲内に戻ります",
		};
		metadata[PropertyKey{ U"Node", U"clippingEnabled" }] = PropertyMetadata{
			.tooltip = U"クリッピングの有効/無効",
			.tooltipDetail = U"有効にすると、コンポーネントや子要素の描画内容が要素の矩形範囲で切り取られます",
		};
		metadata[PropertyKey{ U"Node", U"styleState" }] = PropertyMetadata{
			.tooltip = U"styleState(スタイルステート)",
			.tooltipDetail = U"styleStateとは、要素の状態を識別するために設定する文字列です(例: \"selected\")\n各プロパティの値はstyleState毎に異なる値を設定でき、状態に応じて見た目を変えることができます\nstyleStateはノード毎に1つのみ設定できます\n\n親要素のstyleStateがあればそれを受け継ぎます\n適用の優先度は自身の要素のstyleStateが最も高く、遠い親になるにつれて優先度は下がります",
		};
		
		// Region関連 - AnchorRegion
		metadata[PropertyKey{ U"AnchorRegion", U"type" }] = PropertyMetadata{
			.tooltip = U"Regionの種類",
			.tooltipDetail = U"親要素に対する位置とサイズの決め方の種類を指定します\nAnchorRegion: 親要素の四辺を基に比率と差分値で四辺の位置を決定します\n　※AnchorRegionの要素は親要素のchildrenLayoutの影響を受けません\nInlineRegion: 親要素のchildrenLayoutで指定されたレイアウト方法に応じて、順番に配置されます",
		};
		metadata[PropertyKey{ U"AnchorRegion", U"anchor" }] = PropertyMetadata{
			.tooltip = U"アンカー位置",
		};
		metadata[PropertyKey{ U"AnchorRegion", U"anchorMin" }] = PropertyMetadata{
			.tooltip = U"最小アンカー位置 (0,0)が左上、(1,1)が右下",
		};
		metadata[PropertyKey{ U"AnchorRegion", U"anchorMax" }] = PropertyMetadata{
			.tooltip = U"最大アンカー位置 (0,0)が左上、(1,1)が右下",
		};
		metadata[PropertyKey{ U"AnchorRegion", U"posDelta" }] = PropertyMetadata{
			.tooltip = U"位置 (アンカーからの相対位置)",
		};
		metadata[PropertyKey{ U"AnchorRegion", U"sizeDelta" }] = PropertyMetadata{
			.tooltip = U"サイズ (差分値)",
			.tooltipDetail = U"要素の大きさをピクセル数で指定します。アンカーを基に計算された領域サイズにこのサイズが加算されます",
		};
		metadata[PropertyKey{ U"AnchorRegion", U"sizeDeltaPivot" }] = PropertyMetadata{
			.tooltip = U"サイズ計算の起点 (X、Y)",
		};
		
		// Region関連 - InlineRegion
		metadata[PropertyKey{ U"InlineRegion", U"type" }] = PropertyMetadata{
			.tooltip = U"Regionの種類",
			.tooltipDetail = U"親要素に対する位置とサイズの決め方の種類を指定します\nAnchorRegion: 親要素の四辺を基に比率と差分値で四辺の位置を決定します\n　※AnchorRegionの要素は親要素のchildrenLayoutの影響を受けません\nInlineRegion: 親要素のchildrenLayoutで指定されたレイアウト方法に応じて、順番に配置されます",
		};
		metadata[PropertyKey{ U"InlineRegion", U"margin" }] = PropertyMetadata{
			.tooltip = U"マージン (左、右、上、下)",
			.tooltipDetail = U"要素の外側の余白を指定します\n※全ての子要素間で共通の間隔を設定したい場合は、こちらではなく親要素のchildrenLayoutに対してspacingの値を指定してください",
		};
		metadata[PropertyKey{ U"InlineRegion", U"sizeRatio" }] = PropertyMetadata{
			.tooltip = U"親要素に対するサイズ比率 (0.0～1.0)",
			.tooltipDetail = U"親要素のサイズに対する比率を指定します。0.0は親要素のサイズを無視し、1.0は親要素のサイズと同じになります\n※要素間で自動的にサイズを分配する必要がある場合、sizeRatioではなくflexibleWeightを使用してください",
		};
		metadata[PropertyKey{ U"InlineRegion", U"sizeDelta" }] = PropertyMetadata{
			.tooltip = U"サイズ (差分値)",
			.tooltipDetail = U"要素の大きさをピクセル数で指定します。sizeRatioおよびflexibleWeightと併用した場合、このサイズが差分値として加算されます",
		};
		metadata[PropertyKey{ U"InlineRegion", U"flexibleWeight" }] = PropertyMetadata{
			.tooltip = U"フレキシブル要素の伸縮の重み",
			.tooltipDetail = U"0以外の値を設定すると、余った領域を重みの比率に応じて他のフレキシブル要素と分け合います\n(FlowLayoutとHorizontalLayoutでは横方向、VerticalLayoutでは縦方向の領域を分け合います)\n※例1: 全てのフレキシブル要素に1を指定すると、余った領域を均等に分配します\n※例2: ある要素に2、それ以外の全ての要素に1を指定すると、2を指定した要素は他の要素の2倍の領域が割り当てられます",
		};
		
		// AnchorPreset用プロパティ
		metadata[PropertyKey{ U"AnchorRegion", U"top" }] = PropertyMetadata{
			.tooltip = U"親要素の上端からの距離",
		};
		metadata[PropertyKey{ U"AnchorRegion", U"left" }] = PropertyMetadata{
			.tooltip = U"親要素の左端からの距離",
		};
		metadata[PropertyKey{ U"AnchorRegion", U"right" }] = PropertyMetadata{
			.tooltip = U"親要素の右端からの距離",
		};
		metadata[PropertyKey{ U"AnchorRegion", U"bottom" }] = PropertyMetadata{
			.tooltip = U"親要素の下端からの距離",
		};
		metadata[PropertyKey{ U"AnchorRegion", U"size" }] = PropertyMetadata{
			.tooltip = U"サイズ (幅、高さ)",
		};
		metadata[PropertyKey{ U"AnchorRegion", U"width" }] = PropertyMetadata{
			.tooltip = U"幅",
			.dragValueChangeStep = 1.0,
		};
		metadata[PropertyKey{ U"AnchorRegion", U"height" }] = PropertyMetadata{
			.tooltip = U"高さ",
			.dragValueChangeStep = 1.0,
		};
		metadata[PropertyKey{ U"AnchorRegion", U"xDelta" }] = PropertyMetadata{
			.tooltip = U"X軸の位置",
		};
		metadata[PropertyKey{ U"AnchorRegion", U"yDelta" }] = PropertyMetadata{
			.tooltip = U"Y軸の位置",
		};
		metadata[PropertyKey{ U"AnchorRegion", U"maxWidth" }] = PropertyMetadata{
			.tooltip = U"最大幅",
			.tooltipDetail = U"要素の幅の最大値を指定します。チェックボックスをOFFにすると、最大値の制限がなくなります",
			.dragValueChangeStep = 1.0,
		};
		metadata[PropertyKey{ U"AnchorRegion", U"maxHeight" }] = PropertyMetadata{
			.tooltip = U"最大高さ",
			.tooltipDetail = U"要素の高さの最大値を指定します。チェックボックスをOFFにすると、最大値の制限がなくなります",
			.dragValueChangeStep = 1.0,
		};
		
		// Layout関連
		metadata[PropertyKey{ U"FlowLayout", U"type" }] = PropertyMetadata{
			.tooltip = U"レイアウトの種類",
			.tooltipDetail = U"FlowLayout: 子要素を左から右へ並べ、右端で折り返します\nHorizontalLayout: 子要素を水平方向に並べます\nVerticalLayout: 子要素を垂直方向に並べます\n※childrenLayoutはInlineRegionが指定された子要素のみに影響します。AnchorRegionを持つ子要素に対しては影響しません",
		};
		metadata[PropertyKey{ U"FlowLayout", U"padding" }] = PropertyMetadata{
			.tooltip = U"内側の余白 (左、右、上、下)",
		};
		metadata[PropertyKey{ U"FlowLayout", U"spacing" }] = PropertyMetadata{
			.tooltip = U"子要素同士の間隔 (X、Y)",
			.tooltipDetail = U"子要素同士の間隔を指定します\n全ての子要素に共通の間隔を指定したい場合に使用します\n※子要素のInlineRegionのmarginにも値が設定されている場合、spacingとmarginの合計値が子要素間の間隔として適用されます",
		};
		metadata[PropertyKey{ U"FlowLayout", U"horizontalAlign" }] = PropertyMetadata{
			.tooltip = U"水平方向の配置",
		};
		metadata[PropertyKey{ U"FlowLayout", U"verticalAlign" }] = PropertyMetadata{
			.tooltip = U"垂直方向の配置",
		};
		
		metadata[PropertyKey{ U"HorizontalLayout", U"type" }] = PropertyMetadata{
			.tooltip = U"レイアウトの種類",
			.tooltipDetail = U"FlowLayout: 子要素を左から右へ並べ、右端で折り返します\nHorizontalLayout: 子要素を水平方向に並べます\nVerticalLayout: 子要素を垂直方向に並べます\n※childrenLayoutはInlineRegionが指定された子要素のみに影響します。AnchorRegionを持つ子要素に対しては影響しません",
		};
		metadata[PropertyKey{ U"HorizontalLayout", U"padding" }] = PropertyMetadata{
			.tooltip = U"内側の余白 (左、右、上、下)",
		};
		metadata[PropertyKey{ U"HorizontalLayout", U"spacing" }] = PropertyMetadata{
			.tooltip = U"子要素同士の間隔 (X、Y)",
			.tooltipDetail = U"子要素同士の間隔を指定します\n全ての子要素に共通の間隔を指定したい場合に使用します\n※子要素のInlineRegionのmarginにも値が設定されている場合、spacingとmarginの合計値が子要素間の間隔として適用されます",
		};
		metadata[PropertyKey{ U"HorizontalLayout", U"horizontalAlign" }] = PropertyMetadata{
			.tooltip = U"水平方向の配置",
		};
		metadata[PropertyKey{ U"HorizontalLayout", U"verticalAlign" }] = PropertyMetadata{
			.tooltip = U"垂直方向の配置",
		};
		
		metadata[PropertyKey{ U"VerticalLayout", U"type" }] = PropertyMetadata{
			.tooltip = U"レイアウトの種類",
			.tooltipDetail = U"FlowLayout: 子要素を左から右へ並べ、右端で折り返します\nHorizontalLayout: 子要素を水平方向に並べます\nVerticalLayout: 子要素を垂直方向に並べます\n※childrenLayoutはInlineRegionが指定された子要素のみに影響します。AnchorRegionを持つ子要素に対しては影響しません",
		};
		metadata[PropertyKey{ U"VerticalLayout", U"padding" }] = PropertyMetadata{
			.tooltip = U"内側の余白 (左、右、上、下)",
		};
		metadata[PropertyKey{ U"VerticalLayout", U"spacing" }] = PropertyMetadata{
			.tooltip = U"子要素同士の間隔 (X、Y)",
			.tooltipDetail = U"子要素同士の間隔を指定します\n全ての子要素に共通の間隔を指定したい場合に使用します\n※子要素のInlineRegionのmarginにも値が設定されている場合、spacingとmarginの合計値が子要素間の間隔として適用されます",
		};
		metadata[PropertyKey{ U"VerticalLayout", U"horizontalAlign" }] = PropertyMetadata{
			.tooltip = U"水平方向の配置",
		};
		metadata[PropertyKey{ U"VerticalLayout", U"verticalAlign" }] = PropertyMetadata{
			.tooltip = U"垂直方向の配置",
		};
		
		// TransformEffect関連
		metadata[PropertyKey{ U"TransformEffect", U"position" }] = PropertyMetadata{
			.tooltip = U"位置",
			.tooltipDetail = U"要素の位置を移動させます\nこの値による位置変更はレイアウト計算に影響を与えません\n※TransformEffectはレイアウトの再計算を必要としないため、要素の位置を高速に変更できます。そのため、アニメーション等の用途で利用できます\n※appliesToHitTestがtrueの場合のみ、マウスカーソルのホバー判定に移動後の位置が利用されます",
		};
		metadata[PropertyKey{ U"TransformEffect", U"scale" }] = PropertyMetadata{
			.tooltip = U"スケール",
			.tooltipDetail = U"要素のサイズを拡大・縮小するスケールを指定します\nこの値による拡大縮小はレイアウト計算に影響を与えません\n※TransformEffectはレイアウトの再計算を必要としないため、要素の大きさを高速に変更できます。そのため、アニメーション等の用途で利用できます\n※描画内容はスケールに応じて伸縮されます\n※appliesToHitTestがtrueの場合のみ、マウスカーソルのホバー判定に拡大縮小後のサイズが利用されます",
		};
		metadata[PropertyKey{ U"TransformEffect", U"pivot" }] = PropertyMetadata{
			.tooltip = U"基準点 (X、Y)",
			.tooltipDetail = U"scaleによる拡大縮小とrotationによる回転の基準点となる位置を0～1の比率で指定します\n(0,0)は左上、(1,1)は右下を表します",
		};
		metadata[PropertyKey{ U"TransformEffect", U"rotation" }] = PropertyMetadata{
			.tooltip = U"回転角度",
			.tooltipDetail = U"要素の回転角度を度数法で指定します\n正の値で時計回り、負の値で反時計回りに回転します\n回転の中心はpivotで指定した基準点になります\n※この値による回転はレイアウト計算に影響を与えません\n※appliesToHitTestがtrueの場合のみ、マウスカーソルのホバー判定に回転が適用されます",
			.dragValueChangeStep = 1.0,
		};
		metadata[PropertyKey{ U"TransformEffect", U"appliesToHitTest" }] = PropertyMetadata{
			.tooltip = U"ヒットテスト領域へ適用するか",
			.tooltipDetail = U"TransformEffectの位置・スケール・回転をマウスのホバー判定に適用するかどうかを指定します\ntrueの場合：position, scale, rotationの変換がホバー判定に反映されます\nfalseの場合：変換は描画のみに適用され、ホバー判定は元の位置で行われます",
		};
		metadata[PropertyKey{ U"TransformEffect", U"color" }] = PropertyMetadata{
			.tooltip = U"乗算カラー",
			.tooltipDetail = U"子孫を含む要素の描画に対する乗算カラーを指定します\n親要素が乗算カラーを持つ場合、再帰的に乗算したカラーが適用されます",
		};
		
		// Componentのプロパティ
		// RectRenderer
		metadata[PropertyKey{ U"RectRenderer", U"fillGradationType" }] = PropertyMetadata{
			.tooltip = U"塗りつぶしグラデーションタイプ",
			.tooltipDetail = U"塗りつぶしのグラデーションタイプを選択します\nNone: 単色塗りつぶし\nTopBottom: 上下グラデーション\nLeftRight: 左右グラデーション",
			.refreshInspectorOnChange = true,
		};
		metadata[PropertyKey{ U"RectRenderer", U"fillColor" }] = PropertyMetadata{
			.tooltip = U"塗りつぶし色",
			.visibilityCondition = [](const ComponentBase& component)
			{
				if (const auto* rectRenderer = dynamic_cast<const RectRenderer*>(&component))
				{
					return rectRenderer->fillGradationType().hasAnyStateEqualTo(RectFillGradationType::None);
				}
				return false;
			},
		};
		metadata[PropertyKey{ U"RectRenderer", U"fillGradationColor1" }] = PropertyMetadata{
			.tooltip = U"グラデーション色 1",
			.tooltipDetail = U"TopBottom: 上側の色\nLeftRight: 左側の色",
			.visibilityCondition = [](const ComponentBase& component)
			{
				if (const auto* rectRenderer = dynamic_cast<const RectRenderer*>(&component))
				{
					return !rectRenderer->fillGradationType().hasAnyStateEqualTo(RectFillGradationType::None);
				}
				return false;
			},
		};
		metadata[PropertyKey{ U"RectRenderer", U"fillGradationColor2" }] = PropertyMetadata{
			.tooltip = U"グラデーション色 2",
			.tooltipDetail = U"TopBottom: 下側の色\nLeftRight: 右側の色",
			.visibilityCondition = [](const ComponentBase& component)
			{
				if (const auto* rectRenderer = dynamic_cast<const RectRenderer*>(&component))
				{
					return !rectRenderer->fillGradationType().hasAnyStateEqualTo(RectFillGradationType::None);
				}
				return false;
			},
		};
		metadata[PropertyKey{ U"RectRenderer", U"blendMode" }] = PropertyMetadata{
			.tooltip = U"ブレンドモード",
			.tooltipDetail = U"描画時のブレンドモードを指定します\nNormal: 通常の描画\nAdditive: 加算合成\nSubtractive: 減算合成\nMultiply: 乗算合成",
		};
		metadata[PropertyKey{ U"RectRenderer", U"outlineColor" }] = PropertyMetadata{
			.tooltip = U"アウトライン色",
		};
		metadata[PropertyKey{ U"RectRenderer", U"outlineThickness" }] = PropertyMetadata{
			.tooltip = U"アウトラインの太さ",
			.dragValueChangeStep = 1.0,
		};
		metadata[PropertyKey{ U"RectRenderer", U"cornerRadius" }] = PropertyMetadata{
			.tooltip = U"角の丸み半径",
			.dragValueChangeStep = 1.0,
		};
		metadata[PropertyKey{ U"RectRenderer", U"shadowColor" }] = PropertyMetadata{
			.tooltip = U"影の色",
		};
		metadata[PropertyKey{ U"RectRenderer", U"shadowOffset" }] = PropertyMetadata{
			.tooltip = U"影のオフセット (位置のずらし量)",
		};
		metadata[PropertyKey{ U"RectRenderer", U"shadowBlur" }] = PropertyMetadata{
			.tooltip = U"影のぼかし度合い",
			.dragValueChangeStep = 1.0,
		};
		metadata[PropertyKey{ U"RectRenderer", U"shadowSpread" }] = PropertyMetadata{
			.tooltip = U"影の拡散サイズ",
			.dragValueChangeStep = 1.0,
		};
		
		// Label
		metadata[PropertyKey{ U"Label", U"text" }] = PropertyMetadata{
			.tooltip = U"表示するテキスト",
			.numTextAreaLines = 3,
		};
		metadata[PropertyKey{ U"Label", U"fontAssetName" }] = PropertyMetadata{
			.tooltip = U"フォントアセット名",
			.tooltipDetail = U"指定されている場合、プログラム上ではこのキー名をもとに取得したFontAssetのフォントを使用します\n※プレビューには反映されません",
		};
		metadata[PropertyKey{ U"Label", U"fontSize" }] = PropertyMetadata{
			.tooltip = U"フォントサイズ",
			.dragValueChangeStep = 1.0,
		};
		metadata[PropertyKey{ U"Label", U"sizingMode" }] = PropertyMetadata{
			.tooltip = U"サイズに関するモード",
			.tooltipDetail = U"Fixed: 固定フォントサイズで描画します\nShrinkToFit: ノードサイズに収まるようフォントサイズを自動縮小します\n※ShrinkToFitはテキストやその他の値に変化が発生した時のフォントサイズの再計算にかかる負荷が高いため、自動縮小が不要な場合はなるべくFixedを指定することを推奨します",
			.refreshInspectorOnChange = true,
		};
		metadata[PropertyKey{ U"Label", U"minFontSize" }] = PropertyMetadata{
			.tooltip = U"最小フォントサイズ",
			.tooltipDetail = U"ShrinkToFit時の最小フォントサイズ",
			.visibilityCondition = [](const ComponentBase& component)
			{
				if (auto label = dynamic_cast<const Label*>(&component))
				{
					return label->sizingMode().defaultValue == LabelSizingMode::ShrinkToFit;
				}
				return false;
			},
			.dragValueChangeStep = 1.0,
		};
		metadata[PropertyKey{ U"Label", U"color" }] = PropertyMetadata{
			.tooltip = U"テキスト色",
		};
		metadata[PropertyKey{ U"Label", U"horizontalAlign" }] = PropertyMetadata{
			.tooltip = U"水平方向の配置",
		};
		metadata[PropertyKey{ U"Label", U"verticalAlign" }] = PropertyMetadata{
			.tooltip = U"垂直方向の配置",
		};
		metadata[PropertyKey{ U"Label", U"padding" }] = PropertyMetadata{
			.tooltip = U"内側の余白 (左、右、上、下)",
		};
		metadata[PropertyKey{ U"Label", U"horizontalOverflow" }] = PropertyMetadata{
			.tooltip = U"水平方向にはみ出す場合の処理",
			.tooltipDetail = U"Wrap: 自動的に折り返します\nOverflow: 右へはみ出して描画します",
		};
		metadata[PropertyKey{ U"Label", U"verticalOverflow" }] = PropertyMetadata{
			.tooltip = U"垂直方向にはみ出す場合の処理",
			.tooltipDetail = U"Clip: 領域をはみ出した文字は描画しません\nOverflow: 下へはみ出して描画します",
		};
		metadata[PropertyKey{ U"Label", U"characterSpacing" }] = PropertyMetadata{
			.tooltip = U"文字同士の間隔 (X, Y)",
		};
		metadata[PropertyKey{ U"Label", U"underlineStyle" }] = PropertyMetadata{
			.tooltip = U"下線のスタイル",
		};
		metadata[PropertyKey{ U"Label", U"underlineColor" }] = PropertyMetadata{
			.tooltip = U"下線の色",
		};
		metadata[PropertyKey{ U"Label", U"underlineThickness" }] = PropertyMetadata{
			.tooltip = U"下線の太さ",
			.dragValueChangeStep = 1.0,
		};
		
		// Sprite
		metadata[PropertyKey{ U"Sprite", U"textureFilePath" }] = PropertyMetadata{
			.tooltip = U"テクスチャファイルのパス",
			.tooltipDetail = U"textureAssetName使用時は、Editor上でのプレビュー用としてのみ使用されます",
		};
		metadata[PropertyKey{ U"Sprite", U"textureAssetName" }] = PropertyMetadata{
			.tooltip = U"TextureAssetのキー名 (任意)",
			.tooltipDetail = U"指定されている場合、プログラム上ではこのキー名をもとに取得したTextureAssetのテクスチャを使用します\n※プレビューには反映されません\n※これを使用しなくてもライブラリ側で内部的にファイルパスをもとにしたキー名でTextureAssetを使用するため、\n　パフォーマンス上の利点は特にありません。TextureAssetのキー名を手動で管理したい場合のみ使用してください",
		};
		metadata[PropertyKey{ U"Sprite", U"color" }] = PropertyMetadata{
			.tooltip = U"スプライトの色",
			.tooltipDetail = U"テクスチャの色に乗算されます\nアルファ値は透明度を制御します",
		};
		metadata[PropertyKey{ U"Sprite", U"addColor" }] = PropertyMetadata{
			.tooltip = U"加算カラー",
			.tooltipDetail = U"テクスチャの色に加算されます\n完全に黒(0,0,0,0)の場合は加算処理がスキップされます",
		};
		metadata[PropertyKey{ U"Sprite", U"blendMode" }] = PropertyMetadata{
			.tooltip = U"ブレンドモード",
			.tooltipDetail = U"描画時のブレンドモードを指定します\nNormal: 通常の描画\nAdditive: 加算合成\nSubtractive: 減算合成\nMultiply: 乗算合成",
		};
		metadata[PropertyKey{ U"Sprite", U"preserveAspect" }] = PropertyMetadata{
			.tooltip = U"アスペクト比を保持",
			.tooltipDetail = U"有効にすると、テクスチャの縦横比を保持してノードの領域内に収まるように描画されます",
		};
		metadata[PropertyKey{ U"Sprite", U"nineSliceEnabled" }] = PropertyMetadata{
			.tooltip = U"9スライス機能を有効にするか",
			.tooltipDetail = U"画像を9つの領域に分割し、角を固定サイズで表示しながら辺と中央を伸縮させます",
			.refreshInspectorOnChange = true,
		};
		metadata[PropertyKey{ U"Sprite", U"nineSliceMargin" }] = PropertyMetadata{
			.tooltip = U"9スライスのマージン(素材の端からの距離)",
			.tooltipDetail = U"素材画像の端から何ピクセル内側で領域分割するかを指定します",
			.visibilityCondition = nineSliceVisibilityCondition,
		};
		metadata[PropertyKey{ U"Sprite", U"nineSliceScale" }] = PropertyMetadata{
			.tooltip = U"9スライスのスケール",
			.visibilityCondition = nineSliceVisibilityCondition,
		};
		metadata[PropertyKey{ U"Sprite", U"nineSliceCenterTiled" }] = PropertyMetadata{
			.tooltip = U"中央領域をタイル表示するか",
			.visibilityCondition = nineSliceVisibilityCondition,
		};
		metadata[PropertyKey{ U"Sprite", U"nineSliceTopTiled" }] = PropertyMetadata{
			.tooltip = U"上端領域をタイル表示するか",
			.visibilityCondition = nineSliceVisibilityCondition,
		};
		metadata[PropertyKey{ U"Sprite", U"nineSliceBottomTiled" }] = PropertyMetadata{
			.tooltip = U"下端領域をタイル表示するか",
			.visibilityCondition = nineSliceVisibilityCondition,
		};
		metadata[PropertyKey{ U"Sprite", U"nineSliceLeftTiled" }] = PropertyMetadata{
			.tooltip = U"左端領域をタイル表示するか",
			.visibilityCondition = nineSliceVisibilityCondition,
		};
		metadata[PropertyKey{ U"Sprite", U"nineSliceRightTiled" }] = PropertyMetadata{
			.tooltip = U"右端領域をタイル表示するか",
			.visibilityCondition = nineSliceVisibilityCondition,
		};
		metadata[PropertyKey{ U"Sprite", U"nineSliceFallback" }] = PropertyMetadata{
			.tooltip = U"要素が9スライスのマージンより小さい場合に通常描画にフォールバックするかどうか",
			.visibilityCondition = nineSliceVisibilityCondition,
		};
		
		// TextBox
		metadata[PropertyKey{ U"TextBox", U"fontAssetName" }] = PropertyMetadata{
			.tooltip = U"FontAssetのキー名 (任意)",
			.tooltipDetail = U"指定されている場合、プログラム上ではこのキー名をもとに取得したFontAssetのフォントを使用します\n※プレビューには反映されません",
		};
		metadata[PropertyKey{ U"TextBox", U"fontSize" }] = PropertyMetadata{
			.tooltip = U"フォントサイズ",
			.dragValueChangeStep = 1.0,
		};
		metadata[PropertyKey{ U"TextBox", U"color" }] = PropertyMetadata{
			.tooltip = U"テキスト色",
		};
		metadata[PropertyKey{ U"TextBox", U"horizontalPadding" }] = PropertyMetadata{
			.tooltip = U"水平方向の内側の余白 (左、右)",
		};
		metadata[PropertyKey{ U"TextBox", U"verticalPadding" }] = PropertyMetadata{
			.tooltip = U"垂直方向の内側の余白 (上、下)",
		};
		metadata[PropertyKey{ U"TextBox", U"horizontalAlign" }] = PropertyMetadata{
			.tooltip = U"水平方向の配置",
		};
		metadata[PropertyKey{ U"TextBox", U"verticalAlign" }] = PropertyMetadata{
			.tooltip = U"垂直方向の配置",
		};
		metadata[PropertyKey{ U"TextBox", U"cursorColor" }] = PropertyMetadata{
			.tooltip = U"カーソルの色",
		};
		metadata[PropertyKey{ U"TextBox", U"selectionColor" }] = PropertyMetadata{
			.tooltip = U"選択範囲の色",
		};
		metadata[PropertyKey{ U"TextBox", U"text" }] = PropertyMetadata{
			.refreshesEveryFrame = true,
		};
		metadata[PropertyKey{ U"TextBox", U"readOnly" }] = PropertyMetadata{
			.tooltip = U"読み取り専用",
			.tooltipDetail = U"有効にすると編集不可になりますが、テキストの選択やコピーは可能です",
		};
		
		// TextArea
		metadata[PropertyKey{ U"TextArea", U"fontAssetName" }] = PropertyMetadata{
			.tooltip = U"FontAssetのキー名 (任意)",
			.tooltipDetail = U"指定されている場合、プログラム上ではこのキー名をもとに取得したFontAssetのフォントを使用します\n※プレビューには反映されません",
		};
		metadata[PropertyKey{ U"TextArea", U"fontSize" }] = PropertyMetadata{
			.tooltip = U"フォントサイズ",
			.dragValueChangeStep = 1.0,
		};
		metadata[PropertyKey{ U"TextArea", U"color" }] = PropertyMetadata{
			.tooltip = U"テキスト色",
		};
		metadata[PropertyKey{ U"TextArea", U"horizontalPadding" }] = PropertyMetadata{
			.tooltip = U"水平方向の内側の余白 (左、右)",
		};
		metadata[PropertyKey{ U"TextArea", U"verticalPadding" }] = PropertyMetadata{
			.tooltip = U"垂直方向の内側の余白 (上、下)",
		};
		metadata[PropertyKey{ U"TextArea", U"cursorColor" }] = PropertyMetadata{
			.tooltip = U"カーソルの色",
		};
		metadata[PropertyKey{ U"TextArea", U"selectionColor" }] = PropertyMetadata{
			.tooltip = U"選択範囲の色",
		};
		metadata[PropertyKey{ U"TextArea", U"text" }] = PropertyMetadata{
			.numTextAreaLines = 3,
			.refreshesEveryFrame = true,
		};
		metadata[PropertyKey{ U"TextArea", U"readOnly" }] = PropertyMetadata{
			.tooltip = U"読み取り専用",
			.tooltipDetail = U"有効にすると編集不可になりますが、テキストの選択やコピーは可能です",
		};
		
		// EventTrigger
		metadata[PropertyKey{ U"EventTrigger", U"tag" }] = PropertyMetadata{
			.tooltip = U"プログラムから参照する際のタグ名",
			.tooltipDetail = U"EventTriggerはCanvas上で発生したイベントを統一的に管理するためのコンポーネントです\nプログラム上では毎フレーム、isEventFiredWithTag関数. getFiredEvent(s)WithTag関数, getFiredEventsAll関数を呼ぶことで発生したイベントを取得できます\n\nEventTriggerを使うことでプログラム上からノードを直接操作せずにイベントを受け取れるため、ノード構造の異なるCanvasでもイベント処理が再利用しやすくなります",
		};
		metadata[PropertyKey{ U"EventTrigger", U"triggerType" }] = PropertyMetadata{
			.tooltip = U"イベントを発火させる操作の種類",
		};
		metadata[PropertyKey{ U"EventTrigger", U"recursive" }] = PropertyMetadata{
			.tooltip = U"子孫要素の操作でもイベント発火するかどうか",
		};
		
		// CursorChanger
		metadata[PropertyKey{ U"CursorChanger", U"cursorStyle" }] = PropertyMetadata{
			.tooltip = U"マウスカーソルのスタイル",
			.tooltipDetail = U"要素へのマウスカーソルのホバー中に設定するカーソルスタイルを指定します",
		};
		metadata[PropertyKey{ U"CursorChanger", U"recursive" }] = PropertyMetadata{
			.tooltip = U"子孫要素のホバーでもカーソルを変更するかどうか",
		};
		metadata[PropertyKey{ U"CursorChanger", U"includingDisabled" }] = PropertyMetadata{
			.tooltip = U"InteractionStateがDisabledの要素へのホバーでもカーソルを変更するかどうか",
		};
		
		// Placeholder
		metadata[PropertyKey{ U"Placeholder", U"tag" }] = PropertyMetadata{
			.tooltip = U"プログラムから参照する際のタグ名",
			.tooltipDetail = U"Placeholderはプログラム上からコンポーネント追加や編集等の操作を行う目印として使用するコンポーネントです\nプログラム上ではwalkPlaceholders関数を使用して、タグ名をもとにPlaceholderを巡回できます\n例えば、tagに独自に作成したコンポーネントの種類名を入力し、プログラム上からそのコンポーネントを追加する用途で利用できます",
		};
		metadata[PropertyKey{ U"Placeholder", U"data" }] = PropertyMetadata{
			.tooltip = U"プレースホルダーのデータ (任意)",
			.tooltipDetail = U"自由なデータを文字列で指定できます\nプログラム上ではwalkPlaceholders関数でPlaceholderを巡回し、dataを参照できます",
			.numTextAreaLines = 3,
		};

		// UISound
		metadata[PropertyKey{ U"UISound", U"audioFilePath" }] = PropertyMetadata{
			.tooltip = U"音声ファイルのパス",
			.tooltipDetail = U"audioAssetName使用時は、Editor上でのプレビュー用としてのみ使用されます",
		};
		metadata[PropertyKey{ U"UISound", U"audioAssetName" }] = PropertyMetadata{
			.tooltip = U"AudioAssetのキー名 (任意)",
			.tooltipDetail = U"指定されている場合、プログラム上ではこのキー名をもとに取得したAudioAssetの音声を使用します\n※プレビューには反映されません\n※これを使用しなくてもライブラリ側で内部的にファイルパスをもとにしたキー名でAudioAssetを使用するため、\n　パフォーマンス上の利点は特にありません。AudioAssetのキー名を手動で管理したい場合のみ使用してください",
		};
		metadata[PropertyKey{ U"UISound", U"triggerType" }] = PropertyMetadata{
			.tooltip = U"音声を再生する操作の種類",
		};
		metadata[PropertyKey{ U"UISound", U"volume" }] = PropertyMetadata{
			.tooltip = U"音量 (0.0 ~ 1.0)",
			.dragValueChangeStep = 0.01,
		};
		metadata[PropertyKey{ U"UISound", U"recursive" }] = PropertyMetadata{
			.tooltip = U"子孫要素のインタラクションも対象にするかどうか",
		};
		metadata[PropertyKey{ U"UISound", U"includingDisabled" }] = PropertyMetadata{
			.tooltip = U"InteractionStateがDisabledの要素への操作でも音声を再生するかどうか",
		};
		
		// Tween
		metadata[PropertyKey{ U"Tween", U"active" }] = PropertyMetadata{
			.tooltip = U"アニメーションの再生状態",
		};
		metadata[PropertyKey{ U"Tween", U"target" }] = PropertyMetadata{
			.tooltip = U"アニメーション対象",
			.tooltipDetail = U"None: アニメーションしない\nPosition: TransformEffectのpositionプロパティ\nScale: TransformEffectのscaleプロパティ\nRotation: TransformEffectのrotationプロパティ\nColor: TransformEffectのcolorプロパティ",
			.refreshInspectorOnChange = true,
		};
		
		// Vec2用プロパティの条件付き表示
		const auto tweenVec2VisibilityCondition = [](const ComponentBase& component) -> bool
		{
			if (const auto* tween = dynamic_cast<const Tween*>(&component))
			{
				const auto target = tween->target();
				return target == TweenTarget::Position || target == TweenTarget::Scale;
			}
			return false;
		};
		
		metadata[PropertyKey{ U"Tween", U"value1Vec2" }] = PropertyMetadata{
			.tooltip = U"開始値",
			.visibilityCondition = tweenVec2VisibilityCondition,
		};
		metadata[PropertyKey{ U"Tween", U"value2Vec2" }] = PropertyMetadata{
			.tooltip = U"終了値",
			.visibilityCondition = tweenVec2VisibilityCondition,
		};
		
		// double用プロパティの条件付き表示（Rotation用）
		const auto tweenDoubleVisibilityCondition = [](const ComponentBase& component) -> bool
		{
			if (const auto* tween = dynamic_cast<const Tween*>(&component))
			{
				return tween->target() == TweenTarget::Rotation;
			}
			return false;
		};
		
		metadata[PropertyKey{ U"Tween", U"value1Double" }] = PropertyMetadata{
			.tooltip = U"開始角度（度）",
			.visibilityCondition = tweenDoubleVisibilityCondition,
			.dragValueChangeStep = 1.0,
		};
		metadata[PropertyKey{ U"Tween", U"value2Double" }] = PropertyMetadata{
			.tooltip = U"終了角度（度）",
			.visibilityCondition = tweenDoubleVisibilityCondition,
			.dragValueChangeStep = 1.0,
		};
		
		// ColorF用プロパティの条件付き表示
		const auto tweenColorVisibilityCondition = [](const ComponentBase& component) -> bool
		{
			if (const auto* tween = dynamic_cast<const Tween*>(&component))
			{
				return tween->target() == TweenTarget::Color;
			}
			return false;
		};
		
		metadata[PropertyKey{ U"Tween", U"value1Color" }] = PropertyMetadata{
			.tooltip = U"開始値",
			.visibilityCondition = tweenColorVisibilityCondition,
		};
		metadata[PropertyKey{ U"Tween", U"value2_color" }] = PropertyMetadata{
			.tooltip = U"終了値",
			.visibilityCondition = tweenColorVisibilityCondition,
		};
		
		metadata[PropertyKey{ U"Tween", U"easing" }] = PropertyMetadata{
			.tooltip = U"イージング関数",
			.tooltipDetail = U"時間に対する値の変化のさせ方を指定します",
		};
		metadata[PropertyKey{ U"Tween", U"duration" }] = PropertyMetadata{
			.tooltip = U"アニメーション時間（秒）",
			.dragValueChangeStep = 0.1,
		};
		metadata[PropertyKey{ U"Tween", U"delay" }] = PropertyMetadata{
			.tooltip = U"開始までの遅延時間（秒）",
			.dragValueChangeStep = 0.1,
		};
		metadata[PropertyKey{ U"Tween", U"loopType" }] = PropertyMetadata{
			.tooltip = U"ループの種類",
			.tooltipDetail = U"None: ループなし\nLoop: 通常ループ\nPingPong: 往復ループ",
		};
		metadata[PropertyKey{ U"Tween", U"restartsOnActive" }] = PropertyMetadata{
			.tooltip = U"アクティブ時に最初から再生",
			.tooltipDetail = U"activeプロパティがfalse→trueになった時、またはノード自体のアクティブ状態がfalse→trueになった時に、アニメーションを最初から再生し直すかどうか",
		};
		
		return metadata;
	}
}
