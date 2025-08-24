#include "NocoUI/ComponentFactory.hpp"
#include "NocoUI/Component/Component.hpp"

namespace noco
{
	ComponentFactory ComponentFactory::CreateWithBuiltinComponents()
	{
		ComponentFactory factory;
		
		factory.registerComponentType<Label>(U"Label");
		factory.registerComponentType<TextureFontLabel>(U"TextureFontLabel");
		factory.registerComponentType<Sprite>(U"Sprite");
		factory.registerComponentType<RectRenderer>(U"RectRenderer");
		factory.registerComponentType<TextBox>(U"TextBox");
		factory.registerComponentType<TextArea>(U"TextArea");
		factory.registerComponentType<EventTrigger>(U"EventTrigger");
		factory.registerComponentType<CursorChanger>(U"CursorChanger");
		factory.registerComponentType<UISound>(U"UISound");
		factory.registerComponentType<Tween>(U"Tween");
		
		return factory;
	}
	
	const ComponentFactory& ComponentFactory::GetBuiltinFactory()
	{
		static ComponentFactory builtinFactory = CreateWithBuiltinComponents();
		return builtinFactory;
	}
}