#include "system.h"
#include "../components/transform.h"
#include "../components/meshrenderer.h"
#include "../coordinator.h"
#include "renderer.h"
namespace Engine::ECS
{
	class MeshSystem : public System
	{
	public:
		void Update(Coordinator& coord)
		{
			for (auto const& entity : entities)
			{
				auto& transform = coord.GetComponent<Transform>(entity);
				auto& mesh = coord.GetComponent<MeshRenderer>(entity);
				Engine::Math::Mat4x4 mat = Engine::Math::Mat4x4::FromTranslation(transform.position) * Engine::Math::Mat4x4::FromOrientation(transform.rotation) * Engine::Math::Mat4x4::FromScale(transform.scale);
				Engine::Graphics::Renderer::GetInstancePtr()->Render(mat, mesh.vertexBuffer, mesh.indexBuffer, mesh.indexCount, mesh.mat);
				//Engine::Graphics::Renderer::GetInstancePtr()->Render
				//transform.position += Engine::Math::Vec3::UnitX * Engine::Utils::Time::deltaTime;
			}
		}
		static ui32 GetTypeID()
		{
			return 1;
		}
	};
}