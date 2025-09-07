#pragma once
#include <NocoUI.hpp>
#include "PlaceholderComponent.hpp"
#include "Defaults.hpp"
#include "Tooltip.hpp"
#include "Toolbar.hpp"
#include "TabStop.hpp"
#include "ContextMenu.hpp"
#include "EditorDialog.hpp"
#include "PropertyMetaData.hpp"
#include "Vec4PropertyTextBox.hpp"
#include "PropertyLabelDragger.hpp"
#include "PropertyTextBox.hpp"
#include "AddParamDialog.hpp"
#include "ParamRefDialog.hpp"
#include "ParamReferencesDialog.hpp"
#include "ComponentSchema.hpp"
#include "ComponentSchemaLoader.hpp"

namespace noco::editor
{
	class Inspector
	{
	private:
		std::shared_ptr<Canvas> m_canvas;
		std::shared_ptr<Canvas> m_editorCanvas;
		std::shared_ptr<Canvas> m_editorOverlayCanvas;
		std::shared_ptr<Node> m_inspectorFrameNode;
		std::shared_ptr<Node> m_inspectorInnerFrameNode;
		std::shared_ptr<Node> m_inspectorRootNode;
		std::shared_ptr<ContextMenu> m_contextMenu;
		std::shared_ptr<DialogOpener> m_dialogOpener;
		HashTable<PropertyKey, PropertyMetadata> m_propertyMetadata;
		std::weak_ptr<Node> m_targetNode;
		std::function<void()> m_onChangeNodeName;
		std::function<void()> m_onChangeNodeActive;
		std::shared_ptr<ComponentFactory> m_componentFactory;
		
		void renameParam(const String& oldName, const String& newName)
		{
			if (!m_canvas)
			{
				return;
			}
			
			const auto paramValue = m_canvas->param(oldName);
			if (!paramValue)
			{
				return;
			}
			
			m_canvas->setParamValue(newName, *paramValue);
			
			m_canvas->replaceParamRefs(oldName, newName);
			
			m_canvas->removeParam(oldName);
		}

		IsFoldedYN m_isFoldedRegion = IsFoldedYN::No;
		IsFoldedYN m_isFoldedNodeSetting = IsFoldedYN::Yes;
		IsFoldedYN m_isFoldedLayout = IsFoldedYN::Yes;
		IsFoldedYN m_isFoldedTransform = IsFoldedYN::Yes;
		IsFoldedYN m_isFoldedCanvasSetting = IsFoldedYN::No;  // Canvas Settingセクションの折り畳み状態
		IsFoldedYN m_isFoldedParams = IsFoldedYN::No;  // Paramsセクションの折り畳み状態
		Array<std::weak_ptr<ComponentBase>> m_foldedComponents;

		std::shared_ptr<Defaults> m_defaults;
		
		// コンポーネントのコピー用クリップボード
		Optional<JSON> m_copiedComponentJSON;
		Optional<String> m_copiedComponentType;

		template <class TComponent, class... Args>
		void onClickAddComponent(Args&&... args)
		{
			const auto node = m_targetNode.lock();
			if (!node)
			{
				return;
			}
			node->emplaceComponent<TComponent>(std::forward<Args>(args)...);
			refreshInspector();
		}
		
		void onClickAddCustomComponent(const String& typeName)
		{
			const auto node = m_targetNode.lock();
			if (!node)
			{
				return;
			}
			
			// スキーマに書かれた初期値を含むJSONを作成
			JSON componentJson;
			componentJson[U"type"] = typeName;
			if (const ComponentSchema* schema = ComponentSchemaLoader::GetSchema(typeName))
			{
				for (const auto& prop : schema->properties)
				{
					// variantからJSON値を設定
					if (!std::holds_alternative<std::monostate>(prop.defaultValue))
					{
						std::visit([&componentJson, &prop](auto&& value)
						{
							using T = std::decay_t<decltype(value)>;
							if constexpr (std::is_same_v<T, std::monostate>)
							{
								// 値なし
							}
							else if constexpr (std::is_same_v<T, bool>)
							{
								componentJson[prop.name] = value;
							}
							else if constexpr (std::is_arithmetic_v<T> && !std::is_same_v<T, bool>)
							{
								componentJson[prop.name] = static_cast<double>(value);
							}
							else if constexpr (std::is_same_v<T, String>)
							{
								componentJson[prop.name] = value;
							}
							else if constexpr (std::is_same_v<T, ColorF>)
							{
								componentJson[prop.name] = ValueToString(value);
							}
							else if constexpr (std::is_same_v<T, Vec2>)
							{
								componentJson[prop.name] = ValueToString(value);
							}
							else if constexpr (std::is_same_v<T, LRTB>)
							{
								componentJson[prop.name] = ValueToString(value);
							}
						}, prop.defaultValue);
					}
				}
			}
			
			// PlaceholderComponentを作成して追加
			auto placeholder = PlaceholderComponent::Create(typeName, componentJson);
			node->addComponent(placeholder);
			refreshInspector();
		}
		
		void onClickCopyComponent(const std::shared_ptr<SerializableComponentBase>& component)
		{
			if (!component)
			{
				return;
			}
			m_copiedComponentJSON = component->toJSON();
			m_copiedComponentType = component->type();
			
			// Inspectorを更新してコンテキストメニューに貼り付けオプションを反映
			refreshInspector();
		}
		
		void onClickPasteComponentTo(const std::shared_ptr<SerializableComponentBase>& component)
		{
			if (!component || !m_copiedComponentJSON.has_value() || !m_copiedComponentType.has_value())
			{
				return;
			}
			// 同じタイプのコンポーネントにのみ貼り付け可能
			if (component->type() != *m_copiedComponentType)
			{
				return;
			}
			component->tryReadFromJSON(*m_copiedComponentJSON);
			refreshInspector();
		}
		
		void onClickPasteComponentAsNew()
		{
			const auto node = m_targetNode.lock();
			if (!node || !m_copiedComponentJSON || !m_copiedComponentType)
			{
				return;
			}
			
			// クリップボードのJSONからコンポーネントを作成
			auto componentJSON = *m_copiedComponentJSON;
			componentJSON[U"type"] = *m_copiedComponentType;
			if (m_componentFactory)
			{
				if (const auto component = m_componentFactory->createComponentFromJSON(componentJSON))
				{
					node->addComponent(component);
					refreshInspector();
				}
			}
		}
		
		void doSnapNodeSizeToTexture(const std::shared_ptr<Sprite>& sprite, const std::shared_ptr<Node>& node)
		{
			const String texturePath = sprite->textureFilePath().defaultValue();
			if (texturePath.isEmpty())
			{
				return;
			}
			
			Texture texture = noco::Asset::GetOrLoadTexture(texturePath);
			if (!texture)
			{
				return;
			}
			
			const Vec2 textureSize{ texture.size() };
			
			if (const auto* pInlineRegion = node->inlineRegion())
			{
				InlineRegion newRegion = *pInlineRegion;
				newRegion.sizeDelta = textureSize;
				newRegion.sizeRatio = Vec2::Zero();
				newRegion.flexibleWeight = 0.0;
				node->setRegion(newRegion);
			}
			else if (const auto* pAnchorRegion = node->anchorRegion())
			{
				AnchorRegion newRegion = *pAnchorRegion;
				newRegion.sizeDelta = textureSize;
				newRegion.anchorMin = noco::Anchor::MiddleCenter;
				newRegion.anchorMax = noco::Anchor::MiddleCenter;
				node->setRegion(newRegion);
			}
			else
			{
				throw Error{ U"Unknown region type" };
			}
		}

	public:
		explicit Inspector(const std::shared_ptr<Canvas>& canvas, const std::shared_ptr<Canvas>& editorCanvas, const std::shared_ptr<Canvas>& editorOverlayCanvas, const std::shared_ptr<ContextMenu>& contextMenu, const std::shared_ptr<Defaults>& defaults, const std::shared_ptr<DialogOpener>& dialogOpener, const std::shared_ptr<ComponentFactory>& componentFactory, std::function<void()> onChangeNodeName, std::function<void()> onChangeNodeActive)
			: m_canvas(canvas)
			, m_editorCanvas(editorCanvas)
			, m_editorOverlayCanvas(editorOverlayCanvas)
			, m_inspectorFrameNode(editorCanvas->emplaceChild(
				U"InspectorFrame",
				AnchorRegion
				{
					.anchorMin = Anchor::TopRight,
					.anchorMax = Anchor::BottomRight,
					.posDelta = Vec2{ 0, MenuBarHeight + Toolbar::ToolbarHeight },
					.sizeDelta = Vec2{ 400, -(MenuBarHeight + Toolbar::ToolbarHeight) },
					.sizeDeltaPivot = Anchor::TopRight,
				}))
			, m_inspectorInnerFrameNode(m_inspectorFrameNode->emplaceChild(
				U"InspectorInnerFrame",
				AnchorRegion
				{
					.anchorMin = Anchor::TopLeft,
					.anchorMax = Anchor::BottomRight,
					.posDelta = Vec2{ 0, 0 },
					.sizeDelta = Vec2{ -2, -2 },
					.sizeDeltaPivot = Anchor::MiddleCenter,
				},
				IsHitTargetYN::Yes,
				InheritChildrenStateFlags::Pressed))
			, m_inspectorRootNode(m_inspectorInnerFrameNode->emplaceChild(
				U"Inspector",
				AnchorRegion
				{
					.anchorMin = Anchor::TopLeft,
					.anchorMax = Anchor::BottomRight,
					.posDelta = Vec2{ 0, 0 },
					.sizeDelta = Vec2{ -10, -10 },
					.sizeDeltaPivot = Anchor::MiddleCenter,
				},
				IsHitTargetYN::Yes))
			, m_contextMenu(contextMenu)
			, m_defaults(defaults)
			, m_dialogOpener(dialogOpener)
			, m_onChangeNodeName(std::move(onChangeNodeName))
			, m_onChangeNodeActive(std::move(onChangeNodeActive))
			, m_componentFactory(componentFactory)
			, m_propertyMetadata(InitPropertyMetadata())
		{
			m_inspectorFrameNode->emplaceComponent<RectRenderer>(ColorF{ 0.5, 0.4 }, Palette::Black, 0.0, 10.0);
			m_inspectorInnerFrameNode->emplaceComponent<RectRenderer>(ColorF{ 0.1, 0.8 }, Palette::Black, 0.0, 10.0);
			m_inspectorRootNode->setChildrenLayout(VerticalLayout{ .padding = LRTB{ 0, 0, 4, 4 } });
			m_inspectorRootNode->setVerticalScrollable(true);
		}

		// thisをキャプチャしているのでコピー・ムーブ不可
		Inspector(const Inspector&) = delete;
		Inspector(Inspector&&) = delete;
		Inspector& operator=(const Inspector&) = delete;
		Inspector&& operator=(Inspector&&) = delete;

		void refreshInspector(PreserveScrollYN preserveScroll = PreserveScrollYN::Yes)
		{
			const double scrollY = m_inspectorRootNode->scrollOffset().y;
			
			// 現在フォーカスされているノードの情報を保存
			std::shared_ptr<Node> focusedNode = CurrentFrame::GetFocusedNode();
			String focusedNodeName;
			bool isInInspector = false;
			
			if (focusedNode)
			{
				// フォーカスされているノードがInspector内にあるかチェック
				auto currentNode = focusedNode;
				while (currentNode)
				{
					if (currentNode == m_inspectorRootNode)
					{
						isInInspector = true;
						focusedNodeName = focusedNode->name();
						break;
					}
					currentNode = currentNode->parentNode();
				}
			}
			
			setTargetNode(m_targetNode.lock());
			if (preserveScroll)
			{
				m_inspectorRootNode->resetScrollOffset(RecursiveYN::No);
				m_inspectorRootNode->scroll(Vec2{ 0, scrollY });
			}
			m_editorCanvas->refreshLayoutImmediately();
			
			// TabStopを持つすべてのノードを収集してリンクを設定
			setupTabStopLinks();
			
			// フォーカスを復元
			if (isInInspector && !focusedNodeName.empty())
			{
				// 同じ名前のノードを探してフォーカスを復元
				auto newFocusNode = m_inspectorRootNode->getChildByNameOrNull(focusedNodeName, RecursiveYN::Yes);
				if (newFocusNode && newFocusNode->getComponentOrNull<TabStop>())
				{
					CurrentFrame::SetFocusedNode(newFocusNode);
				}
			}
		}
		
		void setupTabStopLinks()
		{
			// TabStopを持つすべてのノードを収集
			Array<std::shared_ptr<Node>> tabStopNodes;
			populateTabStopNodes(m_inspectorRootNode, tabStopNodes);
			
			if (tabStopNodes.empty())
			{
				return;
			}
			
			// 各ノードのTabStopに次と前のノードを設定
			for (size_t i = 0; i < tabStopNodes.size(); ++i)
			{
				const auto tabStop = tabStopNodes[i]->getComponentOrNull<TabStop>();
				if (!tabStop)
				{
					continue;
				}
				
				// 次のノードを設定（最後の要素は最初に戻る）
				const size_t nextIndex = (i + 1) % tabStopNodes.size();
				tabStop->setNextNode(tabStopNodes[nextIndex]);
				
				// 前のノードを設定（最初の要素は最後に戻る）
				const size_t prevIndex = (i == 0) ? tabStopNodes.size() - 1 : i - 1;
				tabStop->setPreviousNode(tabStopNodes[prevIndex]);
			}
		}
		
		void populateTabStopNodes(const std::shared_ptr<Node>& node, Array<std::shared_ptr<Node>>& tabStopNodes)
		{
			if (!node)
			{
				return;
			}
			
			// このノードがTabStopを持っているかチェック
			if (node->getComponentOrNull<TabStop>())
			{
				tabStopNodes.push_back(node);
			}
			
			// 子ノードを再帰的に探索
			for (const auto& child : node->children())
			{
				populateTabStopNodes(child, tabStopNodes);
			}
		}

		void setTargetNode(const std::shared_ptr<Node>& targetNode)
		{
			if (!targetNode || targetNode.get() != m_targetNode.lock().get())
			{
				// 選択ノードが変更された場合、以前のノード用の折り畳み状況をクリア
				m_foldedComponents.clear();
			}

			m_targetNode = targetNode;

			m_inspectorRootNode->removeChildrenAll();

			if (targetNode)
			{
				const auto nodeNameNode = createNodeNameNode(targetNode);
				m_inspectorRootNode->addChild(nodeNameNode);

				const auto regionNode = createRegionNode(targetNode);
				m_inspectorRootNode->addChild(regionNode);

				const auto nodeSettingNode = createNodeSettingNode(targetNode);
				m_inspectorRootNode->addChild(nodeSettingNode);

				const auto layoutNode = createChildrenLayoutNode(targetNode);
				m_inspectorRootNode->addChild(layoutNode);

				const auto transformNode = createTransformNode(&targetNode->transform());
				m_inspectorRootNode->addChild(transformNode);

				for (const std::shared_ptr<ComponentBase>& component : targetNode->components())
				{
					const IsFoldedYN isFolded{ m_foldedComponents.contains_if([&component](const auto& c) { return c.lock().get() == component.get(); }) };

					if (const auto serializableComponent = std::dynamic_pointer_cast<SerializableComponentBase>(component))
					{
						const auto componentNode = createComponentNode(targetNode, serializableComponent, isFolded,
							[this, componentWeak = std::weak_ptr{ component }](IsFoldedYN isFolded)
							{
								if (isFolded)
								{
									m_foldedComponents.push_back(componentWeak);
								}
								else
								{
									m_foldedComponents.remove_if([&componentWeak](const auto& c) { return c.lock().get() == componentWeak.lock().get(); });
								}
							});
						m_inspectorRootNode->addChild(componentNode);
					}
				}

				// コンポーネント追加メニューを設定
				// 既存のContextMenuOpenerを削除
				m_inspectorInnerFrameNode->removeComponentsIf([](const std::shared_ptr<ComponentBase>& component)
				{
					return std::dynamic_pointer_cast<ContextMenuOpener>(component) != nullptr;
				});
				
				Array<MenuElement> menuElements = {
					MenuItem{ U"Sprite を追加", U"", KeyS, [this] { onClickAddComponent<Sprite>(); } },
					MenuItem{ U"RectRenderer を追加", U"", KeyR, [this] { onClickAddComponent<RectRenderer>(); } },
					MenuItem{ U"ShapeRenderer を追加", U"", KeyH, [this] { onClickAddComponent<ShapeRenderer>(); } },
					MenuItem{ U"TextBox を追加", U"", KeyT, [this] { onClickAddComponent<TextBox>(); } },
					MenuItem{ U"TextArea を追加", U"", KeyA, [this] { onClickAddComponent<TextArea>(); } },
					MenuItem{ U"Label を追加", U"", KeyL, [this] { onClickAddComponent<Label>(); } },
					MenuItem{ U"TextureFontLabel を追加", U"", KeyF, [this] { onClickAddComponent<TextureFontLabel>(); } },
					MenuItem{ U"EventTrigger を追加", U"", KeyE, [this] { onClickAddComponent<EventTrigger>(); } },
					MenuItem{ U"CursorChanger を追加", U"", KeyC, [this] { onClickAddComponent<CursorChanger>(); } },
					MenuItem{ U"UISound を追加", U"", KeyU, [this] { onClickAddComponent<UISound>(); } },
					MenuItem{ U"Tween を追加", U"", KeyW, [this] { onClickAddComponent<Tween>(); } },
				};
				
				// カスタムコンポーネントをメニューに追加
				const auto& schemas = ComponentSchemaLoader::GetAllSchemas();
				if (!schemas.empty())
				{
					menuElements.push_back(MenuSeparator{});
					for (const auto& [typeName, schema] : schemas)
					{
						// 構造化束縛の変数を明示的にコピー
						const String typeNameCopy = typeName;
						menuElements.push_back(MenuItem{ 
							U"{} を追加"_fmt(typeName), 
							U"", 
							Input{}, 
							[this, typeNameCopy] { onClickAddCustomComponent(typeNameCopy); } 
						});
					}
				}
				
				// コピーされたコンポーネントがある場合は貼り付けメニューを追加
				if (m_copiedComponentType)
				{
					menuElements.push_back(MenuSeparator{});
					menuElements.push_back(MenuItem{ U"{} を貼り付け"_fmt(*m_copiedComponentType), U"", KeyV, [this] { onClickPasteComponentAsNew(); } });
				}
				
				m_inspectorInnerFrameNode->emplaceComponent<ContextMenuOpener>(m_contextMenu, menuElements);

				m_inspectorRootNode->addChild(CreateButtonNode(
					U"＋ コンポーネントを追加(A)",
					InlineRegion
					{
						.sizeRatio = Vec2{ 1, 0 },
						.sizeDelta = Vec2{ 0, 24 },
						.margin = LRTB{ 0, 0, 24, 24 },
						.maxWidth = 240,
					},
					[this] (const std::shared_ptr<Node>& node)
					{
						m_inspectorInnerFrameNode->getComponent<ContextMenuOpener>()->openManually(node->regionRect().center());
					}))->addClickHotKey(KeyA, CtrlYN::No, AltYN::Yes, ShiftYN::No, EnabledWhileTextEditingYN::Yes);
			}
			else
			{
				// Canvas設定を表示
				const auto canvasNameNode = Node::Create(
					U"CanvasName",
					InlineRegion
					{
						.sizeRatio = Vec2{ 1, 0 },
						.sizeDelta = Vec2{ 0, 32 },
						.margin = LRTB{ 0, 0, 4, 4 },
					});
				canvasNameNode->emplaceComponent<Label>(
					U"Canvas",
					U"",
					18,
					Palette::White,
					HorizontalAlign::Center,
					VerticalAlign::Middle);
				canvasNameNode->emplaceComponent<RectRenderer>(ColorF{ 0.2, 0.2 }, ColorF{ 0.4, 0.4 }, 0.0, 4.0);
				m_inspectorRootNode->addChild(canvasNameNode);

				// Canvas Settingセクションを追加
				const auto canvasSettingNode = createCanvasSettingNode();
				m_inspectorRootNode->addChild(canvasSettingNode);

				// Paramsセクションを追加
				const auto paramsNode = createParamsNode();
				m_inspectorRootNode->addChild(paramsNode);

				// Children Layoutセクションを追加
				const auto canvasLayoutNode = createCanvasChildrenLayoutNode();
				m_inspectorRootNode->addChild(canvasLayoutNode);
			}
			
			// TabStopを持つすべてのノードを収集してリンクを設定
			setupTabStopLinks();
		}

		[[nodiscard]]
		static std::shared_ptr<Node> CreateHeadingNode(StringView name, const ColorF& color, IsFoldedYN isFolded, std::function<void(IsFoldedYN)> onToggleFold = nullptr)
		{
			auto headingNode = Node::Create(U"Heading", InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 24 },
					.margin = LRTB{ 0, 0, 0, 0 },
				});
			headingNode->emplaceComponent<RectRenderer>(
				PropertyValue<ColorF>(ColorF{ color, 0.8 }).withHovered(ColorF{ color + ColorF{ 0.05 }, 0.8 }).withPressed(ColorF{ color - ColorF{ 0.05 }, 0.8 }),
				Palette::Black,
				0.0,
				3.0);
			const auto arrowLabel = headingNode->emplaceComponent<Label>(
				isFolded ? U"▶" : U"▼",
				U"",
				14,
				ColorF{ 1.0, 0.6 },
				HorizontalAlign::Left,
				VerticalAlign::Middle,
				LRTB{ 5, 5, 0, 0 },
				HorizontalOverflow::Wrap,
				VerticalOverflow::Clip);
			headingNode->emplaceComponent<Label>(
				name,
				U"",
				14,
				Palette::White,
				HorizontalAlign::Left,
				VerticalAlign::Middle,
				LRTB{ 25, 5, 0, 0 },
				HorizontalOverflow::Wrap,
				VerticalOverflow::Clip);
			headingNode->addOnClick([arrowLabel, onToggleFold = std::move(onToggleFold)](const std::shared_ptr<Node>& node)
				{
					if (const auto parent = node->parentNode())
					{
						bool inactiveNodeExists = false;

						// 現在の折り畳み状態
						bool currentlyFolded = false;
						for (const auto& child : parent->children())
						{
							if (child != node)
							{
								currentlyFolded = !child->activeSelf();
								break;
							}
						}
						
						// 折り畳み状態を反転
						const bool willBeFolded = !currentlyFolded;
						inactiveNodeExists = willBeFolded;
						
						// 各子ノードの表示状態を更新
						for (const auto& child : parent->children())
						{
							if (child != node)
							{
								// 保存された可視情報を取得(デフォルトは表示可能)
								const auto visibilityData = child->getStoredDataOr<PropertyVisibilityData>({ .isVisibleByCondition = true });
								
								// 折り畳まれていない場合は、可視条件に従って表示
								// 折り畳まれている場合は、可視条件に関わらず非表示
								if (willBeFolded || !visibilityData.isVisibleByCondition)
								{
									child->setActive(false);
								}
								else
								{
									child->setActive(true);
								}
							}
						}

						// 矢印を回転
						arrowLabel->setText(inactiveNodeExists ? U"▶" : U"▼");

						// 折り畳み時はpaddingを付けない
						LayoutVariant layout = parent->childrenLayout();
						if (auto pVerticalLayout = std::get_if<VerticalLayout>(&layout))
						{
							pVerticalLayout->padding = inactiveNodeExists ? LRTB::Zero() : LRTB{ 0, 0, 0, 8 };
						}
						parent->setChildrenLayout(layout);

						// 高さをフィットさせる
						parent->setInlineRegionToFitToChildren(FitTarget::HeightOnly);

						// トグル時処理があれば実行
						if (onToggleFold)
						{
							onToggleFold(IsFoldedYN{ inactiveNodeExists });
						}
					}
				});

			return headingNode;
		}


		[[nodiscard]]
		static std::shared_ptr<Node> CreateNodeNameTextboxNode(StringView name, StringView value, std::function<void(StringView)> fnSetValue)
		{
			const auto propertyNode = Node::Create(
				name,
				InlineRegion
				{
					.sizeDelta = Vec2{ 0, 32 },
					.flexibleWeight = 1,
					.margin = LRTB{ 10, 8, 0, 0 },
				},
				IsHitTargetYN::Yes);
			propertyNode->setChildrenLayout(HorizontalLayout{ .verticalAlign = VerticalAlign::Middle });
			const auto textBoxNode = propertyNode->emplaceChild(
				U"TextBox",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 26 },
				});
			textBoxNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"focused", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textBox = textBoxNode->emplaceComponent<TextBox>(U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, HorizontalAlign::Left, VerticalAlign::Middle, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textBox->setText(value, IgnoreIsChangedYN::Yes);
			textBoxNode->addComponent(std::make_shared<PropertyTextBox>(textBox, std::move(fnSetValue)));
			textBoxNode->emplaceComponent<TabStop>();
			textBoxNode->addClickHotKey(KeyF2);
			return propertyNode;
		}

		[[nodiscard]]
		std::shared_ptr<Node> createPropertyNodeWithTooltip(StringView componentName, StringView propertyName, StringView value, std::function<void(StringView)> fnSetValue, HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No, HasParameterRefYN hasParameterRef = HasParameterRefYN::No, std::function<String()> fnGetValue = nullptr)
		{
			std::shared_ptr<Node> propertyNode;
			String displayName{ propertyName };  // デフォルトは実際のプロパティ名
			
			if (const auto it = m_propertyMetadata.find(PropertyKey{ String{ componentName }, String{ propertyName } }); it != m_propertyMetadata.end())
			{
				const auto& metadata = it->second;
				
				// 表示名が設定されていれば使用
				if (metadata.displayName)
				{
					displayName = *metadata.displayName;
				}
				
				if (metadata.numTextAreaLines.has_value())
				{
					// TextAreaを使用
					propertyNode = CreatePropertyNodeWithTextArea(displayName, value, std::move(fnSetValue), hasInteractivePropertyValue, *metadata.numTextAreaLines, std::move(fnGetValue), hasParameterRef);
				}
				else
				{
					// 通常のTextBoxを使用
					propertyNode = CreatePropertyNode(displayName, value, std::move(fnSetValue), hasInteractivePropertyValue, hasParameterRef, std::move(fnGetValue), metadata.dragValueChangeStep);
				}
				
				// ツールチップを追加
				if (metadata.tooltip)
				{
					if (const auto labelNode = propertyNode->getChildByNameOrNull(U"Label", RecursiveYN::Yes))
					{
						labelNode->emplaceComponent<TooltipOpener>(m_editorOverlayCanvas, *metadata.tooltip, metadata.tooltipDetail.value_or(U""));
					}
				}
			}
			else
			{
				// メタデータがない場合は通常のTextBoxを使用
				propertyNode = CreatePropertyNode(displayName, value, std::move(fnSetValue), hasInteractivePropertyValue, hasParameterRef, std::move(fnGetValue), none);
			}
			
			return propertyNode;
		}


		[[nodiscard]]
		std::shared_ptr<Node> createVec2PropertyNodeWithTooltip(StringView componentName, StringView propertyName, const Vec2& currentValue, std::function<void(const Vec2&)> fnSetValue, HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No, HasParameterRefYN hasParameterRef = HasParameterRefYN::No)
		{
			String displayName{ propertyName };  // デフォルトは実際のプロパティ名
			Optional<double> dragStep = none;
			
			// メタデータをチェックして表示名を取得
			if (const auto it = m_propertyMetadata.find(PropertyKey{ String{ componentName }, String{ propertyName } }); it != m_propertyMetadata.end())
			{
				const auto& metadata = it->second;
				if (metadata.displayName)
				{
					displayName = *metadata.displayName;
				}
				dragStep = metadata.dragValueChangeStep;
			}
			
			const auto propertyNode = CreateVec2PropertyNode(displayName, currentValue, std::move(fnSetValue), hasInteractivePropertyValue, hasParameterRef, dragStep);
			
			// メタデータに基づいてツールチップを追加
			if (const auto it = m_propertyMetadata.find(PropertyKey{ String{ componentName }, String{ propertyName } }); it != m_propertyMetadata.end())
			{
				const auto& metadata = it->second;
				if (metadata.tooltip)
				{
					if (const auto labelNode = propertyNode->getChildByNameOrNull(U"Label", RecursiveYN::Yes))
					{
						labelNode->emplaceComponent<TooltipOpener>(m_editorOverlayCanvas, *metadata.tooltip, metadata.tooltipDetail.value_or(U""));
					}
				}
			}
			
			return propertyNode;
		}

		[[nodiscard]]
		std::shared_ptr<Node> createEnumPropertyNodeWithTooltip(StringView componentName, StringView propertyName, StringView value, std::function<void(StringView)> fnSetValue, const std::shared_ptr<ContextMenu>& contextMenu, const Array<String>& enumValues, HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No, HasParameterRefYN hasParameterRef = HasParameterRefYN::No)
		{
			String displayName{ propertyName };  // デフォルトは実際のプロパティ名
			
			// メタデータをチェックして表示名を取得
			if (const auto it = m_propertyMetadata.find(PropertyKey{ String{ componentName }, String{ propertyName } }); it != m_propertyMetadata.end())
			{
				const auto& metadata = it->second;
				if (metadata.displayName)
				{
					displayName = *metadata.displayName;
				}
			}
			
			const auto propertyNode = CreateEnumPropertyNode(displayName, value, std::move(fnSetValue), contextMenu, enumValues, hasInteractivePropertyValue, hasParameterRef);
			
			// メタデータに基づいてツールチップを追加
			if (const auto it = m_propertyMetadata.find(PropertyKey{ String{ componentName }, String{ propertyName } }); it != m_propertyMetadata.end())
			{
				const auto& metadata = it->second;
				if (metadata.tooltip)
				{
					if (const auto labelNode = propertyNode->getChildByNameOrNull(U"Label", RecursiveYN::Yes))
					{
						labelNode->emplaceComponent<TooltipOpener>(m_editorOverlayCanvas, *metadata.tooltip, metadata.tooltipDetail.value_or(U""));
					}
				}
			}
			
			return propertyNode;
		}

		[[nodiscard]]
		std::shared_ptr<Node> createLRTBPropertyNodeWithTooltip(StringView componentName, StringView propertyName, const LRTB& currentValue, std::function<void(const LRTB&)> fnSetValue, HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No, HasParameterRefYN hasParameterRef = HasParameterRefYN::No)
		{
			String displayName{ propertyName };  // デフォルトは実際のプロパティ名
			
			// メタデータをチェックして表示名を取得
			if (const auto it = m_propertyMetadata.find(PropertyKey{ String{ componentName }, String{ propertyName } }); it != m_propertyMetadata.end())
			{
				const auto& metadata = it->second;
				if (metadata.displayName)
				{
					displayName = *metadata.displayName;
				}
			}
			
			const auto propertyNode = CreateLRTBPropertyNode(displayName, currentValue, std::move(fnSetValue), hasInteractivePropertyValue, hasParameterRef);
			
			// メタデータに基づいてツールチップを追加
			if (const auto it = m_propertyMetadata.find(PropertyKey{ String{ componentName }, String{ propertyName } }); it != m_propertyMetadata.end())
			{
				const auto& metadata = it->second;
				if (metadata.tooltip)
				{
					// Line1とLine2の両方のLabelノードにツールチップを追加
					if (const auto line1 = propertyNode->getChildByNameOrNull(U"Line1", RecursiveYN::No))
					{
						if (const auto labelNode = line1->getChildByNameOrNull(U"Label", RecursiveYN::No))
						{
							labelNode->emplaceComponent<TooltipOpener>(m_editorOverlayCanvas, *metadata.tooltip, metadata.tooltipDetail.value_or(U""));
						}
					}
					if (const auto line2 = propertyNode->getChildByNameOrNull(U"Line2", RecursiveYN::No))
					{
						if (const auto labelNode = line2->getChildByNameOrNull(U"Label", RecursiveYN::No))
						{
							labelNode->emplaceComponent<TooltipOpener>(m_editorOverlayCanvas, *metadata.tooltip, metadata.tooltipDetail.value_or(U""));
						}
					}
				}
			}
			
			return propertyNode;
		}

		[[nodiscard]]
		std::shared_ptr<Node> createBoolPropertyNodeWithTooltip(StringView componentName, StringView propertyName, bool currentValue, std::function<void(bool)> fnSetValue, HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No, HasParameterRefYN hasParameterRef = HasParameterRefYN::No)
		{
			String displayName{ propertyName };  // デフォルトは実際のプロパティ名
			
			// メタデータをチェックして表示名を取得
			if (const auto it = m_propertyMetadata.find(PropertyKey{ String{ componentName }, String{ propertyName } }); it != m_propertyMetadata.end())
			{
				const auto& metadata = it->second;
				if (metadata.displayName)
				{
					displayName = *metadata.displayName;
				}
			}
			
			const auto propertyNode = CreateBoolPropertyNode(displayName, currentValue, std::move(fnSetValue), hasInteractivePropertyValue, hasParameterRef);
			
			// メタデータに基づいてツールチップを追加
			if (const auto it = m_propertyMetadata.find(PropertyKey{ String{ componentName }, String{ propertyName } }); it != m_propertyMetadata.end())
			{
				const auto& metadata = it->second;
				if (metadata.tooltip)
				{
					// boolプロパティの場合は、propertyNode全体にツールチップを追加
					propertyNode->emplaceComponent<TooltipOpener>(m_editorOverlayCanvas, *metadata.tooltip, metadata.tooltipDetail.value_or(U""));
				}
			}
			
			return propertyNode;
		}

		[[nodiscard]]
		std::shared_ptr<Node> createColorPropertyNodeWithTooltip(StringView componentName, StringView propertyName, const ColorF& currentValue, std::function<void(const ColorF&)> fnSetValue, HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No, HasParameterRefYN hasParameterRef = HasParameterRefYN::No)
		{
			String displayName{ propertyName };  // デフォルトは実際のプロパティ名
			
			// メタデータをチェックして表示名を取得
			if (const auto it = m_propertyMetadata.find(PropertyKey{ String{ componentName }, String{ propertyName } }); it != m_propertyMetadata.end())
			{
				const auto& metadata = it->second;
				if (metadata.displayName)
				{
					displayName = *metadata.displayName;
				}
			}
			
			const auto propertyNode = CreateColorPropertyNode(displayName, currentValue, std::move(fnSetValue), hasInteractivePropertyValue, hasParameterRef);
			
			// メタデータに基づいてツールチップを追加
			if (const auto it = m_propertyMetadata.find(PropertyKey{ String{ componentName }, String{ propertyName } }); it != m_propertyMetadata.end())
			{
				const auto& metadata = it->second;
				if (metadata.tooltip)
				{
					if (const auto labelNode = propertyNode->getChildByNameOrNull(U"Label", RecursiveYN::Yes))
					{
						labelNode->emplaceComponent<TooltipOpener>(m_editorOverlayCanvas, *metadata.tooltip, metadata.tooltipDetail.value_or(U""));
					}
				}
			}
			
			return propertyNode;
		}

		[[nodiscard]]
		static std::shared_ptr<Node> CreatePropertyNode(StringView name, StringView value, std::function<void(StringView)> fnSetValue, HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No, HasParameterRefYN hasParameterRef = HasParameterRefYN::No, std::function<String()> fnGetValue = nullptr, Optional<double> dragValueChangeStep = none)
		{
			const auto propertyNode = Node::Create(
				name,
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 32 },
				},
				IsHitTargetYN::Yes,
				InheritChildrenStateFlags::Hovered);
			propertyNode->setChildrenLayout(HorizontalLayout{ .padding = LRTB{ 10, 8, 0, 0 } });
			propertyNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }), Palette::Black, 0.0, 3.0);
			
			const auto labelNode = propertyNode->emplaceChild(
				U"Label",
				InlineRegion
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 0.85,
				},
				IsHitTargetYN::Yes,
				InheritChildrenStateFlags::Hovered | InheritChildrenStateFlags::Pressed);
			labelNode->setChildrenLayout(HorizontalLayout{});
			const auto labelComponent = labelNode->emplaceComponent<Label>(
				name,
				U"",
				14,
				Palette::White,
				HorizontalAlign::Left,
				VerticalAlign::Middle,
				LRTB{ 5, 5, 5, 5 },
				HorizontalOverflow::Wrap,
				VerticalOverflow::Clip,
				Vec2::Zero(),
				(hasParameterRef || hasInteractivePropertyValue) ? LabelUnderlineStyle::Solid : LabelUnderlineStyle::None,
				hasParameterRef ? ColorF{ Palette::Cyan, 0.5 } : ColorF{ Palette::Yellow, 0.5 },
				2.0,
				LabelSizingMode::AutoShrink);
			
			const auto textBoxNode = propertyNode->emplaceChild(
				U"TextBox",
				InlineRegion
				{
					.sizeDelta = Vec2{ 0, 26 },
					.flexibleWeight = 1,
				});
			textBoxNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"focused", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textBox = textBoxNode->emplaceComponent<TextBox>(U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, HorizontalAlign::Left, VerticalAlign::Middle, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textBox->setText(value, IgnoreIsChangedYN::Yes);
			textBoxNode->addComponent(std::make_shared<PropertyTextBox>(textBox, fnSetValue, fnGetValue));
			textBoxNode->emplaceComponent<TabStop>();
			
			// dragValueChangeStepが設定されている場合、ラベルにPropertyLabelDraggerを追加
			if (dragValueChangeStep.has_value())
			{
				// textBoxへの参照を取得してラムダでキャプチャ
				const auto textBoxWeak = std::weak_ptr<TextBox>(textBox);
				const auto labelComponentWeak = std::weak_ptr<Label>(labelComponent);
				
				// fnGetValueがある場合はそれを使い、ない場合はtextBoxから直接取得
				if (fnGetValue)
				{
					// 初期値を記録
					double initialValue = ParseOpt<double>(fnGetValue()).value_or(0.0);
					
					labelNode->emplaceComponent<PropertyLabelDragger>(
						[fnSetValue, textBoxWeak, labelComponentWeak, initialValue, hasInteractivePropertyValue, hasParameterRef](double newValue) mutable {
							// 値が実際に変更された場合のみ更新
							if (std::abs(newValue - initialValue) > 0.0001)  // 浮動小数点の誤差を考慮
							{
								fnSetValue(Format(newValue));
								if (const auto tb = textBoxWeak.lock())
								{
									tb->setText(Format(newValue));
								}
								// インタラクティブ値があった場合は下線を消す（パラメータ参照がある場合は緑の下線を維持）
								if (hasInteractivePropertyValue && !hasParameterRef)
								{
									if (const auto label = labelComponentWeak.lock())
									{
										label->setUnderlineStyle(LabelUnderlineStyle::None);
									}
								}
								initialValue = newValue;
							}
						},
						[fnGetValue]() { return ParseOpt<double>(fnGetValue()).value_or(0.0); },
						*dragValueChangeStep
					);
				}
				else
				{
					labelNode->emplaceComponent<PropertyLabelDragger>(
						[fnSetValue, textBoxWeak, labelComponentWeak, hasInteractivePropertyValue, hasParameterRef](double newValue) {
							// 現在の値を取得
							double currentValue = 0.0;
							if (const auto tb = textBoxWeak.lock())
							{
								currentValue = ParseOpt<double>(tb->text()).value_or(0.0);
							}
							
							// 値が実際に変更された場合のみ更新
							if (std::abs(newValue - currentValue) > 0.0001)  // 浮動小数点の誤差を考慮
							{
								fnSetValue(Format(newValue));
								if (const auto tb = textBoxWeak.lock())
								{
									tb->setText(Format(newValue));
								}
								// インタラクティブ値があった場合は下線を消す（パラメータ参照がある場合は緑の下線を維持）
								if (hasInteractivePropertyValue && !hasParameterRef)
								{
									if (const auto label = labelComponentWeak.lock())
									{
										label->setUnderlineStyle(LabelUnderlineStyle::None);
									}
								}
							}
						},
						[textBoxWeak]() { 
							if (const auto tb = textBoxWeak.lock())
							{
								return ParseOpt<double>(tb->text()).value_or(0.0);
							}
							return 0.0;
						},
						*dragValueChangeStep
					);
				}
			}
			return propertyNode;
		}

		[[nodiscard]]
		static std::shared_ptr<Node> CreatePropertyNodeWithTextArea(StringView name, StringView value, std::function<void(StringView)> fnSetValue, HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No, int32 numLines = 3, std::function<String()> fnGetValue = nullptr, HasParameterRefYN hasParameterRef = HasParameterRefYN::No)
		{
			const double textAreaHeight = numLines * 20.0 + 14.0;
			const double nodeHeight = textAreaHeight + 6.0;
			
			const auto propertyNode = Node::Create(
				name,
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, nodeHeight },
				},
				IsHitTargetYN::Yes,
				InheritChildrenStateFlags::Hovered);
			propertyNode->setChildrenLayout(HorizontalLayout{ .padding = LRTB{ 10, 8, 0, 0 } });
			propertyNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }), Palette::Black, 0.0, 3.0);
			const auto labelNode = propertyNode->emplaceChild(
				U"Label",
				InlineRegion
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 0.85,
				});
			labelNode->emplaceComponent<Label>(
				name,
				U"",
				14,
				Palette::White,
				HorizontalAlign::Left,
				VerticalAlign::Middle,
				LRTB{ 5, 5, 5, 5 },
				HorizontalOverflow::Wrap,
				VerticalOverflow::Clip,
				Vec2::Zero(),
				(hasParameterRef || hasInteractivePropertyValue) ? LabelUnderlineStyle::Solid : LabelUnderlineStyle::None,
				hasParameterRef ? ColorF{ Palette::Cyan, 0.5 } : ColorF{ Palette::Yellow, 0.5 },
				2.0,
				LabelSizingMode::AutoShrink);
			const auto textAreaNode = propertyNode->emplaceChild(
				U"TextArea",
				InlineRegion
				{
					.sizeDelta = Vec2{ 0, textAreaHeight },
					.flexibleWeight = 1,
				});
			textAreaNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"focused", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textArea = textAreaNode->emplaceComponent<TextArea>(U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textArea->setText(value, IgnoreIsChangedYN::Yes);
			
			// PropertyTextAreaコンポーネントを追加
			class PropertyTextArea : public ComponentBase
			{
			private:
				std::shared_ptr<TextArea> m_textArea;
				std::function<void(StringView)> m_fnSetValue;
				std::function<String()> m_fnGetValue;
				String m_prevExternalValue;

			public:
				PropertyTextArea(const std::shared_ptr<TextArea>& textArea, std::function<void(StringView)> fnSetValue, std::function<String()> fnGetValue = nullptr)
					: ComponentBase{ {} }
					, m_textArea{ textArea }
					, m_fnSetValue{ std::move(fnSetValue) }
					, m_fnGetValue{ std::move(fnGetValue) }
					, m_prevExternalValue{ m_fnGetValue ? String{ m_fnGetValue() } : U"" }
				{
				}

				void update(const std::shared_ptr<Node>&) override
				{
					// 外部からの値の変更をチェック
					if (m_fnGetValue)
					{
						const String currentExternalValue = String{ m_fnGetValue() };
						if (!m_textArea->isEditing() && currentExternalValue != m_prevExternalValue)
						{
							m_textArea->setText(currentExternalValue, IgnoreIsChangedYN::Yes);
							m_prevExternalValue = currentExternalValue;
						}
					}

					// ユーザーによる変更をチェック
					if (m_textArea->isChanged())
					{
						m_fnSetValue(m_textArea->text());
						if (m_fnGetValue)
						{
							m_prevExternalValue = String{ m_fnGetValue() };
						}
					}
				}
			};
			
			textAreaNode->addComponent(std::make_shared<PropertyTextArea>(textArea, std::move(fnSetValue), std::move(fnGetValue)));
			textAreaNode->emplaceComponent<TabStop>();
			return propertyNode;
		}

		[[nodiscard]]
		static std::shared_ptr<Node> CreateVec2PropertyNode(
			StringView name,
			const Vec2& currentValue,
			std::function<void(const Vec2&)> fnSetValue,
			HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No,
			HasParameterRefYN hasParameterRef = HasParameterRefYN::No,
			Optional<double> dragValueChangeStep = none)
		{
			double step = dragValueChangeStep.value_or(1.0);
			const auto propertyNode = Node::Create(
				name,
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 32 },
				},
				IsHitTargetYN::Yes,
				InheritChildrenStateFlags::Hovered);
			propertyNode->setChildrenLayout(HorizontalLayout{ .padding = LRTB{ 10, 8, 0, 0 } });
			propertyNode->emplaceComponent<RectRenderer>(
				PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }),
				Palette::Black,
				0.0,
				3.0);

			const auto labelNode = propertyNode->emplaceChild(
				U"Label",
				InlineRegion
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 0.85,
				});
			labelNode->emplaceComponent<Label>(
				name,
				U"",
				14,
				Palette::White,
				HorizontalAlign::Left,
				VerticalAlign::Middle,
				LRTB{ 5, 5, 5, 5 },
				HorizontalOverflow::Wrap,
				VerticalOverflow::Clip,
				Vec2::Zero(),
				(hasParameterRef || hasInteractivePropertyValue) ? LabelUnderlineStyle::Solid : LabelUnderlineStyle::None,
				hasParameterRef ? ColorF{ Palette::Cyan, 0.5 } : ColorF{ Palette::Yellow, 0.5 },
				2.0,
				LabelSizingMode::AutoShrink);

			const auto textBoxParentNode = propertyNode->emplaceChild(
				U"TextBoxParent",
				InlineRegion
				{
					.sizeDelta = Vec2{ 0, 26 },
					.flexibleWeight = 1,
				},
				IsHitTargetYN::No,
				InheritChildrenStateFlags::Hovered);
			textBoxParentNode->setChildrenLayout(HorizontalLayout{});

			// Xラベル
			const auto xLabelNode = textBoxParentNode->emplaceChild(
				U"XLabel",
				InlineRegion
				{
					.sizeDelta = Vec2{ 20, 26 },
					.flexibleWeight = 0,
				});
			xLabelNode->emplaceComponent<Label>(
				U"X", U"", 12, ColorF{ 0.8, 0.8, 0.8 },
				HorizontalAlign::Center, VerticalAlign::Middle);
			// Xラベルに背景を追加（ホバー時のフィードバック用）
			xLabelNode->emplaceComponent<RectRenderer>(
				PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }),
				Palette::Black, 0.0, 2.0);

			// X
			const auto textBoxXNode = textBoxParentNode->emplaceChild(
				U"TextBoxX",
				InlineRegion
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 1,
					.margin = LRTB{ 0, 6, 0, 0 },
				});
			textBoxXNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"focused", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textBoxX = textBoxXNode->emplaceComponent<TextBox>(
				U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, HorizontalAlign::Left, VerticalAlign::Middle, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textBoxXNode->emplaceComponent<TabStop>();
			textBoxX->setText(Format(currentValue.x), IgnoreIsChangedYN::Yes);

			// Yラベル
			const auto yLabelNode = textBoxParentNode->emplaceChild(
				U"YLabel",
				InlineRegion
				{
					.sizeDelta = Vec2{ 20, 26 },
					.flexibleWeight = 0,
				});
			yLabelNode->emplaceComponent<Label>(
				U"Y", U"", 12, ColorF{ 0.8, 0.8, 0.8 },
				HorizontalAlign::Center, VerticalAlign::Middle);
			// Yラベルに背景を追加（ホバー時のフィードバック用）
			yLabelNode->emplaceComponent<RectRenderer>(
				PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }),
				Palette::Black, 0.0, 2.0);

			// Y
			const auto textBoxYNode = textBoxParentNode->emplaceChild(
				U"TextBoxY",
				InlineRegion
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 1,
					.margin = LRTB{ 0, 0, 0, 0 },
				});
			textBoxYNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"focused", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textBoxY = textBoxYNode->emplaceComponent<TextBox>(
				U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, HorizontalAlign::Left, VerticalAlign::Middle, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textBoxYNode->emplaceComponent<TabStop>();
			textBoxY->setText(Format(currentValue.y), IgnoreIsChangedYN::Yes);

			const auto vec2PropertyTextBox = std::make_shared<Vec2PropertyTextBox>(
				textBoxX,
				textBoxY,
				fnSetValue,
				currentValue);
			propertyNode->addComponent(vec2PropertyTextBox);

			// PropertyLabelDraggerを追加
			{
				// Xラベルにドラッグ機能を追加
				xLabelNode->emplaceComponent<PropertyLabelDragger>(
					[vec2PropertyTextBox, textBoxX](double value)
					{
						const Vec2 currentVec = vec2PropertyTextBox->value();
						vec2PropertyTextBox->setValue(Vec2{ value, currentVec.y }, true);
						textBoxX->setText(Format(value));
					},
					[vec2PropertyTextBox]() -> double
					{
						return vec2PropertyTextBox->value().x;
					},
					step,  // 感度
					-std::numeric_limits<double>::max(),  // 最小値
					std::numeric_limits<double>::max()   // 最大値
				);

				// Yラベルにドラッグ機能を追加
				yLabelNode->emplaceComponent<PropertyLabelDragger>(
					[vec2PropertyTextBox, textBoxY](double value)
					{
						const Vec2 currentVec = vec2PropertyTextBox->value();
						vec2PropertyTextBox->setValue(Vec2{ currentVec.x, value }, true);
						textBoxY->setText(Format(value));
					},
					[vec2PropertyTextBox]() -> double
					{
						return vec2PropertyTextBox->value().y;
					},
					step,  // 感度
					-std::numeric_limits<double>::max(),  // 最小値
					std::numeric_limits<double>::max()   // 最大値
				);
			}

			return propertyNode;
		}

		[[nodiscard]]
		static std::shared_ptr<Node> CreateVec4PropertyNode(
			StringView name,
			const Vec4& currentValue,
			std::function<void(const Vec4&)> fnSetValue,
			HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No,
			HasParameterRefYN hasParameterRef = HasParameterRefYN::No)
		{
			const auto propertyNode = Node::Create(
				name,
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 32 },
				},
				IsHitTargetYN::Yes,
				InheritChildrenStateFlags::Hovered);
			propertyNode->emplaceComponent<RectRenderer>(
				PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }),
				Palette::Black,
				0.0,
				3.0);

			const auto labelNode = propertyNode->emplaceChild(
				U"Label",
				InlineRegion
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 0.85,
				});
			labelNode->emplaceComponent<Label>(
				name,
				U"",
				14,
				Palette::White,
				HorizontalAlign::Left,
				VerticalAlign::Middle,
				LRTB{ 5, 5, 5, 5 },
				HorizontalOverflow::Wrap,
				VerticalOverflow::Clip,
				Vec2::Zero(),
				(hasParameterRef || hasInteractivePropertyValue) ? LabelUnderlineStyle::Solid : LabelUnderlineStyle::None,
				hasParameterRef ? ColorF{ Palette::Cyan, 0.5 } : ColorF{ Palette::Yellow, 0.5 },
				2.0,
				LabelSizingMode::AutoShrink);

			const auto textBoxParentNode = propertyNode->emplaceChild(
				U"TextBoxParent",
				InlineRegion
				{
					.sizeDelta = Vec2{ 0, 26 },
					.flexibleWeight = 1,
				},
				IsHitTargetYN::No,
				InheritChildrenStateFlags::Hovered);
			textBoxParentNode->setChildrenLayout(HorizontalLayout{});

			// X
			const auto textBoxXNode = textBoxParentNode->emplaceChild(
				U"TextBoxX",
				InlineRegion
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 1,
					.margin = LRTB{ 0, 2, 0, 0 },
				});
			textBoxXNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"focused", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textBoxX = textBoxXNode->emplaceComponent<TextBox>(
				U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, HorizontalAlign::Left, VerticalAlign::Middle, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textBoxXNode->emplaceComponent<TabStop>();
			textBoxX->setText(Format(currentValue.x), IgnoreIsChangedYN::Yes);

			// Y
			const auto textBoxYNode = textBoxParentNode->emplaceChild(
				U"TextBoxY",
				InlineRegion
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 1,
					.margin = LRTB{ 2, 2, 0, 0 },
				});
			textBoxYNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"focused", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textBoxY = textBoxYNode->emplaceComponent<TextBox>(
				U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, HorizontalAlign::Left, VerticalAlign::Middle, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textBoxYNode->emplaceComponent<TabStop>();
			textBoxY->setText(Format(currentValue.y), IgnoreIsChangedYN::Yes);

			// Z
			const auto textBoxZNode = textBoxParentNode->emplaceChild(
				U"TextBoxZ",
				InlineRegion
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 1,
					.margin = LRTB{ 2, 2, 0, 0 },
				});
			textBoxZNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"focused", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textBoxZ = textBoxZNode->emplaceComponent<TextBox>(
				U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, HorizontalAlign::Left, VerticalAlign::Middle, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textBoxZNode->emplaceComponent<TabStop>();
			textBoxZ->setText(Format(currentValue.z), IgnoreIsChangedYN::Yes);

			// W
			const auto textBoxWNode = textBoxParentNode->emplaceChild(
				U"TextBoxW",
				InlineRegion
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 1,
					.margin = LRTB{ 2, 0, 0, 0 },
				});
			textBoxWNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"focused", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textBoxW = textBoxWNode->emplaceComponent<TextBox>(
				U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, HorizontalAlign::Left, VerticalAlign::Middle, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textBoxWNode->emplaceComponent<TabStop>();
			textBoxW->setText(Format(currentValue.w), IgnoreIsChangedYN::Yes);

			propertyNode->addComponent(std::make_shared<Vec4PropertyTextBox>(
				textBoxX,
				textBoxY,
				textBoxZ,
				textBoxW,
				fnSetValue,
				currentValue));

			return propertyNode;
		}

		[[nodiscard]]
		static std::shared_ptr<Node> CreateLRTBPropertyNode(
			StringView name,
			const LRTB& currentValue,
			std::function<void(const LRTB&)> fnSetValue,
			HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No,
			HasParameterRefYN hasParameterRef = HasParameterRefYN::No)
		{
			constexpr int32 LineHeight = 32;
			const auto propertyNode = Node::Create(
				name,
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, LineHeight * 2 },
				},
				IsHitTargetYN::Yes,
				InheritChildrenStateFlags::Hovered);
			propertyNode->setChildrenLayout(VerticalLayout{ .padding = LRTB{ 10, 8, 0, 0 } });
			propertyNode->emplaceComponent<RectRenderer>(
				PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }),
				Palette::Black,
				0.0,
				3.0);

			const auto line1 = propertyNode->emplaceChild(
				U"Line1",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, LineHeight },
				},
				IsHitTargetYN::No,
				InheritChildrenStateFlags::Hovered);
			line1->setChildrenLayout(HorizontalLayout{});

			const auto line1LabelNode =
				line1->emplaceChild(
					U"Label",
					InlineRegion
					{
						.sizeRatio = Vec2{ 0, 1 },
						.flexibleWeight = 0.85,
					});
			line1LabelNode->emplaceComponent<Label>(
				name,
				U"",
				14,
				Palette::White,
				HorizontalAlign::Left,
				VerticalAlign::Middle,
				LRTB{ 5, 5, 5, 5 },
				HorizontalOverflow::Wrap,
				VerticalOverflow::Clip,
				Vec2::Zero(),
				(hasParameterRef || hasInteractivePropertyValue) ? LabelUnderlineStyle::Solid : LabelUnderlineStyle::None,
				hasParameterRef ? ColorF{ Palette::Cyan, 0.5 } : ColorF{ Palette::Yellow, 0.5 },
				2.0,
				LabelSizingMode::AutoShrink);

			const auto line1TextBoxParentNode = line1->emplaceChild(
				U"TextBoxParent",
				InlineRegion
				{
					.sizeDelta = Vec2{ 0, 26 },
					.flexibleWeight = 1,
				},
				IsHitTargetYN::No,
				InheritChildrenStateFlags::Hovered);
			line1TextBoxParentNode->setChildrenLayout(HorizontalLayout{});

			// Lラベル
			const auto lLabelNode = line1TextBoxParentNode->emplaceChild(
				U"LLabel",
				InlineRegion
				{
					.sizeDelta = Vec2{ 16, 26 },
					.flexibleWeight = 0,
				});
			lLabelNode->emplaceComponent<Label>(
				U"L", U"", 12, ColorF{ 0.8, 0.8, 0.8 },
				HorizontalAlign::Center, VerticalAlign::Middle);
			// Lラベルに背景を追加（ホバー時のフィードバック用）
			lLabelNode->emplaceComponent<RectRenderer>(
				PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }),
				Palette::Black, 0.0, 2.0);

			// L
			const auto textBoxLNode = line1TextBoxParentNode->emplaceChild(
				U"TextBoxL",
				InlineRegion
				{
					.sizeDelta = Vec2{ 0, 26 },
					.flexibleWeight = 1,
					.margin = LRTB{ 0, 4, 0, 0 },
				});
			textBoxLNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"focused", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textBoxL = textBoxLNode->emplaceComponent<TextBox>(
				U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, HorizontalAlign::Left, VerticalAlign::Middle, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textBoxLNode->emplaceComponent<TabStop>();
			textBoxL->setText(Format(currentValue.left), IgnoreIsChangedYN::Yes);

			// Rラベル
			const auto rLabelNode = line1TextBoxParentNode->emplaceChild(
				U"RLabel",
				InlineRegion
				{
					.sizeDelta = Vec2{ 16, 26 },
					.flexibleWeight = 0,
				});
			rLabelNode->emplaceComponent<Label>(
				U"R", U"", 12, ColorF{ 0.8, 0.8, 0.8 },
				HorizontalAlign::Center, VerticalAlign::Middle);
			// Rラベルに背景を追加（ホバー時のフィードバック用）
			rLabelNode->emplaceComponent<RectRenderer>(
				PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }),
				Palette::Black, 0.0, 2.0);

			// R
			const auto textBoxRNode = line1TextBoxParentNode->emplaceChild(
				U"TextBoxR",
				InlineRegion
				{
					.sizeDelta = Vec2{ 0, 26 },
					.flexibleWeight = 1,
					.margin = LRTB{ 0, 0, 0, 0 },
				});
			textBoxRNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"focused", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textBoxR = textBoxRNode->emplaceComponent<TextBox>(
				U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, HorizontalAlign::Left, VerticalAlign::Middle, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textBoxRNode->emplaceComponent<TabStop>();
			textBoxR->setText(Format(currentValue.right), IgnoreIsChangedYN::Yes);

			const auto line2 = propertyNode->emplaceChild(
				U"Line2",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, LineHeight },
				},
				IsHitTargetYN::No,
				InheritChildrenStateFlags::Hovered);
			line2->setChildrenLayout(HorizontalLayout{});

			const auto line2LabelNode =
				line2->emplaceChild(
					U"Label",
					InlineRegion
					{
						.sizeRatio = Vec2{ 0, 1 },
						.flexibleWeight = 0.85,
					});
			line2LabelNode->emplaceComponent<Label>(
				U"",
				U"",
				14,
				Palette::White,
				HorizontalAlign::Left,
				VerticalAlign::Middle,
				LRTB{ 5, 5, 5, 5 },
				HorizontalOverflow::Wrap,
				VerticalOverflow::Clip,
				Vec2::Zero(),
				LabelUnderlineStyle::None,
				ColorF{ Palette::Yellow, 0.5 },
				2.0,
				LabelSizingMode::AutoShrink);

			const auto line2TextBoxParentNode = line2->emplaceChild(
				U"TextBoxParent",
				InlineRegion
				{
					.sizeDelta = Vec2{ 0, 26 },
					.flexibleWeight = 1,
				},
				IsHitTargetYN::No,
				InheritChildrenStateFlags::Hovered);
			line2TextBoxParentNode->setChildrenLayout(HorizontalLayout{});

			// Tラベル
			const auto tLabelNode = line2TextBoxParentNode->emplaceChild(
				U"TLabel",
				InlineRegion
				{
					.sizeDelta = Vec2{ 16, 26 },
					.flexibleWeight = 0,
				});
			tLabelNode->emplaceComponent<Label>(
				U"T", U"", 12, ColorF{ 0.8, 0.8, 0.8 },
				HorizontalAlign::Center, VerticalAlign::Middle);
			// Tラベルに背景を追加（ホバー時のフィードバック用）
			tLabelNode->emplaceComponent<RectRenderer>(
				PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }),
				Palette::Black, 0.0, 2.0);

			// T
			const auto textBoxTNode = line2TextBoxParentNode->emplaceChild(
				U"TextBoxT",
				InlineRegion
				{
					.sizeDelta = Vec2{ 0, 26 },
					.flexibleWeight = 1,
					.margin = LRTB{ 0, 4, 0, 0 },
				});
			textBoxTNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"focused", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textBoxT = textBoxTNode->emplaceComponent<TextBox>(
				U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, HorizontalAlign::Left, VerticalAlign::Middle, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textBoxTNode->emplaceComponent<TabStop>();
			textBoxT->setText(Format(currentValue.top), IgnoreIsChangedYN::Yes);

			// Bラベル
			const auto bLabelNode = line2TextBoxParentNode->emplaceChild(
				U"BLabel",
				InlineRegion
				{
					.sizeDelta = Vec2{ 16, 26 },
					.flexibleWeight = 0,
				});
			bLabelNode->emplaceComponent<Label>(
				U"B", U"", 12, ColorF{ 0.8, 0.8, 0.8 },
				HorizontalAlign::Center, VerticalAlign::Middle);
			// Bラベルに背景を追加（ホバー時のフィードバック用）
			bLabelNode->emplaceComponent<RectRenderer>(
				PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }),
				Palette::Black, 0.0, 2.0);

			// B
			const auto textBoxBNode = line2TextBoxParentNode->emplaceChild(
				U"TextBoxB",
				InlineRegion
				{
					.sizeDelta = Vec2{ 0, 26 },
					.flexibleWeight = 1,
					.margin = LRTB{ 0, 0, 0, 0 },
				});
			textBoxBNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"focused", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textBoxB = textBoxBNode->emplaceComponent<TextBox>(
				U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, HorizontalAlign::Left, VerticalAlign::Middle, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textBoxBNode->emplaceComponent<TabStop>();
			textBoxB->setText(Format(currentValue.bottom), IgnoreIsChangedYN::Yes);

			const auto lrtbPropertyTextBox = std::make_shared<LRTBPropertyTextBox>(
				textBoxL,
				textBoxR,
				textBoxT,
				textBoxB,
				fnSetValue,
				currentValue);
			propertyNode->addComponent(lrtbPropertyTextBox);

			// PropertyLabelDraggerを追加
			{
				// Lラベルにドラッグ機能を追加
				lLabelNode->emplaceComponent<PropertyLabelDragger>(
					[lrtbPropertyTextBox, textBoxL](double value)
					{
						const LRTB currentLRTB = lrtbPropertyTextBox->value();
						lrtbPropertyTextBox->setValue(LRTB{ value, currentLRTB.right, currentLRTB.top, currentLRTB.bottom }, true);
						textBoxL->setText(Format(value));
					},
					[lrtbPropertyTextBox]() -> double
					{
						return lrtbPropertyTextBox->value().left;
					},
					1.0,  // 感度
					-std::numeric_limits<double>::max(),  // 最小値
					std::numeric_limits<double>::max()   // 最大値
				);

				// Rラベルにドラッグ機能を追加
				rLabelNode->emplaceComponent<PropertyLabelDragger>(
					[lrtbPropertyTextBox, textBoxR](double value)
					{
						const LRTB currentLRTB = lrtbPropertyTextBox->value();
						lrtbPropertyTextBox->setValue(LRTB{ currentLRTB.left, value, currentLRTB.top, currentLRTB.bottom }, true);
						textBoxR->setText(Format(value));
					},
					[lrtbPropertyTextBox]() -> double
					{
						return lrtbPropertyTextBox->value().right;
					},
					1.0,  // 感度
					-std::numeric_limits<double>::max(),  // 最小値
					std::numeric_limits<double>::max()   // 最大値
				);

				// Tラベルにドラッグ機能を追加
				tLabelNode->emplaceComponent<PropertyLabelDragger>(
					[lrtbPropertyTextBox, textBoxT](double value)
					{
						const LRTB currentLRTB = lrtbPropertyTextBox->value();
						lrtbPropertyTextBox->setValue(LRTB{ currentLRTB.left, currentLRTB.right, value, currentLRTB.bottom }, true);
						textBoxT->setText(Format(value));
					},
					[lrtbPropertyTextBox]() -> double
					{
						return lrtbPropertyTextBox->value().top;
					},
					1.0,  // 感度
					-std::numeric_limits<double>::max(),  // 最小値
					std::numeric_limits<double>::max()   // 最大値
				);

				// Bラベルにドラッグ機能を追加
				bLabelNode->emplaceComponent<PropertyLabelDragger>(
					[lrtbPropertyTextBox, textBoxB](double value)
					{
						const LRTB currentLRTB = lrtbPropertyTextBox->value();
						lrtbPropertyTextBox->setValue(LRTB{ currentLRTB.left, currentLRTB.right, currentLRTB.top, value }, true);
						textBoxB->setText(Format(value));
					},
					[lrtbPropertyTextBox]() -> double
					{
						return lrtbPropertyTextBox->value().bottom;
					},
					1.0,  // 感度
					-std::numeric_limits<double>::max(),  // 最小値
					std::numeric_limits<double>::max()   // 最大値
				);
			}

			return propertyNode;
		}

		[[nodiscard]]
		static std::shared_ptr<Node> CreateColorPropertyNode(
			StringView name,
			const ColorF& currentValue,
			std::function<void(const ColorF&)> fnSetValue,
			HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No,
			HasParameterRefYN hasParameterRef = HasParameterRefYN::No)
		{
			const auto propertyNode = Node::Create(
				name,
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 36 },
				},
				IsHitTargetYN::Yes,
				InheritChildrenStateFlags::Hovered);
			propertyNode->setChildrenLayout(HorizontalLayout{ .padding = LRTB{ 10, 8, 0, 0 } });
			propertyNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }), Palette::Black, 0.0, 3.0);

			const auto labelNode = propertyNode->emplaceChild(
				U"Label",
				InlineRegion
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 0.85,
				});
			labelNode->emplaceComponent<Label>(
				name,
				U"",
				14,
				Palette::White,
				HorizontalAlign::Left,
				VerticalAlign::Middle,
				LRTB{ 5, 5, 5, 5 },
				HorizontalOverflow::Wrap,
				VerticalOverflow::Clip,
				Vec2::Zero(),
				(hasParameterRef || hasInteractivePropertyValue) ? LabelUnderlineStyle::Solid : LabelUnderlineStyle::None,
				hasParameterRef ? ColorF{ Palette::Cyan, 0.5 } : ColorF{ Palette::Yellow, 0.5 },
				2.0,
				LabelSizingMode::AutoShrink);

			const auto rowNode = propertyNode->emplaceChild(
				U"ColorPropertyRow",
				InlineRegion
				{
					.sizeDelta = Vec2{ 0, 26 },
					.flexibleWeight = 1,
				},
				IsHitTargetYN::No,
				InheritChildrenStateFlags::Hovered);
			rowNode->setChildrenLayout(HorizontalLayout{});

			const auto previewRootNode = rowNode->emplaceChild(
				U"ColorPreviewRoot",
				InlineRegion
				{
					.sizeRatio = Vec2{ 0, 1 },
					.sizeDelta = Vec2{ 26, 0 },
					.margin = LRTB{ 0, 2, 0, 0 },
				},
				IsHitTargetYN::No);

			// 透明の市松模様
			constexpr int32 GridSize = 3;
			for (int32 y = 0; y < GridSize; ++y)
			{
				for (int32 x = 0; x < GridSize; ++x)
				{
					const bool isOdd = (x + y) % 2 == 1;
					const auto previewNode = previewRootNode->emplaceChild(
						U"Transparent",
						AnchorRegion
						{
							.anchorMin = { static_cast<double>(x) / GridSize, static_cast<double>(y) / GridSize },
							.anchorMax = { static_cast<double>(x + 1) / GridSize, static_cast<double>(y + 1) / GridSize },
							.sizeDeltaPivot = Anchor::TopLeft,
						},
						IsHitTargetYN::No)
						->emplaceComponent<RectRenderer>(ColorF{ isOdd ? 0.9 : 1.0 });
				}
			}

			// 色プレビュー
			const auto previewNode = previewRootNode->emplaceChild(
				U"ColorPreview",
				InlineRegion
				{
					.sizeRatio = Vec2{ 0, 1 },
					.sizeDelta = Vec2{ 26, 0 },
					.margin = LRTB{ 0, 2, 0, 0 },
				},
				IsHitTargetYN::No);
			const auto previewRectRenderer = previewNode->emplaceComponent<RectRenderer>(currentValue, ColorF{ 1.0, 0.3 }, 1.0, 0.0);

			const auto textBoxParentNode = rowNode->emplaceChild(
				U"TextBoxParent",
				InlineRegion
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 1,
				},
				IsHitTargetYN::No,
				InheritChildrenStateFlags::Hovered);
			textBoxParentNode->setChildrenLayout(HorizontalLayout{});

			// Rラベル
			const auto rLabelNode = textBoxParentNode->emplaceChild(
				U"RLabel",
				InlineRegion
				{
					.sizeDelta = Vec2{ 16, 26 },
					.flexibleWeight = 0,
				});
			rLabelNode->emplaceComponent<Label>(
				U"R", U"", 12, ColorF{ 1.0, 0.7, 0.7 },
				HorizontalAlign::Center, VerticalAlign::Middle);
			// Rラベルに背景を追加（ホバー時のフィードバック用）
			rLabelNode->emplaceComponent<RectRenderer>(
				PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }),
				Palette::Black, 0.0, 2.0);

			// R
			const auto textBoxRNode = textBoxParentNode->emplaceChild(
				U"TextBoxR",
				InlineRegion
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 1,
					.margin = LRTB{ 0, 4, 0, 0 },
				});
			textBoxRNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"focused", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textBoxR = textBoxRNode->emplaceComponent<TextBox>(
				U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, HorizontalAlign::Left, VerticalAlign::Middle, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textBoxRNode->emplaceComponent<TabStop>();
			textBoxR->setText(Format(currentValue.r), IgnoreIsChangedYN::Yes);

			// Gラベル
			const auto gLabelNode = textBoxParentNode->emplaceChild(
				U"GLabel",
				InlineRegion
				{
					.sizeDelta = Vec2{ 16, 26 },
					.flexibleWeight = 0,
				});
			gLabelNode->emplaceComponent<Label>(
				U"G", U"", 12, ColorF{ 0.7, 1.0, 0.7 },
				HorizontalAlign::Center, VerticalAlign::Middle);
			// Gラベルに背景を追加（ホバー時のフィードバック用）
			gLabelNode->emplaceComponent<RectRenderer>(
				PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }),
				Palette::Black, 0.0, 2.0);

			// G
			const auto textBoxGNode = textBoxParentNode->emplaceChild(
				U"TextBoxG",
				InlineRegion
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 1,
					.margin = LRTB{ 0, 4, 0, 0 },
				});
			textBoxGNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"focused", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textBoxG = textBoxGNode->emplaceComponent<TextBox>(
				U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, HorizontalAlign::Left, VerticalAlign::Middle, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textBoxGNode->emplaceComponent<TabStop>();
			textBoxG->setText(Format(currentValue.g), IgnoreIsChangedYN::Yes);

			// Bラベル
			const auto bLabelNode = textBoxParentNode->emplaceChild(
				U"BLabel",
				InlineRegion
				{
					.sizeDelta = Vec2{ 16, 26 },
					.flexibleWeight = 0,
				});
			bLabelNode->emplaceComponent<Label>(
				U"B", U"", 12, ColorF{ 0.7, 0.7, 1.0 },
				HorizontalAlign::Center, VerticalAlign::Middle);
			// Bラベルに背景を追加（ホバー時のフィードバック用）
			bLabelNode->emplaceComponent<RectRenderer>(
				PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }),
				Palette::Black, 0.0, 2.0);

			// B
			const auto textBoxBNode = textBoxParentNode->emplaceChild(
				U"TextBoxB",
				InlineRegion
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 1,
					.margin = LRTB{ 0, 4, 0, 0 },
				});
			textBoxBNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"focused", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textBoxB = textBoxBNode->emplaceComponent<TextBox>(
				U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, HorizontalAlign::Left, VerticalAlign::Middle, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textBoxBNode->emplaceComponent<TabStop>();
			textBoxB->setText(Format(currentValue.b), IgnoreIsChangedYN::Yes);

			// Aラベル
			const auto aLabelNode = textBoxParentNode->emplaceChild(
				U"ALabel",
				InlineRegion
				{
					.sizeDelta = Vec2{ 16, 26 },
					.flexibleWeight = 0,
				});
			aLabelNode->emplaceComponent<Label>(
				U"A", U"", 12, ColorF{ 0.8, 0.8, 0.8 },
				HorizontalAlign::Center, VerticalAlign::Middle);
			// Aラベルに背景を追加（ホバー時のフィードバック用）
			aLabelNode->emplaceComponent<RectRenderer>(
				PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }),
				Palette::Black, 0.0, 2.0);

			// A
			const auto textBoxANode = textBoxParentNode->emplaceChild(
				U"TextBoxA",
				InlineRegion
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 1,
					.margin = LRTB{ 0, 0, 0, 0 },
				});
			textBoxANode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"focused", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textBoxA = textBoxANode->emplaceComponent<TextBox>(
				U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, HorizontalAlign::Left, VerticalAlign::Middle, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textBoxANode->emplaceComponent<TabStop>();
			textBoxA->setText(Format(currentValue.a), IgnoreIsChangedYN::Yes);

			const auto colorPropertyTextBox = std::make_shared<ColorPropertyTextBox>(
				textBoxR,
				textBoxG,
				textBoxB,
				textBoxA,
				previewRectRenderer,
				fnSetValue,
				currentValue);
			propertyNode->addComponent(colorPropertyTextBox);

			// PropertyLabelDraggerを追加
			{
				// Rラベルにドラッグ機能を追加
				rLabelNode->emplaceComponent<PropertyLabelDragger>(
					[colorPropertyTextBox, textBoxR](double value)
					{
						const ColorF currentColor = colorPropertyTextBox->value();
						colorPropertyTextBox->setValue(ColorF{ value, currentColor.g, currentColor.b, currentColor.a }, true);
						textBoxR->setText(Format(value));
					},
					[colorPropertyTextBox]() -> double
					{
						return colorPropertyTextBox->value().r;
					},
					0.01,  // 感度（色は0-1の範囲なので小さめ）
					0.0,   // 最小値
					1.0   // 最大値
				);

				// Gラベルにドラッグ機能を追加
				gLabelNode->emplaceComponent<PropertyLabelDragger>(
					[colorPropertyTextBox, textBoxG](double value)
					{
						const ColorF currentColor = colorPropertyTextBox->value();
						colorPropertyTextBox->setValue(ColorF{ currentColor.r, value, currentColor.b, currentColor.a }, true);
						textBoxG->setText(Format(value));
					},
					[colorPropertyTextBox]() -> double
					{
						return colorPropertyTextBox->value().g;
					},
					0.01,  // 感度
					0.0,   // 最小値
					1.0   // 最大値
				);

				// Bラベルにドラッグ機能を追加
				bLabelNode->emplaceComponent<PropertyLabelDragger>(
					[colorPropertyTextBox, textBoxB](double value)
					{
						const ColorF currentColor = colorPropertyTextBox->value();
						colorPropertyTextBox->setValue(ColorF{ currentColor.r, currentColor.g, value, currentColor.a }, true);
						textBoxB->setText(Format(value));
					},
					[colorPropertyTextBox]() -> double
					{
						return colorPropertyTextBox->value().b;
					},
					0.01,  // 感度
					0.0,   // 最小値
					1.0   // 最大値
				);

				// Aラベルにドラッグ機能を追加
				aLabelNode->emplaceComponent<PropertyLabelDragger>(
					[colorPropertyTextBox, textBoxA](double value)
					{
						const ColorF currentColor = colorPropertyTextBox->value();
						colorPropertyTextBox->setValue(ColorF{ currentColor.r, currentColor.g, currentColor.b, value }, true);
						textBoxA->setText(Format(value));
					},
					[colorPropertyTextBox]() -> double
					{
						return colorPropertyTextBox->value().a;
					},
					0.01,  // 感度
					0.0,   // 最小値
					1.0   // 最大値
				);
			}

			return propertyNode;
		}

		[[nodiscard]]
		static std::shared_ptr<Node> CreateEnumPropertyNode(
			StringView name,
			StringView currentValue,
			std::function<void(StringView)> fnSetValue,
			const std::shared_ptr<ContextMenu>& contextMenu,
			const Array<String>& enumCandidates,
			HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No,
			HasParameterRefYN hasParameterRef = HasParameterRefYN::No)
		{
			const auto propertyNode = Node::Create(
				name,
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 32 },
				},
				IsHitTargetYN::Yes,
				InheritChildrenStateFlags::Hovered);
			propertyNode->setChildrenLayout(HorizontalLayout{ .padding = LRTB{ 10, 8, 0, 0 } });
			propertyNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }), Palette::Black, 0.0, 3.0);

			const auto labelNode =
				propertyNode->emplaceChild(
					U"Label",
					InlineRegion
					{
						.sizeRatio = Vec2{ 0, 1 },
						.flexibleWeight = 0.85,
					});
			labelNode->emplaceComponent<Label>(
				name,
				U"",
				14,
				Palette::White,
				HorizontalAlign::Left,
				VerticalAlign::Middle,
				LRTB{ 5, 5, 5, 5 },
				HorizontalOverflow::Wrap,
				VerticalOverflow::Clip,
				Vec2::Zero(),
				(hasParameterRef || hasInteractivePropertyValue) ? LabelUnderlineStyle::Solid : LabelUnderlineStyle::None,
				hasParameterRef ? ColorF{ Palette::Cyan, 0.5 } : ColorF{ Palette::Yellow, 0.5 },
				2.0,
				LabelSizingMode::AutoShrink);

			const auto comboBoxNode = propertyNode->emplaceChild(
				U"ComboBox",
				InlineRegion
				{
					.sizeDelta = Vec2{ 0, 26 },
					.flexibleWeight = 1,
				});
			comboBoxNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(ColorF{ 1.0, 0.6 }).withSmoothTime(0.05), 1.0, 4.0);

			const auto enumLabel = comboBoxNode->emplaceComponent<Label>(
				currentValue,
				U"",
				14,
				Palette::White,
				HorizontalAlign::Left,
				VerticalAlign::Middle,
				LRTB{ 3, 18, 3, 3 })
				->setSizingMode(LabelSizingMode::AutoShrink);

			comboBoxNode->addComponent(std::make_shared<EnumPropertyComboBox>(
				currentValue,
				fnSetValue,
				enumLabel,
				contextMenu,
				enumCandidates));

			comboBoxNode->emplaceComponent<Label>(
				U"▼",
				U"",
				10,
				Palette::White,
				HorizontalAlign::Right,
				VerticalAlign::Middle,
				LRTB{ 5, 7, 5, 5 });

			return propertyNode;
		}

		[[nodiscard]]
		static std::shared_ptr<Node> CreateCheckboxNode(
			bool initialValue,
			std::function<void(bool)> fnSetValue,
			bool useParentHoverState = false)
		{
			auto checkboxNode = Node::Create(
				U"Checkbox",
				InlineRegion
				{
					.sizeRatio = Vec2{ 0, 0 },
					.sizeDelta = Vec2{ 18, 18 },
				},
				useParentHoverState ? IsHitTargetYN::No : IsHitTargetYN::Yes);

			checkboxNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"focused", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);

			const auto checkLabel = checkboxNode->emplaceComponent<Label>(
				initialValue ? U"✓" : U"",
				U"",
				14,
				Palette::White,
				HorizontalAlign::Center,
				VerticalAlign::Middle);

			checkboxNode->addComponent(std::make_shared<CheckboxToggler>(
				initialValue,
				std::move(fnSetValue),
				checkLabel,
				useParentHoverState));

			return checkboxNode;
		}

		[[nodiscard]]
		static std::shared_ptr<Node> CreateBoolPropertyNode(
			StringView name,
			bool currentValue,
			std::function<void(bool)> fnSetValue,
			HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No,
			HasParameterRefYN hasParameterRef = HasParameterRefYN::No)
		{
			auto propertyNode = Node::Create(
				name,
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 32 },
				});
			propertyNode->setChildrenLayout(HorizontalLayout{ .padding = LRTB{ 10, 8, 0, 0 } });
			propertyNode->emplaceComponent<RectRenderer>(
				PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }),
				Palette::Black,
				0.0,
				3.0);

			const auto labelNode = propertyNode->emplaceChild(
				U"Label",
				InlineRegion
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 0.85,
				},
				IsHitTargetYN::No);
			labelNode->emplaceComponent<Label>(
				name,
				U"",
				14,
				Palette::White,
				HorizontalAlign::Left,
				VerticalAlign::Middle,
				LRTB{ 5, 5, 5, 5 },
				HorizontalOverflow::Overflow,
				VerticalOverflow::Clip,
				Vec2::Zero(),
				(hasParameterRef || hasInteractivePropertyValue) ? LabelUnderlineStyle::Solid : LabelUnderlineStyle::None,
				hasParameterRef ? ColorF{ Palette::Cyan, 0.5 } : ColorF{ Palette::Yellow, 0.5 },
				2.0,
				LabelSizingMode::AutoShrink);

			const auto checkboxParentNode = propertyNode->emplaceChild(
				U"CheckboxParent",
				InlineRegion
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 1,
				},
				IsHitTargetYN::No);
			const auto checkboxNode = CreateCheckboxNode(currentValue, fnSetValue, true);
			checkboxNode->setRegion(
				AnchorRegion
				{
					.anchorMin = Anchor::MiddleRight,
					.anchorMax = Anchor::MiddleRight,
					.posDelta = Vec2{ -6, 0 },
					.sizeDelta = Vec2{ 18, 18 },
					.sizeDeltaPivot = Anchor::MiddleRight,
				});
			checkboxParentNode->addChild(checkboxNode);

			return propertyNode;
		}

		[[nodiscard]]
		std::shared_ptr<Node> createNodeNameNode(const std::shared_ptr<Node>& node)
		{
			const auto nodeNameNode = Node::Create(
				U"NodeName",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 40 },
					.margin = LRTB{ 0, 0, 0, 8 },
				});
			nodeNameNode->setChildrenLayout(HorizontalLayout{ .padding = 6, .verticalAlign = VerticalAlign::Middle });
			nodeNameNode->emplaceComponent<RectRenderer>(ColorF{ 0.3, 0.3 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0);

			// activeSelfのチェックボックス
			const auto activeCheckboxNode = CreateCheckboxNode(node->activeSelfProperty().propertyValue(), [this, node](bool value) { node->setActive(value); m_onChangeNodeActive(); });
			{
				// ツールチップ
				const PropertyKey key{ U"Node", U"activeSelf" };
				if (const auto it = m_propertyMetadata.find(key); it != m_propertyMetadata.end())
				{
					const auto& metadata = it->second;
					if (metadata.tooltip)
					{
						activeCheckboxNode->emplaceComponent<TooltipOpener>(m_editorOverlayCanvas, *metadata.tooltip, metadata.tooltipDetail.value_or(U""));
					}
				}
			}
			{
				// コンテキストメニュー
				Array<MenuElement> menuElements
				{
					MenuItem
					{
						.text = U"参照パラメータを選択...",
						.hotKeyText = U"",
						.mnemonicInput = KeyP,
						.onClick = [this, node]
						{
							m_dialogOpener->openDialog(std::make_shared<ParamRefDialog>(
								&node->activeSelfProperty(),
								m_canvas,
								[this] { refreshInspector(); },
								m_dialogOpener
							));
						},
					},
					MenuItem
					{
						.text = U"参照パラメータをクリア",
						.hotKeyText = U"",
						.mnemonicInput = KeyC,
						.onClick = [node, this]
						{
							node->setActiveSelfParamRef(U"");
							refreshInspector();
						},
						.fnIsEnabled = [node] { return !node->activeSelfParamRef().isEmpty(); },
					},
				};
				
				activeCheckboxNode->emplaceComponent<ContextMenuOpener>(m_contextMenu, menuElements, nullptr, RecursiveYN::Yes);
			}
			// パラメータ参照がある場合は下線を表示
			if (!node->activeSelfParamRef().isEmpty())
			{
				activeCheckboxNode->emplaceChild(
					U"ParamRefUnderline",
					AnchorRegion
					{
						.anchorMin = Anchor::BottomCenter,
						.anchorMax = Anchor::BottomCenter,
						.posDelta = Vec2{ 0, 4 },
						.sizeDelta = Vec2{ 18, 2 },
					},
					IsHitTargetYN::No)
					->emplaceComponent<RectRenderer>(ColorF{ Palette::Aqua, 0.5 });
			}
			nodeNameNode->addChild(activeCheckboxNode);
			
			// Nameテキストボックスを追加
			const auto nameTextboxNode = CreateNodeNameTextboxNode(U"name", node->name(),
				[this, node](StringView value)
				{
					if (value.empty())
					{
						node->setName(U"Node");
					}
					else
					{
						node->setName(value);
					}
					m_onChangeNodeName();
				});
			// Nameテキストボックスにツールチップを追加
			{
				const PropertyKey key{ U"Node", U"name" };
				if (const auto it = m_propertyMetadata.find(key); it != m_propertyMetadata.end())
				{
					const auto& metadata = it->second;
					if (metadata.tooltip)
					{
						// CreateNodeNameTextboxNodeはLabelを含むNodeを返すので、そのLabelを探す
						if (const auto labelNode = nameTextboxNode->getChildByNameOrNull(U"Label", RecursiveYN::Yes))
						{
							labelNode->emplaceComponent<TooltipOpener>(m_editorOverlayCanvas, *metadata.tooltip, metadata.tooltipDetail.value_or(U""));
						}
					}
				}
			}
			nodeNameNode->addChild(nameTextboxNode);

			return nodeNameNode;
		}

		[[nodiscard]]
		std::shared_ptr<Node> createNodeSettingNode(const std::shared_ptr<Node>& node)
		{
			auto nodeSettingNode = Node::Create(
				U"NodeSetting",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.margin = LRTB{ 0, 0, 0, 8 },
				});
			nodeSettingNode->setChildrenLayout(VerticalLayout{ .padding = m_isFoldedNodeSetting ? LRTB::Zero() : LRTB{ 0, 0, 0, 8 } });
			nodeSettingNode->emplaceComponent<RectRenderer>(ColorF{ 0.3, 0.3 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0);

			nodeSettingNode->addChild(CreateHeadingNode(U"Node Settings", ColorF{ 0.5, 0.3, 0.3 }, m_isFoldedNodeSetting,
				[this](IsFoldedYN isFolded)
				{
					m_isFoldedNodeSetting = isFolded;
				}));

			nodeSettingNode->addChild(
				Node::Create(
					U"TopPadding",
					InlineRegion
					{
						.sizeRatio = Vec2{ 1, 0 },
						.sizeDelta = Vec2{ 0, 8 },
					}))->setActive(!m_isFoldedNodeSetting.getBool());

			const auto fnAddBoolChild =
				[this, &nodeSettingNode](StringView name, bool currentValue, auto fnSetValue)
				{
					nodeSettingNode->addChild(createBoolPropertyNodeWithTooltip(U"Node", name, currentValue, fnSetValue))->setActive(!m_isFoldedNodeSetting.getBool());
				};
			const auto fnAddLRTBChild =
				[this, &nodeSettingNode](StringView name, const LRTB& currentValue, auto fnSetValue)
				{
					nodeSettingNode->addChild(createLRTBPropertyNodeWithTooltip(U"Node", name, currentValue, fnSetValue))->setActive(!m_isFoldedNodeSetting.getBool());
				};
			fnAddBoolChild(U"isHitTarget", node->isHitTarget(), [this, node](bool value) { 
				node->setIsHitTarget(value); 
				refreshInspector();
			});
			// isHitTargetがtrueの場合のみhitPaddingを表示
			if (node->isHitTarget())
			{
				fnAddLRTBChild(U"hitPadding", node->hitPadding(), [node](const LRTB& value) { node->setHitPadding(value); });
			}
			fnAddBoolChild(U"inheritChildrenHover", node->inheritChildrenHover(), [node](bool value) { node->setInheritChildrenHover(value); });
			fnAddBoolChild(U"inheritChildrenPress", node->inheritChildrenPress(), [node](bool value) { node->setInheritChildrenPress(value); });
			{
				const auto propertyNode = nodeSettingNode->addChild(createBoolPropertyNodeWithTooltip(U"Node", U"interactable", node->interactable(), [node](bool value) { node->setInteractable(value); }, HasInteractivePropertyValueYN::No, HasParameterRefYN{ !node->interactableParamRef().isEmpty() }));
				propertyNode->setActive(!m_isFoldedNodeSetting.getBool());
				
				Array<MenuElement> menuElements
				{
					MenuItem
					{
						.text = U"参照パラメータを選択...",
						.hotKeyText = U"",
						.mnemonicInput = KeyP,
						.onClick = [this, node]
						{
							m_dialogOpener->openDialog(std::make_shared<ParamRefDialog>(
								&node->interactableProperty(),
								m_canvas,
								[this] { refreshInspector(); },
								m_dialogOpener
							));
						},
					},
					MenuItem
					{
						.text = U"参照パラメータをクリア",
						.hotKeyText = U"",
						.mnemonicInput = KeyC,
						.onClick = [node, this]
						{
							node->setInteractableParamRef(U"");
							refreshInspector();
						},
						.fnIsEnabled = [node] { return !node->interactableParamRef().isEmpty(); },
					},
				};
				
				propertyNode->emplaceComponent<ContextMenuOpener>(m_contextMenu, menuElements, nullptr, RecursiveYN::Yes);
			}
			fnAddBoolChild(U"horizontalScrollable", node->horizontalScrollable(), [node](bool value) { node->setHorizontalScrollable(value); });
			fnAddBoolChild(U"verticalScrollable", node->verticalScrollable(), [node](bool value) { node->setVerticalScrollable(value); });
			fnAddBoolChild(U"wheelScrollEnabled", node->wheelScrollEnabled(), [this, node](bool value) { 
				node->setWheelScrollEnabled(value); 
				refreshInspector();
			});
			fnAddBoolChild(U"dragScrollEnabled", node->dragScrollEnabled(), [this, node](bool value) { 
				node->setDragScrollEnabled(value); 
				refreshInspector();
			});
			// dragScrollEnabledが有効な場合のみ表示
			if (node->dragScrollEnabled())
			{
				const auto fnAddDoubleChild =
					[this, &nodeSettingNode](StringView name, double currentValue, auto fnSetValue)
					{
						nodeSettingNode->addChild(createPropertyNodeWithTooltip(U"Node", name, Format(currentValue), [fnSetValue = std::move(fnSetValue)](StringView value) { fnSetValue(ParseOpt<double>(value).value_or(0.0)); }, HasInteractivePropertyValueYN::No, HasParameterRefYN::No, nullptr))->setActive(!m_isFoldedNodeSetting.getBool());
					};
				fnAddDoubleChild(U"decelerationRate", node->decelerationRate(), [node](double value) { node->setDecelerationRate(Clamp(value, 0.0, 1.0)); });
			}
			// wheelScrollEnabledまたはdragScrollEnabledが有効な場合のみ表示
			if (node->wheelScrollEnabled() || node->dragScrollEnabled())
			{
				fnAddBoolChild(U"rubberBandScrollEnabled", node->rubberBandScrollEnabled(), [node](bool value) { node->setRubberBandScrollEnabled(value); });
			}
			fnAddBoolChild(U"clippingEnabled", node->clippingEnabled(), [node](bool value) { node->setClippingEnabled(value); });
			
			{
				const auto propertyNode = nodeSettingNode->addChild(createPropertyNodeWithTooltip(U"Node", U"styleState", node->styleState(), [node](StringView value) { node->setStyleState(String(value)); }, HasInteractivePropertyValueYN::No, HasParameterRefYN{ !node->styleStateParamRef().isEmpty() }));
				propertyNode->setActive(!m_isFoldedNodeSetting.getBool());
				Array<MenuElement> menuElements
				{
					MenuItem
					{ 
						.text = U"参照パラメータを選択...", 
						.hotKeyText = U"", 
						.mnemonicInput = KeyP, 
						.onClick = [this, node] 
						{ 
							m_dialogOpener->openDialog(std::make_shared<ParamRefDialog>(
								&node->styleStateProperty(), // 参照戻り値のポインタを使っているので注意
								m_canvas,
								[this] { refreshInspector(); },
								m_dialogOpener
							)); 
						},
					},
					MenuItem
					{
						.text = U"参照パラメータをクリア",
						.hotKeyText = U"",
						.mnemonicInput = KeyC,
						.onClick = [node, this]
						{
							node->setStyleStateParamRef(U"");
							refreshInspector();
						},
						.fnIsEnabled = [node] { return !node->styleStateParamRef().isEmpty(); },
					},
				};
				
				propertyNode->emplaceComponent<ContextMenuOpener>(m_contextMenu, menuElements, nullptr, RecursiveYN::Yes);
			}

			nodeSettingNode->setInlineRegionToFitToChildren(FitTarget::HeightOnly);

			return nodeSettingNode;
		}

		enum class LayoutType
		{
			FlowLayout,
			HorizontalLayout,
			VerticalLayout,
		};

		[[nodiscard]]
		std::shared_ptr<Node> createCanvasChildrenLayoutNode()
		{
			auto layoutNode = Node::Create(
				U"CanvasChildrenLayout",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.margin = LRTB{ 0, 0, 0, 8 },
				});
			layoutNode->setChildrenLayout(VerticalLayout{ .padding = m_isFoldedLayout ? LRTB::Zero() : LRTB{ 0, 0, 0, 8 } });
			layoutNode->emplaceComponent<RectRenderer>(ColorF{ 0.3, 0.3 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0);
			layoutNode->addChild(CreateHeadingNode(U"Children Layout", ColorF{ 0.5, 0.3, 0.3 }, m_isFoldedLayout,
				[this](IsFoldedYN isFolded)
				{
					m_isFoldedLayout = isFolded;
				}));
			
			// 現在のLayoutタイプを取得
			String layoutTypeName;
			if (m_canvas->childrenFlowLayout())
			{
				layoutTypeName = U"FlowLayout";
			}
			else if (m_canvas->childrenHorizontalLayout())
			{
				layoutTypeName = U"HorizontalLayout";
			}
			else if (m_canvas->childrenVerticalLayout())
			{
				layoutTypeName = U"VerticalLayout";
			}
			
			const auto fnAddChild =
				[this, &layoutNode, &layoutTypeName](StringView name, const auto& value, auto fnSetValue)
				{
					layoutNode->addChild(createPropertyNodeWithTooltip(layoutTypeName, name, Format(value), fnSetValue))->setActive(!m_isFoldedLayout.getBool());
				};
			const auto fnAddVec2Child =
				[this, &layoutNode, &layoutTypeName](StringView name, const Vec2& currentValue, auto fnSetValue)
				{
					layoutNode->addChild(createVec2PropertyNodeWithTooltip(layoutTypeName, name, currentValue, fnSetValue))->setActive(!m_isFoldedLayout.getBool());
				};
			const auto fnAddDoubleChild =
				[this, &layoutNode, &layoutTypeName](StringView name, double currentValue, auto fnSetValue)
				{
					layoutNode->addChild(createPropertyNodeWithTooltip(layoutTypeName, name, Format(currentValue), [fnSetValue = std::move(fnSetValue)](StringView value) { fnSetValue(ParseOpt<double>(value).value_or(0.0)); }, HasInteractivePropertyValueYN::No, HasParameterRefYN::No, nullptr))->setActive(!m_isFoldedLayout.getBool());
				};
			const auto fnAddLRTBChild =
				[this, &layoutNode, &layoutTypeName](StringView name, const LRTB& currentValue, auto fnSetValue)
				{
					layoutNode->addChild(createLRTBPropertyNodeWithTooltip(layoutTypeName, name, currentValue, fnSetValue))->setActive(!m_isFoldedLayout.getBool());
				};
			const auto fnAddEnumChild =
				[this, &layoutNode, &layoutTypeName]<typename EnumType>(const String & name, EnumType currentValue, auto fnSetValue)
				{
					auto fnSetEnumValue = [fnSetValue = std::move(fnSetValue), currentValue](StringView value) { fnSetValue(StringToEnum<EnumType>(value, currentValue)); };
					layoutNode->addChild(createEnumPropertyNodeWithTooltip(layoutTypeName, name, EnumToString(currentValue), fnSetEnumValue, m_contextMenu, EnumNames<EnumType>()))->setActive(!m_isFoldedLayout.getBool());
				};
			
			if (const auto pFlowLayout = m_canvas->childrenFlowLayout())
			{
				fnAddEnumChild(
					U"type",
					LayoutType::FlowLayout,
					[this](LayoutType type)
					{
						switch (type)
						{
						case LayoutType::FlowLayout:
							break;
						case LayoutType::HorizontalLayout:
							m_canvas->setChildrenLayout(HorizontalLayout{});
							refreshInspector(); // 項目に変更があるため更新
							break;
						case LayoutType::VerticalLayout:
							m_canvas->setChildrenLayout(VerticalLayout{});
							refreshInspector(); // 項目に変更があるため更新
							break;
						}
					});
				fnAddLRTBChild(U"padding", pFlowLayout->padding, [this](const LRTB& value) { auto newLayout = *m_canvas->childrenFlowLayout(); newLayout.padding = value; m_canvas->setChildrenLayout(newLayout); });
				fnAddVec2Child(U"spacing", pFlowLayout->spacing, [this](const Vec2& value) { auto newLayout = *m_canvas->childrenFlowLayout(); newLayout.spacing = value; m_canvas->setChildrenLayout(newLayout); });
				fnAddEnumChild(U"horizontalAlign", pFlowLayout->horizontalAlign, [this](HorizontalAlign value) { auto newLayout = *m_canvas->childrenFlowLayout(); newLayout.horizontalAlign = value; m_canvas->setChildrenLayout(newLayout); });
				fnAddEnumChild(U"verticalAlign", pFlowLayout->verticalAlign, [this](VerticalAlign value) { auto newLayout = *m_canvas->childrenFlowLayout(); newLayout.verticalAlign = value; m_canvas->setChildrenLayout(newLayout); });
			}
			else if (const auto pHorizontalLayout = m_canvas->childrenHorizontalLayout())
			{
				fnAddEnumChild(
					U"type",
					LayoutType::HorizontalLayout,
					[this](LayoutType type)
					{
						switch (type)
						{
						case LayoutType::FlowLayout:
							m_canvas->setChildrenLayout(FlowLayout{});
							refreshInspector(); // 項目に変更があるため更新
							break;
						case LayoutType::HorizontalLayout:
							break;
						case LayoutType::VerticalLayout:
							m_canvas->setChildrenLayout(VerticalLayout{});
							refreshInspector(); // 項目に変更があるため更新
							break;
						}
					});
				fnAddLRTBChild(U"padding", pHorizontalLayout->padding, [this](const LRTB& value) { auto newLayout = *m_canvas->childrenHorizontalLayout(); newLayout.padding = value; m_canvas->setChildrenLayout(newLayout); });
				fnAddDoubleChild(U"spacing", pHorizontalLayout->spacing, [this](double value) { auto newLayout = *m_canvas->childrenHorizontalLayout(); newLayout.spacing = value; m_canvas->setChildrenLayout(newLayout); });
				fnAddEnumChild(U"horizontalAlign", pHorizontalLayout->horizontalAlign, [this](HorizontalAlign value) { auto newLayout = *m_canvas->childrenHorizontalLayout(); newLayout.horizontalAlign = value; m_canvas->setChildrenLayout(newLayout); });
				fnAddEnumChild(U"verticalAlign", pHorizontalLayout->verticalAlign, [this](VerticalAlign value) { auto newLayout = *m_canvas->childrenHorizontalLayout(); newLayout.verticalAlign = value; m_canvas->setChildrenLayout(newLayout); });
			}
			else if (const auto pVerticalLayout = m_canvas->childrenVerticalLayout())
			{
				fnAddEnumChild(
					U"type",
					LayoutType::VerticalLayout,
					[this](LayoutType type)
					{
						switch (type)
						{
						case LayoutType::FlowLayout:
							m_canvas->setChildrenLayout(FlowLayout{});
							refreshInspector(); // 項目に変更があるため更新
							break;
						case LayoutType::HorizontalLayout:
							m_canvas->setChildrenLayout(HorizontalLayout{});
							refreshInspector(); // 項目に変更があるため更新
							break;
						case LayoutType::VerticalLayout:
							break;
						}
					});
				fnAddLRTBChild(U"padding", pVerticalLayout->padding, [this](const LRTB& value) { auto newLayout = *m_canvas->childrenVerticalLayout(); newLayout.padding = value; m_canvas->setChildrenLayout(newLayout); });
				fnAddDoubleChild(U"spacing", pVerticalLayout->spacing, [this](double value) { auto newLayout = *m_canvas->childrenVerticalLayout(); newLayout.spacing = value; m_canvas->setChildrenLayout(newLayout); });
				fnAddEnumChild(U"horizontalAlign", pVerticalLayout->horizontalAlign, [this](HorizontalAlign value) { auto newLayout = *m_canvas->childrenVerticalLayout(); newLayout.horizontalAlign = value; m_canvas->setChildrenLayout(newLayout); });
				fnAddEnumChild(U"verticalAlign", pVerticalLayout->verticalAlign, [this](VerticalAlign value) { auto newLayout = *m_canvas->childrenVerticalLayout(); newLayout.verticalAlign = value; m_canvas->setChildrenLayout(newLayout); });
			}
			else
			{
				throw Error{ U"Unknown layout type" };
			}

			layoutNode->setInlineRegionToFitToChildren(FitTarget::HeightOnly);

			return layoutNode;
		}

		[[nodiscard]]
		std::shared_ptr<Node> createChildrenLayoutNode(const std::shared_ptr<Node>& node)
		{
			auto layoutNode = Node::Create(
				U"ChildrenLayout",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.margin = LRTB{ 0, 0, 0, 8 },
				});
			layoutNode->setChildrenLayout(VerticalLayout{ .padding = m_isFoldedLayout ? LRTB::Zero() : LRTB{ 0, 0, 0, 8 } });
			layoutNode->emplaceComponent<RectRenderer>(ColorF{ 0.3, 0.3 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0);
			layoutNode->addChild(CreateHeadingNode(U"Children Layout", ColorF{ 0.5, 0.3, 0.3 }, m_isFoldedLayout,
				[this](IsFoldedYN isFolded)
				{
					m_isFoldedLayout = isFolded;
				}));
			// 現在のLayoutタイプを取得
			String layoutTypeName;
			if (node->childrenFlowLayout())
			{
				layoutTypeName = U"FlowLayout";
			}
			else if (node->childrenHorizontalLayout())
			{
				layoutTypeName = U"HorizontalLayout";
			}
			else if (node->childrenVerticalLayout())
			{
				layoutTypeName = U"VerticalLayout";
			}
			
			const auto fnAddChild =
				[this, &layoutNode, &layoutTypeName](StringView name, const auto& value, auto fnSetValue)
				{
					layoutNode->addChild(createPropertyNodeWithTooltip(layoutTypeName, name, Format(value), fnSetValue))->setActive(!m_isFoldedLayout.getBool());
				};
			const auto fnAddVec2Child =
				[this, &layoutNode, &layoutTypeName](StringView name, const Vec2& currentValue, auto fnSetValue)
				{
					layoutNode->addChild(createVec2PropertyNodeWithTooltip(layoutTypeName, name, currentValue, fnSetValue))->setActive(!m_isFoldedLayout.getBool());
				};
			const auto fnAddDoubleChild =
				[this, &layoutNode, &layoutTypeName](StringView name, double currentValue, auto fnSetValue)
				{
					layoutNode->addChild(createPropertyNodeWithTooltip(layoutTypeName, name, Format(currentValue), [fnSetValue = std::move(fnSetValue)](StringView value) { fnSetValue(ParseOpt<double>(value).value_or(0.0)); }, HasInteractivePropertyValueYN::No, HasParameterRefYN::No, nullptr))->setActive(!m_isFoldedLayout.getBool());
				};
			const auto fnAddLRTBChild =
				[this, &layoutNode, &layoutTypeName](StringView name, const LRTB& currentValue, auto fnSetValue)
				{
					layoutNode->addChild(createLRTBPropertyNodeWithTooltip(layoutTypeName, name, currentValue, fnSetValue))->setActive(!m_isFoldedLayout.getBool());
				};
			const auto fnAddEnumChild =
				[this, &layoutNode, &layoutTypeName]<typename EnumType>(const String & name, EnumType currentValue, auto fnSetValue)
				{
					auto fnSetEnumValue = [fnSetValue = std::move(fnSetValue), currentValue](StringView value) { fnSetValue(StringToEnum<EnumType>(value, currentValue)); };
					layoutNode->addChild(createEnumPropertyNodeWithTooltip(layoutTypeName, name, EnumToString(currentValue), fnSetEnumValue, m_contextMenu, EnumNames<EnumType>()))->setActive(!m_isFoldedLayout.getBool());
				};
			if (const auto pFlowLayout = node->childrenFlowLayout())
			{
				fnAddEnumChild(
					U"type",
					LayoutType::FlowLayout,
					[this, node](LayoutType type)
					{
						switch (type)
						{
						case LayoutType::FlowLayout:
							break;
						case LayoutType::HorizontalLayout:
							node->setChildrenLayout(HorizontalLayout{});
							refreshInspector(); // 項目に変更があるため更新
							break;
						case LayoutType::VerticalLayout:
							node->setChildrenLayout(VerticalLayout{});
							refreshInspector(); // 項目に変更があるため更新
							break;
						}
					});
				fnAddLRTBChild(U"padding", pFlowLayout->padding, [this, node](const LRTB& value) { auto newLayout = *node->childrenFlowLayout(); newLayout.padding = value; node->setChildrenLayout(newLayout); });
				fnAddVec2Child(U"spacing", pFlowLayout->spacing, [this, node](const Vec2& value) { auto newLayout = *node->childrenFlowLayout(); newLayout.spacing = value; node->setChildrenLayout(newLayout); });
				fnAddEnumChild(U"horizontalAlign", pFlowLayout->horizontalAlign, [this, node](HorizontalAlign value) { auto newLayout = *node->childrenFlowLayout(); newLayout.horizontalAlign = value; node->setChildrenLayout(newLayout); });
				fnAddEnumChild(U"verticalAlign", pFlowLayout->verticalAlign, [this, node](VerticalAlign value) { auto newLayout = *node->childrenFlowLayout(); newLayout.verticalAlign = value; node->setChildrenLayout(newLayout); });
			}
			else if (const auto pHorizontalLayout = node->childrenHorizontalLayout())
			{
				fnAddEnumChild(
					U"type",
					LayoutType::HorizontalLayout,
					[this, node](LayoutType type)
					{
						switch (type)
						{
						case LayoutType::FlowLayout:
							node->setChildrenLayout(FlowLayout{});
							refreshInspector(); // 項目に変更があるため更新
							break;
						case LayoutType::HorizontalLayout:
							break;
						case LayoutType::VerticalLayout:
							node->setChildrenLayout(VerticalLayout{});
							refreshInspector(); // 項目に変更があるため更新
							break;
						}
					});
				fnAddLRTBChild(U"padding", pHorizontalLayout->padding, [this, node](const LRTB& value) { auto newLayout = *node->childrenHorizontalLayout(); newLayout.padding = value; node->setChildrenLayout(newLayout); });
				fnAddDoubleChild(U"spacing", pHorizontalLayout->spacing, [this, node](double value) { auto newLayout = *node->childrenHorizontalLayout(); newLayout.spacing = value; node->setChildrenLayout(newLayout); });
				fnAddEnumChild(U"horizontalAlign", pHorizontalLayout->horizontalAlign, [this, node](HorizontalAlign value) { auto newLayout = *node->childrenHorizontalLayout(); newLayout.horizontalAlign = value; node->setChildrenLayout(newLayout); });
				fnAddEnumChild(U"verticalAlign", pHorizontalLayout->verticalAlign, [this, node](VerticalAlign value) { auto newLayout = *node->childrenHorizontalLayout(); newLayout.verticalAlign = value; node->setChildrenLayout(newLayout); });
			}
			else if (const auto pVerticalLayout = node->childrenVerticalLayout())
			{
				fnAddEnumChild(
					U"type",
					LayoutType::VerticalLayout,
					[this, node](LayoutType type)
					{
						switch (type)
						{
						case LayoutType::FlowLayout:
							node->setChildrenLayout(FlowLayout{});
							refreshInspector(); // 項目に変更があるため更新
							break;
						case LayoutType::HorizontalLayout:
							node->setChildrenLayout(HorizontalLayout{});
							refreshInspector(); // 項目に変更があるため更新
							break;
						case LayoutType::VerticalLayout:
							break;
						}
					});
				fnAddLRTBChild(U"padding", pVerticalLayout->padding, [this, node](const LRTB& value) { auto newLayout = *node->childrenVerticalLayout(); newLayout.padding = value; node->setChildrenLayout(newLayout); });
				fnAddDoubleChild(U"spacing", pVerticalLayout->spacing, [this, node](double value) { auto newLayout = *node->childrenVerticalLayout(); newLayout.spacing = value; node->setChildrenLayout(newLayout); });
				fnAddEnumChild(U"horizontalAlign", pVerticalLayout->horizontalAlign, [this, node](HorizontalAlign value) { auto newLayout = *node->childrenVerticalLayout(); newLayout.horizontalAlign = value; node->setChildrenLayout(newLayout); });
				fnAddEnumChild(U"verticalAlign", pVerticalLayout->verticalAlign, [this, node](VerticalAlign value) { auto newLayout = *node->childrenVerticalLayout(); newLayout.verticalAlign = value; node->setChildrenLayout(newLayout); });
			}
			else
			{
				throw Error{ U"Unknown layout type" };
			}

			layoutNode->setInlineRegionToFitToChildren(FitTarget::HeightOnly);

			return layoutNode;
		}

		[[nodiscard]]
		std::shared_ptr<Node> createRegionNode(const std::shared_ptr<Node>& node)
		{
			auto regionNode = Node::Create(
				U"Region",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.margin = LRTB{ 0, 0, 0, 8 },
				});
			regionNode->setChildrenLayout(VerticalLayout{ .padding = m_isFoldedRegion ? LRTB::Zero() : LRTB{ 0, 0, 0, 8 } });
			regionNode->emplaceComponent<RectRenderer>(ColorF{ 0.3, 0.3 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0);

			regionNode->addChild(CreateHeadingNode(U"Region", ColorF{ 0.5, 0.3, 0.3 }, m_isFoldedRegion,
				[this](IsFoldedYN isFolded)
				{
					m_isFoldedRegion = isFolded;
				}));

			// 現在のRegionタイプを取得
			const String regionTypeName = node->inlineRegion() ? U"InlineRegion" : U"AnchorRegion";
			
			const auto fnAddChild =
				[this, &regionNode, &regionTypeName](StringView name, const auto& value, auto fnSetValue)
				{
					regionNode->addChild(createPropertyNodeWithTooltip(regionTypeName, name, Format(value), fnSetValue))->setActive(!m_isFoldedRegion.getBool());
				};
			const auto fnAddDoubleChild =
				[this, &regionNode, &regionTypeName](StringView name, double currentValue, auto fnSetValue)
				{
					regionNode->addChild(createPropertyNodeWithTooltip(regionTypeName, name, Format(currentValue), [fnSetValue = std::move(fnSetValue)](StringView value) { fnSetValue(ParseOpt<double>(value).value_or(0.0)); }, HasInteractivePropertyValueYN::No, HasParameterRefYN::No, nullptr))->setActive(!m_isFoldedRegion.getBool());
				};
			const auto fnAddEnumChild =
				[this, &regionNode, &regionTypeName]<typename EnumType>(const String & name, EnumType currentValue, auto fnSetValue)
				{
					auto fnSetEnumValue = [fnSetValue = std::move(fnSetValue), currentValue](StringView value) { fnSetValue(StringToEnum<EnumType>(value, currentValue)); };
					regionNode->addChild(createEnumPropertyNodeWithTooltip(regionTypeName, name, EnumToString(currentValue), fnSetEnumValue, m_contextMenu, EnumNames<EnumType>()))->setActive(!m_isFoldedRegion.getBool());
				};
			const auto fnAddVec2Child =
				[this, &regionNode, &regionTypeName](StringView name, const Vec2& currentValue, auto fnSetValue)
				{
					regionNode->addChild(createVec2PropertyNodeWithTooltip(regionTypeName, name, currentValue, fnSetValue))->setActive(!m_isFoldedRegion.getBool());
				};
			const auto fnAddLRTBChild =
				[this, &regionNode, &regionTypeName](StringView name, const LRTB& currentValue, auto fnSetValue)
				{
					regionNode->addChild(createLRTBPropertyNodeWithTooltip(regionTypeName, name, currentValue, fnSetValue))->setActive(!m_isFoldedRegion.getBool());
				};
			const auto fnAddOptionalDoubleChild =
				[this, &regionNode, &regionTypeName](StringView name, const Optional<double>& currentValue, auto fnSetValue)
				{
					const auto propertyNode = Node::Create(
						name,
						InlineRegion
						{
							.sizeRatio = Vec2{ 1, 0 },
							.sizeDelta = Vec2{ 0, 32 },
						},
						IsHitTargetYN::Yes,
						InheritChildrenStateFlags::Hovered);
					propertyNode->setChildrenLayout(HorizontalLayout{ .padding = LRTB{ 10, 8, 0, 0 } });
					propertyNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }), Palette::Black, 0.0, 3.0);
					
					// ラベル領域（チェックボックスを含む）
					const auto labelNode = propertyNode->emplaceChild(
						U"Label",
						InlineRegion
						{
							.sizeRatio = Vec2{ 0, 1 },
							.flexibleWeight = 0.85,
						},
						IsHitTargetYN::Yes,
						InheritChildrenStateFlags::Hovered);
					labelNode->setChildrenLayout(HorizontalLayout{ .verticalAlign = VerticalAlign::Middle });
					
					// チェックボックスは後で追加（textBoxとtextBoxNodeを参照するため）
					
					// プロパティ名
					labelNode->emplaceComponent<Label>(
						name,
						U"",
						14,
						Palette::White,
						HorizontalAlign::Left,
						VerticalAlign::Middle,
						LRTB{ 18 + 4, 5, 5, 5 },
						HorizontalOverflow::Wrap,
						VerticalOverflow::Clip)
						->setSizingMode(LabelSizingMode::AutoShrink);
					
					// メタデータに基づいてツールチップを追加
					if (const auto it = m_propertyMetadata.find(PropertyKey{ String{ regionTypeName }, String{ name } }); it != m_propertyMetadata.end())
					{
						const auto& metadata = it->second;
						if (metadata.tooltip)
						{
							labelNode->emplaceComponent<TooltipOpener>(m_editorOverlayCanvas, *metadata.tooltip, metadata.tooltipDetail.value_or(U""));
						}
					}
					
					// 初期値設定
					const bool hasValue = currentValue.has_value();
					const auto hasValueShared = std::make_shared<bool>(hasValue);
					
					// テキストボックス
					const auto textBoxNode = propertyNode->emplaceChild(
						U"TextBox",
						InlineRegion
						{
							.sizeDelta = Vec2{ 0, 26 },
							.flexibleWeight = 1,
						});
					textBoxNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"focused", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
					const auto textBox = textBoxNode->emplaceComponent<TextBox>(U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, HorizontalAlign::Left, VerticalAlign::Middle, Palette::White, ColorF{ Palette::Orange, 0.5 });
					textBox->setText(Format(currentValue.value_or(0.0)), IgnoreIsChangedYN::Yes);
					textBoxNode->setInteractable(hasValue ? InteractableYN::Yes : InteractableYN::No);
					
					// チェックボックス（親のホバー状態を使用）
					const auto checkboxNode = CreateCheckboxNode(hasValue, 
						[hasValueShared, textBox, fnSetValue, textBoxNode](bool newValue)
						{
							*hasValueShared = newValue;
							textBoxNode->setInteractable(newValue ? InteractableYN::Yes : InteractableYN::No);
							if (newValue)
							{
								if (auto value = ParseOpt<double>(textBox->text()))
								{
									fnSetValue(*value);
								}
							}
							else
							{
								fnSetValue(none);
							}
						}, true);  // useParentHoverState = true
					checkboxNode->setRegion(InlineRegion
					{
						.sizeDelta = Vec2{ 18, 18 },
						.margin = LRTB{ 0, 4, 0, 0 },
					});
					labelNode->addChild(checkboxNode);
					
					// PropertyTextBoxでテキストボックスの変更を処理
					textBoxNode->addComponent(std::make_shared<PropertyTextBox>(textBox, 
						[hasValueShared, fnSetValue](StringView text)
						{
							if (*hasValueShared)
							{
								if (auto value = ParseOpt<double>(text))
								{
									fnSetValue(*value);
								}
							}
						}));
					
					propertyNode->setActive(!m_isFoldedRegion.getBool());
					regionNode->addChild(propertyNode);
				};

			if (const auto pInlineRegion = node->inlineRegion())
			{
				fnAddEnumChild(
					U"type",
					RegionType::InlineRegion,
					[this, node](RegionType type)
					{
						switch (type)
						{
						case RegionType::AnchorRegion:
							node->setRegion(AnchorRegion
							{
								.anchorMin = Anchor::MiddleCenter,
								.anchorMax = Anchor::MiddleCenter,
								.posDelta = Vec2::Zero(),
								.sizeDelta = node->regionRect().size,
								.sizeDeltaPivot = Vec2{ 0.5, 0.5 },
							});
							m_defaults->regionType = RegionType::AnchorRegion; // 次回のデフォルト値として記憶
							refreshInspector(); // 項目に変更があるため更新
							break;
						case RegionType::InlineRegion:
							break;
						}
					});
				fnAddVec2Child(U"sizeRatio", pInlineRegion->sizeRatio, [this, node](const Vec2& value) { auto newRegion = *node->inlineRegion(); newRegion.sizeRatio = value; node->setRegion(newRegion); });
				fnAddVec2Child(U"sizeDelta", pInlineRegion->sizeDelta, [this, node](const Vec2& value) { auto newRegion = *node->inlineRegion(); newRegion.sizeDelta = value; node->setRegion(newRegion); });
				fnAddDoubleChild(U"flexibleWeight", pInlineRegion->flexibleWeight, [this, node](double value) { auto newRegion = *node->inlineRegion(); newRegion.flexibleWeight = value; node->setRegion(newRegion); });
				fnAddLRTBChild(U"margin", pInlineRegion->margin, [this, node](const LRTB& value) { auto newRegion = *node->inlineRegion(); newRegion.margin = value; node->setRegion(newRegion); });
				
				fnAddOptionalDoubleChild(U"minWidth", pInlineRegion->minWidth,
					[this, node](const Optional<double>& value) { auto newRegion = *node->inlineRegion(); newRegion.minWidth = value; node->setRegion(newRegion); });
				fnAddOptionalDoubleChild(U"minHeight", pInlineRegion->minHeight,
					[this, node](const Optional<double>& value) { auto newRegion = *node->inlineRegion(); newRegion.minHeight = value; node->setRegion(newRegion); });
				fnAddOptionalDoubleChild(U"maxWidth", pInlineRegion->maxWidth,
					[this, node](const Optional<double>& value) { auto newRegion = *node->inlineRegion(); newRegion.maxWidth = value; node->setRegion(newRegion); });
				fnAddOptionalDoubleChild(U"maxHeight", pInlineRegion->maxHeight,
					[this, node](const Optional<double>& value) { auto newRegion = *node->inlineRegion(); newRegion.maxHeight = value; node->setRegion(newRegion); });
			}
			else if (const auto pAnchorRegion = node->anchorRegion())
			{
				auto setDouble =
					[this, node](auto setter)
					{
						return
							[this, node, setter](StringView s)
							{
								if (auto optVal = ParseOpt<double>(s))
								{
									if (auto ac = node->anchorRegion())
									{
										auto copy = *ac;
										setter(copy, *optVal);
										node->setRegion(copy);
										m_canvas->refreshLayoutImmediately();
									}
								}
							};
					};
				auto setVec2 =
					[this, node](auto setter)
					{
						return
							[this, node, setter](const Vec2& val)
							{
								if (auto ac = node->anchorRegion())
								{
									auto copy = *ac;
									setter(copy, val);
									node->setRegion(copy);
									m_canvas->refreshLayoutImmediately();
								}
							};
					};

				fnAddEnumChild(
					U"type",
					RegionType::AnchorRegion,
					[this, node](RegionType type)
					{
						switch (type)
						{
						case RegionType::AnchorRegion:
							break;
						case RegionType::InlineRegion:
							node->setRegion(InlineRegion
							{
								.sizeRatio = Vec2::Zero(),
								.sizeDelta = node->regionRect().size,
							});
							m_defaults->regionType = RegionType::InlineRegion; // 次回のデフォルト値として記憶
							refreshInspector(); // 項目に変更があるため更新
							break;
						}
					}
				);

				const AnchorPreset anchorPreset =
					pAnchorRegion->isCustomAnchorInEditor
						? AnchorPreset::Custom
						: ToAnchorPreset(pAnchorRegion->anchorMin, pAnchorRegion->anchorMax, pAnchorRegion->sizeDeltaPivot);

				fnAddEnumChild(
					U"anchor",
					anchorPreset,
					[this, node](AnchorPreset preset)
					{
						if (const auto pAnchorRegion = node->anchorRegion())
						{
							auto copy = *pAnchorRegion;
							if (const auto tuple = FromAnchorPreset(preset))
							{
								// プリセットを選んだ場合
								std::tie(copy.anchorMin, copy.anchorMax, copy.sizeDeltaPivot) = *tuple;
								copy.isCustomAnchorInEditor = false;
							}
							else
							{
								// Customを選んだ場合
								copy.isCustomAnchorInEditor = true;
							}

							// 変更がある場合のみ更新
							if (copy != *pAnchorRegion)
							{
								if (!copy.isCustomAnchorInEditor)
								{
									const auto beforePreset = ToAnchorPreset(pAnchorRegion->anchorMin, pAnchorRegion->anchorMax, pAnchorRegion->sizeDeltaPivot);

									// 横ストレッチに変更した場合はleftとrightを0にする
									const auto fnIsHorizontalStretch = [](AnchorPreset preset)
										{
											return preset == AnchorPreset::StretchTop ||
												preset == AnchorPreset::StretchMiddle ||
												preset == AnchorPreset::StretchBottom ||
												preset == AnchorPreset::StretchFull;
										};
									if (!fnIsHorizontalStretch(beforePreset) && fnIsHorizontalStretch(preset))
									{
										copy.posDelta.x = 0;
										copy.sizeDelta.x = 0;
									}

									// 縦ストレッチに変更した場合はtopとbottomを0にする
									const auto fnIsVerticalStretch = [](AnchorPreset preset)
										{
											return preset == AnchorPreset::StretchLeft ||
												preset == AnchorPreset::StretchCenter ||
												preset == AnchorPreset::StretchRight ||
												preset == AnchorPreset::StretchFull;
										};
									if (!fnIsVerticalStretch(beforePreset) && fnIsVerticalStretch(preset))
									{
										copy.posDelta.y = 0;
										copy.sizeDelta.y = 0;
									}
								}

								node->setRegion(copy);
								m_canvas->refreshLayoutImmediately();
								refreshInspector(); // 項目に変更があるため更新
							}
						}
					}
				);
				switch (anchorPreset)
				{
				case AnchorPreset::TopLeft:
					fnAddChild(U"top", pAnchorRegion->posDelta.y, setDouble([](AnchorRegion& c, double v) { c.posDelta.y = v; }));
					fnAddChild(U"left", pAnchorRegion->posDelta.x, setDouble([](AnchorRegion& c, double v) { c.posDelta.x = v; }));
					fnAddVec2Child(U"size", pAnchorRegion->sizeDelta, setVec2([](AnchorRegion& c, const Vec2& v) { c.sizeDelta = v; }));
					break;

				case AnchorPreset::TopCenter:
					fnAddChild(U"top", pAnchorRegion->posDelta.y, setDouble([](AnchorRegion& c, double v) { c.posDelta.y = v; }));
					fnAddVec2Child(U"size", pAnchorRegion->sizeDelta, setVec2([](AnchorRegion& c, const Vec2& v) { c.sizeDelta = v; }));
					fnAddChild(U"xDelta", pAnchorRegion->posDelta.x, setDouble([](AnchorRegion& c, double v) { c.posDelta.x = v; }));
					break;

				case AnchorPreset::TopRight:
					fnAddChild(U"top", pAnchorRegion->posDelta.y, setDouble([](AnchorRegion& c, double v) { c.posDelta.y = v; }));
					fnAddChild(U"right", -pAnchorRegion->posDelta.x, setDouble([](AnchorRegion& c, double v) { c.posDelta.x = -v; }));
					fnAddVec2Child(U"size", pAnchorRegion->sizeDelta, setVec2([](AnchorRegion& c, const Vec2& v) { c.sizeDelta = v; }));
					break;

				case AnchorPreset::MiddleLeft:
					fnAddChild(U"left", pAnchorRegion->posDelta.x, setDouble([](AnchorRegion& c, double v) { c.posDelta.x = v; }));
					fnAddVec2Child(U"size", pAnchorRegion->sizeDelta, setVec2([](AnchorRegion& c, const Vec2& v) { c.sizeDelta = v; }));
					fnAddChild(U"yDelta", pAnchorRegion->posDelta.y, setDouble([](AnchorRegion& c, double v) { c.posDelta.y = v; }));
					break;

				case AnchorPreset::MiddleCenter:
					fnAddVec2Child(U"size", pAnchorRegion->sizeDelta, setVec2([](AnchorRegion& c, const Vec2& v) { c.sizeDelta = v; }));
					fnAddVec2Child(U"posDelta", pAnchorRegion->posDelta, setVec2([](AnchorRegion& c, const Vec2& v) { c.posDelta = v; }));
					break;

				case AnchorPreset::MiddleRight:
					fnAddChild(U"right", -pAnchorRegion->posDelta.x, setDouble([](AnchorRegion& c, double v) { c.posDelta.x = -v; }));
					fnAddVec2Child(U"size", pAnchorRegion->sizeDelta, setVec2([](AnchorRegion& c, const Vec2& v) { c.sizeDelta = v; }));
					fnAddChild(U"yDelta", pAnchorRegion->posDelta.y, setDouble([](AnchorRegion& c, double v) { c.posDelta.y = v; }));
					break;

				case AnchorPreset::BottomLeft:
					fnAddChild(U"left", pAnchorRegion->posDelta.x, setDouble([](AnchorRegion& c, double v) { c.posDelta.x = v; }));
					fnAddChild(U"bottom", -pAnchorRegion->posDelta.y, setDouble([](AnchorRegion& c, double v) { c.posDelta.y = -v; }));
					fnAddVec2Child(U"size", pAnchorRegion->sizeDelta, setVec2([](AnchorRegion& c, const Vec2& v) { c.sizeDelta = v; }));
					break;

				case AnchorPreset::BottomCenter:
					fnAddChild(U"bottom", -pAnchorRegion->posDelta.y, setDouble([](AnchorRegion& c, double v) { c.posDelta.y = -v; }));
					fnAddVec2Child(U"size", pAnchorRegion->sizeDelta, setVec2([](AnchorRegion& c, const Vec2& v) { c.sizeDelta = v; }));
					fnAddChild(U"xDelta", pAnchorRegion->posDelta.x, setDouble([](AnchorRegion& c, double v) { c.posDelta.x = v; }));
					break;

				case AnchorPreset::BottomRight:
					fnAddChild(U"right", -pAnchorRegion->posDelta.x, setDouble([](AnchorRegion& c, double v) { c.posDelta.x = -v; }));
					fnAddChild(U"bottom", -pAnchorRegion->posDelta.y, setDouble([](AnchorRegion& c, double v) { c.posDelta.y = -v; }));
					fnAddVec2Child(U"size", pAnchorRegion->sizeDelta, setVec2([](AnchorRegion& c, const Vec2& v) { c.sizeDelta = v; }));
					break;

				case AnchorPreset::StretchTop:
					fnAddChild(U"top", pAnchorRegion->posDelta.y, setDouble([](AnchorRegion& c, double v) { c.posDelta.y = v; }));
					fnAddChild(U"left", pAnchorRegion->posDelta.x,
						setDouble([pAnchorRegion](AnchorRegion& c, double v)
							{
								double oldLeft = pAnchorRegion->posDelta.x;
								double delta = oldLeft - v;
								c.posDelta.x = v;
								c.sizeDelta.x += delta;
							}));
					fnAddChild(U"right", -(pAnchorRegion->posDelta.x + pAnchorRegion->sizeDelta.x),
						setDouble([pAnchorRegion](AnchorRegion& c, double v)
							{
								double oldRight = -(pAnchorRegion->posDelta.x + pAnchorRegion->sizeDelta.x);
								double delta = v - oldRight;
								c.sizeDelta.x -= delta;
							}));
					fnAddChild(U"height", pAnchorRegion->sizeDelta.y, setDouble([](AnchorRegion& c, double v) { c.sizeDelta.y = v; }));
					fnAddOptionalDoubleChild(U"minWidth", pAnchorRegion->minWidth, 
						[this, node](const Optional<double>& v) { auto newRegion = *node->anchorRegion(); newRegion.minWidth = v; node->setRegion(newRegion); });
					fnAddOptionalDoubleChild(U"maxWidth", pAnchorRegion->maxWidth, 
						[this, node](const Optional<double>& v) { auto newRegion = *node->anchorRegion(); newRegion.maxWidth = v; node->setRegion(newRegion); });
					break;

				case AnchorPreset::StretchMiddle:
					fnAddChild(U"left", pAnchorRegion->posDelta.x,
						setDouble([pAnchorRegion](AnchorRegion& c, double v)
							{
								double oldLeft = pAnchorRegion->posDelta.x;
								double delta = oldLeft - v;
								c.posDelta.x = v;
								c.sizeDelta.x += delta;
							}));
					fnAddChild(U"right", -(pAnchorRegion->posDelta.x + pAnchorRegion->sizeDelta.x),
						setDouble([pAnchorRegion](AnchorRegion& c, double v)
							{
								double oldRight = -(pAnchorRegion->posDelta.x + pAnchorRegion->sizeDelta.x);
								double delta = v - oldRight;
								c.sizeDelta.x -= delta;
							}));
					fnAddChild(U"height", pAnchorRegion->sizeDelta.y, setDouble([](AnchorRegion& c, double v) { c.sizeDelta.y = v; }));
					fnAddChild(U"yDelta", pAnchorRegion->posDelta.y, setDouble([](AnchorRegion& c, double v) { c.posDelta.y = v; }));
					fnAddOptionalDoubleChild(U"minWidth", pAnchorRegion->minWidth, 
						[this, node](const Optional<double>& v) { auto newRegion = *node->anchorRegion(); newRegion.minWidth = v; node->setRegion(newRegion); });
					fnAddOptionalDoubleChild(U"maxWidth", pAnchorRegion->maxWidth, 
						[this, node](const Optional<double>& v) { auto newRegion = *node->anchorRegion(); newRegion.maxWidth = v; node->setRegion(newRegion); });
					break;

				case AnchorPreset::StretchBottom:
					fnAddChild(U"left", pAnchorRegion->posDelta.x,
						setDouble([pAnchorRegion](AnchorRegion& c, double v)
							{
								double oldLeft = pAnchorRegion->posDelta.x;
								double delta = oldLeft - v;
								c.posDelta.x = v;
								c.sizeDelta.x += delta;
							}));
					fnAddChild(U"right", -(pAnchorRegion->posDelta.x + pAnchorRegion->sizeDelta.x),
						setDouble([pAnchorRegion](AnchorRegion& c, double v)
							{
								double oldRight = -(pAnchorRegion->posDelta.x + pAnchorRegion->sizeDelta.x);
								double delta = v - oldRight;
								c.sizeDelta.x -= delta;
							}));
					fnAddChild(U"bottom", pAnchorRegion->posDelta.y, setDouble([](AnchorRegion& c, double v) { c.posDelta.y = -v; }));
					fnAddChild(U"height", pAnchorRegion->sizeDelta.y, setDouble([](AnchorRegion& c, double v) { c.sizeDelta.y = v; }));
					fnAddOptionalDoubleChild(U"minWidth", pAnchorRegion->minWidth, 
						[this, node](const Optional<double>& v) { auto newRegion = *node->anchorRegion(); newRegion.minWidth = v; node->setRegion(newRegion); });
					fnAddOptionalDoubleChild(U"maxWidth", pAnchorRegion->maxWidth, 
						[this, node](const Optional<double>& v) { auto newRegion = *node->anchorRegion(); newRegion.maxWidth = v; node->setRegion(newRegion); });
					break;

				case AnchorPreset::StretchLeft:
					fnAddChild(U"top", pAnchorRegion->posDelta.y,
						setDouble([pAnchorRegion](AnchorRegion& c, double v)
							{
								double oldTop = pAnchorRegion->posDelta.y;
								double delta = oldTop - v;
								c.posDelta.y = v;
								c.sizeDelta.y += delta;
							}));
					fnAddChild(U"bottom", -(pAnchorRegion->posDelta.y + pAnchorRegion->sizeDelta.y),
						setDouble([pAnchorRegion](AnchorRegion& c, double v)
							{
								double oldBottom = -(pAnchorRegion->posDelta.y + pAnchorRegion->sizeDelta.y);
								double delta = v - oldBottom;
								c.sizeDelta.y -= delta;
							}));
					fnAddChild(U"left", pAnchorRegion->posDelta.x, setDouble([](AnchorRegion& c, double v) { c.posDelta.x = v; }));
					fnAddChild(U"width", pAnchorRegion->sizeDelta.x, setDouble([](AnchorRegion& c, double v) { c.sizeDelta.x = v; }));
					fnAddOptionalDoubleChild(U"minHeight", pAnchorRegion->minHeight, 
						[this, node](const Optional<double>& v) { auto newRegion = *node->anchorRegion(); newRegion.minHeight = v; node->setRegion(newRegion); });
					fnAddOptionalDoubleChild(U"maxHeight", pAnchorRegion->maxHeight, 
						[this, node](const Optional<double>& v) { auto newRegion = *node->anchorRegion(); newRegion.maxHeight = v; node->setRegion(newRegion); });
					break;

				case AnchorPreset::StretchCenter:
					fnAddChild(U"top", pAnchorRegion->posDelta.y,
						setDouble([pAnchorRegion](AnchorRegion& c, double v)
							{
								double oldTop = pAnchorRegion->posDelta.y;
								double delta = oldTop - v;
								c.posDelta.y = v;
								c.sizeDelta.y += delta;
							}));
					fnAddChild(U"bottom", -(pAnchorRegion->posDelta.y + pAnchorRegion->sizeDelta.y),
						setDouble([pAnchorRegion](AnchorRegion& c, double v)
							{
								double oldBottom = -(pAnchorRegion->posDelta.y + pAnchorRegion->sizeDelta.y);
								double delta = v - oldBottom;
								c.sizeDelta.y -= delta;
							}));
					fnAddChild(U"width", pAnchorRegion->sizeDelta.x, setDouble([](AnchorRegion& c, double v) { c.sizeDelta.x = v; }));
					fnAddChild(U"xDelta", pAnchorRegion->posDelta.x, setDouble([](AnchorRegion& c, double v) { c.posDelta.x = v; }));
					fnAddOptionalDoubleChild(U"minHeight", pAnchorRegion->minHeight, 
						[this, node](const Optional<double>& v) { auto newRegion = *node->anchorRegion(); newRegion.minHeight = v; node->setRegion(newRegion); });
					fnAddOptionalDoubleChild(U"maxHeight", pAnchorRegion->maxHeight, 
						[this, node](const Optional<double>& v) { auto newRegion = *node->anchorRegion(); newRegion.maxHeight = v; node->setRegion(newRegion); });
					break;

				case AnchorPreset::StretchRight:
					fnAddChild(U"top", pAnchorRegion->posDelta.y,
						setDouble([pAnchorRegion](AnchorRegion& c, double v)
							{
								double oldTop = pAnchorRegion->posDelta.y;
								double delta = oldTop - v;
								c.posDelta.y = v;
								c.sizeDelta.y += delta;
							}));
					fnAddChild(U"bottom", -(pAnchorRegion->posDelta.y + pAnchorRegion->sizeDelta.y),
						setDouble([pAnchorRegion](AnchorRegion& c, double v)
							{
								double oldBottom = -(pAnchorRegion->posDelta.y + pAnchorRegion->sizeDelta.y);
								double delta = v - oldBottom;
								c.sizeDelta.y -= delta;
							}));
					fnAddChild(U"right", pAnchorRegion->posDelta.x, setDouble([](AnchorRegion& c, double v) { c.posDelta.x = -v; }));
					fnAddChild(U"width", pAnchorRegion->sizeDelta.x, setDouble([](AnchorRegion& c, double v) { c.sizeDelta.x = v; }));
					fnAddOptionalDoubleChild(U"minHeight", pAnchorRegion->minHeight, 
						[this, node](const Optional<double>& v) { auto newRegion = *node->anchorRegion(); newRegion.minHeight = v; node->setRegion(newRegion); });
					fnAddOptionalDoubleChild(U"maxHeight", pAnchorRegion->maxHeight, 
						[this, node](const Optional<double>& v) { auto newRegion = *node->anchorRegion(); newRegion.maxHeight = v; node->setRegion(newRegion); });
					break;

				case AnchorPreset::StretchFull:
					fnAddChild(U"left", pAnchorRegion->posDelta.x,
						setDouble([pAnchorRegion](AnchorRegion& c, double v)
							{
								double oldLeft = pAnchorRegion->posDelta.x;
								double delta = oldLeft - v;
								c.posDelta.x = v;
								c.sizeDelta.x += delta;
							}));
					fnAddChild(U"right", -(pAnchorRegion->posDelta.x + pAnchorRegion->sizeDelta.x),
						setDouble([pAnchorRegion](AnchorRegion& c, double v)
							{
								double oldRight = -(pAnchorRegion->posDelta.x + pAnchorRegion->sizeDelta.x);
								double delta = v - oldRight;
								c.sizeDelta.x -= delta;
							}));
					fnAddChild(U"top", pAnchorRegion->posDelta.y,
						setDouble([pAnchorRegion](AnchorRegion& c, double v)
							{
								double oldTop = pAnchorRegion->posDelta.y;
								double delta = oldTop - v;
								c.posDelta.y = v;
								c.sizeDelta.y += delta;
							}));
					fnAddChild(U"bottom", -(pAnchorRegion->posDelta.y + pAnchorRegion->sizeDelta.y),
						setDouble([pAnchorRegion](AnchorRegion& c, double v)
							{
								double oldBottom = -(pAnchorRegion->posDelta.y + pAnchorRegion->sizeDelta.y);
								double delta = v - oldBottom;
								c.sizeDelta.y -= delta;
							}));
					fnAddOptionalDoubleChild(U"minWidth", pAnchorRegion->minWidth, 
						[this, node](const Optional<double>& v) { auto newRegion = *node->anchorRegion(); newRegion.minWidth = v; node->setRegion(newRegion); });
					fnAddOptionalDoubleChild(U"minHeight", pAnchorRegion->minHeight, 
						[this, node](const Optional<double>& v) { auto newRegion = *node->anchorRegion(); newRegion.minHeight = v; node->setRegion(newRegion); });
					fnAddOptionalDoubleChild(U"maxWidth", pAnchorRegion->maxWidth, 
						[this, node](const Optional<double>& v) { auto newRegion = *node->anchorRegion(); newRegion.maxWidth = v; node->setRegion(newRegion); });
					fnAddOptionalDoubleChild(U"maxHeight", pAnchorRegion->maxHeight, 
						[this, node](const Optional<double>& v) { auto newRegion = *node->anchorRegion(); newRegion.maxHeight = v; node->setRegion(newRegion); });
					break;

				default:
					fnAddVec2Child(U"anchorMin", pAnchorRegion->anchorMin, setVec2([](AnchorRegion& c, const Vec2& val) { c.anchorMin = val; }));
					fnAddVec2Child(U"anchorMax", pAnchorRegion->anchorMax, setVec2([](AnchorRegion& c, const Vec2& val) { c.anchorMax = val; }));
					fnAddVec2Child(U"sizeDeltaPivot", pAnchorRegion->sizeDeltaPivot, setVec2([](AnchorRegion& c, const Vec2& val) { c.sizeDeltaPivot = val; }));
					fnAddVec2Child(U"posDelta", pAnchorRegion->posDelta, setVec2([](AnchorRegion& c, const Vec2& val) { c.posDelta = val; }));
					fnAddVec2Child(U"sizeDelta", pAnchorRegion->sizeDelta, setVec2([](AnchorRegion& c, const Vec2& val) { c.sizeDelta = val; }));
					fnAddOptionalDoubleChild(U"minWidth", pAnchorRegion->minWidth, 
						[this, node](const Optional<double>& v) { auto newRegion = *node->anchorRegion(); newRegion.minWidth = v; node->setRegion(newRegion); });
					fnAddOptionalDoubleChild(U"minHeight", pAnchorRegion->minHeight, 
						[this, node](const Optional<double>& v) { auto newRegion = *node->anchorRegion(); newRegion.minHeight = v; node->setRegion(newRegion); });
					fnAddOptionalDoubleChild(U"maxWidth", pAnchorRegion->maxWidth, 
						[this, node](const Optional<double>& v) { auto newRegion = *node->anchorRegion(); newRegion.maxWidth = v; node->setRegion(newRegion); });
					fnAddOptionalDoubleChild(U"maxHeight", pAnchorRegion->maxHeight, 
						[this, node](const Optional<double>& v) { auto newRegion = *node->anchorRegion(); newRegion.maxHeight = v; node->setRegion(newRegion); });
					break;
				}
			}
			else
			{
				throw Error{ U"Unknown region type" };
			}

			regionNode->setInlineRegionToFitToChildren(FitTarget::HeightOnly);

			return regionNode;
		}

		[[nodiscard]]
		std::shared_ptr<Node> createTransformNode(Transform* const pTransform)
		{
			auto transformNode = Node::Create(
				U"Transform",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.margin = LRTB{ 0, 0, 0, 8 },
				});
			transformNode->setChildrenLayout(VerticalLayout{ .padding = m_isFoldedTransform ? LRTB::Zero() : LRTB{ 0, 0, 0, 8 } });
			transformNode->emplaceComponent<RectRenderer>(ColorF{ 0.3, 0.3 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0);

			transformNode->addChild(CreateHeadingNode(U"Transform", ColorF{ 0.3, 0.5, 0.3 }, m_isFoldedTransform,
				[this](IsFoldedYN isFolded)
				{
					m_isFoldedTransform = isFolded;
				}));

			const auto fnAddVec2Child =
				[this, &transformNode](StringView name, SmoothProperty<Vec2>* pProperty, auto fnSetValue)
				{
					const auto propertyNode = transformNode->addChild(createVec2PropertyNodeWithTooltip(U"Transform", name, pProperty->propertyValue().defaultValue(), fnSetValue, HasInteractivePropertyValueYN{ pProperty->hasInteractivePropertyValue() }, HasParameterRefYN{ !pProperty->paramRef().isEmpty() }));
					propertyNode->setActive(!m_isFoldedTransform.getBool());
					
					Array<MenuElement> menuElements
					{
						MenuItem
						{ 
							U"ステート毎に値を変更..."_fmt(name), 
							U"", 
							KeyC, 
							[this, pProperty] 
							{ 
								m_dialogOpener->openDialog(std::make_shared<InteractivePropertyValueDialog>(pProperty, [this] { refreshInspector(); }, m_dialogOpener)); 
							} 
						},
						MenuSeparator{},
						MenuItem
						{ 
							.text = U"参照パラメータを選択..."_fmt(name), 
							.hotKeyText = U"", 
							.mnemonicInput = KeyP, 
							.onClick = [this, pProperty] 
							{ 
								m_dialogOpener->openDialog(std::make_shared<ParamRefDialog>(pProperty, m_canvas, [this] { refreshInspector(); }, m_dialogOpener)); 
							},
						},
						MenuItem
						{
							.text = U"参照パラメータをクリア"_fmt(name),
							.hotKeyText = U"",
							.mnemonicInput = KeyC,
							.onClick = [pProperty, this]
							{
								pProperty->setParamRef(U"");
								refreshInspector();
							},
							.fnIsEnabled = [pProperty] { return !pProperty->paramRef().isEmpty(); },
						},
					};
					
					propertyNode->template emplaceComponent<ContextMenuOpener>(m_contextMenu, menuElements, nullptr, RecursiveYN::Yes);
				};
			// Note: アクセサからポインタを取得しているので注意が必要
			fnAddVec2Child(U"translate", &pTransform->translate(), [this, pTransform](const Vec2& value) { pTransform->setTranslate(value); m_canvas->refreshLayoutImmediately(); });
			fnAddVec2Child(U"scale", &pTransform->scale(), [this, pTransform](const Vec2& value) { pTransform->setScale(value); m_canvas->refreshLayoutImmediately(); });
			fnAddVec2Child(U"pivot", &pTransform->pivot(), [this, pTransform](const Vec2& value) { pTransform->setPivot(value); m_canvas->refreshLayoutImmediately(); });
			
			// rotation プロパティを追加
			const auto fnAddDoubleChild =
				[this, &transformNode](StringView name, SmoothProperty<double>* pProperty, auto fnSetValue)
				{
					const auto propertyNode = transformNode->addChild(createPropertyNodeWithTooltip(U"Transform", name, Format(pProperty->propertyValue().defaultValue()), fnSetValue, HasInteractivePropertyValueYN{ pProperty->hasInteractivePropertyValue() }, HasParameterRefYN{ !pProperty->paramRef().isEmpty() }));
					propertyNode->setActive(!m_isFoldedTransform.getBool());
					
					Array<MenuElement> menuElements
					{
						MenuItem
						{ 
							U"ステート毎に値を変更..."_fmt(name), 
							U"", 
							KeyC, 
							[this, pProperty] 
							{ 
								m_dialogOpener->openDialog(std::make_shared<InteractivePropertyValueDialog>(pProperty, [this] { refreshInspector(); }, m_dialogOpener)); 
							} 
						},
						MenuSeparator{},
						MenuItem
						{ 
							.text = U"参照パラメータを選択..."_fmt(name), 
							.hotKeyText = U"", 
							.mnemonicInput = KeyP, 
							.onClick = [this, pProperty] 
							{ 
								m_dialogOpener->openDialog(std::make_shared<ParamRefDialog>(pProperty, m_canvas, [this] { refreshInspector(); }, m_dialogOpener)); 
							},
						},
						MenuItem
						{
							.text = U"参照パラメータをクリア"_fmt(name),
							.hotKeyText = U"",
							.mnemonicInput = KeyC,
							.onClick = [pProperty, this]
							{
								pProperty->setParamRef(U"");
								refreshInspector();
							},
							.fnIsEnabled = [pProperty] { return !pProperty->paramRef().isEmpty(); },
						},
					};
					
					propertyNode->template emplaceComponent<ContextMenuOpener>(m_contextMenu, menuElements, nullptr, RecursiveYN::Yes);
				};
			fnAddDoubleChild(U"rotation", &pTransform->rotation(), [this, pTransform](StringView valueStr) 
			{ 
				if (const auto value = ParseOpt<double>(valueStr))
				{
					// -180～180の範囲に正規化
					double normalizedValue = *value;
					if (std::isnan(normalizedValue) || normalizedValue < -1e9 || normalizedValue > 1e9)
					{
						normalizedValue = 0.0;
					}
					else
					{
						while (normalizedValue > 180.0) normalizedValue -= 360.0;
						while (normalizedValue < -180.0) normalizedValue += 360.0;
					}
					pTransform->setRotation(normalizedValue); 
					m_canvas->refreshLayoutImmediately(); 
				}
			});
			
			const auto fnAddBoolChild =
				[this, &transformNode](StringView name, Property<bool>* pProperty, auto fnSetValue)
				{
					const auto propertyNode = transformNode->addChild(createBoolPropertyNodeWithTooltip(U"Transform", name, pProperty->propertyValue().defaultValue(), fnSetValue, HasInteractivePropertyValueYN{ pProperty->hasInteractivePropertyValue() }, HasParameterRefYN{ !pProperty->paramRef().isEmpty() }));
					propertyNode->setActive(!m_isFoldedTransform.getBool());
					Array<MenuElement> menuElements
					{
						MenuItem
						{ 
							U"ステート毎に値を変更..."_fmt(name), 
							U"", 
							KeyC, 
							[this, pProperty] 
							{ 
								m_dialogOpener->openDialog(std::make_shared<InteractivePropertyValueDialog>(pProperty, [this] { refreshInspector(); }, m_dialogOpener)); 
							} 
						},
						MenuSeparator{},
						MenuItem
						{ 
							.text = U"参照パラメータを選択..."_fmt(name), 
							.hotKeyText = U"", 
							.mnemonicInput = KeyP, 
							.onClick = [this, pProperty] 
							{ 
								m_dialogOpener->openDialog(std::make_shared<ParamRefDialog>(pProperty, m_canvas, [this] { refreshInspector(); }, m_dialogOpener)); 
							},
						},
						MenuItem
						{
							.text = U"参照パラメータをクリア"_fmt(name),
							.hotKeyText = U"",
							.mnemonicInput = KeyC,
							.onClick = [pProperty, this]
							{
								pProperty->setParamRef(U"");
								refreshInspector();
							},
							.fnIsEnabled = [pProperty] { return !pProperty->paramRef().isEmpty(); },
						},
					};
					
					propertyNode->template emplaceComponent<ContextMenuOpener>(m_contextMenu, menuElements, nullptr, RecursiveYN::Yes);
				};
			fnAddBoolChild(U"hitTestAffected", &pTransform->hitTestAffected(), [this, pTransform](bool value) { pTransform->setHitTestAffected(value); });

			const auto fnAddColorChild =
				[this, &transformNode](StringView name, SmoothProperty<ColorF>* pProperty, auto fnSetValue)
				{
					const auto propertyNode = transformNode->addChild(createColorPropertyNodeWithTooltip(U"Transform", name, pProperty->propertyValue().defaultValue(), fnSetValue, HasInteractivePropertyValueYN{ pProperty->hasInteractivePropertyValue() }, HasParameterRefYN{ !pProperty->paramRef().isEmpty() }));
					propertyNode->setActive(!m_isFoldedTransform.getBool());
					
					Array<MenuElement> menuElements
					{
						MenuItem
						{ 
							U"ステート毎に値を変更..."_fmt(name), 
							U"", 
							KeyC, 
							[this, pProperty] 
							{ 
								m_dialogOpener->openDialog(std::make_shared<InteractivePropertyValueDialog>(pProperty, [this] { refreshInspector(); }, m_dialogOpener)); 
							} 
						},
						MenuSeparator{},
						MenuItem
						{ 
							.text = U"参照パラメータを選択..."_fmt(name), 
							.hotKeyText = U"", 
							.mnemonicInput = KeyP, 
							.onClick = [this, pProperty] 
							{ 
								m_dialogOpener->openDialog(std::make_shared<ParamRefDialog>(pProperty, m_canvas, [this] { refreshInspector(); }, m_dialogOpener)); 
							},
						},
						MenuItem
						{
							.text = U"参照パラメータをクリア"_fmt(name),
							.hotKeyText = U"",
							.mnemonicInput = KeyC,
							.onClick = [pProperty, this]
							{
								pProperty->setParamRef(U"");
								refreshInspector();
							},
							.fnIsEnabled = [pProperty] { return !pProperty->paramRef().isEmpty(); },
						},
					};
					
					propertyNode->template emplaceComponent<ContextMenuOpener>(m_contextMenu, menuElements, nullptr, RecursiveYN::Yes);
				};
			fnAddColorChild(U"color", &pTransform->color(), [this, pTransform](const ColorF& value) { pTransform->setColor(value); });

			transformNode->setInlineRegionToFitToChildren(FitTarget::HeightOnly);

			return transformNode;
		}

		[[nodiscard]]
		std::shared_ptr<Node> createSingleParamNode(const String& paramName, const ParamValue& value)
		{
			// パラメータ名に型情報を角括弧で付加
			const String typeStr = ParamTypeToString(GetParamType(value));
			const String labelText = U"{} [{}]"_fmt(paramName, typeStr);
			
			// パラメータの型に応じて適切な編集UIを作成
			std::shared_ptr<Node> propertyNode;
			
			switch (GetParamType(value))
			{
			case ParamType::Bool:
				{
					const bool currentValue = GetParamValueAs<bool>(value).value_or(false);
					propertyNode = CreateBoolPropertyNode(
						labelText,
						currentValue,
						[this, paramName](bool value) 
						{ 
							m_canvas->setParamValue(paramName, value);
						});
				}
				break;
				
			case ParamType::Number:
				{
					const double currentValue = GetParamValueAs<double>(value).value_or(0.0);
					propertyNode = CreatePropertyNode(
						labelText,
						Format(currentValue),
						[this, paramName](StringView text) 
						{ 
							if (const auto val = ParseOpt<double>(text))
							{
								m_canvas->setParamValue(paramName, *val);
							}
						},
						HasInteractivePropertyValueYN::No,
						HasParameterRefYN::No,
						[this, paramName]() -> String { 
							if (auto p = m_canvas->param(paramName))
							{
								return Format(GetParamValueAs<double>(*p).value_or(0.0));
							}
							return U"0";
						},
						0.01  // ドラッグステップ
					);
				}
				break;
				
			case ParamType::String:
				{
					const String currentValue = GetParamValueAs<String>(value).value_or(U"");
					propertyNode = CreatePropertyNodeWithTextArea(
						labelText,
						currentValue,
						[this, paramName](StringView text) 
						{ 
							m_canvas->setParamValue(paramName, String{ text });
						},
						HasInteractivePropertyValueYN::No,
						3);
				}
				break;
				
			case ParamType::Color:
				{
					const ColorF currentColor = GetParamValueAs<ColorF>(value).value_or(ColorF{});
					propertyNode = CreateColorPropertyNode(
						labelText,
						currentColor,
						[this, paramName](const ColorF& color) 
						{ 
							m_canvas->setParamValue(paramName, color);
						});
				}
				break;
				
			case ParamType::Vec2:
				{
					const Vec2 currentVec = GetParamValueAs<Vec2>(value).value_or(Vec2{});
					propertyNode = CreateVec2PropertyNode(
						labelText,
						currentVec,
						[this, paramName](const Vec2& vec) 
						{ 
							m_canvas->setParamValue(paramName, vec);
						});
				}
				break;
				
			case ParamType::LRTB:
				{
					const LRTB currentLRTB = GetParamValueAs<LRTB>(value).value_or(LRTB{});
					propertyNode = CreateLRTBPropertyNode(
						labelText,
						currentLRTB,
						[this, paramName](const LRTB& lrtb) 
						{ 
							m_canvas->setParamValue(paramName, lrtb);
						});
				}
				break;
			
			default:
				break;
			}
			
			// プロパティノードにコンテキストメニューを追加
			if (propertyNode)
			{
				Array<MenuElement> menuElements
				{
					MenuItem
					{
						.text = U"名前の変更...",
						.mnemonicInput = KeyR,
						.onClick = [this, paramName, value]
						{
							const size_t refCount = m_canvas ? m_canvas->countParamRefs(paramName) : 0;
							String dialogMessage = U"新しいパラメータ名を入力してください";
							if (refCount > 0)
							{
								dialogMessage += U"\n({}件の参照も名前変更後のパラメータへ更新されます)"_fmt(refCount);
							}
							
							m_dialogOpener->openDialog(
								std::make_shared<SimpleInputDialog>(
									dialogMessage,
									paramName,
									[this, paramName, value](StringView resultButtonText, StringView inputText)
									{
										if (resultButtonText == U"OK")
										{
											const String newName = String{ inputText }.trimmed();
											
											if (newName.isEmpty())
											{
												m_dialogOpener->openDialogOK(U"パラメータ名を空にすることはできません");
											}
											else if (newName == paramName)
											{
												m_dialogOpener->openDialogOK(U"パラメータ名が変更されていません");
											}
											else if (!IsValidParameterName(newName))
											{
												m_dialogOpener->openDialogOK(U"パラメータ名のルールに合致していません。パラメータ名は半角アルファベットまたは_で始まり、半角英数字と_で構成される名前である必要があります。");
											}
											else if (m_canvas->hasParam(newName))
											{
												m_dialogOpener->openDialogOK(U"パラメータ '{}' は既に存在します"_fmt(newName));
											}
											else
											{
												renameParam(paramName, newName);
												refreshInspector();
											}
										}
									},
									Array<DialogButtonDesc>
									{
										DialogButtonDesc
										{
											.text = U"OK",
											.mnemonicInput = KeyO,
											.isDefaultButton = IsDefaultButtonYN::Yes,
										},
										DialogButtonDesc
										{
											.text = U"キャンセル",
											.mnemonicInput = KeyC,
											.isCancelButton = IsCancelButtonYN::Yes,
										},
									}));
						}
					},
					MenuItem
					{
						.text = U"参照の一覧を表示...",
						.mnemonicInput = KeyL,
						.onClick = [this, paramName]
						{
							m_dialogOpener->openDialog(
								std::make_shared<ParamReferencesDialog>(paramName, m_canvas));
						}
					},
					MenuSeparator{},
					MenuItem
					{
						.text = U"パラメータ削除",
						.mnemonicInput = KeyD,
						.onClick = [this, paramName]
						{
							// 削除確認ダイアログを表示
							const size_t refCount = m_canvas->countParamRefs(paramName);
							m_dialogOpener->openDialog(
								std::make_shared<SimpleDialog>(
									U"パラメータ '{}'を削除しますか？\n参照しているプロパティ数: {}\n※参照しているプロパティからは参照が解除されます"_fmt(paramName, refCount),
									[this, paramName](StringView resultButtonText)
									{
										if (resultButtonText == U"はい")
										{
											m_canvas->clearParamRefs(paramName);
											m_canvas->removeParam(paramName);
											refreshInspector();
										}
									},
									Array<DialogButtonDesc>
									{
										DialogButtonDesc
										{
											.text = U"はい",
											.mnemonicInput = KeyY,
											.isDefaultButton = IsDefaultButtonYN::Yes,
										},
										DialogButtonDesc
										{
											.text = U"いいえ",
											.mnemonicInput = KeyN,
											.isCancelButton = IsCancelButtonYN::Yes,
										},
									}
								)
							);
						}
					},
				};
				
				propertyNode->emplaceComponent<ContextMenuOpener>(m_contextMenu, menuElements, nullptr, RecursiveYN::Yes);
			}
			
			return propertyNode;
		}

		[[nodiscard]]
		std::shared_ptr<Node> createCanvasSettingNode()
		{
			auto canvasSettingNode = Node::Create(
				U"CanvasSetting",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.margin = LRTB{ 0, 0, 0, 8 },
				});
			canvasSettingNode->setChildrenLayout(VerticalLayout{ .padding = m_isFoldedCanvasSetting ? LRTB::Zero() : LRTB{ 0, 0, 0, 8 } });
			canvasSettingNode->emplaceComponent<RectRenderer>(ColorF{ 0.3, 0.3 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0);

			canvasSettingNode->addChild(CreateHeadingNode(U"Canvas Settings", ColorF{ 0.5, 0.3, 0.3 }, m_isFoldedCanvasSetting,
				[this](IsFoldedYN isFolded)
				{
					m_isFoldedCanvasSetting = isFolded;
				}));

			if (!m_isFoldedCanvasSetting.getBool())
			{
				const auto referenceSizePropertyNode = CreateVec2PropertyNode(
					U"referenceSize",
					m_canvas->referenceSize(),
					[this](const Vec2& value)
					{
						m_canvas->setReferenceSize(SizeF{ value });
					});
				canvasSettingNode->addChild(referenceSizePropertyNode);

				const auto autoFitModePropertyNode = createEnumPropertyNodeWithTooltip(
					U"Canvas",
					U"autoFitMode",
					EnumToString(m_canvas->autoFitMode()),
					[this](StringView value)
					{
						if (const auto modeOpt = StringToValueOpt<AutoFitMode>(String{ value }))
						{
							m_canvas->setAutoFitMode(*modeOpt);
						}
					},
					m_contextMenu,
					EnumNames<AutoFitMode>());
				canvasSettingNode->addChild(autoFitModePropertyNode);
			}

			canvasSettingNode->setInlineRegionToFitToChildren(FitTarget::HeightOnly);
			return canvasSettingNode;
		}

		[[nodiscard]]
		std::shared_ptr<Node> createParamsNode()
		{
			auto paramsNode = Node::Create(
				U"Params",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.margin = LRTB{ 0, 0, 0, 8 },
				});
			paramsNode->setChildrenLayout(VerticalLayout{ .padding = m_isFoldedParams ? LRTB::Zero() : LRTB{ 0, 0, 0, 8 } });
			paramsNode->emplaceComponent<RectRenderer>(ColorF{ 0.3, 0.3 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0);

			paramsNode->addChild(CreateHeadingNode(U"Params", ColorF{ 0.3, 0.5, 0.6 }, m_isFoldedParams,
				[this](IsFoldedYN isFolded)
				{
					m_isFoldedParams = isFolded;
				}));

			if (!m_isFoldedParams.getBool())
			{
				// パラメータ一覧を表示
				const auto& params = m_canvas->params();
				
				if (params.empty())
				{
					const auto noParamLabelNode = paramsNode->emplaceChild(
						U"NoParams",
						InlineRegion
						{
							.sizeRatio = { 1, 0 },
							.sizeDelta = { 0, 24 },
							.margin = { .top = 4 },
						});
					noParamLabelNode->emplaceComponent<Label>(
						U"(パラメータなし)",
						U"",
						14,
						Palette::Gray,
						HorizontalAlign::Center,
						VerticalAlign::Middle);
					noParamLabelNode->setActive(!m_isFoldedParams.getBool());
				}
				else
				{
					// パラメータ名を取得してソート
					Array<String> paramNames;
					paramNames.reserve(params.size());
					for (const auto& [name, value] : params)
					{
						paramNames.push_back(name);
					}
					paramNames.sort();
					
					// パラメータ一覧の項目を追加
					for (const auto& name : paramNames)
					{
						const auto paramNode = createSingleParamNode(name, params.at(name));
						paramNode->setActive(!m_isFoldedParams.getBool());
						paramsNode->addChild(paramNode);
					}
				}
				
				// 新規パラメータ追加ボタン
				const auto addButton = paramsNode->addChild(CreateButtonNode(
					U"＋ 新規パラメータ",
					InlineRegion
					{
						.sizeRatio = Vec2{ 1, 0 },
						.sizeDelta = Vec2{ 0, 28 },
						.margin = LRTB{ 0, 0, 8, 0 },
						.maxWidth = 240,
					},
					[this](const std::shared_ptr<Node>&) 
					{ 
						// 新規パラメータ追加ダイアログを開く
						m_dialogOpener->openDialog(std::make_shared<AddParamDialog>(m_canvas, [this] { refreshInspector(); }, m_dialogOpener));
					}));
				addButton->setActive(!m_isFoldedParams.getBool());
			}

			paramsNode->setInlineRegionToFitToChildren(FitTarget::HeightOnly);

			return paramsNode;
		}

		[[nodiscard]]
		std::shared_ptr<Node> createComponentNode(const std::shared_ptr<Node>& node, const std::shared_ptr<SerializableComponentBase>& component, IsFoldedYN isFolded, std::function<void(IsFoldedYN)> onToggleFold)
		{
			String componentDisplayName = component->type();
			String componentMenuName = component->type();
			if (const auto placeholderComponent = std::dynamic_pointer_cast<PlaceholderComponent>(component))
			{
				// PlaceholderComponentの場合は項目名をカスタムコンポーネント名で上書き
				componentDisplayName = placeholderComponent->originalType() + U" (Custom)";
				componentMenuName = placeholderComponent->originalType();
			}
			
			auto componentNode = Node::Create(
				component->type(),
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.margin = LRTB{ 0, 0, 0, 8 },
				});
			componentNode->setChildrenLayout(VerticalLayout{ .padding = isFolded ? LRTB::Zero() : LRTB{ 0, 0, 0, 8 } });
			componentNode->emplaceComponent<RectRenderer>(ColorF{ 0.3, 0.3 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0);

			const auto headingNode = componentNode->addChild(CreateHeadingNode(componentDisplayName, ColorF{ 0.3, 0.3, 0.5 }, isFolded, std::move(onToggleFold)));
			Array<MenuElement> menuElements;
			menuElements.push_back(MenuItem{ U"{} を削除"_fmt(componentMenuName), U"", KeyR, [this, node, component] { node->removeComponent(component); refreshInspector(); } });
			menuElements.push_back(MenuSeparator{});
			menuElements.push_back(MenuItem{ U"{} を上へ移動"_fmt(componentMenuName), U"", KeyU, [this, node, component] { node->moveComponentUp(component); refreshInspector(); } });
			menuElements.push_back(MenuItem{ U"{} を下へ移動"_fmt(componentMenuName), U"", KeyD, [this, node, component] { node->moveComponentDown(component); refreshInspector(); } });
			menuElements.push_back(MenuSeparator{});
			menuElements.push_back(MenuItem{ U"{} の内容をコピー"_fmt(componentMenuName), U"", KeyC, [this, component] { onClickCopyComponent(component); } });
			
			// 同じタイプのコンポーネントがコピーされている場合のみ貼り付けメニューを表示
			if (m_copiedComponentType.has_value() && *m_copiedComponentType == component->type())
			{
				menuElements.push_back(MenuItem{ U"{} の内容を貼り付け"_fmt(componentMenuName), U"", KeyV, [this, component] { onClickPasteComponentTo(component); } });
			}
			
			headingNode->emplaceComponent<ContextMenuOpener>(m_contextMenu, menuElements);

			// PlaceholderComponentの場合はスキーマをもとにプロパティ表示
			if (const auto placeholderComponent = std::dynamic_pointer_cast<PlaceholderComponent>(component))
			{
				const ComponentSchema* schema = ComponentSchemaLoader::GetSchema(placeholderComponent->originalType());
				if (!schema)
				{
					const auto warningNode = componentNode->emplaceChild(
						U"NoSchema",
						InlineRegion
						{
							.sizeRatio = { 1, 0 },
							.sizeDelta = { 0, 24 },
							.margin = { .top = 4 },
						});
					warningNode->emplaceComponent<Label>(
						U"({} のスキーマ定義が存在しません)"_fmt(placeholderComponent->originalType()),
						U"",
						14,
						Palette::Gray,
						HorizontalAlign::Center,
						VerticalAlign::Middle);
					if (isFolded)
					{
						warningNode->setActive(false);
					}
				}
				else
				{
					placeholderComponent->setSchema(schema);

					// サムネイル画像が存在する場合は設定
					if (schema->thumbnailTexture)
					{
						placeholderComponent->setThumbnailTexture(schema->thumbnailTexture);
					}

					if (!isFolded)
					{
						for (const auto& propSchema : schema->properties)
						{
							String currentValue = placeholderComponent->getPropertyValueString(propSchema.name);
							if (currentValue.isEmpty() && !std::holds_alternative<std::monostate>(propSchema.defaultValue))
							{
								// variantからStringに変換
								std::visit([&currentValue, &placeholderComponent, &propSchema](auto&& value)
								{
									using T = std::decay_t<decltype(value)>;
									if constexpr (std::is_same_v<T, std::monostate>)
									{
										// 値なし
									}
									else if constexpr (std::is_same_v<T, bool>)
									{
										currentValue = value ? U"true" : U"false";
										placeholderComponent->setPropertyValueString(propSchema.name, currentValue);
									}
									else if constexpr (std::is_arithmetic_v<T> && !std::is_same_v<T, bool>)
									{
										currentValue = Format(static_cast<double>(value));
										placeholderComponent->setPropertyValueString(propSchema.name, currentValue);
									}
									else if constexpr (std::is_same_v<T, String>)
									{
										currentValue = value;
										placeholderComponent->setPropertyValueString(propSchema.name, currentValue);
									}
									else if constexpr (std::is_same_v<T, ColorF> || std::is_same_v<T, Vec2> || std::is_same_v<T, LRTB>)
									{
										currentValue = ValueToString(value);
										placeholderComponent->setPropertyValueString(propSchema.name, currentValue);
									}
								}, propSchema.defaultValue);
							}
							
							const String displayName = propSchema.displayName.isEmpty() ? propSchema.name : propSchema.displayName;
							std::shared_ptr<Node> propertyNode;
							switch (propSchema.editType)
							{
							case PropertyEditType::Text:
								{
									auto onChange = [placeholderComponent, propName = propSchema.name](StringView value)
									{
										placeholderComponent->setPropertyValueString(propName, String{ value });
									};
									
									// Placeholderではメタデータを動的に設定
									const PropertyMetadata tempMetadata
									{
										.displayName = displayName,
										.tooltip = propSchema.tooltip.isEmpty() ? none : Optional<String>{ propSchema.tooltip },
										.tooltipDetail = propSchema.tooltipDetail.isEmpty() ? none : Optional<String>{ propSchema.tooltipDetail },
										.numTextAreaLines = propSchema.numTextAreaLines,
										.dragValueChangeStep = propSchema.dragValueChangeStep,
									};
									m_propertyMetadata[PropertyKey{ placeholderComponent->originalType(), propSchema.name }] = tempMetadata;
									
									propertyNode = createPropertyNodeWithTooltip(
										placeholderComponent->originalType(),
										propSchema.name,
										currentValue,
										onChange,
										HasInteractivePropertyValueYN{ placeholderComponent->getProperty(propSchema.name)->hasInteractivePropertyValue() },
										HasParameterRefYN{ placeholderComponent->getProperty(propSchema.name)->hasParamRef() });
								}
								break;
								
							case PropertyEditType::Number:
								{
									auto onChange = [placeholderComponent, propName = propSchema.name](StringView value)
									{
										placeholderComponent->setPropertyValueString(propName, String{ value });
									};
									
									// Placeholderではメタデータを動的に設定
									const PropertyMetadata tempMetadata
									{
										.displayName = displayName,
										.tooltip = propSchema.tooltip.isEmpty() ? none : Optional<String>{ propSchema.tooltip },
										.tooltipDetail = propSchema.tooltipDetail.isEmpty() ? none : Optional<String>{ propSchema.tooltipDetail },
										.dragValueChangeStep = propSchema.dragValueChangeStep,
									};
									m_propertyMetadata[PropertyKey{ placeholderComponent->originalType(), propSchema.name }] = tempMetadata;
									
									propertyNode = createPropertyNodeWithTooltip(
										placeholderComponent->originalType(),
										propSchema.name,
										currentValue,
										onChange,
										HasInteractivePropertyValueYN{ placeholderComponent->getProperty(propSchema.name)->hasInteractivePropertyValue() },
										HasParameterRefYN{ placeholderComponent->getProperty(propSchema.name)->hasParamRef() });
								}
								break;
								
							case PropertyEditType::Bool:
								{
									bool value = StringToValueOr<bool>(currentValue, false);
									auto onChange = [placeholderComponent, propName = propSchema.name](bool value)
									{
										placeholderComponent->setPropertyValueString(propName, ValueToString(value));
									};
									
									// Placeholderではメタデータを動的に設定
									const PropertyMetadata tempMetadata
									{
										.displayName = displayName,
										.tooltip = propSchema.tooltip.isEmpty() ? none : Optional<String>{ propSchema.tooltip },
										.tooltipDetail = propSchema.tooltipDetail.isEmpty() ? none : Optional<String>{ propSchema.tooltipDetail },
									};
									m_propertyMetadata[PropertyKey{ placeholderComponent->originalType(), propSchema.name }] = tempMetadata;
									
									propertyNode = createBoolPropertyNodeWithTooltip(
										placeholderComponent->originalType(),
										propSchema.name,
										value,
										onChange,
										HasInteractivePropertyValueYN{ placeholderComponent->getProperty(propSchema.name)->hasInteractivePropertyValue() },
										HasParameterRefYN{ placeholderComponent->getProperty(propSchema.name)->hasParamRef() });
								}
								break;
								
							case PropertyEditType::Vec2:
								{
									Vec2 value = StringToValueOr<Vec2>(currentValue, Vec2{0, 0});
									auto onChange = [placeholderComponent, propName = propSchema.name](const Vec2& value)
									{
										placeholderComponent->setPropertyValueString(propName, ValueToString(value));
									};
									
									// Placeholderではメタデータを動的に設定
									const PropertyMetadata tempMetadata
									{
										.displayName = displayName,
										.tooltip = propSchema.tooltip.isEmpty() ? none : Optional<String>{ propSchema.tooltip },
										.tooltipDetail = propSchema.tooltipDetail.isEmpty() ? none : Optional<String>{ propSchema.tooltipDetail },
									};
									m_propertyMetadata[PropertyKey{ placeholderComponent->originalType(), propSchema.name }] = tempMetadata;
									
									propertyNode = createVec2PropertyNodeWithTooltip(
										placeholderComponent->originalType(),
										propSchema.name,
										value,
										onChange,
										HasInteractivePropertyValueYN{ placeholderComponent->getProperty(propSchema.name)->hasInteractivePropertyValue() },
										HasParameterRefYN{ placeholderComponent->getProperty(propSchema.name)->hasParamRef() });
								}
								break;
								
							case PropertyEditType::Color:
								{
									ColorF value = StringToValueOr<ColorF>(currentValue, ColorF{});
									auto onChange = [placeholderComponent, propName = propSchema.name](const ColorF& value)
									{
										placeholderComponent->setPropertyValueString(propName, ValueToString(value));
									};
									
									// Placeholderではメタデータを動的に設定
									const PropertyMetadata tempMetadata
									{
										.displayName = displayName,
										.tooltip = propSchema.tooltip.isEmpty() ? none : Optional<String>{ propSchema.tooltip },
										.tooltipDetail = propSchema.tooltipDetail.isEmpty() ? none : Optional<String>{ propSchema.tooltipDetail },
									};
									m_propertyMetadata[PropertyKey{ placeholderComponent->originalType(), propSchema.name }] = tempMetadata;
									
									propertyNode = createColorPropertyNodeWithTooltip(
										placeholderComponent->originalType(),
										propSchema.name,
										value,
										onChange,
										HasInteractivePropertyValueYN{ placeholderComponent->getProperty(propSchema.name)->hasInteractivePropertyValue() },
										HasParameterRefYN{ placeholderComponent->getProperty(propSchema.name)->hasParamRef() });
								}
								break;
								
							case PropertyEditType::LRTB:
								{
									LRTB value = StringToValueOr<LRTB>(currentValue, LRTB{0, 0, 0, 0});
									auto onChange = [placeholderComponent, propName = propSchema.name](const LRTB& value)
									{
										placeholderComponent->setPropertyValueString(propName, ValueToString(value));
									};
									
									// Placeholderではメタデータを動的に設定
									const PropertyMetadata tempMetadata
									{
										.displayName = displayName,
										.tooltip = propSchema.tooltip.isEmpty() ? none : Optional<String>{ propSchema.tooltip },
										.tooltipDetail = propSchema.tooltipDetail.isEmpty() ? none : Optional<String>{ propSchema.tooltipDetail },
									};
									m_propertyMetadata[PropertyKey{ placeholderComponent->originalType(), propSchema.name }] = tempMetadata;
									
									propertyNode = createLRTBPropertyNodeWithTooltip(
										placeholderComponent->originalType(),
										propSchema.name,
										value,
										onChange,
										HasInteractivePropertyValueYN{ placeholderComponent->getProperty(propSchema.name)->hasInteractivePropertyValue() },
										HasParameterRefYN{ placeholderComponent->getProperty(propSchema.name)->hasParamRef() });
								}
								break;
								
							case PropertyEditType::Enum:
								{
									auto onChange = [placeholderComponent, propName = propSchema.name](StringView value)
									{
										placeholderComponent->setPropertyValueString(propName, String{ value });
									};
									
									// Placeholderではメタデータを動的に設定
									const PropertyMetadata tempMetadata
									{
										.displayName = displayName,
										.tooltip = propSchema.tooltip.isEmpty() ? none : Optional<String>{ propSchema.tooltip },
										.tooltipDetail = propSchema.tooltipDetail.isEmpty() ? none : Optional<String>{ propSchema.tooltipDetail },
									};
									m_propertyMetadata[PropertyKey{ placeholderComponent->originalType(), propSchema.name }] = tempMetadata;
									
									propertyNode = createEnumPropertyNodeWithTooltip(
										placeholderComponent->originalType(),
										propSchema.name,
										currentValue,
										onChange,
										m_contextMenu,
										propSchema.enumCandidates,
										HasInteractivePropertyValueYN{ placeholderComponent->getProperty(propSchema.name)->hasInteractivePropertyValue() },
										HasParameterRefYN{ placeholderComponent->getProperty(propSchema.name)->hasParamRef() });
								}
								break;
							}
							
							if (propertyNode)
							{
								if (auto placeholderProperty = placeholderComponent->getProperty(propSchema.name))
								{
									const bool isInteractive = placeholderProperty->isInteractiveProperty();
									
									Array<MenuElement> stateMenuElements
									{
										MenuItem
										{ 
											.text = U"ステート毎に値を変更..."_fmt(placeholderProperty->name()), 
											.hotKeyText = U"", 
											.mnemonicInput = KeyC, 
											.onClick = [this, placeholderProperty] 
											{ 
												m_dialogOpener->openDialog(std::make_shared<InteractivePropertyValueDialog>(placeholderProperty, [this] { refreshInspector(); }, m_dialogOpener)); 
											},
											.fnIsEnabled = [isInteractive] { return isInteractive; }
										},
										MenuSeparator{},
										MenuItem
										{ 
											.text = U"参照パラメータを選択..."_fmt(placeholderProperty->name()), 
											.hotKeyText = U"", 
											.mnemonicInput = KeyP, 
											.onClick = [this, placeholderProperty] 
											{ 
												m_dialogOpener->openDialog(std::make_shared<ParamRefDialog>(placeholderProperty, m_canvas, [this] { refreshInspector(); }, m_dialogOpener)); 
											},
										},
										MenuItem
										{
											.text = U"参照パラメータをクリア"_fmt(placeholderProperty->name()),
											.hotKeyText = U"",
											.mnemonicInput = KeyC,
											.onClick = [placeholderProperty, this]
											{
												placeholderProperty->setParamRef(U"");
												refreshInspector();
											},
											.fnIsEnabled = [placeholderProperty] { return !placeholderProperty->paramRef().isEmpty(); },
										},
									};
									
									propertyNode->emplaceComponent<ContextMenuOpener>(
										m_contextMenu,
										stateMenuElements,
										nullptr,
										RecursiveYN::Yes);
								}
								
								componentNode->addChild(propertyNode);
							}
						}
					}
				}
				
				componentNode->setInlineRegionToFitToChildren(FitTarget::HeightOnly);
				
				return componentNode;
			}

			if (component->properties().empty())
			{
				const auto noPropertyLabelNode = componentNode->emplaceChild(
					U"NoProperty",
					InlineRegion
					{
						.sizeRatio = { 1, 0 },
						.sizeDelta = { 0, 24 },
						.margin = { .top = 4 },
					});
				noPropertyLabelNode->emplaceComponent<Label>(
					U"(プロパティなし)",
					U"",
					14,
					Palette::White,
					HorizontalAlign::Center,
					VerticalAlign::Middle);
				if (isFolded)
				{
					noPropertyLabelNode->setActive(false);
				}
			}
			for (const auto& property : component->properties())
			{
				const PropertyEditType editType = property->editType();
				std::shared_ptr<Node> propertyNode;
				switch (editType)
				{
				case PropertyEditType::Number:
				case PropertyEditType::Text:
					{
						std::function<void(StringView)> onChange = [property](StringView value) { property->trySetPropertyValueString(value); };
						
						// メタデータに基づいて変更時の処理を設定
						const PropertyKey propertyKey{ String{ component->type() }, String{ property->name() } };
						std::function<String()> fnGetValue = nullptr;
						if (const auto it = m_propertyMetadata.find(propertyKey); it != m_propertyMetadata.end())
						{
							const auto& metadata = it->second;
							if (metadata.refreshInspectorOnChange)
							{
								onChange = [this, property](StringView value)
								{
									property->trySetPropertyValueString(value);
									refreshInspector();
								};
							}
							// refreshesEveryFrameがtrueの場合のみfnGetValueを設定
							if (metadata.refreshesEveryFrame)
							{
								fnGetValue = [property]() -> String { return property->propertyValueStringOfDefault(); };
							}
						}
						
						propertyNode = componentNode->addChild(
							createPropertyNodeWithTooltip(
								component->type(),
								property->name(),
								property->propertyValueStringOfDefault(),
								onChange,
								HasInteractivePropertyValueYN{ property->hasInteractivePropertyValue() },
								HasParameterRefYN{ !property->paramRef().isEmpty() },
								fnGetValue));
					}
					break;
				case PropertyEditType::Bool:
					{
						std::function<void(bool)> onChange = [property](bool value) { property->trySetPropertyValueString(Format(value)); };
						
						// メタデータに基づいて変更時の処理を設定
						const PropertyKey propertyKey{ String{ component->type() }, String{ property->name() } };
						if (const auto it = m_propertyMetadata.find(propertyKey); it != m_propertyMetadata.end())
						{
							const auto& metadata = it->second;
							if (metadata.refreshInspectorOnChange)
							{
								onChange = [this, property](bool value)
								{
									property->trySetPropertyValueString(Format(value));
									refreshInspector();
								};
							}
						}
						
						propertyNode = componentNode->addChild(
							createBoolPropertyNodeWithTooltip(
								component->type(),
								property->name(),
								ParseOr<bool>(property->propertyValueStringOfDefault(), false),
								onChange,
								HasInteractivePropertyValueYN{ property->hasInteractivePropertyValue() },
								HasParameterRefYN{ !property->paramRef().isEmpty() }));
					}
					break;
				case PropertyEditType::Vec2:
					{
						std::function<void(const Vec2&)> onChange = [property](const Vec2& value) { property->trySetPropertyValueString(Format(value)); };
						
						// メタデータに基づいて変更時の処理を設定
						const PropertyKey propertyKey{ String{ component->type() }, String{ property->name() } };
						if (const auto it = m_propertyMetadata.find(propertyKey); it != m_propertyMetadata.end())
						{
							const auto& metadata = it->second;
							if (metadata.refreshInspectorOnChange)
							{
								onChange = [this, property](const Vec2& value)
								{
									property->trySetPropertyValueString(Format(value));
									refreshInspector();
								};
							}
						}
						
						propertyNode = componentNode->addChild(
							createVec2PropertyNodeWithTooltip(
								component->type(),
								property->name(),
								ParseOr<Vec2>(property->propertyValueStringOfDefault(), Vec2{ 0, 0 }),
								onChange,
								HasInteractivePropertyValueYN{ property->hasInteractivePropertyValue() },
								HasParameterRefYN{ !property->paramRef().isEmpty() }));
					}
					break;
				case PropertyEditType::Color:
					{
						std::function<void(const ColorF&)> onChange = [property](const ColorF& value) { property->trySetPropertyValueString(Format(value)); };
						
						// メタデータに基づいて変更時の処理を設定
						const PropertyKey propertyKey{ String{ component->type() }, String{ property->name() } };
						if (const auto it = m_propertyMetadata.find(propertyKey); it != m_propertyMetadata.end())
						{
							const auto& metadata = it->second;
							if (metadata.refreshInspectorOnChange)
							{
								onChange = [this, property](const ColorF& value)
								{
									property->trySetPropertyValueString(Format(value));
									refreshInspector();
								};
							}
						}
						
						propertyNode = componentNode->addChild(
							createColorPropertyNodeWithTooltip(
								component->type(),
								property->name(),
								ParseOr<ColorF>(property->propertyValueStringOfDefault(), ColorF{}),
								onChange,
								HasInteractivePropertyValueYN{ property->hasInteractivePropertyValue() },
								HasParameterRefYN{ !property->paramRef().isEmpty() }));
					}
					break;
				case PropertyEditType::LRTB:
					{
						std::function<void(const LRTB&)> onChange = [property](const LRTB& value) { property->trySetPropertyValueString(Format(value)); };
						
						// メタデータに基づいて変更時の処理を設定
						const PropertyKey propertyKey{ String{ component->type() }, String{ property->name() } };
						if (const auto it = m_propertyMetadata.find(propertyKey); it != m_propertyMetadata.end())
						{
							const auto& metadata = it->second;
							if (metadata.refreshInspectorOnChange)
							{
								onChange = [this, property](const LRTB& value)
								{
									property->trySetPropertyValueString(Format(value));
									refreshInspector();
								};
							}
						}
						
						propertyNode = componentNode->addChild(
							createLRTBPropertyNodeWithTooltip(
								component->type(),
								property->name(),
								ParseOr<LRTB>(property->propertyValueStringOfDefault(), LRTB{ 0, 0, 0, 0 }),
								onChange,
								HasInteractivePropertyValueYN{ property->hasInteractivePropertyValue() },
								HasParameterRefYN{ !property->paramRef().isEmpty() }));
					}
					break;
				case PropertyEditType::Enum:
					{
						std::function<void(StringView)> onChange = [property](StringView value) { property->trySetPropertyValueString(value); };
						
						// メタデータに基づいて変更時の処理を設定
						const PropertyKey propertyKey{ String{ component->type() }, String{ property->name() } };
						if (const auto it = m_propertyMetadata.find(propertyKey); it != m_propertyMetadata.end())
						{
							const auto& metadata = it->second;
							if (metadata.refreshInspectorOnChange)
							{
								onChange = [this, property](StringView value)
								{
									property->trySetPropertyValueString(value);
									refreshInspector();
								};
							}
						}
						
						propertyNode = componentNode->addChild(
							createEnumPropertyNodeWithTooltip(
								component->type(),
								property->name(),
								property->propertyValueStringOfDefault(),
								onChange,
								m_contextMenu,
								property->enumCandidates(),
								HasInteractivePropertyValueYN{ property->hasInteractivePropertyValue() },
								HasParameterRefYN{ !property->paramRef().isEmpty() }));
					}
					break;
				}
				if (!propertyNode)
				{
					throw Error{ U"Failed to create property node" };
				}

				// 表示条件のチェック
				const PropertyKey visibilityKey{ String{ component->type() }, String{ property->name() } };
				if (const auto it = m_propertyMetadata.find(visibilityKey); it != m_propertyMetadata.end())
				{
					const auto& metadata = it->second;
					// 可視条件のチェック
					bool isVisible = true;
					if (metadata.visibilityCondition)
					{
						isVisible = metadata.visibilityCondition(*component);
					}
					
					// プロパティの可視情報を保存
					propertyNode->storeData<PropertyVisibilityData>({ .isVisibleByCondition = isVisible });
					
					// 可視条件を満たさない、または折り畳まれている場合は非表示
					if (!isVisible || isFolded)
					{
						propertyNode->setActive(false);
					}
				}
				else
				{
					// メタデータがない場合は、常に表示可能として扱う
					propertyNode->storeData<PropertyVisibilityData>({ .isVisibleByCondition = true });
					
					if (isFolded)
					{
						propertyNode->setActive(false);
					}
				}
				
				const Array<ParamType> requiredParamTypes = { GetRequiredParamType(property) };
				const bool isInteractive = property->isInteractiveProperty();
				
				Array<MenuElement> stateMenuElements
				{
					MenuItem
					{ 
						.text = U"ステート毎に値を変更..."_fmt(property->name()), 
						.hotKeyText = U"", 
						.mnemonicInput = KeyC, 
						.onClick = [this, property] 
						{ 
							m_dialogOpener->openDialog(std::make_shared<InteractivePropertyValueDialog>(property, [this] { refreshInspector(); }, m_dialogOpener)); 
						},
						// PropertyNonInteractiveの場合は無効化
						.fnIsEnabled = [isInteractive] { return isInteractive; }
					},
					MenuSeparator{},
					MenuItem
					{ 
						.text = U"参照パラメータを選択..."_fmt(property->name()), 
						.hotKeyText = U"", 
						.mnemonicInput = KeyP, 
						.onClick = [this, property] 
						{ 
							m_dialogOpener->openDialog(std::make_shared<ParamRefDialog>(property, m_canvas, [this] { refreshInspector(); }, m_dialogOpener)); 
						},
					},
					MenuItem
					{
						.text = U"参照パラメータをクリア"_fmt(property->name()),
						.hotKeyText = U"",
						.mnemonicInput = KeyC,
						.onClick = [property, this]
						{
							property->setParamRef(U"");
							refreshInspector();
						},
						.fnIsEnabled = [property] { return !property->paramRef().isEmpty(); },
					},
				};
				
				propertyNode->emplaceComponent<ContextMenuOpener>(
					m_contextMenu,
					stateMenuElements,
					nullptr,
					RecursiveYN::Yes);
			}

			// Spriteコンポーネントの場合、スナップボタンを追加
			// TODO: コンポーネント毎のカスタムInspectorを追加するための仕組みを整備する
			if (auto sprite = std::dynamic_pointer_cast<Sprite>(component))
			{
				auto snapButton = componentNode->addChild(CreateButtonNode(
					U"テクスチャサイズへスナップ",
					InlineRegion
					{
						.sizeRatio = Vec2{ 1, 0 },
						.sizeDelta = Vec2{ 0, 24 },
						.margin = LRTB{ 12, 12, 4, 0 },
					},
					[this, sprite, node](const std::shared_ptr<Node>&)
					{
						doSnapNodeSizeToTexture(sprite, node);
						refreshInspector();
					}));
				
				if (isFolded)
				{
					snapButton->setActive(false);
				}
			}

			componentNode->setInlineRegionToFitToChildren(FitTarget::HeightOnly);

			return componentNode;
		}

		void clearTargetNode()
		{
			setTargetNode(nullptr);
		}

		void update()
		{
		}

		[[nodiscard]]
		const std::shared_ptr<Node>& inspectorFrameNode() const
		{
			return m_inspectorFrameNode;
		}
		
		void setWidth(double width)
		{
			if (auto* pAnchorRegion = m_inspectorFrameNode->anchorRegion())
			{
				auto newRegion = *pAnchorRegion;
				newRegion.sizeDelta.x = width;
				m_inspectorFrameNode->setRegion(newRegion);
			}
			else
			{
				Logger << U"[NocoEditor warning] AnchorRegion not found in inspectorFrameNode";
			}
		}
		
	};
}
