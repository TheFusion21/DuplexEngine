#pragma once
#include "renderer.h"
#include <vulkan/vulkan.h>
#include "namespaces.h"

#define FRAME_LAG 2
namespace DUPLEX_NS_GRAPHICS
{
	class VulkanRenderer : public Renderer
	{
	private:
		VkInstance instance = nullptr;
		VkSurfaceKHR surface = nullptr;
		VkDebugUtilsMessengerEXT debugMessenger = nullptr; //Vulkan Debugger interface


		VkPhysicalDevice physicalDevice = nullptr; //Physical Device 
		VkDevice device = nullptr; //Logical Device

		static PFN_vkGetDeviceProcAddr g_gdpa;

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

		std::vector<const char*> requiredExtensions =
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};
		std::vector<const char*> requiredValidationLayers =
		{
			"VK_LAYER_KHRONOS_validation"
		};
		i32 graphicsFamilyQueueIndex;
		i32 presentFamilyQueueIndex;
		VkQueue graphicsQueue;
		VkQueue presentQueue;

		VkSurfaceFormatKHR swapchainImageFormat;
		VkExtent2D swapchainExtent;
		VkSwapchainKHR swapchain;
	public:
		bool Init(SDL_Window* window, ui32 width, ui32 height);
		void SetViewPort();
		void CreateShader();
		void SetActiveCamera(DUPLEX_NS_MATH::Vec3 eye, DUPLEX_NS_MATH::Mat4x4 viewProj);
		void ClearLights();
		void SetLight(DUPLEX_NS_UTIL::GpuLight lightDescriptor);
		TextureHandle CreateTexture(ui32 width, ui32 height, ui32 levels, TextureFormat format, void* data = nullptr);

		ShaderResourceViewHandle CreateTextureSRV(TextureHandle texture, TextureFormat format);

		void UseTexture(ui32 slot, ShaderResourceViewHandle view);
		void BeginScene();
		void EndScene();
		void Render(DUPLEX_NS_MATH::Mat4x4 transformMat, BufferHandle vertexBuffer, BufferHandle indexBuffer, ui32 indexCount);
		void Shutdown();
		BufferHandle CreateBuffer(BufferType type, const void* data, int dataSize, UsageType usage = UsageType::Default);
		bool Resize(ui32 width, ui32 height);
		bool CheckForFullscreen();

		ShaderResourceViewHandle CreateCubemapSRV(TextureHandle cubemap, TextureFormat format);

		void ReleaseTexture(TextureHandle& texture);
		void ReleaseTextureSRV(ShaderResourceViewHandle& srv);
		void ReleaseBuffer(BufferHandle& buffer);
	private:
		struct VulkanSwapchainSupportDetails {
			VkSurfaceCapabilitiesKHR capabilities;
			std::vector<VkSurfaceFormatKHR> formats;
			std::vector<VkPresentModeKHR> presentModes;
		};
		void GetRequiredExtension(SDL_Window* window, std::vector<const char*>& extensionNames);
		void CreateSurface(SDL_Window* window);
		VkPhysicalDevice SelectPhysicalDevice();
		bool PhysicalDeviceMeetsRequirements(VkPhysicalDevice physicalDevice);
		void DetectQueueFamilyIndices(VkPhysicalDevice physicalDevice, int* graphicsQueueIndex, int* presentQueueIndex);
		VulkanSwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice physicalDevice);
		void CreateLogicalDevice();
		void CreateSwapchain();
		void CreateSwapchainImagesAndViews();
		void CreateRenderPass();
	};
}