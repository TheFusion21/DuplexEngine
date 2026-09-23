#pragma once
#include "physics/physicsworld.h"

namespace Engine::ECS
{
	// Just holds a handle into a PhysicsWorld - Jolt owns the actual position/rotation/
	// velocity state for this entity once created (see PhysicsSystem::Update, which copies it
	// into Transform every fixed step). Created via PhysicsWorld::CreateBoxBody (there's only
	// a box shape helper today - the physics wrapper can grow more shape types as something
	// actually needs them).
	struct RigidBody
	{
		DUPLEX_NS_PHYSICS::BodyHandle handle;
	};
}
