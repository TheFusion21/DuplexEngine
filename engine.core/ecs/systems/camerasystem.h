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
				DUPLEX_NS_GRAPHICS::Renderer::GetInstancePtr()->SetActiveCamera(transform.position, DUPLEX_NS_MATH::Mat4x4::FromPerspectiveFOV(camera.h_fov, 1.7777, camera.nearPlane, camera.farPlane) * DUPLEX_NS_MATH::Mat4x4::FromOrientation(transform.rotation) * DUPLEX_NS_MATH::Mat4x4::FromView(transform.position));
				//transform.position += DUPLEX_NS_MATH::Vec3::UnitX * Engine::Utils::Time::deltaTime;
			}
		}
		static ui32 GetTypeID()
		{
			return 0;
		}
	};
}