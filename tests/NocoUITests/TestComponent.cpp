#include <catch2/catch.hpp>
#include <Siv3D.hpp>
#include <NocoUI.hpp>

// ========================================
// Componentのテスト
// ========================================

TEST_CASE("Component system", "[Component]")
{
	SECTION("Add component")
	{
		auto node = noco::Node::Create();
		auto label = node->emplaceComponent<noco::Label>();
		
		REQUIRE(label != nullptr);
		REQUIRE(node->getComponentOrNull<noco::Label>() == label);
	}

	SECTION("Multiple components")
	{
		auto node = noco::Node::Create();
		auto label = node->emplaceComponent<noco::Label>();
		auto rect = node->emplaceComponent<noco::RectRenderer>();
		
		REQUIRE(node->getComponentOrNull<noco::Label>() == label);
		REQUIRE(node->getComponentOrNull<noco::RectRenderer>() == rect);
	}

	SECTION("Remove component")
	{
		auto node = noco::Node::Create();
		auto label = node->emplaceComponent<noco::Label>();
		
		node->removeComponent(label);
		
		REQUIRE(node->getComponentOrNull<noco::Label>() == nullptr);
	}
}

// Labelコンポーネントのテスト
TEST_CASE("Label component", "[Component][Label]")
{
	SECTION("Basic text properties")
	{
		auto node = noco::Node::Create();
		auto label = node->emplaceComponent<noco::Label>();
		
		label->setText(U"Hello, World!");
		REQUIRE(label->text().defaultValue == U"Hello, World!");
	}
}

// TextBoxコンポーネントのテスト
TEST_CASE("TextBox component", "[Component][TextBox]")
{
	SECTION("Basic text properties")
	{
		auto node = noco::Node::Create();
		auto textBox = node->emplaceComponent<noco::TextBox>();
		
		textBox->setText(U"Initial text");
		REQUIRE(textBox->text() == U"Initial text");
	}
}

// TextAreaコンポーネントのテスト
TEST_CASE("TextArea component", "[Component][TextArea]")
{
	SECTION("Basic text properties")
	{
		auto node = noco::Node::Create();
		auto textArea = node->emplaceComponent<noco::TextArea>();
		
		String multilineText = U"Line 1\nLine 2\nLine 3";
		textArea->setText(multilineText);
		REQUIRE(textArea->text() == multilineText);
	}
}

// onActivated/onDeactivatedの挙動テスト
TEST_CASE("Component lifecycle callbacks", "[Component][Lifecycle]")
{
	SECTION("onActivated/onDeactivated callback behavior")
	{
		// テスト用のComponentBaseを継承したクラス
		class TestComponent : public noco::ComponentBase, public std::enable_shared_from_this<TestComponent>
		{
		public:
			int32 activatedCount = 0;
			int32 deactivatedCount = 0;
			std::shared_ptr<noco::Node> lastActivatedNode;
			std::shared_ptr<noco::Node> lastDeactivatedNode;

			TestComponent() : ComponentBase{ {} } {}

			void onActivated(const std::shared_ptr<noco::Node>& node) override
			{
				activatedCount++;
				lastActivatedNode = node;
			}

			void onDeactivated(const std::shared_ptr<noco::Node>& node) override
			{
				deactivatedCount++;
				lastDeactivatedNode = node;
			}
		};

		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto node = noco::Node::Create(U"TestNode");
		auto testComponent = std::make_shared<TestComponent>();

		// 初期状態
		REQUIRE(testComponent->activatedCount == 0);
		REQUIRE(testComponent->deactivatedCount == 0);

		// まずCanvasに追加してactiveInHierarchyをtrueにする
		canvas->addChild(node);

		// Canvas配下でコンポーネント追加：onActivatedが呼ばれる
		node->addComponent(testComponent);
		REQUIRE(testComponent->activatedCount == 1);
		REQUIRE(testComponent->deactivatedCount == 0);
		REQUIRE(testComponent->lastActivatedNode == node);

		// ノード非アクティブ化：onDeactivatedが呼ばれる
		node->setActive(noco::ActiveYN::No);
		REQUIRE(testComponent->activatedCount == 1);
		REQUIRE(testComponent->deactivatedCount == 1);
		REQUIRE(testComponent->lastDeactivatedNode == node);

		// ノード再アクティブ化：onActivatedが再度呼ばれる
		node->setActive(noco::ActiveYN::Yes);
		REQUIRE(testComponent->activatedCount == 2);
		REQUIRE(testComponent->deactivatedCount == 1);

		// コンポーネント削除：onDeactivatedが呼ばれる
		node->removeComponent(testComponent);
		REQUIRE(testComponent->activatedCount == 2);
		REQUIRE(testComponent->deactivatedCount == 2);
	}

	SECTION("Canvas removal triggers onDeactivated")
	{
		class TestComponent : public noco::ComponentBase, public std::enable_shared_from_this<TestComponent>
		{
		public:
			int32 deactivatedCount = 0;

			TestComponent() : ComponentBase{ {} } {}

			void onDeactivated(const std::shared_ptr<noco::Node>& node) override
			{
				deactivatedCount++;
			}
		};

		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto node = noco::Node::Create(U"TestNode");
		auto testComponent = std::make_shared<TestComponent>();

		node->addComponent(testComponent);
		canvas->addChild(node);

		// 初期状態確認
		REQUIRE(testComponent->deactivatedCount == 0);

		// Canvas削除：onDeactivatedが呼ばれる
		canvas->removeChild(node);
		REQUIRE(testComponent->deactivatedCount == 1);
	}

	SECTION("Recursive onDeactivated for child nodes")
	{
		class TestComponent : public noco::ComponentBase, public std::enable_shared_from_this<TestComponent>
		{
		public:
			int32 deactivatedCount = 0;

			TestComponent() : ComponentBase{ {} } {}

			void onDeactivated(const std::shared_ptr<noco::Node>& node) override
			{
				deactivatedCount++;
			}
		};

		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto parentNode = noco::Node::Create(U"ParentNode");
		auto childNode = noco::Node::Create(U"ChildNode");
		auto grandchildNode = noco::Node::Create(U"GrandchildNode");

		auto parentComponent = std::make_shared<TestComponent>();
		auto childComponent = std::make_shared<TestComponent>();
		auto grandchildComponent = std::make_shared<TestComponent>();

		// 階層構造構築
		parentNode->addComponent(parentComponent);
		childNode->addComponent(childComponent);
		grandchildNode->addComponent(grandchildComponent);

		parentNode->addChild(childNode);
		childNode->addChild(grandchildNode);
		canvas->addChild(parentNode);

		// 初期状態確認
		REQUIRE(parentComponent->deactivatedCount == 0);
		REQUIRE(childComponent->deactivatedCount == 0);
		REQUIRE(grandchildComponent->deactivatedCount == 0);

		// 親ノードをCanvas削除：子孫も含めてonDeactivatedが呼ばれる
		canvas->removeChild(parentNode);
		REQUIRE(parentComponent->deactivatedCount == 1);
		REQUIRE(childComponent->deactivatedCount == 1);
		REQUIRE(grandchildComponent->deactivatedCount == 1);
	}

	SECTION("Canvas destructor triggers onDeactivated")
	{
		class TestComponent : public noco::ComponentBase, public std::enable_shared_from_this<TestComponent>
		{
		public:
			int32 deactivatedCount = 0;

			TestComponent() : ComponentBase{ {} } {}

			void onDeactivated(const std::shared_ptr<noco::Node>& node) override
			{
				deactivatedCount++;
			}
		};

		auto testComponent = std::make_shared<TestComponent>();

		{
			auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
			auto node = noco::Node::Create(U"TestNode");

			node->addComponent(testComponent);
			canvas->addChild(node);

			// 初期状態確認
			REQUIRE(testComponent->deactivatedCount == 0);
		} // Canvasがここでデストラクト

		// CanvasデストラクタでもonDeactivatedが呼ばれる
		REQUIRE(testComponent->deactivatedCount == 1);
	}

	SECTION("Node without Canvas - no onActivated/onDeactivated calls")
	{
		class TestComponent : public noco::ComponentBase, public std::enable_shared_from_this<TestComponent>
		{
		public:
			int32 activatedCount = 0;
			int32 deactivatedCount = 0;

			TestComponent() : ComponentBase{ {} } {}

			void onActivated(const std::shared_ptr<noco::Node>& node) override
			{
				activatedCount++;
			}

			void onDeactivated(const std::shared_ptr<noco::Node>& node) override
			{
				deactivatedCount++;
			}
		};

		auto testComponent = std::make_shared<TestComponent>();

		{
			auto node = noco::Node::Create(U"TestNode");
			
			// Canvas配下にないノードのactiveInHierarchyはfalseなので、onActivatedは呼ばれない
			node->addComponent(testComponent);
			REQUIRE(testComponent->activatedCount == 0);
			REQUIRE(testComponent->deactivatedCount == 0);

			// ノード非アクティブ化：activeInHierarchyがfalseなので状態変化なし
			node->setActive(noco::ActiveYN::No);
			REQUIRE(testComponent->activatedCount == 0);
			REQUIRE(testComponent->deactivatedCount == 0);

			// ノード再アクティブ化：activeInHierarchyがfalseなので状態変化なし
			node->setActive(noco::ActiveYN::Yes);
			REQUIRE(testComponent->activatedCount == 0);
			REQUIRE(testComponent->deactivatedCount == 0);

			// コンポーネント削除：activeInHierarchyがfalseなのでonDeactivatedは呼ばれない
			node->removeComponent(testComponent);
			REQUIRE(testComponent->activatedCount == 0);
			REQUIRE(testComponent->deactivatedCount == 0);
		} // ノードがここでデストラクト（Canvas配下でないので追加のonDeactivatedは呼ばれない）

		// Canvas配下でないNodeでは一切ライフサイクルコールバックは呼ばれない
		REQUIRE(testComponent->activatedCount == 0);
		REQUIRE(testComponent->deactivatedCount == 0);
	}

	SECTION("Inactive Canvas node does not trigger onActivated")
	{
		class TestComponent : public noco::ComponentBase, public std::enable_shared_from_this<TestComponent>
		{
		public:
			int32 activatedCount = 0;
			int32 deactivatedCount = 0;

			TestComponent() : ComponentBase{ {} } {}

			void onActivated(const std::shared_ptr<noco::Node>& node) override
			{
				activatedCount++;
			}

			void onDeactivated(const std::shared_ptr<noco::Node>& node) override
			{
				deactivatedCount++;
			}
		};

		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto node = noco::Node::Create(U"TestNode");
		auto testComponent = std::make_shared<TestComponent>();

		// ノードを先に非アクティブにしてからCanvas配下に追加
		node->setActive(noco::ActiveYN::No);
		canvas->addChild(node);

		// 非アクティブなCanvas配下ノードにコンポーネント追加：onActivatedは呼ばれない
		node->addComponent(testComponent);
		REQUIRE(testComponent->activatedCount == 0);
		REQUIRE(testComponent->deactivatedCount == 0);

		// ノードをアクティブ化：onActivatedが呼ばれる
		node->setActive(noco::ActiveYN::Yes);
		REQUIRE(testComponent->activatedCount == 1);
		REQUIRE(testComponent->deactivatedCount == 0);
	}

	SECTION("removeChildrenAll triggers onDeactivated for all children")
	{
		class TestComponent : public noco::ComponentBase, public std::enable_shared_from_this<TestComponent>
		{
		public:
			int32 deactivatedCount = 0;

			TestComponent() : ComponentBase{ {} } {}

			void onDeactivated(const std::shared_ptr<noco::Node>& node) override
			{
				deactivatedCount++;
			}
		};

		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto node1 = noco::Node::Create(U"Node1");
		auto node2 = noco::Node::Create(U"Node2");
		auto child = noco::Node::Create(U"Child");
		
		auto component1 = std::make_shared<TestComponent>();
		auto component2 = std::make_shared<TestComponent>();
		auto childComponent = std::make_shared<TestComponent>();

		// 階層構造とコンポーネントを設定
		node1->addComponent(component1);
		node2->addComponent(component2);
		child->addComponent(childComponent);
		node1->addChild(child);
		
		canvas->addChild(node1);
		canvas->addChild(node2);

		// 初期状態確認
		REQUIRE(component1->deactivatedCount == 0);
		REQUIRE(component2->deactivatedCount == 0);
		REQUIRE(childComponent->deactivatedCount == 0);

		// removeChildrenAll：全ての子と子孫のonDeactivatedが呼ばれる
		canvas->removeChildrenAll();
		REQUIRE(component1->deactivatedCount == 1);
		REQUIRE(component2->deactivatedCount == 1);
		REQUIRE(childComponent->deactivatedCount == 1);
	}

	SECTION("Parent node setActive triggers child component lifecycle")
	{
		class TestComponent : public noco::ComponentBase, public std::enable_shared_from_this<TestComponent>
		{
		public:
			int32 activatedCount = 0;
			int32 deactivatedCount = 0;
			std::shared_ptr<noco::Node> lastActivatedNode;
			std::shared_ptr<noco::Node> lastDeactivatedNode;

			TestComponent() : ComponentBase{ {} } {}

			void onActivated(const std::shared_ptr<noco::Node>& node) override
			{
				activatedCount++;
				lastActivatedNode = node;
			}

			void onDeactivated(const std::shared_ptr<noco::Node>& node) override
			{
				deactivatedCount++;
				lastDeactivatedNode = node;
			}
		};

		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto parent = noco::Node::Create(U"Parent");
		auto child = noco::Node::Create(U"Child");
		auto grandchild = noco::Node::Create(U"Grandchild");
		
		auto parentComponent = std::make_shared<TestComponent>();
		auto childComponent = std::make_shared<TestComponent>();
		auto grandchildComponent = std::make_shared<TestComponent>();

		// 階層構造とコンポーネントを設定
		parent->addComponent(parentComponent);
		child->addComponent(childComponent);
		grandchild->addComponent(grandchildComponent);
		
		parent->addChild(child);
		child->addChild(grandchild);
		canvas->addChild(parent);

		// 初期状態：全てactiveでonActivatedが呼ばれている
		REQUIRE(parentComponent->activatedCount == 1);
		REQUIRE(childComponent->activatedCount == 1);
		REQUIRE(grandchildComponent->activatedCount == 1);
		REQUIRE(parentComponent->deactivatedCount == 0);
		REQUIRE(childComponent->deactivatedCount == 0);
		REQUIRE(grandchildComponent->deactivatedCount == 0);

		// 親ノードを非アクティブ化：子孫も含めてonDeactivatedが呼ばれる
		parent->setActive(noco::ActiveYN::No);
		REQUIRE(parentComponent->activatedCount == 1);
		REQUIRE(childComponent->activatedCount == 1);
		REQUIRE(grandchildComponent->activatedCount == 1);
		REQUIRE(parentComponent->deactivatedCount == 1);
		REQUIRE(childComponent->deactivatedCount == 1);
		REQUIRE(grandchildComponent->deactivatedCount == 1);
		
		// onDeactivatedが正しいノードで呼ばれていることを確認
		REQUIRE(parentComponent->lastDeactivatedNode == parent);
		REQUIRE(childComponent->lastDeactivatedNode == child);
		REQUIRE(grandchildComponent->lastDeactivatedNode == grandchild);

		// 親ノードを再アクティブ化：子孫も含めてonActivatedが再度呼ばれる
		parent->setActive(noco::ActiveYN::Yes);
		REQUIRE(parentComponent->activatedCount == 2);
		REQUIRE(childComponent->activatedCount == 2);
		REQUIRE(grandchildComponent->activatedCount == 2);
		REQUIRE(parentComponent->deactivatedCount == 1);
		REQUIRE(childComponent->deactivatedCount == 1);
		REQUIRE(grandchildComponent->deactivatedCount == 1);
		
		// onActivatedが正しいノードで呼ばれていることを確認
		REQUIRE(parentComponent->lastActivatedNode == parent);
		REQUIRE(childComponent->lastActivatedNode == child);
		REQUIRE(grandchildComponent->lastActivatedNode == grandchild);
	}

	SECTION("Middle node setActive affects only descendants")
	{
		class TestComponent : public noco::ComponentBase, public std::enable_shared_from_this<TestComponent>
		{
		public:
			int32 activatedCount = 0;
			int32 deactivatedCount = 0;

			TestComponent() : ComponentBase{ {} } {}

			void onActivated(const std::shared_ptr<noco::Node>& node) override
			{
				activatedCount++;
			}

			void onDeactivated(const std::shared_ptr<noco::Node>& node) override
			{
				deactivatedCount++;
			}
		};

		auto canvas = noco::Canvas::Create(SizeF{ 800, 600 });
		auto parent = noco::Node::Create(U"Parent");
		auto child = noco::Node::Create(U"Child");
		auto grandchild = noco::Node::Create(U"Grandchild");
		
		auto parentComponent = std::make_shared<TestComponent>();
		auto childComponent = std::make_shared<TestComponent>();
		auto grandchildComponent = std::make_shared<TestComponent>();

		// 階層構造とコンポーネントを設定
		parent->addComponent(parentComponent);
		child->addComponent(childComponent);
		grandchild->addComponent(grandchildComponent);
		
		parent->addChild(child);
		child->addChild(grandchild);
		canvas->addChild(parent);

		// 初期状態確認
		REQUIRE(parentComponent->activatedCount == 1);
		REQUIRE(childComponent->activatedCount == 1);
		REQUIRE(grandchildComponent->activatedCount == 1);

		// 中間ノード（child）を非アクティブ化：childとgrandchildのみ影響
		child->setActive(noco::ActiveYN::No);
		REQUIRE(parentComponent->activatedCount == 1);
		REQUIRE(childComponent->activatedCount == 1);
		REQUIRE(grandchildComponent->activatedCount == 1);
		REQUIRE(parentComponent->deactivatedCount == 0);  // 親は影響なし
		REQUIRE(childComponent->deactivatedCount == 1);   // 子は非アクティブ
		REQUIRE(grandchildComponent->deactivatedCount == 1); // 孫も非アクティブ

		// 中間ノード（child）を再アクティブ化：childとgrandchildが再度アクティブ
		child->setActive(noco::ActiveYN::Yes);
		REQUIRE(parentComponent->activatedCount == 1);
		REQUIRE(childComponent->activatedCount == 2);     // 子が再アクティブ
		REQUIRE(grandchildComponent->activatedCount == 2); // 孫も再アクティブ
		REQUIRE(parentComponent->deactivatedCount == 0);  // 親は変化なし
		REQUIRE(childComponent->deactivatedCount == 1);
		REQUIRE(grandchildComponent->deactivatedCount == 1);
	}
}

TEST_CASE("Component type checking from JSON", "[Component]")
{
	SECTION("Label component type mismatch resets to zero")
	{
		// fontSizeが文字列の場合
		JSON json;
		json[U"type"] = U"Label";
		json[U"text"] = U"Test Label";
		json[U"fontSize"] = U"14";  // 文字列（数値であるべき）
		
		auto label = std::make_shared<noco::Label>(U"Initial Text");
		REQUIRE(label->fontSize().defaultValue == 24.0);
		
		bool result = label->tryReadFromJSON(json);
		REQUIRE(result == true);  // JSONの読み込み自体は成功
		REQUIRE(label->text().defaultValue == U"Test Label");  // textは正しく読み込まれる
		// fontSizeが文字列で与えられた場合、T{}（0.0）で初期化される
		REQUIRE(label->fontSize().defaultValue == 0.0);
	}
	
	SECTION("RectRenderer component color as number resets to zero")
	{
		// fillColorが数値の場合
		JSON json;
		json[U"type"] = U"RectRenderer";
		json[U"fillColor"] = 123;  // 数値（文字列形式のColorFであるべき）
		
		auto rect = std::make_shared<noco::RectRenderer>(Palette::White);
		REQUIRE(rect->fillColor().defaultValue.r == 1.0f);
		REQUIRE(rect->fillColor().defaultValue.g == 1.0f);
		REQUIRE(rect->fillColor().defaultValue.b == 1.0f);
		REQUIRE(rect->fillColor().defaultValue.a == 1.0f);
		
		rect->tryReadFromJSON(json);
		// fillColorが数値で与えられた場合、ColorF{}（0,0,0,0）で初期化される
		REQUIRE(rect->fillColor().defaultValue.r == 0.0f);
		REQUIRE(rect->fillColor().defaultValue.g == 0.0f);
		REQUIRE(rect->fillColor().defaultValue.b == 0.0f);
		REQUIRE(rect->fillColor().defaultValue.a == 0.0f);
	}
}
