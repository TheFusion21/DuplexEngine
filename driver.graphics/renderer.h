#pragma once

#include <dxgi.h>
#include <d3d11.h>

#include "gameobject.h"
#include "math/types.h"

namespace Engine::Graphics
{
	class Renderer
	{
	private:
		BOOL isFullscreen = false;
		BOOL vsyncEnable = true;
		BOOL wireFrame = false;

	protected:
		Renderer() { }
		static Renderer* mpInstance;
		Renderer(const Renderer&) { }
		Renderer& operator=(const Renderer&) { }
	public:
		enum class BufferType
		{
			Index,
			Vertex,
			Constant,
			DepthStencil,
			ShaderResource,
		};
		enum class UsageType
		{
			Default,
			Dynamic
		};
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

		virtual void SetActiveCamera(Engine::Math::Vec3 eye, Engine::Math::Mat4x4 viewProj) = 0;

		virtual void BeginScene() = 0;

		virtual void EndScene() = 0;

		virtual void Render(const GameObject* object) = 0;

		virtual void Shutdown() = 0;

		virtual GraphicsBufferPtr CreateBuffer(BufferType type, const void* data, int dataSize, UsageType usage = UsageType::Default) = 0;

		virtual GraphicsBufferPtr CreateTexture2D(BufferType type, DXGI_FORMAT format, const void* data, ui32 width, ui32 height, ui32 pitch, UsageType usage = UsageType::Default) = 0;

		virtual ShaderResourcePtr CreateShaderResource(GraphicsBufferPtr resource, D3D11_SHADER_RESOURCE_VIEW_DESC* desc) = 0;

		virtual bool Resize(ui32 width, ui32 height) = 0;

		virtual bool CheckForFullscreen() = 0;

	};
}