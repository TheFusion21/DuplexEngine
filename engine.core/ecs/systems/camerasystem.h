#pragma once
#include <entt/entt.hpp>
#include "../components/transform.h"
#include "../components/camera.h"
#include "renderer.h"
namespace Engine::ECS
{
	class CameraSystem
	{
	public:
		static void Update(entt::registry& registry, DUPLEX_NS_GRAPHICS::Renderer& renderer)
		{
			for (auto&& [entity, transform, camera] : registry.view<Transform, Camera>().each())
			{
				// FromView(pos) in the old math library ignored rotation entirely (its "axes"
				// were always the fixed world basis) - it was just Translate(-pos). Preserved
				// exactly as glm::translate(mat4(1), -pos) rather than carried forward as a
				// named helper, since that behavior was never actually about a camera-relative
				// view space.
				DUPLEX_NS_MATH::Mat4x4 perspective = glm::perspective(glm::radians(camera.h_fov), 1.7777f, camera.nearPlane, camera.farPlane);
				DUPLEX_NS_MATH::Mat4x4 orientation = glm::mat4_cast(transform.rotation);
				DUPLEX_NS_MATH::Mat4x4 view = glm::translate(DUPLEX_NS_MATH::Mat4x4(1.0f), -transform.position);
				renderer.SetActiveCamera(transform.position, perspective * orientation * view);
				//transform.position += DUPLEX_NS_MATH::Vec3UnitX * Engine::Utils::Time::deltaTime;
			}
		}
	};
}
