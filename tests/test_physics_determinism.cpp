// The single most important thing to verify before Phase 9's physics can be trusted: Jolt's
// own documentation confirms CROSS_PLATFORM_DETERMINISTIC gives bit-exact results across
// platforms/compilers/architectures, and confirms determinism given the same sequence of API
// calls - but doesn't explicitly state whether results are independent of *worker thread count*
// specifically (JobSystemThreadPool splits work across N threads; if the physics engine's
// summation/reduction order depended on which thread finished a job first, running the same
// scenario with 1 thread vs. many could silently diverge even with CROSS_PLATFORM_DETERMINISTIC
// on). This runs an identical, non-trivial scenario (a stack of boxes dropped onto a floor,
// enough contacts/constraints to actually exercise the parallelized narrowphase/solver, not
// just one free-falling sphere) once with a single worker thread and once with many, and diffs
// every dynamic body's final position/rotation bit-for-bit.
//
// Standalone - no EnTT/engine dependency, just Jolt directly, so this can run (and be re-run
// independently) without anything else in the engine being wired up.

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

#include <cstdio>
#include <cstring>
#include <vector>
#include <thread>
#include <algorithm>

using namespace JPH;
using namespace JPH::literals;

namespace
{
	constexpr ObjectLayer LAYER_NON_MOVING = 0;
	constexpr ObjectLayer LAYER_MOVING = 1;
	constexpr uint LAYER_COUNT = 2;

	class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter
	{
	public:
		bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override
		{
			if (inObject1 == LAYER_NON_MOVING)
				return inObject2 == LAYER_MOVING;
			return true;
		}
	};

	namespace BroadPhaseLayers
	{
		static constexpr BroadPhaseLayer NON_MOVING(0);
		static constexpr BroadPhaseLayer MOVING(1);
		static constexpr uint NUM_LAYERS(2);
	}

	class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface
	{
	public:
		BPLayerInterfaceImpl()
		{
			mObjectToBroadPhase[LAYER_NON_MOVING] = BroadPhaseLayers::NON_MOVING;
			mObjectToBroadPhase[LAYER_MOVING] = BroadPhaseLayers::MOVING;
		}
		uint GetNumBroadPhaseLayers() const override { return BroadPhaseLayers::NUM_LAYERS; }
		BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override
		{
			return mObjectToBroadPhase[inLayer];
		}
		// Only declared as pure virtual in the base class when profiling is compiled in - see
		// Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h. Whether JPH_PROFILE_ENABLED is
		// defined turned out to depend on build config (an incremental build had it, a
		// from-scratch reconfigure didn't), so this has to match the same guard rather than
		// always providing an `override`.
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
		const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override
		{
			switch ((BroadPhaseLayer::Type)inLayer)
			{
			case (BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING: return "NON_MOVING";
			case (BroadPhaseLayer::Type)BroadPhaseLayers::MOVING: return "MOVING";
			default: return "INVALID";
			}
		}
#endif
	private:
		BroadPhaseLayer mObjectToBroadPhase[LAYER_COUNT];
	};

	class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter
	{
	public:
		bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override
		{
			if (inLayer1 == LAYER_NON_MOVING)
				return inLayer2 == BroadPhaseLayers::MOVING;
			return true;
		}
	};

	struct BodySnapshot
	{
		RVec3 position;
		Quat rotation;
	};

	// Drops a 4x4x4 grid of boxes (enough simultaneous contacts to actually exercise the
	// parallelized solver, unlike a single free-falling sphere) onto a static floor, steps the
	// simulation a fixed number of times using a JobSystemThreadPool with `threadCount` worker
	// threads, and returns each dynamic body's final transform in a fixed, thread-independent
	// order (sorted by BodyID, not by whichever order they happen to be stored in - iteration
	// order itself could otherwise mask or fake a thread-count dependency).
	std::vector<BodySnapshot> RunScenario(uint threadCount)
	{
		const uint cMaxBodies = 256;
		const uint cNumBodyMutexes = 0;
		const uint cMaxBodyPairs = 1024;
		const uint cMaxContactConstraints = 1024;

		BPLayerInterfaceImpl broadPhaseLayerInterface;
		ObjectVsBroadPhaseLayerFilterImpl objectVsBroadPhaseLayerFilter;
		ObjectLayerPairFilterImpl objectVsObjectLayerFilter;

		PhysicsSystem physicsSystem;
		physicsSystem.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints,
			broadPhaseLayerInterface, objectVsBroadPhaseLayerFilter, objectVsObjectLayerFilter);

		TempAllocatorImpl tempAllocator(10 * 1024 * 1024);
		JobSystemThreadPool jobSystem(cMaxPhysicsJobs, cMaxPhysicsBarriers, static_cast<int>(threadCount));

		BodyInterface& bodyInterface = physicsSystem.GetBodyInterface();

		BoxShapeSettings floorShapeSettings(Vec3(50.0f, 1.0f, 50.0f));
		floorShapeSettings.SetEmbedded();
		ShapeRefC floorShape = floorShapeSettings.Create().Get();
		BodyCreationSettings floorSettings(floorShape, RVec3(0.0_r, -1.0_r, 0.0_r), Quat::sIdentity(), EMotionType::Static, LAYER_NON_MOVING);
		Body* floor = bodyInterface.CreateBody(floorSettings);
		bodyInterface.AddBody(floor->GetID(), EActivation::DontActivate);

		std::vector<BodyID> boxIds;
		const int gridSize = 4;
		for (int x = 0; x < gridSize; x++)
		{
			for (int y = 0; y < gridSize; y++)
			{
				for (int z = 0; z < gridSize; z++)
				{
					// Slightly irregular spacing/offsets so boxes tumble and collide with each
					// other on the way down, not just fall straight and land independently -
					// that's what actually exercises the contact solver's ordering.
					RVec3 pos(
						static_cast<Real>(x) * 1.05_r - 1.5_r,
						static_cast<Real>(y) * 1.05_r + 5.0_r,
						static_cast<Real>(z) * 1.05_r - 1.5_r + (x % 2 == 0 ? 0.3_r : -0.3_r));
					BodyCreationSettings boxSettings(new BoxShape(Vec3(0.5f, 0.5f, 0.5f)), pos, Quat::sIdentity(), EMotionType::Dynamic, LAYER_MOVING);
					BodyID id = bodyInterface.CreateAndAddBody(boxSettings, EActivation::Activate);
					boxIds.push_back(id);
				}
			}
		}

		physicsSystem.OptimizeBroadPhase();

		const float fixedDeltaTime = 1.0f / 60.0f;
		const int stepsToSimulate = 180; // 3 seconds - long enough for the stack to settle
		for (int step = 0; step < stepsToSimulate; step++)
		{
			physicsSystem.Update(fixedDeltaTime, 1, &tempAllocator, &jobSystem);
		}

		// Sort by BodyID's index so the returned order is identical regardless of which thread
		// count produced it - only the physics RESULTS should be compared, not incidental
		// container/creation ordering.
		std::sort(boxIds.begin(), boxIds.end(), [](const BodyID& a, const BodyID& b) {
			return a.GetIndex() < b.GetIndex();
		});

		std::vector<BodySnapshot> result;
		result.reserve(boxIds.size());
		for (BodyID id : boxIds)
		{
			result.push_back({ bodyInterface.GetPosition(id), bodyInterface.GetRotation(id) });
		}

		bodyInterface.RemoveBody(floor->GetID());
		bodyInterface.DestroyBody(floor->GetID());
		for (BodyID id : boxIds)
		{
			bodyInterface.RemoveBody(id);
			bodyInterface.DestroyBody(id);
		}

		return result;
	}
}

int main()
{
	RegisterDefaultAllocator();
	Factory::sInstance = new Factory();
	RegisterTypes();

	int failures = 0;

	uint hwThreads = std::thread::hardware_concurrency();
	uint manyThreads = hwThreads > 1 ? hwThreads - 1 : 1; // matches JobSystemThreadPool's own "leave one for the main thread" convention (see HelloWorld sample)

	std::printf("test_physics_determinism: running with 1 worker thread...\n");
	std::vector<BodySnapshot> singleThreaded = RunScenario(1);

	std::printf("test_physics_determinism: running with %u worker thread(s)...\n", manyThreads);
	std::vector<BodySnapshot> multiThreaded = RunScenario(manyThreads);

	if (singleThreaded.size() != multiThreaded.size())
	{
		std::fprintf(stderr, "FAIL: body count differs between runs (%zu vs %zu)\n", singleThreaded.size(), multiThreaded.size());
		failures++;
	}
	else
	{
		for (size_t i = 0; i < singleThreaded.size(); i++)
		{
			const BodySnapshot& a = singleThreaded[i];
			const BodySnapshot& b = multiThreaded[i];
			bool posMatches = std::memcmp(&a.position, &b.position, sizeof(RVec3)) == 0;
			bool rotMatches = std::memcmp(&a.rotation, &b.rotation, sizeof(Quat)) == 0;
			if (!posMatches || !rotMatches)
			{
				std::fprintf(stderr, "FAIL: body %zu diverged between 1 thread and %u threads: pos (%f,%f,%f) vs (%f,%f,%f), rot (%f,%f,%f,%f) vs (%f,%f,%f,%f)\n",
					i, manyThreads,
					static_cast<double>(a.position.GetX()), static_cast<double>(a.position.GetY()), static_cast<double>(a.position.GetZ()),
					static_cast<double>(b.position.GetX()), static_cast<double>(b.position.GetY()), static_cast<double>(b.position.GetZ()),
					static_cast<double>(a.rotation.GetX()), static_cast<double>(a.rotation.GetY()), static_cast<double>(a.rotation.GetZ()), static_cast<double>(a.rotation.GetW()),
					static_cast<double>(b.rotation.GetX()), static_cast<double>(b.rotation.GetY()), static_cast<double>(b.rotation.GetZ()), static_cast<double>(b.rotation.GetW()));
				failures++;
			}
		}
	}

	UnregisterTypes();
	delete Factory::sInstance;
	Factory::sInstance = nullptr;

	if (failures == 0)
	{
		std::printf("test_physics_determinism: all %zu bodies bit-exact between 1 and %u threads\n", singleThreaded.size(), manyThreads);
		return 0;
	}
	std::fprintf(stderr, "test_physics_determinism: %d divergence(s) found\n", failures);
	return 1;
}
