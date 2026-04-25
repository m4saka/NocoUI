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

		/// @brief 反映モードJSONから、紐付けに残っていないキーのエントリを取り除く
		/// @return 変更があった場合の最小化JSON文字列、なければnone
		[[nodiscard]]
		Optional<String> RemoveOrphanedParamBindingModes(const String& modesJSONString, const HashSet<String>& keepKeys)
		{
			const JSON modesJSON = ParseBindingsJSON(modesJSONString);
			if (modesJSON.isEmpty())
			{
				return none;
			}
			JSON newJSON = JSON::Parse(U"{}");
			bool changed = false;
			for (const auto& [key, value] : modesJSON)
			{
				if (keepKeys.contains(key))
				{
					newJSON[key] = value;
				}
				else
				{
					changed = true;
				}
			}
			if (!changed)
			{
				return none;
			}
			return newJSON.formatMinimum();
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

		// Canvas読み込み後はパラメータ再反映が必要なのでクリア
		m_appliedSerializedParamsJSON.clear();
		m_appliedSerializedParamBindingsJSON.clear();
		m_appliedSerializedParamBindingModesJSON.clear();
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
			const bool paramOverrideCacheChanged = (m_appliedSerializedParamsJSON != currentSerializedParamsJSON);
			if (paramOverrideCacheChanged)
			{
				m_paramOverrideCache.clear();
				if (!currentSerializedParamsJSON.isEmpty() && currentSerializedParamsJSON != U"{}")
				{
					const JSON json = JSON::Parse(currentSerializedParamsJSON);
					if (json.isObject())
					{
						m_canvas->setParamsByJSON(json);

						// 元となる上書き値が必要なモードで使う静的な値キャッシュを構築
						const auto& subCanvasParams = m_canvas->params();
						for (const auto& [key, valueJSON] : json)
						{
							const auto it = subCanvasParams.find(key);
							if (it == subCanvasParams.end())
							{
								continue;
							}
							const ParamType type = GetParamType(it->second);
							if (auto pv = ParamValueFromJSONValue(valueJSON, type))
							{
								m_paramOverrideCache[key] = *pv;
							}
						}
					}
				}
				m_appliedSerializedParamsJSON = currentSerializedParamsJSON;
			}

			// serializedParamBindingsJSONが変更されていたらパースしてキャッシュを更新
			const String& currentParamBindingsJSON = m_serializedParamBindingsJSON.value();
			if (m_appliedSerializedParamBindingsJSON != currentParamBindingsJSON)
			{
				m_paramBindingMappingCache.clear();
				if (!currentParamBindingsJSON.isEmpty() && currentParamBindingsJSON != U"{}")
				{
					const JSON json = JSON::Parse(currentParamBindingsJSON);
					if (json.isObject())
					{
						for (const auto& [subCanvasParamName, parentParamNameJSON] : json)
						{
							if (parentParamNameJSON.isString())
							{
								m_paramBindingMappingCache[subCanvasParamName] = parentParamNameJSON.getString();
							}
						}
					}
				}
				m_appliedSerializedParamBindingsJSON = currentParamBindingsJSON;
			}

			// serializedParamBindingModesJSONまたは上書き値キャッシュが変わった場合はモードキャッシュを再構築
			const String& currentParamBindingModesJSON = m_serializedParamBindingModesJSON.value();
			if (m_appliedSerializedParamBindingModesJSON != currentParamBindingModesJSON || paramOverrideCacheChanged)
			{
				m_paramBindingModeCache.clear();
				if (!currentParamBindingModesJSON.isEmpty() && currentParamBindingModesJSON != U"{}")
				{
					const JSON json = JSON::Parse(currentParamBindingModesJSON);
					if (json.isObject())
					{
						const auto& subCanvasParams = m_canvas->params();
						for (const auto& [subCanvasParamName, modeJSON] : json)
						{
							if (!modeJSON.isString())
							{
								continue;
							}
							const auto opt = StringToEnumOpt<ParamRefMode>(modeJSON.getString());
							if (!opt)
							{
								Logger << U"[NocoUI warning] Unknown ParamRefMode '{}' for SubCanvas binding '{}'"_fmt(modeJSON.getString(), subCanvasParamName);
								continue;
							}
							// 紐付け対象のparamが子Canvasに無い場合はスキップ
							const auto it = subCanvasParams.find(subCanvasParamName);
							if (it == subCanvasParams.end())
							{
								continue;
							}
							const ParamType type = GetParamType(it->second);
							const ParamRefMode mode = ValidateParamRefModeFromJSON(*opt, AvailableParamRefModesFor(type), subCanvasParamName);

							// 元となる上書き値が必要なモードで上書きが無い場合はNormal相当にフォールバック
							if (ParamRefModeRequiresBaseValue(mode) && !m_paramOverrideCache.contains(subCanvasParamName))
							{
								Logger << U"[NocoUI warning] SubCanvas param binding mode '{}' for '{}' requires a value override but none is set. Falling back to Normal."_fmt(EnumToString(mode), subCanvasParamName);
								continue;
							}

							m_paramBindingModeCache[subCanvasParamName] = mode;
						}
					}
				}
				m_appliedSerializedParamBindingModesJSON = currentParamBindingModesJSON;
			}

			// serializedParamBindingsJSONに従って親Canvasのパラメータを子Canvasに毎フレーム適用
			if (!m_paramBindingMappingCache.empty())
			{
				if (auto parentCanvas = node->containedCanvas())
				{
					const auto& parentParams = parentCanvas->params();
					const auto& subCanvasParams = m_canvas->params();
					for (const auto& [subCanvasParamName, parentParamName] : m_paramBindingMappingCache)
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
						const ParamType type = GetParamType(subCanvasIt->second);
						if (!IsParamTypeCompatibleWith(GetParamType(parentIt->second), type))
						{
							continue;
						}

						ParamRefMode mode = ParamRefMode::Normal;
						if (auto modeIt = m_paramBindingModeCache.find(subCanvasParamName); modeIt != m_paramBindingModeCache.end())
						{
							mode = modeIt->second;
						}

						// 元となる上書き値が必要なモードではキャッシュから取得
						// 上書きが無い場合はキャッシュ構築時にNormal相当へフォールバック済み
						ParamValue base = subCanvasIt->second;
						if (ParamRefModeRequiresBaseValue(mode))
						{
							if (auto overrideIt = m_paramOverrideCache.find(subCanvasParamName); overrideIt != m_paramOverrideCache.end())
							{
								base = overrideIt->second;
							}
						}

						if (auto result = ApplyParamMode(base, parentIt->second, mode, type))
						{
							m_canvas->setParamValue(subCanvasParamName, *result);
						}
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

	size_t SubCanvas::countParamBindingRefs(StringView paramName) const
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

	void SubCanvas::clearParamBindingRefs(StringView paramName)
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
		HashSet<String> keepKeys;
		bool changed = false;
		for (const auto& [subCanvasParamName, parentParamNameJSON] : json)
		{
			if (parentParamNameJSON.isString() && parentParamNameJSON.getString() == paramName)
			{
				changed = true;
				continue;
			}
			newJSON[subCanvasParamName] = parentParamNameJSON;
			keepKeys.insert(subCanvasParamName);
		}
		if (changed)
		{
			setSerializedParamBindingsJSON(newJSON.formatMinimum());
			if (auto newModes = RemoveOrphanedParamBindingModes(m_serializedParamBindingModesJSON.value(), keepKeys))
			{
				setSerializedParamBindingModesJSON(*newModes);
			}
		}
	}

	void SubCanvas::replaceParamBindingRefs(StringView oldName, StringView newName)
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

	Array<String> SubCanvas::removeInvalidParamBindingRefs(const Node& node)
	{
		const auto parentCanvas = node.containedCanvas();
		if (!parentCanvas)
		{
			return {};
		}
		const JSON json = ParseBindingsJSON(m_serializedParamBindingsJSON.value());
		if (json.isEmpty())
		{
			return {};
		}
		const auto& validParams = parentCanvas->params();
		HashSet<String> clearedParamsSet;
		HashSet<String> keepKeys;
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
			const auto parentIt = validParams.find(parentParamName);
			if (parentIt == validParams.end())
			{
				clearedParamsSet.insert(parentParamName);
				changed = true;
				continue;
			}
			// 子paramが存在し、かつ型が非互換の場合はクリア
			if (m_canvas)
			{
				const auto& subCanvasParams = m_canvas->params();
				if (const auto childIt = subCanvasParams.find(subCanvasParamName);
					childIt != subCanvasParams.end()
					&& !IsParamTypeCompatibleWith(GetParamType(parentIt->second), GetParamType(childIt->second)))
				{
					clearedParamsSet.insert(parentParamName);
					changed = true;
					continue;
				}
			}
			newJSON[subCanvasParamName] = parentParamNameJSON;
			keepKeys.insert(subCanvasParamName);
		}
		if (changed)
		{
			setSerializedParamBindingsJSON(newJSON.formatMinimum());
			if (auto newModes = RemoveOrphanedParamBindingModes(m_serializedParamBindingModesJSON.value(), keepKeys))
			{
				setSerializedParamBindingModesJSON(*newModes);
			}
		}
		return Array<String>(clearedParamsSet.begin(), clearedParamsSet.end());
	}

	void SubCanvas::populateParamBindingRefs(HashSet<String>* pParamRefs) const
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
		replaceParamBindingRefs(oldName, newName);
	}

	void SubCanvas::normalizeParamBindingModes()
	{
		const JSON bindingsJSON = ParseBindingsJSON(m_serializedParamBindingsJSON.value());
		if (bindingsJSON.isEmpty())
		{
			const String& currentModes = m_serializedParamBindingModesJSON.value();
			if (!currentModes.isEmpty() && currentModes != U"{}")
			{
				setSerializedParamBindingModesJSON(U"{}");
			}
			return;
		}
		const JSON modesJSON = ParseBindingsJSON(m_serializedParamBindingModesJSON.value());
		JSON newModesJSON = modesJSON.isEmpty() ? JSON::Parse(U"{}") : modesJSON;
		bool changed = false;
		for (const auto& [subCanvasParamName, parentParamNameJSON] : bindingsJSON)
		{
			if (!newModesJSON.hasElement(subCanvasParamName))
			{
				newModesJSON[subCanvasParamName] = EnumToString(ParamRefMode::Normal);
				changed = true;
			}
		}
		if (changed)
		{
			setSerializedParamBindingModesJSON(newModesJSON.formatMinimum());
		}
	}
}
