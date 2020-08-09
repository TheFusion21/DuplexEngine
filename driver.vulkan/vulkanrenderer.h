#pragma once
#include "renderer.h"
#include "vulkan/vulkan.h"
#include "vulkan/vk_sdk_platform.h"
namespace Engine::Graphics
{
	class VulkanRenderer : public Renderer
	{
	private:
		VkInstance instance;
		VkPhysicalDevice physicalDevice;
		VkDevice device;
		VkSurfaceKHR surface;
		VkPhysicalDeviceProperties deviceProps;
		ui32 queueFamilyCount;
		VkQueueFamilyProperties* queueProps = nullptr;
		VkFormat format;
		VkColorSpaceKHR colorSpace;
		ui32 enabledExtensionCount = 0;
		ui32 enabledLayerCount = 0;
		ui32 graphicsQueueFamilyIndex = 0;
		VkQueue graphicsQueue = nullptr;
		const char* enabledLayers[64];
		const char* extensionNames[64];

		PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fpGetPhysicalDeviceSurfaceFormatsKHR;

		bool validate = true;

#ifdef VK_USE_PLATFORM_WIN32_KHR
		HINSTANCE connection;
#endif
	public:
		bool Init(ui64 instance, ui64 handle, ui32 width, ui32 height);
		void SetViewPort();
		void CreateShader();
		void SetActiveCamera(Engine::Math::Vec3 eye, Engine::Math::Mat4x4 viewProj);
		void BeginScene();
		void EndScene();
		void Render(const GameObject* object);
		void Shutdown();
		GraphicsBufferPtr CreateBuffer(BufferType type, const void* data, int dataSize, UsageType usage = UsageType::Default);
		GraphicsBufferPtr CreateTexture2D(BufferType type, DXGI_FORMAT format, const void* data, ui32 width, ui32 height, ui32 pitch, UsageType usage = UsageType::Default);
		ShaderResourcePtr CreateShaderResource(GraphicsBufferPtr resource, D3D11_SHADER_RESOURCE_VIEW_DESC* desc);
		bool Resize(ui32 width, ui32 height);
		bool CheckForFullscreen();
	};
}