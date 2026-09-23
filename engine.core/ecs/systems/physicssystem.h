#pragma once
#include <entt/entt.hpp>
#include "../components/transform.h"
#include "../components/rigidbody.h"
#include "physics/physicsworld.h"

namespace Engine::ECS
{
	class PhysicsSystem
	{
	public:
		// Steps `world` (internally at its own fixed timestep - see PhysicsWorld::Update) using
		// this frame's real delta time, then syncs every RigidBody entity's Transform from the
		// simulation. Physics owns position/rotation for these entities from this point on -
		// application code shouldn't also be writing transform.position/rotation for the same
		// entity, the same way nothing else writes to a Transform that MeshSystem is reading a
		// world matrix from mid-frame.
		static void Update(entt::registry& registry, DUPLEX_NS_PHYSICS::PhysicsWorld& world, float realDeltaTime)
		{
			world.Update(realDeltaTime);

			for (auto&& [entity, transform, rigidBody] : registry.view<Transform, RigidBody>().each())
			{
				transform.position = world.GetPosition(rigidBody.handle);
				transform.rotation = world.GetRotation(rigidBody.handle);
			}
		}
	};
}
