#pragma once
#include "renderer.h"
#include <vulkan/vulkan.h>
#include <vulkan/vk_sdk_platform.h>
#define FRAME_LAG 2
namespace Engine::Graphics
{
	class VulkanRenderer : public Renderer
	{
	private:
		static PFN_vkGetDeviceProcAddr g_gdpa;
		VkInstance instance{};

		VkPhysicalDevice physicalDevice{};
		VkDevice device{};
		VkPhysicalDeviceProperties deviceProps{};

		ui32 swapchainImageCount;
		VkSwapchainKHR swapchain;

		VkSurfaceKHR surface{};
		VkQueueFamilyProperties* queueProps = nullptr;
		VkPhysicalDeviceMemoryProperties memoryProperties{};
		VkFormat format;
		VkColorSpaceKHR colorSpace;
		ui32 enabledExtensionCount = 0;
		ui32 enabledLayerCount = 0;

		ui32 queueFamilyCount = 0;
		ui32 graphicsQueueFamilyIndex = 0;
		ui32 presentQueueFamilyIndex = 0;
		VkQueue graphicsQueue = nullptr;
		VkQueue presentQueue = nullptr;
		VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
		VkFence fences[FRAME_LAG];
		VkSemaphore imageAcquiredSemaphores[FRAME_LAG];
		VkSemaphore drawCompleteSemaphores[FRAME_LAG];
		VkSemaphore imageOwnershipSemaphores[FRAME_LAG];

		bool seperatePresentQueue = false;
		const char* enabledLayers[64];
		const char* extensionNames[64];

		//KHR HELPERS
		PFN_vkGetPhysicalDeviceSurfaceSupportKHR fpGetPhysicalDeviceSurfaceSupportKHR;
		PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR fpGetPhysicalDeviceSurfaceCapabilitiesKHR;
		PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fpGetPhysicalDeviceSurfaceFormatsKHR;
		PFN_vkGetPhysicalDeviceSurfacePresentModesKHR fpGetPhysicalDeviceSurfacePresentModesKHR;
		PFN_vkCreateSwapchainKHR fpCreateSwapchainKHR;
		PFN_vkDestroySwapchainKHR fpDestroySwapchainKHR;
		PFN_vkGetSwapchainImagesKHR fpGetSwapchainImagesKHR;
		PFN_vkAcquireNextImageKHR fpAcquireNextImageKHR;
		PFN_vkQueuePresentKHR fpQueuePresentKHR;

		int frameIndex = 0;

		VkCommandPool cmdPool;
		VkCommandPool presentCmdPool;
		VkCommandBuffer cmdBuffer;  // Buffer for initialization commands

		bool validate = false;

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
		void Render(Engine::Math::Mat4x4 transformMat, GraphicsBufferPtr vertexBuffer, GraphicsBufferPtr indexBuffer, ui32 indexCount);
		void Shutdown();
		GraphicsBufferPtr CreateBuffer(BufferType type, const void* data, int dataSize, UsageType usage = UsageType::Default);
		ShaderResourcePtr CreateShaderResource(GraphicsBufferPtr resource, D3D11_SHADER_RESOURCE_VIEW_DESC* desc);
		bool Resize(ui32 width, ui32 height);
		bool CheckForFullscreen();
	};
}