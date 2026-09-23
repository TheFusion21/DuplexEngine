#pragma once
#include <entt/entt.hpp>
#include "../components/transform.h"
#include "../components/meshrenderer.h"
#include "renderer.h"
#include "simd/transformbatch.h"
#include <vector>
namespace Engine::ECS
{
	class MeshSystem
	{
	public:
		static void Update(entt::registry& registry, DUPLEX_NS_GRAPHICS::Renderer& renderer)
		{
			// Gathered into contiguous arrays so world matrix composition can go through
			// ComputeWorldMatricesBatch (see simd/transformbatch.h) - AVX2+FMA-accelerated
			// when the running CPU supports it, otherwise the same per-entity composition this
			// loop used to do inline.
			std::vector<DUPLEX_NS_MATH::Vec3> positions;
			std::vector<DUPLEX_NS_MATH::Quaternion> rotations;
			std::vector<DUPLEX_NS_MATH::Vec3> scales;
			std::vector<MeshRenderer*> orderedMeshes;
			auto view = registry.view<Transform, MeshRenderer>();
			positions.reserve(view.size_hint());
			rotations.reserve(view.size_hint());
			scales.reserve(view.size_hint());
			orderedMeshes.reserve(view.size_hint());
			for (auto&& [entity, transform, mesh] : view.each())
			{
				positions.push_back(transform.position);
				rotations.push_back(transform.rotation);
				scales.push_back(transform.scale);
				orderedMeshes.push_back(&mesh);
			}

			std::vector<DUPLEX_NS_MATH::Mat4x4> worldMatrices(orderedMeshes.size());
			DUPLEX_NS_SIMD::ComputeWorldMatricesBatch(positions.data(), rotations.data(), scales.data(), worldMatrices.data(), static_cast<unsigned int>(orderedMeshes.size()));

			for (size_t entityIndex = 0; entityIndex < orderedMeshes.size(); entityIndex++)
			{
				auto& mesh = *orderedMeshes[entityIndex];
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
	};
}
