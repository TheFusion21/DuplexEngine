#pragma once
#include "math/vec3.h"
#include "math/quaternion.h"
#include "math/mat4x4.h"
namespace Engine::ECS
{
	class Camera
	{
	public:
		void SetFov(real h_fov)
		{
			this->h_fov = h_fov;
		}
		void SetPlanes(real nearPlane, real farPlane)
		{
			this->nearPlane = nearPlane;
			this->farPlane = farPlane;
		}
		static ui32 GetTypeID()
		{
			return 1;
		}
	private:
		real h_fov = static_cast<real>(75.0);
		real nearPlane = static_cast<real>(0.0001);
		real farPlane = static_cast<real>(1000.0);
		friend class CameraSystem;
	};
}