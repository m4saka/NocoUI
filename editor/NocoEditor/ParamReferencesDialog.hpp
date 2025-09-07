#pragma once
#include <NocoUI.hpp>
#include "EditorDialog.hpp"

namespace noco::editor
{
	class ParamReferencesDialog : public IDialog
	{
	private:
		String m_paramName;
		std::shared_ptr<Canvas> m_canvas;
		
		// ダイアログ内のコントロール
		std::shared_ptr<Node> m_listNode;
		
	public:
		explicit ParamReferencesDialog(
			const String& paramName,
			std::shared_ptr<Canvas> canvas)
			: m_paramName{ paramName }
			, m_canvas{ std::move(canvas) }
		{
		}
		
		double dialogWidth() const override
		{
			return 400;
		}
		
		Array<DialogButtonDesc> buttonDescs() const override
		{
			return Array<DialogButtonDesc>
			{
				DialogButtonDesc
				{
					.text = U"閉じる",
					.mnemonicInput = KeyC,
					.isDefaultButton = IsDefaultButtonYN::Yes,
				},
			};
		}
		
		void createDialogContent(const std::shared_ptr<Node>& contentRootNode, const std::shared_ptr<ContextMenu>&, std::function<void()>) override
		{
			contentRootNode->setChildrenLayout(VerticalLayout{ .padding = LRTB{ 16 } });
			
			// ヘッダー情報
			const auto headerNode = contentRootNode->addChild(Node::Create(
				U"Header",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 40 },
					.margin = LRTB{ 0, 0, 0, 16 },
				}));
			headerNode->setChildrenLayout(VerticalLayout{});
			
			// パラメータ名表示
			headerNode->emplaceComponent<Label>(
				U"パラメータ: {}"_fmt(m_paramName),
				U"",
				16,
				Palette::White,
				HorizontalAlign::Left,
				VerticalAlign::Top);
			
			// 参照数表示
			Array<std::tuple<String, String, String>> tempReferences;
			findParameterReferences(m_paramName, tempReferences);
			const size_t refCount = tempReferences.size();
			headerNode->emplaceComponent<Label>(
				U"参照数: {}"_fmt(refCount),
				U"",
				14,
				ColorF{ 0.8 },
				HorizontalAlign::Left,
				VerticalAlign::Bottom);
			
			// スクロール可能なリストエリア
			const auto scrollAreaNode = contentRootNode->addChild(Node::Create(
				U"ScrollArea",
				InlineRegion
				{
					.sizeRatio = Vec2{ 1, 1 },
					.margin = LRTB{ 0, 0, 0, 16 },
				}));
			scrollAreaNode->emplaceComponent<RectRenderer>(
				ColorF{ 0.15 },
				ColorF{ 0.4 },
				1.0,
				0.0,
				4.0);
			scrollAreaNode->setChildrenLayout(VerticalLayout{ .padding = LRTB{ 8 } });
			
			// 参照一覧を作成
			m_listNode = scrollAreaNode;
			populateReferenceList();
		}
		
		void onResult(StringView) override
		{
		}
		
	private:
		void populateReferenceList()
		{
			if (!m_canvas || !m_listNode)
			{
				return;
			}
			
			// 既存の子ノードをクリア
			m_listNode->removeChildrenAll();
			
			// パラメータ参照を検索
			Array<std::tuple<String, String, String>> references;
			findParameterReferences(m_paramName, references);
			
			if (references.empty())
			{
				// 参照なしの場合
				const auto noRefNode = m_listNode->addChild(Node::Create(
					U"NoReferences",
					InlineRegion
					{
						.sizeRatio = Vec2{ 1, 0 },
						.sizeDelta = Vec2{ 0, 32 },
						.margin = LRTB{ 8 },
					}));
				noRefNode->emplaceComponent<Label>(
					U"参照しているプロパティはありません",
					U"",
					14,
					ColorF{ 0.6 },
					HorizontalAlign::Center,
					VerticalAlign::Middle);
			}
			else
			{
				// 参照リストを表示
				for (const auto& [nodeName, componentName, propertyName] : references)
				{
					const auto refItemNode = m_listNode->addChild(Node::Create(
						U"RefItem_{}_{}"_fmt(componentName, propertyName),
						InlineRegion
						{
							.sizeRatio = Vec2{ 1, 0 },
							.sizeDelta = Vec2{ 0, 24 },
							.margin = LRTB{ 4, 4, 2, 2 },
						}));
					refItemNode->setChildrenLayout(HorizontalLayout{});
					refItemNode->emplaceComponent<RectRenderer>(
						ColorF{ 0.2, 0.3 },
						ColorF{ 0.5, 0.3 },
						1.0,
						0.0,
						2.0);
					
					// ノード名
					const auto nodeNameNode = refItemNode->addChild(Node::Create(
						U"NodeName",
						InlineRegion
						{
							.sizeRatio = Vec2{ 0, 1 },
							.sizeDelta = Vec2{ 0, 0 },
							.flexibleWeight = 4,
							.margin = LRTB{ 8, 4, 0, 0 },
						}));
					nodeNameNode->emplaceComponent<Label>(
						nodeName.isEmpty() ? U"(Canvas)" : nodeName,
						U"",
						12,
						ColorF{ 0.8, 0.9, 1.0 },
						HorizontalAlign::Left,
						VerticalAlign::Middle);
					
					// コンポーネント名とプロパティ名
					const auto propInfoNode = refItemNode->addChild(Node::Create(
						U"PropertyInfo",
						InlineRegion
						{
							.sizeRatio = Vec2{ 0, 1 },
							.sizeDelta = Vec2{ 0, 0 },
							.flexibleWeight = 6,
							.margin = LRTB{ 4, 8, 0, 0 },
						}));
					propInfoNode->emplaceComponent<Label>(
						U"{}.{}"_fmt(componentName, propertyName),
						U"",
						12,
						Palette::White,
						HorizontalAlign::Left,
						VerticalAlign::Middle);
				}
			}
			
			m_listNode->setInlineRegionToFitToChildren(FitTarget::HeightOnly);
		}
		
		void findParameterReferences(const String& paramName, Array<std::tuple<String, String, String>>& references)
		{
			if (!m_canvas)
			{
				return;
			}
			
			std::function<void(std::shared_ptr<Node>)> walkNode = [&](std::shared_ptr<Node> node)
			{
				const String nodeName = node->name();
				
				// Transformコンポーネントのプロパティをチェック
				const auto& transform = node->transform();
				if (transform.translate().paramRef() == paramName)
				{
					references.emplace_back(nodeName, U"Transform", U"translate");
				}
				if (transform.scale().paramRef() == paramName)
				{
					references.emplace_back(nodeName, U"Transform", U"scale");
				}
				if (transform.pivot().paramRef() == paramName)
				{
					references.emplace_back(nodeName, U"Transform", U"pivot");
				}
				if (transform.rotation().paramRef() == paramName)
				{
					references.emplace_back(nodeName, U"Transform", U"rotation");
				}
				if (transform.hitTestAffected().paramRef() == paramName)
				{
					references.emplace_back(nodeName, U"Transform", U"hitTestAffected");
				}
				if (transform.color().paramRef() == paramName)
				{
					references.emplace_back(nodeName, U"Transform", U"color");
				}
				
				// Node自体のプロパティをチェック
				if (node->activeSelfParamRef() == paramName)
				{
					references.emplace_back(nodeName, U"Node", U"activeSelf");
				}
				if (node->interactableParamRef() == paramName)
				{
					references.emplace_back(nodeName, U"Node", U"interactable");
				}
				if (node->styleStateParamRef() == paramName)
				{
					references.emplace_back(nodeName, U"Node", U"styleState");
				}
				
				// コンポーネントのプロパティをチェック
				for (const auto& component : node->components())
				{
					for (IProperty* property : component->properties())
					{
						if (property->paramRef() == paramName)
						{
							String componentName = U"Component";
							if (const auto* serializableComponent = dynamic_cast<SerializableComponentBase*>(component.get()))
							{
								componentName = serializableComponent->type();
							}
							
							references.emplace_back(nodeName, componentName, String{ property->name() });
						}
					}
				}

				for (const auto& child : node->children())
				{
					walkNode(child);
				}
			};

			for (const auto& child : m_canvas->children())
			{
				walkNode(child);
			}
		}
	};
}
