#include "physicsworld.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

#include <thread>
#include <cstdio>

using namespace DUPLEX_NS_PHYSICS;
using namespace DUPLEX_NS_MATH;

namespace
{
	constexpr JPH::ObjectLayer LAYER_NON_MOVING = 0;
	constexpr JPH::ObjectLayer LAYER_MOVING = 1;
	constexpr JPH::uint LAYER_COUNT = 2;

	class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
	{
	public:
		bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
		{
			if (inObject1 == LAYER_NON_MOVING)
				return inObject2 == LAYER_MOVING;
			return true;
		}
	};

	namespace BroadPhaseLayers
	{
		static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
		static constexpr JPH::BroadPhaseLayer MOVING(1);
		static constexpr JPH::uint NUM_LAYERS(2);
	}

	class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
	{
	public:
		BPLayerInterfaceImpl()
		{
			mObjectToBroadPhase[LAYER_NON_MOVING] = BroadPhaseLayers::NON_MOVING;
			mObjectToBroadPhase[LAYER_MOVING] = BroadPhaseLayers::MOVING;
		}
		JPH::uint GetNumBroadPhaseLayers() const override { return BroadPhaseLayers::NUM_LAYERS; }
		JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
		{
			return mObjectToBroadPhase[inLayer];
		}
		// Only declared as pure virtual in the base class when profiling is compiled in - see
		// Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h. Matching that guard here (rather
		// than always providing an `override`) is required, not cosmetic: whether this build
		// defines JPH_PROFILE_ENABLED turned out to depend on build config in ways that bit us
		// once already (an incremental build had it, a from-scratch reconfigure didn't).
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
		const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
		{
			switch ((JPH::BroadPhaseLayer::Type)inLayer)
			{
			case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING: return "NON_MOVING";
			case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING: return "MOVING";
			default: return "INVALID";
			}
		}
#endif
	private:
		JPH::BroadPhaseLayer mObjectToBroadPhase[LAYER_COUNT];
	};

	class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
	{
	public:
		bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
		{
			if (inLayer1 == LAYER_NON_MOVING)
				return inLayer2 == BroadPhaseLayers::MOVING;
			return true;
		}
	};

	// Global Jolt state (Factory/type registration) is process-wide, not per-PhysicsWorld -
	// guarded so creating more than one PhysicsWorld (not that anything does today) doesn't
	// double-register or tear down state a sibling instance still needs.
	int g_joltRefCount = 0;

	JPH::Vec3 ToJolt(Vec3 v) { return JPH::Vec3(v.x, v.y, v.z); }
	Vec3 FromJolt(JPH::Vec3 v) { return Vec3(v.GetX(), v.GetY(), v.GetZ()); }
	JPH::Quat ToJolt(Quaternion q) { return JPH::Quat(q.x, q.y, q.z, q.w); }
	Quaternion FromJolt(JPH::Quat q) { return Quaternion(q.GetW(), q.GetX(), q.GetY(), q.GetZ()); }
}

struct PhysicsWorld::Impl
{
	BPLayerInterfaceImpl broadPhaseLayerInterface;
	ObjectVsBroadPhaseLayerFilterImpl objectVsBroadPhaseLayerFilter;
	ObjectLayerPairFilterImpl objectVsObjectLayerFilter;

	JPH::PhysicsSystem physicsSystem;
	// 10 MB matches Jolt's own HelloWorld sample - pre-allocated so the physics update itself
	// doesn't need to allocate.
	JPH::TempAllocatorImpl tempAllocator{ 10 * 1024 * 1024 };
	// The explicit ask for this phase: Jolt's own internal job-system thread pool, not a
	// general-purpose engine job system. hardware_concurrency() - 1 leaves the main thread
	// free, matching Jolt's own sample convention; clamped so a single-core (or
	// hardware_concurrency()-can't-detect, which returns 0) machine still gets a valid count.
	JPH::JobSystemThreadPool jobSystem{
		JPH::cMaxPhysicsJobs,
		JPH::cMaxPhysicsBarriers,
		static_cast<int>(std::thread::hardware_concurrency() > 1 ? std::thread::hardware_concurrency() - 1 : 1)
	};

	float accumulator = 0.0f;
};

// A fixed step is what makes the simulation reproducible run to run - matches the 60Hz this
// engine's demo content was already tuned around (see the old FromPerspectiveFOV/BSDF math
// migration notes).
static constexpr float kFixedTimestep = 1.0f / 60.0f;
// Caps how many fixed steps a single Update() call will run - without this, a stalled frame
// (e.g. a breakpoint, a slow load) would make the next Update() try to "catch up" with a huge
// burst of steps, which can itself stall the next several frames even further (the classic
// "spiral of death"). Falling behind the target rate is preferable to that.
static constexpr int kMaxStepsPerUpdate = 4;

PhysicsWorld::PhysicsWorld() = default;
PhysicsWorld::~PhysicsWorld()
{
	Shutdown();
}

void PhysicsWorld::Init()
{
	if (g_joltRefCount == 0)
	{
		JPH::RegisterDefaultAllocator();
		JPH::Factory::sInstance = new JPH::Factory();
		JPH::RegisterTypes();
	}
	g_joltRefCount++;

	impl = std::make_unique<Impl>();

	const JPH::uint maxBodies = 4096;
	const JPH::uint numBodyMutexes = 0; // 0 = Jolt picks a default
	const JPH::uint maxBodyPairs = 4096;
	const JPH::uint maxContactConstraints = 4096;
	impl->physicsSystem.Init(maxBodies, numBodyMutexes, maxBodyPairs, maxContactConstraints,
		impl->broadPhaseLayerInterface, impl->objectVsBroadPhaseLayerFilter, impl->objectVsObjectLayerFilter);
	impl->physicsSystem.SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));
}

void PhysicsWorld::Shutdown()
{
	if (!impl)
		return;

	impl.reset();

	g_joltRefCount--;
	if (g_joltRefCount == 0)
	{
		JPH::UnregisterTypes();
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;
	}
}

void PhysicsWorld::Update(float realDeltaTime)
{
	impl->accumulator += realDeltaTime;

	int steps = 0;
	while (impl->accumulator >= kFixedTimestep && steps < kMaxStepsPerUpdate)
	{
		impl->physicsSystem.Update(kFixedTimestep, 1, &impl->tempAllocator, &impl->jobSystem);
		impl->accumulator -= kFixedTimestep;
		steps++;
	}
	if (steps == kMaxStepsPerUpdate)
	{
		// Fell behind - drop the rest rather than spiral. See kMaxStepsPerUpdate.
		impl->accumulator = 0.0f;
	}
}

BodyHandle PhysicsWorld::CreateBoxBody(Vec3 halfExtents, Vec3 position, Quaternion rotation, BodyMotionType motionType)
{
	JPH::EMotionType joltMotionType;
	JPH::ObjectLayer layer;
	switch (motionType)
	{
	case BodyMotionType::Dynamic:
		joltMotionType = JPH::EMotionType::Dynamic;
		layer = LAYER_MOVING;
		break;
	case BodyMotionType::Kinematic:
		joltMotionType = JPH::EMotionType::Kinematic;
		layer = LAYER_MOVING;
		break;
	case BodyMotionType::Static:
	default:
		joltMotionType = JPH::EMotionType::Static;
		layer = LAYER_NON_MOVING;
		break;
	}

	JPH::BodyCreationSettings settings(
		new JPH::BoxShape(ToJolt(halfExtents)),
		JPH::RVec3(position.x, position.y, position.z),
		ToJolt(rotation),
		joltMotionType,
		layer);

	JPH::BodyID id = impl->physicsSystem.GetBodyInterface().CreateAndAddBody(settings,
		motionType == BodyMotionType::Static ? JPH::EActivation::DontActivate : JPH::EActivation::Activate);

	BodyHandle handle;
	handle.id = id.GetIndexAndSequenceNumber();
	return handle;
}

void PhysicsWorld::DestroyBody(BodyHandle handle)
{
	if (!handle.IsValid())
		return;
	JPH::BodyID id(handle.id);
	JPH::BodyInterface& bodyInterface = impl->physicsSystem.GetBodyInterface();
	bodyInterface.RemoveBody(id);
	bodyInterface.DestroyBody(id);
}

Vec3 PhysicsWorld::GetPosition(BodyHandle handle) const
{
	if (!handle.IsValid())
		return Vec3Zero;
	return FromJolt(JPH::Vec3(impl->physicsSystem.GetBodyInterface().GetPosition(JPH::BodyID(handle.id))));
}

Quaternion PhysicsWorld::GetRotation(BodyHandle handle) const
{
	if (!handle.IsValid())
		return QuatIdentity;
	return FromJolt(impl->physicsSystem.GetBodyInterface().GetRotation(JPH::BodyID(handle.id)));
}
