#include <Catch2/catch.hpp>
#include <NocoUI/ComponentFactory.hpp>
#include <NocoUI/Component/Component.hpp>

TEST_CASE("ComponentFactory basic functionality", "[ComponentFactory]")
{
	SECTION("Get builtin factory")
	{
		const auto& componentFactory = noco::ComponentFactory::GetBuiltinFactory();
		REQUIRE(componentFactory.hasType(U"Label"));
		REQUIRE(componentFactory.hasType(U"RectRenderer"));
		REQUIRE(componentFactory.hasType(U"TextBox"));
		REQUIRE(componentFactory.hasType(U"Sprite"));
		REQUIRE(componentFactory.hasType(U"EventTrigger"));
		REQUIRE(componentFactory.hasType(U"UISound"));
		REQUIRE(componentFactory.hasType(U"Tween"));
		REQUIRE(componentFactory.hasType(U"CursorChanger"));
		
		auto types = componentFactory.getRegisteredTypes();
		REQUIRE(types.size() >= 8);
		REQUIRE(types.contains(U"Label"));
		REQUIRE(types.contains(U"RectRenderer"));
	}
	
	SECTION("Check type registration")
	{
		const auto& componentFactory = noco::ComponentFactory::GetBuiltinFactory();
		
		REQUIRE_FALSE(componentFactory.hasType(U"NonExistentComponent"));
		REQUIRE_FALSE(componentFactory.hasType(U""));
	}
}

TEST_CASE("ComponentFactory component creation", "[ComponentFactory]")
{
	const auto& componentFactory = noco::ComponentFactory::GetBuiltinFactory();
	
	SECTION("Create known components")
	{
		{
			JSON labelJson;
			labelJson[U"type"] = U"Label";
			labelJson[U"text"] = U"Test Label";
			
			auto component = componentFactory.createComponentFromJSON(labelJson);
			REQUIRE(component != nullptr);
			
			auto label = std::dynamic_pointer_cast<noco::Label>(component);
			REQUIRE(label != nullptr);
			REQUIRE(label->text().defaultValue == U"Test Label");
		}
		
		{
			JSON rectJson;
			rectJson[U"type"] = U"RectRenderer";
			rectJson[U"fillColor"] = U"#FF0000FF";
			
			auto component = componentFactory.createComponentFromJSON(rectJson);
			REQUIRE(component != nullptr);
			
			auto rect = std::dynamic_pointer_cast<noco::RectRenderer>(component);
			REQUIRE(rect != nullptr);
		}
	}
	
	SECTION("Unknown component handling - Skip behavior")
	{
		auto factory = noco::ComponentFactory::CreateWithBuiltinComponents();
		factory.setUnknownComponentBehavior(noco::UnknownComponentBehavior::Skip);
		
		JSON unknownJson;
		unknownJson[U"type"] = U"UnknownType";
		unknownJson[U"customProperty"] = U"customValue";
		
		auto component = factory.createComponentFromJSON(unknownJson);
		REQUIRE(component == nullptr);
	}
	
	SECTION("Unknown component handling - Placeholder behavior")
	{
		auto factory = noco::ComponentFactory::CreateWithBuiltinComponents();
		factory.setUnknownComponentBehavior(noco::UnknownComponentBehavior::CreatePlaceholder);
		
		JSON unknownJson;
		unknownJson[U"type"] = U"UnknownType";
		unknownJson[U"customProperty"] = U"customValue";
		
		auto component = factory.createComponentFromJSON(unknownJson);
		REQUIRE(component != nullptr);
		
		auto placeholder = std::dynamic_pointer_cast<noco::PlaceholderComponent>(component);
		REQUIRE(placeholder != nullptr);
		REQUIRE(placeholder->originalType() == U"UnknownType");
	}
	
	SECTION("Unknown component handling - Error behavior")
	{
		auto factory = noco::ComponentFactory::CreateWithBuiltinComponents();
		factory.setUnknownComponentBehavior(noco::UnknownComponentBehavior::ThrowError);
		
		JSON unknownJson;
		unknownJson[U"type"] = U"UnknownType";
		
		REQUIRE_THROWS_AS(factory.createComponentFromJSON(unknownJson), s3d::Error);
	}
}

TEST_CASE("ComponentFactory custom component factory", "[ComponentFactory]")
{
	SECTION("Create custom component factory")
	{
		noco::ComponentFactory customRegistry;
		
		// 初期状態では何も登録されていない
		REQUIRE_FALSE(customRegistry.hasType(U"Label"));
		REQUIRE(customRegistry.getRegisteredTypes().isEmpty());
		
		// Labelだけ登録
		customRegistry.registerComponentType<noco::Label>(U"Label");
		
		REQUIRE(customRegistry.hasType(U"Label"));
		REQUIRE_FALSE(customRegistry.hasType(U"RectRenderer"));
		
		auto types = customRegistry.getRegisteredTypes();
		REQUIRE(types.size() == 1);
		REQUIRE(types.contains(U"Label"));
	}
}

TEST_CASE("ComponentFactory JSON serialization with unknown components", "[ComponentFactory]")
{
	SECTION("Placeholder component creation and JSON output")
	{
		auto factory = noco::ComponentFactory::CreateWithBuiltinComponents();
		factory.setUnknownComponentBehavior(noco::UnknownComponentBehavior::CreatePlaceholder);
		
		// 未知コンポーネントのJSONデータ
		JSON unknownComponentJson;
		unknownComponentJson[U"type"] = U"CustomButton";
		unknownComponentJson[U"buttonText"] = U"Click Me";
		unknownComponentJson[U"color"] = U"#FF0000FF";
		
		auto unknownComponent = factory.createComponentFromJSON(unknownComponentJson);
		REQUIRE(unknownComponent != nullptr);
		
		auto placeholder = std::dynamic_pointer_cast<noco::PlaceholderComponent>(unknownComponent);
		REQUIRE(placeholder != nullptr);
		REQUIRE(placeholder->originalType() == U"CustomButton");
	}
}

TEST_CASE("PlaceholderComponent behavior", "[PlaceholderComponent]")
{
	SECTION("Create placeholder component")
	{
		JSON originalData;
		originalData[U"type"] = U"CustomWidget";
		originalData[U"width"] = U"100";
		originalData[U"height"] = U"50";
		originalData[U"_instanceId"] = 12345;
		
		auto placeholder = noco::PlaceholderComponent::Create(U"CustomWidget", originalData, noco::detail::WithInstanceIdYN::Yes);
		REQUIRE(placeholder != nullptr);
		REQUIRE(placeholder->originalType() == U"CustomWidget");
		REQUIRE(placeholder->instanceId() == 12345);
		
		JSON outputJson = placeholder->toJSON(noco::detail::WithInstanceIdYN::Yes);
		REQUIRE(outputJson[U"type"].getString() == U"CustomWidget");
		REQUIRE(outputJson[U"width"].getString() == U"100");
		REQUIRE(outputJson[U"height"].getString() == U"50");
		REQUIRE(outputJson[U"_instanceId"].get<uint64>() == 12345);
	}
	
	SECTION("getPropertyValueString returns empty for non-strings")
	{
		JSON originalData;
		originalData[U"type"] = U"CustomWidget";
		originalData[U"text"] = U"Hello";
		originalData[U"width"] = 100;
		originalData[U"height"] = 50.5;
		originalData[U"enabled"] = true;
		originalData[U"items"] = Array<JSON>{};
		originalData[U"config"] = JSON{};
		
		auto placeholder = noco::PlaceholderComponent::Create(U"CustomWidget", originalData);
		
		// 文字列型のみ値を返す
		REQUIRE(placeholder->getPropertyValueString(U"text") == U"Hello");
		
		// 文字列以外の型は受け付けないため空文字列を返す（仕様）
		REQUIRE(placeholder->getPropertyValueString(U"width") == U"");
		REQUIRE(placeholder->getPropertyValueString(U"height") == U"");
		REQUIRE(placeholder->getPropertyValueString(U"enabled") == U"");
		REQUIRE(placeholder->getPropertyValueString(U"items") == U"");
		REQUIRE(placeholder->getPropertyValueString(U"config") == U"");
		
		REQUIRE(placeholder->getPropertyValueString(U"nonexistent") == U"");
	}
}

TEST_CASE("ComponentFactory behavior configuration", "[ComponentFactory]")
{
	SECTION("Default behavior is Skip")
	{
		auto factory = noco::ComponentFactory::CreateWithBuiltinComponents();
		REQUIRE(factory.unknownComponentBehavior() == noco::UnknownComponentBehavior::Skip);
	}
	
	SECTION("Behavior can be changed")
	{
		auto factory = noco::ComponentFactory::CreateWithBuiltinComponents();
		
		factory.setUnknownComponentBehavior(noco::UnknownComponentBehavior::CreatePlaceholder);
		REQUIRE(factory.unknownComponentBehavior() == noco::UnknownComponentBehavior::CreatePlaceholder);
		
		factory.setUnknownComponentBehavior(noco::UnknownComponentBehavior::ThrowError);
		REQUIRE(factory.unknownComponentBehavior() == noco::UnknownComponentBehavior::ThrowError);
		
		factory.setUnknownComponentBehavior(noco::UnknownComponentBehavior::Skip);
		REQUIRE(factory.unknownComponentBehavior() == noco::UnknownComponentBehavior::Skip);
	}
}

TEST_CASE("PlaceholderComponent roundtrip serialization", "[PlaceholderComponent]")
{
	SECTION("Save and load with original types preserved")
	{
		JSON originalData;
		originalData[U"type"] = U"CustomWidget";
		originalData[U"width"] = 100;
		originalData[U"height"] = 50.5;
		originalData[U"enabled"] = true;
		originalData[U"name"] = U"MyWidget";
		
		auto placeholder = noco::PlaceholderComponent::Create(U"CustomWidget", originalData);
		REQUIRE(placeholder != nullptr);
		REQUIRE(placeholder->originalType() == U"CustomWidget");
		
		JSON savedJson = placeholder->toJSON();
		REQUIRE(savedJson[U"type"].getString() == U"CustomWidget");
		// 文字列以外は空文字列として保存される
		REQUIRE(savedJson[U"width"].getString() == U"");
		REQUIRE(savedJson[U"height"].getString() == U"");
		REQUIRE(savedJson[U"enabled"].getString() == U"");
		REQUIRE(savedJson[U"name"].getString() == U"MyWidget");
		
		auto loaded = std::make_shared<noco::PlaceholderComponent>(U"", JSON{});
		
		bool loadSuccess = loaded->tryReadFromJSON(savedJson);
		REQUIRE(loadSuccess);
		
		REQUIRE(loaded->originalType() == U"CustomWidget");
		// 空文字列として読み込まれる
		REQUIRE(loaded->originalData()[U"width"].getString() == U"");
		REQUIRE(loaded->originalData()[U"height"].getString() == U"");
		REQUIRE(loaded->originalData()[U"enabled"].getString() == U"");
		REQUIRE(loaded->originalData()[U"name"].getString() == U"MyWidget");
	}
	
	SECTION("Editor property modification stores as strings")
	{
		JSON originalData;
		originalData[U"type"] = U"CustomButton";
		originalData[U"x"] = 10;
		originalData[U"y"] = 20;
		originalData[U"visible"] = true;
		
		auto placeholder = noco::PlaceholderComponent::Create(U"CustomButton", originalData);
		
		placeholder->setPropertyValueString(U"x", U"30");
		placeholder->setPropertyValueString(U"visible", U"false");
		
		JSON savedJson = placeholder->toJSON();
		
		// 全プロパティの値が文字列で保存される
		REQUIRE(savedJson[U"x"].isString());
		REQUIRE(savedJson[U"x"].getString() == U"30");
		REQUIRE(savedJson[U"y"].isString());
		REQUIRE(savedJson[U"y"].getString() == U"");  // 未編集の場合も文字列以外は空文字列で保存される
		REQUIRE(savedJson[U"visible"].isString());
		REQUIRE(savedJson[U"visible"].getString() == U"false");
	}
}
