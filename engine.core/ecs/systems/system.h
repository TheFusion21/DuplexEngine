#pragma once
#include <set>
#include "../entities/entity.h"

namespace Engine::ECS
{
	class Coordinator;
	class System
	{
	public:
		virtual void Update(Coordinator& coord) = 0;
		std::set<Entity> entities;
	};
}