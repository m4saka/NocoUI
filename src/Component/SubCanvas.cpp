#include "NocoUI/Component/SubCanvas.hpp"
#include "NocoUI/Node.hpp"
#include "NocoUI/Canvas.hpp"
#include "NocoUI/Asset.hpp"

namespace noco
{
	SubCanvas::~SubCanvas()
	{
		if (m_canvas)
		{
			m_canvas->setContainedSubCanvas({});
		}
	}

	void SubCanvas::loadCanvasInternal(const std::shared_ptr<Node>& node)
	{
		const String& path = m_canvasPath.value();

		// 空のパスの場合は何もしない
		if (path.isEmpty())
		{
			if (m_canvas)
			{
				m_canvas->setContainedSubCanvas({});
			}
			m_canvas.reset();
			m_loadedPath.clear();
			return;
		}

		// 絶対パスの場合は無視
		if (path.starts_with(U'/') || path.starts_with(U'\\') || path.contains(U':'))
		{
			if (m_canvas)
			{
				m_canvas->setContainedSubCanvas({});
			}
			m_canvas.reset();
			m_loadedPath.clear();
			return;
		}

		// 既に同じパスが処理済みの場合はスキップ
		if (m_loadedPath == path)
		{
			return;
		}

		// ファイルが存在しない場合はスキップ
		const FilePath fullPath = FileSystem::PathAppend(noco::Asset::GetBaseDirectoryPath(), path);
		if (!FileSystem::Exists(fullPath))
		{
			if (m_canvas)
			{
				m_canvas->setContainedSubCanvas({});
			}
			m_canvas.reset();
			m_loadedPath.clear();
			return;
		}

		// 親方向に辿って循環参照を確認
		const FilePath normalizedFullPath = FileSystem::FullPath(fullPath);
		HashSet<FilePath> loadingPaths;
		loadingPaths.insert(normalizedFullPath);

		auto currentNode = node;
		constexpr int32 kMaxNestLevel = 10;
		int32 nestLevel = 0;

		while (currentNode)
		{
			if (++nestLevel > kMaxNestLevel)
			{
				Logger << U"[NocoUI error] SubCanvas: Max nest level exceeded: " << path;
				if (m_canvas)
				{
					m_canvas->setContainedSubCanvas({});
				}
				m_canvas.reset();
				m_loadedPath.clear();
				return;
			}

			if (auto parentCanvas = currentNode->containedCanvas())
			{
				if (auto parentSubCanvas = parentCanvas->containedSubCanvas().lock())
				{
					const String& parentPath = parentSubCanvas->loadedPath();
					if (!parentPath.isEmpty())
					{
						const FilePath parentFullPath = FileSystem::FullPath(
							FileSystem::PathAppend(noco::Asset::GetBaseDirectoryPath(), parentPath));

						if (loadingPaths.contains(parentFullPath))
						{
							Logger << U"[NocoUI error] SubCanvas: Circular reference detected: " << path;
							if (m_canvas)
							{
								m_canvas->setContainedSubCanvas({});
							}
							m_canvas.reset();
							m_loadedPath.clear();
							return;
						}
						loadingPaths.insert(parentFullPath);
					}
				}
			}

			currentNode = currentNode->parentNode();
		}

		// 古いCanvasの参照をクリア
		if (m_canvas)
		{
			m_canvas->setContainedSubCanvas({});
		}

		// Canvasを読み込み
		auto canvas = Canvas::LoadFromFile(fullPath);
		if (!canvas)
		{
			m_canvas.reset();
			m_loadedPath = path;
			return;
		}

		// このCanvasを含むSubCanvasとして自身を設定
		canvas->setContainedSubCanvas(weak_from_this());

		m_canvas = canvas;
		m_loadedPath = path;
	}

	void SubCanvas::update(const std::shared_ptr<Node>& node)
	{
		// パスが変更されていたら再読み込み
		if (m_loadedPath != m_canvasPath.value())
		{
			loadCanvasInternal(node);
		}

		// Canvasを更新
		if (m_canvas)
		{
			// paramsJSONが変更されていたらパースして適用
			const String& currentParamsJSON = m_paramsJSON.value();
			if (m_appliedParamsJSON != currentParamsJSON)
			{
				if (!currentParamsJSON.isEmpty() && currentParamsJSON != U"{}")
				{
					const JSON json = JSON::Parse(currentParamsJSON);
					if (json.isObject())
					{
						m_canvas->setParamsByJSON(json);
					}
				}
				m_appliedParamsJSON = currentParamsJSON;
			}

			m_canvas->update(node->regionRect().size);

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

	void SubCanvas::reloadCanvasFile(const std::shared_ptr<Node>& node)
	{
		m_loadedPath.clear();
		loadCanvasInternal(node);
	}
}
