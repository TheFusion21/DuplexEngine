
// EXTERNAL INCLUDES
// INTERNAL INCLUDES
#include "meshrenderer.h"
#include "utils/util.h"
#include "d3d11renderer.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
using namespace Engine::Components;
using namespace Engine::Graphics;

ComponentType MeshRenderer::baseType = ComponentType::MeshRenderer;

MeshRenderer::MeshRenderer(GameObject* attach) : Component(attach, baseType)
{

}

void MeshRenderer::SetMaterial(Material mat)
{
	this->mat = mat;
}

void MeshRenderer::SetMesh(Engine::Resources::Mesh mesh)
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

	vertexBuffer = D3D11Renderer::GetInstance().CreateBuffer(D3D11Renderer::BufferType::Vertex, mesh.vertices.data(), static_cast<int>(mesh.vertices.size()) * sizeof(Vertex));
	if (!vertexBuffer)
	{
		MessageBoxA(NULL, "Could not create vertex buffer", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	indexBuffer = D3D11Renderer::GetInstance().CreateBuffer(D3D11Renderer::BufferType::Index, mesh.indices.data(), static_cast<int>(mesh.indices.size()) * sizeof(int));
	if (!indexBuffer)
	{
		MessageBoxA(NULL, "Could not create index buffer", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	indexCount = static_cast<int>(mesh.indices.size());
}

void MeshRenderer::SetShaderType(ShaderType type)
{
	shaderType = type;
}

void MeshRenderer::UseTexture(const char* filename)
{
	int width, height, channels;
	unsigned char* data = stbi_load(filename, &width, &height, &channels, 0);
	if (data == nullptr)return;
	ui32 pitch = width * sizeof(unsigned char) * 4;
	GraphicsBufferPtr texBuffer = D3D11Renderer::GetInstance().CreateTexture2D(D3D11Renderer::BufferType::ShaderResource, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM, data, width, height, pitch);
	if (texBuffer == nullptr)return;

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MipLevels = 1;

	texture = D3D11Renderer::GetInstance().CreateShaderResource(texBuffer, &SRVDesc);
}
