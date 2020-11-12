#pragma once

#include <dxgi.h>
#include <d3d11.h>

#include "gameobject.h"
#include "math/types.h"
#include "enums.h"
#include "mesh.h"
#include "shadercb.h"
#include "namespaces.h"

namespace DUPLEX_NS_GRAPHICS
{
	class Renderer
	{
	protected:
		BOOL isFullscreen = false;
		BOOL vsyncEnable = true;
		BOOL wireFrame = false;
		Renderer() { }
		static Renderer* mpInstance;
		Renderer(const Renderer&) { }
		Renderer& operator=(const Renderer&) { }
		ui32 width = 0, height = 0;
	public:
		
		static void CreateInstance(Renderer* renderer)
		{
			if (mpInstance != nullptr)return;
			mpInstance = renderer;
		}
		static Renderer* GetInstancePtr()
		{
			return mpInstance;
		}
		virtual bool Init(ui64 instance, ui64 handle, ui32 width, ui32 height) = 0;

		virtual void SetViewPort() = 0;

		virtual void CreateShader() = 0;

		virtual void SetActiveCamera(DUPLEX_NS_MATH::Vec3 eye, DUPLEX_NS_MATH::Mat4x4 viewProj) = 0;

		virtual void ClearLights() = 0;
		virtual void SetLight(DUPLEX_NS_UTIL::GpuLight lightDescriptor) = 0;

		virtual void BeginScene() = 0;

		virtual void EndScene() = 0;

		virtual void Render(DUPLEX_NS_MATH::Mat4x4 transformMat, GraphicsBufferPtr vertexBuffer, GraphicsBufferPtr indexBuffer, ui32 indexCount) = 0;

		virtual void Shutdown() = 0;

		virtual IntPtr CreateTexture(ui32 width, ui32 height, ui32 levels, TextureFormat format, void* data = nullptr) = 0;

		virtual IntPtr CreateTextureSRV(IntPtr texture, TextureFormat format) = 0;

		virtual void UseTexture(ui32 slot, GraphicsBufferPtr view) = 0;

		virtual IntPtr CreateCubemapSRV(IntPtr cubemap, TextureFormat format) = 0;

		virtual GraphicsBufferPtr CreateBuffer(BufferType type, const void* data, int dataSize, UsageType usage = UsageType::Default) = 0;

		virtual bool Resize(ui32 width, ui32 height) = 0;

		virtual bool CheckForFullscreen() = 0;

		virtual void ReleaseTexture(IntPtr& texture) = 0;
		virtual void ReleaseTextureSRV(IntPtr& srv) = 0;
		virtual void ReleaseBuffer(IntPtr& buffer) = 0;

	protected:
		ui32 PixelSizeFromTextureFormat(TextureFormat format);


	};
}