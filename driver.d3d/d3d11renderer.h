#pragma once
// EXTERNAL INCLUDES
#include <dxgi.h>
#include <d3d11.h>
#include <vector>
// INTERNAL INCLUDES
#include "renderer.h"

namespace Engine::Graphics
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
		GraphicsBufferPtr modelBuffer = nullptr;
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
		Engine::Utils::worldConstant worldLocalBuffer;
		GraphicsBufferPtr worldBuffer = nullptr;
		std::vector<Engine::Utils::GpuLight> lights;
		

		ID3D11Debug* debug = nullptr;
		std::vector<ID3D11ShaderResourceView*> textureViews;
		//Lights
		//std::vector<Engine::Components::DirectionalLight*> dirLights;
		//std::vector<Engine::Components::PointLight*> pointLights;

	public:
		/// <summary>
		/// 
		/// </summary>
		/// <returns>init was successful</returns>
		bool Init(ui64 instance, ui64 handle, ui32 width, ui32 height);

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
		//void RenderObject(Engine::Math::Transform transform, int indexCount, GraphicsBufferPtr vertexBuffer, GraphicsBufferPtr indexBuffer);
		void Render(Engine::Math::Mat4x4 transformMat, GraphicsBufferPtr vertexBuffer, GraphicsBufferPtr indexBuffer, ui32 indexCount);
		/// <summary>
		/// Assign a camera to be used for rendering
		/// </summary>
		/// <param name="camera"></param>
		void SetActiveCamera(Engine::Math::Vec3 eye, Engine::Math::Mat4x4 viewProj);

		void ClearLights();

		void SetLight(Engine::Utils::GpuLight lightDescriptor);

		bool CheckForFullscreen();

		IntPtr CreateTexture(ui32 width, ui32 height, ui32 levels, TextureFormat format, void* data = nullptr);
		void ReleaseTexture(IntPtr& texture);
		void UseTexture(ui32 slot, GraphicsBufferPtr view);
		IntPtr CreateTextureSRV(IntPtr texture, TextureFormat format);
		void ReleaseTextureSRV(IntPtr& srv);

		IntPtr CreateCubemapSRV(IntPtr cubemap, TextureFormat format);
		/// <summary>
		/// Create a Buffer in a specific type with data and the defined usage
		/// </summary>
		/// <param name="type"></param>
		/// <param name="data"></param>
		/// <param name="dataSize"></param>
		/// <param name="usage"></param>
		/// <returns>A generic pointer to be used for rendering or updating the buffer</returns>
		GraphicsBufferPtr CreateBuffer(BufferType type, const void* data, int dataSize, UsageType usage = UsageType::Default);
		void ReleaseBuffer(IntPtr& buffer);


		//void AddDirectionalLight(Engine::Components::DirectionalLight& dirLight);
		//void AddPointLight(Engine::Components::PointLight& pointLight);
	private:
		DXGI_FORMAT FromTextureFormat(TextureFormat format);
	};
}