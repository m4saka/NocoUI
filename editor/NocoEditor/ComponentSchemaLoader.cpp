#include "ComponentSchemaLoader.hpp"
#include "NocoUI/Serialization.hpp"

namespace noco::editor
{
	HashTable<String, ComponentSchema> ComponentSchemaLoader::s_schemas;
	
	void ComponentSchemaLoader::LoadFromDirectory(const FilePath& directory)
	{
		s_schemas.clear();
		
		if (!FileSystem::Exists(directory))
		{
			return;
		}
		
		LoadFromDirectoryRecursive(directory);
	}
	
	void ComponentSchemaLoader::LoadFromDirectoryRecursive(const FilePath& directory)
	{
		for (const auto& path : FileSystem::DirectoryContents(directory))
		{
			if (FileSystem::IsDirectory(path))
			{
				LoadFromDirectoryRecursive(path);
			}
			else if (path.ends_with(U".json"))
			{
				if (auto schema = LoadSchemaFile(path))
				{
					s_schemas[schema->type] = *schema;
				}
			}
		}
	}
	
	Optional<ComponentSchema> ComponentSchemaLoader::LoadSchemaFile(const FilePath& path)
	{
		const JSON json = JSON::Load(path);
		if (!json)
		{
			Logger << U"[NocoUI warning] Failed to load component schema file: {}"_fmt(path);
			return none;
		}
		
		if (!json.contains(U"type"))
		{
			Logger << U"[NocoUI warning] Component schema missing 'type' field: {}"_fmt(path);
			return none;
		}
		
		ComponentSchema schema;
		schema.type = json[U"type"].getString();
		
		if (json.contains(U"properties"))
		{
			for (const auto& propJson : json[U"properties"].arrayView())
			{
				if (auto prop = ParsePropertySchema(propJson))
				{
					schema.properties.push_back(*prop);
				}
			}
		}

		// 同じファイル名で.png拡張子のファイルが存在する場合は読み込む
		const FilePath baseName = FileSystem::BaseName(path);
		const FilePath directory = FileSystem::ParentPath(path);
		const FilePath thumbnailPath = FileSystem::PathAppend(directory, baseName + U".png");
		if (FileSystem::Exists(thumbnailPath))
		{
			schema.thumbnailTexture = Texture{ thumbnailPath };
			if (schema.thumbnailTexture->isEmpty())
			{
				Logger << U"[NocoUI warning] Failed to load thumbnail image: {}"_fmt(thumbnailPath);
				schema.thumbnailTexture = none;
			}
		}

		return schema;
	}
	
	const ComponentSchema* ComponentSchemaLoader::GetSchema(const String& typeName)
	{
		if (auto it = s_schemas.find(typeName); it != s_schemas.end())
		{
			return &it->second;
		}
		return nullptr;
	}
	
	const HashTable<String, ComponentSchema>& ComponentSchemaLoader::GetAllSchemas()
	{
		return s_schemas;
	}
	
	bool ComponentSchemaLoader::HasSchema(const String& typeName)
	{
		return s_schemas.contains(typeName);
	}
	
	Optional<PropertySchema> ComponentSchemaLoader::ParsePropertySchema(const JSON& json)
	{
		if (!json.contains(U"name"))
		{
			return none;
		}
		
		PropertySchema prop;
		prop.name = json[U"name"].getString();
		
		if (json.contains(U"displayName"))
		{
			prop.displayName = json[U"displayName"].getString();
		}
		
		if (json.contains(U"editType"))
		{
			prop.editType = StringToEnum<PropertyEditType>(json[U"editType"].getString(), PropertyEditType::Text);
		}
		else
		{
			prop.editType = PropertyEditType::Text;
		}
		
		if (json.contains(U"defaultValue"))
		{
			prop.defaultValue = json[U"defaultValue"].getString();
		}
		
		if (json.contains(U"tooltip"))
		{
			prop.tooltip = json[U"tooltip"].getString();
		}
		
		if (json.contains(U"tooltipDetail"))
		{
			prop.tooltipDetail = json[U"tooltipDetail"].getString();
		}
		
		if (json.contains(U"enumCandidates"))
		{
			for (const auto& candidate : json[U"enumCandidates"].arrayView())
			{
				prop.enumCandidates.push_back(candidate.getString());
			}
		}
		
		if (json.contains(U"numTextAreaLines"))
		{
			prop.numTextAreaLines = json[U"numTextAreaLines"].get<int32>();
		}
		
		if (json.contains(U"dragValueChangeStep"))
		{
			prop.dragValueChangeStep = json[U"dragValueChangeStep"].get<double>();
		}
		
		if (json.contains(U"refreshInspectorOnChange"))
		{
			prop.refreshInspectorOnChange = json[U"refreshInspectorOnChange"].get<bool>();
		}
		
		return prop;
	}
}
