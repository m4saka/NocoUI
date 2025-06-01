#pragma once
#include <Siv3D.hpp>
#include "NocoUI.hpp"
#include "EditorTypes.hpp"
#include "Menu.hpp"
#include "EditorDefaults.hpp"

class Hierarchy
{
private:
	std::shared_ptr<noco::Canvas> m_canvas;
	std::shared_ptr<noco::Node> m_hierarchyFrameNode;
	std::shared_ptr<noco::Node> m_hierarchyInnerFrameNode;
	std::shared_ptr<noco::Node> m_hierarchyRootNode;
	std::weak_ptr<noco::Canvas> m_editorCanvas;
	std::weak_ptr<noco::Node> m_editorHoveredNode;
	std::weak_ptr<noco::Node> m_shiftSelectOriginNode;
	std::weak_ptr<noco::Node> m_lastEditorSelectedNode;
	std::shared_ptr<ContextMenu> m_contextMenu;
	Array<JSON> m_copiedNodeJSONs;
	std::shared_ptr<Defaults> m_defaults;

	struct ElementDetail
	{
		size_t nestLevel = 0;
		std::shared_ptr<noco::Node> node;
		std::shared_ptr<noco::Node> hierarchyNode;
		std::shared_ptr<noco::RectRenderer> hierarchyRectRenderer;
		std::shared_ptr<noco::Label> hierarchyStateLabel;
		std::shared_ptr<noco::Node> hierarchyToggleFoldedNode;
		std::shared_ptr<noco::Label> hierarchyToggleFoldedLabel;
	};

	class Element
	{
	private:
		Hierarchy* m_pHierarchy = nullptr;
		ElementDetail m_elementDetail;
		noco::EditorSelectedYN m_editorSelected = noco::EditorSelectedYN::No;
		noco::FoldedYN m_folded = noco::FoldedYN::No;

	public:
		Element(Hierarchy* pHierarchy, const ElementDetail& elementDetail);

		[[nodiscard]]
		noco::EditorSelectedYN editorSelected() const;

		void setEditorSelected(noco::EditorSelectedYN editorSelected);

		[[nodiscard]]
		const ElementDetail& elementDetail() const;

		[[nodiscard]]
		const std::shared_ptr<noco::Node>& node() const;

		[[nodiscard]]
		const std::shared_ptr<noco::Node>& hierarchyNode() const;

		void toggleFolded();

		void setFolded(noco::FoldedYN folded);

		[[nodiscard]]
		noco::FoldedYN folded() const;

		[[nodiscard]]
		static noco::PropertyValue<ColorF> HierarchyRectFillColor(noco::EditorSelectedYN editorSelected);

		[[nodiscard]]
		static noco::PropertyValue<ColorF> HierarchyRectOutlineColor(noco::EditorSelectedYN editorSelected);
	};
	Array<Element> m_elements;

	void addElementRecursive(const std::shared_ptr<noco::Node>& node, size_t nestLevel, noco::RefreshesLayoutYN refreshesLayout);

	Element createElement(const std::shared_ptr<noco::Node>& node, size_t nestLevel);

	[[nodiscard]]
	bool isFoldedRecursive(const std::shared_ptr<noco::Node>& node) const;

	[[nodiscard]]
	bool isEditorSelectedRecursive(const std::shared_ptr<noco::Node>& node, RecursionYN recursive) const;

	[[nodiscard]]
	Array<Element*> getSelectedElements();

	[[nodiscard]]
	Array<const Element*> getSelectedElements() const;

	[[nodiscard]]
	Array<std::shared_ptr<noco::Node>> getSelectedNodes();

	[[nodiscard]]
	Array<std::shared_ptr<noco::Node>> getSelectedNodes() const;

	void setNodesEditorSelected(const Array<std::shared_ptr<noco::Node>>& nodes, noco::EditorSelectedYN editorSelected);

	[[nodiscard]]
	Element* getElementByHierarchyNode(const std::shared_ptr<noco::Node>& hierarchyNode);

	[[nodiscard]]
	const Element* getElementByHierarchyNode(const std::shared_ptr<noco::Node>& hierarchyNode) const;

	[[nodiscard]]
	Element* getElementByNode(const std::shared_ptr<noco::Node>& node);

	[[nodiscard]]
	const Element* getElementByNode(const std::shared_ptr<noco::Node>& node) const;

	void applyFoldingRecursive(const Element& element, bool visible);

	[[nodiscard]]
	bool hasAncestorSelected(const std::shared_ptr<noco::Node>& node) const;

	[[nodiscard]]
	bool canMoveUp() const;

	[[nodiscard]]
	bool canMoveDown() const;

public:
	explicit Hierarchy(const std::shared_ptr<noco::Canvas>& canvas, const std::shared_ptr<noco::Canvas>& editorCanvas, const std::shared_ptr<ContextMenu>& contextMenu, const std::shared_ptr<Defaults>& defaults);

	void refreshNodeList();

	void refreshNodeNames();

	void selectNodes(const Array<std::shared_ptr<noco::Node>>& nodes);

	void selectAll();

	void selectSingleNode(const std::shared_ptr<noco::Node>& node);

	void clearSelection(bool clearShiftSelectOrigin = true);

	[[nodiscard]]
	bool hasSelection() const;

	void update();

	void onClickNewNode();
	
	void onClickNewNode(const std::shared_ptr<noco::Node>& parentNode);

	void onClickDelete();

	void onClickCut();

	void onClickCopy();

	void onClickPaste();
	
	void onClickPaste(const std::shared_ptr<noco::Node>& parentNode);

	bool canPaste() const;

	void onClickDuplicate();

	void onClickCreateEmptyParent();

	void onClickMoveUp();

	void onClickMoveDown();

	void drawSelectedNodesGizmo() const;

	[[nodiscard]]
	Array<std::shared_ptr<noco::Node>> getSelectedNodesExcludingChildren() const;

	void unfoldForNode(const std::shared_ptr<noco::Node>& node);

	void applyFolding();

	[[nodiscard]]
	std::weak_ptr<noco::Node> selectedNode() const { return m_lastEditorSelectedNode; }
};