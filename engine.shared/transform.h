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
		DUPLEX_NS_MATH::Vec3 position;
		DUPLEX_NS_MATH::Quaternion rotation;
		DUPLEX_NS_MATH::Vec3 scale;
	protected:
		Transform(GameObject* attach);
		friend class GameObject;
	};
}