#include "Hierarchy.hpp"

using namespace noco;

Hierarchy::Element::Element(Hierarchy* pHierarchy, const ElementDetail& elementDetail)
	: m_pHierarchy{ pHierarchy }
	, m_elementDetail{ elementDetail }
{
	if (m_pHierarchy == nullptr)
	{
		throw Error{ U"Hierarchy is nullptr" };
	}
}

[[nodiscard]]
EditorSelectedYN Hierarchy::Element::editorSelected() const
{
	return m_editorSelected;
}

void Hierarchy::Element::setEditorSelected(EditorSelectedYN editorSelected)
{
	m_editorSelected = editorSelected;
	m_elementDetail.hierarchyRectRenderer->setFillColor(HierarchyRectFillColor(m_editorSelected));
	m_elementDetail.hierarchyRectRenderer->setOutlineColor(HierarchyRectOutlineColor(m_editorSelected));
}

[[nodiscard]]
const Hierarchy::ElementDetail& Hierarchy::Element::elementDetail() const
{
	return m_elementDetail;
}

[[nodiscard]]
const std::shared_ptr<Node>& Hierarchy::Element::node() const
{
	return m_elementDetail.node;
}

[[nodiscard]]
const std::shared_ptr<Node>& Hierarchy::Element::hierarchyNode() const
{
	return m_elementDetail.hierarchyNode;
}

void Hierarchy::Element::toggleFolded()
{
	setFolded(m_folded ? FoldedYN::No : FoldedYN::Yes);
}

void Hierarchy::Element::setFolded(FoldedYN folded)
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
FoldedYN Hierarchy::Element::folded() const
{
	return m_folded;
}

[[nodiscard]]
PropertyValue<ColorF> Hierarchy::Element::HierarchyRectFillColor(EditorSelectedYN editorSelected)
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
PropertyValue<ColorF> Hierarchy::Element::HierarchyRectOutlineColor(EditorSelectedYN editorSelected)
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

Hierarchy::Hierarchy(const std::shared_ptr<Canvas>& canvas, const std::shared_ptr<Canvas>& editorCanvas, const std::shared_ptr<ContextMenu>& contextMenu, const std::shared_ptr<Defaults>& defaults)
	: m_canvas(canvas)
	, m_hierarchyFrameNode(canvas->rootNode()->emplaceChild(
		U"HierarchyFrame",
		AnchorConstraint
		{
			.anchorMin = Anchor::TopLeft,
			.anchorMax = Anchor::BottomLeft,
			.posDelta = Vec2{ 0, MenuBarHeight },
			.sizeDelta = Vec2{ 256, -MenuBarHeight },
			.sizeDeltaPivot = Anchor::TopLeft,
		}))
	, m_hierarchyInnerFrameNode(m_hierarchyFrameNode->emplaceChild(
		U"HierarchyInnerFrame",
		AnchorConstraint
		{
			.anchorMin = Anchor::TopLeft,
			.anchorMax = Anchor::BottomRight,
			.posDelta = Vec2{ 0, 0 },
			.sizeDelta = Vec2{ 0, 0 },
			.sizeDeltaPivot = Anchor::TopLeft,
		}))
	, m_hierarchyRootNode(m_hierarchyInnerFrameNode->emplaceChild(
		U"HierarchyRoot",
		BoxConstraint
		{
			.sizeDelta = { 0, 0 },
		}))
	, m_editorCanvas(editorCanvas)
	, m_contextMenu(contextMenu)
	, m_defaults(defaults)
{
	m_hierarchyFrameNode->emplaceComponent<RectRenderer>(ColorF{ 0.8 });
	m_hierarchyInnerFrameNode->setVerticalScrollable(true);
	m_hierarchyInnerFrameNode->setBoxChildrenLayout(VerticalLayout{ .padding = LRTB{ 6, 6, 6, 6 } });
	m_hierarchyRootNode->setBoxChildrenLayout(VerticalLayout{});
}

void Hierarchy::addElementRecursive(const std::shared_ptr<Node>& node, size_t nestLevel, RefreshesLayoutYN refreshesLayout)
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

Hierarchy::Element Hierarchy::createElement(const std::shared_ptr<Node>& node, size_t nestLevel)
{
	const auto hierarchyNode = Node::Create(
		U"Element",
		BoxConstraint
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
			MenuItem{ U"削除", U"Delete", KeyR, [this] { onClickDelete(); } },
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
	auto hierarchyRectRenderer = hierarchyNode->emplaceComponent<RectRenderer>(Element::HierarchyRectFillColor(EditorSelectedYN::No), Element::HierarchyRectOutlineColor(EditorSelectedYN::No), 1.0, 3.0);
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

			const auto rect = hierarchyNode->rect();
			if (const auto moveAsSiblingRectTop = RectF{ rect.x, rect.y, rect.w, MoveAsSiblingThresholdPixels };
				moveAsSiblingRectTop.mouseOver())
			{
				// targetの上に兄弟ノードとして移動
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
					const auto pTargetParent = targetElement.node()->parent();
					if (pTargetParent == nullptr)
					{
						return;
					}
					sourceElement.node()->removeFromParent();
					const size_t index = pTargetParent->indexOfChild(targetElement.node());
					pTargetParent->addChildAtIndex(sourceElement.node(), index);

					newSelection.push_back(sourceElement.node());
				}
			}
			else if (const auto moveAsSiblingRectBottom = RectF{ rect.x, rect.y + rect.h - MoveAsSiblingThresholdPixels, rect.w, MoveAsSiblingThresholdPixels };
				moveAsSiblingRectBottom.mouseOver() && (targetElement.folded() || !targetElement.node()->hasChildren()))
			{
				// targetの下に兄弟ノードとして移動
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
					const auto pTargetParent = targetElement.node()->parent();
					if (pTargetParent == nullptr)
					{
						return;
					}
					sourceElement.node()->removeFromParent();
					const size_t index = pTargetParent->indexOfChild(targetElement.node()) + 1;
					pTargetParent->addChildAtIndex(sourceElement.node(), index);

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
					sourceElement.node()->removeFromParent();
					targetElement.node()->addChild(sourceElement.node());

					newSelection.push_back(sourceElement.node());
				}
			}

			refreshNodeList();
			selectNodes(newSelection);
		});

	const auto nodeNameContainerNode = hierarchyNode->emplaceChild(
		U"NodeNameContainer",
		BoxConstraint
		{
			.sizeRatio = Vec2{ 1, 1 },
			.margin = LRTB{ nestLevel * 20.0, 20, 0, 0 },
		},
		IsHitTargetYN::No,
		InheritChildrenStateFlags::Hovered);
	nodeNameContainerNode->setBoxChildrenLayout(HorizontalLayout{});

	std::shared_ptr<Node> hierarchyToggleFoldedNode;
	std::shared_ptr<Label> hierarchyToggleFoldedLabel;
	if (node->hasChildren())
	{
		hierarchyToggleFoldedNode = nodeNameContainerNode->emplaceChild(
			U"ToggleFolded",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0, 1 },
				.sizeDelta = Vec2{ 20, 0 },
			},
			IsHitTargetYN::Yes,
			InheritChildrenStateFlags::Hovered);
		hierarchyToggleFoldedLabel = hierarchyToggleFoldedNode->emplaceComponent<Label>(U"▼", U"", 12, ColorF{ 0.0 }, HorizontalAlign::Center, VerticalAlign::Middle);
	}

	nodeNameContainerNode->emplaceComponent<Label>(node->name(), U"", 12, ColorF{ 0.0 }, HorizontalAlign::Left, VerticalAlign::Middle);

	const auto hierarchyStateLabel = hierarchyNode->emplaceComponent<Label>(U"", U"", 10, ColorF{ 0.3 }, HorizontalAlign::Right, VerticalAlign::Middle, LRTB{ 0, 4, 0, 0 });

	return Element{ this, ElementDetail
		{
			.nestLevel = nestLevel,
			.node = node,
			.hierarchyNode = hierarchyNode,
			.hierarchyRectRenderer = hierarchyRectRenderer,
			.hierarchyStateLabel = hierarchyStateLabel,
			.hierarchyToggleFoldedNode = hierarchyToggleFoldedNode,
			.hierarchyToggleFoldedLabel = hierarchyToggleFoldedLabel,
		} };
}

bool Hierarchy::isFoldedRecursive(const std::shared_ptr<Node>& node) const
{
	if (const auto pElement = getElementByNode(node))
	{
		if (pElement->folded())
		{
			return true;
		}
	}
	if (const auto parent = node->parent())
	{
		return isFoldedRecursive(parent);
	}
	return false;
}

bool Hierarchy::isEditorSelectedRecursive(const std::shared_ptr<Node>& node, RecursionYN recursive) const
{
	if (const auto pElement = getElementByNode(node))
	{
		if (pElement->editorSelected())
		{
			return true;
		}
	}
	if (recursive)
	{
		for (const auto& child : node->children())
		{
			if (isEditorSelectedRecursive(child, RecursionYN::Yes))
			{
				return true;
			}
		}
	}
	return false;
}

Array<Hierarchy::Element*> Hierarchy::getSelectedElements()
{
	Array<Element*> selectedElements;
	for (auto& element : m_elements)
	{
		if (element.editorSelected())
		{
			selectedElements.push_back(&element);
		}
	}
	return selectedElements;
}

Array<const Hierarchy::Element*> Hierarchy::getSelectedElements() const
{
	Array<const Element*> selectedElements;
	for (const auto& element : m_elements)
	{
		if (element.editorSelected())
		{
			selectedElements.push_back(&element);
		}
	}
	return selectedElements;
}

Array<std::shared_ptr<Node>> Hierarchy::getSelectedNodes()
{
	Array<std::shared_ptr<Node>> selectedNodes;
	for (const auto& element : m_elements)
	{
		if (element.editorSelected())
		{
			selectedNodes.push_back(element.node());
		}
	}
	return selectedNodes;
}

Array<std::shared_ptr<Node>> Hierarchy::getSelectedNodes() const
{
	Array<std::shared_ptr<Node>> selectedNodes;
	for (const auto& element : m_elements)
	{
		if (element.editorSelected())
		{
			selectedNodes.push_back(element.node());
		}
	}
	return selectedNodes;
}

void Hierarchy::setNodesEditorSelected(const Array<std::shared_ptr<Node>>& nodes, EditorSelectedYN editorSelected)
{
	for (const auto& node : nodes)
	{
		if (auto pElement = getElementByNode(node))
		{
			pElement->setEditorSelected(editorSelected);
		}
	}
}

Hierarchy::Element* Hierarchy::getElementByHierarchyNode(const std::shared_ptr<Node>& hierarchyNode)
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

const Hierarchy::Element* Hierarchy::getElementByHierarchyNode(const std::shared_ptr<Node>& hierarchyNode) const
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

Hierarchy::Element* Hierarchy::getElementByNode(const std::shared_ptr<Node>& node)
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

const Hierarchy::Element* Hierarchy::getElementByNode(const std::shared_ptr<Node>& node) const
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

void Hierarchy::applyFoldingRecursive(const Element& element, bool visible)
{
	for (const auto& child : element.node()->children())
	{
		if (const auto pChildElement = getElementByNode(child))
		{
			pChildElement->hierarchyNode()->setActive(visible && !element.folded() ? ActiveYN::Yes : ActiveYN::No);
			applyFoldingRecursive(*pChildElement, visible && !element.folded());
		}
	}
}

bool Hierarchy::hasAncestorSelected(const std::shared_ptr<Node>& node) const
{
	if (const auto parent = node->parent())
	{
		if (const auto pParentElement = getElementByNode(parent))
		{
			if (pParentElement->editorSelected())
			{
				return true;
			}
		}
		return hasAncestorSelected(parent);
	}
	return false;
}

bool Hierarchy::canMoveUp() const
{
	const auto selectedNodes = getSelectedNodes();
	if (selectedNodes.size() != 1)
	{
		return false;
	}
	const auto& node = selectedNodes.front();
	const auto parent = node->parent();
	if (parent == nullptr)
	{
		return false;
	}
	const auto index = parent->indexOfChild(node);
	return index > 0;
}

bool Hierarchy::canMoveDown() const
{
	const auto selectedNodes = getSelectedNodes();
	if (selectedNodes.size() != 1)
	{
		return false;
	}
	const auto& node = selectedNodes.front();
	const auto parent = node->parent();
	if (parent == nullptr)
	{
		return false;
	}
	const auto index = parent->indexOfChild(node);
	return index < parent->children().size() - 1;
}

void Hierarchy::refreshNodeList()
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
	addElementRecursive(m_canvas->rootNode(), 0, RefreshesLayoutYN::No);

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

void Hierarchy::refreshNodeNames()
{
	for (const auto& element : m_elements)
	{
		for (const auto& child : element.hierarchyNode()->children())
		{
			if (child->name() == U"NodeNameContainer")
			{
				for (const auto& grandChild : child->children())
				{
					if (const auto pLabel = grandChild->getComponent<Label>())
					{
						if (grandChild->name() != U"ToggleFolded")
						{
							pLabel->setText(element.node()->name());
							break;
						}
					}
				}
				break;
			}
		}
	}
}

void Hierarchy::selectNodes(const Array<std::shared_ptr<Node>>& nodes)
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

void Hierarchy::selectAll()
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

void Hierarchy::selectSingleNode(const std::shared_ptr<Node>& node)
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

void Hierarchy::clearSelection(bool clearShiftSelectOrigin)
{
	for (auto& element : m_elements)
	{
		if (element.editorSelected())
		{
			element.setEditorSelected(EditorSelectedYN::No);
		}
	}
	if (clearShiftSelectOrigin)
	{
		m_shiftSelectOriginNode.reset();
	}
}

bool Hierarchy::hasSelection() const
{
	return std::any_of(m_elements.begin(), m_elements.end(), [](const Element& element) { return element.editorSelected(); });
}

void Hierarchy::onClickNewNode()
{
	// 最後に選択したノードの兄弟として新規ノードを作成
	if (const auto lastEditorSelectedNode = m_lastEditorSelectedNode.lock())
	{
		if (const auto parentNode = lastEditorSelectedNode->parent())
		{
			onClickNewNode(parentNode);
		}
		else
		{
			onClickNewNode(m_canvas->rootNode());
		}
	}
	else
	{
		onClickNewNode(m_canvas->rootNode());
	}
}

void Hierarchy::onClickNewNode(const std::shared_ptr<Node>& parentNode)
{
	if (!parentNode)
	{
		throw Error{ U"Parent node is nullptr" };
	}

	// 記憶された種類のConstraintを使用してノードを作成
	std::shared_ptr<Node> newNode = parentNode->emplaceChild(
		U"Node",
		m_defaults->defaultConstraint());
	refreshNodeList();
	selectSingleNode(newNode);
}

void Hierarchy::onClickDelete()
{
	for (auto it = m_elements.begin(); it != m_elements.end();)
	{
		if (it->editorSelected())
		{
			if (it->node()->removeFromParent())
			{
				it = m_elements.erase(it);
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
	refreshNodeList();
	clearSelection();
}

void Hierarchy::onClickCut()
{
	onClickCopy();
	onClickDelete();
}

void Hierarchy::onClickCopy()
{
	// 選択中のノードをコピー（親が選択中の場合は子は含めない）
	m_copiedNodeJSONs.clear();
	const auto selectedNodesExcludingChildren = getSelectedNodesExcludingChildren();
	m_copiedNodeJSONs.reserve(selectedNodesExcludingChildren.size());
	for (const auto& node : selectedNodesExcludingChildren)
	{
		m_copiedNodeJSONs.push_back(node->toJSON());
	}
}

void Hierarchy::onClickPaste()
{
	// 最後に選択したノードの子として貼り付け
	const auto lastEditorSelectedNode = m_lastEditorSelectedNode.lock();
	onClickPaste(lastEditorSelectedNode ? lastEditorSelectedNode : m_canvas->rootNode());
}

void Hierarchy::onClickPaste(const std::shared_ptr<Node>& parentNode)
{
	if (!parentNode)
	{
		throw Error{ U"Parent node is nullptr" };
	}
	if (m_copiedNodeJSONs.empty())
	{
		return;
	}

	Array<std::shared_ptr<Node>> pastedNodes;
	pastedNodes.reserve(m_copiedNodeJSONs.size());
	for (const auto& json : m_copiedNodeJSONs)
	{
		pastedNodes.push_back(parentNode->addChild(Node::CreateFromJSON(json)));
	}
	refreshNodeList();
	selectNodes(pastedNodes);
}

bool Hierarchy::canPaste() const
{
	return !m_copiedNodeJSONs.empty();
}

void Hierarchy::onClickDuplicate()
{
	// 選択中のノードを複製（親が選択中の場合は子は含めない）
	Array<std::shared_ptr<Node>> nodesToDuplicate = getSelectedNodesExcludingChildren();
	Array<std::shared_ptr<Node>> duplicatedNodes;
	duplicatedNodes.reserve(nodesToDuplicate.size());
	for (const auto& node : nodesToDuplicate)
	{
		if (const auto parentNode = node->parent())
		{
			const auto duplicatedNode = parentNode->addChild(Node::CreateFromJSON(node->toJSON()));
			duplicatedNodes.push_back(duplicatedNode);
		}
	}
	refreshNodeList();
	selectNodes(duplicatedNodes);
}

void Hierarchy::onClickCreateEmptyParent()
{
	const auto selectedNodes = getSelectedNodes();
	if (selectedNodes.size() != 1)
	{
		return;
	}
	const auto selectedNode = selectedNodes.front();
	const auto parent = selectedNode->parent();
	if (parent == nullptr)
	{
		return;
	}

	// 現在位置に空の親ノードを作成
	const auto emptyParent = Node::Create(U"Empty", selectedNode->constraint());
	const size_t index = parent->indexOfChild(selectedNode);
	parent->addChildAtIndex(emptyParent, index);

	// 選択中のノードを空の親ノードの子にする
	selectedNode->removeFromParent();
	selectedNode->setConstraint(AnchorConstraint
		{
			.anchorMin = Anchor::MiddleCenter,
			.anchorMax = Anchor::MiddleCenter,
			.sizeDelta = selectedNode->size(),
			.sizeDeltaPivot = Anchor::MiddleCenter,
		});
	emptyParent->addChild(selectedNode);

	refreshNodeList();
	selectSingleNode(emptyParent);
}

void Hierarchy::onClickMoveUp()
{
	// 複数選択されている場合は親ごとにグループ化して処理
	HashTable<std::shared_ptr<Node>, Array<std::shared_ptr<Node>>> nodesByParent;
	for (const auto& element : m_elements)
	{
		if (element.editorSelected())
		{
			const auto parent = element.node()->parent();
			if (parent)
			{
				nodesByParent[parent].push_back(element.node());
			}
		}
	}

	// 各親に対して処理
	for (auto& [parent, nodes] : nodesByParent)
	{
		// インデックスの小さい順にソート
		nodes.sort_by([parent](const auto& a, const auto& b) { return parent->indexOfChild(a) < parent->indexOfChild(b); });

		// 小さいインデックスから処理
		for (const auto& node : nodes)
		{
			const size_t index = parent->indexOfChild(node);
			if (index > 0)
			{
				node->removeFromParent();
				parent->addChildAtIndex(node, index - 1);
			}
		}
	}

	refreshNodeList();
	// 選択状態を復元
	for (const auto& [parent, nodes] : nodesByParent)
	{
		setNodesEditorSelected(nodes, EditorSelectedYN::Yes);
	}
}

void Hierarchy::onClickMoveDown()
{
	// 複数選択されている場合は親ごとにグループ化して処理
	HashTable<std::shared_ptr<Node>, Array<std::shared_ptr<Node>>> nodesByParent;
	for (const auto& element : m_elements)
	{
		if (element.editorSelected())
		{
			const auto parent = element.node()->parent();
			if (parent)
			{
				nodesByParent[parent].push_back(element.node());
			}
		}
	}

	// 各親に対して処理
	for (auto& [parent, nodes] : nodesByParent)
	{
		// インデックスの大きい順にソート
		nodes.sort_by([parent](const auto& a, const auto& b) { return parent->indexOfChild(a) > parent->indexOfChild(b); });

		// 大きいインデックスから処理
		for (const auto& node : nodes)
		{
			const size_t index = parent->indexOfChild(node);
			if (index < parent->children().size() - 1)
			{
				node->removeFromParent();
				parent->addChildAtIndex(node, index + 1);
			}
		}
	}

	refreshNodeList();
	// 選択状態を復元
	for (const auto& [parent, nodes] : nodesByParent)
	{
		setNodesEditorSelected(nodes, EditorSelectedYN::Yes);
	}
}

void Hierarchy::drawSelectedNodesGizmo() const
{
	// 選択中のノードを描画
	for (const auto& element : m_elements)
	{
		if (element.editorSelected() && element.node()->activeInHierarchy())
		{
			element.node()->rect().drawFrame(2, Palette::Orange);
		}
	}

	// ホバー中のノードを描画
	if (const auto editorHoveredNode = m_editorHoveredNode.lock())
	{
		if (editorHoveredNode->activeInHierarchy())
		{
			editorHoveredNode->rect().draw(ColorF{ 1.0, 0.15 });
			editorHoveredNode->rect().drawFrame(1, Palette::White);
		}
	}
}

Array<std::shared_ptr<Node>> Hierarchy::getSelectedNodesExcludingChildren() const
{
	// 選択中のノードを列挙
	// ただし、親が選択中の場合は子は含めない
	Array<std::shared_ptr<Node>> selectedNodes;
	for (const auto& element : m_elements)
	{
		if (element.editorSelected())
		{
			if (!hasAncestorSelected(element.node()))
			{
				selectedNodes.push_back(element.node());
			}
		}
	}
	return selectedNodes;
}

void Hierarchy::unfoldForNode(const std::shared_ptr<Node>& node)
{
	if (const auto parent = node->parent())
	{
		if (auto pParentElement = getElementByNode(parent))
		{
			pParentElement->setFolded(FoldedYN::No);
		}
		unfoldForNode(parent);
	}
}

void Hierarchy::applyFolding()
{
	for (const auto& element : m_elements)
	{
		applyFoldingRecursive(element, true);
	}
}

void Hierarchy::update()
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
			const InteractState interactState = element.node()->currentInteractState();
			switch (interactState)
			{
			case InteractState::Default:
				element.elementDetail().hierarchyStateLabel->setText(element.node()->selected() ? U"[Selected]" : U"[Default]");
				break;
			case InteractState::Hovered:
				element.elementDetail().hierarchyStateLabel->setText(element.node()->selected() ? U"[Selected, Hovered]" : U"[Hovered]");
				break;
			case InteractState::Pressed:
				element.elementDetail().hierarchyStateLabel->setText(element.node()->selected() ? U"[Selected, Pressed]" : U"[Pressed]");
				break;
			case InteractState::Disabled:
				element.elementDetail().hierarchyStateLabel->setText(element.node()->selected() ? U"[Selected, Disabled]" : U"[Disabled]");
				break;
			default:
				throw Error{ U"Invalid InteractState: {}"_fmt(static_cast<std::underlying_type_t<InteractState>>(interactState)) };
			}
		}
		else
		{
			element.elementDetail().hierarchyStateLabel->setText(element.node()->selected() ? U"[Selected]" : U"");
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

		if (element.elementDetail().hierarchyToggleFoldedNode && element.elementDetail().hierarchyToggleFoldedNode->isClicked())
		{
			element.toggleFolded();
		}
	}

	if (m_hierarchyRootNode->isClicked())
	{
		clearSelection();
	}
}