#pragma once
// Thin wrapper around JPH::PhysicsSystem - keeps Jolt's headers out of anything that doesn't
// need them directly (PIMPL), same encapsulation philosophy as Renderer hiding native
// device/context handles. Built with CROSS_PLATFORM_DETERMINISTIC (see top-level CMakeLists.txt)
// and stepped at a fixed timestep internally (see Update()) - both required for the
// determinism this phase exists for; verified in tests/test_physics_determinism.cpp that this
// also holds across different JobSystemThreadPool thread counts, not just across platforms.
#include <memory>
#include "math/vec3.h"
#include "math/quaternion.h"
#include "namespaces.h"

namespace DUPLEX_NS_PHYSICS
{
	// Wraps JPH::BodyID (already a generation-checked index internally) without exposing any
	// Jolt type to callers that only need to hold/pass a handle around (e.g. the RigidBody
	// ECS component).
	struct BodyHandle
	{
		static constexpr unsigned int InvalidId = 0xffffffffu; // matches JPH::BodyID::cInvalidBodyID
		unsigned int id = InvalidId;

		bool IsValid() const { return id != InvalidId; }
	};

	enum class BodyMotionType
	{
		Static,
		Dynamic,
		Kinematic
	};

	class PhysicsWorld
	{
	public:
		PhysicsWorld();
		~PhysicsWorld();
		PhysicsWorld(const PhysicsWorld&) = delete;
		PhysicsWorld& operator=(const PhysicsWorld&) = delete;

		void Init();
		void Shutdown();

		// Accumulates realDeltaTime and steps the simulation zero or more times at a fixed
		// rate (see kFixedTimestep in the .cpp) - a fixed step is required for determinism,
		// unlike the variable-rate render loop this is called once per frame from.
		void Update(float realDeltaTime);

		BodyHandle CreateBoxBody(DUPLEX_NS_MATH::Vec3 halfExtents, DUPLEX_NS_MATH::Vec3 position, DUPLEX_NS_MATH::Quaternion rotation, BodyMotionType motionType);
		void DestroyBody(BodyHandle handle);

		DUPLEX_NS_MATH::Vec3 GetPosition(BodyHandle handle) const;
		DUPLEX_NS_MATH::Quaternion GetRotation(BodyHandle handle) const;

	private:
		struct Impl;
		std::unique_ptr<Impl> impl;
	};
}
