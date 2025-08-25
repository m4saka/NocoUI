#pragma once
#include <NocoUI.hpp>
#include "ContextMenu.hpp"
#include "Defaults.hpp"
#include "EditorYN.hpp"
#include "EditorEnums.hpp"

namespace noco::editor
{
	class Hierarchy
	{
	private:
		std::shared_ptr<Canvas> m_canvas;
		std::shared_ptr<Node> m_hierarchyFrameNode;
		std::shared_ptr<Node> m_hierarchyInnerFrameNode;
		std::shared_ptr<Node> m_hierarchyRootNode;
		std::shared_ptr<Node> m_hierarchyTailNode;
		std::weak_ptr<Canvas> m_editorCanvas;
		std::weak_ptr<Node> m_editorHoveredNode;
		std::weak_ptr<Node> m_shiftSelectOriginNode;
		std::weak_ptr<Node> m_lastEditorSelectedNode;
		std::weak_ptr<Node> m_prevCheckedSelectedNode;
		bool m_prevSelectedNodeExists = false;
		std::shared_ptr<ContextMenu> m_contextMenu;
		Array<JSON> m_copiedNodeJSONs;
		bool m_prevClipboardHasContent = false;
		std::shared_ptr<Defaults> m_defaults;
		std::shared_ptr<DialogOpener> m_dialogOpener;

		struct ElementDetail
		{
			size_t nestLevel = 0;
			std::shared_ptr<Node> node;
			std::shared_ptr<Node> hierarchyNode;
			std::shared_ptr<RectRenderer> hierarchyRectRenderer;
			std::shared_ptr<Label> hierarchyStateLabel;
			std::shared_ptr<Node> hierarchyToggleFoldedNode;
			std::shared_ptr<Label> hierarchyToggleFoldedLabel;
		};

		class Element
		{
		private:
			Hierarchy* m_pHierarchy = nullptr;
			ElementDetail m_elementDetail;
			EditorSelectedYN m_editorSelected = EditorSelectedYN::No;
			FoldedYN m_folded = FoldedYN::No;

		public:
			Element(Hierarchy* pHierarchy, const ElementDetail& elementDetail)
				: m_pHierarchy{ pHierarchy }
				, m_elementDetail{ elementDetail }
			{
				if (m_pHierarchy == nullptr)
				{
					throw Error{ U"Hierarchy is nullptr" };
				}
			}

			[[nodiscard]]
			EditorSelectedYN editorSelected() const
			{
				return m_editorSelected;
			}

			void setEditorSelected(EditorSelectedYN editorSelected)
			{
				m_editorSelected = editorSelected;
				m_elementDetail.hierarchyRectRenderer->setFillColor(HierarchyRectFillColor(m_editorSelected));
				m_elementDetail.hierarchyRectRenderer->setOutlineColor(HierarchyRectOutlineColor(m_editorSelected));
			}

			[[nodiscard]]
			const ElementDetail& elementDetail() const
			{
				return m_elementDetail;
			}

			[[nodiscard]]
			const std::shared_ptr<Node>& node() const
			{
				return m_elementDetail.node;
			}

			[[nodiscard]]
			const std::shared_ptr<Node>& hierarchyNode() const
			{
				return m_elementDetail.hierarchyNode;
			}

			void toggleFolded()
			{
				setFolded(m_folded ? FoldedYN::No : FoldedYN::Yes);
			}

			void setFolded(FoldedYN folded)
			{
				m_folded = folded;
				if (m_folded)
				{
					m_elementDetail.hierarchyToggleFoldedLabel->setText(U"▶");
				}
				else
				{
					m_elementDetail.hierarchyToggleFoldedLabel->setText(U"▼");
				}
				m_pHierarchy->applyFolding();
			}

			[[nodiscard]]
			FoldedYN folded() const
			{
				return m_folded;
			}

			[[nodiscard]]
			static PropertyValue<ColorF> HierarchyRectFillColor(EditorSelectedYN editorSelected)
			{
				if (editorSelected)
				{
					return ColorF{ Palette::Orange, 0.3 };
				}
				else
				{
					return PropertyValue<ColorF>{ ColorF{ 1.0, 0.0 } }.withHovered(ColorF{ 1.0, 0.2 });
				}
			}

			[[nodiscard]]
			static PropertyValue<ColorF> HierarchyRectOutlineColor(EditorSelectedYN editorSelected)
			{
				if (editorSelected)
				{
					return ColorF{ Palette::Orange, 0.6 };
				}
				else
				{
					return PropertyValue<ColorF>{ ColorF{ 1.0, 0.0 } }.withHovered(ColorF{ 1.0, 0.6 });
				}
			}
		};
		Array<Element> m_elements;

		void addElementRecursive(const std::shared_ptr<Node>& node, size_t nestLevel, RefreshesLayoutYN refreshesLayout)
		{
			if (node == nullptr)
			{
				throw Error{ U"Node is nullptr" };
			}

			m_elements.push_back(createElement(node, nestLevel));
			m_hierarchyRootNode->addChild(m_elements.back().elementDetail().hierarchyNode, RefreshesLayoutYN::No);

			for (const auto& child : node->children())
			{
				addElementRecursive(child, nestLevel + 1, RefreshesLayoutYN::No);
			}

			if (refreshesLayout)
			{
				m_canvas->refreshLayout();
			}
		}

		Element createElement(const std::shared_ptr<Node>& node, size_t nestLevel)
		{
			const auto hierarchyNode = Node::Create(
				U"Element",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 24 },
				},
				IsHitTargetYN::Yes,
				InheritChildrenStateFlags::Hovered);
			hierarchyNode->emplaceComponent<ContextMenuOpener>(
				m_contextMenu,
				Array<MenuElement>
				{
					MenuItem{ U"新規ノード", U"", KeyN, [this] { onClickNewNode(); } },
					MenuItem{ U"子として新規ノード", U"", KeyE, [this, node] { onClickNewNode(node); } },
					MenuSeparator{},
					MenuItem{ U"切り取り", U"Ctrl+X", KeyT, [this] { onClickCut(); } },
					MenuItem{ U"コピー", U"Ctrl+C", KeyC, [this] { onClickCopy(); } },
					MenuItem{ U"貼り付け", U"Ctrl+V", KeyP, [this] { onClickPaste(); }, [this] { return canPaste(); } },
					MenuItem{ U"子として貼り付け", U"", KeyA, [this, node] { onClickPaste(node); }, [this] { return canPaste(); } },
					MenuItem{ U"複製を作成", U"Ctrl+D", KeyL, [this] { onClickDuplicate(); } },
					MenuItem{ U"削除", U"Delete", none, [this] { onClickDelete(); } },
					MenuSeparator{},
					MenuItem{ U"上に移動", U"Alt+Up", KeyU, [this] { onClickMoveUp(); } },
					MenuItem{ U"下に移動", U"Alt+Down", KeyD, [this] { onClickMoveDown(); } },
					MenuSeparator{},
					MenuItem{ U"空の親ノードを作成", U"", KeyM, [this] { onClickCreateEmptyParent(); } },
				},
				[this, nodeWeak = std::weak_ptr{ node }]
				{
					const std::shared_ptr<Node> node = nodeWeak.lock();
					if (!node)
					{
						return;
					}

					Element* const pElement = getElementByNode(node);
					if (pElement == nullptr)
					{
						throw Error{ U"Element not found" };
					}
					Element& element = *pElement;

					// 既に選択中の場合は何もしない
					if (element.editorSelected())
					{
						return;
					}
					clearSelection();
					element.setEditorSelected(EditorSelectedYN::Yes);
					m_lastEditorSelectedNode = node;
					m_shiftSelectOriginNode = node;
				});
			hierarchyNode->emplaceComponent<RectRenderer>(Element::HierarchyRectFillColor(EditorSelectedYN::No), Element::HierarchyRectOutlineColor(EditorSelectedYN::No), 1.0, 3.0);
			hierarchyNode->emplaceComponent<DragDropSource>([this, hierarchyNode]() -> Array<std::shared_ptr<Node>>
				{
					// 未選択のノードをドラッグ開始した場合は単一選択
					if (const auto pElement = getElementByHierarchyNode(hierarchyNode))
					{
						if (!pElement->editorSelected())
						{
							selectSingleNode(pElement->node());
						}
					}

					// 選択中ノードを返す(親子関係にあるノードは子ノードを除く)
					return getSelectedNodesExcludingChildren().map([this](const auto& node) { return getElementByNode(node)->hierarchyNode(); });
				});

			constexpr double MoveAsSiblingThresholdPixels = 6.0;
			hierarchyNode->emplaceComponent<DragDropTarget>([this, hierarchyNode](const Array<std::shared_ptr<Node>>& sourceNodes)
				{
					const auto pTargetElement = getElementByHierarchyNode(hierarchyNode);
					if (pTargetElement == nullptr)
					{
						return;
					}
					const auto& targetElement = *pTargetElement;

					Array<std::shared_ptr<Node>> newSelection;
					newSelection.reserve(sourceNodes.size());

					const auto rect = hierarchyNode->regionRect();
					const auto mouseX = Cursor::PosF().x;
				
					// X座標による階層の判定
					const double desiredNestLevel = Math::Max(0.0, (mouseX - rect.x - 15) / 20.0);
				
					if (const auto moveAsSiblingRectTop = RectF{ rect.x, rect.y, rect.w, MoveAsSiblingThresholdPixels };
						moveAsSiblingRectTop.mouseOver())
					{
						// targetの上に兄弟ノードとして移動
						// X座標に基づいて、適切な親ノードを見つける
						std::shared_ptr<Node> moveToParent = targetElement.node()->parentNode();
						size_t actualNestLevel = targetElement.elementDetail().nestLevel;
					
						// マウスが左側にある場合、より上位の階層に移動
						while (moveToParent && actualNestLevel > desiredNestLevel)
						{
							const auto grandParent = moveToParent->parentNode();
							if (!grandParent)
							{
								break;
							}
							moveToParent = grandParent;
							actualNestLevel--;
						}
					
						for (const auto& sourceNode : sourceNodes)
						{
							const auto pSourceElement = getElementByHierarchyNode(sourceNode);
							if (pSourceElement == nullptr)
							{
								return;
							}
							const auto& sourceElement = *pSourceElement;
							if (sourceElement.node() == targetElement.node())
							{
								// 自分自身には移動不可
								return;
							}
							if (sourceElement.node()->isAncestorOf(targetElement.node()))
							{
								// 子孫には移動不可
								return;
							}
						
								sourceElement.node()->removeFromParent();
						
							// 移動先での挿入位置を計算
							if (!moveToParent)
							{
									m_canvas->addChild(sourceElement.node());
							}
							else if (moveToParent == targetElement.node()->parentNode())
							{
								const size_t index = moveToParent->indexOfChild(targetElement.node());
								moveToParent->addChildAtIndex(sourceElement.node(), index);
							}
							else
							{
								std::shared_ptr<Node> insertBefore = targetElement.node();
								while (insertBefore->parentNode() != moveToParent)
								{
									insertBefore = insertBefore->parentNode();
									if (!insertBefore)
									{
										// 最後に追加
										moveToParent->addChild(sourceElement.node());
										break;
									}
								}
								if (insertBefore)
								{
									const size_t index = moveToParent->indexOfChild(insertBefore);
									moveToParent->addChildAtIndex(sourceElement.node(), index);
								}
							}

							newSelection.push_back(sourceElement.node());
						}
					}
					else if (const auto moveAsSiblingRectBottom = RectF{ rect.x, rect.y + rect.h - MoveAsSiblingThresholdPixels, rect.w, MoveAsSiblingThresholdPixels };
						moveAsSiblingRectBottom.mouseOver() && (targetElement.folded() || !targetElement.node()->hasChildren()))
					{
						// targetの下に兄弟ノードとして移動
						// X座標に基づいて、適切な親ノードを見つける
						std::shared_ptr<Node> moveToParent = targetElement.node()->parentNode();
						size_t actualNestLevel = targetElement.elementDetail().nestLevel;
					
						// マウスが左側にある場合、より上位の階層に移動
						while (moveToParent && actualNestLevel > desiredNestLevel)
						{
							const auto grandParent = moveToParent->parentNode();
							if (!grandParent)
							{
								break;
							}
							moveToParent = grandParent;
							actualNestLevel--;
						}
					
						for (const auto& sourceNode : sourceNodes)
						{
							const auto pSourceElement = getElementByHierarchyNode(sourceNode);
							if (pSourceElement == nullptr)
							{
								return;
							}
							const auto& sourceElement = *pSourceElement;
							if (sourceElement.node() == nullptr || targetElement.node() == nullptr)
							{
								return;
							}
							if (sourceElement.node() == targetElement.node())
							{
								// 自分自身には移動不可
								return;
							}
							if (sourceElement.node()->isAncestorOf(targetElement.node()))
							{
								// 子孫には移動不可
								return;
							}
						
								sourceElement.node()->removeFromParent();
						
							// 移動先での挿入位置を計算
							if (!moveToParent)
							{
									m_canvas->addChild(sourceElement.node());
							}
							else if (moveToParent == targetElement.node()->parentNode())
							{
								const size_t index = moveToParent->indexOfChild(targetElement.node()) + 1;
								moveToParent->addChildAtIndex(sourceElement.node(), index);
							}
							else
							{
								std::shared_ptr<Node> insertAfter = targetElement.node();
								while (insertAfter->parentNode() != moveToParent)
								{
									insertAfter = insertAfter->parentNode();
									if (!insertAfter)
									{
										// 最後に追加
										moveToParent->addChild(sourceElement.node());
										break;
									}
								}
								if (insertAfter)
								{
									const size_t index = moveToParent->indexOfChild(insertAfter) + 1;
									moveToParent->addChildAtIndex(sourceElement.node(), index);
								}
							}

							newSelection.push_back(sourceElement.node());
						}
					}
					else
					{
						// 子ノードとして移動
						for (const auto& sourceNode : sourceNodes)
						{
							const auto pSourceElement = getElementByHierarchyNode(sourceNode);
							if (pSourceElement == nullptr)
							{
								return;
							}
							const auto& sourceElement = *pSourceElement;
							if (sourceElement.node() == nullptr || targetElement.node() == nullptr)
							{
								return;
							}
							if (sourceElement.node() == targetElement.node())
							{
								// 自分自身には移動不可
								return;
							}
							if (sourceElement.node()->isAncestorOf(targetElement.node()))
							{
								// 子孫には移動不可
								return;
							}
							if (sourceElement.node()->parentNode() == targetElement.node())
							{
								// 親子関係が既にある場合は移動不可
								return;
							}
							sourceElement.node()->setParent(targetElement.node());

							newSelection.push_back(sourceElement.node());
						}
					}
					refreshNodeList();
					selectNodes(newSelection);
				},
				[this, hierarchyNode](const Array<std::shared_ptr<Node>>& sourceNodes) -> bool
				{
					// ドラッグ中のノードが全てHierarchy上の要素の場合のみドロップ操作を受け付ける
					return sourceNodes.all([&](const auto& sourceNode)
						{
							return getElementByHierarchyNode(sourceNode) != nullptr;
						});
				},
				[this, nestLevel, hierarchyNode](const Node& node)
				{
					const auto pTargetElement = getElementByHierarchyNode(hierarchyNode);
					if (pTargetElement == nullptr)
					{
						return;
					}
					const auto& targetElement = *pTargetElement;

					constexpr double Thickness = 4.0;
					const auto rect = node.regionRect();
					const auto mouseX = Cursor::PosF().x;
				
					// X座標による階層の判定
					const double desiredNestLevel = Math::Max(0.0, (mouseX - rect.x - 15) / 20.0);
					size_t actualNestLevel = targetElement.elementDetail().nestLevel;
				
					// 実際の描画位置を計算
					std::shared_ptr<Node> moveToParent = targetElement.node()->parentNode();
					while (moveToParent && actualNestLevel > desiredNestLevel)
					{
						const auto grandParent = moveToParent->parentNode();
						if (!grandParent)
						{
							break;
						}
						moveToParent = grandParent;
						actualNestLevel--;
					}
				
					if (const auto moveAsSiblingRectTop = RectF{ rect.x, rect.y, rect.w, MoveAsSiblingThresholdPixels };
						moveAsSiblingRectTop.mouseOver())
					{
						const Line line{ rect.tl() + Vec2::Right(15 + 20 * static_cast<double>(actualNestLevel)), rect.tr() };
						line.draw(Thickness, Palette::Orange);
						Circle{ line.begin, Thickness }.draw(Palette::Orange);
						Circle{ line.end, Thickness }.draw(Palette::Orange);
					}
					else if (const auto moveAsSiblingRectBottom = RectF{ rect.x, rect.y + rect.h - MoveAsSiblingThresholdPixels, rect.w, MoveAsSiblingThresholdPixels };
						moveAsSiblingRectBottom.mouseOver() && (targetElement.folded() || !targetElement.node()->hasChildren()))
					{
						const Line line{ rect.bl() + Vec2::Right(15 + 20 * static_cast<double>(actualNestLevel)), rect.br() };
						line.draw(Thickness, Palette::Orange);
						Circle{ line.begin, Thickness }.draw(Palette::Orange);
						Circle{ line.end, Thickness }.draw(Palette::Orange);
					}
					else
					{
						rect.draw(ColorF{ 1.0, 0.3 });
					}
				});
			const auto nameLabel = hierarchyNode->emplaceComponent<Label>(
				node->name(),
				U"",
				14,
				Palette::White,
				HorizontalAlign::Left,
				VerticalAlign::Middle,
				LRTB{ 20 + static_cast<double>(nestLevel) * 20, 5, 0, 0 },
				HorizontalOverflow::Wrap,
				VerticalOverflow::Clip);

			const auto stateLabel = hierarchyNode->emplaceComponent<Label>(
				U"",
				U"",
				14,
				Palette::White,
				HorizontalAlign::Right,
				VerticalAlign::Middle,
				LRTB{ 0, 5, 0, 0 },
				HorizontalOverflow::Overflow,
				VerticalOverflow::Clip);

			const auto toggleFoldedNode = hierarchyNode->emplaceChild(
				U"ToggleFolded",
				AnchorRegion
				{
					.anchorMin = Anchor::TopLeft,
					.anchorMax = Anchor::BottomLeft,
					.posDelta = Vec2{ 10 + nestLevel * 20, 0 },
					.sizeDelta = Vec2{ 30, 0 },
					.sizeDeltaPivot = Anchor::MiddleCenter,
				});
			toggleFoldedNode->setActive(node->hasChildren() ? ActiveYN::Yes : ActiveYN::No);
			toggleFoldedNode->addOnClick([this, node](const std::shared_ptr<Node>&)
				{
					if (!node->hasChildren())
					{
						// 子がないので折り畳み不可
						return;
					}

					if (const auto pElement = getElementByNode(node))
					{
						pElement->toggleFolded();
					}
				});
			const auto toggleFoldedLabel = toggleFoldedNode->emplaceComponent<Label>(
				U"▼",
				U"",
				10,
				ColorF{ 1.0, 0.6 },
				HorizontalAlign::Center,
				VerticalAlign::Middle);

			return Element
			{
				this,
				ElementDetail
				{
					.nestLevel = nestLevel,
					.node = node,
					.hierarchyNode = hierarchyNode,
					.hierarchyRectRenderer = hierarchyNode->getComponent<RectRenderer>(),
					.hierarchyStateLabel = stateLabel,
					.hierarchyToggleFoldedNode = toggleFoldedNode,
					.hierarchyToggleFoldedLabel = toggleFoldedLabel,
				}
			};
		}

		Element* getElementByNode(const std::shared_ptr<Node>& node)
		{
			if (node == nullptr)
			{
				return nullptr;
			}
			const auto it = std::find_if(m_elements.begin(), m_elements.end(),
				[node](const auto& e) { return e.node() == node; });
			if (it == m_elements.end())
			{
				return nullptr;
			}
			return &(*it);
		}

		Element* getElementByHierarchyNode(const std::shared_ptr<Node>& hierarchyNode)
		{
			if (hierarchyNode == nullptr)
			{
				return nullptr;
			}
			const auto it = std::find_if(m_elements.begin(), m_elements.end(),
				[hierarchyNode](const auto& e) { return e.hierarchyNode() == hierarchyNode; });
			if (it == m_elements.end())
			{
				return nullptr;
			}
			return &(*it);
		}

		void applyFoldingRecursive(Element& element, FoldedYN parentFoldedInHierarchy)
		{
			// 親が折り畳まれている場合はHierarchy上で非表示にする
			element.hierarchyNode()->setActive(parentFoldedInHierarchy ? ActiveYN::No : ActiveYN::Yes);

			// 再帰的に適用
			for (auto& childNode : element.node()->children())
			{
				if (Element* childElement = getElementByNode(childNode))
				{
					applyFoldingRecursive(*childElement, FoldedYN{ parentFoldedInHierarchy || element.folded() });
				}
			}
		}

	public:
		explicit Hierarchy(const std::shared_ptr<Canvas>& canvas, const std::shared_ptr<Canvas>& editorCanvas, const std::shared_ptr<ContextMenu>& contextMenu, const std::shared_ptr<Defaults>& defaults, const std::shared_ptr<DialogOpener>& dialogOpener)
			: m_canvas(canvas)
			, m_hierarchyFrameNode(editorCanvas->emplaceChild(
				U"HierarchyFrame",
				AnchorRegion
				{
					.anchorMin = Anchor::TopLeft,
					.anchorMax = Anchor::BottomLeft,
					.posDelta = Vec2{ 0, MenuBarHeight + Toolbar::ToolbarHeight },
					.sizeDelta = Vec2{ 300, -(MenuBarHeight + Toolbar::ToolbarHeight) },
					.sizeDeltaPivot = Anchor::TopLeft,
				}))
			, m_hierarchyInnerFrameNode(m_hierarchyFrameNode->emplaceChild(
				U"HierarchyInnerFrame",
				AnchorRegion
				{
					.anchorMin = Anchor::TopLeft,
					.anchorMax = Anchor::BottomRight,
					.posDelta = Vec2{ 0, 0 },
					.sizeDelta = Vec2{ -2, -2 },
					.sizeDeltaPivot = Anchor::MiddleCenter,
				},
				IsHitTargetYN::Yes,
				InheritChildrenStateFlags::Hovered | InheritChildrenStateFlags::Pressed))
			, m_hierarchyRootNode(m_hierarchyInnerFrameNode->emplaceChild(
				U"Hierarchy",
				AnchorRegion
				{
					.anchorMin = Anchor::TopLeft,
					.anchorMax = Anchor::BottomRight,
					.posDelta = Vec2{ 0, 0 },
					.sizeDelta = Vec2{ -10, -14 },
					.sizeDeltaPivot = Anchor::MiddleCenter,
				}))
			, m_editorCanvas(editorCanvas)
			, m_contextMenu(contextMenu)
			, m_defaults(defaults)
			, m_dialogOpener(dialogOpener)
		{
			m_hierarchyFrameNode->emplaceComponent<RectRenderer>(ColorF{ 0.5, 0.4 }, Palette::Black, 0.0, 10.0);
			m_hierarchyInnerFrameNode->emplaceComponent<RectRenderer>(ColorF{ 0.1, 0.8 }, Palette::Black, 0.0, 10.0);
			m_hierarchyInnerFrameNode->emplaceComponent<ContextMenuOpener>(contextMenu,
				Array<MenuElement>
				{
					MenuItem{ U"新規ノード", U"", KeyN, [this] { onClickNewNode(); } },
					MenuItem{ U"貼り付け", U"Ctrl+V", KeyP, [this] { onClickPaste(); }, [this] { return canPaste(); } },
				});
			m_hierarchyRootNode->setChildrenLayout(VerticalLayout{ .padding = 2 });
			m_hierarchyRootNode->setVerticalScrollable(true);

			refreshNodeList();
		}

		void refreshNodeList()
		{
			Array<std::weak_ptr<Node>> foldedNodes;
			foldedNodes.reserve(m_elements.size());
			for (const auto& element : m_elements)
			{
				if (element.folded())
				{
					foldedNodes.push_back(element.node());
				}
			}

			clearSelection();
			m_elements.clear();
			m_hierarchyRootNode->removeChildrenAll();
			for (const auto& child : m_canvas->children())
			{
				addElementRecursive(child, 0, RefreshesLayoutYN::No);
			}

			// 末尾に空のノードを追加してドロップ領域とする
			m_hierarchyTailNode = m_hierarchyRootNode->emplaceChild(
				U"HierarchyTail",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 0 },
					.flexibleWeight = 1.0,
				},
				IsHitTargetYN::Yes);
		
			// 末尾ノードにもContextMenuOpenerを追加
			m_hierarchyTailNode->emplaceComponent<ContextMenuOpener>(m_contextMenu,
				Array<MenuElement>
				{
					MenuItem{ U"新規ノード", U"", KeyN, [this] { onClickNewNode(); } },
					MenuItem{ U"貼り付け", U"Ctrl+V", KeyP, [this] { onClickPaste(); }, [this] { return canPaste(); } },
				});
		
			// 末尾ノードにDragDropTargetを追加
			m_hierarchyTailNode->emplaceComponent<DragDropTarget>([this](const Array<std::shared_ptr<Node>>& sourceNodes)
				{
					Array<std::shared_ptr<Node>> newSelection;
					newSelection.reserve(sourceNodes.size());
				
					// ルートノードの末尾に移動
					for (const auto& sourceNode : sourceNodes)
					{
						const auto pSourceElement = getElementByHierarchyNode(sourceNode);
						if (pSourceElement == nullptr)
						{
							continue;
						}
						const auto& sourceElement = *pSourceElement;
					
						sourceElement.node()->removeFromParent();
						m_canvas->addChild(sourceElement.node());
					
						newSelection.push_back(sourceElement.node());
					}
				
					if (!newSelection.empty())
					{
						refreshNodeList();
						selectNodes(newSelection);
					}
				},
				[this](const Array<std::shared_ptr<Node>>& sourceNodes) -> bool
				{
					// ドラッグ中のノードが全てHierarchy上の要素の場合のみドロップ操作を受け付ける
					return sourceNodes.all([&](const auto& sourceNode)
						{
							return getElementByHierarchyNode(sourceNode) != nullptr;
						});
				},
				[this](const Node& node)
				{
					// 末尾にドロップする際のオレンジ色の線を描画
					constexpr double Thickness = 4.0;
					const auto rect = node.regionRect();
				
					// ドラッグ中のノードを除外して最後の表示要素を見つける
					const Element* pLastVisibleElement = nullptr;
					for (auto it = m_elements.rbegin(); it != m_elements.rend(); ++it)
					{
						// アクティブでない（折りたたまれた親の中にある）要素はスキップ
						if (it->hierarchyNode()->activeInHierarchy() == ActiveYN::No)
						{
							continue;
						}
						// ドラッグ中（選択中）の要素はスキップ
						if (it->editorSelected() == EditorSelectedYN::Yes)
						{
							continue;
						}
						pLastVisibleElement = &(*it);
						break;
					}
				
					if (pLastVisibleElement)
					{
						const auto lastRect = pLastVisibleElement->hierarchyNode()->regionRect();
						// ルートノードの子として移動（nestLevel=0のラベル位置に合わせる）
						const double lineY = lastRect.y + lastRect.h;
						const Line line{ Vec2{rect.x + 35, lineY}, Vec2{rect.x + rect.w, lineY} };
						line.draw(Thickness, Palette::Orange);
						Circle{ line.begin, Thickness }.draw(Palette::Orange);
						Circle{ line.end, Thickness }.draw(Palette::Orange);
					}
					else
					{
						// 要素が空の場合も同様
						const Line line{ rect.tl() + Vec2::Right(35), rect.tr() };
						line.draw(Thickness, Palette::Orange);
						Circle{ line.begin, Thickness }.draw(Palette::Orange);
						Circle{ line.end, Thickness }.draw(Palette::Orange);
					}
				});

			for (const auto& node : foldedNodes)
			{
				if (auto pElement = getElementByNode(node.lock()))
				{
					pElement->setFolded(FoldedYN::Yes);
				}
			}

			if (const auto editorCanvas = m_editorCanvas.lock())
			{
				editorCanvas->refreshLayout();
			}
		}

		void refreshNodeNames()
		{
			for (const auto& element : m_elements)
			{
				element.hierarchyNode()->getComponent<Label>()->setText(element.node()->name());
			}
		}

		void selectNodes(const Array<std::shared_ptr<Node>>& nodes)
		{
			clearSelection();
			for (const auto& node : nodes)
			{
				if (auto pElement = getElementByNode(node))
				{
					pElement->setEditorSelected(EditorSelectedYN::Yes);
					unfoldForNode(node);
				}
			}
			if (nodes.size() == 1)
			{
				m_lastEditorSelectedNode = nodes.front();
				m_shiftSelectOriginNode = nodes.front();
			}
		}

		void selectAll()
		{
			if (m_elements.empty())
			{
				return;
			}

			for (auto& element : m_elements)
			{
				element.setEditorSelected(EditorSelectedYN::Yes);
			}

			m_lastEditorSelectedNode = m_elements.back().node();
			m_shiftSelectOriginNode = m_elements.front().node();
		}

		void selectSingleNode(const std::shared_ptr<Node>& node)
		{
			clearSelection();
			if (auto it = std::find_if(m_elements.begin(), m_elements.end(), [&node](const Element& element) { return element.node() == node; });
				it != m_elements.end())
			{
				it->setEditorSelected(EditorSelectedYN::Yes);
				unfoldForNode(node);
				m_lastEditorSelectedNode = node;
				m_shiftSelectOriginNode = node;
			}
		}

		bool hasSelection() const
		{
			return std::any_of(m_elements.begin(), m_elements.end(), [](const Element& element) { return element.editorSelected(); });
		}

		void unfoldForNode(const std::shared_ptr<Node>& node)
		{
			if (auto pElement = getElementByNode(node))
			{
				pElement->setFolded(FoldedYN::No);
				if (auto parentNode = node->parentNode())
				{
					unfoldForNode(parentNode);
				}
			}
		}

		bool canPaste() const
		{
			return !m_copiedNodeJSONs.empty();
		}

		void onClickNewNode()
		{
			// 最後に選択したノードの兄弟として新規ノードを作成
			if (const auto lastEditorSelectedNode = m_lastEditorSelectedNode.lock())
			{
				if (const auto parentNode = lastEditorSelectedNode->parentNode())
				{
					onClickNewNode(parentNode);
				}
				else
				{
						onClickNewNodeToCanvas();
				}
			}
			else
			{
				onClickNewNodeToCanvas();
			}
		}

		void onClickNewNodeToCanvas()
		{
			std::shared_ptr<Node> newNode = m_canvas->emplaceChild(
				U"Node",
				m_defaults->defaultRegion());
			refreshNodeList();
			selectSingleNode(newNode);
		}

		void onClickNewNode(std::shared_ptr<Node> parentNode)
		{
			if (!parentNode)
			{
				throw Error{ U"Parent node is nullptr" };
			}

			// 記憶された種類のRegionを使用してノードを作成
			std::shared_ptr<Node> newNode = parentNode->emplaceChild(
				U"Node",
				m_defaults->defaultRegion());
			refreshNodeList();
			selectSingleNode(newNode);
		}

		void onClickDelete()
		{
			bool hasDeleted = false;
			for (auto it = m_elements.begin(); it != m_elements.end();)
			{
				if (it->editorSelected())
				{
					if (it->node()->removeFromParent())
					{
						it = m_elements.erase(it);
						hasDeleted = true;
					}
					else
					{
						++it;
					}
				}
				else
				{
					++it;
				}
			}
			if (!hasDeleted)
			{
				return;
			}
			refreshNodeList();
			clearSelection();
		}

		void onClickCut()
		{
			onClickCopy();
			onClickDelete();
		}

		[[nodiscard]]
		Array<std::shared_ptr<Node>> getSelectedNodesExcludingChildren() const
		{
			// 選択中のノードを列挙
			// ただし、親が選択中の場合は子は含めない
			Array<std::shared_ptr<Node>> selectedNodes;
			for (const auto& element : m_elements)
			{
				if (element.editorSelected())
				{
					bool parentSelected = false;
					for (const auto& parent : selectedNodes)
					{
						if (parent->containsChild(element.node(), RecursiveYN::Yes))
						{
							parentSelected = true;
							break;
						}
					}
					if (!parentSelected)
					{
						selectedNodes.push_back(element.node());
					}
				}
			}
			return selectedNodes;
		}

		void onClickCopy()
		{
			m_copiedNodeJSONs.clear();

			// 選択中のノードをコピー
			const auto selectedNodes = getSelectedNodesExcludingChildren();
			m_copiedNodeJSONs.reserve(selectedNodes.size());
			for (const auto& selectedNode : selectedNodes)
			{
				m_copiedNodeJSONs.push_back(selectedNode->toJSON());
			}
		}

		void onClickDuplicate()
		{
			const auto selectedNodes = getSelectedNodesExcludingChildren();
			if (selectedNodes.empty())
			{
				return;
			}

			// 複製実行
			Array<std::shared_ptr<Node>> newNodes;
			newNodes.reserve(selectedNodes.size());
			for (const auto& selectedNode : selectedNodes)
			{
				const auto parentNode = selectedNode->parentNode();
				if (!parentNode)
				{
					continue;
				}
				const auto newNode = parentNode->addChildFromJSON(selectedNode->toJSON(), RefreshesLayoutYN::No);
				newNodes.push_back(newNode);
			}
			m_canvas->refreshLayout();
			refreshNodeList();
			selectNodes(newNodes);
		}

		void showClearedParamRefsDialog(const Array<String>& clearedParams)
		{
			if (clearedParams.empty())
			{
				return;
			}
			
			// アルファベット順でソート
			Array<String> sortedParams = clearedParams;
			sortedParams.sort();
			
			// 最大10件まで表示
			String paramList;
			const size_t displayCount = Min(sortedParams.size(), size_t(10));
			for (size_t i = 0; i < displayCount; ++i)
			{
				if (i > 0)
				{
					paramList += U"\n";
				}
				paramList += U"・" + sortedParams[i];
			}
			
			// 10件を超える場合は合計数を表示
			if (sortedParams.size() > 10)
			{
				paramList += U"\n... (全" + Format(sortedParams.size()) + U"件)";
			}
			
			// ダイアログを表示
			m_dialogOpener->openDialogOK(U"以下のパラメータ参照は利用できないため解除されました。\n\n" + paramList);
		}

		void onClickPaste()
		{
			// 最後に選択したノードの兄弟として貼り付け
			if (const auto lastEditorSelectedNode = m_lastEditorSelectedNode.lock())
			{
				if (lastEditorSelectedNode->parentNode())
				{
					onClickPaste(lastEditorSelectedNode->parentNode(), lastEditorSelectedNode->siblingIndex() + 1);
				}
				else
				{
					// Canvasの直接の子として貼り付け
					onClickPasteToCanvas();
				}
			}
			else
			{
				// Canvasの直接の子として貼り付け
				onClickPasteToCanvas();
			}
		}

		void onClickPasteToCanvas()
		{
			if (m_copiedNodeJSONs.empty())
			{
				return;
			}

			Array<std::shared_ptr<Node>> newNodes;
			for (const auto& copiedNodeJSON : m_copiedNodeJSONs)
			{
				newNodes.push_back(m_canvas->addChildFromJSON(copiedNodeJSON, RefreshesLayoutYN::No));
			}
			m_canvas->refreshLayout();
			// 無効なパラメータ参照を解除
			const auto clearedParams = m_canvas->removeInvalidParamRefs();
			refreshNodeList();
			selectNodes(newNodes);
			showClearedParamRefsDialog(clearedParams);
		}

		void onClickPaste(std::shared_ptr<Node> parentNode, Optional<size_t> index = none)
		{
			if (!parentNode)
			{
				throw Error{ U"Parent node is nullptr" };
			}

			if (m_copiedNodeJSONs.empty())
			{
				return;
			}

			// 貼り付け実行
			Array<std::shared_ptr<Node>> newNodes;
			if (index.has_value())
			{
				size_t indexValue = Min(index.value(), parentNode->children().size());
				for (const auto& copiedNodeJSON : m_copiedNodeJSONs)
				{
					newNodes.push_back(parentNode->addChildAtIndexFromJSON(copiedNodeJSON, indexValue, RefreshesLayoutYN::No));
					++indexValue;
				}
			}
			else
			{
				for (const auto& copiedNodeJSON : m_copiedNodeJSONs)
				{
					newNodes.push_back(parentNode->addChildFromJSON(copiedNodeJSON, RefreshesLayoutYN::No));
				}
			}
			m_canvas->refreshLayout();
			// 無効なパラメータ参照を解除
			const auto clearedParams = m_canvas->removeInvalidParamRefs();
			refreshNodeList();
			selectNodes(newNodes);
			showClearedParamRefsDialog(clearedParams);
		}

		void onClickCreateEmptyParent()
		{
			const auto selectedNode = m_lastEditorSelectedNode.lock();
			if (!selectedNode)
			{
				return;
			}

			auto oldParent = selectedNode->parentNode();
			if (!oldParent)
			{
				return;
			}

			// selectedNodeが兄弟同士の中で何番目の要素かを調べる
			auto& siblings = oldParent->children();
			auto it = std::find(siblings.begin(), siblings.end(), selectedNode);
			if (it == siblings.end())
			{
				return;
			}
			const size_t idx = std::distance(siblings.begin(), it);

			// 親から切り離す
			selectedNode->removeFromParent();

			// 元ノードと同じインデックスに同じレイアウト設定で空の親ノードを生成
			const auto newParent = Node::Create(U"Node", selectedNode->region());
			oldParent->addChildAtIndex(newParent, idx);

			// 新しい親のもとへ子として追加
			newParent->addChild(selectedNode);

			// 元オブジェクトはアンカーがMiddleCenterのAnchorRegionに変更する
			const RectF originalCalculatedRect = selectedNode->regionRect();
			selectedNode->setRegion(AnchorRegion
			{
				.anchorMin = Anchor::MiddleCenter,
				.anchorMax = Anchor::MiddleCenter,
				.posDelta = Vec2{ 0, 0 },
				.sizeDelta = originalCalculatedRect.size,
				.sizeDeltaPivot = Anchor::MiddleCenter,
			});

			refreshNodeList();
			selectSingleNode(newParent);
		}

		void onClickMoveUp()
		{
			// 選択中のノードを列挙
			Array<std::shared_ptr<Node>> selectedNodes;
			for (const auto& element : m_elements)
			{
				if (element.editorSelected())
				{
					selectedNodes.push_back(element.node());
				}
			}
			if (selectedNodes.isEmpty())
			{
				return;
			}

			HashTable<std::shared_ptr<Node>, Array<std::shared_ptr<Node>>> selectionByParent;
			Array<std::shared_ptr<Node>> topLevelNodes;
			
			selectionByParent.reserve(selectedNodes.size());
			for (const auto& child : selectedNodes)
			{
				if (auto parent = child->parentNode())
				{
					selectionByParent[parent].push_back(child);
				}
				else if (child->isTopLevelNode())
				{
					topLevelNodes.push_back(child);
				}
			}
			
			if (!topLevelNodes.isEmpty())
			{
				const auto& siblings = m_canvas->children();
				
				Array<size_t> indices;
				indices.reserve(topLevelNodes.size());
				for (const auto& child : topLevelNodes)
				{
					const auto it = std::find(siblings.begin(), siblings.end(), child);
					if (it != siblings.end())
					{
						indices.push_back(std::distance(siblings.begin(), it));
					}
				}
				
				std::sort(indices.begin(), indices.end());
				for (auto index : indices)
				{
					if (index > 0)
					{
						m_canvas->swapChildren(index, index - 1);
					}
				}
			}
			
			for (auto& [parent, childrenToMove] : selectionByParent)
			{
				const auto& siblings = parent->children();

				Array<size_t> indices;
				indices.reserve(childrenToMove.size());
				for (const auto& child : childrenToMove)
				{
					const auto it = std::find(siblings.begin(), siblings.end(), child);
					if (it != siblings.end())
					{
						indices.push_back(std::distance(siblings.begin(), it));
					}
				}

				std::sort(indices.begin(), indices.end());
				for (auto index : indices)
				{
					if (index > 0)
					{
						parent->swapChildren(index, index - 1);
					}
				}
			}
			m_canvas->refreshLayout();
			refreshNodeList();
			selectNodes(selectedNodes);
		}

		void onClickMoveDown()
		{
			// 選択中のノードを列挙
			Array<std::shared_ptr<Node>> selectedNodes;
			for (const auto& element : m_elements)
			{
				if (element.editorSelected())
				{
					selectedNodes.push_back(element.node());
				}
			}
			if (selectedNodes.isEmpty())
			{
				return;
			}

			HashTable<std::shared_ptr<Node>, Array<std::shared_ptr<Node>>> selectionByParent;
			Array<std::shared_ptr<Node>> topLevelNodes;
			
			selectionByParent.reserve(selectedNodes.size());
			for (const auto& child : selectedNodes)
			{
				if (auto parent = child->parentNode())
				{
					selectionByParent[parent].push_back(child);
				}
				else if (child->isTopLevelNode())
				{
					topLevelNodes.push_back(child);
				}
			}
			
			if (!topLevelNodes.isEmpty())
			{
				const auto& siblings = m_canvas->children();
				
				Array<size_t> indices;
				indices.reserve(topLevelNodes.size());
				for (const auto& child : topLevelNodes)
				{
					const auto it = std::find(siblings.begin(), siblings.end(), child);
					if (it != siblings.end())
					{
						indices.push_back(std::distance(siblings.begin(), it));
					}
				}
				
				std::sort(indices.begin(), indices.end(), std::greater<size_t>());
				for (auto index : indices)
				{
					if (index < siblings.size() - 1)
					{
						m_canvas->swapChildren(index, index + 1);
					}
				}
			}
			
			for (auto& [parent, childrenToMove] : selectionByParent)
			{
				const auto& siblings = parent->children();

				Array<size_t> indices;
				indices.reserve(childrenToMove.size());
				for (const auto& child : childrenToMove)
				{
					const auto it = std::find(siblings.begin(), siblings.end(), child);
					if (it != siblings.end())
					{
						indices.push_back(std::distance(siblings.begin(), it));
					}
				}

				std::sort(indices.begin(), indices.end(), std::greater<size_t>());
				for (auto index : indices)
				{
					if (index < siblings.size() - 1)
					{
						parent->swapChildren(index, index + 1);
					}
				}
			}
			m_canvas->refreshLayout();
			refreshNodeList();
			selectNodes(selectedNodes);
		}

		void clearSelection(bool clearShiftSelectOrigin = true)
		{
			for (auto& element : m_elements)
			{
				element.setEditorSelected(EditorSelectedYN::No);
			}
			if (clearShiftSelectOrigin)
			{
				m_shiftSelectOriginNode.reset();
			}
			m_lastEditorSelectedNode = std::weak_ptr<Node>{};
		}

		void update()
		{
			m_editorHoveredNode.reset();
			for (size_t i = 0; i < m_elements.size(); ++i)
			{
				auto& element = m_elements[i];
				if (element.hierarchyNode()->isHovered())
				{
					m_editorHoveredNode = element.node();
				}

				if (element.node()->isHitTarget())
				{
					const InteractionState interactionState = element.node()->currentInteractionState();
					// 状態表示を動的に生成
					String stateText;
					const String& styleState = element.node()->styleState();
					const String interactionStateStr = EnumToString(interactionState);
				
					if (!styleState.empty())
					{
						if (interactionState == InteractionState::Default)
						{
							stateText = U"[{}]"_fmt(styleState);
						}
						else
						{
							stateText = U"[{}, {}]"_fmt(styleState, interactionStateStr);
						}
					}
					else
					{
						stateText = U"[{}]"_fmt(interactionStateStr);
					}
				
					element.elementDetail().hierarchyStateLabel->setText(stateText);
				}
				else
				{
					const String& styleState = element.node()->styleState();
					element.elementDetail().hierarchyStateLabel->setText(!styleState.empty() ? U"[{}]"_fmt(styleState) : U"");
				}

				if (element.hierarchyNode()->isClicked())
				{
					if (KeyShift.pressed() && !m_shiftSelectOriginNode.expired())
					{
						const auto originIt = std::find_if(m_elements.begin(), m_elements.end(), [originNode = m_shiftSelectOriginNode.lock()](const auto& e) { return e.node() == originNode; });
						if (originIt == m_elements.end())
						{
							throw Error{ U"Shift select origin node not found in m_elements" };
						}
						clearSelection(false);
						const auto originIndex = static_cast<size_t>(std::distance(m_elements.begin(), originIt));
						const auto start = Min(originIndex, i);
						const auto end = Max(originIndex, i);
						for (size_t j = start; j <= end; ++j)
						{
							m_elements[j].setEditorSelected(EditorSelectedYN::Yes);
						}
					}
					else
					{
						if (KeyControl.pressed())
						{
							// Ctrlキーを押しながらクリックした場合は選択/非選択を切り替え
							const EditorSelectedYN newSelected = EditorSelectedYN{ !element.editorSelected() };
							element.setEditorSelected(newSelected);
							if (newSelected)
							{
								m_shiftSelectOriginNode = element.node();
							}
							else
							{
								m_shiftSelectOriginNode.reset();
							}
						}
						else
						{
							// 普通にクリックした場合は1つだけ選択
							// (複数回押しても選択/非選択の切り替えはしない)
							clearSelection();
							element.setEditorSelected(EditorSelectedYN::Yes);
							m_shiftSelectOriginNode = element.node();
						}
					}

					if (m_elements.count_if([](const auto& e) { return e.editorSelected().getBool(); }) == 1)
					{
						const auto selectedNode = std::find_if(m_elements.begin(), m_elements.end(), [](const auto& e) { return e.editorSelected(); })->node();
						m_lastEditorSelectedNode = selectedNode;
					}
					else
					{
						m_lastEditorSelectedNode = std::weak_ptr<Node>{};
					}
				}

				if (m_hierarchyRootNode->isClicked() || m_hierarchyTailNode->isClicked())
				{
					// Hierarchyの空白部分がクリックされた場合は選択を解除
					clearSelection();
				}
			}
		}

		void applyFolding()
		{
			if (m_elements.empty())
			{
				return;
			}
			auto& rootElement = m_elements.front();
			applyFoldingRecursive(rootElement, FoldedYN::No);
		}

		[[nodiscard]]
		const std::weak_ptr<Node>& selectedNode() const
		{
			return m_lastEditorSelectedNode;
		}

		[[nodiscard]]
		bool checkSelectionChanged()
		{
			const auto currentSelectedNode = m_lastEditorSelectedNode.lock();
			const auto prevSelectedNode = m_prevCheckedSelectedNode.lock();
		
			// 選択状態が変化したかチェック
			const bool currentExists = (currentSelectedNode != nullptr);
			const bool changed = currentSelectedNode != prevSelectedNode || currentExists != m_prevSelectedNodeExists;
		
			// 状態を更新
			if (changed)
			{
				m_prevCheckedSelectedNode = m_lastEditorSelectedNode;
				m_prevSelectedNodeExists = currentExists;
			}
		
			return changed;
		}
	
		[[nodiscard]]
		bool toolbarRefreshRequested()
		{
			bool refreshNeeded = false;
		
			// クリップボードの空/非空状態が変化したかチェック
			const bool currentHasContent = !m_copiedNodeJSONs.empty();
			if (currentHasContent != m_prevClipboardHasContent)
			{
				m_prevClipboardHasContent = currentHasContent;
				refreshNeeded = true;
			}
		
			return refreshNeeded;
		}

		const std::shared_ptr<Node>& hierarchyFrameNode() const
		{
			return m_hierarchyFrameNode;
		}
	
		void setWidth(double width)
		{
			if (auto* pAnchorRegion = m_hierarchyFrameNode->anchorRegion())
			{
				auto newRegion = *pAnchorRegion;
				newRegion.sizeDelta.x = width;
				m_hierarchyFrameNode->setRegion(newRegion);
			}
			else
			{
				Logger << U"[NocoEditor warning] AnchorRegion not found in hierarchyFrameNode";
			}
		}

		void drawSelectedNodesGizmo() const
		{
			const auto editorHoveredNode = m_editorHoveredNode.lock();

			for (const auto& element : m_elements)
			{
				const auto& node = element.node();
				if (!node->activeInHierarchy())
				{
					continue;
				}

				constexpr double Thickness = 2.0;
				const EditorSelectedYN editorSelected = element.editorSelected();
				if (editorSelected)
				{
					const Quad quad = node->hitQuad();
					quad.drawFrame(Thickness, Palette::Orange);

					// 上下左右にリサイズハンドルを表示
					// TODO: リサイズ可能にする
					/*constexpr double HandleSize = 3.0;
					const auto rect = node->rect().stretched(Thickness / 2);
					Circle{ rect.topCenter(), HandleSize }.draw(Palette::Orange);
					Circle{ rect.bottomCenter(), HandleSize }.draw(Palette::Orange);
					Circle{ rect.leftCenter(), HandleSize }.draw(Palette::Orange);
					Circle{ rect.rightCenter(), HandleSize }.draw(Palette::Orange);*/
				}

				if (node == editorHoveredNode)
				{
					const Quad quad = node->hitQuad();
					quad.draw(ColorF{ 1.0, 0.1 });
					if (!editorSelected)
					{
						quad.drawFrame(Thickness, ColorF{ 1.0 });
					}
				}
			}
		}
	};
}
