#include "entity.h"

using namespace Engine::Ecs;

unsigned int Entity::lastID = 0;

Engine::Ecs::Entity::Entity() : id(lastID++)
{

}
