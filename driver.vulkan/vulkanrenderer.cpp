#include "vulkanrenderer.h"
#include "math/vec3.h"
#include "math/vec2.h"
#include "math/mat4x4.h"
#include "utils\util.h"

PFN_vkGetDeviceProcAddr Engine::Graphics::VulkanRenderer::g_gdpa = nullptr;

#define GET_INSTANCE_PROC_ADDR(inst, entrypoint)																		\
    {																													\
        this->fp##entrypoint = (PFN_vk##entrypoint)vkGetInstanceProcAddr(inst, "vk" #entrypoint);						\
        if (this->fp##entrypoint == nullptr) {																			\
            return false;																								\
        }																												\
    }
#define GET_DEVICE_PROC_ADDR(dev, entrypoint)																			\
    {																													\
        if (!this->g_gdpa) this->g_gdpa = (PFN_vkGetDeviceProcAddr)vkGetInstanceProcAddr(this->instance, "vkGetDeviceProcAddr");	\
        this->fp##entrypoint = (PFN_vk##entrypoint)this->g_gdpa(dev, "vk" #entrypoint);										\
        if (this->fp##entrypoint == NULL) {																				\
            return false;																								\
        }																												\
    }
bool Engine::Graphics::VulkanRenderer::Init(ui64 instance, ui64 handle, ui32 width, ui32 height)
{
	//INIT VK
	{
		ui32 instanceExtensionCount = 0;
		ui32 instanceLayerCount = 0;
		const char* instanceValidationLayers[] = { "VK_LAYER_KHRONOS_validation" };

		VkBool32 surfaceExtFound = 0;
		VkBool32 platformSurfaceExtFound = 0;
		memset(this->extensionNames, 0, sizeof(this->extensionNames));
		if (FAILED(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr)))
		{
			MessageBoxA(NULL, "Could not enumerate extensions", "ERROR", MB_OK | MB_ICONEXCLAMATION);
			return false;
		}

		if (instanceExtensionCount > 0)
		{
			VkExtensionProperties* instanceExtensions = new VkExtensionProperties[instanceExtensionCount];
			if (FAILED(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, instanceExtensions)))
			{
				MessageBoxA(NULL, "Could not get extension properties", "ERROR", MB_OK | MB_ICONEXCLAMATION);
				return false;
			}
			for (ui32 i = 0; i < instanceExtensionCount; i++)
			{
				if (!strcmp(VK_KHR_SURFACE_EXTENSION_NAME, instanceExtensions[i].extensionName))
				{
					surfaceExtFound = 1;
					this->extensionNames[this->enabledExtensionCount++] = VK_KHR_SURFACE_EXTENSION_NAME;
				}
#if defined(VK_USE_PLATFORM_WIN32_KHR)
				if (!strcmp(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, instanceExtensions[i].extensionName))
				{
					platformSurfaceExtFound = 1;
					this->extensionNames[this->enabledExtensionCount++] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
				}
#else

#endif
				if (!strcmp(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, instanceExtensions[i].extensionName))
				{
					if (this->validate) {
						this->extensionNames[this->enabledExtensionCount++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
					}
				}
			}
			SAFEDELETEARR(instanceExtensions);
		}
		if (!surfaceExtFound)
		{
			MessageBoxA(NULL, "No " VK_KHR_SURFACE_EXTENSION_NAME "found", "ERROR", MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
		if (!platformSurfaceExtFound)
		{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
			MessageBoxA(NULL, "No " VK_KHR_WIN32_SURFACE_EXTENSION_NAME "found", "ERROR", MB_OK | MB_ICONEXCLAMATION);
#else

#endif
			return false;
		}
		VkApplicationInfo app = {};
		app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app.pNext = nullptr;
		app.pApplicationName = "DuplexEngineGame";
		app.applicationVersion = 0;
		app.pEngineName = "DuplexEngine";
		app.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo instInfo = {};
		instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instInfo.pNext = nullptr;
		instInfo.pApplicationInfo = &app;
		instInfo.enabledLayerCount = this->enabledLayerCount;
		instInfo.ppEnabledLayerNames = static_cast<const char* const*>(instanceValidationLayers);
		instInfo.enabledExtensionCount = this->enabledExtensionCount;
		instInfo.ppEnabledExtensionNames = this->extensionNames;

		ui32 gpuCount = 0;

		if (FAILED(vkCreateInstance(&instInfo, nullptr, &this->instance)))
		{
			MessageBoxA(NULL, "Could not create vulkan instance", "ERROR", MB_OK | MB_ICONEXCLAMATION);
			return false;
		}

		if (FAILED(vkEnumeratePhysicalDevices(this->instance, &gpuCount, nullptr)))
		{
			MessageBoxA(NULL, "Could not enumerate devices", "ERROR", MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
		if (gpuCount == 0)
		{
			MessageBoxA(NULL, "Could not find compatible devices", "ERROR", MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
		VkPhysicalDevice* physicalDevices = new VkPhysicalDevice[gpuCount];
		if (FAILED(vkEnumeratePhysicalDevices(this->instance, &gpuCount, physicalDevices)))
		{
			MessageBoxA(NULL, "Could not get any accessible devices", "ERROR", MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
		this->physicalDevice = physicalDevices[0];
		SAFEDELETEARR(physicalDevices);

		/* Look for device extensions */
		ui32 deviceExtensionCount = 0;
		VkBool32 swapchainExtFound = 0;
		this->enabledExtensionCount = 0;
		memset(this->extensionNames, 0, sizeof(this->extensionNames));
		if (FAILED(vkEnumerateDeviceExtensionProperties(this->physicalDevice, nullptr, &deviceExtensionCount, nullptr)))
		{
			MessageBoxA(NULL, "Could not get any extensions", "ERROR", MB_OK | MB_ICONEXCLAMATION);
			return false;
		}

		if (deviceExtensionCount > 0)
		{
			VkExtensionProperties* deviceExtensions = new VkExtensionProperties[deviceExtensionCount];
			if (FAILED(vkEnumerateDeviceExtensionProperties(this->physicalDevice, nullptr, &deviceExtensionCount, deviceExtensions)))
			{
				MessageBoxA(NULL, "Could not get any extensions properties", "ERROR", MB_OK | MB_ICONEXCLAMATION);
				return false;
			}
			for (ui32 i = 0; i < deviceExtensionCount; i++)
			{
				if (!strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME, deviceExtensions[i].extensionName))
				{
					swapchainExtFound = 1;
					this->extensionNames[this->enabledExtensionCount++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
				}
			}
		}
		if (!swapchainExtFound)
		{
			MessageBoxA(NULL, "Could not find " VK_KHR_SWAPCHAIN_EXTENSION_NAME, "ERROR", MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
		vkGetPhysicalDeviceProperties(this->physicalDevice, &this->deviceProps);
		printf("Graphics Device: %s\n", this->deviceProps.deviceName);
		vkGetPhysicalDeviceQueueFamilyProperties(this->physicalDevice, &this->queueFamilyCount, nullptr);

		queueProps = new VkQueueFamilyProperties[this->queueFamilyCount];
		vkGetPhysicalDeviceQueueFamilyProperties(this->physicalDevice, &this->queueFamilyCount, this->queueProps);

		// Query fine-grained feature support for this device.
		//  If app has specific feature requirements it should check supported
		//  features based on this query
		VkPhysicalDeviceFeatures physDevFeatures;
		vkGetPhysicalDeviceFeatures(this->physicalDevice, &physDevFeatures);

		GET_INSTANCE_PROC_ADDR(this->instance, GetPhysicalDeviceSurfaceSupportKHR);
		GET_INSTANCE_PROC_ADDR(this->instance, GetPhysicalDeviceSurfaceCapabilitiesKHR);
		GET_INSTANCE_PROC_ADDR(this->instance, GetPhysicalDeviceSurfaceFormatsKHR);
		GET_INSTANCE_PROC_ADDR(this->instance, GetPhysicalDeviceSurfacePresentModesKHR);
		GET_INSTANCE_PROC_ADDR(this->instance, GetSwapchainImagesKHR);
	}

	//INIT SURFACE AND QUEUES
	{
		//Create Win32 Surface
#if defined(VK_USE_PLATFORM_WIN32_KHR)
		VkWin32SurfaceCreateInfoKHR createInfo;
		createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.hinstance = reinterpret_cast<HINSTANCE>(instance);
		createInfo.hwnd = reinterpret_cast<HWND>(handle);

		if (FAILED(vkCreateWin32SurfaceKHR(this->instance, &createInfo, nullptr, &this->surface)))
		{
			MessageBoxA(NULL, "Could not create Windows surface", "ERROR", MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
#else
		MessageBoxA(NULL, "No implementation of Vulkan available on this platform", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
#endif
	}
	//INIT SWAPCHAIN
	{
		// Iterate over each queue to learn whether it supports presenting:

		VkBool32* supportsPresent = new VkBool32[this->queueFamilyCount];
		for (ui32 i = 0; i < this->queueFamilyCount; i++)
		{
			this->fpGetPhysicalDeviceSurfaceSupportKHR(this->physicalDevice, i, this->surface, &supportsPresent[i]);
		}
		// Search for a graphics and a present queue in the array of queue
		// families, try to find one that supports both

		ui32 graphicsQueueFamilyIndex = UINT32_MAX;
		ui32 presentQueueFamilyIndex = UINT32_MAX;
		for (ui32 i = 0; i < this->queueFamilyCount; i++)
		{
			if ((this->queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
			{
				if (graphicsQueueFamilyIndex == UINT32_MAX)
					graphicsQueueFamilyIndex = i;
				if (supportsPresent[i] == VK_TRUE)
				{
					graphicsQueueFamilyIndex = i;
					presentQueueFamilyIndex = i;
					break;
				}
			}
		}
		// If didn't find a queue that supports both graphics and present, then
		// find a separate present queue.

		if (presentQueueFamilyIndex == UINT32_MAX)
		{
			for (ui32 i = 0; i < this->queueFamilyCount; i++)
			{
				if (supportsPresent[i] == VK_TRUE)
				{
					presentQueueFamilyIndex = i;
					break;
				}
			}
		}
		if (graphicsQueueFamilyIndex == UINT32_MAX || presentQueueFamilyIndex == UINT32_MAX)
		{
			MessageBoxA(NULL, "Could not find any graphics and present queues", "ERROR", MB_OK | MB_ICONEXCLAMATION);
			return false;
		}

		this->graphicsQueueFamilyIndex = graphicsQueueFamilyIndex;
		this->presentQueueFamilyIndex = presentQueueFamilyIndex;
		this->seperatePresentQueue = (this->graphicsQueueFamilyIndex != this->presentQueueFamilyIndex);

		SAFEDELETEARR(supportsPresent);

	}
	//CREATE DEVICE
	{
		float queuePriorities[1] = { 0.0 };
		VkDeviceQueueCreateInfo queues[2];
		queues[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queues[0].pNext = nullptr;
		queues[0].queueFamilyIndex = this->graphicsQueueFamilyIndex;
		queues[0].queueCount = 1;
		queues[0].pQueuePriorities = queuePriorities;
		queues[0].flags = 0;

		VkDeviceCreateInfo deviceInfo = {};
		deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceInfo.pNext = nullptr;
		deviceInfo.queueCreateInfoCount = 1;
		deviceInfo.pQueueCreateInfos = queues;
		deviceInfo.enabledLayerCount = 0;
		deviceInfo.ppEnabledLayerNames = nullptr;
		deviceInfo.enabledExtensionCount = this->enabledExtensionCount;
		deviceInfo.ppEnabledExtensionNames = this->extensionNames;
		deviceInfo.pEnabledFeatures = nullptr;  // If specific features are required, pass them in here

		if (this->seperatePresentQueue)
		{
			queues[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queues[1].pNext = nullptr;
			queues[1].queueFamilyIndex = this->presentQueueFamilyIndex;
			queues[1].queueCount = 1;
			queues[1].pQueuePriorities = queuePriorities;
			queues[1].flags = 0;
			deviceInfo.queueCreateInfoCount = 2;
		}

		if (FAILED(vkCreateDevice(this->physicalDevice, &deviceInfo, nullptr, &this->device)))
		{
			MessageBoxA(NULL, "Could not create device link", "ERROR", MB_OK | MB_ICONEXCLAMATION);
			return false;
		}

		GET_DEVICE_PROC_ADDR(this->device, CreateSwapchainKHR);
		GET_DEVICE_PROC_ADDR(this->device, DestroySwapchainKHR);
		GET_DEVICE_PROC_ADDR(this->device, GetSwapchainImagesKHR);
		GET_DEVICE_PROC_ADDR(this->device, AcquireNextImageKHR);
		GET_DEVICE_PROC_ADDR(this->device, QueuePresentKHR);
	}

	vkGetDeviceQueue(this->device, this->graphicsQueueFamilyIndex, 0, &this->graphicsQueue);

	if (!this->seperatePresentQueue)
	{
		this->presentQueue = this->graphicsQueue;
	}
	else
	{
		vkGetDeviceQueue(this->device, this->presentQueueFamilyIndex, 0, &this->presentQueue);
	}

	ui32 formatCount = 0;

	if (FAILED(this->fpGetPhysicalDeviceSurfaceFormatsKHR(this->physicalDevice, this->surface, &formatCount, nullptr)))
	{
		MessageBoxA(NULL, "Could not get format count", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	VkSurfaceFormatKHR* surfFormats = new VkSurfaceFormatKHR[formatCount];
	if (FAILED(this->fpGetPhysicalDeviceSurfaceFormatsKHR(this->physicalDevice, this->surface, &formatCount, surfFormats)))
	{
		MessageBoxA(NULL, "Could not get formats", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	VkSurfaceFormatKHR surfaceFormat = {};
	for (ui32 i = 0; i < formatCount; i++)
	{
		const VkFormat format = surfFormats[i].format;

		if (format == VK_FORMAT_R8G8B8A8_UNORM || format == VK_FORMAT_B8G8R8A8_UNORM ||
			format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 || format == VK_FORMAT_A2R10G10B10_UNORM_PACK32 ||
			format == VK_FORMAT_R16G16B16A16_SFLOAT)
		{
			surfaceFormat = surfFormats[i];
			break;
		}
	}
	this->format = surfaceFormat.format;
	this->colorSpace = surfaceFormat.colorSpace;
	SAFEDELETEARR(surfFormats);

	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = NULL;
	semaphoreCreateInfo.flags = 0;

	VkFenceCreateInfo fenceCreate = {};
	fenceCreate.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreate.pNext = NULL;
	fenceCreate.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (ui32 i = 0; i < FRAME_LAG; i++)
	{
		if (FAILED(vkCreateFence(this->device, &fenceCreate, nullptr, &this->fences[i])))
		{
			MessageBoxA(NULL, "Could not create fence", "ERROR", MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
		if (FAILED(vkCreateSemaphore(this->device, &semaphoreCreateInfo, nullptr, &this->imageOwnershipSemaphores[i])))
		{
			MessageBoxA(NULL, "Could not create semaphore", "ERROR", MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
		if (FAILED(vkCreateSemaphore(this->device, &semaphoreCreateInfo, nullptr, &this->drawCompleteSemaphores[i])))
		{
			MessageBoxA(NULL, "Could not create semaphore", "ERROR", MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
		if (this->seperatePresentQueue)
		{
			if (FAILED(vkCreateSemaphore(this->device, &semaphoreCreateInfo, nullptr, &this->imageOwnershipSemaphores[i])))
			{
				MessageBoxA(NULL, "Could not create semaphore", "ERROR", MB_OK | MB_ICONEXCLAMATION);
				return false;
			}
		}
	}
	vkGetPhysicalDeviceMemoryProperties(this->physicalDevice, &this->memoryProperties);
	printf("Graphics available Memory: %d MB\n", static_cast<ui32>(this->memoryProperties.memoryHeaps[0].size * 9.5367E-7f));

	VkCommandPoolCreateInfo cmdPoolCI = {};
	cmdPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmdPoolCI.pNext = nullptr;
	cmdPoolCI.queueFamilyIndex = this->graphicsQueueFamilyIndex;
	cmdPoolCI.flags = 0;

	if (FAILED(vkCreateCommandPool(this->device, &cmdPoolCI, nullptr, &this->cmdPool)))
	{
		MessageBoxA(NULL, "Could not create command pool", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	VkCommandBufferAllocateInfo cmdAI = {};
	cmdAI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdAI.pNext = nullptr;
	cmdAI.commandPool = this->cmdPool;
	cmdAI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdAI.commandBufferCount = 1;
	if (FAILED(vkAllocateCommandBuffers(this->device, &cmdAI, &this->cmdBuffer)))
	{
		MessageBoxA(NULL, "Could not allocate command buffer", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	VkCommandBufferBeginInfo cmdBI = {};
	cmdBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBI.pNext = nullptr;
	cmdBI.flags = 0;
	cmdBI.pInheritanceInfo = nullptr;

	if (FAILED(vkBeginCommandBuffer(this->cmdBuffer, &cmdBI)))
	{
		MessageBoxA(NULL, "Could not begin command buffer", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	VkSwapchainKHR oldSwapchain = this->swapchain;

	VkSurfaceCapabilitiesKHR surfCapabilities;

	if (FAILED(this->fpGetPhysicalDeviceSurfaceCapabilitiesKHR(this->physicalDevice, this->surface, &surfCapabilities)))
	{
		MessageBoxA(NULL, "Could not retrieve surface Capabilities", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	ui32 presentModeCount;
	if (FAILED(this->fpGetPhysicalDeviceSurfacePresentModesKHR(this->physicalDevice, this->surface, &presentModeCount, nullptr)))
	{
		MessageBoxA(NULL, "Could not retrieve present mode count", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	VkPresentModeKHR* presentModes = new VkPresentModeKHR[presentModeCount];
	if (FAILED(this->fpGetPhysicalDeviceSurfacePresentModesKHR(this->physicalDevice, this->surface, &presentModeCount, presentModes)))
	{
		MessageBoxA(NULL, "Could not retrieve present modes", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	VkExtent2D swapchainExtent;
	if (surfCapabilities.currentExtent.width == 0xFFFFFFFF)
	{
		swapchainExtent.width = width;
		swapchainExtent.height = height;

		if (swapchainExtent.width < surfCapabilities.minImageExtent.width)
		{
			swapchainExtent.width = surfCapabilities.minImageExtent.width;
		}
		else if (swapchainExtent.width > surfCapabilities.maxImageExtent.width)
		{
			swapchainExtent.width = surfCapabilities.maxImageExtent.width;
		}

		if (swapchainExtent.height < surfCapabilities.minImageExtent.height)
		{
			swapchainExtent.height = surfCapabilities.minImageExtent.height;
		}
		else if (swapchainExtent.height > surfCapabilities.maxImageExtent.height)
		{
			swapchainExtent.height = surfCapabilities.maxImageExtent.height;
		}
	}
	else
	{
		swapchainExtent = surfCapabilities.currentExtent;
		width = surfCapabilities.currentExtent.width;
		height = surfCapabilities.currentExtent.height;
	}
	VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

	if (this->presentMode != swapchainPresentMode)
	{
		for (size_t i = 0; i < presentModeCount; ++i) {
			if (presentModes[i] == this->presentMode) {
				swapchainPresentMode = this->presentMode;
				break;
			}
		}
	}
	if (swapchainPresentMode != this->presentMode)
	{
		MessageBoxA(NULL, "Present mode is not supported", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	ui32 desiredSwapchainImageCount = 2;
	if (desiredSwapchainImageCount < surfCapabilities.minImageCount)
	{
		desiredSwapchainImageCount = surfCapabilities.minImageCount;
	}

	if ((surfCapabilities.maxImageCount > 0) && (desiredSwapchainImageCount > surfCapabilities.maxImageCount))
	{
		desiredSwapchainImageCount = surfCapabilities.maxImageCount;
	}

	VkSurfaceTransformFlagBitsKHR preTransform;
	if (surfCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else {
		preTransform = surfCapabilities.currentTransform;
	}

	// Find a supported composite alpha mode - one of these is guaranteed to be set
	VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	VkCompositeAlphaFlagBitsKHR compositeAlphaFlags[4] = {
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
	};
	for (uint32_t i = 0; i < ARRAY_SIZE(compositeAlphaFlags); i++) {
		if (surfCapabilities.supportedCompositeAlpha & compositeAlphaFlags[i]) {
			compositeAlpha = compositeAlphaFlags[i];
			break;
		}
	}

	VkSwapchainCreateInfoKHR swapchainCI = {};
	swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCI.pNext = NULL;
	swapchainCI.surface = this->surface;
	swapchainCI.minImageCount = desiredSwapchainImageCount;
	swapchainCI.imageFormat = this->format;
	swapchainCI.imageColorSpace = this->colorSpace;
	swapchainCI.imageExtent.width = swapchainExtent.width;
	swapchainCI.imageExtent.height = swapchainExtent.height;
	swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCI.preTransform = preTransform;
	swapchainCI.compositeAlpha = compositeAlpha;
	swapchainCI.imageArrayLayers = 1;
	swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCI.queueFamilyIndexCount = 0;
	swapchainCI.pQueueFamilyIndices = NULL;
	swapchainCI.presentMode = swapchainPresentMode;
	swapchainCI.oldSwapchain = oldSwapchain;
	swapchainCI.clipped = true;

	ui32 i = 0;
	if (FAILED(this->fpCreateSwapchainKHR(this->device, &swapchainCI, nullptr, &this->swapchain)))
	{
		MessageBoxA(NULL, "Could not create swapchain", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	if (oldSwapchain != VK_NULL_HANDLE)
	{
		this->fpDestroySwapchainKHR(this->device, oldSwapchain, nullptr);
	}

	if (FAILED(this->fpGetSwapchainImagesKHR(this->device, this->swapchain, &this->swapchainImageCount, nullptr)))
	{
		MessageBoxA(NULL, "Could not retrieve swapchain size", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	VkImage* swapchainImages = new VkImage[this->swapchainImageCount];

	if (FAILED(this->fpGetSwapchainImagesKHR(this->device, this->swapchain, &this->swapchainImageCount, swapchainImages)))
	{
		MessageBoxA(NULL, "Could not retrieve swapchain images", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}


	return true;
}

void Engine::Graphics::VulkanRenderer::SetViewPort()
{
}

void Engine::Graphics::VulkanRenderer::CreateShader()
{
}

void Engine::Graphics::VulkanRenderer::SetActiveCamera(Engine::Math::Vec3 eye, Engine::Math::Mat4x4 viewProj)
{
}

void Engine::Graphics::VulkanRenderer::BeginScene()
{
}

void Engine::Graphics::VulkanRenderer::EndScene()
{
}

void Engine::Graphics::VulkanRenderer::Render(Engine::Math::Mat4x4 transformMat, GraphicsBufferPtr vertexBuffer, GraphicsBufferPtr indexBuffer, ui32 indexCount)
{
}

void Engine::Graphics::VulkanRenderer::Shutdown()
{
}

GraphicsBufferPtr Engine::Graphics::VulkanRenderer::CreateBuffer(BufferType type, const void* data, int dataSize, UsageType usage)
{
	return GraphicsBufferPtr();
}


ShaderResourcePtr Engine::Graphics::VulkanRenderer::CreateShaderResource(GraphicsBufferPtr resource, D3D11_SHADER_RESOURCE_VIEW_DESC* desc)
{
	return ShaderResourcePtr();
}

bool Engine::Graphics::VulkanRenderer::Resize(ui32 width, ui32 height)
{
	return false;
}

bool Engine::Graphics::VulkanRenderer::CheckForFullscreen()
{
	return false;
}
