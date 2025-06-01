#pragma once
#include <Siv3D.hpp>
#include "NocoUI.hpp"
#include "EditorTypes.hpp"
#include "Menu.hpp"
#include "Dialog.hpp"
#include "EditorDefaults.hpp"

class Inspector
{
private:
	std::shared_ptr<noco::Canvas> m_canvas;
	std::shared_ptr<noco::Canvas> m_editorCanvas;
	std::shared_ptr<noco::Node> m_inspectorFrameNode;
	std::shared_ptr<noco::Node> m_inspectorInnerFrameNode;
	std::shared_ptr<noco::Node> m_inspectorRootNode;
	std::shared_ptr<ContextMenu> m_contextMenu;
	std::shared_ptr<DialogOpener> m_dialogOpener;
	std::weak_ptr<noco::Node> m_targetNode;
	IsFoldedYN m_nodeSettingFolded = IsFoldedYN::No;
	IsFoldedYN m_childrenLayoutFolded = IsFoldedYN::No;
	IsFoldedYN m_constraintFolded = IsFoldedYN::No;
	IsFoldedYN m_transformEffectFolded = IsFoldedYN::No;
	HashTable<const noco::ComponentBase*, IsFoldedYN> m_componentFoldedStates;
	std::shared_ptr<Defaults> m_defaults;
	std::function<void()> m_hierarchyRefreshNodeList;

	std::shared_ptr<noco::Node> createNodeNameNode();
	
	std::shared_ptr<noco::Node> createNodeSettingNode();
	
	std::shared_ptr<noco::Node> createBoxChildrenLayoutNode();
	
	std::shared_ptr<noco::Node> createConstraintNode();
	
	std::shared_ptr<noco::Node> createTransformEffectNode();
	
	std::shared_ptr<noco::Node> createComponentNode(const std::shared_ptr<noco::ComponentBase>& component, const std::shared_ptr<noco::Node>& node, HasInteractivePropertyValueYN hasInteractivePropertyValue);

public:
	static constexpr ColorF HeadingColor = ColorF{ 0.3, 0.8 };

	explicit Inspector(const std::shared_ptr<noco::Canvas>& canvas, const std::shared_ptr<noco::Canvas>& editorCanvas, const std::shared_ptr<ContextMenu>& contextMenu, const std::shared_ptr<DialogOpener>& dialogOpener, const std::shared_ptr<Defaults>& defaults, std::function<void()> hierarchyRefreshNodeList);

	void refreshInspector();

	void setTargetNode(const std::shared_ptr<noco::Node>& targetNode);

	void update();

	// Static helper methods for creating property nodes
	static std::shared_ptr<noco::Node> CreateHeadingNode(StringView headingText, IsFoldedYN& isFoldedRef, std::function<void()> onToggle = nullptr);

	static std::shared_ptr<noco::Node> CreateNodeNameTextboxNode(const std::shared_ptr<noco::Node>& node, std::function<void()> onChange);

	static std::shared_ptr<noco::Node> CreatePropertyNode(StringView headingText, StringView value, std::function<void(StringView)> onSubmit);

	static std::shared_ptr<noco::Node> CreateVec2PropertyNode(StringView headingText, const Vec2& value, std::function<void(const Vec2&)> onChange, const Vec2& stepSize = { 1, 1 });

	static std::shared_ptr<noco::Node> CreateVec4PropertyNode(StringView headingText, const Vec4& value, std::function<void(const Vec4&)> onChange, const Vec4& stepSize = { 1, 1, 1, 1 });

	static std::shared_ptr<noco::Node> CreateLRTBPropertyNode(StringView headingText, const noco::LRTB& value, std::function<void(const noco::LRTB&)> onChange, const Vec4& stepSize = { 1, 1, 1, 1 });

	static std::shared_ptr<noco::Node> CreateColorPropertyNode(StringView headingText, const ColorF& value, std::function<void(const ColorF&)> onChange, const Vec4& stepSize = { 0.01, 0.01, 0.01, 0.01 });

	static std::shared_ptr<noco::Node> CreateEnumPropertyNode(StringView headingText, StringView value, std::function<void(StringView)> onChange, const std::shared_ptr<ContextMenu>& contextMenu, const Array<String>& enumCandidates);

	static std::shared_ptr<noco::Node> CreateCheckboxNode(bool checked, std::function<void(bool)> onChange);

	static std::shared_ptr<noco::Node> CreateBoolPropertyNode(StringView headingText, bool value, std::function<void(bool)> onChange);
};