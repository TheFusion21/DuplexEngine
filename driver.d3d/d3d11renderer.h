#pragma once
// EXTERNAL INCLUDES
#include <dxgi.h>
#include <d3d11.h>
#include <vector>
// INTERNAL INCLUDES
#include "renderer.h"

namespace DUPLEX_NS_GRAPHICS
{
	class D3D11Renderer : public Renderer
	{
	private:
		bool vsyncEnable = true;
		bool wireframe = false;
		BOOL inFullscreen = false;

		IDXGISwapChain* swapChain = nullptr;
		//the render device
		ID3D11Device* device = nullptr;
		ID3D11DeviceContext* context = nullptr;
		ID3D11DepthStencilView* depthView = nullptr;
		ID3D11RasterizerState* rasterState = nullptr;
		//The view to render to
		ID3D11RenderTargetView* rtv = nullptr;

		FLOAT clearColor[4] = { 0.529411f, 0.807843f, 0.921686f, 1.0f };
		//Mesh Buffers
		BufferHandle modelBuffer;
		//Shaders
		ID3D11VertexShader* vertexShader = nullptr;
		ID3D11InputLayout* vertexLayout = nullptr;
		ID3D11PixelShader* pixelShader = nullptr;
		ID3D11PixelShader* pixelSDFShader = nullptr;

		//Texture Sampler
		ID3D11SamplerState* defaultSampler = nullptr;
		ID3D11SamplerState* computeSampler = nullptr;

		ID3D11BlendState* blendState = nullptr;
		//Camera
		DUPLEX_NS_UTIL::worldConstant worldLocalBuffer;
		BufferHandle worldBuffer;
		std::vector<DUPLEX_NS_UTIL::GpuLight> lights;

		// Backs the generation-checked handles returned to callers; owns the actual COM
		// resources so Get()/reinterpret_cast tricks on the handle bits itself aren't needed.
		HandlePool<ID3D11Buffer*, BufferHandle> bufferPool;
		HandlePool<ID3D11Texture2D*, TextureHandle> texturePool;
		HandlePool<ID3D11ShaderResourceView*, ShaderResourceViewHandle> srvPool;

		ID3D11Debug* debug = nullptr;
		std::vector<ID3D11ShaderResourceView*> textureViews;

		// A 1x1 white pixel, bound for any material texture slot a draw call didn't set via
		// UseTexture() - matches D3D12Renderer::defaultTextureView/VulkanRenderer::
		// defaultTextureHandle, which both already did this. Without it, an unmaterialed object
		// (e.g. the physics demo's floor/boxes) sampled a null SRV and rendered solid black,
		// instead of the plain lit-white look the other two backends show for the same object.
		TextureHandle defaultTextureHandle;
		ShaderResourceViewHandle defaultTextureView;
		//Lights
		//std::vector<Engine::Components::DirectionalLight*> dirLights;
		//std::vector<Engine::Components::PointLight*> pointLights;

	public:
		/// <summary>
		///
		/// </summary>
		/// <returns>init was successful</returns>
		bool Init(SDL_Window* window, ui32 width, ui32 height);

		void SetViewPort();

		bool CreateRenderTarget();

		bool CreateDepthStencil();

		bool Resize(ui32 width, ui32 height);
		/// <summary>
		/// Creates shaders and layout for vertex and pixel shader loaded from file
		/// </summary>
		void CreateShader();
		/// <summary>
		/// To begin a render and clear the screen with clearColor
		/// </summary>
		void BeginScene();
		/// <summary>
		/// To end a render and present rtv using vsync if enabled
		/// </summary>
		void EndScene();
		/// <summary>
		/// Release and clear/delete previously created pointers and exit fullscreen if required
		/// </summary>
		void Shutdown();
		void Render(DUPLEX_NS_MATH::Mat4x4 transformMat, BufferHandle vertexBuffer, BufferHandle indexBuffer, ui32 indexCount);
		/// <summary>
		/// Assign a camera to be used for rendering
		/// </summary>
		/// <param name="camera"></param>
		void SetActiveCamera(DUPLEX_NS_MATH::Vec3 eye, DUPLEX_NS_MATH::Mat4x4 viewProj);

		void ClearLights();

		void SetLight(DUPLEX_NS_UTIL::GpuLight lightDescriptor);

		bool CheckForFullscreen();

		TextureHandle CreateTexture(ui32 width, ui32 height, ui32 levels, TextureFormat format, void* data = nullptr);
		void ReleaseTexture(TextureHandle& texture);
		void UseTexture(ui32 slot, ShaderResourceViewHandle view);
		ShaderResourceViewHandle CreateTextureSRV(TextureHandle texture, TextureFormat format);
		void ReleaseTextureSRV(ShaderResourceViewHandle& srv);

		ShaderResourceViewHandle CreateCubemapSRV(TextureHandle cubemap, TextureFormat format);
		/// <summary>
		/// Create a Buffer in a specific type with data and the defined usage
		/// </summary>
		/// <param name="type"></param>
		/// <param name="data"></param>
		/// <param name="dataSize"></param>
		/// <param name="usage"></param>
		/// <returns>A handle to be used for rendering or updating the buffer</returns>
		BufferHandle CreateBuffer(BufferType type, const void* data, int dataSize, UsageType usage = UsageType::Default);
		void ReleaseBuffer(BufferHandle& buffer);

		bool InitImGui(SDL_Window* window);
		void ImGuiNewFrame(SDL_Window* window);
		void ImGuiRenderDrawData();

		//void AddDirectionalLight(Engine::Components::DirectionalLight& dirLight);
		//void AddPointLight(Engine::Components::PointLight& pointLight);
	private:
		bool imguiInitialized = false;
		DXGI_FORMAT FromTextureFormat(TextureFormat format);
	};
}
