#include "system.h"
#include "../components/transform.h"
#include "../components/meshrenderer.h"
#include "../coordinator.h"
#include "renderer.h"
#include "simd/transformbatch.h"
#include <vector>
namespace Engine::ECS
{
	class MeshSystem : public System
	{
	public:
		void Update(Coordinator& coord, DUPLEX_NS_GRAPHICS::Renderer& renderer)
		{
			// Gathered into contiguous arrays so world matrix composition can go through
			// ComputeWorldMatricesBatch (see simd/transformbatch.h) - AVX2+FMA-accelerated
			// when the running CPU supports it, otherwise the same per-entity composition this
			// loop used to do inline.
			std::vector<DUPLEX_NS_MATH::Vec3> positions;
			std::vector<DUPLEX_NS_MATH::Quaternion> rotations;
			std::vector<DUPLEX_NS_MATH::Vec3> scales;
			std::vector<Entity> orderedEntities;
			positions.reserve(entities.size());
			rotations.reserve(entities.size());
			scales.reserve(entities.size());
			orderedEntities.reserve(entities.size());
			for (auto const& entity : entities)
			{
				auto& transform = coord.GetComponent<Transform>(entity);
				positions.push_back(transform.position);
				rotations.push_back(transform.rotation);
				scales.push_back(transform.scale);
				orderedEntities.push_back(entity);
			}

			std::vector<DUPLEX_NS_MATH::Mat4x4> worldMatrices(orderedEntities.size());
			DUPLEX_NS_SIMD::ComputeWorldMatricesBatch(positions.data(), rotations.data(), scales.data(), worldMatrices.data(), static_cast<unsigned int>(orderedEntities.size()));

			for (size_t entityIndex = 0; entityIndex < orderedEntities.size(); entityIndex++)
			{
				auto& mesh = coord.GetComponent<MeshRenderer>(orderedEntities[entityIndex]);
				const DUPLEX_NS_MATH::Mat4x4& mat = worldMatrices[entityIndex];
				for (ui32 i = 0;i<mesh.mesh.subMeshes.size();i++)
				{
					if (mesh.materials.size() > i)
					{
						// Slots match the BSDF pixel shader's register(tN) slots (see bsdfPixel.hlsl):
						// t0 albedo, t1 metallic, t2 occlusion, t3 roughness, t4 normal, t5 emission.
						// ambientOcclusion previously also targeted slot 1, silently overwriting
						// metallic right after it was set - _Metallic.Sample() in the shader was
						// actually reading the AO texture. (t2/_Occlusion is declared in the shader
						// but never sampled, so this was invisible until the texture came out wrong.)
						renderer.UseTexture(0, mesh.materials[i].albedo.GetShaderResourceView());
						renderer.UseTexture(1, mesh.materials[i].metallic.GetShaderResourceView());
						renderer.UseTexture(2, mesh.materials[i].ambientOcclusion.GetShaderResourceView());
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
