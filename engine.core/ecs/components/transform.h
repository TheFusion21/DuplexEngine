#pragma once
#include "math/vec3.h"
#include "math/quaternion.h"
namespace Engine::ECS
{
	class Transform
	{
	public:
		Engine::Math::Vec3 position = Engine::Math::Vec3::Zero;
		Engine::Math::Quaternion rotation = Engine::Math::Quaternion::Zero;
		Engine::Math::Vec3 scale = Engine::Math::Vec3::UnitScale;

		static ui32 GetTypeID()
		{
			return 0;
		}
	};
}