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
		/// @brief 複数ディレクトリからスキーマを読み込み(先頭のディレクトリほど優先され、同名typeは優先度の高い方を採用)
		static void LoadFromDirectories(const Array<FilePath>& directories);
		
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
