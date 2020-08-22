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
		void Update(Coordinator& coord)
		{
			Engine::Graphics::Renderer::GetInstancePtr()->ClearLights();
			for (auto const& entity : entities)
			{
				auto& transform = coord.GetComponent<Transform>(entity);
				auto& light = coord.GetComponent<Engine::Graphics::Light>(entity);

				Engine::Utils::GpuLight gpuLight;
				gpuLight.type = static_cast<ui32>(light.type);
				gpuLight.color = { light.color.r, light.color.g, light.color.b };
				gpuLight.intensity = light.intensity;
				gpuLight.indirectMul = light.indirectMultiplier;
				gpuLight.angularDiameter = light.angularDiameter;
				gpuLight.outerAngle = light.outerAngle;
				gpuLight.innerAngle = light.innerAnglePercent;
				gpuLight.radius = light.radius;
				gpuLight.range = light.range;
				if(light.type == Engine::Graphics::Light::LightType::Directional)
					gpuLight.transform = Engine::Math::Mat4x4::FromOrientation(transform.rotation);
				else
					gpuLight.transform = Engine::Math::Mat4x4::FromTranslation(transform.position) * Engine::Math::Mat4x4::FromOrientation(transform.rotation);
				gpuLight.position = transform.position;
				Engine::Graphics::Renderer::GetInstancePtr()->SetLight(gpuLight);
			}
		}
		static ui32 GetTypeID()
		{
			return 2;
		}
	};
}