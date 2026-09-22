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
		void Update(Coordinator& coord, DUPLEX_NS_GRAPHICS::Renderer& renderer)
		{
			for (auto const& entity : entities)
			{
				auto& transform = coord.GetComponent<Transform>(entity);
				auto& mesh = coord.GetComponent<MeshRenderer>(entity);
				DUPLEX_NS_MATH::Mat4x4 mat = DUPLEX_NS_MATH::Mat4x4::FromTranslation(transform.position) * DUPLEX_NS_MATH::Mat4x4::FromOrientation(transform.rotation) * DUPLEX_NS_MATH::Mat4x4::FromScale(transform.scale);
				for (ui32 i = 0;i<mesh.mesh.subMeshes.size();i++)
				{
					if (mesh.materials.size() > i)
					{
						renderer.UseTexture(0, mesh.materials[i].albedo.GetShaderResourceView());
						renderer.UseTexture(1, mesh.materials[i].metallic.GetShaderResourceView());
						renderer.UseTexture(1, mesh.materials[i].ambientOcclusion.GetShaderResourceView());
						renderer.UseTexture(3, mesh.materials[i].roughness.GetShaderResourceView());
						renderer.UseTexture(4, mesh.materials[i].normal.GetShaderResourceView());
						renderer.UseTexture(5, mesh.materials[i].emission.GetShaderResourceView());
					}
					renderer.Render(mat, mesh.vertexBuffers[i], mesh.indexBuffers[i], mesh.indexCounts[i]);
				}
			}
		}
		static ui32 GetTypeID()
		{
			return 1;
		}
	};
}
