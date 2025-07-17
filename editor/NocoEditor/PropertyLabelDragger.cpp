#include "PropertyLabelDragger.hpp"
#include "NocoUI/Node.hpp"

namespace noco::editor
{
	void PropertyLabelDragger::update(const std::shared_ptr<Node>& node)
	{
		if (!node || !m_fnGetValue || !m_fnSetValue)
		{
			return;
		}
		
		const bool isHovered = node->isHovered();
		
		// ドラッグ開始
		if (isHovered && MouseL.down() && !m_isDragging)
		{
			m_isDragging = true;
			m_dragStartPos = Cursor::PosF();
			m_dragStartValue = m_fnGetValue();
			
			// カーソルを変更
			Cursor::RequestStyle(CursorStyle::ResizeLeftRight);
			
			// ドラッグ開始イベント（履歴記録などに使用）
			if (m_fnOnDragStart)
			{
				m_fnOnDragStart();
			}
		}
		
		if (m_isDragging && MouseL.pressed())
		{
			node->preventDragScroll();
		}
		
		// ドラッグ中
		if (m_isDragging)
		{
			if (MouseL.pressed())
			{
				// ドラッグ距離を計算
				Vec2 currentPos = Cursor::PosF();
				const double deltaX = currentPos.x - m_dragStartPos.x;
				
				// 画面端でのカーソル位置ループ処理
				const auto screenSize = Scene::Size();
				
				if (currentPos.x <= 0)
				{
					// 左端に到達したら右端に移動
					Cursor::SetPos(Point{ static_cast<int32>(screenSize.x - 1), static_cast<int32>(currentPos.y) });
					m_dragStartPos.x = screenSize.x - 1 - deltaX;
				}
				else if (currentPos.x >= screenSize.x - 1)
				{
					// 右端に到達したら左端に移動
					Cursor::SetPos(Point{ 0, static_cast<int32>(currentPos.y) });
					m_dragStartPos.x = 0 - deltaX;
				}
				
				// 速度調整
				double speedMultiplier = 0.25;
				if (KeyShift.pressed() && KeyControl.pressed())
				{
					speedMultiplier = 2.5; // 10倍速、刻みなし
				}
				else if (KeyAlt.pressed() && KeyControl.pressed())
				{
					speedMultiplier = 0.025; // 1/10倍速、刻みなし
				}
				else if (KeyShift.pressed())
				{
					speedMultiplier = 2.5; // 10倍速
				}
				else if (KeyAlt.pressed())
				{
					speedMultiplier = 0.025; // 1/10倍速
				}
				else if (KeyControl.pressed())
				{
					speedMultiplier = 0.25; // 通常速度
				}
				
				// 新しい値を計算
				double newValue = m_dragStartValue + deltaX * m_step * speedMultiplier;
				
				// 刻み調整（m_stepをステップ値として使用）
				if (KeyShift.pressed() && KeyControl.pressed())
				{
					// 刻みなし（10倍速）
					// 丸め処理を行わない
				}
				else if (KeyAlt.pressed() && KeyControl.pressed())
				{
					// 刻みなし（1/10倍速）
					// 丸め処理を行わない
				}
				else if (KeyShift.pressed())
				{
					// 10倍のステップ刻み
					const double largeStep = m_step * 10.0;
					newValue = Math::Round(newValue / largeStep) * largeStep;
				}
				else if (KeyAlt.pressed())
				{
					// 1/10のステップ刻み
					const double smallStep = m_step * 0.1;
					newValue = Math::Round(newValue / smallStep) * smallStep;
				}
				else if (KeyControl.pressed())
				{
					// 刻みなし（自由な値）
					// 丸め処理を行わない
				}
				else
				{
					// 通常のステップ刻み（デフォルト）
					newValue = Math::Round(newValue / m_step) * m_step;
				}
				
				// 値の範囲制限
				newValue = Clamp(newValue, m_minValue, m_maxValue);
				
				// 値を設定
				m_fnSetValue(newValue);
				
				// カーソルを維持
				Cursor::RequestStyle(CursorStyle::ResizeLeftRight);
			}
			else
			{
				// ドラッグ終了
				m_isDragging = false;
				
				// ドラッグ終了イベント（履歴記録などに使用）
				if (m_fnOnDragEnd)
				{
					m_fnOnDragEnd();
				}
			}
		}
		
		// ホバー時のカーソル変更
		if (isHovered && !m_isDragging)
		{
			Cursor::RequestStyle(CursorStyle::ResizeLeftRight);
		}
	}
}
