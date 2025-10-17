#include "PropertyMetaData.hpp"

namespace noco::editor
{
	[[nodiscard]]
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
			.tooltipDetail = U"このNodeとその子要素の表示/非表示状態を制御します\n無効の場合、各コンポーネントのupdate関数およびdraw関数は呼び出されません",
		};
		metadata[PropertyKey{ U"Node", U"isHitTarget" }] = PropertyMetadata{
			.tooltip = U"ヒットテストの対象にするどうか",
			.tooltipDetail = U"無効にすると、この要素はヒットテスト(要素にマウスカーソルがホバーしているかどうかの判定)の対象外となり、親要素のInteractionStateを受け継ぎます\n※無効の場合、ヒットテストでは要素の存在自体が無視されるため、背面にある要素にホバーが可能となります\n※無効の場合、TextBox等のマウス操作を利用するコンポーネントも入力を受け付けなくなります",
		};
		metadata[PropertyKey{ U"Node", U"hitPadding" }] = PropertyMetadata{
			.tooltip = U"ヒットテスト領域の拡縮 (左、右、上、下)",
			.tooltipDetail = U"ヒットテスト(要素にマウスカーソルがホバーしているかどうかの判定)に使用する領域を、指定されたピクセル数だけ拡大・縮小します\n正の値で領域を拡大、負の値で領域を縮小します\n実際の見た目よりもずれた位置にマウスカーソルがあっても反応させたい場合に使用できます",
		};
		metadata[PropertyKey{ U"Node", U"inheritChildrenHover" }] = PropertyMetadata{
			.tooltip = U"子要素のホバー状態(Hovered)を継承するかどうか",
			.tooltipDetail = U"有効にすると、子要素のInteractionStateがHoveredの場合に、このNodeのInteractionStateがHoveredになります\n※このNodeのInteractionStateがPressed・Disabledの場合は影響を受けません",
		};
		metadata[PropertyKey{ U"Node", U"inheritChildrenPress" }] = PropertyMetadata{
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
			.tooltipDetail = U"styleStateとは、要素の状態を識別するために設定する文字列です(例: \"focused\")\n各プロパティの値はstyleState毎に異なる値を設定でき、状態に応じて見た目を変えることができます\nstyleStateはノード毎に1つのみ設定できます\n\n親要素のstyleStateがあればそれを受け継ぎます\n適用の優先度は自身の要素のstyleStateが最も高く、遠い親になるにつれて優先度は下がります",
		};
		metadata[PropertyKey{ U"Node", U"zOrderInSiblings" }] = PropertyMetadata{
			.tooltip = U"兄弟要素間での表示順序",
			.tooltipDetail = U"兄弟間で値が大きい要素を手前に表示します\nupdateKeyInput・drawの実行順序、およびヒットテストの優先度に影響します\n※update・lateUpdateの実行順序には影響しません\n※兄弟要素間の実行順序にのみ影響します。異なる親を持つ要素同士の実行順序には影響しません\n※要素間でzOrderInSiblingsの値が同じ場合、Hierarchy上で下にある要素が手前に表示されます",
			.dragValueChangeStep = 1.0,
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
		
		// Transform関連
		metadata[PropertyKey{ U"Transform", U"translate" }] = PropertyMetadata{
			.tooltip = U"平行移動",
			.tooltipDetail = U"要素を平行移動させます\nこの値による平行移動はレイアウト計算に影響を与えません\n※Transformはレイアウトの再計算を必要としないため、要素を高速に平行移動できます。そのため、アニメーション等の用途で利用できます\n※hitTestAffectedがtrueの場合、マウスカーソルのホバー判定にも平行移動を適用します",
		};
		metadata[PropertyKey{ U"Transform", U"scale" }] = PropertyMetadata{
			.tooltip = U"スケール",
			.tooltipDetail = U"要素のサイズを拡大・縮小するスケールを指定します\nこの値による拡大縮小はレイアウト計算に影響を与えません\n※Transformはレイアウトの再計算を必要としないため、要素の大きさを高速に変更できます。そのため、アニメーション等の用途で利用できます\n※描画内容はスケールに応じて伸縮されます\n※hitTestAffectedがtrueの場合、マウスカーソルのホバー判定にも拡大縮小を適用します",
		};
		metadata[PropertyKey{ U"Transform", U"pivot" }] = PropertyMetadata{
			.tooltip = U"基準点 (X、Y)",
			.tooltipDetail = U"scaleによる拡大縮小とrotationによる回転の基準点を0～1の比率で指定します\n(0,0)は左上、(1,1)は右下を表します",
		};
		metadata[PropertyKey{ U"Transform", U"rotation" }] = PropertyMetadata{
			.tooltip = U"回転角度",
			.tooltipDetail = U"要素の回転角度を度数法で指定します\n正の値で時計回り、負の値で反時計回りに回転します\n回転の中心はpivotで指定した基準点になります\n※この値による回転はレイアウト計算に影響を与えません\n※hitTestAffectedがtrueの場合、マウスカーソルのホバー判定にも回転を適用します",
			.dragValueChangeStep = 1.0,
		};
		metadata[PropertyKey{ U"Transform", U"hitTestAffected" }] = PropertyMetadata{
			.tooltip = U"ヒットテスト領域へ適用するか",
			.tooltipDetail = U"Transformの平行移動・スケール・回転をマウスのホバー判定に適用するかどうかを指定します\ntrueの場合：translate, scale, rotationの変換がホバー判定に反映されます\nfalseの場合：変換は描画のみに適用され、ホバー判定は元の位置で行われます",
		};
		metadata[PropertyKey{ U"Transform", U"color" }] = PropertyMetadata{
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
		metadata[PropertyKey{ U"RectRenderer", U"outlineThicknessInner" }] = PropertyMetadata{
			.tooltip = U"アウトラインの内側の太さ",
			.dragValueChangeStep = 1.0,
		};
		metadata[PropertyKey{ U"RectRenderer", U"outlineThicknessOuter" }] = PropertyMetadata{
			.tooltip = U"アウトラインの外側の太さ",
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
		
		// ShapeRenderer
		metadata[PropertyKey{ U"ShapeRenderer", U"shapeType" }] = PropertyMetadata{
			.tooltip = U"描画する図形の種類",
			.tooltipDetail = U"Cross: バツ印\nPlus: プラス記号\nPentagon: 正五角形\nHexagon: 正六角形\nNgon: 正N角形\nStar: 五芒星\nNStar: 星形\nArrow: 矢印\nDoubleHeadedArrow: 両方向矢印\nRhombus: ひし形\nRectBalloon: 長方形の吹き出し\nStairs: 階段形\nHeart: ハート形\nSquircle: 正方形と円の中間\nAstroid: 星芒形",
			.refreshInspectorOnChange = true,
		};
		metadata[PropertyKey{ U"ShapeRenderer", U"preserveAspect" }] = PropertyMetadata{
			.tooltip = U"アスペクト比を保持",
			.tooltipDetail = U"trueの場合、図形のアスペクト比を保持し、ノードの短い辺に内接するようサイズを調整します。\nfalseの場合、ノードのサイズに合わせて図形を変形します。",
			.visibilityCondition = [](const ComponentBase& component)
			{
				if (const auto* shapeRenderer = dynamic_cast<const ShapeRenderer*>(&component))
				{
					const auto type = shapeRenderer->shapeType();
					return
						type != ShapeType::RectBalloon &&
						type != ShapeType::Arrow &&
						type != ShapeType::DoubleHeadedArrow &&
						type != ShapeType::Rhombus &&
						type != ShapeType::Stairs &&
						type != ShapeType::Astroid;
				}
				return false;
			},
		};
		metadata[PropertyKey{ U"ShapeRenderer", U"thickness" }] = PropertyMetadata{
			.tooltip = U"太さ",
			.tooltipDetail = U"Cross、Plus、Arrow、DoubleHeadedArrowの線の太さ",
			.visibilityCondition = [](const ComponentBase& component)
			{
				if (const auto* shapeRenderer = dynamic_cast<const ShapeRenderer*>(&component))
				{
					const auto type = shapeRenderer->shapeType();
					return
						type == ShapeType::Cross ||
						type == ShapeType::Plus ||
						type == ShapeType::Arrow ||
						type == ShapeType::DoubleHeadedArrow;
				}
				return false;
			},
			.dragValueChangeStep = 1.0,
		};
		metadata[PropertyKey{ U"ShapeRenderer", U"sides" }] = PropertyMetadata{
			.tooltip = U"辺の数",
			.tooltipDetail = U"Ngon(正N角形)の辺の数",
			.visibilityCondition = [](const ComponentBase& component)
			{
				if (const auto* shapeRenderer = dynamic_cast<const ShapeRenderer*>(&component))
				{
					return shapeRenderer->shapeType() == ShapeType::Ngon;
				}
				return false;
			},
			.dragValueChangeStep = 1.0,
		};
		metadata[PropertyKey{ U"ShapeRenderer", U"points" }] = PropertyMetadata{
			.tooltip = U"尖端の数",
			.tooltipDetail = U"NStar(星形)の尖端の数",
			.visibilityCondition = [](const ComponentBase& component)
			{
				if (const auto* shapeRenderer = dynamic_cast<const ShapeRenderer*>(&component))
				{
					return shapeRenderer->shapeType() == ShapeType::NStar;
				}
				return false;
			},
			.dragValueChangeStep = 1.0,
		};
		metadata[PropertyKey{ U"ShapeRenderer", U"innerRatio" }] = PropertyMetadata{
			.tooltip = U"内周の比率",
			.tooltipDetail = U"NStar(星形)の内周の半径を外周に対する比率で指定 (0.0〜1.0)",
			.visibilityCondition = [](const ComponentBase& component)
			{
				if (const auto* shapeRenderer = dynamic_cast<const ShapeRenderer*>(&component))
				{
					return shapeRenderer->shapeType() == ShapeType::NStar;
				}
				return false;
			},
			.dragValueChangeStep = 0.1,
		};
		metadata[PropertyKey{ U"ShapeRenderer", U"startPoint" }] = PropertyMetadata{
			.tooltip = U"始点",
			.tooltipDetail = U"Arrow、DoubleHeadedArrowの始点 (要素サイズに対する 0〜1 の比率。0,0 が左上)",
			.visibilityCondition = [](const ComponentBase& component)
			{
				if (const auto* shapeRenderer = dynamic_cast<const ShapeRenderer*>(&component))
				{
					const auto type = shapeRenderer->shapeType();
					return type == ShapeType::Arrow || type == ShapeType::DoubleHeadedArrow;
				}
				return false;
			},
			.dragValueChangeStep = 0.1,
		};
		metadata[PropertyKey{ U"ShapeRenderer", U"endPoint" }] = PropertyMetadata{
			.tooltip = U"終点",
			.tooltipDetail = U"Arrow、DoubleHeadedArrowの終点 (要素サイズに対する 0〜1 の比率。0,0 が左上)",
			.visibilityCondition = [](const ComponentBase& component)
			{
				if (const auto* shapeRenderer = dynamic_cast<const ShapeRenderer*>(&component))
				{
					const auto type = shapeRenderer->shapeType();
					return type == ShapeType::Arrow || type == ShapeType::DoubleHeadedArrow;
				}
				return false;
			},
			.dragValueChangeStep = 0.1,
		};
		metadata[PropertyKey{ U"ShapeRenderer", U"arrowHeadSize" }] = PropertyMetadata{
			.tooltip = U"矢じりのサイズ (幅, 高さ, px)",
			.tooltipDetail = U"Arrow、DoubleHeadedArrowの矢じりのサイズをピクセル単位で指定",
			.visibilityCondition = [](const ComponentBase& component)
			{
				if (const auto* shapeRenderer = dynamic_cast<const ShapeRenderer*>(&component))
				{
					const auto type = shapeRenderer->shapeType();
					return type == ShapeType::Arrow || type == ShapeType::DoubleHeadedArrow;
				}
				return false;
			},
			.dragValueChangeStep = 1.0,
		};
		metadata[PropertyKey{ U"ShapeRenderer", U"targetPoint" }] = PropertyMetadata{
			.tooltip = U"ターゲット座標",
			.tooltipDetail = U"RectBalloon(吹き出し)の先端が指す座標 (0〜1 の比率で指定、0,0 が左上)",
			.visibilityCondition = [](const ComponentBase& component)
			{
				if (const auto* shapeRenderer = dynamic_cast<const ShapeRenderer*>(&component))
				{
					return shapeRenderer->shapeType() == ShapeType::RectBalloon;
				}
				return false;
			},
			.dragValueChangeStep = 0.1,
		};
		metadata[PropertyKey{ U"ShapeRenderer", U"tailRatio" }] = PropertyMetadata{
			.tooltip = U"吹き出しの根元の比率",
			.tooltipDetail = U"RectBalloon(吹き出し)の根元の位置の比率 (0.0〜1.0)",
			.visibilityCondition = [](const ComponentBase& component)
			{
				if (const auto* shapeRenderer = dynamic_cast<const ShapeRenderer*>(&component))
				{
					return shapeRenderer->shapeType() == ShapeType::RectBalloon;
				}
				return false;
			},
			.dragValueChangeStep = 0.1,
		};
		metadata[PropertyKey{ U"ShapeRenderer", U"stairCount" }] = PropertyMetadata{
			.tooltip = U"階段数",
			.tooltipDetail = U"Stairs(階段形)の階段数",
			.visibilityCondition = [](const ComponentBase& component)
			{
				if (const auto* shapeRenderer = dynamic_cast<const ShapeRenderer*>(&component))
				{
					return shapeRenderer->shapeType() == ShapeType::Stairs;
				}
				return false;
			},
			.dragValueChangeStep = 1.0,
		};
		metadata[PropertyKey{ U"ShapeRenderer", U"upStairs" }] = PropertyMetadata{
			.tooltip = U"右上に上がるか",
			.tooltipDetail = U"Stairs(階段形)が右上に上がるか、左上に上がるか",
			.visibilityCondition = [](const ComponentBase& component)
			{
				if (const auto* shapeRenderer = dynamic_cast<const ShapeRenderer*>(&component))
				{
					return shapeRenderer->shapeType() == ShapeType::Stairs;
				}
				return false;
			},
		};
		metadata[PropertyKey{ U"ShapeRenderer", U"squircleQuality" }] = PropertyMetadata{
			.tooltip = U"品質",
			.tooltipDetail = U"Squircle(正方形と円の中間)の描画品質 (頂点数)",
			.visibilityCondition = [](const ComponentBase& component)
			{
				if (const auto* shapeRenderer = dynamic_cast<const ShapeRenderer*>(&component))
				{
					return shapeRenderer->shapeType() == ShapeType::Squircle;
				}
				return false;
			},
			.dragValueChangeStep = 1.0,
		};
		metadata[PropertyKey{ U"ShapeRenderer", U"fillColor" }] = PropertyMetadata{
			.tooltip = U"塗りつぶし色",
		};
		metadata[PropertyKey{ U"ShapeRenderer", U"outlineColor" }] = PropertyMetadata{
			.tooltip = U"アウトライン色",
		};
		metadata[PropertyKey{ U"ShapeRenderer", U"outlineThickness" }] = PropertyMetadata{
			.tooltip = U"アウトラインの太さ",
			.dragValueChangeStep = 1.0,
		};
		metadata[PropertyKey{ U"ShapeRenderer", U"blendMode" }] = PropertyMetadata{
			.tooltip = U"ブレンドモード",
			.tooltipDetail = U"描画時のブレンドモードを指定します\nNormal: 通常の描画\nAdditive: 加算合成\nSubtractive: 減算合成\nMultiply: 乗算合成",
		};
		
		// Label
		metadata[PropertyKey{ U"Label", U"text" }] = PropertyMetadata{
			.tooltip = U"表示するテキスト",
			.numTextAreaLines = 3,
		};
		metadata[PropertyKey{ U"Label", U"fontAssetName" }] = PropertyMetadata{
			.tooltip = U"FontAssetのキー名 (任意)",
			.tooltipDetail = U"指定されている場合、プログラム上ではこのキー名をもとに取得したFontAssetのフォントを使用します\n※エディタ上でプレビューするには、Custom/FontAssets内にJSONファイルを作成してください",
		};
		metadata[PropertyKey{ U"Label", U"fontSize" }] = PropertyMetadata{
			.tooltip = U"フォントサイズ",
			.dragValueChangeStep = 1.0,
		};
		metadata[PropertyKey{ U"Label", U"gradationType" }] = PropertyMetadata{
			.tooltip = U"テキストのグラデーションタイプ",
			.tooltipDetail = U"None: 単色で描画します\nTopBottom: 上下にグラデーションをかけます\nLeftRight: 左右にグラデーションをかけます",
			.refreshInspectorOnChange = true,
		};
		metadata[PropertyKey{ U"Label", U"color" }] = PropertyMetadata{
			.tooltip = U"テキスト色",
			.visibilityCondition = [](const ComponentBase& component)
			{
				if (const auto* label = dynamic_cast<const Label*>(&component))
				{
					return label->gradationType().hasAnyStateEqualTo(LabelGradationType::None);
				}
				return false;
			},
		};
		metadata[PropertyKey{ U"Label", U"gradationColor1" }] = PropertyMetadata{
			.tooltip = U"グラデーション色 1",
			.tooltipDetail = U"TopBottom: 上側の色\nLeftRight: 左側の色",
			.visibilityCondition = [](const ComponentBase& component)
			{
				if (const auto* label = dynamic_cast<const Label*>(&component))
				{
					return !label->gradationType().hasAnyStateEqualTo(LabelGradationType::None);
				}
				return false;
			},
		};
		metadata[PropertyKey{ U"Label", U"gradationColor2" }] = PropertyMetadata{
			.tooltip = U"グラデーション色 2",
			.tooltipDetail = U"TopBottom: 下側の色\nLeftRight: 右側の色",
			.visibilityCondition = [](const ComponentBase& component)
			{
				if (const auto* label = dynamic_cast<const Label*>(&component))
				{
					return !label->gradationType().hasAnyStateEqualTo(LabelGradationType::None);
				}
				return false;
			},
		};
		metadata[PropertyKey{ U"Label", U"sizingMode" }] = PropertyMetadata{
			.tooltip = U"サイズに関するモード",
			.tooltipDetail = U"Fixed: 固定フォントサイズで描画します\nAutoShrink: ノードサイズに収まるようフォントサイズを自動縮小します\nAutoShrinkWidth: ノードサイズに収まるよう文字の幅のみを自動縮小します\nAutoResize: テキストの内容に応じてノードサイズを自動でリサイズします\n\n※AutoShrink、AutoShrinkWidth、AutoResizeはテキストやその他の値に変化が発生した時の\n　再計算にかかる負荷が高いため、不要な場合はなるべくFixedを指定することを推奨します\n\n※AutoResizeを利用してテキストの周囲や背面に装飾を加えたい場合、paddingを設定してノードサイズが\n　テキストの内容より大きくなるようにして、同じノードのLabelの手前にコンポーネントを追加してください",
			.refreshInspectorOnChange = true,
		};
		metadata[PropertyKey{ U"Label", U"minFontSize" }] = PropertyMetadata{
			.tooltip = U"最小フォントサイズ",
			.tooltipDetail = U"AutoShrink時の最小フォントサイズ",
			.visibilityCondition = [](const ComponentBase& component)
			{
				if (auto label = dynamic_cast<const Label*>(&component))
				{
					return label->sizingMode().defaultValue() == LabelSizingMode::AutoShrink;
				}
				return false;
			},
			.dragValueChangeStep = 1.0,
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
			.refreshInspectorOnChange = true,
		};
		metadata[PropertyKey{ U"Label", U"underlineColor" }] = PropertyMetadata{
			.tooltip = U"下線の色",
			.visibilityCondition = [](const ComponentBase& component) -> bool
			{
				if (const auto* label = dynamic_cast<const Label*>(&component))
				{
					return label->underlineStyle().hasAnyStateEqualTo(LabelUnderlineStyle::Solid);
				}
				return false;
			},
		};
		metadata[PropertyKey{ U"Label", U"underlineThickness" }] = PropertyMetadata{
			.tooltip = U"下線の太さ",
			.visibilityCondition = [](const ComponentBase& component) -> bool
			{
				if (const auto* label = dynamic_cast<const Label*>(&component))
				{
					return label->underlineStyle().hasAnyStateEqualTo(LabelUnderlineStyle::Solid);
				}
				return false;
			},
			.dragValueChangeStep = 1.0,
		};
		metadata[PropertyKey{ U"Label", U"outlineColor" }] = PropertyMetadata{
			.tooltip = U"アウトラインの色",
			.tooltipDetail = U"※ビットマップフォントの場合はアウトラインは描画されません",
		};
		metadata[PropertyKey{ U"Label", U"outlineFactorInner" }] = PropertyMetadata{
			.tooltip = U"アウトライン内側のしきい値",
			.tooltipDetail = U"SDF/MSDFフォント用のアウトライン内側しきい値（通常0.5、0.0でアウトラインなし）\n※ビットマップフォントの場合はアウトラインは描画されません",
			.dragValueChangeStep = 0.01,
		};
		metadata[PropertyKey{ U"Label", U"outlineFactorOuter" }] = PropertyMetadata{
			.tooltip = U"アウトライン外側のしきい値",
			.tooltipDetail = U"SDF/MSDFフォント用のアウトライン外側しきい値（両方、0.0でアウトラインなし）\n※ビットマップフォントの場合はアウトラインは描画されません",
			.dragValueChangeStep = 0.01,
		};
		metadata[PropertyKey{ U"Label", U"shadowColor" }] = PropertyMetadata{
			.tooltip = U"シャドウの色",
			.tooltipDetail = U"アルファ値が0より大きい場合にシャドウが有効になります\n※ビットマップフォントの場合はシャドウは描画されません",
		};
		metadata[PropertyKey{ U"Label", U"shadowOffset" }] = PropertyMetadata{
			.tooltip = U"シャドウのオフセット (X, Y)",
			.tooltipDetail = U"シャドウの表示位置をピクセル単位で指定\n※ビットマップフォントの場合はシャドウは描画されません",
			.dragValueChangeStep = 1.0,
		};
		
		// TextureFontLabel
		metadata[PropertyKey{ U"TextureFontLabel", U"textureFilePath" }] = PropertyMetadata{
			.tooltip = U"テクスチャファイルのパス",
			.tooltipDetail = U"※textureAssetName使用時は、Editor上でのプレビュー用としてのみ使用されます",
		};
		metadata[PropertyKey{ U"TextureFontLabel", U"textureAssetName" }] = PropertyMetadata{
			.tooltip = U"TextureAssetのキー名 (任意)",
			.tooltipDetail = U"指定されている場合、プログラム上ではこのキー名をもとに取得したTextureAssetのテクスチャを使用します\n※プレビューには反映されません\n※これを使用しなくてもライブラリ側で内部的にファイルパスをもとにしたHashTableでアセットが管理されるため、\n　パフォーマンス上の利点は特にありません。TextureAssetのキー名を手動で管理したい場合のみ使用してください",
		};
		metadata[PropertyKey{ U"TextureFontLabel", U"characterSet" }] = PropertyMetadata{
			.tooltip = U"文字セット",
			.tooltipDetail = U"テクスチャに含まれる文字を左上から右下への順番で指定します\n※改行は無視されるため、見やすさのために自由に改行を入れることができます",
			.numTextAreaLines = 3,
		};
		metadata[PropertyKey{ U"TextureFontLabel", U"textureCellSize" }] = PropertyMetadata{
			.tooltip = U"1文字分のセルサイズ (幅, 高さ)",
			.tooltipDetail = U"テクスチャ上の1文字分のピクセルサイズ",
			.dragValueChangeStep = 1.0,
		};
		metadata[PropertyKey{ U"TextureFontLabel", U"textureOffset" }] = PropertyMetadata{
			.tooltip = U"テクスチャのオフセット (X, Y)",
			.tooltipDetail = U"文字グリッドの開始位置のオフセット",
			.dragValueChangeStep = 1.0,
		};
		metadata[PropertyKey{ U"TextureFontLabel", U"textureGridColumns" }] = PropertyMetadata{
			.tooltip = U"グリッドの列数",
			.tooltipDetail = U"テクスチャグリッドの横方向の文字数",
			.dragValueChangeStep = 1.0,
		};
		metadata[PropertyKey{ U"TextureFontLabel", U"textureGridRows" }] = PropertyMetadata{
			.tooltip = U"グリッドの行数",
			.tooltipDetail = U"テクスチャグリッドの縦方向の文字数",
			.dragValueChangeStep = 1.0,
		};
		metadata[PropertyKey{ U"TextureFontLabel", U"text" }] = PropertyMetadata{
			.tooltip = U"表示するテキスト",
			.numTextAreaLines = 3,
		};
		metadata[PropertyKey{ U"TextureFontLabel", U"sizingMode" }] = PropertyMetadata{
			.tooltip = U"サイズに関するモード",
			.tooltipDetail = U"Fixed: 固定文字サイズで描画します\nAutoShrink: ノードサイズに収まるよう文字サイズを自動縮小します\nAutoShrinkWidth: ノードサイズに収まるよう文字の幅のみを自動縮小します\nAutoResize: テキストの内容に応じてノードサイズを自動でリサイズします\n\n※AutoShrink、AutoShrinkWidth、AutoResizeはテキストやその他の値に変化が発生した時の\n　再計算にかかる負荷が高いため、不要な場合はなるべくFixedを指定することを推奨します\n\n※AutoResizeを利用してテキストの周囲や背面に装飾を加えたい場合、paddingを設定してノードサイズが\n　テキストの内容より大きくなるようにして、同じノードのTextureFontLabelの手前にコンポーネントを追加してください",
			.refreshInspectorOnChange = true,
		};
		metadata[PropertyKey{ U"TextureFontLabel", U"characterSize" }] = PropertyMetadata{
			.tooltip = U"文字の描画サイズ (幅, 高さ)",
			.tooltipDetail = U"描画時の1文字分のサイズ",
			.dragValueChangeStep = 1.0,
		};
		metadata[PropertyKey{ U"TextureFontLabel", U"characterSpacing" }] = PropertyMetadata{
			.tooltip = U"文字間隔 (横間隔, 行間隔)",
			.tooltipDetail = U"X: 文字同士の横間隔\nY: 行同士の間隔",
		};
		metadata[PropertyKey{ U"TextureFontLabel", U"horizontalAlign" }] = PropertyMetadata{
			.tooltip = U"水平方向の配置",
		};
		metadata[PropertyKey{ U"TextureFontLabel", U"verticalAlign" }] = PropertyMetadata{
			.tooltip = U"垂直方向の配置",
		};
		metadata[PropertyKey{ U"TextureFontLabel", U"padding" }] = PropertyMetadata{
			.tooltip = U"内側の余白 (左、右、上、下)",
		};
		metadata[PropertyKey{ U"TextureFontLabel", U"horizontalOverflow" }] = PropertyMetadata{
			.tooltip = U"水平方向にはみ出す場合の処理",
			.tooltipDetail = U"Wrap: 自動的に折り返します\nOverflow: 右へはみ出して描画します",
		};
		metadata[PropertyKey{ U"TextureFontLabel", U"verticalOverflow" }] = PropertyMetadata{
			.tooltip = U"垂直方向にはみ出す場合の処理",
			.tooltipDetail = U"Clip: 領域をはみ出した文字は描画しません\nOverflow: 下へはみ出して描画します",
		};
		metadata[PropertyKey{ U"TextureFontLabel", U"preserveAspect" }] = PropertyMetadata{
			.tooltip = U"アスペクト比を保持",
			.tooltipDetail = U"テクスチャの文字のアスペクト比を保持して描画します",
		};
		metadata[PropertyKey{ U"TextureFontLabel", U"color" }] = PropertyMetadata{
			.tooltip = U"文字の色",
			.tooltipDetail = U"テクスチャの色に乗算されます",
		};
		metadata[PropertyKey{ U"TextureFontLabel", U"addColor" }] = PropertyMetadata{
			.tooltip = U"加算カラー",
			.tooltipDetail = U"テクスチャの色に加算されます",
		};
		metadata[PropertyKey{ U"TextureFontLabel", U"blendMode" }] = PropertyMetadata{
			.tooltip = U"ブレンドモード",
			.tooltipDetail = U"Normal: 通常\nAdditive: 加算\nSubtractive: 減算\nMultiply: 乗算",
		};
		metadata[PropertyKey{ U"TextureFontLabel", U"textureFilter" }] = PropertyMetadata{
			.tooltip = U"テクスチャフィルター",
			.tooltipDetail = U"Default: デフォルトのフィルタリング\nNearest: 最近傍（ピクセルアート向き）\nLinear: リニア（滑らか）\nAniso: 異方性フィルタリング（高品質）",
		};
		metadata[PropertyKey{ U"TextureFontLabel", U"textureAddressMode" }] = PropertyMetadata{
			.tooltip = U"テクスチャアドレスモード",
			.tooltipDetail = U"Default: デフォルトのアドレスモード\nRepeat: タイル状に繰り返し\nMirror: 反転しながら繰り返し\nClamp: 端の色を延長\nBorderColor: 範囲外は境界色",
		};

		// Sprite
		metadata[PropertyKey{ U"Sprite", U"textureFilePath" }] = PropertyMetadata{
			.tooltip = U"テクスチャファイルのパス",
			.tooltipDetail = U"※textureAssetName使用時は、Editor上でのプレビュー用としてのみ使用されます",
		};
		metadata[PropertyKey{ U"Sprite", U"textureAssetName" }] = PropertyMetadata{
			.tooltip = U"TextureAssetのキー名 (任意)",
			.tooltipDetail = U"指定されている場合、プログラム上ではこのキー名をもとに取得したTextureAssetのテクスチャを使用します\n※プレビューには反映されません\n※これを使用しなくてもライブラリ側で内部的にファイルパスをもとにしたHashTableでアセットが管理されるため、\n　パフォーマンス上の利点は特にありません。TextureAssetのキー名を手動で管理したい場合のみ使用してください",
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
		metadata[PropertyKey{ U"Sprite", U"textureRegionMode" }] = PropertyMetadata{
			.tooltip = U"テクスチャ領域の指定モード",
			.tooltipDetail = U"Full: テクスチャ全体を使用\nOffsetSize: textureOffset/textureSizeで指定した領域を使用\nGrid: グリッド配置の中からtextureGridIndexで指定したセルを使用",
			.refreshInspectorOnChange = true,
		};
		
		// textureRegionModeがOffsetSizeの時のみ表示するプロパティの条件
		const auto offsetSizeRegionVisibilityCondition = [](const ComponentBase& component) -> bool
		{
			if (const auto* sprite = dynamic_cast<const Sprite*>(&component))
			{
				return sprite->textureRegionMode().hasAnyStateEqualTo(TextureRegionMode::OffsetSize);
			}
			return false;
		};
		
		// textureRegionModeがGridの時のみ表示するプロパティの条件
		const auto gridRegionVisibilityCondition = [](const ComponentBase& component) -> bool
		{
			if (const auto* sprite = dynamic_cast<const Sprite*>(&component))
			{
				return sprite->textureRegionMode().hasAnyStateEqualTo(TextureRegionMode::Grid);
			}
			return false;
		};
		
		// OffsetSizeまたはGridの時に表示するプロパティの条件
		const auto offsetOrGridRegionVisibilityCondition = [](const ComponentBase& component) -> bool
		{
			if (const auto* sprite = dynamic_cast<const Sprite*>(&component))
			{
				return sprite->textureRegionMode().hasAnyStateEqualTo(TextureRegionMode::OffsetSize) ||
				       sprite->textureRegionMode().hasAnyStateEqualTo(TextureRegionMode::Grid);
			}
			return false;
		};
		
		metadata[PropertyKey{ U"Sprite", U"textureOffset" }] = PropertyMetadata{
			.tooltip = U"切り出し開始位置 (ピクセル)",
			.tooltipDetail = U"OffsetSize: テクスチャの切り出し開始位置\nGrid: グリッド全体の開始位置",
			.visibilityCondition = offsetOrGridRegionVisibilityCondition,
		};
		metadata[PropertyKey{ U"Sprite", U"textureSize" }] = PropertyMetadata{
			.tooltip = U"切り出しサイズ (ピクセル)",
			.tooltipDetail = U"切り出す領域のサイズを指定します",
			.visibilityCondition = offsetSizeRegionVisibilityCondition,
		};
		metadata[PropertyKey{ U"Sprite", U"textureGridCellSize" }] = PropertyMetadata{
			.tooltip = U"グリッドの1セルのサイズ (ピクセル)",
			.tooltipDetail = U"テクスチャの各セルのサイズを指定します",
			.visibilityCondition = gridRegionVisibilityCondition,
		};
		metadata[PropertyKey{ U"Sprite", U"textureGridColumns" }] = PropertyMetadata{
			.tooltip = U"グリッドの列数",
			.tooltipDetail = U"テクスチャの横方向のセル数を指定します",
			.visibilityCondition = gridRegionVisibilityCondition,
			.dragValueChangeStep = 1.0,
		};
		metadata[PropertyKey{ U"Sprite", U"textureGridRows" }] = PropertyMetadata{
			.tooltip = U"グリッドの行数",
			.tooltipDetail = U"テクスチャの縦方向のセル数を指定します",
			.visibilityCondition = gridRegionVisibilityCondition,
			.dragValueChangeStep = 1.0,
		};
		// アニメーション無効時かつGrid表示の時のみtextureGridIndexを表示する条件
		const auto gridRegionNoAnimationVisibilityCondition = [](const ComponentBase& component) -> bool
		{
			if (const auto* sprite = dynamic_cast<const Sprite*>(&component))
			{
				return sprite->textureRegionMode().hasAnyStateEqualTo(TextureRegionMode::Grid) &&
					   !sprite->gridAnimationType().hasAnyStateEqualTo(SpriteGridAnimationType::OneShot) &&
					   !sprite->gridAnimationType().hasAnyStateEqualTo(SpriteGridAnimationType::Loop);
			}
			return false;
		};
		
		metadata[PropertyKey{ U"Sprite", U"textureGridIndex" }] = PropertyMetadata{
			.tooltip = U"表示するセル番号",
			.tooltipDetail = U"0から始まるインデックス\n左上から横方向に数えます\n※アニメーション有効時は使用されません",
			.visibilityCondition = gridRegionNoAnimationVisibilityCondition,
			.dragValueChangeStep = 1.0,
		};
		
		// アニメーション有効時のみ表示する条件
		const auto gridAnimationEnabledVisibilityCondition = [](const ComponentBase& component) -> bool
		{
			if (const auto* sprite = dynamic_cast<const Sprite*>(&component))
			{
				return sprite->textureRegionMode().hasAnyStateEqualTo(TextureRegionMode::Grid) &&
					   (sprite->gridAnimationType().hasAnyStateEqualTo(SpriteGridAnimationType::OneShot) ||
					    sprite->gridAnimationType().hasAnyStateEqualTo(SpriteGridAnimationType::Loop));
			}
			return false;
		};
		
		metadata[PropertyKey{ U"Sprite", U"gridAnimationType" }] = PropertyMetadata{
			.tooltip = U"アニメーションの種類",
			.tooltipDetail = U"None: アニメーションなし\nOneShot: 一度だけ再生\nLoop: ループ再生",
			.visibilityCondition = gridRegionVisibilityCondition,
			.refreshInspectorOnChange = true,
		};
		metadata[PropertyKey{ U"Sprite", U"gridAnimationFPS" }] = PropertyMetadata{
			.tooltip = U"アニメーションFPS",
			.tooltipDetail = U"アニメーションの再生速度（フレーム/秒）",
			.visibilityCondition = gridAnimationEnabledVisibilityCondition,
			.dragValueChangeStep = 1.0,
		};
		metadata[PropertyKey{ U"Sprite", U"gridAnimationStartIndex" }] = PropertyMetadata{
			.tooltip = U"アニメーション開始インデックス",
			.tooltipDetail = U"アニメーションの開始フレーム番号\n0から始まるインデックス",
			.visibilityCondition = gridAnimationEnabledVisibilityCondition,
			.dragValueChangeStep = 1.0,
		};
		metadata[PropertyKey{ U"Sprite", U"gridAnimationEndIndex" }] = PropertyMetadata{
			.tooltip = U"アニメーション終了インデックス",
			.tooltipDetail = U"アニメーションの終了フレーム番号\n0から始まるインデックス",
			.visibilityCondition = gridAnimationEnabledVisibilityCondition,
			.dragValueChangeStep = 1.0,
		};
		
		// OffsetSizeモードでのアニメーション条件
		const auto offsetAnimationVisibilityCondition = [](const ComponentBase& component) -> bool
		{
			if (const auto* sprite = dynamic_cast<const Sprite*>(&component))
			{
				return sprite->textureRegionMode().hasAnyStateEqualTo(TextureRegionMode::OffsetSize);
			}
			return false;
		};
		
		const auto offsetAnimationEnabledVisibilityCondition = [](const ComponentBase& component) -> bool
		{
			if (const auto* sprite = dynamic_cast<const Sprite*>(&component))
			{
				return sprite->textureRegionMode().hasAnyStateEqualTo(TextureRegionMode::OffsetSize) &&
					   sprite->offsetAnimationType().hasAnyStateEqualTo(SpriteOffsetAnimationType::Scroll);
			}
			return false;
		};
		
		metadata[PropertyKey{ U"Sprite", U"offsetAnimationType" }] = PropertyMetadata{
			.tooltip = U"アニメーションの種類",
			.tooltipDetail = U"None: アニメーションなし\nScroll: スクロール\n　※スクロールに使用するテクスチャはループ素材であることを前提とします",
			.visibilityCondition = offsetAnimationVisibilityCondition,
			.refreshInspectorOnChange = true,
		};
		metadata[PropertyKey{ U"Sprite", U"offsetAnimationSpeed" }] = PropertyMetadata{
			.tooltip = U"スクロール速度",
			.tooltipDetail = U"1秒あたりのスクロール量（ピクセル）\nX: 水平速度, Y: 垂直速度",
			.visibilityCondition = offsetAnimationEnabledVisibilityCondition,
			.dragValueChangeStep = 1.0,
		};

		metadata[PropertyKey{ U"Sprite", U"textureFilter" }] = PropertyMetadata{
			.tooltip = U"テクスチャフィルタ",
			.tooltipDetail = U"テクスチャの補間方法\nDefault: 現在の設定を使用\nNearest: 最近傍補間（ドット絵向け）\nLinear: バイリニア補間（滑らか）\nAniso: 異方性フィルタリング（高品質）",
		};

		metadata[PropertyKey{ U"Sprite", U"textureAddressMode" }] = PropertyMetadata{
			.tooltip = U"テクスチャアドレスモード",
			.tooltipDetail = U"テクスチャ座標が範囲外の時の動作\nDefault: 現在の設定を使用\nRepeat: 繰り返し\nMirror: ミラー繰り返し\nClamp: 端の色で埋める\nBorderColor: 境界色で埋める",
		};
		
		// TextBox
		metadata[PropertyKey{ U"TextBox", U"fontAssetName" }] = PropertyMetadata{
			.tooltip = U"FontAssetのキー名 (任意)",
			.tooltipDetail = U"指定されている場合、プログラム上ではこのキー名をもとに取得したFontAssetのフォントを使用します\n※エディタ上でプレビューするには、Custom/FontAssets内にJSONファイルを作成してください",
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
			.tooltip = U"テキスト",
			.tooltipDetail = U"入力されたテキスト内容\n※フォーカス時は要素のstyleStateが\"focused\"へ、フォーカスされていない時は\"unfocused\"へ上書きされます",
			.refreshesEveryFrame = true,
		};
		metadata[PropertyKey{ U"TextBox", U"placeholderText" }] = PropertyMetadata{
			.tooltip = U"プレースホルダー表示のテキスト",
			.tooltipDetail = U"テキストが空の場合に表示されるテキスト",
		};
		metadata[PropertyKey{ U"TextBox", U"placeholderColor" }] = PropertyMetadata{
			.tooltip = U"プレースホルダー表示の文字色",
		};
		metadata[PropertyKey{ U"TextBox", U"readOnly" }] = PropertyMetadata{
			.tooltip = U"読み取り専用",
			.tooltipDetail = U"有効にすると編集不可になりますが、テキストの選択やコピーは可能です",
		};
		metadata[PropertyKey{ U"TextBox", U"tag" }] = PropertyMetadata{
			.tooltip = U"タグ",
			.tooltipDetail = U"TextBoxを識別するためのタグ文字列です\nCanvas::getTextValueByTag()やsetTextValueByTag()で\nそのタグを持つTextBoxのテキストを取得・設定できます",
		};
		
		// TextArea
		metadata[PropertyKey{ U"TextArea", U"fontAssetName" }] = PropertyMetadata{
			.tooltip = U"FontAssetのキー名 (任意)",
			.tooltipDetail = U"指定されている場合、プログラム上ではこのキー名をもとに取得したFontAssetのフォントを使用します\n※エディタ上でプレビューするには、Custom/FontAssets内にJSONファイルを作成してください",
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
			.tooltip = U"テキスト",
			.tooltipDetail = U"入力されたテキスト内容\n※フォーカス時は要素のstyleStateが\"focused\"へ、フォーカスされていない時は\"unfocused\"へ上書きされます",
			.numTextAreaLines = 3,
			.refreshesEveryFrame = true,
		};
		metadata[PropertyKey{ U"TextArea", U"placeholderText" }] = PropertyMetadata{
			.tooltip = U"プレースホルダー表示のテキスト",
			.tooltipDetail = U"テキストが空の場合に表示されるテキスト",
			.numTextAreaLines = 3,
		};
		metadata[PropertyKey{ U"TextArea", U"placeholderColor" }] = PropertyMetadata{
			.tooltip = U"プレースホルダー表示の文字色",
		};
		metadata[PropertyKey{ U"TextArea", U"readOnly" }] = PropertyMetadata{
			.tooltip = U"読み取り専用",
			.tooltipDetail = U"有効にすると編集不可になりますが、テキストの選択やコピーは可能です",
		};
		metadata[PropertyKey{ U"TextArea", U"tag" }] = PropertyMetadata{
			.tooltip = U"タグ",
			.tooltipDetail = U"TextAreaを識別するためのタグ文字列です\nCanvas::getTextValueByTag()やsetTextValueByTag()で\nそのタグを持つTextAreaのテキストを取得・設定できます",
		};

		// Toggleのプロパティ
		metadata[PropertyKey{ U"Toggle", U"value" }] = PropertyMetadata{
			.tooltip = U"トグルの値",
			.tooltipDetail = U"現在のオン/オフ状態を表します\n※現在のvalueの値に応じて要素のstyleStateが\"on\"/\"off\"へ上書きされます",
			.refreshesEveryFrame = true,
		};
		metadata[PropertyKey{ U"Toggle", U"tag" }] = PropertyMetadata{
			.tooltip = U"タグ",
			.tooltipDetail = U"Toggleを識別するためのタグ文字列です\nCanvas::getToggleValueByTag()やsetToggleValueByTag()で\nそのタグを持つToggleの値を取得・設定できます",
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
		
		// UISound
		metadata[PropertyKey{ U"UISound", U"audioFilePath" }] = PropertyMetadata{
			.tooltip = U"音声ファイルのパス",
			.tooltipDetail = U"※audioAssetName使用時は、Editor上でのプレビュー用としてのみ使用されます",
		};
		metadata[PropertyKey{ U"UISound", U"audioAssetName" }] = PropertyMetadata{
			.tooltip = U"AudioAssetのキー名 (任意)",
			.tooltipDetail = U"指定されている場合、プログラム上ではこのキー名をもとに取得したAudioAssetの音声を使用します\n※プレビューには反映されません\n※これを使用しなくてもライブラリ側で内部的にファイルパスをもとにしたHashTableでアセットが管理されるため、\n　パフォーマンス上の利点は特にありません。AudioAssetのキー名を手動で管理したい場合のみ使用してください",
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
			.tooltip = U"Tweenアニメーションの再生状態",
		};

		// Translate関連
		metadata[PropertyKey{ U"Tween", U"translateEnabled" }] = PropertyMetadata{
			.tooltip = U"Translateアニメーションを有効にする",
			.tooltipDetail = U"有効にすると、Transformのtranslateプロパティをアニメーションします",
			.refreshInspectorOnChange = true,
		};

		metadata[PropertyKey{ U"Tween", U"translateFrom" }] = PropertyMetadata{
			.tooltip = U"Translate開始値",
			.tooltipDetail = U"アニメーション開始時のtranslate値",
			.visibilityCondition = [](const ComponentBase& component) -> bool {
				if (const auto* tween = dynamic_cast<const Tween*>(&component))
				{
					return tween->translateEnabled();
				}
				return false;
			},
		};

		metadata[PropertyKey{ U"Tween", U"translateTo" }] = PropertyMetadata{
			.tooltip = U"Translate終了値",
			.tooltipDetail = U"アニメーション終了時のtranslate値",
			.visibilityCondition = [](const ComponentBase& component) -> bool {
				if (const auto* tween = dynamic_cast<const Tween*>(&component))
				{
					return tween->translateEnabled();
				}
				return false;
			},
		};

		// Scale関連
		metadata[PropertyKey{ U"Tween", U"scaleEnabled" }] = PropertyMetadata{
			.tooltip = U"Scaleアニメーションを有効にする",
			.tooltipDetail = U"有効にすると、Transformのscaleプロパティをアニメーションします",
			.refreshInspectorOnChange = true,
		};

		metadata[PropertyKey{ U"Tween", U"scaleFrom" }] = PropertyMetadata{
			.tooltip = U"Scale開始値",
			.tooltipDetail = U"アニメーション開始時のscale値",
			.visibilityCondition = [](const ComponentBase& component) -> bool {
				if (const auto* tween = dynamic_cast<const Tween*>(&component))
				{
					return tween->scaleEnabled();
				}
				return false;
			},
		};

		metadata[PropertyKey{ U"Tween", U"scaleTo" }] = PropertyMetadata{
			.tooltip = U"Scale終了値",
			.tooltipDetail = U"アニメーション終了時のscale値",
			.visibilityCondition = [](const ComponentBase& component) -> bool {
				if (const auto* tween = dynamic_cast<const Tween*>(&component))
				{
					return tween->scaleEnabled();
				}
				return false;
			},
		};
		
		// Rotation関連
		metadata[PropertyKey{ U"Tween", U"rotationEnabled" }] = PropertyMetadata{
			.tooltip = U"Rotationアニメーションを有効にする",
			.tooltipDetail = U"有効にすると、Transformのrotationプロパティをアニメーションします",
			.refreshInspectorOnChange = true,
		};

		metadata[PropertyKey{ U"Tween", U"rotationFrom" }] = PropertyMetadata{
			.tooltip = U"Rotation開始値",
			.tooltipDetail = U"アニメーション開始時のrotation値（度単位）",
			.visibilityCondition = [](const ComponentBase& component) -> bool {
				if (const auto* tween = dynamic_cast<const Tween*>(&component))
				{
					return tween->rotationEnabled();
				}
				return false;
			},
			.dragValueChangeStep = 1.0,
		};

		metadata[PropertyKey{ U"Tween", U"rotationTo" }] = PropertyMetadata{
			.tooltip = U"Rotation終了値",
			.tooltipDetail = U"アニメーション終了時のrotation値（度単位）",
			.visibilityCondition = [](const ComponentBase& component) -> bool {
				if (const auto* tween = dynamic_cast<const Tween*>(&component))
				{
					return tween->rotationEnabled();
				}
				return false;
			},
			.dragValueChangeStep = 1.0,
		};
		
		// Color関連
		metadata[PropertyKey{ U"Tween", U"colorEnabled" }] = PropertyMetadata{
			.tooltip = U"Colorアニメーションを有効にする",
			.tooltipDetail = U"有効にすると、Transformのcolorプロパティをアニメーションします",
			.refreshInspectorOnChange = true,
		};

		metadata[PropertyKey{ U"Tween", U"colorFrom" }] = PropertyMetadata{
			.tooltip = U"Color開始値",
			.tooltipDetail = U"アニメーション開始時のcolor値",
			.visibilityCondition = [](const ComponentBase& component) -> bool {
				if (const auto* tween = dynamic_cast<const Tween*>(&component))
				{
					return tween->colorEnabled();
				}
				return false;
			},
		};

		metadata[PropertyKey{ U"Tween", U"colorTo" }] = PropertyMetadata{
			.tooltip = U"Color終了値",
			.tooltipDetail = U"アニメーション終了時のcolor値",
			.visibilityCondition = [](const ComponentBase& component) -> bool {
				if (const auto* tween = dynamic_cast<const Tween*>(&component))
				{
					return tween->colorEnabled();
				}
				return false;
			},
		};
		
		metadata[PropertyKey{ U"Tween", U"easing" }] = PropertyMetadata{
			.tooltip = U"イージング関数",
			.tooltipDetail = U"時間に対する値の変化のさせ方を指定します",
		};
		
		metadata[PropertyKey{ U"Tween", U"duration" }] = PropertyMetadata{
			.tooltip = U"アニメーション時間(秒)",
			.dragValueChangeStep = 0.1,
		};
		metadata[PropertyKey{ U"Tween", U"delay" }] = PropertyMetadata{
			.tooltip = U"開始までの遅延時間(秒)",
			.dragValueChangeStep = 0.1,
		};
		metadata[PropertyKey{ U"Tween", U"loopType" }] = PropertyMetadata{
			.tooltip = U"ループの種類",
			.tooltipDetail = U"None: ループなし\nLoop: 通常ループ\nPingPong: 往復ループ\n手動モードでも有効です",
			.refreshInspectorOnChange = true,
		};

		const auto tweenLoopDurationVisibilityCondition = [](const ComponentBase& component) -> bool
		{
			if (const auto* tween = dynamic_cast<const Tween*>(&component))
			{
				return tween->loopType() != TweenLoopType::None;
			}
			return false;
		};

		metadata[PropertyKey{ U"Tween", U"loopDuration" }] = PropertyMetadata{
			.tooltip = U"ループ周期(秒)",
			.tooltipDetail = U"ループの周期を指定します\n0の場合: durationのみでループ(delayは初回のみ)\n0より大きい値: delayを含めて指定した時間でループ\n複数のTweenを組み合わせた一連のアニメーションをループさせる際に活用できます",
			.visibilityCondition = tweenLoopDurationVisibilityCondition,
			.dragValueChangeStep = 0.1,
		};
		const auto tweenRestartsVisibilityCondition = [](const ComponentBase& component) -> bool
		{
			if (const auto* tween = dynamic_cast<const Tween*>(&component))
			{
				return !HasAnyTrueState(tween->manualMode());
			}
			return true;
		};
		metadata[PropertyKey{ U"Tween", U"restartOnActive" }] = PropertyMetadata{
			.tooltip = U"アクティブ時に最初から再生",
			.tooltipDetail = U"activeプロパティがfalse→trueになった時、またはノード自体のアクティブ状態がfalse→trueになった時に、アニメーションを最初から再生し直すかどうか",
			.visibilityCondition = tweenRestartsVisibilityCondition,
		};
		
		metadata[PropertyKey{ U"Tween", U"manualMode" }] = PropertyMetadata{
			.tooltip = U"手動制御モード",
			.tooltipDetail = U"有効にすると、時間経過ではなくmanualTimeプロパティの値(0.0〜1.0)でアニメーションの進行を制御します",
			.refreshInspectorOnChange = true,
		};
		
		metadata[PropertyKey{ U"Tween", U"applyDuringDelay" }] = PropertyMetadata{
			.tooltip = U"遅延時間中に0%の値を適用",
			.tooltipDetail = U"有効にすると、delay時間中も開始値(0%の値)を適用します。\n無効の場合、delay時間中は何もしません",
		};
		
		const auto tweenManualTimeVisibilityCondition = [](const ComponentBase& component) -> bool
		{
			if (const auto* tween = dynamic_cast<const Tween*>(&component))
			{
				return HasAnyTrueState(tween->manualMode());
			}
			return false;
		};
		
		metadata[PropertyKey{ U"Tween", U"manualTime" }] = PropertyMetadata{
			.tooltip = U"手動制御の再生時間(秒)",
			.tooltipDetail = U"アニメーションの再生時間を手動で指定します",
			.visibilityCondition = tweenManualTimeVisibilityCondition,
			.dragValueChangeStep = 0.1,
		};

		metadata[PropertyKey{ U"Tween", U"tag" }] = PropertyMetadata{
			.tooltip = U"タグ",
			.tooltipDetail = U"Tweenを一括制御するためのタグ文字列です\nCanvas::setTweenActiveByTag()で同じタグを持つ\nTweenを一括でアクティブ/非アクティブにできます",
		};

		// Canvasのプロパティ
		metadata[PropertyKey{ U"Canvas", U"size" }] = PropertyMetadata{
			.tooltip = U"Canvasのサイズ",
			.tooltipDetail = U"Canvasの幅と高さを設定します\nすべての子要素のレイアウト計算の基準となります",
			.dragValueChangeStep = 1.0,
		};
		metadata[PropertyKey{ U"Canvas", U"autoScaleMode" }] = PropertyMetadata{
			.tooltip = U"自動スケールモード",
			.tooltipDetail = U"シーンサイズに応じた自動スケール調整を設定します\n\nNone: スケールしない\nShrinkToFit: Canvas全体がシーン内に収まるよう縮小拡大\nExpandToFill: シーン全体をCanvasで埋めるよう縮小拡大\nFitHeight: シーンの高さに合わせる\nFitWidth: シーンの幅に合わせる\n\n※エディタ上のプレビューには反映されません",
		};
		metadata[PropertyKey{ U"Canvas", U"autoResizeMode" }] = PropertyMetadata{
			.tooltip = U"自動リサイズモード",
			.tooltipDetail = U"シーンサイズに応じた自動リサイズを設定します\n\nNone: リサイズしない\nMatchSceneSize: シーンサイズに合わせる\n\n※AutoScaleModeとは異なり、Canvasのサイズ自体が変更されます\n※エディタ上のプレビューには反映されません",
		};
		
		return metadata;
	}
}
