#pragma once
#include <Siv3D.hpp>
#include "INodeContainer.hpp"
#include "Node.hpp"
#include "Component/IFocusable.hpp"
#include "Param.hpp"
#include "ParamUtils.hpp"

namespace noco
{
	class ITextBox;
	class IFocusable;
	class ComponentFactory;
	class SubCanvas;

	struct CanvasUpdateContext
	{
		bool keyInputBlocked = false;
		std::weak_ptr<Node> hoveredNode;
		std::weak_ptr<Node> scrollableHoveredNode;
		std::weak_ptr<ITextBox> editingTextBox;
		std::weak_ptr<Node> draggingNode;
		std::weak_ptr<Node> focusedNode;

		// フレーム間で引き継ぐためresetしない
		std::weak_ptr<Node> dragScrollingNode;

		void clear()
		{
			keyInputBlocked = false;
			editingTextBox.reset();
			hoveredNode.reset();
			scrollableHoveredNode.reset();
			draggingNode.reset();
			// focusedNodeはフレーム間で保持するためクリアしない
		}
	};

	namespace detail
	{
		inline int32 s_lastCopiedCanvasUpdateContextToPrevFrameCount = -1;
		inline CanvasUpdateContext s_canvasUpdateContext;
		inline CanvasUpdateContext s_prevCanvasUpdateContext;
		inline bool s_isEditorMode = false;

		inline void ClearCanvasUpdateContextIfNeeded()
		{
			const int32 currentFrameCount = Scene::FrameCount();
			if (s_lastCopiedCanvasUpdateContextToPrevFrameCount == currentFrameCount)
			{
				return;
			}

			// 前のフレームの状態を保存
			s_lastCopiedCanvasUpdateContextToPrevFrameCount = currentFrameCount;
			s_prevCanvasUpdateContext = s_canvasUpdateContext;

			// 現在のフレームの状態をクリア
			s_canvasUpdateContext.clear();
		}
	}

	namespace CurrentFrame
	{
		/// @brief 現在フレームでホバー中のノードが存在するかどうかを取得
		/// @return ホバー中のノードが存在する場合はtrue、存在しない場合はfalseを返す
		[[nodiscard]]
		inline bool AnyNodeHovered()
		{
			return !detail::s_canvasUpdateContext.hoveredNode.expired();
		}

		/// @brief 現在フレームでホバー中のノードを取得
		/// @return ホバー中のノード。存在しない場合はnullptrを返す
		[[nodiscard]]
		inline std::shared_ptr<Node> GetHoveredNode()
		{
			return detail::s_canvasUpdateContext.hoveredNode.lock();
		}

		/// @brief 現在フレームでホバー中のスクロール可能ノードが存在するかどうかを取得
		/// @return ホバー中のスクロール可能ノードが存在する場合はtrue、存在しない場合はfalseを返す
		[[nodiscard]]
		inline bool AnyScrollableNodeHovered()
		{
			return !detail::s_canvasUpdateContext.scrollableHoveredNode.expired();
		}

		/// @brief 現在フレームでホバー中のスクロール可能ノードを取得
		/// @return ホバー中のスクロール可能ノード。存在しない場合はnullptrを返す
		[[nodiscard]]
		inline std::shared_ptr<Node> GetScrollableHoveredNode()
		{
			return detail::s_canvasUpdateContext.scrollableHoveredNode.lock();
		}

		/// @brief 現在フレームでテキストボックス(TextBox/TextArea)を編集中かどうかを取得
		/// @return テキストボックスを編集中の場合はtrue、編集中でない場合はfalseを返す
		[[nodiscard]]
		inline bool IsEditingTextBox()
		{
			return !detail::s_canvasUpdateContext.editingTextBox.expired();
		}

		/// @brief 現在フレームで編集中のテキストボックス(TextBox/TextArea)を取得
		/// @return 編集中のテキストボックス。編集中でない場合はnullptrを返す
		[[nodiscard]]
		inline std::shared_ptr<ITextBox> GetEditingTextBox()
		{
			return detail::s_canvasUpdateContext.editingTextBox.lock();
		}

		/// @brief 現在フレームでいずれかのノードをドラッグ中かどうかを取得
		/// @return ドラッグ中の場合はtrue、ドラッグ中でない場合はfalseを返す
		/// @note DragDropSourceコンポーネントを持つノードが対象となる
		[[nodiscard]]
		inline bool IsDraggingNode()
		{
			return !detail::s_canvasUpdateContext.draggingNode.expired();
		}

		/// @brief 現在フレームでドラッグ中のノードを取得
		/// @return ドラッグ中のノード。ドラッグ中でない場合はnullptrを返す
		/// @note DragDropSourceコンポーネントを持つノードが対象となる
		[[nodiscard]]
		inline std::shared_ptr<Node> GetDraggingNode()
		{
			return detail::s_canvasUpdateContext.draggingNode.lock();
		}

		/// @brief 現在フレームでの以降のupdateKeyInputの呼び出しをしないようにする
		/// @note ダイアログの背面にキー入力が伝わらないようにする場合などに使用
		inline void BlockKeyInput()
		{
			detail::s_canvasUpdateContext.keyInputBlocked = true;
		}

		/// @brief 現在フレームでキー入力がブロックされているかどうかを取得
		/// @return キー入力がブロックされている場合はtrue、ブロックされていない場合はfalseを返す
		[[nodiscard]]
		inline bool HasKeyInputBlocked()
		{
			return detail::s_canvasUpdateContext.keyInputBlocked;
		}
		
		/// @brief 現在フレームでフォーカス中のノードが存在するかどうかを取得
		/// @return フォーカス中のノードが存在する場合はtrue
		[[nodiscard]]
		inline bool IsFocused()
		{
			return !detail::s_canvasUpdateContext.focusedNode.expired();
		}
		
		/// @brief 現在フレームでフォーカス中のノードを取得
		/// @return フォーカス中のノード。存在しない場合はnullptrを返す
		[[nodiscard]]
		inline std::shared_ptr<Node> GetFocusedNode()
		{
			return detail::s_canvasUpdateContext.focusedNode.lock();
		}
		
		/// @brief 現在フレームでフォーカス中のノードを設定
		/// @param node フォーカスを設定するノード。nullptrを指定すると、フォーカスを外す
		/// @note テキストボックスやフォーカス可能なコンポーネントでの排他処理で使用される
		///       IFocusableを継承したコンポーネントを持つノードを指定すると、フォーカス時にfocus関数、フォーカス解除時にblur関数が呼ばれる
		inline void SetFocusedNode(const std::shared_ptr<Node>& node)
		{
			auto currentFocused = detail::s_canvasUpdateContext.focusedNode.lock();
			
			// 同じノードの場合は何もしない
			if (currentFocused == node)
			{
				return;
			}

			// 既存のフォーカスがあった場合は外れるためコンポーネントのblurを呼ぶ
			if (currentFocused)
			{
				for (const auto& component : currentFocused->components())
				{
					if (auto focusable = std::dynamic_pointer_cast<IFocusable>(component))
					{
						focusable->blur(currentFocused);
					}
				}
			}

			// 新しいフォーカスを設定
			detail::s_canvasUpdateContext.focusedNode = node;

			// フォーカスした場合はコンポーネントのfocusを呼ぶ
			if (node)
			{
				for (const auto& component : node->components())
				{
					if (auto focusable = std::dynamic_pointer_cast<IFocusable>(component))
					{
						focusable->focus(node);
					}
				}
			}
		}

		/// @brief 指定したノードがフォーカス中であればフォーカスを外す
		/// @param node フォーカスを外す対象のノード
		inline bool UnfocusNodeIfFocused(const std::shared_ptr<Node>& node)
		{
			if (detail::s_canvasUpdateContext.focusedNode.lock() == node)
			{
				SetFocusedNode(nullptr);
				return true;
			}
			return false;
		}
	}

	namespace PrevFrame
	{
		/// @brief 前フレームでホバー中のノードが存在するかどうかを取得
		/// @return ホバー中のノードが存在する場合はtrue、存在しない場合はfalseを返す
		[[nodiscard]]
		inline bool AnyNodeHovered()
		{
			return !detail::s_prevCanvasUpdateContext.hoveredNode.expired();
		}
		
		/// @brief 前フレームでホバー中のノードを取得
		/// @return ホバー中のノード。存在しない場合はnullptrを返す
		[[nodiscard]]
		inline std::shared_ptr<Node> GetHoveredNode()
		{
			return detail::s_prevCanvasUpdateContext.hoveredNode.lock();
		}
		
		/// @brief 前フレームでホバー中のスクロール可能ノードが存在するかどうかを取得
		/// @return ホバー中のスクロール可能ノードが存在する場合はtrue、存在しない場合はfalseを返す
		[[nodiscard]]
		inline bool AnyScrollableNodeHovered()
		{
			return !detail::s_prevCanvasUpdateContext.scrollableHoveredNode.expired();
		}
		
		/// @brief 前フレームでホバー中のスクロール可能ノードを取得
		/// @return ホバー中のスクロール可能ノード。存在しない場合はnullptrを返す
		[[nodiscard]]
		inline std::shared_ptr<Node> GetScrollableHoveredNode()
		{
			return detail::s_prevCanvasUpdateContext.scrollableHoveredNode.lock();
		}

		/// @brief 前フレームでテキストボックス(TextBox/TextArea)を編集中かどうかを取得
		/// @return テキストボックスを編集中の場合はtrue、編集中でない場合はfalseを返す
		[[nodiscard]]
		inline bool IsEditingTextBox()
		{
			return !detail::s_prevCanvasUpdateContext.editingTextBox.expired();
		}

		/// @brief 前フレームで編集中のテキストボックス(TextBox/TextArea)を取得
		/// @return 編集中のテキストボックス。編集中でない場合はnullptrを返す
		[[nodiscard]]
		inline std::shared_ptr<ITextBox> GetEditingTextBox()
		{
			return detail::s_prevCanvasUpdateContext.editingTextBox.lock();
		}

		/// @brief 前フレームでいずれかのノードをドラッグ中かどうかを取得
		/// @return ドラッグ中の場合はtrue、ドラッグ中でない場合はfalseを返す
		/// @note DragDropSourceコンポーネントを持つノードが対象となる
		[[nodiscard]]
		inline bool IsDraggingNode()
		{
			return !detail::s_prevCanvasUpdateContext.draggingNode.expired();
		}

		/// @brief 前フレームでドラッグ中のノードを取得
		/// @return ドラッグ中のノード。ドラッグ中でない場合はnullptrを返す
		/// @note DragDropSourceコンポーネントを持つノードが対象となる
		[[nodiscard]]
		inline std::shared_ptr<Node> GetDraggingNode()
		{
			return detail::s_prevCanvasUpdateContext.draggingNode.lock();
		}
	}

	/// @brief ホバー中のノードが存在するかどうかを取得
	/// @return ホバー中のノードが存在する場合はtrue、存在しない場合はfalseを返す
	/// @note フレーム内での実行順序の影響を受けないよう、現在フレームおよび前フレームの両方を見る
	[[nodiscard]]
	inline bool AnyNodeHovered()
	{
		return CurrentFrame::AnyNodeHovered() || PrevFrame::AnyNodeHovered();
	}

	/// @brief ホバー中のノードを取得
	/// @return ホバー中のノード。存在しない場合はnullptrを返す
	/// @note フレーム内での実行順序の影響を受けないよう、現在フレームおよび前フレームの両方を見る
	[[nodiscard]]
	inline std::shared_ptr<Node> GetHoveredNode()
	{
		if (auto node = CurrentFrame::GetHoveredNode())
		{
			return node;
		}
		return PrevFrame::GetHoveredNode();
	}

	/// @brief ホバー中のスクロール可能ノードが存在するかどうかを取得
	/// @return ホバー中のスクロール可能ノードが存在する場合はtrue、存在しない場合はfalseを返す
	/// @note フレーム内での実行順序の影響を受けないよう、現在フレームおよび前フレームの両方を見る
	[[nodiscard]]
	inline bool AnyScrollableNodeHovered()
	{
		return CurrentFrame::AnyScrollableNodeHovered() || PrevFrame::AnyScrollableNodeHovered();
	}

	/// @brief ホバー中のスクロール可能ノードを取得
	/// @return ホバー中のスクロール可能ノード。存在しない場合はnullptrを返す
	/// @note フレーム内での実行順序の影響を受けないよう、現在フレームおよび前フレームの両方を見る
	[[nodiscard]]
	inline std::shared_ptr<Node> GetScrollableHoveredNode()
	{
		if (auto node = CurrentFrame::GetScrollableHoveredNode())
		{
			return node;
		}
		return PrevFrame::GetScrollableHoveredNode();
	}

	/// @brief テキストボックス(TextBox/TextArea)を編集中かどうかを取得
	/// @return テキストボックスを編集中の場合はtrue、編集中でない場合はfalseを返す
	/// @note フレーム内での実行順序の影響を受けないよう、現在フレームおよび前フレームの両方を見る
	[[nodiscard]]
	inline bool IsEditingTextBox()
	{
		return CurrentFrame::IsEditingTextBox() || PrevFrame::IsEditingTextBox();
	}

	/// @brief 編集中のテキストボックス(TextBox/TextArea)を取得
	/// @return 編集中のテキストボックス。編集中でない場合はnullptrを返す
	/// @note フレーム内での実行順序の影響を受けないよう、現在フレームおよび前フレームの両方を見る
	[[nodiscard]]
	inline std::shared_ptr<ITextBox> GetEditingTextBox()
	{
		if (auto textBox = CurrentFrame::GetEditingTextBox())
		{
			return textBox;
		}
		return PrevFrame::GetEditingTextBox();
	}

	/// @brief いずれかのノードをドラッグ中かどうかを取得
	/// @return ドラッグ中の場合はtrue、ドラッグ中でない場合はfalseを返す
	/// @note フレーム内での実行順序の影響を受けないよう、現在フレームおよび前フレームの両方を見る
	/// @note DragDropSourceコンポーネントを持つノードが対象となる
	[[nodiscard]]
	inline bool IsDraggingNode()
	{
		return CurrentFrame::IsDraggingNode() || PrevFrame::IsDraggingNode();
	}

	/// @brief ドラッグ中のノードを取得
	/// @return ドラッグ中のノード。ドラッグ中でない場合はnullptrを返す
	/// @note フレーム内での実行順序の影響を受けないよう、現在フレームおよび前フレームの両方を見る
	/// @note DragDropSourceコンポーネントを持つノードが対象となる
	[[nodiscard]]
	inline std::shared_ptr<Node> GetDraggingNode()
	{
		if (auto node = CurrentFrame::GetDraggingNode())
		{
			return node;
		}
		return PrevFrame::GetDraggingNode();
	}

	namespace detail
	{
		inline void SetEditorMode(bool isEditorMode)
		{
			s_isEditorMode = isEditorMode;
		}

		[[nodiscard]]
		inline bool IsEditorMode()
		{
			return s_isEditorMode;
		}
	}

	enum class AutoFitMode : uint8
	{
		None,                 // 自動調整なし
		Contain,              // アスペクト比維持で全体が見える
		Cover,                // アスペクト比維持で画面を埋める
		FitWidth,             // 幅に合わせてスケール（アスペクト比維持）
		FitHeight,            // 高さに合わせてスケール（アスペクト比維持）
		FitWidthMatchHeight,  // 幅はスケール、高さはサイズ変更
		FitHeightMatchWidth,  // 高さはスケール、幅はサイズ変更
		MatchSize,            // Canvasサイズをシーンサイズに変更
	};

	enum class EventTriggerType : uint8
	{
		None,
		Click,
		RightClick,
		HoverStart,
		HoverEnd,
		PressStart,
		PressEnd,
		RightPressStart,
		RightPressEnd,
	};

	struct Event
	{
		EventTriggerType triggerType = EventTriggerType::None;
		String tag;
		std::weak_ptr<Node> sourceNode;
	};

	class Canvas : public INodeContainer, public std::enable_shared_from_this<Canvas>
	{
		friend class Node;

	private:
		class EventRegistry
		{
		private:
			Array<Event> m_events;

		public:
			EventRegistry() = default;

			void addEvent(const Event& event);

			void clear();

			[[nodiscard]]
			bool isEventFiredWithTag(StringView tag) const;

			[[nodiscard]]
			Optional<Event> getFiredEventWithTag(StringView tag) const;

			[[nodiscard]]
			Array<Event> getFiredEventsWithTag(StringView tag) const;

			[[nodiscard]]
			const Array<Event>& getFiredEventsAll() const;
		};

		SizeF m_referenceSize = DefaultSize;
		LayoutVariant m_childrenLayout = FlowLayout{};
		Array<std::shared_ptr<Node>> m_children;
		AutoFitMode m_autoFitMode = AutoFitMode::None;
		HashTable<String, ParamValue> m_params;
		String m_defaultFontAssetName = U"";
		/* NonSerialized */ SizeF m_size = DefaultSize;
		/* NonSerialized */ Vec2 m_position = Vec2::Zero();
		/* NonSerialized */ Vec2 m_scale = Vec2::One();
		/* NonSerialized */ double m_rotation = 0.0;
		/* NonSerialized */ EventRegistry m_eventRegistry;
		/* NonSerialized */ bool m_prevDragScrollingWithThresholdExceeded = false;
		/* NonSerialized */ Optional<SizeF> m_lastAutoFitSceneSize;
		/* NonSerialized */ bool m_isEditorPreview = false;
		/* NonSerialized */ int32 m_serializedVersion = CurrentSerializedVersion; // これは読み込んだバージョンで、シリアライズ時はこの変数の値ではなくCurrentSerializedVersionが固定で出力される
		/* NonSerialized */ bool m_isLayoutDirty = false; // レイアウト更新が必要かどうか
		/* NonSerialized */ InteractableYN m_interactable = InteractableYN::Yes;
		/* NonSerialized */ Mat3x2 m_parentTransformMat = Mat3x2::Identity(); // 親Transformの変換行列(SubCanvas用)
		/* NonSerialized */ Mat3x2 m_parentHitTestMat = Mat3x2::Identity(); // 親Transformのヒットテスト用変換行列(SubCanvas用)
		/* NonSerialized */ mutable Array<std::shared_ptr<Node>> m_tempChildrenBuffer; // 子ノードの一時バッファ(update内で別のCanvasのupdateが呼ばれる場合があるためthread_local staticにはできない。drawで呼ぶためmutableだが、drawはシングルスレッド前提なのでロック不要)

		[[nodiscard]]
		Mat3x2 rootPosScaleMat() const;

		void updateAutoFitIfNeeded(const SizeF& sceneSize, bool force = false);

		// ノードツリー内でinstanceIdによるノード検索（再帰）
		[[nodiscard]]
		std::shared_ptr<Node> findNodeByInstanceIdRecursive(const std::shared_ptr<Node>& node, uint64 instanceId) const;

		Canvas();

	public:
		~Canvas();

		/// @brief Canvasのデフォルトサイズ
		static constexpr SizeF DefaultSize{ 800, 600 };
		
		/// @brief Canvasを作成
		/// @param referenceSize 基準となるサイズ
		/// @return 作成されたCanvas
		[[nodiscard]]
		static std::shared_ptr<Canvas> Create(const SizeF& referenceSize = DefaultSize);
		
		/// @brief Canvasを作成
		/// @param width 基準となるサイズの幅
		/// @param height 基準となるサイズの高さ
		[[nodiscard]]
		static std::shared_ptr<Canvas> Create(double width, double height);

		/// @brief レイアウトを即座に更新
		/// @param onlyIfDirty レイアウト更新が必要な場合のみ更新するかどうか(Noを指定すると強制的に更新する。通常はYesのままで問題ない)
		void refreshLayoutImmediately(OnlyIfDirtyYN onlyIfDirty = OnlyIfDirtyYN::Yes);
		
		/// @brief レイアウト更新が必要であることをマークする
		/// @note マークされたノードは、フレームの最後にレイアウト更新が実行される
		void markLayoutAsDirty();

		/// @brief ノード名をもとに子ノードを検索
		/// @param name ノード名
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @return 最初に見つかったノードを返す。見つからなかった場合はnullptrを返す
		[[nodiscard]]
		std::shared_ptr<Node> findByName(StringView nodeName, RecursiveYN recursive = RecursiveYN::Yes, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No) const;

		/// @brief CanvasをJSON形式でシリアライズ
		/// @param withInstanceId 出力内容にインスタンスIDを含めるかどうか(NocoEditorの内部実装向けのため、通常は指定不要)
		/// @return JSON
		[[nodiscard]]
		JSON toJSON(detail::WithInstanceIdYN withInstanceId = detail::WithInstanceIdYN::No) const;

		/// @brief JSONからCanvasを作成
		/// @param json JSON
		/// @param withInstanceId 入力内容からインスタンスIDを読み込むどうか(NocoEditorの内部実装向けのため、通常は指定不要)
		/// @return 生成されたCanvas
		[[nodiscard]]
		static std::shared_ptr<Canvas> CreateFromJSON(const JSON& json, detail::WithInstanceIdYN withInstanceId = detail::WithInstanceIdYN::No);
		
		/// @brief JSONからCanvasを作成
		/// @param json JSON
		/// @param factory コンポーネントを生成するためのファクトリ
		/// @param withInstanceId 入力内容からインスタンスIDを読み込むどうか(NocoEditorの内部実装向けのため、通常は指定不要)
		/// @return 生成されたCanvas
		[[nodiscard]]
		static std::shared_ptr<Canvas> CreateFromJSON(const JSON& json, const ComponentFactory& factory, detail::WithInstanceIdYN withInstanceId = detail::WithInstanceIdYN::No);

		/// @brief ファイルからCanvasを読み込む
		/// @param path ファイルパス
		/// @param allowExceptions 例外を発生させるか
		/// @return 読み込まれたCanvas。AllowExceptions::Noで読み込みに失敗した場合はnullptrを返す
		/// @throws AllowExceptions::Yesで読み込みに失敗した場合は例外を送出する
		[[nodiscard]]
		static std::shared_ptr<Canvas> LoadFromFile(FilePathView path, AllowExceptions allowExceptions = AllowExceptions::No);

		/// @brief ファイルからCanvasを読み込む
		/// @param path ファイルパス
		/// @param factory コンポーネントを生成するためのファクトリ
		/// @param allowExceptions 例外を発生させるか
		/// @return 読み込まれたCanvas。AllowExceptions::Noで読み込みに失敗した場合はnullptrを返す
		/// @throws AllowExceptions::Yesで読み込みに失敗した場合は例外を送出する
		[[nodiscard]]
		static std::shared_ptr<Canvas> LoadFromFile(FilePathView path, const ComponentFactory& factory, AllowExceptions allowExceptions = AllowExceptions::No);

		/// @brief JSONからCanvasを読み込む
		/// @param json JSON
		/// @param withInstanceId 入力内容からインスタンスIDを読み込むどうか(NocoEditorの内部実装向けのため、通常は指定不要)
		/// @return 読み込みに成功した場合はtrue、失敗した場合はfalseを返す
		bool tryReadFromJSON(const JSON& json, detail::WithInstanceIdYN withInstanceId = detail::WithInstanceIdYN::No);
		
		/// @brief JSONからCanvasを読み込む
		/// @param json JSON
		/// @param factory コンポーネントを生成するためのファクトリ
		/// @param withInstanceId 入力内容からインスタンスIDを読み込むどうか(NocoEditorの内部実装向けのため、通常は指定不要)
		/// @return 読み込みに成功した場合はtrue、失敗した場合はfalseを返す
		bool tryReadFromJSON(const JSON& json, const ComponentFactory& factory, detail::WithInstanceIdYN withInstanceId);

		/// @brief 毎フレームの更新処理
		/// @param hitTestEnabled ヒットテストを有効にするかどうか
		void update(HitTestEnabledYN hitTestEnabled = HitTestEnabledYN::Yes);

		/// @brief 毎フレームの更新処理
		/// @param customSceneSize カスタムのシーンサイズ(ウィンドウサイズと異なるサイズで使用する場合に指定)
		/// @param hitTestEnabled ヒットテストを有効にするかどうか
		void update(const SizeF& customSceneSize, HitTestEnabledYN hitTestEnabled = HitTestEnabledYN::Yes);

		/// @brief 毎フレームの更新処理
		/// @param customSceneSize カスタムのシーンサイズ(ウィンドウサイズと異なるサイズで使用する場合に指定)
		/// @param parentTransformMat 親Transformの変換行列(SubCanvas用)
		/// @param parentHitTestMat 親Transformのヒットテスト用変換行列(SubCanvas用)
		/// @param hitTestEnabled ヒットテストを有効にするか
		void update(const SizeF& customSceneSize, const Mat3x2& parentTransformMat, const Mat3x2& parentHitTestMat, HitTestEnabledYN hitTestEnabled = HitTestEnabledYN::Yes);

		/// @brief 指定した座標にあるヒットテスト対象ノードを取得
		/// @param point 座標
		/// @param onlyScrollable スクロール可能なノードのみを対象とするかどうか
		/// @param usePrevZOrderInSiblings 兄弟ノードのZオーダーに前回のフレームの値を使用するかどうか(ライブラリの内部実装用のため、通常は指定不要)
		/// @return ヒットしたノードを返す。ヒットしているノードが存在しない場合はnullptrを返す
		[[nodiscard]]
		std::shared_ptr<Node> hitTest(const Vec2& point, OnlyScrollableYN onlyScrollable = OnlyScrollableYN::No, detail::UsePrevZOrderInSiblingsYN usePrevZOrderInSiblings = detail::UsePrevZOrderInSiblingsYN::No) const;

		/// @brief 毎フレームの描画処理
		void draw() const;

		/// @brief Canvasをクリア
		/// @note Canvasのすべての設定が初期化され、ノード、パラメータが削除される
		void clearAll();

		/// @brief Canvasの位置を設定
		/// @param position 位置
		/// @return Canvas自身(メソッドチェーン用)
		std::shared_ptr<Canvas> setPosition(const Vec2& position);

		/// @brief Canvasの位置を取得
		/// @return 位置
		[[nodiscard]]
		const Vec2& position() const
		{
			return m_position;
		}

		/// @brief Canvasの中心位置を設定
		/// @param center 中心位置
		/// @return Canvas自身(メソッドチェーン用)
		std::shared_ptr<Canvas> setCenter(const Vec2& center);

		/// @brief Canvasの中心位置を取得
		/// @return 中心位置
		[[nodiscard]]
		Vec2 center() const;

		/// @brief 基準サイズを取得
		/// @return 基準サイズ
		[[nodiscard]]
		const SizeF& referenceSize() const
		{
			return m_referenceSize;
		}
		
		/// @brief 基準サイズを設定
		/// @param size 基準サイズ
		/// @return Canvas自身(メソッドチェーン用)
		std::shared_ptr<Canvas> setReferenceSize(const SizeF& size);

		/// @brief 自動フィットモードを取得
		/// @return 自動フィットモード
		[[nodiscard]]
		AutoFitMode autoFitMode() const
		{
			return m_autoFitMode;
		}

		/// @brief 自動フィットモードを設定
		/// @param mode 自動フィットモード
		/// @return Canvas自身(メソッドチェーン用)
		std::shared_ptr<Canvas> setAutoFitMode(AutoFitMode mode);

		/// @brief エディタプレビューモードかどうかを設定(NocoEditorの内部実装向け)
		/// @param isEditorPreview エディタプレビューモードかどうか
		/// @return Canvas自身(メソッドチェーン用)
		std::shared_ptr<Canvas> setEditorPreviewInternal(bool isEditorPreview);

		/// @brief エディタプレビューモードかどうかを取得(NocoEditorの内部実装向け)
		/// @return エディタプレビューモードの場合はtrue、そうでない場合はfalse
		[[nodiscard]]
		bool isEditorPreviewInternal() const
		{
			return m_isEditorPreview;
		}

		/// @brief Canvasのスケールを設定
		/// @param scale スケール
		/// @return Canvas自身(メソッドチェーン用)
		std::shared_ptr<Canvas> setScale(const Vec2& scale);

		/// @brief Canvasのスケールを取得
		/// @return スケール
		[[nodiscard]]
		const Vec2& scale() const
		{
			return m_scale;
		}

		/// @brief Canvasの位置とスケールを設定
		/// @param position 位置
		/// @param scale スケール
		/// @return Canvas自身(メソッドチェーン用)
		std::shared_ptr<Canvas> setPositionScale(const Vec2& position, const Vec2& scale);

		/// @brief Canvasの回転角度を設定
		/// @param rotation 回転角度(度単位)
		/// @return Canvas自身(メソッドチェーン用)
		std::shared_ptr<Canvas> setRotation(double rotation);

		/// @brief Canvasの回転角度を取得
		/// @return 回転角度(度単位)
		[[nodiscard]]
		double rotation() const
		{
			return m_rotation;
		}

		/// @brief すべてのノードのスクロール位置をリセット
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		void resetScrollOffsetRecursive(IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No);

		/// @brief イベントを発火させる
		/// @param event 発火させるイベント
		/// @note EventTriggerなどイベントを発火させるコンポーネントから呼び出される
		void fireEvent(const Event& event);

		/// @brief 指定したタグのイベントが発火した瞬間かどうかを取得
		/// @param tag タグ
		/// @return 発火した場合はtrue、発火していない場合はfalseを返す
		[[nodiscard]]
		bool isEventFiredWithTag(StringView tag) const;

		/// @brief 指定したタグのイベントを取得
		/// @param tag タグ
		/// @return 発火したイベント。発火していない場合はnoneを返す
		[[nodiscard]]
		Optional<Event> getFiredEventWithTag(StringView tag) const;

		/// @brief 指定したタグのすべての発火したイベントを取得
		/// @param tag タグ
		/// @return 発火したイベントの配列
		[[nodiscard]]
		Array<Event> getFiredEventsWithTag(StringView tag) const;

		/// @brief すべての発火したイベントを取得
		/// @return 発火したイベントの配列
		[[nodiscard]]
		const Array<Event>& getFiredEventsAll() const;

		/// @brief パラメータのハッシュテーブルを取得
		/// @return パラメータのハッシュテーブル
		[[nodiscard]]
		const HashTable<String, ParamValue>& params() const
		{
			return m_params;
		}

		/// @brief パラメータのハッシュテーブルを取得
		/// @return パラメータのハッシュテーブル
		[[nodiscard]]
		HashTable<String, ParamValue>& params()
		{
			return m_params;
		}

		/// @brief パラメータの値を設定
		/// @tparam T パラメータの型(bool, int32, double, String, Color, Vec2, LRTBのいずれか)
		/// @param name パラメータ名(英数字とアンダースコアのみ使用可能。無効な名前の場合は無視される)
		/// @param value パラメータの値
		template<typename T>
		void setParamValue(const String& name, const T& value)
		{
			if (!IsValidParameterName(name))
			{
				Logger << U"[NocoUI warning] Invalid parameter name '{}' rejected. Parameter names must start with a letter or underscore and contain only letters, digits, and underscores."_fmt(name);
				return;
			}
			m_params[name] = MakeParamValue(value);
		}

		/// @brief パラメータの値を一括設定
		/// @param params パラメータ名と値のペアの初期化リスト
		void setParamValues(std::initializer_list<std::pair<String, std::variant<bool, int32, double, const char32_t*, String, Color, ColorF, Vec2, LRTB>>> params)
		{
			for (const auto& pair : params)
			{
				const auto& name = pair.first;
				const auto& value = pair.second;
				std::visit([this, &name](const auto& v) {
					if constexpr (std::is_same_v<std::decay_t<decltype(v)>, const char32_t*>)
					{
						setParamValue(name, String{ v });
					}
					else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, ColorF>)
					{
						setParamValue(name, Color{ v });  // ColorFをColorに変換
					}
					else
					{
						setParamValue(name, v);
					}
				}, value);
			}
		}

		/// @brief JSONからパラメータを一括設定
		/// @param json パラメータのJSON(キー:パラメータ名、値:パラメータ値 のオブジェクト形式)
		void setParamsByJSON(const JSON& json);

		/// @brief パラメータの値を取得
		/// @param name パラメータ名
		/// @return パラメータの値。存在しない場合はnoneを返す
		[[nodiscard]]
		Optional<ParamValue> paramValueOpt(const String& name) const;

		/// @brief パラメータの値を型指定で取得
		/// @tparam T パラメータの型
		/// @param name パラメータ名
		/// @return パラメータの値。存在しない場合はnoneを返す
		template <typename T>
		[[nodiscard]]
		Optional<T> paramValueAsOpt(const String& name) const
		{
			if (auto p = paramValueOpt(name))
			{
				return GetParamValueAs<T>(*p);
			}
			return none;
		}

		/// @brief パラメータ名が存在するかどうかを取得
		/// @param name パラメータ名
		/// @return パラメータ名が存在する場合はtrue、存在しない場合はfalseを返す
		[[nodiscard]]
		bool hasParam(const String& name) const;

		/// @brief パラメータ名が存在し、かつ指定した型であるかどうかを取得
		/// @param name パラメータ名
		/// @param paramType パラメータの型
		/// @return パラメータ名が存在して指定した型である場合はtrue、そうでない場合はfalseを返す
		[[nodiscard]]
		bool hasParamOfType(const String& name, ParamType paramType) const;

		/// @brief パラメータを削除
		/// @param name パラメータ名
		void removeParam(const String& name);

		/// @brief すべてのパラメータを削除
		void clearParams();

		/// @brief 指定したパラメータ名を参照している数を取得
		/// @param paramName パラメータ名
		/// @return 参照している数
		[[nodiscard]]
		size_t countParamRefs(StringView paramName) const;

		/// @brief 指定したパラメータ名の参照をすべて削除
		/// @param paramName パラメータ名
		void clearParamRefs(StringView paramName);

		/// @brief 存在しないパラメータや型が異なるパラメータを参照しているパラメータ参照をすべて削除
		/// @return 削除したパラメータ参照の名前の配列
		Array<String> removeInvalidParamRefs();

		/// @brief Canvas配下にパラメータ参照のあるパラメータ名を列挙
		/// @param pParamRefs 挿入先のハッシュセットのポインタ
		void populateParamRefs(HashSet<String>* pParamRefs) const;

		/// @brief サイズを設定
		/// @param width 幅
		/// @param height 高さ
		void setSize(double width, double height)
		{
			m_size = SizeF{ width, height };
			markLayoutAsDirty();
		}
		
		/// @brief サイズを設定
		/// @param size サイズ
		void setSize(const SizeF& size)
		{
			m_size = size;
			markLayoutAsDirty();
		}
		
		/// @brief 幅を設定
		/// @param width 幅
		void setWidth(double width)
		{
			m_size.x = width;
			markLayoutAsDirty();
		}
		
		/// @brief 高さを設定
		/// @param height 高さ
		void setHeight(double height)
		{
			m_size.y = height;
			markLayoutAsDirty();
		}

		/// @brief 幅を取得
		/// @return 幅
		[[nodiscard]]
		double width() const
		{
			return m_size.x;
		}

		/// @brief 高さを取得
		/// @return 高さ
		[[nodiscard]]
		double height() const
		{
			return m_size.y;
		}
		
		/// @brief サイズを取得
		/// @return サイズ
		[[nodiscard]]
		const SizeF& size() const
		{
			return m_size;
		}

		/// @brief 矩形を取得
		/// @return 矩形
		[[nodiscard]]
		Quad quad() const;

		/// @brief このCanvasがシリアライズされたバージョンを取得
		[[nodiscard]]
		int32 serializedVersion() const
		{
			return m_serializedVersion;
		}

		/// @brief 子レイアウト(InlineRegionを持つ子ノードの並べ方のルール)を取得
		/// @return 子レイアウト
		[[nodiscard]]
		const LayoutVariant& childrenLayout() const override
		{
			return m_childrenLayout;
		}

		/// @brief 子レイアウト(InlineRegionを持つ子ノードの並べ方のルール)を設定
		/// @param layout 子レイアウト
		/// @return Canvas自身(メソッドチェーンのため)
		std::shared_ptr<Canvas> setChildrenLayout(const LayoutVariant& layout);

		/// @brief このCanvasがインタラクション可能かどうかを取得
		/// @return インタラクション可能な場合はtrue、そうでない場合はfalse
		[[nodiscard]]
		bool interactable() const
		{
			return m_interactable.getBool();
		}

		/// @brief このCanvasがインタラクション可能かどうかを設定
		/// @param interactable インタラクション可能かどうか
		/// @return Canvas自身(メソッドチェーンのため)
		std::shared_ptr<Canvas> setInteractable(InteractableYN interactable);

		/// @brief このCanvasがインタラクション可能かどうかを設定
		/// @param interactable インタラクション可能かどうか
		/// @return Canvas自身(メソッドチェーンのため)
		std::shared_ptr<Canvas> setInteractable(bool interactable)
		{
			return setInteractable(InteractableYN{ interactable });
		}

		/// @brief FlowLayoutを取得
		/// @return FlowLayoutのポインタを返す。Canvasに設定された子レイアウトがFlowLayoutでない場合はnullptrを返す
		[[nodiscard]]
		const FlowLayout* childrenFlowLayout() const override
		{
			return std::get_if<FlowLayout>(&m_childrenLayout);
		}

		/// @brief HorizontalLayoutを取得
		/// @return HorizontalLayoutのポインタを返す。Canvasに設定された子レイアウトがHorizontalLayoutでない場合はnullptrを返す
		[[nodiscard]]
		const HorizontalLayout* childrenHorizontalLayout() const override
		{
			return std::get_if<HorizontalLayout>(&m_childrenLayout);
		}

		/// @brief VerticalLayoutを取得
		/// @return VerticalLayoutのポインタを返す。Canvasに設定された子レイアウトがVerticalLayoutでない場合はnullptrを返す
		[[nodiscard]]
		const VerticalLayout* childrenVerticalLayout() const override
		{
			return std::get_if<VerticalLayout>(&m_childrenLayout);
		}

		/// @brief 子ノードの配列を取得
		/// @return 子ノードの配列
		[[nodiscard]]
		const Array<std::shared_ptr<Node>>& children() const override
		{
			return m_children;
		}

		/// @brief 子ノードの数を取得
		/// @return 子ノードの数
		[[nodiscard]]
		size_t childCount() const override
		{
			return m_children.size();
		}

		/// @brief 指定したインデックスの子ノードを取得
		/// @param index 子ノードのインデックス
		/// @return 子ノード。インデックスが範囲外の場合はnullptrを返す
		[[nodiscard]]
		std::shared_ptr<Node> childAt(size_t index) const override;

		/// @brief 子ノードを追加
		/// @param child 追加する子ノード
		/// @return 追加されたノード
		const std::shared_ptr<Node>& addChild(const std::shared_ptr<Node>& node) override;

		/// @brief 子ノードを削除
		void removeChild(const std::shared_ptr<Node>& node) override;
		
		/// @brief すべての子ノードを削除
		void removeChildrenAll() override;

		/// @brief 子ノードを指定したインデックスに追加
		/// @param child 追加する子ノード
		/// @param index 追加先のインデックス
		/// @return 追加されたノード
		/// @note indexが範囲外の場合は末尾に追加される
		const std::shared_ptr<Node>& addChildAtIndex(const std::shared_ptr<Node>& child, size_t index) override;

		/// @brief 指定した子ノード同士を入れ替える
		/// @param child1 子ノード1
		/// @param child2 子ノード2
		void swapChildren(const std::shared_ptr<Node>& child1, const std::shared_ptr<Node>& child2);

		/// @brief 指定したインデックスの子ノード同士を入れ替える
		/// @param index1 子ノード1のインデックス
		/// @param index2 子ノード2のインデックス
		void swapChildren(size_t index1, size_t index2) override;

		/// @brief 子ノードとして持っているかどうかを取得
		/// @param child 判定対象の子ノード
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @return 子ノードとして持っている場合はtrue、そうでなければfalseを返す
		[[nodiscard]]
		bool containsChild(
			const std::shared_ptr<Node>& child,
			RecursiveYN recursive = RecursiveYN::No,
			IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No) const override;

		/// @brief ノード名をもとに子ノードを検索
		/// @param name ノード名
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @return 最初に見つかったノードを返す。見つからなかった場合はnullptrを返す
		[[nodiscard]]
		std::shared_ptr<Node> findByName(
			StringView name,
			RecursiveYN recursive = RecursiveYN::Yes,
			IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No) override;

		/// @brief 指定した子ノードのインデックスを取得する
		/// @param child 子ノード
		/// @return 子ノードのインデックス
		/// @throws Error 見つからない場合は例外を送出する
		[[nodiscard]]
		size_t indexOfChild(const std::shared_ptr<Node>& child) const;

		/// @brief 指定した子ノードのインデックスを取得する(Optional版)
		/// @param child 子ノード
		/// @return 子ノードのインデックス。見つからない場合はnoneを返す
		[[nodiscard]]
		Optional<size_t> indexOfChildOpt(
			const std::shared_ptr<Node>& child) const override;
		
		/// @brief 子ノードを構築して追加
		/// @param name ノード名
		/// @param region リージョン(ノードの位置やサイズ)
		/// @param isHitTarget ヒットテストの対象にするかどうか
		/// @param inheritChildrenStateFlags 子ノードのインタラクションステートを受け継ぐかどうかのビットフラグ
		/// @return 追加されたノード
		const std::shared_ptr<Node>& emplaceChild(
			StringView name = U"Node",
			const RegionVariant& region = InlineRegion{},
			IsHitTargetYN isHitTarget = IsHitTargetYN::Yes,
			InheritChildrenStateFlags inheritChildrenStateFlags = InheritChildrenStateFlags::None) override;

		/// @brief JSONから子ノードを追加
		/// @param json JSON
		/// @return 追加されたノード
		const std::shared_ptr<Node>& addChildFromJSON(const JSON& json) override;

		/// @brief JSONから子ノードを追加
		/// @param json JSON
		/// @param factory コンポーネントを生成するためのファクトリ
		/// @return 追加されたノード
		const std::shared_ptr<Node>& addChildFromJSON(const JSON& json, const ComponentFactory& factory) override;

		/// @brief SubCanvasを持つノードを子として追加
		/// @param canvasPath SubCanvasファイルのパス
		/// @param params SubCanvasに設定するパラメータ
		/// @return 追加されたノード
		/// @note 作成されるノードはInlineRegionでSubCanvasのreferenceSizeと同じサイズになる
		const std::shared_ptr<Node>& addSubCanvasNodeAsChild(
			StringView canvasPath,
			std::initializer_list<std::pair<String, std::variant<bool, int32, double, const char32_t*, String, Color, ColorF, Vec2, LRTB>>> params = {});

		/// @brief パラメータ参照名を置換
		/// @param oldName 置換対象のパラメータ参照名
		/// @param newName 新しいパラメータ参照名
		void replaceParamRefs(const String& oldName, const String& newName);

		/// @brief 現在のフレームのプロパティ値の上書き状態をクリア
		/// @param recursive 子孫ノードも対象とするかどうか
		void clearCurrentFrameOverride();

		/// @brief インスタンスIDによるノード検索(NocoEditorの内部実装向け)
		/// @param instanceId インスタンスID
		/// @return 見つかったノードを返す。見つからなかった場合はnullptrを返す
		[[nodiscard]]
		std::shared_ptr<Node> findNodeByInstanceId(uint64 instanceId) const;

		/// @brief 指定したタグを持つSubCanvasを取得
		/// @param tag タグ
		/// @param recursive 子孫ノードも対象とするかどうか
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @return SubCanvas。見つからない場合はnullptrを返す
		[[nodiscard]]
		std::shared_ptr<class SubCanvas> getSubCanvasByTag(StringView tag, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No) const;

		/// @brief 指定したタグを持つSubCanvasのパラメータ値を設定
		/// @param tag タグ
		/// @param paramName パラメータ名
		/// @param value パラメータ値
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		void setSubCanvasParamValueByTag(StringView tag, const String& paramName, const ParamValue& value, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No);

		/// @brief 指定したタグを持つSubCanvasのパラメータ値を一括設定
		/// @param tag タグ
		/// @param params パラメータ名と値のペアの初期化リスト
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		void setSubCanvasParamValuesByTag(StringView tag, std::initializer_list<std::pair<String, std::variant<bool, int32, double, const char32_t*, String, Color, ColorF, Vec2, LRTB>>> params, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No);

		/// @brief 指定したタグを持つTweenコンポーネントのアクティブ状態を一括設定
		/// @param active アクティブ状態
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		void setTweenActiveAll(bool active, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No);

		/// @brief 指定したタグを持つTweenコンポーネントのアクティブ状態を一括設定
		/// @param tag タグ
		/// @param active アクティブ状態
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		void setTweenActiveByTag(StringView tag, bool active, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No);

		/// @brief 指定したタグを持つTweenコンポーネントが再生中かどうかを取得
		/// @param tag タグ
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @return Tweenコンポーネントが再生中の場合はtrue、そうでなければfalseを返す
		/// @note 指定したタグを持つTweenコンポーネントが複数ある場合、いずれかが再生中であればtrueを返す。指定したタグを持つTweenコンポーネントが見つからない場合はfalseを返す
		[[nodiscard]]
		bool isTweenPlayingByTag(StringView tag, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No) const;

		/// @brief 指定したタグを持つTextBox/TextAreaのテキストを取得
		/// @param tag タグ
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @return TextBox/TextAreaのテキスト。見つからない場合は空文字列を返す
		[[nodiscard]]
		String getTextValueByTag(StringView tag, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No) const;

		/// @brief 指定したタグを持つTextBox/TextAreaのテキストを取得(Optional版)
		/// @param tag タグ
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @return TextBox/TextAreaのテキスト。見つからない場合はnoneを返す
		[[nodiscard]]
		Optional<String> getTextValueByTagOpt(StringView tag, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No) const;

		/// @brief 指定したタグを持つTextBox/TextAreaのテキストを設定
		/// @param tag タグ
		/// @param text テキスト
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @note 該当するすべてのTextBox/TextAreaに設定される
		void setTextValueByTag(StringView tag, StringView text, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No);

		/// @brief 指定したタグを持つToggleコンポーネントの値を取得
		/// @param tag タグ
		/// @param defaultValue 見つからなかった場合のデフォルト値
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @return Toggleコンポーネントの値
		[[nodiscard]]
		bool getToggleValueByTag(StringView tag, bool defaultValue = false, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No) const;

		/// @brief 指定したタグを持つToggleコンポーネントの値を取得(Optional版)
		/// @param tag タグ
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		/// @return Toggleコンポーネントの値。見つからなかった場合はnoneを返す
		[[nodiscard]]
		Optional<bool> getToggleValueByTagOpt(StringView tag, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No) const;

		/// @brief 指定したタグを持つToggleコンポーネントの値を設定
		/// @param tag タグ
		/// @param value 値
		/// @param includeSubCanvas SubCanvas配下のノードも対象とするかどうか
		void setToggleValueByTag(StringView tag, bool value, IncludeSubCanvasYN includeSubCanvas = IncludeSubCanvasYN::No);

		/// @brief デフォルトのフォントアセット名を取得
		/// @return デフォルトのフォントアセット名
		[[nodiscard]]
		const String& defaultFontAssetName() const
		{
			return m_defaultFontAssetName;
		}

		/// @brief デフォルトのフォントアセット名を設定
		/// @param fontAssetName デフォルトのフォントアセット名
		/// @return Canvas自身(メソッドチェーン用)
		std::shared_ptr<Canvas> setDefaultFontAssetName(StringView fontAssetName)
		{
			m_defaultFontAssetName = fontAssetName;
			clearFontCache();
			return shared_from_this();
		}

		/// @brief フォントキャッシュをクリア
		void clearFontCache();
	};
}
