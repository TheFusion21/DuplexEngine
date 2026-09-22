#pragma once
// EXTERNAL INCLUDES
// INTERNAL INCLUDES
#include "component.h"
#include "math/types.h"
#include "mesh.h"
#include "graphics/stb_image.h"
#include "utils/util.h"
#include "renderer.h"
#include "graphics/bsdfmaterial.h"
namespace Engine::ECS
{
	class MeshRenderer
	{
	public:
		// Buffers are intentionally not released here: a destructor can't take the Renderer&
		// it would need, and there's currently no "entity destroyed" system pass that could
		// call SetMesh's release path with one. Same leak-on-destroy the previous (dead, this
		// was always commented out) body would have had either way - not something this pass
		// changes, just now stated instead of left in a stale comment.
		~MeshRenderer() = default;
		MeshRenderer(){}
		MeshRenderer(const MeshRenderer& copy) = default;
		MeshRenderer& operator=(const MeshRenderer& copy) = default;
		void SetMesh(DUPLEX_NS_GRAPHICS::Renderer& renderer, Engine::Resources::Mesh mesh)
		{
			this->mesh = mesh;
			for (DUPLEX_NS_GRAPHICS::BufferHandle& vb : vertexBuffers)
			{
				renderer.ReleaseBuffer(vb);
			}
			vertexBuffers.clear();
			for (DUPLEX_NS_GRAPHICS::BufferHandle& ib : indexBuffers)
			{
				renderer.ReleaseBuffer(ib);
			}
			indexBuffers.clear();
			indexCounts.clear();
			for (Engine::Resources::Mesh::SubMesh sm : this->mesh.subMeshes)
			{
				DUPLEX_NS_GRAPHICS::BufferHandle vb = renderer.CreateBuffer(DUPLEX_NS_GRAPHICS::BufferType::Vertex, sm.vertices.data(), static_cast<ui32>(sm.vertices.size()) * sizeof(Vertex));
				vertexBuffers.push_back(vb);
				DUPLEX_NS_GRAPHICS::BufferHandle ib = renderer.CreateBuffer(DUPLEX_NS_GRAPHICS::BufferType::Index, sm.indices.data(), static_cast<ui32>(sm.indices.size()) * sizeof(ui32));
				indexBuffers.push_back(ib);
				indexCounts.push_back(static_cast<ui32>(sm.indices.size()));
			}
		}
		static ui32 GetTypeID()
		{
			return 2;
		}
		std::vector< DUPLEX_NS_GRAPHICS::BsdfMaterial> materials;
		Engine::Resources::Mesh mesh;
	private:
		std::vector<DUPLEX_NS_GRAPHICS::BufferHandle> vertexBuffers;
		std::vector<DUPLEX_NS_GRAPHICS::BufferHandle> indexBuffers;
		std::vector<ui32> indexCounts;
	protected:
		friend class MeshSystem;
	};
}
