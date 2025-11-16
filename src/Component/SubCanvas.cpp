#include "NocoUI/Component/SubCanvas.hpp"
#include "NocoUI/Node.hpp"
#include "NocoUI/Canvas.hpp"
#include "NocoUI/Asset.hpp"

namespace noco
{
	void SubCanvas::loadCanvasInternal(const std::shared_ptr<Node>& node)
	{
		const String& path = m_canvasPath.value();

		// 空のパスの場合は何もしない
		if (path.isEmpty())
		{
			m_canvas.reset();
			m_loadedPath.clear();
			return;
		}

		// 絶対パスの場合は無視
		if (path.starts_with(U'/') || path.starts_with(U'\\') || path.contains(U':'))
		{
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
			m_canvas.reset();
			m_loadedPath.clear();
			return;
		}

		// Canvasを読み込み
		auto canvas = Canvas::LoadFromFile(fullPath);
		if (!canvas)
		{
			m_canvas.reset();
			m_loadedPath = path;
			return;
		}

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
