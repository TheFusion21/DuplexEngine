#include "vulkanrenderer.h"
#include "math/vec3.h"
#include "math/vec2.h"
#include "math/mat4x4.h"
#include "utils\util.h"

#define FRAME_LAG 2

bool Engine::Graphics::VulkanRenderer::Init(ui64 instance, ui64 handle, ui32 width, ui32 height)
{
	ui32 instanceExtensionCount = 0;
	ui32 instanceLayerCount = 0;
	const char* instanceValidationLayers[] = { "VK_LAYER_KHRONOS_validation" };

	VkBool32 surfaceExtFound = 0;
	VkBool32 platformSurfaceExtFound = 0;
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

	if (FAILED(vkCreateInstance(&instInfo, nullptr, &this->instance)))
	{
		MessageBoxA(NULL, "Could not create vulkan instance", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	ui32 gpuCount = 0;
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
	VkPhysicalDevice* devices = new VkPhysicalDevice[gpuCount];
	if (FAILED(vkEnumeratePhysicalDevices(this->instance, &gpuCount, devices)))
	{
		MessageBoxA(NULL, "Could not get any accessible devices", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	this->physicalDevice = devices[0];
	SAFEDELETEARR(devices);

	ui32 deviceExtensionCount = 0;
	if (FAILED(vkEnumerateDeviceExtensionProperties(this->physicalDevice, nullptr, &deviceExtensionCount, nullptr)))
	{
		MessageBoxA(NULL, "Could not get any extensions", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	uint32_t device_extension_count = 0;
	VkBool32 swapchainExtFound = 0;
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

	vkGetPhysicalDeviceQueueFamilyProperties(this->physicalDevice, &this->queueFamilyCount, nullptr);

	queueProps = new VkQueueFamilyProperties[this->queueFamilyCount];
	vkGetPhysicalDeviceQueueFamilyProperties(this->physicalDevice, &this->queueFamilyCount, this->queueProps);

	// Query fine-grained feature support for this device.
	//  If app has specific feature requirements it should check supported
	//  features based on this query
	VkPhysicalDeviceFeatures physDevFeatures;
	vkGetPhysicalDeviceFeatures(this->physicalDevice, &physDevFeatures);


#if defined(VK_USE_PLATFORM_WIN32_KHR)
	VkWin32SurfaceCreateInfoKHR createInfo;
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.hinstance = reinterpret_cast<HINSTANCE>(instance);
	createInfo.hwnd = reinterpret_cast<HWND>(handle);

	if (FAILED(vkCreateWin32SurfaceKHR(this->instance, &createInfo, NULL, &this->surface)))
	{
		MessageBoxA(NULL, "Could not create Windows surface", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
#else
	MessageBoxA(NULL, "No implementation of Vulkan available on this platform", "ERROR", MB_OK | MB_ICONEXCLAMATION);
	return false;
#endif

	vkGetDeviceQueue(this->device, this->graphicsQueueFamilyIndex, 0, &this->graphicsQueue);

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

void Engine::Graphics::VulkanRenderer::Render(const GameObject* object)
{
}

void Engine::Graphics::VulkanRenderer::Shutdown()
{
}

GraphicsBufferPtr Engine::Graphics::VulkanRenderer::CreateBuffer(BufferType type, const void* data, int dataSize, UsageType usage)
{
	return GraphicsBufferPtr();
}

GraphicsBufferPtr Engine::Graphics::VulkanRenderer::CreateTexture2D(BufferType type, DXGI_FORMAT format, const void* data, ui32 width, ui32 height, ui32 pitch, UsageType usage)
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
