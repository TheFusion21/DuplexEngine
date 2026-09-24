#pragma once
// EXTERNAL INCLUDES
#include <d3d12.h>
#include <dxgi1_4.h>
#include <vector>
// INTERNAL INCLUDES
#include "renderer.h"
#include "namespaces.h"

// Two frames in flight, same as driver.vulkan's FRAME_LAG - D3D12, like Vulkan and unlike
// D3D11's immediate context, needs the caller to manage its own CPU/GPU overlap explicitly
// (per-frame command allocators + a fence), so this backend's frame-sync shape mirrors
// VulkanRenderer rather than D3D11Renderer even though its resource-creation calls read closer
// to D3D11's.
#define D3D12_FRAME_COUNT 2
// Same budget/rationale as driver.vulkan's MAX_DRAWS_PER_FRAME: how many Render() calls can
// happen in a single frame before its slice of the per-frame constant/descriptor allocations
// would wrap into a slot that might still be in flight on the GPU.
#define MAX_DRAWS_PER_FRAME 128
// t0.._5 in bsdfPixel.hlsl (albedo/metallic/occlusion/roughness/normal/emissive) - see
// meshsystem.h's UseTexture() call sites for the engine-slot -> register mapping.
#define SRVS_PER_DRAW 6

namespace DUPLEX_NS_GRAPHICS
{
	class D3D12Renderer : public Renderer
	{
	private:
		ID3D12Device* device = nullptr;
		ID3D12CommandQueue* commandQueue = nullptr;
		IDXGISwapChain3* swapChain = nullptr;
		BOOL inFullscreen = FALSE;
		bool vsyncEnable = true;
		bool wireframe = false;

		ID3D12DescriptorHeap* rtvHeap = nullptr;
		ui32 rtvDescriptorSize = 0;
		ID3D12Resource* renderTargets[D3D12_FRAME_COUNT] = { nullptr, nullptr };

		ID3D12DescriptorHeap* dsvHeap = nullptr;
		ID3D12Resource* depthStencilBuffer = nullptr;

		ID3D12CommandAllocator* commandAllocators[D3D12_FRAME_COUNT] = { nullptr, nullptr };
		ID3D12GraphicsCommandList* commandList = nullptr;

		// A separate allocator/list pair for one-off texture uploads (CreateTexture can be
		// called well outside any BeginScene()/EndScene() window - e.g. at asset-load time in
		// Game::Client::Application::Init()) - reset and reused synchronously per upload rather
		// than sharing the per-frame commandAllocators above. Mirrors VulkanRenderer's
		// BeginSingleTimeCommands/EndSingleTimeCommands, which allocates/frees a one-shot command
		// buffer from the same pool for the same reason.
		ID3D12CommandAllocator* uploadCommandAllocator = nullptr;
		ID3D12GraphicsCommandList* uploadCommandList = nullptr;

		ID3D12RootSignature* rootSignature = nullptr;
		ID3D12PipelineState* pipelineState = nullptr;

		// Shader-visible CBV_SRV_UAV heap, D3D12_FRAME_COUNT * MAX_DRAWS_PER_FRAME * SRVS_PER_DRAW
		// descriptors - Render() copies each draw's 6 material SRVs (see CreateTextureSRV's own
		// cpuSrvHeap below) into this frame's slice and binds a descriptor table pointing at it.
		// Mirrors VulkanRenderer's per-frame descriptor pools, just with D3D12's split between
		// "where a view lives" and "what's actually bound" made explicit via CopyDescriptorsSimple
		// instead of Vulkan's vkUpdateDescriptorSets.
		ID3D12DescriptorHeap* srvHeap = nullptr;
		ui32 srvDescriptorSize = 0;

		// CPU-only (non-shader-visible) heap backing every SRV CreateTextureSRV() ever hands out -
		// materials create their SRVs once and keep them for the material's lifetime (see
		// texture2d.cpp), so this only ever grows; entries are never reclaimed on
		// ReleaseTextureSRV (same "do the simple thing first" call as Phase 11's transform system -
		// nothing in the engine repeatedly creates/destroys SRVs at runtime yet). Sized generously
		// since a non-shader-visible heap slot is cheap.
		static const ui32 CpuSrvHeapCapacity = 4096;
		ID3D12DescriptorHeap* cpuSrvHeap = nullptr;
		ui32 cpuSrvDescriptorSize = 0;
		ui32 cpuSrvHeapNextSlot = 0;

		// Dedicated 1-descriptor shader-visible heap for Dear ImGui's font atlas SRV - separate
		// from srvHeap above so ImGui's descriptor doesn't collide with this renderer's own
		// per-draw slices (mirrors VulkanRenderer's separate imguiDescriptorPool).
		bool imguiInitialized = false;
		ID3D12DescriptorHeap* imguiSrvHeap = nullptr;

		ID3D12Fence* fence = nullptr;
		ui64 fenceValues[D3D12_FRAME_COUNT] = { 0, 0 };
		ui64 nextFenceValue = 1;
		HANDLE fenceEvent = nullptr;

		ui32 frameIndex = 0;
		ui32 drawIndexThisFrame = 0;

		FLOAT clearColor[4] = { 0.529411f, 0.807843f, 0.921686f, 1.0f };

		// A persistently-mapped upload-heap resource per frame, holding MAX_DRAWS_PER_FRAME
		// 256-byte-aligned modelConstant slots (modelBuffers) or a single worldConstant
		// (worldBuffers) - written directly via memcpy, read by the GPU via root CBVs (no
		// descriptor heap entry needed for a root descriptor, unlike the SRVs above).
		struct UploadSlots
		{
			ID3D12Resource* resource = nullptr;
			ui8* mapped = nullptr;
			D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = 0;
			ui32 slotStride = 0;
		};
		UploadSlots modelBuffers[D3D12_FRAME_COUNT];
		UploadSlots worldBuffers[D3D12_FRAME_COUNT];

		DUPLEX_NS_UTIL::worldConstant worldLocalBuffer;
		std::vector<DUPLEX_NS_UTIL::GpuLight> lights;

		// Accumulated by UseTexture(), consumed (and cleared) by the next Render() call - mirrors
		// D3D11Renderer::textureViews / VulkanRenderer::pendingTextureViews.
		std::vector<ShaderResourceViewHandle> pendingTextureViews;

		// A 1x1 white pixel, bound for any material texture slot a draw call didn't set via
		// UseTexture() - same reasoning as VulkanRenderer::defaultTextureHandle: every SRV table
		// entry needs to reference a validly-laid-out resource, there's no "leave it null" option.
		TextureHandle defaultTextureHandle;
		ShaderResourceViewHandle defaultTextureView;

		// Backs the generation-checked handles, same pattern as the other two backends' pools.
		struct D3D12Buffer
		{
			ID3D12Resource* resource = nullptr;
			ui8* mapped = nullptr;
			ui32 size = 0;
		};
		struct D3D12Texture
		{
			ID3D12Resource* resource = nullptr;
			DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
		};
		struct D3D12ShaderResourceView
		{
			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{};
			// Unlike ID3D11ShaderResourceView, a D3D12 descriptor holds no COM reference to its
			// source resource at all - it's just metadata written into a heap slot, so releasing
			// the caller's own TextureHandle (see texture2d.cpp, which creates a texture, creates
			// its SRV, then immediately releases the texture - relying on D3D11's automatic
			// view-holds-a-reference behavior) would otherwise destroy the resource out from
			// under every SRV still pointing at it. Confirmed directly via GPU-based validation:
			// "Invalid resource pointed to by descriptor ... resource has been destroyed",
			// manifesting as a GPU hang/TDR a couple of frames in once the destroyed resource's
			// backing memory got reused. An explicit extra AddRef()/Release() here (see
			// CreateTextureSRV/CreateCubemapSRV/ReleaseTextureSRV) reproduces D3D11's real
			// lifetime semantics instead.
			ID3D12Resource* resource = nullptr;
		};
		HandlePool<D3D12Buffer, BufferHandle> bufferPool;
		HandlePool<D3D12Texture, TextureHandle> texturePool;
		HandlePool<D3D12ShaderResourceView, ShaderResourceViewHandle> srvPool;

	public:
		bool Init(SDL_Window* window, ui32 width, ui32 height) override;

		void SetViewPort() override;

		void CreateShader() override;

		void SetActiveCamera(DUPLEX_NS_MATH::Vec3 eye, DUPLEX_NS_MATH::Mat4x4 viewProj) override;

		void ClearLights() override;
		void SetLight(DUPLEX_NS_UTIL::GpuLight lightDescriptor) override;

		void BeginScene() override;

		void EndScene() override;

		void Render(DUPLEX_NS_MATH::Mat4x4 transformMat, BufferHandle vertexBuffer, BufferHandle indexBuffer, ui32 indexCount) override;

		void Shutdown() override;

		TextureHandle CreateTexture(ui32 width, ui32 height, ui32 levels, TextureFormat format, void* data = nullptr) override;

		ShaderResourceViewHandle CreateTextureSRV(TextureHandle texture, TextureFormat format) override;

		void UseTexture(ui32 slot, ShaderResourceViewHandle view) override;

		ShaderResourceViewHandle CreateCubemapSRV(TextureHandle cubemap, TextureFormat format) override;

		BufferHandle CreateBuffer(BufferType type, const void* data, int dataSize, UsageType usage = UsageType::Default) override;

		bool Resize(ui32 width, ui32 height) override;

		bool CheckForFullscreen() override;

		void ReleaseTexture(TextureHandle& texture) override;
		void ReleaseTextureSRV(ShaderResourceViewHandle& srv) override;
		void ReleaseBuffer(BufferHandle& buffer) override;

		bool InitImGui(SDL_Window* window) override;
		void ImGuiNewFrame(SDL_Window* window) override;
		void ImGuiRenderDrawData() override;

	private:
		bool CreateDeviceAndSwapChain(SDL_Window* window, ui32 width, ui32 height);
		bool CreateRtvAndDsvHeaps();
		bool CreateFrameResources();
		bool CreateDepthStencil();
		void ReleaseFrameResources();
		bool CreateRootSignature();
		bool CreateCommandObjects();
		bool CreateFenceObjects();
		bool CreateDescriptorHeaps();
		bool CreateConstantBuffers();
		bool CreateDefaultTexture();

		// Blocks until every command list submitted so far has finished on the GPU - used by
		// Shutdown()/Resize(), where every frame's resources must be safe to destroy, not just
		// the current one's (see MoveToNextFrame() for the steady-state per-frame wait instead).
		void WaitForGpu();
		void MoveToNextFrame();

		DXGI_FORMAT FromTextureFormat(TextureFormat format);
		ID3D12Resource* CreateUploadResource(ui64 size, const void* data);
		ID3D12Resource* CreateCommittedTexture2D(ui32 width, ui32 height, DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE* clearValue);
		// Records a copy from a freshly-created upload-heap staging buffer into `texture` onto
		// the currently-open uploadCommandList, and returns that staging buffer - the caller
		// (CreateTexture) owns keeping it alive until the GPU copy has actually executed (i.e.
		// until after the WaitForGpu() that follows submitting uploadCommandList), then releasing
		// it. Returns nullptr on allocation failure, in which case nothing was recorded.
		ID3D12Resource* UploadTextureData(ID3D12Resource* texture, ui32 width, ui32 height, DXGI_FORMAT format, ui32 pixelSize, const void* data);
	};
}
