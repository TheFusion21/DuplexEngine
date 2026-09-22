#pragma once
#include "renderer.h"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include "namespaces.h"

#define FRAME_LAG 2
// Generous fixed budget for how many Render() calls can happen within a single frame before
// modelConstantBuffers would need to wrap around into a slot that might still be in flight.
// The demo scene draws 1 object; 128 leaves a lot of headroom for more without needing a
// dynamic/growable scheme yet.
#define MAX_DRAWS_PER_FRAME 128

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

		VmaAllocator allocator = nullptr;

		std::vector<VkImage> swapchainImages;
		std::vector<VkImageView> swapchainImageViews;
		std::vector<VkFramebuffer> swapchainFramebuffers;

		VkImage depthImage = nullptr;
		VmaAllocation depthImageAllocation = nullptr;
		VkImageView depthImageView = nullptr;

		VkRenderPass renderPass = nullptr;

		// Binding layout (see res/CMakeLists.txt for why t/s registers are shifted at
		// compile time): 0 = modelConstant (vertex), 1 = worldConstant (vertex+fragment),
		// 10/11/13/14/15 = material textures (fragment; matches bsdfPixel.hlsl's
		// _BaseColor/_Metallic/_Roughness/_NormalMap/_EmissiveColorMap - t2/_Occlusion is
		// declared in the shader but never sampled, so DXC strips it; no binding 12 here
		// either), 20 = defaultSampler (fragment).
		VkDescriptorSetLayout descriptorSetLayout = nullptr;
		VkPipelineLayout pipelineLayout = nullptr;
		VkPipeline graphicsPipeline = nullptr;

		VkCommandPool commandPool = nullptr;
		std::vector<VkCommandBuffer> commandBuffers; // FRAME_LAG

		std::vector<VkSemaphore> imageAvailableSemaphores; // FRAME_LAG
		std::vector<VkSemaphore> renderFinishedSemaphores; // FRAME_LAG
		std::vector<VkFence> inFlightFences; // FRAME_LAG

		// Textures/materials can differ per draw call, so descriptor sets are allocated fresh
		// per Render() call rather than once - one pool per frame-in-flight, reset at the start
		// of that frame's BeginScene() once its previous fence has signalled.
		std::vector<VkDescriptorPool> descriptorPools; // FRAME_LAG

		ui32 currentFrame = 0;
		ui32 currentImageIndex = 0;
		ui32 drawIndexThisFrame = 0;
		// False whenever the current frame's swapchain image acquire failed/was skipped
		// (e.g. VK_ERROR_OUT_OF_DATE_KHR) - Render()/EndScene() no-op for the rest of that frame.
		bool frameActive = false;

		struct MappedBuffer
		{
			VkBuffer buffer = nullptr;
			VmaAllocation allocation = nullptr;
			void* mapped = nullptr;
		};
		// [frame][drawIndexThisFrame] - see MAX_DRAWS_PER_FRAME.
		std::vector<MappedBuffer> modelConstantBuffers; // FRAME_LAG * MAX_DRAWS_PER_FRAME
		std::vector<MappedBuffer> worldConstantBuffers; // FRAME_LAG, written once per BeginScene

		VkSampler defaultSampler = nullptr;

		// A 1x1 white pixel, bound for any material texture slot a draw call didn't set via
		// UseTexture() - Vulkan (unlike D3D11) requires every VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
		// write to reference a real, validly-laid-out image view, so there's no such thing as
		// "just leave it null" here.
		TextureHandle defaultTextureHandle;
		ShaderResourceViewHandle defaultTextureView;

		// Accumulated by UseTexture(), consumed (and cleared) by the next Render() call -
		// mirrors D3D11Renderer::textureViews.
		std::vector<ShaderResourceViewHandle> pendingTextureViews;

		DUPLEX_NS_UTIL::worldConstant worldLocalBuffer;
		std::vector<DUPLEX_NS_UTIL::GpuLight> lights;

		// Backs the generation-checked handles, same pattern as D3D11Renderer's pools (see
		// Phase 1) - just with Vulkan-native resource structs instead of a single COM pointer.
		struct VulkanBuffer
		{
			VkBuffer buffer = nullptr;
			VmaAllocation allocation = nullptr;
		};
		struct VulkanTexture
		{
			VkImage image = nullptr;
			VmaAllocation allocation = nullptr;
			VkFormat format = VK_FORMAT_UNDEFINED;
		};
		struct VulkanImageView
		{
			VkImageView view = nullptr;
		};
		HandlePool<VulkanBuffer, BufferHandle> bufferPool;
		HandlePool<VulkanTexture, TextureHandle> texturePool;
		HandlePool<VulkanImageView, ShaderResourceViewHandle> srvPool;

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

		void CreateAllocator();
		void CreateDepthResources();
		void CreateFramebuffers();
		void CreateDescriptorSetLayout();
		void CreateCommandPool();
		void CreateCommandBuffers();
		void CreateSyncObjects();
		void CreateDescriptorPools();
		void CreateDefaultSampler();
		void CreateModelConstantBuffers();
		void CreateWorldConstantBuffers();
		void CreateDefaultTexture();
		void CleanupSwapchain();

		VkShaderModule CreateShaderModuleFromFile(const char* path);
		VkFormat FromTextureFormat(TextureFormat format);

		VkCommandBuffer BeginSingleTimeCommands();
		void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
		void TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
		void CopyBufferToImage(VkBuffer buffer, VkImage image, ui32 width, ui32 height);
	};
}
