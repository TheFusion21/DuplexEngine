#include "vulkanrenderer.h"
#include "math/vec3.h"
#include "math/vec2.h"
#include "math/mat4x4.h"
#include "math/mathutils.h"
#include "utils\util.h"

using namespace DUPLEX_NS_GRAPHICS;
using namespace DUPLEX_NS_MATH;
using namespace DUPLEX_NS_UTIL;

#define VK_CHECK(expr) { \
    ASSERT(expr == VK_SUCCESS); \
}
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    //TODO: add logger messages
    switch (messageSeverity) {
    default:
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        //Logger::Error(pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        //Logger::Warn(pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        //Logger::Log(pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        //Logger::Trace(pCallbackData->pMessage);
        break;
    }

    return VK_FALSE;
}

PFN_vkGetDeviceProcAddr VulkanRenderer::g_gdpa = nullptr;

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
bool VulkanRenderer::Init(ui64 instance, ui64 handle, ui32 width, ui32 height)
{
    this->width = width;
    this->height = height;
    VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.apiVersion = VK_API_VERSION_1_2;
    appInfo.pApplicationName = "Duplex Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    VkInstanceCreateInfo instanceCreateInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instanceCreateInfo.pApplicationInfo = &appInfo;

    std::vector<const char*> platformExtensions;
    this->GetRequiredExtension(platformExtensions);

    instanceCreateInfo.enabledExtensionCount = static_cast<ui32>(platformExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = platformExtensions.data();

    
    //Get the available layers
    ui32 availableLayerCount = 0;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr));
    std::vector<VkLayerProperties> availableLayer(availableLayerCount);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayer.data()));

    bool success = true;
    for (ui32 i = 0; i < static_cast<ui32>(this->requiredValidationLayers.size()); i++)
    {
        bool found = false;
        for (ui32 j = 0; j < availableLayerCount; j++)
        {
            if (strcmp(this->requiredValidationLayers[i], availableLayer[j].layerName) == 0)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            success = false;
            throw std::string("Required validation layer is missing: %s", this->requiredValidationLayers[i]);
        }
    }
    instanceCreateInfo.enabledLayerCount = static_cast<ui32>(this->requiredValidationLayers.size());
    instanceCreateInfo.ppEnabledLayerNames = this->requiredValidationLayers.data();

    VK_CHECK(vkCreateInstance(&instanceCreateInfo, nullptr, &this->instance));

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT, VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debugCreateInfo.pfnUserCallback = debugCallback;
    debugCreateInfo.pUserData = this;

    PFN_vkCreateDebugUtilsMessengerEXT func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(this->instance, "vkCreateDebugUtilsMessengerEXT"));
    ASSERT_MSG(func, "Failed to create debug messenger");
    func(this->instance, &debugCreateInfo, nullptr, &this->debugMessenger);

    this->CreateSurface(instance, handle);
    this->physicalDevice = this->SelectPhysicalDevice();
    this->CreateLogicalDevice();
    //TODO: CREATE SHADER
    this->CreateSwapchain();
	return true;
}

void VulkanRenderer::SetViewPort()
{
}

void VulkanRenderer::CreateShader()
{
}

void VulkanRenderer::SetActiveCamera(Vec3 eye, Mat4x4 viewProj)
{
}

void VulkanRenderer::ClearLights()
{
}

void VulkanRenderer::SetLight(GpuLight lightDescriptor)
{
}

IntPtr VulkanRenderer::CreateTexture(ui32 width, ui32 height, ui32 levels, TextureFormat format, void* data)
{
	return nullptr;
}

IntPtr VulkanRenderer::CreateTextureSRV(IntPtr texture, TextureFormat format)
{
	return nullptr;
}

void VulkanRenderer::UseTexture(ui32 slot, GraphicsBufferPtr view)
{
}

void VulkanRenderer::BeginScene()
{
}

void VulkanRenderer::EndScene()
{
}

void VulkanRenderer::Render(Mat4x4 transformMat, GraphicsBufferPtr vertexBuffer, GraphicsBufferPtr indexBuffer, ui32 indexCount)
{
}

void VulkanRenderer::Shutdown()
{
}

GraphicsBufferPtr VulkanRenderer::CreateBuffer(BufferType type, const void* data, int dataSize, UsageType usage)
{
	return nullptr;
}


ShaderResourcePtr VulkanRenderer::CreateShaderResource(GraphicsBufferPtr resource, D3D11_SHADER_RESOURCE_VIEW_DESC* desc)
{
	return nullptr;
}

bool VulkanRenderer::Resize(ui32 width, ui32 height)
{
	return false;
}

bool VulkanRenderer::CheckForFullscreen()
{
	return false;
}

void VulkanRenderer::ReleaseTexture(IntPtr& texture)
{
}

void VulkanRenderer::ReleaseTextureSRV(IntPtr& srv)
{

}

void VulkanRenderer::ReleaseBuffer(IntPtr& buffer)
{
}

void VulkanRenderer::GetRequiredExtension(std::vector<const char*>& extensionNames)
{
    extensionNames.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#if defined(ENGINE_COMPILE_DEBUG)
    extensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    extensionNames.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    extensionNames.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    extensionNames.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
    extensionNames.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_DISPLAY_KHR)
    extensionNames.push_back(VK_KHR_DISPLAY_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
    extensionNames.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_METAL_EXT)
    extensionNames.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
#endif
}

void VulkanRenderer::CreateSurface(ui64 instance, ui64 handle)
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
    surfaceCreateInfo.pNext = NULL;
    surfaceCreateInfo.flags = 0;
    surfaceCreateInfo.hinstance = reinterpret_cast<HINSTANCE>(instance);
    surfaceCreateInfo.hwnd = reinterpret_cast<HWND>(handle);

    VK_CHECK(vkCreateWin32SurfaceKHR(this->instance, &surfaceCreateInfo, NULL, &this->surface));
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    VkWaylandSurfaceCreateInfoKHR surfaceCreateInfo = { VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR };
    //createInfo.pNext = NULL;
    //createInfo.flags = 0;
    //createInfo.display = demo->display;
    //createInfo.surface = demo->window;
    //
    //err = vkCreateWaylandSurfaceKHR(demo->inst, &createInfo, NULL, &demo->surface);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    VkXcbSurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR };
    //createInfo.pNext = NULL;
    //createInfo.flags = 0;
    //createInfo.connection = demo->connection;
    //createInfo.window = demo->xcb_window;

    //err = vkCreateXcbSurfaceKHR(demo->inst, &createInfo, NULL, &demo->surface);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
    VkWaylandSurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR };
    //createInfo.pNext = NULL;
    //createInfo.flags = 0;
    //createInfo.display = demo->display;
    //createInfo.surface = demo->window;
    //
    //err = vkCreateWaylandSurfaceKHR(demo->inst, &createInfo, NULL, &demo->surface);
#elif defined(VK_USE_PLATFORM_DISPLAY_KHR)

#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
    VkAndroidSurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR };
    //createInfo.pNext = NULL;
    //createInfo.flags = 0;
    //createInfo.window = (struct ANativeWindow*)(demo->window);

    //err = vkCreateAndroidSurfaceKHR(demo->inst, &createInfo, NULL, &demo->surface);
#elif defined(VK_USE_PLATFORM_METAL_EXT)
    VkMetalSurfaceCreateInfoEXT surface = { VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT };
    //surface.pNext = NULL;
    //surface.flags = 0;
    //surface.pLayer = demo->caMetalLayer;
    //
    //err = vkCreateMetalSurfaceEXT(demo->inst, &surface, NULL, &demo->surface);
#endif
}


VkPhysicalDevice VulkanRenderer::SelectPhysicalDevice()
{
    ui32 deviceCount = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(this->instance, &deviceCount, nullptr));
    if (deviceCount == 0)
    {
        throw std::string("No supported physical device were found.");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    VK_CHECK(vkEnumeratePhysicalDevices(this->instance, &deviceCount, devices.data()));

    for (ui32 i = 0; i < deviceCount; i++)
    {
        if (this->PhysicalDeviceMeetsRequirements(devices[i]))
            return devices[i];
    }
    throw std::string("No supported physical device were found.");
    return nullptr;
}

bool VulkanRenderer::PhysicalDeviceMeetsRequirements(VkPhysicalDevice physicalDevice)
{
    i32 graphicsQueueIndex = -1;
    i32 presentQueueIndex = -1;
    this->DetectQueueFamilyIndices(physicalDevice, &graphicsQueueIndex, &presentQueueIndex);

    VulkanSwapchainSupportDetails swapchainSupport = this->QuerySwapchainSupport(physicalDevice);

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(physicalDevice, &features);

    bool supportsRequiredQueueFamilies = (graphicsQueueIndex != -1) && (presentQueueIndex != -1);


    //Get All available extension for this device
    ui32 extensionCount = 0;
    VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr));
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data()));

    bool success = true;
    for (ui64 i = 0; i < this->requiredExtensions.size(); i++)
    {
        bool found = false;
        for (ui64 j = 0; j < extensionCount; j++)
        {
            if (strcmp(this->requiredExtensions[i], availableExtensions[j].extensionName) == 0)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            success = false;
            break;
        }
    }

    bool swapchainMeetsRequirements = false;
    if (supportsRequiredQueueFamilies)
    {
        swapchainMeetsRequirements = swapchainSupport.formats.size() > 0 && swapchainSupport.presentModes.size() > 0;
    }

    return supportsRequiredQueueFamilies && swapchainMeetsRequirements && features.samplerAnisotropy;
}

void VulkanRenderer::DetectQueueFamilyIndices(VkPhysicalDevice physicalDevice, int* graphicsQueueIndex, int* presentQueueIndex)
{
    ui32 queueFamiliyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliyCount, nullptr);
    std::vector<VkQueueFamilyProperties> familyProperties(queueFamiliyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliyCount, familyProperties.data());

    for (ui32 i = 0; i < queueFamiliyCount; i++)
    {
        if (familyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            *graphicsQueueIndex = i;
        }

        VkBool32 supportsPresent = VK_FALSE;
        VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, this->surface, &supportsPresent));
        if (supportsPresent)
            *presentQueueIndex = i;
    }
}

VulkanRenderer::VulkanSwapchainSupportDetails VulkanRenderer::QuerySwapchainSupport(VkPhysicalDevice physicalDevice)
{
    VulkanSwapchainSupportDetails details;

    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, this->surface, &details.capabilities));

    ui32 formatCount = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, this->surface, &formatCount, nullptr));
    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, this->surface, &formatCount, details.formats.data()));
    }

    ui32 presentModeCount = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, this->surface, &presentModeCount, nullptr));
    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, this->surface, &presentModeCount, details.presentModes.data()));
    }

    return details;
}

void VulkanRenderer::CreateLogicalDevice()
{
    i32 graphicsQueueIndex = -1;
    i32 presentQueueIndex = -1;
    DetectQueueFamilyIndices(this->physicalDevice, &graphicsQueueIndex, &presentQueueIndex);

    bool presentSharesGraphicsQueue = graphicsQueueIndex == presentQueueIndex;

    std::vector<ui32> indices;
    indices.push_back(graphicsQueueIndex);
    if (!presentSharesGraphicsQueue)
        indices.push_back(presentQueueIndex);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(indices.size());
    for (ui32 i = 0; i < indices.size(); i++)
    {
        queueCreateInfos[i] = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
        queueCreateInfos[i].queueFamilyIndex = indices[i];
        queueCreateInfos[i].queueCount = 1;
        queueCreateInfos[i].flags = 0;
        queueCreateInfos[i].pNext = nullptr;
        float queuePriority = 1.0f;
        queueCreateInfos[i].pQueuePriorities = &queuePriority;
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo deviceCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.queueCreateInfoCount = static_cast<ui32>(indices.size());
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount = static_cast<ui32>(this->requiredExtensions.size());
    deviceCreateInfo.pNext = nullptr;
    deviceCreateInfo.ppEnabledExtensionNames = this->requiredExtensions.data();

#if defined(ENGINE_COMPILE_DEBUG)
    deviceCreateInfo.enabledLayerCount = static_cast<ui32>(this->requiredValidationLayers.size());
    deviceCreateInfo.ppEnabledLayerNames = this->requiredValidationLayers.data();
#else
    deviceCreateInfo.enabledLayerCount = 0U;
    deviceCreateInfo.ppEnabledLayerNames = nullptr;
#endif
    VK_CHECK(vkCreateDevice(this->physicalDevice, &deviceCreateInfo, nullptr, &this->device));

    this->graphicsFamilyQueueIndex = graphicsQueueIndex;
    this->presentFamilyQueueIndex = presentQueueIndex;

    vkGetDeviceQueue(this->device, this->graphicsFamilyQueueIndex, 0, &this->graphicsQueue);
    vkGetDeviceQueue(this->device, this->presentFamilyQueueIndex, 0, &this->presentQueue);

}

void VulkanRenderer::CreateSwapchain()
{
    VulkanSwapchainSupportDetails swapchainSupport = QuerySwapchainSupport(this->physicalDevice);
    VkSurfaceCapabilitiesKHR capabilities = swapchainSupport.capabilities;


    //pick a format by specification
    bool found = false;
    for (VkSurfaceFormatKHR format : swapchainSupport.formats)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            this->swapchainImageFormat = format;
            found = true;
            break;
        }
    }
    if (!found)
    {
        this->swapchainImageFormat = swapchainSupport.formats[0];
    }

    //pick a present mode by specification
    VkPresentModeKHR presentMode;
    found = false;
    for (VkPresentModeKHR mode : swapchainSupport.presentModes)
    {
        /* Check if the modes contains a MailBox mode
         * VK_PRESENT_MODE_MAILBOX_KHR -> An internal single-entry queue is used to hold pending presentation request
         */
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            presentMode = mode;
            found = true;
        }
    }
    if (!found)
    {
        /* No Mailbox Mode found so we fallback to
         * VK_PRESENT_MODE_FIFO_KHR -> An internal queue is used to hold pending presentation requests
         */
        presentMode = VK_PRESENT_MODE_FIFO_KHR;
    }

    if (capabilities.currentExtent.width != UI32MAX)
    {
        this->swapchainExtent = capabilities.currentExtent;
    }
    else
    {
        this->swapchainExtent = { this->width, this->height };

        //Clamp the dimensions to values supported by GPU
        this->swapchainExtent.width = Clamp(this->swapchainExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        this->swapchainExtent.height = Clamp(this->swapchainExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }

    ui32 imageCount = capabilities.minImageCount + 1;

    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
    {
        imageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchainCreateInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swapchainCreateInfo.surface = this->surface;
    swapchainCreateInfo.minImageCount = imageCount;
    swapchainCreateInfo.imageFormat = this->swapchainImageFormat.format;
    swapchainCreateInfo.imageColorSpace = this->swapchainImageFormat.colorSpace;
    swapchainCreateInfo.imageExtent = this->swapchainExtent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (this->graphicsFamilyQueueIndex != this->presentFamilyQueueIndex)
    {
        ui32 queueFamiliyIndices[] = { static_cast<ui32>(this->graphicsFamilyQueueIndex),static_cast<ui32>(this->presentFamilyQueueIndex) };
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = queueFamiliyIndices;
    }
    else
    {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 0;
        swapchainCreateInfo.pQueueFamilyIndices = nullptr;
    }

    swapchainCreateInfo.preTransform = capabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = nullptr;

    VK_CHECK(vkCreateSwapchainKHR(this->device, &swapchainCreateInfo, nullptr, &this->swapchain));
}
void VulkanRenderer::CreateSwapchainImagesAndViews()
{

}
void VulkanRenderer::CreateRenderPass()
{

}