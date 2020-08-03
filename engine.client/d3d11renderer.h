#pragma once
// EXTERNAL INCLUDES
#include <dxgi.h>
#include <d3d11.h>
#include <vector>
// INTERNAL INCLUDES
#include "utils/singleton.h"
#include "camera.h"
#include "renderer.h"

namespace Engine::Graphics
{
	class D3D11Renderer : public Renderer
	{
	private:
		bool vsyncEnable = false;
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
		ID3D11SamplerState* sampler = nullptr;

		ID3D11BlendState* blendState = nullptr;
		//Camera
		GraphicsBufferPtr worldBuffer = nullptr;
		ui32 width = 0, height = 0;

		//Lights
		//std::vector<Engine::Components::DirectionalLight*> dirLights;
		//std::vector<Engine::Components::PointLight*> pointLights;

	public:
		/// <summary>
		/// 
		/// </summary>
		/// <param name="window">Window to init to</param>
		/// <returns>init was successful</returns>
		bool Init(Engine::Core::AppWindow &window);

		void SetViewPort();

		bool CreateRenderTarget();

		bool CreateDepthStencil();

		bool Resize(Engine::Core::AppWindow& window);
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
		void Render(const GameObject* object);
		/// <summary>
		/// Assign a camera to be used for rendering
		/// </summary>
		/// <param name="camera"></param>
		void SetActiveCamera(Engine::Components::Camera* camera);

		bool CheckForFullscreen();
		/// <summary>
		/// Create a Buffer in a specific type with data and the defined usage
		/// </summary>
		/// <param name="type"></param>
		/// <param name="data"></param>
		/// <param name="dataSize"></param>
		/// <param name="usage"></param>
		/// <returns>A generic pointer to be used for rendering or updating the buffer</returns>
		GraphicsBufferPtr CreateBuffer(BufferType type, const void* data, int dataSize, UsageType usage = UsageType::Default);
		
		GraphicsBufferPtr CreateTexture2D(BufferType type, DXGI_FORMAT format, const void* data, ui32 width, ui32 height, ui32 pitch, UsageType usage = UsageType::Default);

		ShaderResourcePtr CreateShaderResource(GraphicsBufferPtr resource, D3D11_SHADER_RESOURCE_VIEW_DESC* desc);

		//void AddDirectionalLight(Engine::Components::DirectionalLight& dirLight);
		//void AddPointLight(Engine::Components::PointLight& pointLight);
	};
}