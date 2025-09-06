#include <catch2/catch.hpp>
#include <NocoUI/ComponentFactory.hpp>
#include <NocoUI/Component/Component.hpp>

// ========================================
// ComponentFactoryのテスト
// ========================================

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
			REQUIRE(label->text().defaultValue() == U"Test Label");
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
	
	SECTION("Unknown component handling - No handler")
	{
		auto factory = noco::ComponentFactory::CreateWithBuiltinComponents();
		// デフォルトではハンドラーが設定されていない
		
		JSON unknownJson;
		unknownJson[U"type"] = U"UnknownType";
		unknownJson[U"customProperty"] = U"customValue";
		
		auto component = factory.createComponentFromJSON(unknownJson);
		REQUIRE(component == nullptr);
	}
	
	SECTION("Unknown component handling - With handler")
	{
		auto factory = noco::ComponentFactory::CreateWithBuiltinComponents();
		
		// カスタムコールバックを設定
		bool handlerCalled = false;
		String capturedType;
		factory.setUnknownComponentHandler(
			[&handlerCalled, &capturedType](const String& type, const JSON&, noco::detail::WithInstanceIdYN) -> std::shared_ptr<noco::ComponentBase>
			{
				handlerCalled = true;
				capturedType = type;
				return std::make_shared<noco::Label>(U"Unknown: " + type);
			});
		
		JSON unknownJson;
		unknownJson[U"type"] = U"UnknownType";
		unknownJson[U"customProperty"] = U"customValue";
		
		auto component = factory.createComponentFromJSON(unknownJson);
		REQUIRE(component != nullptr);
		REQUIRE(handlerCalled);
		REQUIRE(capturedType == U"UnknownType");
		
		auto label = std::dynamic_pointer_cast<noco::Label>(component);
		REQUIRE(label != nullptr);
		REQUIRE(label->text().defaultValue() == U"Unknown: UnknownType");
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

TEST_CASE("ComponentFactory handler configuration", "[ComponentFactory]")
{
	SECTION("Handler can be set and cleared with nullptr")
	{
		auto factory = noco::ComponentFactory::CreateWithBuiltinComponents();
		
		// デフォルトでコールバックは設定されていない（警告メッセージのみ）
		JSON unknownJson;
		unknownJson[U"type"] = U"UnknownType";
		auto component = factory.createComponentFromJSON(unknownJson);
		REQUIRE(component == nullptr);
		
		// コールバックを設定
		bool handlerCalled = false;
		factory.setUnknownComponentHandler(
			[&handlerCalled](const String&, const JSON&, noco::detail::WithInstanceIdYN) -> std::shared_ptr<noco::ComponentBase>
			{
				handlerCalled = true;
				return std::make_shared<noco::Label>(U"Mock");
			});
		
		component = factory.createComponentFromJSON(unknownJson);
		REQUIRE(handlerCalled);
		REQUIRE(component != nullptr);
		
		// nullptrを設定してコールバックをクリア
		factory.setUnknownComponentHandler(nullptr);
		handlerCalled = false;
		component = factory.createComponentFromJSON(unknownJson);
		REQUIRE_FALSE(handlerCalled);
		REQUIRE(component == nullptr);
	}
}
