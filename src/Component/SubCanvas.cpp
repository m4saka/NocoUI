#include "NocoUI/Component/SubCanvas.hpp"
#include "NocoUI/Node.hpp"
#include "NocoUI/Canvas.hpp"
#include "NocoUI/Asset.hpp"

namespace noco
{
	namespace
	{
		thread_local Array<FilePath> t_loadingPaths;

		class LoadingPathGuard
		{
		private:
			bool m_pushed;

		public:
			LoadingPathGuard(const FilePath& path)
				: m_pushed(true)
			{
				t_loadingPaths.push_back(path);
			}

			~LoadingPathGuard()
			{
				if (m_pushed)
				{
					t_loadingPaths.pop_back();
				}
			}

			LoadingPathGuard(const LoadingPathGuard&) = delete;
			LoadingPathGuard& operator=(const LoadingPathGuard&) = delete;
		};
	}

	void SubCanvas::loadCanvasInternal()
	{
		const String& path = m_canvasPath.value();

		// 空のパスの場合は何もしない
		if (path.isEmpty())
		{
			m_canvas.reset();
			m_loadedPath.clear();
			m_loadedAssetBasePath.clear();
			return;
		}

		// 既に同じパスが処理済みの場合はスキップ
		const FilePath& currentBasePath = noco::Asset::GetBaseDirectoryPath();
		if (m_loadedPath == path && m_loadedAssetBasePath == currentBasePath)
		{
			return;
		}

		// 絶対パス/相対パスをフルパスに変換
		const FilePath fullPath = noco::Asset::GetFullPath(path);

		// ファイルが存在しない場合はスキップ
		if (!FileSystem::Exists(fullPath))
		{
			m_canvas.reset();
			m_loadedPath.clear();
			m_loadedAssetBasePath.clear();
			return;
		}

		// グローバルスタックを使用して循環参照とネストレベルを確認
		const FilePath normalizedFullPath = FileSystem::FullPath(fullPath);

		// 最大ネストレベルチェック
		constexpr int32 kMaxNestLevel = 10;
		if (t_loadingPaths.size() >= kMaxNestLevel)
		{
			Logger << U"[NocoUI error] SubCanvas load aborted due to exceeding maximum nest level ({}): {}"_fmt(kMaxNestLevel, path);
			m_canvas.reset();
			m_loadedPath = path; // 再読み込みさせないため、既に読み込み済み扱いとする
			m_loadedAssetBasePath = currentBasePath;
			return;
		}

		// 循環参照チェック
		if (t_loadingPaths.contains(normalizedFullPath))
		{
			Logger << U"[NocoUI error] SubCanvas load aborted due to circular reference detected: {}"_fmt(path);
			m_canvas.reset();
			m_loadedPath = path; // 再読み込みさせないため、既に読み込み済み扱いとする
			m_loadedAssetBasePath = currentBasePath;
			return;
		}

		const LoadingPathGuard guard{ normalizedFullPath };

		const JSON& json = noco::Asset::GetOrLoadJSON(path);
		if (!json)
		{
			m_canvas.reset();
			m_loadedPath = path;
			m_loadedAssetBasePath = currentBasePath;
			return;
		}

		auto canvas = Canvas::CreateFromJSON(json);
		if (!canvas)
		{
			m_canvas.reset();
			m_loadedPath = path;
			m_loadedAssetBasePath = currentBasePath;
			return;
		}

		m_canvas = canvas;
		m_loadedPath = path;
		m_loadedAssetBasePath = currentBasePath;
	}

	void SubCanvas::update(const std::shared_ptr<Node>& node)
	{
		// パスが変更されていたら再読み込み
		if (m_loadedPath != m_canvasPath.value() || m_loadedAssetBasePath != noco::Asset::GetBaseDirectoryPath())
		{
			loadCanvasInternal();
		}

		// Canvasを更新
		if (m_canvas)
		{
			// serializedParamsJSONが変更されていたらパースして適用
			const String& currentSerializedParamsJSON = m_serializedParamsJSON.value();
			if (m_appliedSerializedParamsJSON != currentSerializedParamsJSON)
			{
				if (!currentSerializedParamsJSON.isEmpty() && currentSerializedParamsJSON != U"{}")
				{
					const JSON json = JSON::Parse(currentSerializedParamsJSON);
					if (json.isObject())
					{
						m_canvas->setParamsByJSON(json);
					}
				}
				m_appliedSerializedParamsJSON = currentSerializedParamsJSON;
			}

			// 親のinteractableを子Canvasに伝播
			m_canvas->setInteractable(node->interactable());

			// Canvas::updateに指定されたhitTestEnabledを子Canvasに伝播
			HitTestEnabledYN hitTestEnabled = HitTestEnabledYN::Yes;
			if (auto parentCanvas = node->containedCanvas())
			{
				if (auto enabled = parentCanvas->hitTestEnabledForCurrentUpdate())
				{
					hitTestEnabled = *enabled;
				}
			}

			m_canvas->update(node->regionRect().size, hitTestEnabled);

			// イベント伝播が有効な場合、子Canvasのイベントを親Canvasに伝播
			if (m_propagateEvents.value())
			{
				const auto& events = m_canvas->getFiredEventsAll();
				if (!events.isEmpty())
				{
					if (auto parentNode = node->parentNode())
					{
						if (auto parentCanvas = parentNode->containedCanvas())
						{
							for (const auto& event : events)
							{
								parentCanvas->fireEvent(event);
							}
						}
					}
				}
			}
		}
	}

	void SubCanvas::draw(const Node& node) const
	{
		if (m_canvas)
		{
			const Transformer2D transformer{ Mat3x2::Translate(node.regionRect().pos) };
			m_canvas->draw();
		}
	}

	void SubCanvas::reloadCanvasFile()
	{
		if (!m_canvasPath.value().isEmpty())
		{
			noco::Asset::ReloadJSON(m_canvasPath.value());
		}
		m_loadedPath.clear();
		loadCanvasInternal();
	}
}
