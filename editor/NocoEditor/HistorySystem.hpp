#pragma once
#include <Siv3D.hpp>

// 元に戻す/やり直し機能を管理するクラス
class HistorySystem
{
private:
	static constexpr size_t MaxStackSize = 50;
	static constexpr Duration MinTimeBetweenRecords = 0.5s; // 記録の最小間隔
	
	Array<JSON> m_undoStack;
	Array<JSON> m_redoStack;
	Stopwatch m_lastRecordTime;
	Optional<JSON> m_lastRecordedState;
	bool m_isRestoring = false;

public:
	HistorySystem();
	
	// スタックをクリア
	void clear();
	
	// 現在の状態を記録するかチェックし、必要なら記録
	void recordStateIfNeeded(const JSON& currentState);
	
	// 元に戻す
	[[nodiscard]]
	Optional<JSON> undo(const JSON& currentState);
	
	// やり直す
	[[nodiscard]]
	Optional<JSON> redo(const JSON& currentState);
	
	// 復元処理完了を通知
	void endRestore();
	
	// Undo可能かチェック
	[[nodiscard]]
	bool canUndo() const;
	
	// Redo可能かチェック
	[[nodiscard]]
	bool canRedo() const;
};