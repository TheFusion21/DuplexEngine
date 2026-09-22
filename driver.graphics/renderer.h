#pragma once

#include "math/types.h"
#include "handles.h"
#include "enums.h"
#include "mesh.h"
#include "shadercb.h"
#include "namespaces.h"

struct SDL_Window;

namespace DUPLEX_NS_GRAPHICS
{
	class Renderer
	{
	protected:
		BOOL isFullscreen = false;
		BOOL vsyncEnable = true;
		BOOL wireFrame = false;
		Renderer() { }
		// Copying a base-class Renderer by value would slice the concrete backend, so that's
		// explicitly disallowed rather than silently doing the wrong thing.
		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;
		ui32 width = 0, height = 0;
	public:
		// Owned via unique_ptr by whoever creates a backend (see Game::Client::Application) and
		// deleted through this base pointer - needs to be virtual for that to run the concrete
		// backend's destructor instead of just Renderer's.
		virtual ~Renderer() = default;

		// window is the SDL2 window whatever caller (see Game::Client::Application) already
		// created; each backend pulls whatever native handle it actually needs out of it
		// (SDL_GetWindowWMInfo for D3D11's HWND, SDL_Vulkan_CreateSurface for Vulkan's surface).
		virtual bool Init(SDL_Window* window, ui32 width, ui32 height) = 0;

		virtual void SetViewPort() = 0;

		virtual void CreateShader() = 0;

		virtual void SetActiveCamera(DUPLEX_NS_MATH::Vec3 eye, DUPLEX_NS_MATH::Mat4x4 viewProj) = 0;

		virtual void ClearLights() = 0;
		virtual void SetLight(DUPLEX_NS_UTIL::GpuLight lightDescriptor) = 0;

		virtual void BeginScene() = 0;

		virtual void EndScene() = 0;

		virtual void Render(DUPLEX_NS_MATH::Mat4x4 transformMat, BufferHandle vertexBuffer, BufferHandle indexBuffer, ui32 indexCount) = 0;

		virtual void Shutdown() = 0;

		virtual TextureHandle CreateTexture(ui32 width, ui32 height, ui32 levels, TextureFormat format, void* data = nullptr) = 0;

		virtual ShaderResourceViewHandle CreateTextureSRV(TextureHandle texture, TextureFormat format) = 0;

		virtual void UseTexture(ui32 slot, ShaderResourceViewHandle view) = 0;

		virtual ShaderResourceViewHandle CreateCubemapSRV(TextureHandle cubemap, TextureFormat format) = 0;

		virtual BufferHandle CreateBuffer(BufferType type, const void* data, int dataSize, UsageType usage = UsageType::Default) = 0;

		virtual bool Resize(ui32 width, ui32 height) = 0;

		virtual bool CheckForFullscreen() = 0;

		virtual void ReleaseTexture(TextureHandle& texture) = 0;
		virtual void ReleaseTextureSRV(ShaderResourceViewHandle& srv) = 0;
		virtual void ReleaseBuffer(BufferHandle& buffer) = 0;

	protected:
		ui32 PixelSizeFromTextureFormat(TextureFormat format);
	};
}
