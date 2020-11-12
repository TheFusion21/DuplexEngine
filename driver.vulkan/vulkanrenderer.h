#pragma once
#include "renderer.h"
#include <vulkan/vulkan.h>
#include <vulkan/vk_sdk_platform.h>
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
#if defined(VK_USE_PLATFORM_WIN32_KHR)
		HINSTANCE connection;
#endif
	public:
		bool Init(ui64 instance, ui64 handle, ui32 width, ui32 height);
		void SetViewPort();
		void CreateShader();
		void SetActiveCamera(DUPLEX_NS_MATH::Vec3 eye, DUPLEX_NS_MATH::Mat4x4 viewProj);
		void ClearLights();
		void SetLight(DUPLEX_NS_UTIL::GpuLight lightDescriptor);
		IntPtr CreateTexture(ui32 width, ui32 height, ui32 levels, TextureFormat format, void* data = nullptr);

		IntPtr CreateTextureSRV(IntPtr texture, TextureFormat format);

		void UseTexture(ui32 slot, GraphicsBufferPtr view);
		void BeginScene();
		void EndScene();
		void Render(DUPLEX_NS_MATH::Mat4x4 transformMat, GraphicsBufferPtr vertexBuffer, GraphicsBufferPtr indexBuffer, ui32 indexCount);
		void Shutdown();
		GraphicsBufferPtr CreateBuffer(BufferType type, const void* data, int dataSize, UsageType usage = UsageType::Default);
		ShaderResourcePtr CreateShaderResource(GraphicsBufferPtr resource, D3D11_SHADER_RESOURCE_VIEW_DESC* desc);
		bool Resize(ui32 width, ui32 height);
		bool CheckForFullscreen();

		void ReleaseTexture(IntPtr& texture);
		void ReleaseTextureSRV(IntPtr& srv);
		void ReleaseBuffer(IntPtr& buffer);
	private:
		struct VulkanSwapchainSupportDetails {
			VkSurfaceCapabilitiesKHR capabilities;
			std::vector<VkSurfaceFormatKHR> formats;
			std::vector<VkPresentModeKHR> presentModes;
		};
		void GetRequiredExtension(std::vector<const char*>& extensionNames);
		void CreateSurface(ui64 instance, ui64 handle);
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