#pragma once
// EXTERNAL INCLUDES
#include <d3dcompiler.h>
#include <d3d11.h>
// INTERNAL INCLUDES
#include "component.h"
#include "math/types.h"
#include "mesh.h"
#include "graphics/material.h"
#include "graphics/stb_image.h"
#include "utils/util.h"
#include "d3d11renderer.h"
namespace Engine::ECS
{
	class MeshRenderer
	{
	public:
		static ComponentType baseType;
		enum class ShaderType
		{
			BlinnPhong,
			SignedDistanceField
		};
		void SetMaterial(Material mat)
		{
			this->mat = mat;
		}
		void SetMesh(Engine::Resources::Mesh mesh)
		{
			if (vertexBuffer != nullptr)
			{
				auto d11Buffer = reinterpret_cast<ID3D11Buffer*>(vertexBuffer);
				SAFERELEASE(d11Buffer);
				vertexBuffer = nullptr;
			}
			if (indexBuffer != nullptr)
			{
				auto d11Buffer = reinterpret_cast<ID3D11Buffer*>(indexBuffer);
				SAFERELEASE(d11Buffer);
				indexBuffer = nullptr;
			}

			vertexBuffer = Engine::Graphics::Renderer::GetInstancePtr()->CreateBuffer(Engine::Graphics::D3D11Renderer::BufferType::Vertex, mesh.vertices.data(), static_cast<int>(mesh.vertices.size()) * sizeof(Vertex));
			if (!vertexBuffer)
			{
				MessageBoxA(NULL, "Could not create vertex buffer", "ERROR", MB_OK | MB_ICONEXCLAMATION);
				return;
			}
			indexBuffer = Engine::Graphics::Renderer::GetInstancePtr()->CreateBuffer(Engine::Graphics::D3D11Renderer::BufferType::Index, mesh.indices.data(), static_cast<int>(mesh.indices.size()) * sizeof(int));
			if (!indexBuffer)
			{
				MessageBoxA(NULL, "Could not create index buffer", "ERROR", MB_OK | MB_ICONEXCLAMATION);
				return;
			}
			indexCount = static_cast<int>(mesh.indices.size());
		}
		void SetShaderType(ShaderType type)
		{
			this->shaderType = type;
		}
		void UseTexture(const char* filename)
		{
			int width, height, channels;
			unsigned char* data = stbi_load(filename, &width, &height, &channels, 0);
			if (data == nullptr)return;
			ui32 pitch = width * sizeof(unsigned char) * 4;
			GraphicsBufferPtr texBuffer = Engine::Graphics::Renderer::GetInstancePtr()->CreateTexture2D(Engine::Graphics::D3D11Renderer::BufferType::ShaderResource, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM, data, width, height, pitch);
			if (texBuffer == nullptr)return;

			D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
			SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
			SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			SRVDesc.Texture2D.MipLevels = 1;

			texture = reinterpret_cast<ID3D11ShaderResourceView*>(Engine::Graphics::Renderer::GetInstancePtr()->CreateShaderResource(texBuffer, &SRVDesc));

			stbi_image_free(data);
		}
		static ui32 GetTypeID()
		{
			return 2;
		}
	private:
		ShaderType shaderType = ShaderType::BlinnPhong;
		GraphicsBufferPtr vertexBuffer = nullptr;
		GraphicsBufferPtr indexBuffer = nullptr;
		ID3D11ShaderResourceView* texture = nullptr;
		ui32 indexCount = 0;
		Material mat;
		//friend class Engine::Graphics::D3D11Renderer;
	protected:
		friend class MeshSystem;
	};
}