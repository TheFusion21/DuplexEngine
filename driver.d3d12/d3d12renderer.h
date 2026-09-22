#pragma once
// EXTERNAL INCLUDES
#include <d3d12.h>
#include <dxgi1_6.h>
// INTERNAL INCLUDES
#include "renderer.h"
#include "namespaces.h"

#define D3D12_FRAME_COUNT 2

namespace DUPLEX_NS_GRAPHICS
{
	// D3D12 backend scaffold: implements the Renderer interface so the build/link graph
	// is complete, but device/command-queue/swapchain bring-up is not implemented yet.
	// See VulkanRenderer's Init() for the equivalent bring-up sequence to mirror
	// (adapter/device selection -> command queue -> swapchain -> RTV heap).
	class D3D12Renderer : public Renderer
	{
	private:
		ID3D12Device* device = nullptr;
		IDXGISwapChain3* swapChain = nullptr;
		ID3D12CommandQueue* commandQueue = nullptr;
		ID3D12CommandAllocator* commandAllocator = nullptr;
		ID3D12GraphicsCommandList* commandList = nullptr;
		ID3D12DescriptorHeap* rtvHeap = nullptr;
		ui32 rtvDescriptorSize = 0;
		ID3D12Resource* renderTargets[D3D12_FRAME_COUNT] = { nullptr, nullptr };
		ID3D12Fence* fence = nullptr;
		ui64 fenceValue = 0;
		HANDLE fenceEvent = nullptr;
		ui32 frameIndex = 0;

	public:
		bool Init(ui64 instance, ui64 handle, ui32 width, ui32 height) override;

		void SetViewPort() override;

		void CreateShader() override;

		void SetActiveCamera(DUPLEX_NS_MATH::Vec3 eye, DUPLEX_NS_MATH::Mat4x4 viewProj) override;

		void ClearLights() override;
		void SetLight(DUPLEX_NS_UTIL::GpuLight lightDescriptor) override;

		void BeginScene() override;

		void EndScene() override;

		void Render(DUPLEX_NS_MATH::Mat4x4 transformMat, GraphicsBufferPtr vertexBuffer, GraphicsBufferPtr indexBuffer, ui32 indexCount) override;

		void Shutdown() override;

		IntPtr CreateTexture(ui32 width, ui32 height, ui32 levels, TextureFormat format, void* data = nullptr) override;

		IntPtr CreateTextureSRV(IntPtr texture, TextureFormat format) override;

		void UseTexture(ui32 slot, GraphicsBufferPtr view) override;

		IntPtr CreateCubemapSRV(IntPtr cubemap, TextureFormat format) override;

		GraphicsBufferPtr CreateBuffer(BufferType type, const void* data, int dataSize, UsageType usage = UsageType::Default) override;

		bool Resize(ui32 width, ui32 height) override;

		bool CheckForFullscreen() override;

		void ReleaseTexture(IntPtr& texture) override;
		void ReleaseTextureSRV(IntPtr& srv) override;
		void ReleaseBuffer(IntPtr& buffer) override;
	};
}
