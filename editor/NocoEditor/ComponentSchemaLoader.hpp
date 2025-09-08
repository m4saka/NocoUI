#pragma once
#include <Siv3D.hpp>
#include "ComponentSchema.hpp"

namespace noco::editor
{
	class ComponentSchemaLoader
	{
	private:
		static HashTable<String, ComponentSchema> s_schemas;
		
	public:
		static void LoadFromDirectory(const FilePath& directory);
		
		static Optional<ComponentSchema> LoadSchemaFile(const FilePath& path);
		
		[[nodiscard]]
		static const ComponentSchema* GetSchema(const String& typeName);
		
		[[nodiscard]]
		static const HashTable<String, ComponentSchema>& GetAllSchemas();
		
		[[nodiscard]]
		static bool HasSchema(const String& typeName);
		
	private:
		static void LoadFromDirectoryRecursive(const FilePath& directory);
		
		static Optional<PropertySchema> ParsePropertySchema(const JSON& json);
	};
}
