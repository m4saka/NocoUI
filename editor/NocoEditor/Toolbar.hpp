#pragma once
#include <NocoUI.hpp>
#include "MenuBar.hpp"

namespace noco::editor
{
	class Toolbar
	{
	public:
		static constexpr int32 ToolbarHeight = 32;
		
	private:
		static constexpr int32 ButtonSize = 28;
		static constexpr int32 ButtonMargin = 4;
		static constexpr int32 BorderLineThickness = 2;

		std::shared_ptr<Canvas> m_editorCanvas;
		std::shared_ptr<Canvas> m_editorOverlayCanvas;
		std::shared_ptr<Node> m_toolbarRootNode;
		Font m_iconFont{ FontMethod::MSDF, 18, Typeface::Icon_MaterialDesign };
		
		// ボタンノードを管理するためのマップ
		HashTable<String, std::shared_ptr<Node>> m_buttonNodes;
		
		// ボタンの有効/無効を判定する関数
		struct ButtonInfo
		{
			std::shared_ptr<Node> node;
			std::function<bool()> enableCondition;
		};
		HashTable<String, ButtonInfo> m_buttons;

	public:
		explicit Toolbar(const std::shared_ptr<Canvas>& editorCanvas, const std::shared_ptr<Canvas>& editorOverlayCanvas)
			: m_editorCanvas(editorCanvas)
			, m_editorOverlayCanvas(editorOverlayCanvas)
			, m_toolbarRootNode(editorCanvas->rootNode()->emplaceChild(
				U"Toolbar",
				AnchorRegion
				{
					.anchorMin = Anchor::TopLeft,
					.anchorMax = Anchor::TopRight,
					.posDelta = Vec2{ 0, MenuBarHeight },
					.sizeDelta = Vec2{ 0, ToolbarHeight },
					.sizeDeltaPivot = Anchor::TopLeft,
				}))
		{
			m_toolbarRootNode->setChildrenLayout(
				HorizontalLayout
				{
					.padding = LRTB{ .left = ButtonMargin, .top = BorderLineThickness },
					.spacing = ButtonMargin,
					.verticalAlign = VerticalAlign::Middle,
				});
			m_toolbarRootNode->emplaceComponent<RectRenderer>(ColorF{ 0.95 });
			
			// MenuBarとの境界線を追加
			m_toolbarRootNode->emplaceChild(
				U"BorderLine",
				AnchorRegion
				{
					.anchorMin = Anchor::TopLeft,
					.anchorMax = Anchor::TopRight,
					.posDelta = Vec2{ 0, 0 },
					.sizeDelta = Vec2{ 0, BorderLineThickness },
					.sizeDeltaPivot = Anchor::TopLeft,
				})
				->emplaceComponent<RectRenderer>(ColorF{ 0.8 });
		}

		std::shared_ptr<Node> addButton(StringView name, StringView icon, StringView tooltip, std::function<void()> onClick, std::function<bool()> enableCondition = nullptr)
		{
			auto buttonNode = m_toolbarRootNode->emplaceChild(
				name,
				InlineRegion
				{
					.sizeRatio = Vec2{ 0, 0 },
					.sizeDelta = Vec2{ ButtonSize, ButtonSize },
				});
			
			// ボタンの背景
			buttonNode->emplaceComponent<RectRenderer>(
				PropertyValue<ColorF>{ ColorF{ 0.95, 0.0 }, ColorF{ 0.88 }, ColorF{ 0.83 }, ColorF{ 0.95, 0.0 }, 0.1 },
				PropertyValue<ColorF>{ ColorF{ 0.0, 0.0 }, ColorF{ 0.4 }, ColorF{ 0.4 }, ColorF{ 0.0, 0.0 }, 0.1 },
				0.0,
				4.0);
			
			// アイコンラベル
			const auto iconLabel = buttonNode->emplaceComponent<Label>(
				icon,
				U"",
				18,
				PropertyValue<ColorF>{ ColorF{ 0.2 } }.withDisabled(ColorF{ 0.2, 0.5 }),
				HorizontalAlign::Center,
				VerticalAlign::Middle);
			iconLabel->setFont(m_iconFont);
			
			// クリック時の処理
			buttonNode->addOnClick([onClick = std::move(onClick)](const std::shared_ptr<Node>&)
			{
				if (onClick)
				{
					onClick();
				}
			});
			
			// ツールチップ
			if (!tooltip.empty())
			{
				buttonNode->emplaceComponent<TooltipOpener>(m_editorOverlayCanvas, tooltip);
			}

			const String nameStr{ name };
			m_buttonNodes[nameStr] = buttonNode;
			m_buttons[nameStr] = ButtonInfo{ buttonNode, enableCondition };
			if (enableCondition)
			{
				buttonNode->setInteractable(enableCondition());
			}

			return buttonNode;
		}

		void addSeparator()
		{
			m_toolbarRootNode->emplaceChild(
				U"Separator",
				InlineRegion
				{
					.sizeRatio = Vec2{ 0, 0.6 },
					.sizeDelta = Vec2{ 1, 0 },
				})
				->emplaceComponent<RectRenderer>(ColorF{ 0.7 });
		}
		
		void updateButtonStates()
		{
			// 各ボタンの有効/無効状態を更新
			for (const auto& [name, buttonInfo] : m_buttons)
			{
				if (buttonInfo.enableCondition)
				{
					// 判定関数がある場合は、その結果に基づいて有効/無効を設定
					buttonInfo.node->setInteractable(buttonInfo.enableCondition());
				}
			}
		}
	};
}
