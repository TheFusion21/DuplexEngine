
// EXTERNAL INCLUDES
// INTERNAL INCLUDES
#include "component.h"
using namespace Engine::Components;

ComponentType Component::baseType = ComponentType::Invalid;

Component::Component(GameObject* attach, ComponentType type) : gameObject(attach), type(type)
{

}