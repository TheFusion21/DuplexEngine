
// EXTERNAL INCLUDES
// INTERNAL INCLUDES
#include "gameobject.h"

using namespace Engine::Components;

GameObject::GameObject() : transform(AddComponent<Transform>())
{

}