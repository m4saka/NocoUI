#pragma once
#include <Siv3D.hpp>
#include "ComponentBase.hpp"
#include "../Node.hpp"

namespace noco
{
	class PlaceholderComponent : public SerializableComponentBase, public std::enable_shared_from_this<PlaceholderComponent>
	{
	private:
		String m_originalType;
		JSON m_originalData;

		/* NonSerialized */ const void* m_schema = nullptr;
		/* NonSerialized */ Optional<Texture> m_thumbnailTexture;

	public:
		PlaceholderComponent(const String& originalType, const JSON& originalData, detail::WithInstanceIdYN withInstanceId = detail::WithInstanceIdYN::No)
			: SerializableComponentBase{ U"Placeholder", {} }
			, m_originalType{ originalType }
			, m_originalData(originalData)  // ここは丸括弧である必要がある(波括弧だとコピーではなく要素数1で配列化されてしまう)
		{
			
			if (withInstanceId && originalData.contains(U"_instanceId"))
			{
				setInstanceId(originalData[U"_instanceId"].get<uint64>());
			}
		}

		[[nodiscard]]
		static std::shared_ptr<PlaceholderComponent> Create(const String& originalType, const JSON& originalData, detail::WithInstanceIdYN withInstanceId = detail::WithInstanceIdYN::No)
		{
			return std::make_shared<PlaceholderComponent>(originalType, originalData, withInstanceId);
		}

		[[nodiscard]]
		const String& originalType() const
		{
			return m_originalType;
		}

		[[nodiscard]]
		const JSON& originalData() const
		{
			return m_originalData;
		}

		JSON toJSONOverrideInternal(detail::WithInstanceIdYN withInstanceId) const override
		{
			JSON result;
			if (m_originalData.isObject())
			{
				// すべての値を文字列として保存
				for (const auto& [key, value] : m_originalData)
				{
					if (key == U"_instanceId" && value.isNumber())
					{
						// _instanceIdは数値のまま保持
						result[key] = value;
					}
					else if (value.isString())
					{
						result[key] = value.getString();
					}
					else
					{
						// 文字列以外の型は受け付けない（全てのプロパティ値は型に関わらず文字列としてシリアライズする仕様のため）
						Logger << U"[NocoUI warning] Property '{}' has non-string value in type '{}', interpreted as empty value"_fmt(key, m_originalType);
						result[key] = U"";
					}
				}
			}
			if (!result.contains(U"type"))
			{
				result[U"type"] = m_originalType;
			}
			if (withInstanceId)
			{
				result[U"_instanceId"] = instanceId();
			}
			return result;
		}

		bool tryReadFromJSONOverrideInternal(const JSON& json, detail::WithInstanceIdYN withInstanceId) override
		{
			if (!json.contains(U"type"))
			{
				return false;
			}
			
			m_originalType = json[U"type"].getString();
			m_originalData = json;
			
			if (withInstanceId && json.contains(U"_instanceId"))
			{
				setInstanceId(json[U"_instanceId"].get<uint64>());
			}
			
			return true;
		}

		void draw(const Node& node) const override
		{
			if (m_thumbnailTexture)
			{
				const RectF rect = node.regionRect();

				// サムネイル画像を描画
				const Vec2 textureSize{ m_thumbnailTexture->size() };
				const Vec2 rectSize = rect.size;

				// アスペクト比を保持しながら矩形に収まるようにスケール計算
				const double scaleX = rectSize.x / textureSize.x;
				const double scaleY = rectSize.y / textureSize.y;
				const double scale = Min(Min(scaleX, scaleY), 1.0); // 最大で原寸サイズ

				const Vec2 drawSize = textureSize * scale;
				const Vec2 drawPos = rect.center() - drawSize / 2;

				m_thumbnailTexture->resized(drawSize).draw(drawPos);
			}
		}
		
		void setSchema(const void* schema) { m_schema = schema; }
		const void* schema() const { return m_schema; }
		void setThumbnailTexture(const Optional<Texture>& texture) { m_thumbnailTexture = texture; }
		const Optional<Texture>& thumbnailTexture() const { return m_thumbnailTexture; }

		[[nodiscard]]
		String getPropertyValueString(const String& propertyName) const
		{
			if (m_originalData.isObject() && m_originalData.contains(propertyName))
			{
				const JSON& value = m_originalData[propertyName];
				if (value.isString())
				{
					return value.getString();
				}
				// 文字列以外の型は受け付けない（全てのプロパティ値は型に関わらず文字列としてシリアライズする仕様のため）
				Logger << U"[NocoUI warning] Property '{}' has non-string value in type '{}', interpreted as empty value"_fmt(propertyName, m_originalType);
				return U"";
			}
			return U"";
		}
		
		void setPropertyValueString(const String& propertyName, const String& value)
		{
			if (!m_originalData.isObject())
			{
				m_originalData = JSON{};
			}
			m_originalData[propertyName] = value;
		}
		
		
		[[nodiscard]]
		bool hasProperty(const String& propertyName) const
		{
			return m_originalData.isObject() && m_originalData.contains(propertyName);
		}
		
		[[nodiscard]]
		Array<String> getPropertyNames() const
		{
			Array<String> names;
			if (m_originalData.isObject())
			{
				for (const auto& member : m_originalData)
				{
					const String key = member.key;
					if (key != U"type" && key != U"_instanceId")
					{
						names.push_back(key);
					}
				}
			}
			return names;
		}
	};
}
