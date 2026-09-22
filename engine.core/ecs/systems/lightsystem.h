#include "system.h"
#include "../components/transform.h"
#include "../../graphics/light.h"
#include "../coordinator.h"
#include "renderer.h"
namespace Engine::ECS
{
	class LightSystem : public System
	{
	public:
		void Update(Coordinator& coord, DUPLEX_NS_GRAPHICS::Renderer& renderer)
		{
			renderer.ClearLights();
			for (auto const& entity : entities)
			{
				auto& transform = coord.GetComponent<Transform>(entity);
				auto& light = coord.GetComponent<DUPLEX_NS_GRAPHICS::Light>(entity);

				DUPLEX_NS_UTIL::GpuLight gpuLight;
				gpuLight.type = static_cast<ui32>(light.type);
				gpuLight.color = { light.color.r, light.color.g, light.color.b };
				gpuLight.intensity = light.intensity;
				gpuLight.indirectMul = light.indirectMultiplier;
				gpuLight.angularDiameter = light.angularDiameter;
				gpuLight.outerAngle = light.outerAngle;
				gpuLight.innerAngle = light.innerAnglePercent;
				gpuLight.radius = light.radius;
				gpuLight.range = light.range;
				if(light.type == DUPLEX_NS_GRAPHICS::Light::LightType::Directional)
					gpuLight.transform = glm::mat4_cast(transform.rotation);
				else
					gpuLight.transform = glm::translate(DUPLEX_NS_MATH::Mat4x4(1.0f), transform.position) * glm::mat4_cast(transform.rotation);
				gpuLight.position = transform.position;
				renderer.SetLight(gpuLight);
			}
		}
		static ui32 GetTypeID()
		{
			return 2;
		}
	};
}