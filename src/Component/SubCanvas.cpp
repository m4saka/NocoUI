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

		/// @brief 紐付けJSON文字列をオブジェクトJSONにパース(失敗時は空オブジェクト)
		[[nodiscard]]
		JSON ParseBindingsJSON(const String& bindingsJSONString)
		{
			if (bindingsJSONString.isEmpty() || bindingsJSONString == U"{}")
			{
				return JSON{};
			}
			const JSON json = JSON::Parse(bindingsJSONString);
			if (!json.isObject())
			{
				return JSON{};
			}
			return json;
		}
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

			// serializedParamBindingsJSONが変更されていたらパースしてキャッシュを更新
			const String& currentParamBindingsJSON = m_serializedParamBindingsJSON.value();
			if (m_appliedSerializedParamBindingsJSON != currentParamBindingsJSON)
			{
				m_paramBindings.clear();
				if (!currentParamBindingsJSON.isEmpty() && currentParamBindingsJSON != U"{}")
				{
					const JSON json = JSON::Parse(currentParamBindingsJSON);
					if (json.isObject())
					{
						for (const auto& [subCanvasParamName, parentParamNameJSON] : json)
						{
							if (parentParamNameJSON.isString())
							{
								m_paramBindings[subCanvasParamName] = parentParamNameJSON.getString();
							}
						}
					}
				}
				m_appliedSerializedParamBindingsJSON = currentParamBindingsJSON;
			}

			// serializedParamBindingsJSONに従って親Canvasのパラメータを子Canvasに毎フレーム適用
			if (!m_paramBindings.empty())
			{
				if (auto parentCanvas = node->containedCanvas())
				{
					const auto& parentParams = parentCanvas->params();
					const auto& subCanvasParams = m_canvas->params();
					for (const auto& [subCanvasParamName, parentParamName] : m_paramBindings)
					{
						const auto parentIt = parentParams.find(parentParamName);
						if (parentIt == parentParams.end())
						{
							continue;
						}
						const auto subCanvasIt = subCanvasParams.find(subCanvasParamName);
						if (subCanvasIt == subCanvasParams.end())
						{
							continue;
						}
						// 型不一致の場合は子paramの型を壊さないようスキップ
						if (GetParamType(parentIt->second) != GetParamType(subCanvasIt->second))
						{
							continue;
						}
						m_canvas->setParamValue(subCanvasParamName, parentIt->second);
					}
				}
			}

			// 親のinteractableを子Canvasに伝播
			m_canvas->setInteractable(node->interactable());

			// 親ノードの変換行列を子Canvasに伝播
			// transformMatInHierarchyにはregionRect.posが含まれていないため、別途加算する
			// hitTestはコンポーネントのhitTestで別途実行されるためNoを指定
			const Vec2 centerOffset = (node->regionRect().size - m_canvas->size() * m_canvas->scale()) / 2.0;
			const Mat3x2 posTranslate = Mat3x2::Translate(node->regionRect().pos + centerOffset);
			m_canvas->update(
				node->regionRect().size,
				posTranslate * node->transformMatInHierarchy(),
				posTranslate * node->hitTestMatInHierarchy(),
				HitTestEnabledYN::No);

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

	void SubCanvas::draw(const Node&) const
	{
		if (m_canvas)
		{
			const Transformer2D transform{ Mat3x2::Identity(), Transformer2D::Target::SetLocal };
			m_canvas->draw();
		}
	}

	std::shared_ptr<Node> SubCanvas::hitTest(
		const std::shared_ptr<Node>&,
		const Vec2& point,
		OnlyScrollableYN onlyScrollable,
		detail::UsePrevZOrderInSiblingsYN usePrevZOrderInSiblings) const
	{
		if (m_canvas)
		{
			return m_canvas->hitTest(point, onlyScrollable, usePrevZOrderInSiblings);
		}
		return nullptr;
	}

	const Array<std::shared_ptr<Node>>& SubCanvas::subCanvasChildren() const
	{
		if (m_canvas)
		{
			return m_canvas->children();
		}
		static const Array<std::shared_ptr<Node>> empty;
		return empty;
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

	size_t SubCanvas::countBindingParamRefs(StringView paramName) const
	{
		if (paramName.isEmpty())
		{
			return 0;
		}
		const JSON json = ParseBindingsJSON(m_serializedParamBindingsJSON.value());
		if (json.isEmpty())
		{
			return 0;
		}
		size_t count = 0;
		for (const auto& [subCanvasParamName, parentParamNameJSON] : json)
		{
			if (parentParamNameJSON.isString() && parentParamNameJSON.getString() == paramName)
			{
				count++;
			}
		}
		return count;
	}

	void SubCanvas::clearBindingParamRefs(StringView paramName)
	{
		if (paramName.isEmpty())
		{
			return;
		}
		const JSON json = ParseBindingsJSON(m_serializedParamBindingsJSON.value());
		if (json.isEmpty())
		{
			return;
		}
		JSON newJSON = JSON::Parse(U"{}");
		bool changed = false;
		for (const auto& [subCanvasParamName, parentParamNameJSON] : json)
		{
			if (parentParamNameJSON.isString() && parentParamNameJSON.getString() == paramName)
			{
				changed = true;
				continue;
			}
			newJSON[subCanvasParamName] = parentParamNameJSON;
		}
		if (changed)
		{
			setSerializedParamBindingsJSON(newJSON.formatMinimum());
		}
	}

	void SubCanvas::replaceBindingParamRefs(StringView oldName, StringView newName)
	{
		if (oldName.isEmpty())
		{
			return;
		}
		const JSON json = ParseBindingsJSON(m_serializedParamBindingsJSON.value());
		if (json.isEmpty())
		{
			return;
		}
		JSON newJSON = JSON::Parse(U"{}");
		bool changed = false;
		for (const auto& [subCanvasParamName, parentParamNameJSON] : json)
		{
			if (parentParamNameJSON.isString() && parentParamNameJSON.getString() == oldName)
			{
				newJSON[subCanvasParamName] = String{ newName };
				changed = true;
			}
			else
			{
				newJSON[subCanvasParamName] = parentParamNameJSON;
			}
		}
		if (changed)
		{
			setSerializedParamBindingsJSON(newJSON.formatMinimum());
		}
	}

	Array<String> SubCanvas::removeInvalidBindingParamRefs(const HashTable<String, ParamValue>& validParams)
	{
		const JSON json = ParseBindingsJSON(m_serializedParamBindingsJSON.value());
		if (json.isEmpty())
		{
			return {};
		}
		HashSet<String> clearedParamsSet;
		JSON newJSON = JSON::Parse(U"{}");
		bool changed = false;
		for (const auto& [subCanvasParamName, parentParamNameJSON] : json)
		{
			if (!parentParamNameJSON.isString())
			{
				changed = true;
				continue;
			}
			const String parentParamName = parentParamNameJSON.getString();
			if (!validParams.contains(parentParamName))
			{
				clearedParamsSet.insert(parentParamName);
				changed = true;
				continue;
			}
			newJSON[subCanvasParamName] = parentParamNameJSON;
		}
		if (changed)
		{
			setSerializedParamBindingsJSON(newJSON.formatMinimum());
		}
		return Array<String>(clearedParamsSet.begin(), clearedParamsSet.end());
	}

	void SubCanvas::populateBindingParamRefs(HashSet<String>* pParamRefs) const
	{
		if (pParamRefs == nullptr)
		{
			return;
		}
		const JSON json = ParseBindingsJSON(m_serializedParamBindingsJSON.value());
		if (json.isEmpty())
		{
			return;
		}
		for (const auto& [subCanvasParamName, parentParamNameJSON] : json)
		{
			if (parentParamNameJSON.isString())
			{
				const String parentParamName = parentParamNameJSON.getString();
				if (!parentParamName.isEmpty())
				{
					pParamRefs->insert(parentParamName);
				}
			}
		}
	}

	Array<String> SubCanvas::getSubCanvasParamsBoundTo(StringView parentParamName) const
	{
		if (parentParamName.isEmpty())
		{
			return {};
		}
		const JSON json = ParseBindingsJSON(m_serializedParamBindingsJSON.value());
		if (json.isEmpty())
		{
			return {};
		}
		Array<String> result;
		for (const auto& [subCanvasParamName, parentParamNameJSON] : json)
		{
			if (parentParamNameJSON.isString() && parentParamNameJSON.getString() == parentParamName)
			{
				result.push_back(subCanvasParamName);
			}
		}
		return result;
	}

	void SubCanvas::replaceAdditionalParamRefsInternal(StringView oldName, StringView newName)
	{
		replaceBindingParamRefs(oldName, newName);
	}
}
