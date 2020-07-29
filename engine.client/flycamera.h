#pragma once

// EXTERNAL INCLUDES
// INTERNAL INCLUDES
#include "gameobject.h"
#include "math/vec2.h"

namespace Engine::Components
{
	class FlyCamera : public Component
	{
	public:
		static ComponentType baseType;

	private:
		Engine::Math::Vec2 oldMousePosition = Engine::Math::Vec2::Zero;
		real speed = 2.f;
		real rotX = 0, rotY = 0;
	protected:
		void Update();
		FlyCamera(GameObject* attach);
		friend class GameObject;
	};
}