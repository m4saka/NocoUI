#include <catch2/catch.hpp>
#include <NocoUI/ComponentFactory.hpp>
#include <NocoUI/Component/Component.hpp>
#include <NocoUI/Canvas.hpp>
#include <NocoUI/Node.hpp>

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

namespace
{
	// グローバル登録テスト用の独自コンポーネント
	class FactoryTestComponent : public noco::SerializableComponentBase
	{
	private:
		noco::Property<String> m_text{ U"text", U"" };

	public:
		FactoryTestComponent()
			: noco::SerializableComponentBase{ U"FactoryTestComponent", { &m_text } }
		{
		}
	};
}

TEST_CASE("RegisterSerializableComponent global registration", "[ComponentFactory][SubCanvas]")
{
	// 独自コンポーネントを含む子Canvasを一時ファイルに保存
	auto childCanvas = noco::Canvas::Create(SizeF{ 100, 100 });
	auto childNode = noco::Node::Create(U"Child");
	childNode->emplaceComponent<FactoryTestComponent>();
	childCanvas->addChild(childNode);
	const FilePath tempPath = FileSystem::PathAppend(FileSystem::TemporaryDirectoryPath(), U"noco_test_factory_propagation.noco");
	REQUIRE(childCanvas->toJSON().save(tempPath));

	// SubCanvasを持つ親CanvasのJSONを作成
	auto sourceCanvas = noco::Canvas::Create();
	auto ownerNode = noco::Node::Create(U"Owner");
	ownerNode->emplaceComponent<noco::SubCanvas>(tempPath);
	sourceCanvas->addChild(ownerNode);
	const JSON parentJSON = sourceCanvas->toJSON();

	SECTION("Registered custom component is resolved on Canvas load")
	{
		noco::RegisterSerializableComponent<FactoryTestComponent>(U"FactoryTestComponent");

		auto canvas = noco::Canvas::CreateFromJSON(parentJSON);
		REQUIRE(canvas != nullptr);
		canvas->update();

		auto owner = canvas->findByName(U"Owner");
		REQUIRE(owner != nullptr);
		auto subCanvas = owner->getComponent<noco::SubCanvas>();
		REQUIRE(subCanvas != nullptr);
		auto loadedChildCanvas = subCanvas->canvas();
		REQUIRE(loadedChildCanvas != nullptr);

		// SubCanvasが読み込む入れ子Canvasでも独自コンポーネントが解決される
		auto loadedChildNode = loadedChildCanvas->findByName(U"Child");
		REQUIRE(loadedChildNode != nullptr);
		REQUIRE(loadedChildNode->getComponent<FactoryTestComponent>() != nullptr);

		noco::ResetSerializableComponents();
	}

	SECTION("Unregistered custom component is dropped")
	{
		auto canvas = noco::Canvas::CreateFromJSON(parentJSON);
		REQUIRE(canvas != nullptr);
		canvas->update();

		auto subCanvas = canvas->findByName(U"Owner")->getComponent<noco::SubCanvas>();
		REQUIRE(subCanvas != nullptr);
		auto loadedChildCanvas = subCanvas->canvas();
		REQUIRE(loadedChildCanvas != nullptr);

		auto loadedChildNode = loadedChildCanvas->findByName(U"Child");
		REQUIRE(loadedChildNode != nullptr);
		REQUIRE(loadedChildNode->getComponent<FactoryTestComponent>() == nullptr);
	}

	SECTION("SetUnknownComponentHandler is used for unregistered types")
	{
		bool handlerCalled = false;
		noco::SetUnknownComponentHandler(
			[&handlerCalled](const String&, const JSON&, noco::detail::WithInstanceIdYN) -> std::shared_ptr<noco::ComponentBase>
			{
				handlerCalled = true;
				return nullptr;
			});

		auto canvas = noco::Canvas::CreateFromJSON(parentJSON);
		REQUIRE(canvas != nullptr);
		canvas->update();
		REQUIRE(handlerCalled);

		noco::ResetSerializableComponents();
	}

	SECTION("ResetSerializableComponents removes registration")
	{
		noco::RegisterSerializableComponent<FactoryTestComponent>(U"FactoryTestComponent");
		REQUIRE(noco::detail::GetGlobalComponentFactory().hasType(U"FactoryTestComponent"));

		noco::ResetSerializableComponents();
		REQUIRE_FALSE(noco::detail::GetGlobalComponentFactory().hasType(U"FactoryTestComponent"));
	}

	FileSystem::Remove(tempPath);
}
