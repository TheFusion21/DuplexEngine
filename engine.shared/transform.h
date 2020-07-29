#pragma once

// EXTERNAL INCLUDES
// INTERNAL INCLUDES
#include "math/vec3.h"
#include "math/quaternion.h"
#include "component.h"

namespace Engine::Components
{
	class Transform : public Component
	{
	public:
		static ComponentType baseType;
		Engine::Math::Vec3 position;
		Engine::Math::Quaternion rotation;
		Engine::Math::Vec3 scale;
	protected:
		Transform(GameObject* attach);
		friend class GameObject;
	};
}