#pragma once
#include <NocoUI.hpp>
#include "Defaults.hpp"
#include "Tooltip.hpp"
#include "Toolbar.hpp"
#include "TabStop.hpp"
#include "ContextMenu.hpp"
#include "EditorDialog.hpp"
#include "PropertyMetaData.hpp"
#include "Vec4PropertyTextBox.hpp"

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

		IsFoldedYN m_isFoldedConstraint = IsFoldedYN::No;
		IsFoldedYN m_isFoldedNodeSetting = IsFoldedYN::Yes;
		IsFoldedYN m_isFoldedLayout = IsFoldedYN::Yes;
		IsFoldedYN m_isFoldedTransformEffect = IsFoldedYN::Yes;
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
			
			// JSONからコンポーネントを作成
			auto componentJSON = *m_copiedComponentJSON;
			componentJSON[U"type"] = *m_copiedComponentType;
			
			// CreateComponentFromJSONを使用してコンポーネントを作成
			if (const auto component = CreateComponentFromJSON(componentJSON))
			{
				node->addComponent(component);
				refreshInspector();
			}
		}
		
		void doSnapNodeSizeToTexture(const std::shared_ptr<Sprite>& sprite, const std::shared_ptr<Node>& node)
		{
			const String texturePath = sprite->textureFilePath().defaultValue;
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
			
			if (const auto* pBoxConstraint = node->boxConstraint())
			{
				BoxConstraint newConstraint = *pBoxConstraint;
				newConstraint.sizeDelta = textureSize;
				newConstraint.sizeRatio = Vec2::Zero();
				newConstraint.flexibleWeight = 0.0;
				node->setConstraint(newConstraint);
			}
			else if (const auto* pAnchorConstraint = node->anchorConstraint())
			{
				AnchorConstraint newConstraint = *pAnchorConstraint;
				newConstraint.sizeDelta = textureSize;
				newConstraint.anchorMin = noco::Anchor::MiddleCenter;
				newConstraint.anchorMax = noco::Anchor::MiddleCenter;
				node->setConstraint(newConstraint);
			}
		}

	public:
		explicit Inspector(const std::shared_ptr<Canvas>& canvas, const std::shared_ptr<Canvas>& editorCanvas, const std::shared_ptr<Canvas>& editorOverlayCanvas, const std::shared_ptr<ContextMenu>& contextMenu, const std::shared_ptr<Defaults>& defaults, const std::shared_ptr<DialogOpener>& dialogOpener, std::function<void()> onChangeNodeName)
			: m_canvas(canvas)
			, m_editorCanvas(editorCanvas)
			, m_editorOverlayCanvas(editorOverlayCanvas)
			, m_inspectorFrameNode(editorCanvas->rootNode()->emplaceChild(
				U"InspectorFrame",
				AnchorConstraint
				{
					.anchorMin = Anchor::TopRight,
					.anchorMax = Anchor::BottomRight,
					.posDelta = Vec2{ 0, MenuBarHeight + Toolbar::ToolbarHeight },
					.sizeDelta = Vec2{ 400, -(MenuBarHeight + Toolbar::ToolbarHeight) },
					.sizeDeltaPivot = Anchor::TopRight,
				}))
			, m_inspectorInnerFrameNode(m_inspectorFrameNode->emplaceChild(
				U"InspectorInnerFrame",
				AnchorConstraint
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
				AnchorConstraint
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
			, m_propertyMetadata(InitPropertyMetadata())
		{
			m_inspectorFrameNode->emplaceComponent<RectRenderer>(ColorF{ 0.5, 0.4 }, Palette::Black, 0.0, 10.0);
			m_inspectorInnerFrameNode->emplaceComponent<RectRenderer>(ColorF{ 0.1, 0.8 }, Palette::Black, 0.0, 10.0);
			m_inspectorRootNode->setBoxChildrenLayout(VerticalLayout{ .padding = LRTB{ 0, 0, 4, 4 } });
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
					currentNode = currentNode->parent();
				}
			}
			
			setTargetNode(m_targetNode.lock());
			if (preserveScroll)
			{
				m_inspectorRootNode->resetScrollOffset(RecursiveYN::No, RefreshesLayoutYN::No, RefreshesLayoutYN::No);
				m_inspectorRootNode->scroll(Vec2{ 0, scrollY }, RefreshesLayoutYN::No);
			}
			m_editorCanvas->refreshLayout();
			
			// TabStopを持つすべてのノードを収集してリンクを設定
			setupTabStopLinks();
			
			// フォーカスを復元
			if (isInInspector && !focusedNodeName.empty())
			{
				// 同じ名前のノードを探してフォーカスを復元
				auto newFocusNode = m_inspectorRootNode->getChildByNameOrNull(focusedNodeName, RecursiveYN::Yes);
				if (newFocusNode && newFocusNode->getComponentOrNull<nocoeditor::TabStop>())
				{
					CurrentFrame::SetFocusedNode(newFocusNode);
				}
			}
		}
		
		void setupTabStopLinks()
		{
			// TabStopを持つすべてのノードを収集
			Array<std::shared_ptr<Node>> tabStopNodes;
			collectTabStopNodes(m_inspectorRootNode, tabStopNodes);
			
			if (tabStopNodes.size() == 0)
			{
				return;
			}
			
			// 各ノードのTabStopに次と前のノードを設定
			for (size_t i = 0; i < tabStopNodes.size(); ++i)
			{
				auto tabStop = tabStopNodes[i]->getComponentOrNull<nocoeditor::TabStop>();
				if (!tabStop)
				{
					continue;
				}
				
				// 次のノードを設定（最後の要素は最初に戻る）
				size_t nextIndex = (i + 1) % tabStopNodes.size();
				tabStop->setNextNode(tabStopNodes[nextIndex]);
				
				// 前のノードを設定（最初の要素は最後に戻る）
				size_t prevIndex = (i == 0) ? tabStopNodes.size() - 1 : i - 1;
				tabStop->setPreviousNode(tabStopNodes[prevIndex]);
				
			}
		}
		
		void collectTabStopNodes(const std::shared_ptr<Node>& node, Array<std::shared_ptr<Node>>& tabStopNodes)
		{
			if (!node)
			{
				return;
			}
			
			// このノードがTabStopを持っているかチェック
			if (node->getComponentOrNull<nocoeditor::TabStop>())
			{
				tabStopNodes.push_back(node);
			}
			
			// 子ノードを再帰的に探索
			for (const auto& child : node->children())
			{
				collectTabStopNodes(child, tabStopNodes);
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

				const auto constraintNode = createConstraintNode(targetNode);
				m_inspectorRootNode->addChild(constraintNode);

				const auto nodeSettingNode = createNodeSettingNode(targetNode);
				m_inspectorRootNode->addChild(nodeSettingNode);

				const auto layoutNode = createBoxChildrenLayoutNode(targetNode);
				m_inspectorRootNode->addChild(layoutNode);

				const auto transformEffectNode = createTransformEffectNode(&targetNode->transformEffect());
				m_inspectorRootNode->addChild(transformEffectNode);

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
					MenuItem{ U"TextBox を追加", U"", KeyT, [this] { onClickAddComponent<TextBox>(); } },
					MenuItem{ U"TextArea を追加", U"", KeyA, [this] { onClickAddComponent<TextArea>(); } },
					MenuItem{ U"Label を追加", U"", KeyL, [this] { onClickAddComponent<Label>(); } },
					MenuItem{ U"InputBlocker を追加", U"", KeyI, [this] { onClickAddComponent<InputBlocker>(); } },
					MenuItem{ U"EventTrigger を追加", U"", KeyE, [this] { onClickAddComponent<EventTrigger>(); } },
					MenuItem{ U"Placeholder を追加", U"", KeyP, [this] { onClickAddComponent<Placeholder>(); } },
					MenuItem{ U"CursorChanger を追加", U"", KeyC, [this] { onClickAddComponent<CursorChanger>(); } },
					MenuItem{ U"AudioPlayer を追加", U"", KeyA, [this] { onClickAddComponent<AudioPlayer>(); } },
				};
				
				// コピーされたコンポーネントがある場合は貼り付けメニューを追加
				if (m_copiedComponentType)
				{
					menuElements.push_back(MenuSeparator{});
					menuElements.push_back(MenuItem{ U"{} を貼り付け"_fmt(*m_copiedComponentType), U"", KeyV, [this] { onClickPasteComponentAsNew(); } });
				}
				
				m_inspectorInnerFrameNode->emplaceComponent<ContextMenuOpener>(m_contextMenu, menuElements);

				m_inspectorRootNode->addChild(CreateButtonNode(
					U"＋ コンポーネントを追加(A)",
					BoxConstraint
					{
						.sizeRatio = Vec2{ 1, 0 },
						.sizeDelta = Vec2{ 0, 24 },
						.margin = LRTB{ 0, 0, 24, 24 },
						.maxWidth = 240,
					},
					[this] (const std::shared_ptr<Node>& node)
					{
						m_inspectorInnerFrameNode->getComponent<ContextMenuOpener>()->openManually(node->rect().center());
					}))->addClickHotKey(KeyA, CtrlYN::No, AltYN::Yes, ShiftYN::No, EnabledWhileTextEditingYN::Yes);
			}
			
			// TabStopを持つすべてのノードを収集してリンクを設定
			setupTabStopLinks();
		}

		[[nodiscard]]
		static std::shared_ptr<Node> CreateHeadingNode(StringView name, const ColorF& color, IsFoldedYN isFolded, std::function<void(IsFoldedYN)> onToggleFold = nullptr)
		{
			auto headingNode = Node::Create(U"Heading", BoxConstraint
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
					if (const auto parent = node->parent())
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
						LayoutVariant layout = parent->boxChildrenLayout();
						if (auto pVerticalLayout = std::get_if<VerticalLayout>(&layout))
						{
							pVerticalLayout->padding = inactiveNodeExists ? LRTB::Zero() : LRTB{ 0, 0, 0, 8 };
						}
						parent->setBoxChildrenLayout(layout, RefreshesLayoutYN::No);

						// 高さをフィットさせる
						parent->setBoxConstraintToFitToChildren(FitTarget::HeightOnly, RefreshesLayoutYN::Yes);

						// トグル時処理があれば実行
						if (onToggleFold)
						{
							onToggleFold(IsFoldedYN{ inactiveNodeExists });
						}
					}
				});

			return headingNode;
		}

		class PropertyTextBox : public ComponentBase
		{
		private:
			std::shared_ptr<TextBox> m_textBox;
			std::function<void(StringView)> m_fnSetValue;
			std::function<String()> m_fnGetValue;
			String m_prevExternalValue;

			void update(const std::shared_ptr<Node>&) override
			{
				// 外部からの値の変更をチェック
				if (m_fnGetValue)
				{
					const String currentExternalValue = String(m_fnGetValue());
					if (!m_textBox->isEditing() && currentExternalValue != m_prevExternalValue)
					{
						m_textBox->setText(currentExternalValue, IgnoreIsChangedYN::Yes);
						m_prevExternalValue = currentExternalValue;
					}
				}

				// ユーザーによる変更をチェック
				if (m_textBox->isChanged())
				{
					m_fnSetValue(m_textBox->text());
					if (m_fnGetValue)
					{
						m_prevExternalValue = String{ m_fnGetValue() };
					}
				}
			}

			void draw(const Node&) const override
			{
			}

		public:
			explicit PropertyTextBox(const std::shared_ptr<TextBox>& textBox, std::function<void(StringView)> fnSetValue, std::function<String()> fnGetValue = nullptr)
				: ComponentBase{ {} }
				, m_textBox(textBox)
				, m_fnSetValue(std::move(fnSetValue))
				, m_fnGetValue(std::move(fnGetValue))
				, m_prevExternalValue(m_fnGetValue ? String{ m_fnGetValue() } : U"")
			{
			}
		};

		[[nodiscard]]
		static std::shared_ptr<Node> CreateNodeNameTextboxNode(StringView name, StringView value, std::function<void(StringView)> fnSetValue)
		{
			const auto propertyNode = Node::Create(
				name,
				BoxConstraint
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ -24, 32 },
				},
				IsHitTargetYN::Yes);
			const auto textBoxNode = propertyNode->emplaceChild(
				U"TextBox",
				AnchorConstraint
				{
					.anchorMin = Anchor::MiddleLeft,
					.anchorMax = Anchor::MiddleRight,
					.posDelta = Vec2{ 0, 0 },
					.sizeDelta = Vec2{ -16, 26 },
					.sizeDeltaPivot = Anchor::MiddleCenter,
				});
			textBoxNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"selected", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textBox = textBoxNode->emplaceComponent<TextBox>(U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textBox->setText(value, IgnoreIsChangedYN::Yes);
			textBoxNode->addComponent(std::make_shared<PropertyTextBox>(textBox, std::move(fnSetValue)));
			textBoxNode->emplaceComponent<nocoeditor::TabStop>();
			textBoxNode->addClickHotKey(KeyF2);
			return propertyNode;
		}

		[[nodiscard]]
		std::shared_ptr<Node> createPropertyNodeWithTooltip(StringView componentName, StringView propertyName, StringView value, std::function<void(StringView)> fnSetValue, HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No, std::function<String()> fnGetValue = nullptr)
		{
			// メタデータをチェックしてTextAreaを使うかどうか判断
			std::shared_ptr<Node> propertyNode;
			if (const auto it = m_propertyMetadata.find(PropertyKey{ String{ componentName }, String{ propertyName } }); it != m_propertyMetadata.end())
			{
				const auto& metadata = it->second;
				if (metadata.numTextAreaLines.has_value())
				{
					// TextAreaを使用
					propertyNode = CreatePropertyNodeWithTextArea(propertyName, value, std::move(fnSetValue), hasInteractivePropertyValue, *metadata.numTextAreaLines, std::move(fnGetValue));
				}
				else
				{
					// 通常のTextBoxを使用
					propertyNode = CreatePropertyNode(propertyName, value, std::move(fnSetValue), hasInteractivePropertyValue, std::move(fnGetValue));
				}
				
				// ツールチップを追加
				if (metadata.tooltip)
				{
					if (const auto labelNode = propertyNode->getChildByNameOrNull(U"Label", RecursiveYN::Yes))
					{
						labelNode->emplaceComponent<::TooltipOpener>(m_editorOverlayCanvas, *metadata.tooltip, metadata.tooltipDetail.value_or(U""));
					}
				}
			}
			else
			{
				// メタデータがない場合は通常のTextBoxを使用
				propertyNode = CreatePropertyNode(propertyName, value, std::move(fnSetValue), hasInteractivePropertyValue, std::move(fnGetValue));
			}
			
			return propertyNode;
		}


		[[nodiscard]]
		std::shared_ptr<Node> createVec2PropertyNodeWithTooltip(StringView componentName, StringView propertyName, const Vec2& currentValue, std::function<void(const Vec2&)> fnSetValue, HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No)
		{
			const auto propertyNode = CreateVec2PropertyNode(propertyName, currentValue, std::move(fnSetValue), hasInteractivePropertyValue);
			
			// メタデータに基づいてツールチップを追加
			if (const auto it = m_propertyMetadata.find(PropertyKey{ String{ componentName }, String{ propertyName } }); it != m_propertyMetadata.end())
			{
				const auto& metadata = it->second;
				if (metadata.tooltip)
				{
					if (const auto labelNode = propertyNode->getChildByNameOrNull(U"Label", RecursiveYN::Yes))
					{
						labelNode->emplaceComponent<::TooltipOpener>(m_editorOverlayCanvas, *metadata.tooltip, metadata.tooltipDetail.value_or(U""));
					}
				}
			}
			
			return propertyNode;
		}

		[[nodiscard]]
		std::shared_ptr<Node> createEnumPropertyNodeWithTooltip(StringView componentName, StringView propertyName, StringView value, std::function<void(StringView)> fnSetValue, const std::shared_ptr<ContextMenu>& contextMenu, const Array<String>& enumValues, HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No)
		{
			const auto propertyNode = CreateEnumPropertyNode(propertyName, value, std::move(fnSetValue), contextMenu, enumValues, hasInteractivePropertyValue);
			
			// メタデータに基づいてツールチップを追加
			if (const auto it = m_propertyMetadata.find(PropertyKey{ String{ componentName }, String{ propertyName } }); it != m_propertyMetadata.end())
			{
				const auto& metadata = it->second;
				if (metadata.tooltip)
				{
					if (const auto labelNode = propertyNode->getChildByNameOrNull(U"Label", RecursiveYN::Yes))
					{
						labelNode->emplaceComponent<::TooltipOpener>(m_editorOverlayCanvas, *metadata.tooltip, metadata.tooltipDetail.value_or(U""));
					}
				}
			}
			
			return propertyNode;
		}

		[[nodiscard]]
		std::shared_ptr<Node> createLRTBPropertyNodeWithTooltip(StringView componentName, StringView propertyName, const LRTB& currentValue, std::function<void(const LRTB&)> fnSetValue, HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No)
		{
			const auto propertyNode = CreateLRTBPropertyNode(propertyName, currentValue, std::move(fnSetValue), hasInteractivePropertyValue);
			
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
							labelNode->emplaceComponent<::TooltipOpener>(m_editorOverlayCanvas, *metadata.tooltip, metadata.tooltipDetail.value_or(U""));
						}
					}
					if (const auto line2 = propertyNode->getChildByNameOrNull(U"Line2", RecursiveYN::No))
					{
						if (const auto labelNode = line2->getChildByNameOrNull(U"Label", RecursiveYN::No))
						{
							labelNode->emplaceComponent<::TooltipOpener>(m_editorOverlayCanvas, *metadata.tooltip, metadata.tooltipDetail.value_or(U""));
						}
					}
				}
			}
			
			return propertyNode;
		}

		[[nodiscard]]
		std::shared_ptr<Node> createBoolPropertyNodeWithTooltip(StringView componentName, StringView propertyName, bool currentValue, std::function<void(bool)> fnSetValue, HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No)
		{
			const auto propertyNode = CreateBoolPropertyNode(propertyName, currentValue, std::move(fnSetValue), hasInteractivePropertyValue);
			
			// メタデータに基づいてツールチップを追加
			if (const auto it = m_propertyMetadata.find(PropertyKey{ String{ componentName }, String{ propertyName } }); it != m_propertyMetadata.end())
			{
				const auto& metadata = it->second;
				if (metadata.tooltip)
				{
					// boolプロパティの場合は、propertyNode全体にツールチップを追加
					propertyNode->emplaceComponent<::TooltipOpener>(m_editorOverlayCanvas, *metadata.tooltip, metadata.tooltipDetail.value_or(U""));
				}
			}
			
			return propertyNode;
		}

		[[nodiscard]]
		std::shared_ptr<Node> createColorPropertyNodeWithTooltip(StringView componentName, StringView propertyName, const ColorF& currentValue, std::function<void(const ColorF&)> fnSetValue, HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No)
		{
			const auto propertyNode = CreateColorPropertyNode(propertyName, currentValue, std::move(fnSetValue), hasInteractivePropertyValue);
			
			// メタデータに基づいてツールチップを追加
			if (const auto it = m_propertyMetadata.find(PropertyKey{ String{ componentName }, String{ propertyName } }); it != m_propertyMetadata.end())
			{
				const auto& metadata = it->second;
				if (metadata.tooltip)
				{
					if (const auto labelNode = propertyNode->getChildByNameOrNull(U"Label", RecursiveYN::Yes))
					{
						labelNode->emplaceComponent<::TooltipOpener>(m_editorOverlayCanvas, *metadata.tooltip, metadata.tooltipDetail.value_or(U""));
					}
				}
			}
			
			return propertyNode;
		}

		[[nodiscard]]
		static std::shared_ptr<Node> CreatePropertyNode(StringView name, StringView value, std::function<void(StringView)> fnSetValue, HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No, std::function<String()> fnGetValue = nullptr)
		{
			const auto propertyNode = Node::Create(
				name,
				BoxConstraint
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 32 },
				},
				IsHitTargetYN::Yes,
				InheritChildrenStateFlags::Hovered);
			propertyNode->setBoxChildrenLayout(HorizontalLayout{ .padding = LRTB{ 10, 8, 0, 0 } });
			propertyNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }), Palette::Black, 0.0, 3.0);
			
			const auto labelNode = propertyNode->emplaceChild(
				U"Label",
				BoxConstraint
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 0.85,
				},
				IsHitTargetYN::Yes,
				InheritChildrenStateFlags::Hovered | InheritChildrenStateFlags::Pressed);
			labelNode->setBoxChildrenLayout(HorizontalLayout{});
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
				hasInteractivePropertyValue ? LabelUnderlineStyle::Solid : LabelUnderlineStyle::None,
				ColorF{ Palette::Yellow, 0.5 },
				2.0,
				LabelSizingMode::ShrinkToFit,
				8.0);
			
			const auto textBoxNode = propertyNode->emplaceChild(
				U"TextBox",
				BoxConstraint
				{
					.sizeDelta = Vec2{ 0, 26 },
					.flexibleWeight = 1,
				});
			textBoxNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"selected", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textBox = textBoxNode->emplaceComponent<TextBox>(U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textBox->setText(value, IgnoreIsChangedYN::Yes);
			textBoxNode->addComponent(std::make_shared<PropertyTextBox>(textBox, std::move(fnSetValue), std::move(fnGetValue)));
			textBoxNode->emplaceComponent<nocoeditor::TabStop>();
			return propertyNode;
		}

		[[nodiscard]]
		static std::shared_ptr<Node> CreatePropertyNodeWithTextArea(StringView name, StringView value, std::function<void(StringView)> fnSetValue, HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No, int32 numLines = 3, std::function<String()> fnGetValue = nullptr)
		{
			const double textAreaHeight = numLines * 20.0 + 14.0;
			const double nodeHeight = textAreaHeight + 6.0;
			
			const auto propertyNode = Node::Create(
				name,
				BoxConstraint
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, nodeHeight },
				},
				IsHitTargetYN::Yes,
				InheritChildrenStateFlags::Hovered);
			propertyNode->setBoxChildrenLayout(HorizontalLayout{ .padding = LRTB{ 10, 8, 0, 0 } });
			propertyNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }), Palette::Black, 0.0, 3.0);
			const auto labelNode = propertyNode->emplaceChild(
				U"Label",
				BoxConstraint
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
				hasInteractivePropertyValue ? LabelUnderlineStyle::Solid : LabelUnderlineStyle::None,
				ColorF{ Palette::Yellow, 0.5 },
				2.0,
				LabelSizingMode::ShrinkToFit,
				8.0);
			const auto textAreaNode = propertyNode->emplaceChild(
				U"TextArea",
				BoxConstraint
				{
					.sizeDelta = Vec2{ 0, textAreaHeight },
					.flexibleWeight = 1,
				});
			textAreaNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"selected", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
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

				void draw(const Node&) const override
				{
				}
			};
			
			textAreaNode->addComponent(std::make_shared<PropertyTextArea>(textArea, std::move(fnSetValue), std::move(fnGetValue)));
			textAreaNode->emplaceComponent<nocoeditor::TabStop>();
			return propertyNode;
		}

		[[nodiscard]]
		static std::shared_ptr<Node> CreateVec2PropertyNode(
			StringView name,
			const Vec2& currentValue,
			std::function<void(const Vec2&)> fnSetValue,
			HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No)
		{
			const auto propertyNode = Node::Create(
				name,
				BoxConstraint
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 32 },
				},
				IsHitTargetYN::Yes,
				InheritChildrenStateFlags::Hovered);
			propertyNode->setBoxChildrenLayout(HorizontalLayout{ .padding = LRTB{ 10, 8, 0, 0 } });
			propertyNode->emplaceComponent<RectRenderer>(
				PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }),
				Palette::Black,
				0.0,
				3.0);

			const auto labelNode = propertyNode->emplaceChild(
				U"Label",
				BoxConstraint
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
				hasInteractivePropertyValue ? LabelUnderlineStyle::Solid : LabelUnderlineStyle::None,
				ColorF{ Palette::Yellow, 0.5 },
				2.0,
				LabelSizingMode::ShrinkToFit,
				8.0);

			const auto textBoxParentNode = propertyNode->emplaceChild(
				U"TextBoxParent",
				BoxConstraint
				{
					.sizeDelta = Vec2{ 0, 26 },
					.flexibleWeight = 1,
				},
				IsHitTargetYN::No,
				InheritChildrenStateFlags::Hovered);
			textBoxParentNode->setBoxChildrenLayout(HorizontalLayout{});

			// X
			const auto textBoxXNode = textBoxParentNode->emplaceChild(
				U"TextBoxX",
				BoxConstraint
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 1,
					.margin = LRTB{ 0, 2, 0, 0 },
				});
			textBoxXNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"selected", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textBoxX = textBoxXNode->emplaceComponent<TextBox>(
				U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textBoxXNode->emplaceComponent<nocoeditor::TabStop>();
			textBoxX->setText(Format(currentValue.x), IgnoreIsChangedYN::Yes);

			// Y
			const auto textBoxYNode = textBoxParentNode->emplaceChild(
				U"TextBoxY",
				BoxConstraint
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 1,
					.margin = LRTB{ 2, 0, 0, 0 },
				});
			textBoxYNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"selected", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textBoxY = textBoxYNode->emplaceComponent<TextBox>(
				U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textBoxYNode->emplaceComponent<nocoeditor::TabStop>();
			textBoxY->setText(Format(currentValue.y), IgnoreIsChangedYN::Yes);

			propertyNode->addComponent(std::make_shared<Vec2PropertyTextBox>(
				textBoxX,
				textBoxY,
				fnSetValue,
				currentValue));

			return propertyNode;
		}

		[[nodiscard]]
		static std::shared_ptr<Node> CreateVec4PropertyNode(
			StringView name,
			const Vec4& currentValue,
			std::function<void(const Vec4&)> fnSetValue,
			HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No)
		{
			const auto propertyNode = Node::Create(
				name,
				BoxConstraint
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
				BoxConstraint
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
				hasInteractivePropertyValue ? LabelUnderlineStyle::Solid : LabelUnderlineStyle::None,
				ColorF{ Palette::Yellow, 0.5 },
				2.0,
				LabelSizingMode::ShrinkToFit,
				8.0);

			const auto textBoxParentNode = propertyNode->emplaceChild(
				U"TextBoxParent",
				BoxConstraint
				{
					.sizeDelta = Vec2{ 0, 26 },
					.flexibleWeight = 1,
				},
				IsHitTargetYN::No,
				InheritChildrenStateFlags::Hovered);
			textBoxParentNode->setBoxChildrenLayout(HorizontalLayout{});

			// X
			const auto textBoxXNode = textBoxParentNode->emplaceChild(
				U"TextBoxX",
				BoxConstraint
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 1,
					.margin = LRTB{ 0, 2, 0, 0 },
				});
			textBoxXNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"selected", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textBoxX = textBoxXNode->emplaceComponent<TextBox>(
				U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textBoxXNode->emplaceComponent<nocoeditor::TabStop>();
			textBoxX->setText(Format(currentValue.x), IgnoreIsChangedYN::Yes);

			// Y
			const auto textBoxYNode = textBoxParentNode->emplaceChild(
				U"TextBoxY",
				BoxConstraint
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 1,
					.margin = LRTB{ 2, 2, 0, 0 },
				});
			textBoxYNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"selected", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textBoxY = textBoxYNode->emplaceComponent<TextBox>(
				U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textBoxYNode->emplaceComponent<nocoeditor::TabStop>();
			textBoxY->setText(Format(currentValue.y), IgnoreIsChangedYN::Yes);

			// Z
			const auto textBoxZNode = textBoxParentNode->emplaceChild(
				U"TextBoxZ",
				BoxConstraint
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 1,
					.margin = LRTB{ 2, 2, 0, 0 },
				});
			textBoxZNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"selected", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textBoxZ = textBoxZNode->emplaceComponent<TextBox>(
				U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textBoxZNode->emplaceComponent<nocoeditor::TabStop>();
			textBoxZ->setText(Format(currentValue.z), IgnoreIsChangedYN::Yes);

			// W
			const auto textBoxWNode = textBoxParentNode->emplaceChild(
				U"TextBoxW",
				BoxConstraint
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 1,
					.margin = LRTB{ 2, 0, 0, 0 },
				});
			textBoxWNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"selected", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textBoxW = textBoxWNode->emplaceComponent<TextBox>(
				U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textBoxWNode->emplaceComponent<nocoeditor::TabStop>();
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
			HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No)
		{
			constexpr int32 LineHeight = 32;
			const auto propertyNode = Node::Create(
				name,
				BoxConstraint
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, LineHeight * 2 },
				},
				IsHitTargetYN::Yes,
				InheritChildrenStateFlags::Hovered);
			propertyNode->setBoxChildrenLayout(VerticalLayout{ .padding = LRTB{ 10, 8, 0, 0 } });
			propertyNode->emplaceComponent<RectRenderer>(
				PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }),
				Palette::Black,
				0.0,
				3.0);

			const auto line1 = propertyNode->emplaceChild(
				U"Line1",
				BoxConstraint
				{
					.sizeRatio = Vec2{ 1, 0 },
					.flexibleWeight = 1,
				},
				IsHitTargetYN::No,
				InheritChildrenStateFlags::Hovered);
			line1->setBoxChildrenLayout(HorizontalLayout{});

			const auto line1LabelNode =
				line1->emplaceChild(
					U"Label",
					BoxConstraint
					{
						.sizeRatio = Vec2{ 0, 1 },
						.flexibleWeight = 0.85,
					});
			line1LabelNode->emplaceComponent<Label>(
				U"{} (L, R)"_fmt(name),
				U"",
				14,
				Palette::White,
				HorizontalAlign::Left,
				VerticalAlign::Middle,
				LRTB{ 5, 5, 5, 5 },
				HorizontalOverflow::Wrap,
				VerticalOverflow::Clip,
				Vec2::Zero(),
				hasInteractivePropertyValue ? LabelUnderlineStyle::Solid : LabelUnderlineStyle::None,
				ColorF{ Palette::Yellow, 0.5 },
				2.0,
				LabelSizingMode::ShrinkToFit,
				8.0);

			const auto line1TextBoxParentNode = line1->emplaceChild(
				U"TextBoxParent",
				BoxConstraint
				{
					.sizeDelta = Vec2{ 0, 26 },
					.flexibleWeight = 1,
				},
				IsHitTargetYN::No,
				InheritChildrenStateFlags::Hovered);
			line1TextBoxParentNode->setBoxChildrenLayout(HorizontalLayout{});

			// L
			const auto textBoxLNode = line1TextBoxParentNode->emplaceChild(
				U"TextBoxL",
				BoxConstraint
				{
					.sizeDelta = Vec2{ 0, 26 },
					.flexibleWeight = 1,
					.margin = LRTB{ 0, 2, 0, 6 },
				});
			textBoxLNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"selected", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textBoxL = textBoxLNode->emplaceComponent<TextBox>(
				U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textBoxLNode->emplaceComponent<nocoeditor::TabStop>();
			textBoxL->setText(Format(currentValue.left), IgnoreIsChangedYN::Yes);

			// R
			const auto textBoxRNode = line1TextBoxParentNode->emplaceChild(
				U"TextBoxR",
				BoxConstraint
				{
					.sizeDelta = Vec2{ 0, 26 },
					.flexibleWeight = 1,
					.margin = LRTB{ 2, 0, 0, 6 },
				});
			textBoxRNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"selected", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textBoxR = textBoxRNode->emplaceComponent<TextBox>(
				U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textBoxRNode->emplaceComponent<nocoeditor::TabStop>();
			textBoxR->setText(Format(currentValue.right), IgnoreIsChangedYN::Yes);

			const auto line2 = propertyNode->emplaceChild(
				U"Line2",
				BoxConstraint
				{
					.sizeRatio = Vec2{ 1, 0 },
					.flexibleWeight = 1,
				},
				IsHitTargetYN::No,
				InheritChildrenStateFlags::Hovered);
			line2->setBoxChildrenLayout(HorizontalLayout{});

			const auto line2LabelNode =
				line2->emplaceChild(
					U"Label",
					BoxConstraint
					{
						.sizeRatio = Vec2{ 0, 1 },
						.flexibleWeight = 0.85,
					});
			line2LabelNode->emplaceComponent<Label>(
				U"{} (T, B)"_fmt(name),
				U"",
				14,
				Palette::White,
				HorizontalAlign::Left,
				VerticalAlign::Middle,
				LRTB{ 5, 5, 5, 5 },
				HorizontalOverflow::Wrap,
				VerticalOverflow::Clip,
				Vec2::Zero(),
				hasInteractivePropertyValue ? LabelUnderlineStyle::Solid : LabelUnderlineStyle::None,
				ColorF{ Palette::Yellow, 0.5 },
				2.0,
				LabelSizingMode::ShrinkToFit,
				8.0);

			const auto line2TextBoxParentNode = line2->emplaceChild(
				U"TextBoxParent",
				BoxConstraint
				{
					.sizeDelta = Vec2{ 0, 26 },
					.flexibleWeight = 1,
				},
				IsHitTargetYN::No,
				InheritChildrenStateFlags::Hovered);
			line2TextBoxParentNode->setBoxChildrenLayout(HorizontalLayout{});

			// T
			const auto textBoxTNode = line2TextBoxParentNode->emplaceChild(
				U"TextBoxT",
				BoxConstraint
				{
					.sizeDelta = Vec2{ 0, 26 },
					.flexibleWeight = 1,
					.margin = LRTB{ 0, 2, 0, 0 },
				});
			textBoxTNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"selected", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textBoxT = textBoxTNode->emplaceComponent<TextBox>(
				U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textBoxTNode->emplaceComponent<nocoeditor::TabStop>();
			textBoxT->setText(Format(currentValue.top), IgnoreIsChangedYN::Yes);

			// B
			const auto textBoxBNode = line2TextBoxParentNode->emplaceChild(
				U"TextBoxB",
				BoxConstraint
				{
					.sizeDelta = Vec2{ 0, 26 },
					.flexibleWeight = 1,
					.margin = LRTB{ 2, 0, 0, 0 },
				});
			textBoxBNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"selected", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textBoxB = textBoxBNode->emplaceComponent<TextBox>(
				U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textBoxBNode->emplaceComponent<nocoeditor::TabStop>();
			textBoxB->setText(Format(currentValue.bottom), IgnoreIsChangedYN::Yes);

			propertyNode->addComponent(std::make_shared<LRTBPropertyTextBox>(
				textBoxL,
				textBoxR,
				textBoxT,
				textBoxB,
				fnSetValue,
				currentValue));

			return propertyNode;
		}

		[[nodiscard]]
		static std::shared_ptr<Node> CreateColorPropertyNode(
			StringView name,
			const ColorF& currentValue,
			std::function<void(const ColorF&)> fnSetValue,
			HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No)
		{
			const auto propertyNode = Node::Create(
				name,
				BoxConstraint
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 36 },
				},
				IsHitTargetYN::Yes,
				InheritChildrenStateFlags::Hovered);
			propertyNode->setBoxChildrenLayout(HorizontalLayout{ .padding = LRTB{ 10, 8, 0, 0 } });
			propertyNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }), Palette::Black, 0.0, 3.0);

			const auto labelNode = propertyNode->emplaceChild(
				U"Label",
				BoxConstraint
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
				hasInteractivePropertyValue ? LabelUnderlineStyle::Solid : LabelUnderlineStyle::None,
				ColorF{ Palette::Yellow, 0.5 },
				2.0,
				LabelSizingMode::ShrinkToFit,
				8.0);

			const auto rowNode = propertyNode->emplaceChild(
				U"ColorPropertyRow",
				BoxConstraint
				{
					.sizeDelta = Vec2{ 0, 26 },
					.flexibleWeight = 1,
				},
				IsHitTargetYN::No,
				InheritChildrenStateFlags::Hovered);
			rowNode->setBoxChildrenLayout(HorizontalLayout{});

			const auto previewRootNode = rowNode->emplaceChild(
				U"ColorPreviewRoot",
				BoxConstraint
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
						AnchorConstraint
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
				BoxConstraint
				{
					.sizeRatio = Vec2{ 0, 1 },
					.sizeDelta = Vec2{ 26, 0 },
					.margin = LRTB{ 0, 2, 0, 0 },
				},
				IsHitTargetYN::No);
			const auto previewRectRenderer = previewNode->emplaceComponent<RectRenderer>(currentValue, ColorF{ 1.0, 0.3 }, 1.0, 0.0);

			const auto textBoxParentNode = rowNode->emplaceChild(
				U"TextBoxParent",
				BoxConstraint
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 1,
				},
				IsHitTargetYN::No,
				InheritChildrenStateFlags::Hovered);

			// R
			const auto textBoxRNode = textBoxParentNode->emplaceChild(
				U"TextBoxR",
				BoxConstraint
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 1,
					.margin = LRTB{ 2, 2, 0, 0 },
				});
			textBoxRNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"selected", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textBoxR = textBoxRNode->emplaceComponent<TextBox>(
				U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textBoxRNode->emplaceComponent<nocoeditor::TabStop>();
			textBoxR->setText(Format(currentValue.r), IgnoreIsChangedYN::Yes);

			// G
			const auto textBoxGNode = textBoxParentNode->emplaceChild(
				U"TextBoxG",
				BoxConstraint
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 1,
					.margin = LRTB{ 2, 2, 0, 0 },
				});
			textBoxGNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"selected", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textBoxG = textBoxGNode->emplaceComponent<TextBox>(
				U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textBoxGNode->emplaceComponent<nocoeditor::TabStop>();
			textBoxG->setText(Format(currentValue.g), IgnoreIsChangedYN::Yes);

			// B
			const auto textBoxBNode = textBoxParentNode->emplaceChild(
				U"TextBoxB",
				BoxConstraint
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 1,
					.margin = LRTB{ 2, 2, 0, 0 },
				});
			textBoxBNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"selected", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textBoxB = textBoxBNode->emplaceComponent<TextBox>(
				U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textBoxBNode->emplaceComponent<nocoeditor::TabStop>();
			textBoxB->setText(Format(currentValue.b), IgnoreIsChangedYN::Yes);

			// A
			const auto textBoxANode = textBoxParentNode->emplaceChild(
				U"TextBoxA",
				BoxConstraint
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 1,
					.margin = LRTB{ 2, 0, 0, 0 },
				});
			textBoxANode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"selected", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
			const auto textBoxA = textBoxANode->emplaceComponent<TextBox>(
				U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
			textBoxANode->emplaceComponent<nocoeditor::TabStop>();
			textBoxA->setText(Format(currentValue.a), IgnoreIsChangedYN::Yes);

			propertyNode->addComponent(std::make_shared<ColorPropertyTextBox>(
				textBoxR,
				textBoxG,
				textBoxB,
				textBoxA,
				previewRectRenderer,
				fnSetValue,
				currentValue));

			return propertyNode;
		}

		[[nodiscard]]
		static std::shared_ptr<Node> CreateEnumPropertyNode(
			StringView name,
			StringView currentValue,
			std::function<void(StringView)> fnSetValue,
			const std::shared_ptr<ContextMenu>& contextMenu,
			const Array<String>& enumCandidates,
			HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No)
		{
			const auto propertyNode = Node::Create(
				name,
				BoxConstraint
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 32 },
				},
				IsHitTargetYN::Yes,
				InheritChildrenStateFlags::Hovered);
			propertyNode->setBoxChildrenLayout(HorizontalLayout{ .padding = LRTB{ 10, 8, 0, 0 } });
			propertyNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }), Palette::Black, 0.0, 3.0);

			const auto labelNode =
				propertyNode->emplaceChild(
					U"Label",
					BoxConstraint
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
				hasInteractivePropertyValue ? LabelUnderlineStyle::Solid : LabelUnderlineStyle::None,
				ColorF{ Palette::Yellow, 0.5 },
				2.0,
				LabelSizingMode::ShrinkToFit,
				8.0);

			const auto comboBoxNode = propertyNode->emplaceChild(
				U"ComboBox",
				BoxConstraint
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
				->setSizingMode(LabelSizingMode::ShrinkToFit);

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
				BoxConstraint
				{
					.sizeRatio = Vec2{ 0, 0 },
					.sizeDelta = Vec2{ 18, 18 },
				},
				useParentHoverState ? IsHitTargetYN::No : IsHitTargetYN::Yes);

			checkboxNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"selected", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);

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
			HasInteractivePropertyValueYN hasInteractivePropertyValue = HasInteractivePropertyValueYN::No)
		{
			auto propertyNode = Node::Create(
				name,
				BoxConstraint
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 32 },
				});
			propertyNode->setBoxChildrenLayout(HorizontalLayout{ .padding = LRTB{ 10, 8, 0, 0 } });
			propertyNode->emplaceComponent<RectRenderer>(
				PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }),
				Palette::Black,
				0.0,
				3.0);

			const auto labelNode = propertyNode->emplaceChild(
				U"Label",
				BoxConstraint
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
				hasInteractivePropertyValue ? LabelUnderlineStyle::Solid : LabelUnderlineStyle::None,
				ColorF{ Palette::Yellow, 0.5 },
				2.0,
				LabelSizingMode::ShrinkToFit,
				8.0);

			const auto checkboxParentNode = propertyNode->emplaceChild(
				U"CheckboxParent",
				BoxConstraint
				{
					.sizeRatio = Vec2{ 0, 1 },
					.flexibleWeight = 1,
				},
				IsHitTargetYN::No);
			const auto checkboxNode = CreateCheckboxNode(currentValue, fnSetValue, true);
			checkboxNode->setConstraint(
				AnchorConstraint
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
				BoxConstraint
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 40 },
					.margin = LRTB{ 0, 0, 0, 8 },
				});
			nodeNameNode->setBoxChildrenLayout(HorizontalLayout{ .padding = 6 });
			nodeNameNode->emplaceComponent<RectRenderer>(ColorF{ 0.3, 0.3 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0);

			// Activeチェックボックスを追加
			const auto activeCheckboxNode = CreateCheckboxNode(node->activeSelf().getBool(), [node](bool value) { node->setActive(value); });
			// Activeチェックボックスにツールチップを追加
			{
				const PropertyKey key{ U"Node", U"activeSelf" };
				if (const auto it = m_propertyMetadata.find(key); it != m_propertyMetadata.end())
				{
					const auto& metadata = it->second;
					if (metadata.tooltip)
					{
						activeCheckboxNode->emplaceComponent<::TooltipOpener>(m_editorOverlayCanvas, *metadata.tooltip, metadata.tooltipDetail.value_or(U""));
					}
				}
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
							labelNode->emplaceComponent<::TooltipOpener>(m_editorOverlayCanvas, *metadata.tooltip, metadata.tooltipDetail.value_or(U""));
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
				BoxConstraint
				{
					.sizeRatio = Vec2{ 1, 0 },
					.margin = LRTB{ 0, 0, 0, 8 },
				});
			nodeSettingNode->setBoxChildrenLayout(VerticalLayout{ .padding = m_isFoldedNodeSetting ? LRTB::Zero() : LRTB{ 0, 0, 0, 8 } });
			nodeSettingNode->emplaceComponent<RectRenderer>(ColorF{ 0.3, 0.3 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0);

			nodeSettingNode->addChild(CreateHeadingNode(U"Node Settings", ColorF{ 0.5, 0.3, 0.3 }, m_isFoldedNodeSetting,
				[this](IsFoldedYN isFolded)
				{
					m_isFoldedNodeSetting = isFolded;
				}));

			nodeSettingNode->addChild(
				Node::Create(
					U"TopPadding",
					BoxConstraint
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
			fnAddBoolChild(U"isHitTarget", node->isHitTarget().getBool(), [this, node](bool value) { 
				node->setIsHitTarget(value); 
				refreshInspector();
			});
			// isHitTargetがtrueの場合のみhitTestPaddingを表示
			if (node->isHitTarget())
			{
				fnAddLRTBChild(U"hitTestPadding", node->hitTestPadding(), [node](const LRTB& value) { node->setHitTestPadding(value); });
			}
			fnAddBoolChild(U"inheritsChildrenHoveredState", node->inheritsChildrenHoveredState(), [node](bool value) { node->setInheritsChildrenHoveredState(value); });
			fnAddBoolChild(U"inheritsChildrenPressedState", node->inheritsChildrenPressedState(), [node](bool value) { node->setInheritsChildrenPressedState(value); });
			fnAddBoolChild(U"interactable", node->interactable().getBool(), [node](bool value) { node->setInteractable(value); });
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
						nodeSettingNode->addChild(createPropertyNodeWithTooltip(U"Node", name, Format(currentValue), [fnSetValue = std::move(fnSetValue)](StringView value) { fnSetValue(ParseOpt<double>(value).value_or(0.0)); }))->setActive(!m_isFoldedNodeSetting.getBool());
					};
				fnAddDoubleChild(U"decelerationRate", node->decelerationRate(), [node](double value) { node->setDecelerationRate(Clamp(value, 0.0, 1.0)); });
			}
			// wheelScrollEnabledまたはdragScrollEnabledが有効な場合のみ表示
			if (node->wheelScrollEnabled() || node->dragScrollEnabled())
			{
				fnAddBoolChild(U"rubberBandScrollEnabled", node->rubberBandScrollEnabled().getBool(), [node](bool value) { node->setRubberBandScrollEnabled(value); });
			}
			fnAddBoolChild(U"clippingEnabled", node->clippingEnabled().getBool(), [node](bool value) { node->setClippingEnabled(value); });
			
			// styleState入力欄を追加
			const auto fnAddTextChild = 
				[this, &nodeSettingNode](StringView name, const String& currentValue, auto fnSetValue)
				{
					nodeSettingNode->addChild(createPropertyNodeWithTooltip(U"Node", name, currentValue, fnSetValue))->setActive(!m_isFoldedNodeSetting.getBool());
				};
			fnAddTextChild(U"styleState", node->styleState(), [node](StringView value) { node->setStyleState(String(value)); });

			nodeSettingNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly);

			return nodeSettingNode;
		}

		enum class LayoutType
		{
			FlowLayout,
			HorizontalLayout,
			VerticalLayout,
		};

		[[nodiscard]]
		std::shared_ptr<Node> createBoxChildrenLayoutNode(const std::shared_ptr<Node>& node)
		{
			auto layoutNode = Node::Create(
				U"BoxChildrenLayout",
				BoxConstraint
				{
					.sizeRatio = Vec2{ 1, 0 },
					.margin = LRTB{ 0, 0, 0, 8 },
				});
			layoutNode->setBoxChildrenLayout(VerticalLayout{ .padding = m_isFoldedLayout ? LRTB::Zero() : LRTB{ 0, 0, 0, 8 } });
			layoutNode->emplaceComponent<RectRenderer>(ColorF{ 0.3, 0.3 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0);
			layoutNode->addChild(CreateHeadingNode(U"Box Children Layout", ColorF{ 0.5, 0.3, 0.3 }, m_isFoldedLayout,
				[this](IsFoldedYN isFolded)
				{
					m_isFoldedLayout = isFolded;
				}));
			// 現在のLayoutタイプを取得
			String layoutTypeName;
			if (node->childrenFlowLayout()) layoutTypeName = U"FlowLayout";
			else if (node->childrenHorizontalLayout()) layoutTypeName = U"HorizontalLayout";
			else if (node->childrenVerticalLayout()) layoutTypeName = U"VerticalLayout";
			
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
					layoutNode->addChild(createPropertyNodeWithTooltip(layoutTypeName, name, Format(currentValue), [fnSetValue = std::move(fnSetValue)](StringView value) { fnSetValue(ParseOpt<double>(value).value_or(0.0)); }))->setActive(!m_isFoldedLayout.getBool());
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
							node->setBoxChildrenLayout(HorizontalLayout{});
							refreshInspector(); // 項目に変更があるため更新
							break;
						case LayoutType::VerticalLayout:
							node->setBoxChildrenLayout(VerticalLayout{});
							refreshInspector(); // 項目に変更があるため更新
							break;
						}
					});
				fnAddLRTBChild(U"padding", pFlowLayout->padding, [this, node](const LRTB& value) { auto newLayout = *node->childrenFlowLayout(); newLayout.padding = value; node->setBoxChildrenLayout(newLayout); });
				fnAddVec2Child(U"spacing", pFlowLayout->spacing, [this, node](const Vec2& value) { auto newLayout = *node->childrenFlowLayout(); newLayout.spacing = value; node->setBoxChildrenLayout(newLayout); });
				fnAddEnumChild(U"horizontalAlign", pFlowLayout->horizontalAlign, [this, node](HorizontalAlign value) { auto newLayout = *node->childrenFlowLayout(); newLayout.horizontalAlign = value; node->setBoxChildrenLayout(newLayout); });
				fnAddEnumChild(U"verticalAlign", pFlowLayout->verticalAlign, [this, node](VerticalAlign value) { auto newLayout = *node->childrenFlowLayout(); newLayout.verticalAlign = value; node->setBoxChildrenLayout(newLayout); });
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
							node->setBoxChildrenLayout(FlowLayout{});
							refreshInspector(); // 項目に変更があるため更新
							break;
						case LayoutType::HorizontalLayout:
							break;
						case LayoutType::VerticalLayout:
							node->setBoxChildrenLayout(VerticalLayout{});
							refreshInspector(); // 項目に変更があるため更新
							break;
						}
					});
				fnAddLRTBChild(U"padding", pHorizontalLayout->padding, [this, node](const LRTB& value) { auto newLayout = *node->childrenHorizontalLayout(); newLayout.padding = value; node->setBoxChildrenLayout(newLayout); });
				fnAddDoubleChild(U"spacing", pHorizontalLayout->spacing, [this, node](double value) { auto newLayout = *node->childrenHorizontalLayout(); newLayout.spacing = value; node->setBoxChildrenLayout(newLayout); });
				fnAddEnumChild(U"horizontalAlign", pHorizontalLayout->horizontalAlign, [this, node](HorizontalAlign value) { auto newLayout = *node->childrenHorizontalLayout(); newLayout.horizontalAlign = value; node->setBoxChildrenLayout(newLayout); });
				fnAddEnumChild(U"verticalAlign", pHorizontalLayout->verticalAlign, [this, node](VerticalAlign value) { auto newLayout = *node->childrenHorizontalLayout(); newLayout.verticalAlign = value; node->setBoxChildrenLayout(newLayout); });
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
							node->setBoxChildrenLayout(FlowLayout{});
							refreshInspector(); // 項目に変更があるため更新
							break;
						case LayoutType::HorizontalLayout:
							node->setBoxChildrenLayout(HorizontalLayout{});
							refreshInspector(); // 項目に変更があるため更新
							break;
						case LayoutType::VerticalLayout:
							break;
						}
					});
				fnAddLRTBChild(U"padding", pVerticalLayout->padding, [this, node](const LRTB& value) { auto newLayout = *node->childrenVerticalLayout(); newLayout.padding = value; node->setBoxChildrenLayout(newLayout); });
				fnAddDoubleChild(U"spacing", pVerticalLayout->spacing, [this, node](double value) { auto newLayout = *node->childrenVerticalLayout(); newLayout.spacing = value; node->setBoxChildrenLayout(newLayout); });
				fnAddEnumChild(U"horizontalAlign", pVerticalLayout->horizontalAlign, [this, node](HorizontalAlign value) { auto newLayout = *node->childrenVerticalLayout(); newLayout.horizontalAlign = value; node->setBoxChildrenLayout(newLayout); });
				fnAddEnumChild(U"verticalAlign", pVerticalLayout->verticalAlign, [this, node](VerticalAlign value) { auto newLayout = *node->childrenVerticalLayout(); newLayout.verticalAlign = value; node->setBoxChildrenLayout(newLayout); });
			}
			else
			{
				throw Error{ U"Unknown layout type" };
			}

			layoutNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly);

			return layoutNode;
		}

		[[nodiscard]]
		std::shared_ptr<Node> createConstraintNode(const std::shared_ptr<Node>& node)
		{
			auto constraintNode = Node::Create(
				U"Constraint",
				BoxConstraint
				{
					.sizeRatio = Vec2{ 1, 0 },
					.margin = LRTB{ 0, 0, 0, 8 },
				});
			constraintNode->setBoxChildrenLayout(VerticalLayout{ .padding = m_isFoldedConstraint ? LRTB::Zero() : LRTB{ 0, 0, 0, 8 } });
			constraintNode->emplaceComponent<RectRenderer>(ColorF{ 0.3, 0.3 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0);

			constraintNode->addChild(CreateHeadingNode(U"Constraint", ColorF{ 0.5, 0.3, 0.3 }, m_isFoldedConstraint,
				[this](IsFoldedYN isFolded)
				{
					m_isFoldedConstraint = isFolded;
				}));

			// 現在のConstraintタイプを取得
			const String constraintTypeName = node->boxConstraint() ? U"BoxConstraint" : U"AnchorConstraint";
			
			const auto fnAddChild =
				[this, &constraintNode, &constraintTypeName](StringView name, const auto& value, auto fnSetValue)
				{
					constraintNode->addChild(createPropertyNodeWithTooltip(constraintTypeName, name, Format(value), fnSetValue))->setActive(!m_isFoldedConstraint.getBool());
				};
			const auto fnAddDoubleChild =
				[this, &constraintNode, &constraintTypeName](StringView name, double currentValue, auto fnSetValue)
				{
					constraintNode->addChild(createPropertyNodeWithTooltip(constraintTypeName, name, Format(currentValue), [fnSetValue = std::move(fnSetValue)](StringView value) { fnSetValue(ParseOpt<double>(value).value_or(0.0)); }))->setActive(!m_isFoldedConstraint.getBool());
				};
			const auto fnAddEnumChild =
				[this, &constraintNode, &constraintTypeName]<typename EnumType>(const String & name, EnumType currentValue, auto fnSetValue)
				{
					auto fnSetEnumValue = [fnSetValue = std::move(fnSetValue), currentValue](StringView value) { fnSetValue(StringToEnum<EnumType>(value, currentValue)); };
					constraintNode->addChild(createEnumPropertyNodeWithTooltip(constraintTypeName, name, EnumToString(currentValue), fnSetEnumValue, m_contextMenu, EnumNames<EnumType>()))->setActive(!m_isFoldedConstraint.getBool());
				};
			const auto fnAddVec2Child =
				[this, &constraintNode, &constraintTypeName](StringView name, const Vec2& currentValue, auto fnSetValue)
				{
					constraintNode->addChild(createVec2PropertyNodeWithTooltip(constraintTypeName, name, currentValue, fnSetValue))->setActive(!m_isFoldedConstraint.getBool());
				};
			const auto fnAddOptionalDoubleChild =
				[this, &constraintNode, &constraintTypeName](StringView name, const Optional<double>& currentValue, auto fnSetValue)
				{
					const auto propertyNode = Node::Create(
						name,
						BoxConstraint
						{
							.sizeRatio = Vec2{ 1, 0 },
							.sizeDelta = Vec2{ 0, 32 },
						},
						IsHitTargetYN::Yes,
						InheritChildrenStateFlags::Hovered);
					propertyNode->setBoxChildrenLayout(HorizontalLayout{ .padding = LRTB{ 10, 8, 0, 0 } });
					propertyNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHovered(ColorF{ 1.0, 0.1 }), Palette::Black, 0.0, 3.0);
					
					// ラベル領域（チェックボックスを含む）
					const auto labelNode = propertyNode->emplaceChild(
						U"Label",
						BoxConstraint
						{
							.sizeRatio = Vec2{ 0, 1 },
							.flexibleWeight = 0.85,
						},
						IsHitTargetYN::Yes,
						InheritChildrenStateFlags::Hovered);
					labelNode->setBoxChildrenLayout(HorizontalLayout{ .verticalAlign = VerticalAlign::Middle });
					
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
						->setSizingMode(LabelSizingMode::ShrinkToFit);
					
					// メタデータに基づいてツールチップを追加
					if (const auto it = m_propertyMetadata.find(PropertyKey{ String{ constraintTypeName }, String{ name } }); it != m_propertyMetadata.end())
					{
						const auto& metadata = it->second;
						if (metadata.tooltip)
						{
							labelNode->emplaceComponent<::TooltipOpener>(m_editorOverlayCanvas, *metadata.tooltip, metadata.tooltipDetail.value_or(U""));
						}
					}
					
					// 初期値設定
					const bool hasValue = currentValue.has_value();
					const auto hasValueShared = std::make_shared<bool>(hasValue);
					
					// テキストボックス
					const auto textBoxNode = propertyNode->emplaceChild(
						U"TextBox",
						BoxConstraint
						{
							.sizeDelta = Vec2{ 0, 26 },
							.flexibleWeight = 1,
						});
					textBoxNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withStyleState(U"selected", Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
					const auto textBox = textBoxNode->emplaceComponent<TextBox>(U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
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
					checkboxNode->setConstraint(BoxConstraint
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
					
					propertyNode->setActive(!m_isFoldedConstraint.getBool());
					constraintNode->addChild(propertyNode);
				};

			if (const auto pBoxConstraint = node->boxConstraint())
			{
				fnAddEnumChild(
					U"type",
					ConstraintType::BoxConstraint,
					[this, node](ConstraintType type)
					{
						switch (type)
						{
						case ConstraintType::AnchorConstraint:
							node->setConstraint(AnchorConstraint
							{
								.anchorMin = Anchor::MiddleCenter,
								.anchorMax = Anchor::MiddleCenter,
								.posDelta = Vec2::Zero(),
								.sizeDelta = node->layoutAppliedRect().size,
								.sizeDeltaPivot = Vec2{ 0.5, 0.5 },
							});
							m_defaults->constraintType = ConstraintType::AnchorConstraint; // 次回のデフォルト値として記憶
							refreshInspector(); // 項目に変更があるため更新
							break;
						case ConstraintType::BoxConstraint:
							break;
						}
					});
				fnAddVec2Child(U"sizeRatio", pBoxConstraint->sizeRatio, [this, node](const Vec2& value) { auto newConstraint = *node->boxConstraint(); newConstraint.sizeRatio = value; node->setConstraint(newConstraint); });
				fnAddVec2Child(U"sizeDelta", pBoxConstraint->sizeDelta, [this, node](const Vec2& value) { auto newConstraint = *node->boxConstraint(); newConstraint.sizeDelta = value; node->setConstraint(newConstraint); });
				fnAddDoubleChild(U"flexibleWeight", pBoxConstraint->flexibleWeight, [this, node](double value) { auto newConstraint = *node->boxConstraint(); newConstraint.flexibleWeight = value; node->setConstraint(newConstraint); });
				fnAddVec2Child(U"margin (L, R)", Vec2{ pBoxConstraint->margin.left, pBoxConstraint->margin.right }, [this, node](const Vec2& value) { auto newConstraint = *node->boxConstraint(); newConstraint.margin.left = value.x; newConstraint.margin.right = value.y; node->setConstraint(newConstraint); });
				fnAddVec2Child(U"margin (T, B)", Vec2{ pBoxConstraint->margin.top, pBoxConstraint->margin.bottom }, [this, node](const Vec2& value) { auto newConstraint = *node->boxConstraint(); newConstraint.margin.top = value.x; newConstraint.margin.bottom = value.y; node->setConstraint(newConstraint); });
				
				fnAddOptionalDoubleChild(U"minWidth", pBoxConstraint->minWidth,
					[this, node](const Optional<double>& value) { auto newConstraint = *node->boxConstraint(); newConstraint.minWidth = value; node->setConstraint(newConstraint); });
				fnAddOptionalDoubleChild(U"minHeight", pBoxConstraint->minHeight,
					[this, node](const Optional<double>& value) { auto newConstraint = *node->boxConstraint(); newConstraint.minHeight = value; node->setConstraint(newConstraint); });
				fnAddOptionalDoubleChild(U"maxWidth", pBoxConstraint->maxWidth,
					[this, node](const Optional<double>& value) { auto newConstraint = *node->boxConstraint(); newConstraint.maxWidth = value; node->setConstraint(newConstraint); });
				fnAddOptionalDoubleChild(U"maxHeight", pBoxConstraint->maxHeight,
					[this, node](const Optional<double>& value) { auto newConstraint = *node->boxConstraint(); newConstraint.maxHeight = value; node->setConstraint(newConstraint); });
			}
			else if (const auto pAnchorConstraint = node->anchorConstraint())
			{
				auto setDouble =
					[this, node](auto setter)
					{
						return
							[this, node, setter](StringView s)
							{
								if (auto optVal = ParseOpt<double>(s))
								{
									if (auto ac = node->anchorConstraint())
									{
										auto copy = *ac;
										setter(copy, *optVal);
										node->setConstraint(copy);
										m_canvas->refreshLayout();
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
								if (auto ac = node->anchorConstraint())
								{
									auto copy = *ac;
									setter(copy, val);
									node->setConstraint(copy);
									m_canvas->refreshLayout();
								}
							};
					};

				fnAddEnumChild(
					U"type",
					ConstraintType::AnchorConstraint,
					[this, node](ConstraintType type)
					{
						switch (type)
						{
						case ConstraintType::AnchorConstraint:
							break;
						case ConstraintType::BoxConstraint:
							node->setConstraint(BoxConstraint
							{
								.sizeRatio = Vec2::Zero(),
								.sizeDelta = node->rect().size,
							});
							m_defaults->constraintType = ConstraintType::BoxConstraint; // 次回のデフォルト値として記憶
							refreshInspector(); // 項目に変更があるため更新
							break;
						}
					}
				);

				const AnchorPreset anchorPreset =
					pAnchorConstraint->isCustomAnchorInEditor
						? AnchorPreset::Custom
						: ToAnchorPreset(pAnchorConstraint->anchorMin, pAnchorConstraint->anchorMax, pAnchorConstraint->sizeDeltaPivot);

				fnAddEnumChild(
					U"anchor",
					anchorPreset,
					[this, node](AnchorPreset preset)
					{
						if (const auto pAnchorConstraint = node->anchorConstraint())
						{
							auto copy = *pAnchorConstraint;
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
							if (copy != *pAnchorConstraint)
							{
								if (!copy.isCustomAnchorInEditor)
								{
									const auto beforePreset = ToAnchorPreset(pAnchorConstraint->anchorMin, pAnchorConstraint->anchorMax, pAnchorConstraint->sizeDeltaPivot);

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

								node->setConstraint(copy);
								m_canvas->refreshLayout();
								refreshInspector(); // 項目に変更があるため更新
							}
						}
					}
				);
				switch (anchorPreset)
				{
				case AnchorPreset::TopLeft:
					fnAddChild(U"top", pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = v; }));
					fnAddChild(U"left", pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = v; }));
					fnAddVec2Child(U"size", pAnchorConstraint->sizeDelta, setVec2([](AnchorConstraint& c, const Vec2& v) { c.sizeDelta = v; }));
					break;

				case AnchorPreset::TopCenter:
					fnAddChild(U"top", pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = v; }));
					fnAddVec2Child(U"size", pAnchorConstraint->sizeDelta, setVec2([](AnchorConstraint& c, const Vec2& v) { c.sizeDelta = v; }));
					fnAddChild(U"xDelta", pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = v; }));
					break;

				case AnchorPreset::TopRight:
					fnAddChild(U"top", pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = v; }));
					fnAddChild(U"right", -pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = -v; }));
					fnAddVec2Child(U"size", pAnchorConstraint->sizeDelta, setVec2([](AnchorConstraint& c, const Vec2& v) { c.sizeDelta = v; }));
					break;

				case AnchorPreset::MiddleLeft:
					fnAddChild(U"left", pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = v; }));
					fnAddVec2Child(U"size", pAnchorConstraint->sizeDelta, setVec2([](AnchorConstraint& c, const Vec2& v) { c.sizeDelta = v; }));
					fnAddChild(U"yDelta", pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = v; }));
					break;

				case AnchorPreset::MiddleCenter:
					fnAddVec2Child(U"size", pAnchorConstraint->sizeDelta, setVec2([](AnchorConstraint& c, const Vec2& v) { c.sizeDelta = v; }));
					fnAddVec2Child(U"posDelta", pAnchorConstraint->posDelta, setVec2([](AnchorConstraint& c, const Vec2& v) { c.posDelta = v; }));
					break;

				case AnchorPreset::MiddleRight:
					fnAddChild(U"right", -pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = -v; }));
					fnAddVec2Child(U"size", pAnchorConstraint->sizeDelta, setVec2([](AnchorConstraint& c, const Vec2& v) { c.sizeDelta = v; }));
					fnAddChild(U"yDelta", pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = v; }));
					break;

				case AnchorPreset::BottomLeft:
					fnAddChild(U"left", pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = v; }));
					fnAddChild(U"bottom", -pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = -v; }));
					fnAddVec2Child(U"size", pAnchorConstraint->sizeDelta, setVec2([](AnchorConstraint& c, const Vec2& v) { c.sizeDelta = v; }));
					break;

				case AnchorPreset::BottomCenter:
					fnAddChild(U"bottom", -pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = -v; }));
					fnAddVec2Child(U"size", pAnchorConstraint->sizeDelta, setVec2([](AnchorConstraint& c, const Vec2& v) { c.sizeDelta = v; }));
					fnAddChild(U"xDelta", pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = v; }));
					break;

				case AnchorPreset::BottomRight:
					fnAddChild(U"right", -pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = -v; }));
					fnAddChild(U"bottom", -pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = -v; }));
					fnAddVec2Child(U"size", pAnchorConstraint->sizeDelta, setVec2([](AnchorConstraint& c, const Vec2& v) { c.sizeDelta = v; }));
					break;

				case AnchorPreset::StretchTop:
					fnAddChild(U"top", pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = v; }));
					fnAddChild(U"left", pAnchorConstraint->posDelta.x,
						setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
							{
								double oldLeft = pAnchorConstraint->posDelta.x;
								double delta = oldLeft - v;
								c.posDelta.x = v;
								c.sizeDelta.x += delta;
							}));
					fnAddChild(U"right", -(pAnchorConstraint->posDelta.x + pAnchorConstraint->sizeDelta.x),
						setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
							{
								double oldRight = -(pAnchorConstraint->posDelta.x + pAnchorConstraint->sizeDelta.x);
								double delta = v - oldRight;
								c.sizeDelta.x -= delta;
							}));
					fnAddChild(U"height", pAnchorConstraint->sizeDelta.y, setDouble([](AnchorConstraint& c, double v) { c.sizeDelta.y = v; }));
					fnAddOptionalDoubleChild(U"minWidth", pAnchorConstraint->minWidth, 
						[this, node](const Optional<double>& v) { auto newConstraint = *node->anchorConstraint(); newConstraint.minWidth = v; node->setConstraint(newConstraint); });
					fnAddOptionalDoubleChild(U"maxWidth", pAnchorConstraint->maxWidth, 
						[this, node](const Optional<double>& v) { auto newConstraint = *node->anchorConstraint(); newConstraint.maxWidth = v; node->setConstraint(newConstraint); });
					break;

				case AnchorPreset::StretchMiddle:
					fnAddChild(U"left", pAnchorConstraint->posDelta.x,
						setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
							{
								double oldLeft = pAnchorConstraint->posDelta.x;
								double delta = oldLeft - v;
								c.posDelta.x = v;
								c.sizeDelta.x += delta;
							}));
					fnAddChild(U"right", -(pAnchorConstraint->posDelta.x + pAnchorConstraint->sizeDelta.x),
						setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
							{
								double oldRight = -(pAnchorConstraint->posDelta.x + pAnchorConstraint->sizeDelta.x);
								double delta = v - oldRight;
								c.sizeDelta.x -= delta;
							}));
					fnAddChild(U"height", pAnchorConstraint->sizeDelta.y, setDouble([](AnchorConstraint& c, double v) { c.sizeDelta.y = v; }));
					fnAddChild(U"yDelta", pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = v; }));
					fnAddOptionalDoubleChild(U"minWidth", pAnchorConstraint->minWidth, 
						[this, node](const Optional<double>& v) { auto newConstraint = *node->anchorConstraint(); newConstraint.minWidth = v; node->setConstraint(newConstraint); });
					fnAddOptionalDoubleChild(U"maxWidth", pAnchorConstraint->maxWidth, 
						[this, node](const Optional<double>& v) { auto newConstraint = *node->anchorConstraint(); newConstraint.maxWidth = v; node->setConstraint(newConstraint); });
					break;

				case AnchorPreset::StretchBottom:
					fnAddChild(U"left", pAnchorConstraint->posDelta.x,
						setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
							{
								double oldLeft = pAnchorConstraint->posDelta.x;
								double delta = oldLeft - v;
								c.posDelta.x = v;
								c.sizeDelta.x += delta;
							}));
					fnAddChild(U"right", -(pAnchorConstraint->posDelta.x + pAnchorConstraint->sizeDelta.x),
						setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
							{
								double oldRight = -(pAnchorConstraint->posDelta.x + pAnchorConstraint->sizeDelta.x);
								double delta = v - oldRight;
								c.sizeDelta.x -= delta;
							}));
					fnAddChild(U"bottom", pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = -v; }));
					fnAddChild(U"height", pAnchorConstraint->sizeDelta.y, setDouble([](AnchorConstraint& c, double v) { c.sizeDelta.y = v; }));
					fnAddOptionalDoubleChild(U"minWidth", pAnchorConstraint->minWidth, 
						[this, node](const Optional<double>& v) { auto newConstraint = *node->anchorConstraint(); newConstraint.minWidth = v; node->setConstraint(newConstraint); });
					fnAddOptionalDoubleChild(U"maxWidth", pAnchorConstraint->maxWidth, 
						[this, node](const Optional<double>& v) { auto newConstraint = *node->anchorConstraint(); newConstraint.maxWidth = v; node->setConstraint(newConstraint); });
					break;

				case AnchorPreset::StretchLeft:
					fnAddChild(U"top", pAnchorConstraint->posDelta.y,
						setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
							{
								double oldTop = pAnchorConstraint->posDelta.y;
								double delta = oldTop - v;
								c.posDelta.y = v;
								c.sizeDelta.y += delta;
							}));
					fnAddChild(U"bottom", -(pAnchorConstraint->posDelta.y + pAnchorConstraint->sizeDelta.y),
						setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
							{
								double oldBottom = -(pAnchorConstraint->posDelta.y + pAnchorConstraint->sizeDelta.y);
								double delta = v - oldBottom;
								c.sizeDelta.y -= delta;
							}));
					fnAddChild(U"left", pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = v; }));
					fnAddChild(U"width", pAnchorConstraint->sizeDelta.x, setDouble([](AnchorConstraint& c, double v) { c.sizeDelta.x = v; }));
					fnAddOptionalDoubleChild(U"minHeight", pAnchorConstraint->minHeight, 
						[this, node](const Optional<double>& v) { auto newConstraint = *node->anchorConstraint(); newConstraint.minHeight = v; node->setConstraint(newConstraint); });
					fnAddOptionalDoubleChild(U"maxHeight", pAnchorConstraint->maxHeight, 
						[this, node](const Optional<double>& v) { auto newConstraint = *node->anchorConstraint(); newConstraint.maxHeight = v; node->setConstraint(newConstraint); });
					break;

				case AnchorPreset::StretchCenter:
					fnAddChild(U"top", pAnchorConstraint->posDelta.y,
						setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
							{
								double oldTop = pAnchorConstraint->posDelta.y;
								double delta = oldTop - v;
								c.posDelta.y = v;
								c.sizeDelta.y += delta;
							}));
					fnAddChild(U"bottom", -(pAnchorConstraint->posDelta.y + pAnchorConstraint->sizeDelta.y),
						setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
							{
								double oldBottom = -(pAnchorConstraint->posDelta.y + pAnchorConstraint->sizeDelta.y);
								double delta = v - oldBottom;
								c.sizeDelta.y -= delta;
							}));
					fnAddChild(U"width", pAnchorConstraint->sizeDelta.x, setDouble([](AnchorConstraint& c, double v) { c.sizeDelta.x = v; }));
					fnAddChild(U"xDelta", pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = v; }));
					fnAddOptionalDoubleChild(U"minHeight", pAnchorConstraint->minHeight, 
						[this, node](const Optional<double>& v) { auto newConstraint = *node->anchorConstraint(); newConstraint.minHeight = v; node->setConstraint(newConstraint); });
					fnAddOptionalDoubleChild(U"maxHeight", pAnchorConstraint->maxHeight, 
						[this, node](const Optional<double>& v) { auto newConstraint = *node->anchorConstraint(); newConstraint.maxHeight = v; node->setConstraint(newConstraint); });
					break;

				case AnchorPreset::StretchRight:
					fnAddChild(U"top", pAnchorConstraint->posDelta.y,
						setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
							{
								double oldTop = pAnchorConstraint->posDelta.y;
								double delta = oldTop - v;
								c.posDelta.y = v;
								c.sizeDelta.y += delta;
							}));
					fnAddChild(U"bottom", -(pAnchorConstraint->posDelta.y + pAnchorConstraint->sizeDelta.y),
						setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
							{
								double oldBottom = -(pAnchorConstraint->posDelta.y + pAnchorConstraint->sizeDelta.y);
								double delta = v - oldBottom;
								c.sizeDelta.y -= delta;
							}));
					fnAddChild(U"right", pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = -v; }));
					fnAddChild(U"width", pAnchorConstraint->sizeDelta.x, setDouble([](AnchorConstraint& c, double v) { c.sizeDelta.x = v; }));
					fnAddOptionalDoubleChild(U"minHeight", pAnchorConstraint->minHeight, 
						[this, node](const Optional<double>& v) { auto newConstraint = *node->anchorConstraint(); newConstraint.minHeight = v; node->setConstraint(newConstraint); });
					fnAddOptionalDoubleChild(U"maxHeight", pAnchorConstraint->maxHeight, 
						[this, node](const Optional<double>& v) { auto newConstraint = *node->anchorConstraint(); newConstraint.maxHeight = v; node->setConstraint(newConstraint); });
					break;

				case AnchorPreset::StretchFull:
					fnAddChild(U"left", pAnchorConstraint->posDelta.x,
						setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
							{
								double oldLeft = pAnchorConstraint->posDelta.x;
								double delta = oldLeft - v;
								c.posDelta.x = v;
								c.sizeDelta.x += delta;
							}));
					fnAddChild(U"right", -(pAnchorConstraint->posDelta.x + pAnchorConstraint->sizeDelta.x),
						setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
							{
								double oldRight = -(pAnchorConstraint->posDelta.x + pAnchorConstraint->sizeDelta.x);
								double delta = v - oldRight;
								c.sizeDelta.x -= delta;
							}));
					fnAddChild(U"top", pAnchorConstraint->posDelta.y,
						setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
							{
								double oldTop = pAnchorConstraint->posDelta.y;
								double delta = oldTop - v;
								c.posDelta.y = v;
								c.sizeDelta.y += delta;
							}));
					fnAddChild(U"bottom", -(pAnchorConstraint->posDelta.y + pAnchorConstraint->sizeDelta.y),
						setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
							{
								double oldBottom = -(pAnchorConstraint->posDelta.y + pAnchorConstraint->sizeDelta.y);
								double delta = v - oldBottom;
								c.sizeDelta.y -= delta;
							}));
					fnAddOptionalDoubleChild(U"minWidth", pAnchorConstraint->minWidth, 
						[this, node](const Optional<double>& v) { auto newConstraint = *node->anchorConstraint(); newConstraint.minWidth = v; node->setConstraint(newConstraint); });
					fnAddOptionalDoubleChild(U"minHeight", pAnchorConstraint->minHeight, 
						[this, node](const Optional<double>& v) { auto newConstraint = *node->anchorConstraint(); newConstraint.minHeight = v; node->setConstraint(newConstraint); });
					fnAddOptionalDoubleChild(U"maxWidth", pAnchorConstraint->maxWidth, 
						[this, node](const Optional<double>& v) { auto newConstraint = *node->anchorConstraint(); newConstraint.maxWidth = v; node->setConstraint(newConstraint); });
					fnAddOptionalDoubleChild(U"maxHeight", pAnchorConstraint->maxHeight, 
						[this, node](const Optional<double>& v) { auto newConstraint = *node->anchorConstraint(); newConstraint.maxHeight = v; node->setConstraint(newConstraint); });
					break;

				default:
					fnAddVec2Child(U"anchorMin", pAnchorConstraint->anchorMin, setVec2([](AnchorConstraint& c, const Vec2& val) { c.anchorMin = val; }));
					fnAddVec2Child(U"anchorMax", pAnchorConstraint->anchorMax, setVec2([](AnchorConstraint& c, const Vec2& val) { c.anchorMax = val; }));
					fnAddVec2Child(U"sizeDeltaPivot", pAnchorConstraint->sizeDeltaPivot, setVec2([](AnchorConstraint& c, const Vec2& val) { c.sizeDeltaPivot = val; }));
					fnAddVec2Child(U"posDelta", pAnchorConstraint->posDelta, setVec2([](AnchorConstraint& c, const Vec2& val) { c.posDelta = val; }));
					fnAddVec2Child(U"sizeDelta", pAnchorConstraint->sizeDelta, setVec2([](AnchorConstraint& c, const Vec2& val) { c.sizeDelta = val; }));
					fnAddOptionalDoubleChild(U"minWidth", pAnchorConstraint->minWidth, 
						[this, node](const Optional<double>& v) { auto newConstraint = *node->anchorConstraint(); newConstraint.minWidth = v; node->setConstraint(newConstraint); });
					fnAddOptionalDoubleChild(U"minHeight", pAnchorConstraint->minHeight, 
						[this, node](const Optional<double>& v) { auto newConstraint = *node->anchorConstraint(); newConstraint.minHeight = v; node->setConstraint(newConstraint); });
					fnAddOptionalDoubleChild(U"maxWidth", pAnchorConstraint->maxWidth, 
						[this, node](const Optional<double>& v) { auto newConstraint = *node->anchorConstraint(); newConstraint.maxWidth = v; node->setConstraint(newConstraint); });
					fnAddOptionalDoubleChild(U"maxHeight", pAnchorConstraint->maxHeight, 
						[this, node](const Optional<double>& v) { auto newConstraint = *node->anchorConstraint(); newConstraint.maxHeight = v; node->setConstraint(newConstraint); });
					break;
				}
			}
			else
			{
				throw Error{ U"Unknown constraint type" };
			}

			constraintNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly);

			return constraintNode;
		}

		[[nodiscard]]
		std::shared_ptr<Node> createTransformEffectNode(TransformEffect* const pTransformEffect)
		{
			auto transformEffectNode = Node::Create(
				U"TransformEffect",
				BoxConstraint
				{
					.sizeRatio = Vec2{ 1, 0 },
					.margin = LRTB{ 0, 0, 0, 8 },
				});
			transformEffectNode->setBoxChildrenLayout(VerticalLayout{ .padding = m_isFoldedTransformEffect ? LRTB::Zero() : LRTB{ 0, 0, 0, 8 } });
			transformEffectNode->emplaceComponent<RectRenderer>(ColorF{ 0.3, 0.3 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0);

			transformEffectNode->addChild(CreateHeadingNode(U"TransformEffect", ColorF{ 0.3, 0.5, 0.3 }, m_isFoldedTransformEffect,
				[this](IsFoldedYN isFolded)
				{
					m_isFoldedTransformEffect = isFolded;
				}));

			const auto fnAddVec2Child =
				[this, &transformEffectNode](StringView name, SmoothProperty<Vec2>* pProperty, auto fnSetValue)
				{
					const auto propertyNode = transformEffectNode->addChild(createVec2PropertyNodeWithTooltip(U"TransformEffect", name, pProperty->propertyValue().defaultValue, fnSetValue, HasInteractivePropertyValueYN{ pProperty->hasInteractivePropertyValue() }));
					propertyNode->setActive(!m_isFoldedTransformEffect.getBool());
					
					Array<MenuElement> menuElements
					{
						MenuItem{ U"ステート毎に値を変更..."_fmt(name), U"", KeyC, [this, pProperty] { m_dialogOpener->openDialog(std::make_shared<InteractivePropertyValueDialog>(pProperty, [this] { refreshInspector(); }, m_dialogOpener)); } },
					};
					
					propertyNode->template emplaceComponent<ContextMenuOpener>(m_contextMenu, menuElements, nullptr, RecursiveYN::Yes);
				};
			// Note: アクセサからポインタを取得しているので注意が必要
			fnAddVec2Child(U"position", &pTransformEffect->position(), [this, pTransformEffect](const Vec2& value) { pTransformEffect->setPosition(value); m_canvas->refreshLayout(); });
			fnAddVec2Child(U"scale", &pTransformEffect->scale(), [this, pTransformEffect](const Vec2& value) { pTransformEffect->setScale(value); m_canvas->refreshLayout(); });
			fnAddVec2Child(U"pivot", &pTransformEffect->pivot(), [this, pTransformEffect](const Vec2& value) { pTransformEffect->setPivot(value); m_canvas->refreshLayout(); });
			
			const auto fnAddBoolChild =
				[this, &transformEffectNode](StringView name, Property<bool>* pProperty, auto fnSetValue)
				{
					const auto propertyNode = transformEffectNode->addChild(createBoolPropertyNodeWithTooltip(U"TransformEffect", name, pProperty->propertyValue().defaultValue, fnSetValue, HasInteractivePropertyValueYN{ pProperty->hasInteractivePropertyValue() }));
					propertyNode->setActive(!m_isFoldedTransformEffect.getBool());
					propertyNode->template emplaceComponent<ContextMenuOpener>(m_contextMenu, Array<MenuElement>{ MenuItem{ U"ステート毎に値を変更..."_fmt(name), U"", KeyC, [this, pProperty] { m_dialogOpener->openDialog(std::make_shared<InteractivePropertyValueDialog>(pProperty, [this] { refreshInspector(); }, m_dialogOpener)); } } }, nullptr, RecursiveYN::Yes);
				};
			fnAddBoolChild(U"appliesToHitTest", &pTransformEffect->appliesToHitTest(), [this, pTransformEffect](bool value) { pTransformEffect->setAppliesToHitTest(value); });

			const auto fnAddColorChild =
				[this, &transformEffectNode](StringView name, SmoothProperty<ColorF>* pProperty, auto fnSetValue)
				{
					const auto propertyNode = transformEffectNode->addChild(createColorPropertyNodeWithTooltip(U"TransformEffect", name, pProperty->propertyValue().defaultValue, fnSetValue, HasInteractivePropertyValueYN{ pProperty->hasInteractivePropertyValue() }));
					propertyNode->setActive(!m_isFoldedTransformEffect.getBool());
					
					Array<MenuElement> menuElements
					{
						MenuItem{ U"ステート毎に値を変更..."_fmt(name), U"", KeyC, [this, pProperty] { m_dialogOpener->openDialog(std::make_shared<InteractivePropertyValueDialog>(pProperty, [this] { refreshInspector(); }, m_dialogOpener)); } },
					};
					
					propertyNode->template emplaceComponent<ContextMenuOpener>(m_contextMenu, menuElements, nullptr, RecursiveYN::Yes);
				};
			fnAddColorChild(U"color", &pTransformEffect->color(), [this, pTransformEffect](const ColorF& value) { pTransformEffect->setColor(value); });

			transformEffectNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly);

			return transformEffectNode;
		}

		[[nodiscard]]
		std::shared_ptr<Node> createComponentNode(const std::shared_ptr<Node>& node, const std::shared_ptr<SerializableComponentBase>& component, IsFoldedYN isFolded, std::function<void(IsFoldedYN)> onToggleFold)
		{
			auto componentNode = Node::Create(
				component->type(),
				BoxConstraint
				{
					.sizeRatio = Vec2{ 1, 0 },
					.margin = LRTB{ 0, 0, 0, 8 },
				});
			componentNode->setBoxChildrenLayout(VerticalLayout{ .padding = isFolded ? LRTB::Zero() : LRTB{ 0, 0, 0, 8 } });
			componentNode->emplaceComponent<RectRenderer>(ColorF{ 0.3, 0.3 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0);

			const auto headingNode = componentNode->addChild(CreateHeadingNode(component->type(), ColorF{ 0.3, 0.3, 0.5 }, isFolded, std::move(onToggleFold)));
			Array<MenuElement> menuElements;
			menuElements.push_back(MenuItem{ U"{} を削除"_fmt(component->type()), U"", KeyR, [this, node, component] { node->removeComponent(component); refreshInspector(); } });
			menuElements.push_back(MenuSeparator{});
			menuElements.push_back(MenuItem{ U"{} を上へ移動"_fmt(component->type()), U"", KeyU, [this, node, component] { node->moveComponentUp(component); refreshInspector(); } });
			menuElements.push_back(MenuItem{ U"{} を下へ移動"_fmt(component->type()), U"", KeyD, [this, node, component] { node->moveComponentDown(component); refreshInspector(); } });
			menuElements.push_back(MenuSeparator{});
			menuElements.push_back(MenuItem{ U"{} の内容をコピー"_fmt(component->type()), U"", KeyC, [this, component] { onClickCopyComponent(component); } });
			
			// 同じタイプのコンポーネントがコピーされている場合のみ貼り付けメニューを表示
			if (m_copiedComponentType.has_value() && *m_copiedComponentType == component->type())
			{
				menuElements.push_back(MenuItem{ U"{} の内容を貼り付け"_fmt(component->type()), U"", KeyV, [this, component] { onClickPasteComponentTo(component); } });
			}
			
			headingNode->emplaceComponent<ContextMenuOpener>(m_contextMenu, menuElements);

			if (component->properties().empty())
			{
				const auto noPropertyLabelNode = componentNode->emplaceChild(
					U"NoProperty",
					BoxConstraint
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
								HasInteractivePropertyValueYN{ property->hasInteractivePropertyValue() }));
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
								HasInteractivePropertyValueYN{ property->hasInteractivePropertyValue() }));
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
								ParseOr<ColorF>(property->propertyValueStringOfDefault(), ColorF{ 0, 0, 0, 1 }),
								onChange,
								HasInteractivePropertyValueYN{ property->hasInteractivePropertyValue() }));
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
								HasInteractivePropertyValueYN{ property->hasInteractivePropertyValue() }));
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
								HasInteractivePropertyValueYN{ property->hasInteractivePropertyValue() }));
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
				
				if (property->isInteractiveProperty())
				{
					propertyNode->emplaceComponent<ContextMenuOpener>(
						m_contextMenu,
						Array<MenuElement>
						{
							MenuItem{ U"ステート毎に値を変更..."_fmt(property->name()), U"", KeyC, [this, property] { m_dialogOpener->openDialog(std::make_shared<InteractivePropertyValueDialog>(property, [this] { refreshInspector(); }, m_dialogOpener)); } },
						},
						nullptr,
						RecursiveYN::Yes);
				}
			}

			// Spriteコンポーネントの場合、スナップボタンを追加
			// TODO: コンポーネント毎のカスタムInspectorを追加するための仕組みを整備する
			if (auto sprite = std::dynamic_pointer_cast<Sprite>(component))
			{
				auto snapButton = componentNode->addChild(CreateButtonNode(
					U"テクスチャサイズへスナップ",
					BoxConstraint
					{
						.sizeRatio = Vec2{ 1, 0 },
						.sizeDelta = Vec2{ -24, 24 },
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

			componentNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly);

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
			if (auto* constraint = m_inspectorFrameNode->anchorConstraint())
			{
				const_cast<AnchorConstraint*>(constraint)->sizeDelta.x = width;
			}
		}
	};
}
