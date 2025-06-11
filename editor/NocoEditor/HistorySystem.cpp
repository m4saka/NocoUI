#include "HistorySystem.hpp"

HistorySystem::HistorySystem()
{
	m_lastRecordTime.start();
}

// スタックをクリア
void HistorySystem::clear()
{
	m_undoStack.clear();
	m_redoStack.clear();
	m_lastRecordedState = none;
}

// 現在の状態を記録するかチェックし、必要なら記録
void HistorySystem::recordStateIfNeeded(const JSON& currentState)
{
	if (m_isRestoring)
	{
		return;
	}
	
	// 前回の記録から一定時間経過しているかチェック
	if (m_lastRecordTime.elapsed() < MinTimeBetweenRecords)
	{
		return;
	}
	
	// 前回の状態と同じかチェック
	if (m_lastRecordedState.has_value())
	{
		const String currentHash = currentState.formatMinimum();
		const String lastHash = m_lastRecordedState->formatMinimum();
		if (currentHash == lastHash)
		{
			return;
		}
	}
	
	// 状態を記録
	if (m_lastRecordedState.has_value())
	{
		m_undoStack.push_back(*m_lastRecordedState);
		
		// スタックサイズ制限
		if (m_undoStack.size() > MaxStackSize)
		{
			m_undoStack.pop_front();
		}
		
		// 新しい操作が行われたらRedoスタックをクリア
		m_redoStack.clear();
	}
	
	m_lastRecordedState = currentState;
	m_lastRecordTime.restart();
}

// 元に戻す
Optional<JSON> HistorySystem::undo(const JSON& currentState)
{
	if (m_undoStack.empty())
	{
		return none;
	}
	
	// 現在の状態をRedoスタックに保存
	m_redoStack.push_back(currentState);
	
	// Undoスタックから状態を復元
	const JSON restoreState = m_undoStack.back();
	m_undoStack.pop_back();
	
	m_lastRecordedState = restoreState;
	m_lastRecordTime.restart();
	m_isRestoring = true;
	
	return restoreState;
}

// やり直す
Optional<JSON> HistorySystem::redo(const JSON& currentState)
{
	if (m_redoStack.empty())
	{
		return none;
	}
	
	// 現在の状態をUndoスタックに保存
	m_undoStack.push_back(currentState);
	
	// Redoスタックから状態を復元
	const JSON restoreState = m_redoStack.back();
	m_redoStack.pop_back();
	
	m_lastRecordedState = restoreState;
	m_lastRecordTime.restart();
	m_isRestoring = true;
	
	return restoreState;
}

// 復元処理完了を通知
void HistorySystem::endRestore()
{
	m_isRestoring = false;
}

// Undo可能かチェック
bool HistorySystem::canUndo() const
{
	return !m_undoStack.empty();
}

// Redo可能かチェック
bool HistorySystem::canRedo() const
{
	return !m_redoStack.empty();
}