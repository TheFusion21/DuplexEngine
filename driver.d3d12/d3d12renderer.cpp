#include "d3d12renderer.h"

using namespace DUPLEX_NS_GRAPHICS;
using namespace DUPLEX_NS_MATH;
using namespace DUPLEX_NS_UTIL;

// NOTE: This backend is a scaffold only (see d3d12renderer.h). None of the methods below
// have been compiled or run yet - this session has no Windows/MSVC toolchain available to
// verify them. Treat as unverified until built on Windows.

bool D3D12Renderer::Init(ui64 instance, ui64 handle, ui32 width, ui32 height)
{
	this->width = width;
	this->height = height;
	return false;
}

void D3D12Renderer::SetViewPort()
{
}

void D3D12Renderer::CreateShader()
{
}

void D3D12Renderer::SetActiveCamera(Vec3 eye, Mat4x4 viewProj)
{
}

void D3D12Renderer::ClearLights()
{
}

void D3D12Renderer::SetLight(GpuLight lightDescriptor)
{
}

void D3D12Renderer::BeginScene()
{
}

void D3D12Renderer::EndScene()
{
}

void D3D12Renderer::Render(Mat4x4 transformMat, BufferHandle vertexBuffer, BufferHandle indexBuffer, ui32 indexCount)
{
}

void D3D12Renderer::Shutdown()
{
}

TextureHandle D3D12Renderer::CreateTexture(ui32 width, ui32 height, ui32 levels, TextureFormat format, void* data)
{
	return TextureHandle{};
}

ShaderResourceViewHandle D3D12Renderer::CreateTextureSRV(TextureHandle texture, TextureFormat format)
{
	return ShaderResourceViewHandle{};
}

void D3D12Renderer::UseTexture(ui32 slot, ShaderResourceViewHandle view)
{
}

ShaderResourceViewHandle D3D12Renderer::CreateCubemapSRV(TextureHandle cubemap, TextureFormat format)
{
	return ShaderResourceViewHandle{};
}

BufferHandle D3D12Renderer::CreateBuffer(BufferType type, const void* data, int dataSize, UsageType usage)
{
	return BufferHandle{};
}

bool D3D12Renderer::Resize(ui32 width, ui32 height)
{
	return false;
}

bool D3D12Renderer::CheckForFullscreen()
{
	return false;
}

void D3D12Renderer::ReleaseTexture(TextureHandle& texture)
{
}

void D3D12Renderer::ReleaseTextureSRV(ShaderResourceViewHandle& srv)
{
}

void D3D12Renderer::ReleaseBuffer(BufferHandle& buffer)
{
}
