#include "Inspector.hpp"
#include "EditorUtility.hpp"

using namespace noco;

Inspector::Inspector(const std::shared_ptr<Canvas>& canvas, const std::shared_ptr<Canvas>& editorCanvas, const std::shared_ptr<ContextMenu>& contextMenu, const std::shared_ptr<DialogOpener>& dialogOpener, const std::shared_ptr<Defaults>& defaults, std::function<void()> hierarchyRefreshNodeList)
	: m_canvas(canvas)
	, m_editorCanvas(editorCanvas)
	, m_inspectorFrameNode(editorCanvas->rootNode()->emplaceChild(
		U"InspectorFrame",
		AnchorConstraint
		{
			.anchorMin = Anchor::TopRight,
			.anchorMax = Anchor::BottomRight,
			.posDelta = Vec2{ 0, MenuBarHeight },
			.sizeDelta = Vec2{ 400, -MenuBarHeight },
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
	, m_dialogOpener(dialogOpener)
	, m_defaults(defaults)
	, m_hierarchyRefreshNodeList(std::move(hierarchyRefreshNodeList))
{
	m_inspectorFrameNode->emplaceComponent<RectRenderer>(ColorF{ 0.8 });
	m_inspectorInnerFrameNode->setVerticalScrollable(true);
	m_inspectorInnerFrameNode->setBoxChildrenLayout(VerticalLayout{ .padding = LRTB{ 6, 6, 6, 6 } });
	m_inspectorRootNode->setBoxChildrenLayout(VerticalLayout{ .spacing = 4 });
	
	// コンテキストメニューを追加
	m_inspectorInnerFrameNode->emplaceComponent<ContextMenuOpener>(contextMenu,
		Array<MenuElement>
		{
			MenuItem{ U"Sprite を追加", U"", KeyS, [this] 
			{ 
				if (const auto node = m_targetNode.lock())
				{
					node->emplaceComponent<Sprite>();
					refreshInspector();
				}
			} },
			MenuItem{ U"RectRenderer を追加", U"", KeyR, [this] 
			{ 
				if (const auto node = m_targetNode.lock())
				{
					node->emplaceComponent<RectRenderer>();
					refreshInspector();
				}
			} },
			MenuItem{ U"TextBox を追加", U"", KeyT, [this] 
			{ 
				if (const auto node = m_targetNode.lock())
				{
					node->emplaceComponent<TextBox>();
					refreshInspector();
				}
			} },
			MenuItem{ U"Label を追加", U"", KeyL, [this] 
			{ 
				if (const auto node = m_targetNode.lock())
				{
					node->emplaceComponent<Label>();
					refreshInspector();
				}
			} },
			MenuItem{ U"InputBlocker を追加", U"", KeyI, [this] 
			{ 
				if (const auto node = m_targetNode.lock())
				{
					node->emplaceComponent<InputBlocker>();
					refreshInspector();
				}
			} },
			MenuItem{ U"EventTrigger を追加", U"", KeyE, [this] 
			{ 
				if (const auto node = m_targetNode.lock())
				{
					node->emplaceComponent<EventTrigger>();
					refreshInspector();
				}
			} },
			MenuItem{ U"Placeholder を追加", U"", KeyP, [this] 
			{ 
				if (const auto node = m_targetNode.lock())
				{
					node->emplaceComponent<Placeholder>();
					refreshInspector();
				}
			} },
		});
}

void Inspector::refreshInspector()
{
	const double scrollY = m_inspectorRootNode->scrollOffset().y;
	setTargetNode(m_targetNode.lock());
	m_inspectorRootNode->resetScrollOffset(RefreshesLayoutYN::No, RefreshesLayoutYN::No);
	m_inspectorRootNode->scroll(Vec2{ 0, scrollY }, RefreshesLayoutYN::No);
	m_editorCanvas->refreshLayout();
}

void Inspector::setTargetNode(const std::shared_ptr<Node>& targetNode)
{
	if (!targetNode || targetNode.get() != m_targetNode.lock().get())
	{
		// 選択ノードが変更された場合、以前のノード用の折り畳み状況をクリア
		m_componentFoldedStates.clear();
	}

	m_targetNode = targetNode;

	m_inspectorRootNode->removeChildrenAll();

	if (targetNode)
	{
		const auto nodeNameNode = createNodeNameNode();
		m_inspectorRootNode->addChild(nodeNameNode);

		const auto constraintNode = createConstraintNode();
		m_inspectorRootNode->addChild(constraintNode);

		const auto nodeSettingNode = createNodeSettingNode();
		m_inspectorRootNode->addChild(nodeSettingNode);

		const auto layoutNode = createBoxChildrenLayoutNode();
		m_inspectorRootNode->addChild(layoutNode);

		const auto transformEffectNode = createTransformEffectNode();
		m_inspectorRootNode->addChild(transformEffectNode);

		for (const std::shared_ptr<IComponent>& component : targetNode->components())
		{
			const HasInteractivePropertyValueYN hasInteractivePropertyValue = component->hasInteractiveProperty() ? HasInteractivePropertyValueYN::Yes : HasInteractivePropertyValueYN::No;
			
			const auto componentNode = createComponentNode(component, targetNode, hasInteractivePropertyValue);
			m_inspectorRootNode->addChild(componentNode);
		}

		m_inspectorRootNode->addChild(CreateButtonNode(
			U"＋ コンポーネントを追加(A)",
			BoxConstraint
			{
				.sizeDelta = Vec2{ 200, 24 },
				.margin = LRTB{ 0, 0, 24, 24 },
			},
			[this](const std::shared_ptr<Node>& node)
			{
				m_inspectorInnerFrameNode->getComponent<ContextMenuOpener>()->openManually(node->rect().center());
			}))->addClickHotKey(KeyA, CtrlYN::No, AltYN::Yes, ShiftYN::No, EnabledWhileTextEditingYN::Yes);
	}
}

void Inspector::update()
{
	// 必要に応じて更新処理を追加
}

std::shared_ptr<Node> Inspector::CreateHeadingNode(StringView headingText, IsFoldedYN& isFoldedRef, std::function<void()> onToggle)
{
	auto headingNode = Node::Create(U"Heading", BoxConstraint
		{
			.sizeRatio = Vec2{ 1, 0 },
			.sizeDelta = Vec2{ 0, 24 },
			.margin = LRTB{ 0, 0, 0, 0 },
		});
	headingNode->emplaceComponent<RectRenderer>(
		PropertyValue<ColorF>(ColorF{ HeadingColor, 0.8 }).withHovered(ColorF{ HeadingColor + ColorF{ 0.05 }, 0.8 }).withPressed(ColorF{ HeadingColor - ColorF{ 0.05 }, 0.8 }),
		Palette::Black,
		0.0,
		3.0);
	const auto arrowLabel = headingNode->emplaceComponent<Label>(
		isFoldedRef ? U"▶" : U"▼",
		U"",
		14,
		ColorF{ 1.0, 0.6 },
		HorizontalAlign::Left,
		VerticalAlign::Middle,
		LRTB{ 5, 5, 0, 0 });
	headingNode->emplaceComponent<Label>(
		headingText,
		U"",
		14,
		Palette::White,
		HorizontalAlign::Left,
		VerticalAlign::Middle,
		LRTB{ 25, 5, 0, 0 });
	headingNode->addOnClick([arrowLabel, &isFoldedRef, onToggle](const std::shared_ptr<Node>& node)
		{
			if (const auto parent = node->parent())
			{
				bool inactiveNodeExists = false;

				// 見出し以外のアクティブを入れ替え
				for (const auto& child : parent->children())
				{
					if (child != node)
					{
						child->setActive(!child->activeSelf());
						if (!child->activeSelf())
						{
							inactiveNodeExists = true;
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

				// 折り畳み状態を更新
				isFoldedRef = IsFoldedYN{ inactiveNodeExists };

				// トグル時処理があれば実行
				if (onToggle)
				{
					onToggle();
				}
			}
		});

	return headingNode;
}

// 内部クラス定義
namespace
{
	class PropertyTextBox : public ComponentBase
	{
	private:
		std::shared_ptr<TextBox> m_textBox;
		std::function<void(StringView)> m_fnSetValue;

		void update(const std::shared_ptr<Node>&) override
		{
			if (m_textBox->isChanged())
			{
				m_fnSetValue(m_textBox->text());
			}
		}

		void draw(const Node&) const override
		{
		}

	public:
		explicit PropertyTextBox(const std::shared_ptr<TextBox>& textBox, std::function<void(StringView)> fnSetValue)
			: ComponentBase{ {} }
			, m_textBox(textBox)
			, m_fnSetValue(std::move(fnSetValue))
		{
		}
	};

	class CheckboxToggler : public ComponentBase
	{
	private:
		std::shared_ptr<Label> m_checkboxLabel;
		std::function<void(bool)> m_onChange;
		bool m_checked;

		void update(const std::shared_ptr<Node>& node) override
		{
			if (node->isClicked())
			{
				m_checked = !m_checked;
				m_checkboxLabel->setText(m_checked ? U"☑" : U"☐");
				if (m_onChange)
				{
					m_onChange(m_checked);
				}
			}
		}

		void draw(const Node&) const override
		{
		}

	public:
		explicit CheckboxToggler(const std::shared_ptr<Label>& checkboxLabel, bool initialChecked, std::function<void(bool)> onChange)
			: ComponentBase{ {} }
			, m_checkboxLabel(checkboxLabel)
			, m_onChange(std::move(onChange))
			, m_checked(initialChecked)
		{
		}
	};
}

std::shared_ptr<Node> Inspector::CreateNodeNameTextboxNode(const std::shared_ptr<Node>& node, std::function<void()> onChange)
{
	const auto propertyNode = Node::Create(
		U"NodeName",
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
	textBoxNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
	const auto textBox = textBoxNode->emplaceComponent<TextBox>(U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
	textBox->setText(node->name(), IgnoreIsChangedYN::Yes);
	textBoxNode->addComponent(std::make_shared<PropertyTextBox>(textBox, [node, onChange](StringView value)
		{
			if (value.empty())
			{
				node->setName(U"Node");
			}
			else
			{
				node->setName(value);
			}
			if (onChange)
			{
				onChange();
			}
		}));
	textBoxNode->addClickHotKey(KeyF2);
	return propertyNode;
}

std::shared_ptr<Node> Inspector::CreatePropertyNode(StringView headingText, StringView value, std::function<void(StringView)> onSubmit)
{
	const auto propertyNode = Node::Create(
		headingText,
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
		});
	labelNode->emplaceComponent<Label>(
		headingText,
		U"",
		14,
		Palette::White,
		HorizontalAlign::Left,
		VerticalAlign::Top,
		LRTB{ 5, 5, 5, 5 });
	const auto textBoxNode = propertyNode->emplaceChild(
		U"TextBox",
		BoxConstraint
		{
			.sizeDelta = Vec2{ 0, 26 },
			.flexibleWeight = 1,
		});
	textBoxNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
	const auto textBox = textBoxNode->emplaceComponent<TextBox>(U"", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
	textBox->setText(value, IgnoreIsChangedYN::Yes);
	textBoxNode->addComponent(std::make_shared<PropertyTextBox>(textBox, std::move(onSubmit)));
	return propertyNode;
}

std::shared_ptr<Node> Inspector::CreateCheckboxNode(bool checked, std::function<void(bool)> onChange)
{
	const auto checkboxNode = Node::Create(
		U"Checkbox",
		BoxConstraint
		{
			.sizeRatio = Vec2{ 0, 1 },
			.sizeDelta = Vec2{ 32, 0 },
		});
	checkboxNode->emplaceComponent<RectRenderer>(
		PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.2, 0.8 }).withSmoothTime(0.05),
		PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHovered(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05),
		1.0,
		4.0);
	const auto checkboxLabel = checkboxNode->emplaceComponent<Label>(
		checked ? U"☑" : U"☐",
		U"",
		14,
		PropertyValue<ColorF>{ Palette::White }.withDisabled(ColorF{ 0.5 }),
		HorizontalAlign::Center,
		VerticalAlign::Middle);
	checkboxNode->addComponent(std::make_shared<CheckboxToggler>(checkboxLabel, checked, std::move(onChange)));
	return checkboxNode;
}

std::shared_ptr<Node> Inspector::CreateBoolPropertyNode(StringView headingText, bool value, std::function<void(bool)> onChange)
{
	const auto propertyNode = Node::Create(
		headingText,
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
		});
	labelNode->emplaceComponent<Label>(
		headingText,
		U"",
		14,
		Palette::White,
		HorizontalAlign::Left,
		VerticalAlign::Top,
		LRTB{ 5, 5, 5, 5 });
	propertyNode->addChild(CreateCheckboxNode(value, std::move(onChange)));
	return propertyNode;
}

std::shared_ptr<Node> Inspector::createNodeNameNode()
{
	const auto node = m_targetNode.lock();
	if (!node) return nullptr;

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

	nodeNameNode->addChild(CreateCheckboxNode(node->activeSelf().getBool(), [node](bool value) { node->setActive(value); }));
	nodeNameNode->addChild(CreateNodeNameTextboxNode(node, m_hierarchyRefreshNodeList));

	return nodeNameNode;
}

std::shared_ptr<Node> Inspector::createNodeSettingNode()
{
	const auto node = m_targetNode.lock();
	if (!node) return nullptr;

	auto nodeSettingNode = Node::Create(
		U"NodeSetting",
		BoxConstraint
		{
			.sizeRatio = Vec2{ 1, 0 },
			.margin = LRTB{ 0, 0, 0, 8 },
		});
	nodeSettingNode->setBoxChildrenLayout(VerticalLayout{ .padding = m_nodeSettingFolded ? LRTB::Zero() : LRTB{ 0, 0, 0, 8 } });
	nodeSettingNode->emplaceComponent<RectRenderer>(ColorF{ 0.3, 0.3 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0);

	nodeSettingNode->addChild(CreateHeadingNode(U"Node Settings", m_nodeSettingFolded,
		[this]()
		{
			refreshInspector();
		}));

	nodeSettingNode->addChild(
		Node::Create(
			U"TopPadding",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = Vec2{ 0, 8 },
			}))->setActive(!m_nodeSettingFolded.getBool());

	const auto fnAddBoolChild =
		[this, &nodeSettingNode, &node](StringView name, bool currentValue, auto fnSetValue)
		{
			nodeSettingNode->addChild(CreateBoolPropertyNode(name, currentValue, fnSetValue))->setActive(!m_nodeSettingFolded.getBool());
		};
	fnAddBoolChild(U"isHitTarget", node->isHitTarget().getBool(), [node](bool value) { node->setIsHitTarget(value); });
	fnAddBoolChild(U"inheritsChildrenHoveredState", node->inheritsChildrenHoveredState(), [node](bool value) { node->setInheritsChildrenHoveredState(value); });
	fnAddBoolChild(U"inheritsChildrenPressedState", node->inheritsChildrenPressedState(), [node](bool value) { node->setInheritsChildrenPressedState(value); });
	fnAddBoolChild(U"interactable", node->interactable().getBool(), [node](bool value) { node->setInteractable(value); });
	fnAddBoolChild(U"horizontalScrollable", node->horizontalScrollable(), [node](bool value) { node->setHorizontalScrollable(value); });
	fnAddBoolChild(U"verticalScrollable", node->verticalScrollable(), [node](bool value) { node->setVerticalScrollable(value); });
	fnAddBoolChild(U"clippingEnabled", node->clippingEnabled().getBool(), [node](bool value) { node->setClippingEnabled(value); });

	nodeSettingNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly);

	return nodeSettingNode;
}

std::shared_ptr<Node> Inspector::createBoxChildrenLayoutNode()
{
	const auto node = m_targetNode.lock();
	if (!node) return nullptr;

	auto layoutNode = Node::Create(
		U"BoxChildrenLayout",
		BoxConstraint
		{
			.sizeRatio = Vec2{ 1, 0 },
			.margin = LRTB{ 0, 0, 0, 8 },
		});
	layoutNode->setBoxChildrenLayout(VerticalLayout{ .padding = m_childrenLayoutFolded ? LRTB::Zero() : LRTB{ 0, 0, 0, 8 } });
	layoutNode->emplaceComponent<RectRenderer>(ColorF{ 0.3, 0.3 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0);
	
	layoutNode->addChild(CreateHeadingNode(U"Box Children Layout", m_childrenLayoutFolded,
		[this]()
		{
			refreshInspector();
		}));

	// TODO: レイアウト設定の実装
	
	layoutNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly);
	return layoutNode;
}

std::shared_ptr<Node> Inspector::createConstraintNode()
{
	const auto node = m_targetNode.lock();
	if (!node) return nullptr;

	auto constraintNode = Node::Create(
		U"Constraint",
		BoxConstraint
		{
			.sizeRatio = Vec2{ 1, 0 },
			.margin = LRTB{ 0, 0, 0, 8 },
		});
	constraintNode->setBoxChildrenLayout(VerticalLayout{ .padding = m_constraintFolded ? LRTB::Zero() : LRTB{ 0, 0, 0, 8 } });
	constraintNode->emplaceComponent<RectRenderer>(ColorF{ 0.3, 0.3 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0);
	
	constraintNode->addChild(CreateHeadingNode(U"Constraint", m_constraintFolded,
		[this]()
		{
			refreshInspector();
		}));

	// TODO: 制約設定の実装
	
	constraintNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly);
	return constraintNode;
}

std::shared_ptr<Node> Inspector::createTransformEffectNode()
{
	const auto node = m_targetNode.lock();
	if (!node) return nullptr;

	auto transformEffectNode = Node::Create(
		U"TransformEffect",
		BoxConstraint
		{
			.sizeRatio = Vec2{ 1, 0 },
			.margin = LRTB{ 0, 0, 0, 8 },
		});
	transformEffectNode->setBoxChildrenLayout(VerticalLayout{ .padding = m_transformEffectFolded ? LRTB::Zero() : LRTB{ 0, 0, 0, 8 } });
	transformEffectNode->emplaceComponent<RectRenderer>(ColorF{ 0.3, 0.3 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0);
	
	transformEffectNode->addChild(CreateHeadingNode(U"Transform Effect", m_transformEffectFolded,
		[this]()
		{
			refreshInspector();
		}));

	// TODO: トランスフォーム効果の実装
	
	transformEffectNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly);
	return transformEffectNode;
}

std::shared_ptr<Node> Inspector::createComponentNode(const std::shared_ptr<IComponent>& component, const std::shared_ptr<Node>& node, HasInteractivePropertyValueYN hasInteractivePropertyValue)
{
	auto& isFolded = m_componentFoldedStates[component.get()];
	
	auto componentNode = Node::Create(
		component->componentType(),
		BoxConstraint
		{
			.sizeRatio = Vec2{ 1, 0 },
			.margin = LRTB{ 0, 0, 0, 8 },
		});
	componentNode->setBoxChildrenLayout(VerticalLayout{ .padding = isFolded ? LRTB::Zero() : LRTB{ 0, 0, 0, 8 } });
	componentNode->emplaceComponent<RectRenderer>(ColorF{ 0.3, 0.3 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0);
	
	componentNode->addChild(CreateHeadingNode(component->componentType(), isFolded,
		[this]()
		{
			refreshInspector();
		}));

	// TODO: コンポーネントプロパティの実装
	
	componentNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly);
	return componentNode;
}

// 残りのCreateメソッドのスタブ実装
std::shared_ptr<Node> Inspector::CreateVec2PropertyNode(StringView headingText, const Vec2& value, std::function<void(const Vec2&)> onChange, const Vec2& stepSize)
{
	// TODO: 実装
	return CreatePropertyNode(headingText, Format(value), [onChange](StringView) {});
}

std::shared_ptr<Node> Inspector::CreateVec4PropertyNode(StringView headingText, const Vec4& value, std::function<void(const Vec4&)> onChange, const Vec4& stepSize)
{
	// TODO: 実装
	return CreatePropertyNode(headingText, Format(value), [onChange](StringView) {});
}

std::shared_ptr<Node> Inspector::CreateLRTBPropertyNode(StringView headingText, const LRTB& value, std::function<void(const LRTB&)> onChange, const Vec4& stepSize)
{
	// TODO: 実装
	return CreatePropertyNode(headingText, Format(value), [onChange](StringView) {});
}

std::shared_ptr<Node> Inspector::CreateColorPropertyNode(StringView headingText, const ColorF& value, std::function<void(const ColorF&)> onChange, const Vec4& stepSize)
{
	// TODO: 実装
	return CreatePropertyNode(headingText, Format(value), [onChange](StringView) {});
}

std::shared_ptr<Node> Inspector::CreateEnumPropertyNode(StringView headingText, StringView value, std::function<void(StringView)> onChange, const std::shared_ptr<ContextMenu>& contextMenu, const Array<String>& enumCandidates)
{
	// TODO: 実装
	return CreatePropertyNode(headingText, value, onChange);
}