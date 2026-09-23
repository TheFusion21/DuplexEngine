#pragma once
#include "math/vec3.h"
#include "math/quaternion.h"
namespace Engine::ECS
{
	class Transform
	{
	public:
		bool isStatic = false;
		DUPLEX_NS_MATH::Vec3 position = DUPLEX_NS_MATH::Vec3Zero;
		DUPLEX_NS_MATH::Quaternion rotation = DUPLEX_NS_MATH::QuatIdentity;
		DUPLEX_NS_MATH::Vec3 scale = DUPLEX_NS_MATH::Vec3UnitScale;
	};
}