#pragma once

// EXTERNAL INCLUDES
#include <d3dcompiler.h>
#include <d3d11.h>
// INTERNAL INCLUDES
#include "component.h"
#include "math/types.h"
#include "mesh.h"
#include "material.h"

namespace Engine::Graphics
{
	class D3D11Renderer;
}
namespace Engine::Components
{
	class MeshRenderer : public Component
	{
	public:
		static ComponentType baseType;
		enum class ShaderType
		{
			BlinnPhong,
			SignedDistanceField
		};
		void SetMaterial(Material mat);
		void SetMesh(Engine::Resources::Mesh mesh);
		void SetShaderType(ShaderType type);
		void UseTexture(const char* filename);
	private:
		ShaderType shaderType = ShaderType::BlinnPhong;
		GraphicsBufferPtr vertexBuffer = nullptr;
		GraphicsBufferPtr indexBuffer = nullptr;
		ID3D11ShaderResourceView* texture = nullptr;
		int indexCount = 0;
		Material mat;
		friend class Engine::Graphics::D3D11Renderer;
	protected:
		MeshRenderer(GameObject* attach);
		friend class GameObject;
	};
}