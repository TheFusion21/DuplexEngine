#pragma once
#include <set>
#include "../entities/entity.h"

namespace Engine::ECS
{
	class Coordinator;
	// Update() isn't declared here: each concrete system defines its own with whatever
	// parameters it actually needs (e.g. CameraSystem::Update(Coordinator&, Renderer&)).
	// Nothing calls Update() polymorphically through a System* - SystemManager only ever
	// touches the base type for entities bookkeeping (EntityDestroyed/EntitySignatureChanged),
	// and client.cpp calls each system's Update() through its concrete shared_ptr<T>.
	class System
	{
	public:
		std::set<Entity> entities;
	};
}