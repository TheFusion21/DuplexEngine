#include "system.h"
#include "../components/transform.h"
#include "../components/camera.h"
#include "../coordinator.h"
#include "renderer.h"
namespace Engine::ECS
{
	class CameraSystem : public System
	{
	public:
		void Update(Coordinator& coord)
		{
			for (auto const& entity : entities)
			{
				auto& transform = coord.GetComponent<Transform>(entity);
				auto& camera = coord.GetComponent<Camera>(entity);
				Engine::Graphics::Renderer::GetInstancePtr()->SetActiveCamera(transform.position, Engine::Math::Mat4x4::FromPerspectiveFOV(camera.h_fov, 1.7777, camera.nearPlane, camera.farPlane) * Engine::Math::Mat4x4::FromOrientation(transform.rotation) * Engine::Math::Mat4x4::FromView(transform.position));
				//transform.position += Engine::Math::Vec3::UnitX * Engine::Utils::Time::deltaTime;
			}
		}
		static ui32 GetTypeID()
		{
			return 0;
		}
	};
}