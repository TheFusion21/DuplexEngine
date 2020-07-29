#pragma once

// EXTERNAL INCLUDES
// INTERNAL INCLUDES
#include "gameobject.h"
#include "math/mat4x4.h"

namespace Engine::Components
{
	class Camera : public Component
	{
	public:
		static ComponentType baseType;
		void SetFov(real h_fov);
		void SetAspect(real aspect);
		void SetPlanes(real nearPlane, real farPlane);
		static Camera* activeCamera;
		const Engine::Math::Mat4x4& GetViewProjMatrix() { return viewProjMatrix; }
	private:
		Engine::Math::Mat4x4 viewProjMatrix;
		real h_fov;
		real aspect;
		real nearPlane;
		real farPlane;
	protected:
		void Update();
		Camera(GameObject* attach);
		friend class GameObject;
	};
}