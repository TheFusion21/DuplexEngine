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
				for (ui32 i = 0;i<mesh.mesh.subMeshes.size();i++)
				{
					if (mesh.materials.size() > i)
					{
						Engine::Graphics::Renderer::GetInstancePtr()->UseTexture(0, mesh.materials[i].albedo.GetNativeTexturePtr());
						Engine::Graphics::Renderer::GetInstancePtr()->UseTexture(1, mesh.materials[i].metallic.GetNativeTexturePtr());
						Engine::Graphics::Renderer::GetInstancePtr()->UseTexture(1, mesh.materials[i].ambientOcclusion.GetNativeTexturePtr());
						Engine::Graphics::Renderer::GetInstancePtr()->UseTexture(3, mesh.materials[i].roughness.GetNativeTexturePtr());
						Engine::Graphics::Renderer::GetInstancePtr()->UseTexture(4, mesh.materials[i].normal.GetNativeTexturePtr());
						Engine::Graphics::Renderer::GetInstancePtr()->UseTexture(5, mesh.materials[i].emission.GetNativeTexturePtr());
					}
					Engine::Graphics::Renderer::GetInstancePtr()->Render(mat, mesh.vertexBuffers[i], mesh.indexBuffers[i], mesh.indexCounts[i]);
				}
			}
		}
		static ui32 GetTypeID()
		{
			return 1;
		}
	};
}