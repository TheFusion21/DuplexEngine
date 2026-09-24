#include "vulkanrenderer.h"
#include "math/vec3.h"
#include "math/vec2.h"
#include "math/mat4x4.h"
#include "math/mathutils.h"
#include "utils/util.h"
#include "vertex.h"
#include <cstring>
#include <cstdio>
#include <SDL.h>
#include <SDL_vulkan.h>
#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_vulkan.h>

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
    // Was entirely commented out ("TODO: add logger messages") - every validation
    // error/warning was being silently swallowed. There's no Logger class in this codebase to
    // call into, so this prints directly; replace if one gets added later.
    const char* prefix = "Vulkan";
    switch (messageSeverity) {
    default:
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        prefix = "Vulkan ERROR";
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        prefix = "Vulkan WARNING";
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        prefix = "Vulkan INFO";
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        prefix = "Vulkan VERBOSE";
        break;
    }
    fprintf(stderr, "[%s] %s\n", prefix, pCallbackData->pMessage);

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
bool VulkanRenderer::Init(SDL_Window* window, ui32 width, ui32 height)
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
    this->GetRequiredExtension(window, platformExtensions);

    instanceCreateInfo.enabledExtensionCount = static_cast<ui32>(platformExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = platformExtensions.data();


    //Get the available layers
    ui32 availableLayerCount = 0;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr));
    std::vector<VkLayerProperties> availableLayer(availableLayerCount);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayer.data()));

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
            // NOTE: was `std::string("...: %s", requiredValidationLayers[i])` - std::string
            // has no printf-style constructor; that silently compiled anyway via an implicit
            // const char* -> bool -> size_t conversion chain, truncating the thrown string to
            // 0-1 characters and losing the actual message.
            throw std::string("Required validation layer is missing: ") + this->requiredValidationLayers[i];
        }
    }
    instanceCreateInfo.enabledLayerCount = static_cast<ui32>(this->requiredValidationLayers.size());
    instanceCreateInfo.ppEnabledLayerNames = this->requiredValidationLayers.data();

    VK_CHECK(vkCreateInstance(&instanceCreateInfo, nullptr, &this->instance));

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    // NOTE: was "A | B, C" (comma operator - only A|B ever took effect, C was evaluated and
    // discarded, so INFO-severity messages were silently never requested).
    debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debugCreateInfo.pfnUserCallback = debugCallback;
    debugCreateInfo.pUserData = this;

    PFN_vkCreateDebugUtilsMessengerEXT func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(this->instance, "vkCreateDebugUtilsMessengerEXT"));
    ASSERT_MSG(func, "Failed to create debug messenger");
    func(this->instance, &debugCreateInfo, nullptr, &this->debugMessenger);

    this->CreateSurface(window);
    this->physicalDevice = this->SelectPhysicalDevice();
    this->CreateLogicalDevice();
    this->CreateAllocator();
    this->CreateSwapchain();
    this->CreateSwapchainImagesAndViews();
    this->CreateRenderPass();
    this->CreateDepthResources();
    this->CreateFramebuffers();
    this->CreateDescriptorSetLayout();
    this->CreateCommandPool();
    this->CreateCommandBuffers();
    this->CreateSyncObjects();
    this->CreateDescriptorPools();
    this->CreateDefaultSampler();
    this->CreateModelConstantBuffers();
    this->CreateWorldConstantBuffers();
    this->CreateDefaultTexture();
    this->CreateShader();
	return true;
}

void VulkanRenderer::SetViewPort()
{
    // Vulkan's viewport/scissor are set per-command-buffer (see Render()), not as persistent
    // device state like D3D11's RSSetViewports - nothing to do here.
}

void VulkanRenderer::GetRequiredExtension(SDL_Window* window, std::vector<const char*>& extensionNames)
{
    // SDL already knows which surface extension the current platform needs (win32/xlib/
    // wayland/...) and returns VK_KHR_surface plus that one - no more per-platform
    // VK_USE_PLATFORM_* branching (the previous version's non-Win32 branches were unfinished
    // stubs that created nothing).
    ui32 sdlExtensionCount = 0;
    SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, nullptr);
    ui32 offset = static_cast<ui32>(extensionNames.size());
    extensionNames.resize(offset + sdlExtensionCount);
    SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, extensionNames.data() + offset);

#if defined(ENGINE_COMPILE_DEBUG)
    extensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
}

void VulkanRenderer::CreateSurface(SDL_Window* window)
{
    if (!SDL_Vulkan_CreateSurface(window, this->instance, &this->surface))
    {
        throw std::string(SDL_GetError());
    }
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

    // Same shape as the swapchain's queueFamiliyIndices fix below: pQueuePriorities is only read
    // later by vkCreateDevice, so it can't point at a loop-scoped local that's already gone out
    // of scope by then. One shared priority value works fine here since every queue in
    // queueCreateInfos uses the same 1.0f.
    float queuePriority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(indices.size());
    for (ui32 i = 0; i < indices.size(); i++)
    {
        queueCreateInfos[i] = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
        queueCreateInfos[i].queueFamilyIndex = indices[i];
        queueCreateInfos[i].queueCount = 1;
        queueCreateInfos[i].flags = 0;
        queueCreateInfos[i].pNext = nullptr;
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

    // Declared here, not inside the if-block below, because swapchainCreateInfo.
    // pQueueFamilyIndices is only actually read later by vkCreateSwapchainKHR - a local scoped to
    // the if-block would go out of scope before that read, leaving a dangling pointer. Confirmed
    // as a real, live bug (not just theoretical UB): on hardware where the graphics and present
    // queue families actually differ, this exact backend produced this exact symptom - the
    // Vulkan validation layer flagged garbage queueFamilyIndices values reaching
    // vkCreateSwapchainKHR, and it crashed shortly after.
    ui32 queueFamiliyIndices[] = { static_cast<ui32>(this->graphicsFamilyQueueIndex), static_cast<ui32>(this->presentFamilyQueueIndex) };
    if (this->graphicsFamilyQueueIndex != this->presentFamilyQueueIndex)
    {
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


VkShaderModule VulkanRenderer::CreateShaderModuleFromFile(const char* path)
{
    // Both branches below were an uncaught throw std::string(...) - nothing catches exceptions
    // this deep in Init(), so either crashed silently instead of explaining itself. This is the
    // exact failure a wrong working directory produces (e.g. double-clicking the exe in
    // Explorer, which sets CWD to its own folder, not the repo root this relative path assumes).
    FILE* file = fopen(path, "rb");
    if (file == nullptr)
    {
        // fprintf(stderr, ...), not MessageBoxA - this file builds on Linux too (see
        // debugCallback above for the same convention already used for Vulkan validation
        // messages), unlike D3D11Renderer/D3D12Renderer's identical fix for this same bug.
        fprintf(stderr, "Could not find %s - make sure the working directory is the repo root (e.g. run from a terminal cd'd there), not wherever the executable itself lives.\n", path);
        return nullptr;
    }
    fseek(file, 0, SEEK_END);
    long len = ftell(file);
    rewind(file);
    std::vector<char> spirv(len);
    if (fread(spirv.data(), 1, len, file) != static_cast<size_t>(len))
    {
        fclose(file);
        fprintf(stderr, "Failed to read %s\n", path);
        return nullptr;
    }
    fclose(file);

    VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    createInfo.codeSize = spirv.size();
    createInfo.pCode = reinterpret_cast<const ui32*>(spirv.data());

    VkShaderModule module = nullptr;
    VK_CHECK(vkCreateShaderModule(this->device, &createInfo, nullptr, &module));
    return module;
}

void VulkanRenderer::CreateShader()
{
    // Vulkan consumes the SPIR-V res/CMakeLists.txt already compiles directly - no
    // SPIRV-Cross round trip needed (that's only for the D3D11 path, see
    // d3d11renderer.cpp CreateShader()).
    VkShaderModule vertexModule = CreateShaderModuleFromFile("./bin/data/shd/bsdfVertex.spirv");
    VkShaderModule fragmentModule = CreateShaderModuleFromFile("./bin/data/shd/bsdfPixel.spirv");
    if (vertexModule == nullptr || fragmentModule == nullptr)
    {
        // CreateShaderModuleFromFile already reported why (missing/unreadable file) - bail out
        // instead of feeding a null module into vkCreateGraphicsPipelines below.
        if (vertexModule) vkDestroyShaderModule(this->device, vertexModule, nullptr);
        if (fragmentModule) vkDestroyShaderModule(this->device, fragmentModule, nullptr);
        return;
    }

    VkPipelineShaderStageCreateInfo vertStage = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertexModule;
    vertStage.pName = "VS_Main";

    VkPipelineShaderStageCreateInfo fragStage = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragmentModule;
    fragStage.pName = "PS_Main";

    VkPipelineShaderStageCreateInfo stages[] = { vertStage, fragStage };

    // Matches Vertex (engine.shared/vertex.h). The previous comment here claimed bitTangent
    // (location 4) was stripped as unused by DXC and so didn't need a binding - contradicted by
    // Vulkan's own validation layer on real hardware ("Vertex shader consumes input at location 4
    // but not provided"), which then crashed the pipeline. Location 4 is provided here now,
    // matching D3D11Renderer/D3D12Renderer's input layouts, which both already include it.
    VkVertexInputBindingDescription bindingDesc = {};
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(Vertex);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attributeDescs[5] = {};
    attributeDescs[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<ui32>(offsetof(Vertex, position)) };
    attributeDescs[1] = { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<ui32>(offsetof(Vertex, normal)) };
    attributeDescs[2] = { 2, 0, VK_FORMAT_R32G32_SFLOAT, static_cast<ui32>(offsetof(Vertex, texCoords)) };
    attributeDescs[3] = { 3, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<ui32>(offsetof(Vertex, tangent)) };
    attributeDescs[4] = { 4, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<ui32>(offsetof(Vertex, bitTangent)) };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = 5;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport/scissor are dynamic (set per-frame in Render(), matching D3D11's SetViewPort()
    // being callable independently of pipeline state) - only their counts matter here.
    VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkPipelineRasterizationStateCreateInfo rasterizer = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = wireFrame ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    // D3D11's rasterDesc.FrontCounterClockwise = true (see d3d11renderer.cpp Init()) - match it
    // directly, unchanged, despite BeginScene()'s negative-height viewport (see its own comment)
    // fixing Vulkan's upside-down rendering. It's tempting to reason that flipping the viewport's
    // Y axis must also flip the winding the rasterizer perceives, and so cullMode/frontFace need
    // to compensate - that reasoning is backwards here and was actually tried (CW) first: once
    // the Y-flip makes Vulkan's NDC-to-screen mapping match D3D11's exactly, the two pipelines'
    // winding computation becomes equivalent too, so the SAME frontFace as D3D11 is what's
    // correct, not its opposite. Confirmed directly, not just re-derived on paper a second time:
    // VK_FRONT_FACE_CLOCKWISE visibly showed the wrong (inside/back) face of the BoomBox model's
    // handle - a grille-patterned surface where D3D11/D3D12 render it smooth - and switching back
    // to CCW fixed it.
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    // Matches D3D11's blendDesc (SrcBlend=SRC_ALPHA, DestBlend=INV_SRC_ALPHA, BlendOp=ADD;
    // SrcBlendAlpha=ONE, DestBlendAlpha=ZERO).
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    VK_CHECK(vkCreatePipelineLayout(this->device, &pipelineLayoutInfo, nullptr, &this->pipelineLayout));

    VkGraphicsPipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = this->pipelineLayout;
    pipelineInfo.renderPass = this->renderPass;
    pipelineInfo.subpass = 0;

    VK_CHECK(vkCreateGraphicsPipelines(this->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &this->graphicsPipeline));

    vkDestroyShaderModule(this->device, vertexModule, nullptr);
    vkDestroyShaderModule(this->device, fragmentModule, nullptr);
}

void VulkanRenderer::SetActiveCamera(Vec3 eye, Mat4x4 viewProj)
{
    worldLocalBuffer.eye = eye;
    worldLocalBuffer.projView = ToShaderLayout(viewProj);
}

void VulkanRenderer::ClearLights()
{
    lights.clear();
}

void VulkanRenderer::SetLight(GpuLight lightDescriptor)
{
    lightDescriptor.transform = ToShaderLayout(lightDescriptor.transform);
    lights.push_back(lightDescriptor);
}

VkFormat VulkanRenderer::FromTextureFormat(TextureFormat format)
{
    switch (format)
    {
    case TextureFormat::RGBAFLOAT:
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case TextureFormat::RGBFLOAT:
        return VK_FORMAT_R32G32B32_SFLOAT;
    case TextureFormat::RGBA32:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case TextureFormat::RGBA64:
        return VK_FORMAT_R16G16B16A16_UNORM;
    case TextureFormat::ALPHA8:
        // No dedicated "alpha-only" Vulkan format; stored as R8 and swizzled R->A when the
        // image view is created (see CreateTextureSRV).
        return VK_FORMAT_R8_UNORM;
    case TextureFormat::RED8:
        return VK_FORMAT_R8_UNORM;
    case TextureFormat::RED16:
        return VK_FORMAT_R16_UNORM;
    case TextureFormat::REDFLOAT:
        return VK_FORMAT_R32_SFLOAT;
    case TextureFormat::RED1:
        // Not backed by any real 1-bit-per-texel Vulkan format (and never actually
        // instantiated anywhere in the engine today); nearest equivalent.
        return VK_FORMAT_R8_UNORM;
    case TextureFormat::D32:
        return VK_FORMAT_D32_SFLOAT;
    case TextureFormat::RGFLOAT:
        return VK_FORMAT_R32G32_SFLOAT;
    case TextureFormat::RG32:
        return VK_FORMAT_R16G16_UNORM;
    }
    return VK_FORMAT_UNDEFINED;
}

VkCommandBuffer VulkanRenderer::BeginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = this->commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = nullptr;
    VK_CHECK(vkAllocateCommandBuffers(this->device, &allocInfo, &commandBuffer));

    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));
    return commandBuffer;
}

void VulkanRenderer::EndSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    VK_CHECK(vkEndCommandBuffer(commandBuffer));

    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    // Simple and correct (if not maximally efficient) for the upload volumes this engine
    // actually does today: wait for the transfer to fully complete before returning, rather
    // than tracking a separate upload-fence/queue.
    VK_CHECK(vkQueueSubmit(this->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));
    VK_CHECK(vkQueueWaitIdle(this->graphicsQueue));

    vkFreeCommandBuffers(this->device, this->commandPool, 1, &commandBuffer);
}

void VulkanRenderer::TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

    VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        ASSERT_MSG(false, "unsupported image layout transition");
        sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    EndSingleTimeCommands(commandBuffer);
}

void VulkanRenderer::CopyBufferToImage(VkBuffer buffer, VkImage image, ui32 width, ui32 height)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    EndSingleTimeCommands(commandBuffer);
}

TextureHandle VulkanRenderer::CreateTexture(ui32 width, ui32 height, ui32 levels, TextureFormat format, void* data)
{
    VkFormat vkFormat = FromTextureFormat(format);
    bool isDepth = (format == TextureFormat::D32);

    VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = { width, height, 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = vkFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = isDepth
        ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
        : (VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    VulkanTexture tex;
    tex.format = vkFormat;
    VK_CHECK(vmaCreateImage(this->allocator, &imageInfo, &allocInfo, &tex.image, &tex.allocation, nullptr));

    // See VulkanTexture's declaration - this anchors the image's actual lifetime, independent of
    // when ReleaseTexture() clears this handle's own pool slot.
    VmaAllocator allocatorForDeleter = this->allocator;
    VkImage imageForDeleter = tex.image;
    VmaAllocation allocationForDeleter = tex.allocation;
    tex.lifetime = std::shared_ptr<void>(nullptr, [allocatorForDeleter, imageForDeleter, allocationForDeleter](void*)
    {
        vmaDestroyImage(allocatorForDeleter, imageForDeleter, allocationForDeleter);
    });

    if (data != nullptr && !isDepth)
    {
        ui32 pixelSize = PixelSizeFromTextureFormat(format);
        VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * height * pixelSize;

        VkBufferCreateInfo stagingBufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        stagingBufferInfo.size = imageSize;
        stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo stagingAllocInfo = {};
        stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer stagingBuffer = nullptr;
        VmaAllocation stagingAllocation = nullptr;
        VmaAllocationInfo stagingInfo = {};
        VK_CHECK(vmaCreateBuffer(this->allocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuffer, &stagingAllocation, &stagingInfo));
        memcpy(stagingInfo.pMappedData, data, static_cast<size_t>(imageSize));

        TransitionImageLayout(tex.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        CopyBufferToImage(stagingBuffer, tex.image, width, height);
        TransitionImageLayout(tex.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vmaDestroyBuffer(this->allocator, stagingBuffer, stagingAllocation);
    }
    else if (!isDepth)
    {
        // No initial data (e.g. a render target created for later writing) - still needs to
        // leave VK_IMAGE_LAYOUT_UNDEFINED before it can be sampled.
        TransitionImageLayout(tex.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    return texturePool.Create(tex);
}

void VulkanRenderer::ReleaseTexture(TextureHandle& texture)
{
    if (!texture.IsValid())
        return;
    // Actual destruction happens once the VulkanTexture's `lifetime` shared_ptr - possibly also
    // held by one or more VulkanImageViews created from it (see CreateTextureSRV) - drops to
    // zero references, not necessarily here. texturePool.Release() clears this slot's own copy.
    texturePool.Release(texture);
}

ShaderResourceViewHandle VulkanRenderer::CreateTextureSRV(TextureHandle texture, TextureFormat format)
{
    VulkanTexture tex = texturePool.Get(texture);

    VkImageViewCreateInfo viewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    viewInfo.image = tex.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = tex.format;
    if (format == TextureFormat::ALPHA8)
    {
        // See FromTextureFormat: ALPHA8 is stored as plain R8, swizzled to alpha here.
        viewInfo.components = { VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_R };
    }
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VulkanImageView view;
    VK_CHECK(vkCreateImageView(this->device, &viewInfo, nullptr, &view.view));
    // Keeps the source image alive for as long as this view exists, independent of whether/when
    // the caller releases its own TextureHandle - see VulkanTexture's declaration.
    view.textureLifetime = tex.lifetime;
    return srvPool.Create(view);
}

ShaderResourceViewHandle VulkanRenderer::CreateCubemapSRV(TextureHandle cubemap, TextureFormat format)
{
    // Cubemap sampling isn't wired up on the Vulkan path yet (nothing in the engine creates
    // one today - Cubemap::LoadFromFile is never called anywhere either) - matches D3D11's
    // parity target exactly in the sense that this is unexercised either way, but D3D11 does
    // have a real implementation already (VK_IMAGE_VIEW_TYPE_CUBE + a 6-layer image) that this
    // should eventually mirror once something actually uses cubemaps.
    return ShaderResourceViewHandle{};
}

void VulkanRenderer::ReleaseTextureSRV(ShaderResourceViewHandle& srv)
{
    if (!srv.IsValid())
        return;
    VulkanImageView view = srvPool.Get(srv);
    vkDestroyImageView(this->device, view.view, nullptr);
    srvPool.Release(srv);
}

void VulkanRenderer::UseTexture(ui32 slot, ShaderResourceViewHandle view)
{
    while (slot >= pendingTextureViews.size())
    {
        pendingTextureViews.push_back(ShaderResourceViewHandle{});
    }
    pendingTextureViews[slot] = view;
}

BufferHandle VulkanRenderer::CreateBuffer(BufferType type, const void* data, int dataSize, UsageType usage)
{
    VkBufferUsageFlags usageFlags = 0;
    switch (type)
    {
    case BufferType::Vertex:
        usageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        break;
    case BufferType::Index:
        usageFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        break;
    case BufferType::Constant:
        usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        break;
    default:
        return BufferHandle{};
    }

    VulkanBuffer buf;

    if (usage == UsageType::Dynamic)
    {
        // Host-visible and persistently mapped, matching D3D11_USAGE_DYNAMIC + Map/Unmap -
        // written directly via memcpy by whoever holds the handle.
        VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferInfo.size = dataSize;
        bufferInfo.usage = usageFlags;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo info = {};
        VK_CHECK(vmaCreateBuffer(this->allocator, &bufferInfo, &allocInfo, &buf.buffer, &buf.allocation, &info));
        if (data != nullptr)
        {
            memcpy(info.pMappedData, data, static_cast<size_t>(dataSize));
        }
    }
    else
    {
        // Default usage: device-local, uploaded once via a staging buffer.
        VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferInfo.size = dataSize;
        bufferInfo.usage = usageFlags | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

        VK_CHECK(vmaCreateBuffer(this->allocator, &bufferInfo, &allocInfo, &buf.buffer, &buf.allocation, nullptr));

        if (data != nullptr)
        {
            VkBufferCreateInfo stagingInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
            stagingInfo.size = dataSize;
            stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            stagingInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo stagingAllocInfo = {};
            stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
            stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

            VkBuffer stagingBuffer = nullptr;
            VmaAllocation stagingAllocation = nullptr;
            VmaAllocationInfo stagingMapInfo = {};
            VK_CHECK(vmaCreateBuffer(this->allocator, &stagingInfo, &stagingAllocInfo, &stagingBuffer, &stagingAllocation, &stagingMapInfo));
            memcpy(stagingMapInfo.pMappedData, data, static_cast<size_t>(dataSize));

            VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
            VkBufferCopy copyRegion = {};
            copyRegion.size = dataSize;
            vkCmdCopyBuffer(commandBuffer, stagingBuffer, buf.buffer, 1, &copyRegion);
            EndSingleTimeCommands(commandBuffer);

            vmaDestroyBuffer(this->allocator, stagingBuffer, stagingAllocation);
        }
    }

    return bufferPool.Create(buf);
}

void VulkanRenderer::ReleaseBuffer(BufferHandle& buffer)
{
    if (!buffer.IsValid())
        return;
    VulkanBuffer buf = bufferPool.Get(buffer);
    vmaDestroyBuffer(this->allocator, buf.buffer, buf.allocation);
    bufferPool.Release(buffer);
}

void VulkanRenderer::CreateAllocator()
{
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
    allocatorInfo.physicalDevice = this->physicalDevice;
    allocatorInfo.device = this->device;
    allocatorInfo.instance = this->instance;
    VK_CHECK(vmaCreateAllocator(&allocatorInfo, &this->allocator));
}

void VulkanRenderer::CreateSwapchainImagesAndViews()
{
    ui32 imageCount = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(this->device, this->swapchain, &imageCount, nullptr));
    swapchainImages.resize(imageCount);
    VK_CHECK(vkGetSwapchainImagesKHR(this->device, this->swapchain, &imageCount, swapchainImages.data()));

    swapchainImageViews.resize(imageCount);
    for (ui32 i = 0; i < imageCount; i++)
    {
        VkImageViewCreateInfo viewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        viewInfo.image = swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = this->swapchainImageFormat.format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        VK_CHECK(vkCreateImageView(this->device, &viewInfo, nullptr, &swapchainImageViews[i]));
    }
}

void VulkanRenderer::CreateRenderPass()
{
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = this->swapchainImageFormat.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkAttachmentReference depthAttachmentRef = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription attachments[] = { colorAttachment, depthAttachment };
    VkRenderPassCreateInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    renderPassInfo.attachmentCount = 2;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VK_CHECK(vkCreateRenderPass(this->device, &renderPassInfo, nullptr, &this->renderPass));
}

void VulkanRenderer::CreateDepthResources()
{
    VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = { this->swapchainExtent.width, this->swapchainExtent.height, 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_D32_SFLOAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    VK_CHECK(vmaCreateImage(this->allocator, &imageInfo, &allocInfo, &this->depthImage, &this->depthImageAllocation, nullptr));

    VkImageViewCreateInfo viewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    viewInfo.image = this->depthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_D32_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    VK_CHECK(vkCreateImageView(this->device, &viewInfo, nullptr, &this->depthImageView));
}

void VulkanRenderer::CreateFramebuffers()
{
    swapchainFramebuffers.resize(swapchainImageViews.size());
    for (size_t i = 0; i < swapchainImageViews.size(); i++)
    {
        VkImageView attachments[] = { swapchainImageViews[i], depthImageView };
        VkFramebufferCreateInfo framebufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        framebufferInfo.renderPass = this->renderPass;
        framebufferInfo.attachmentCount = 2;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = this->swapchainExtent.width;
        framebufferInfo.height = this->swapchainExtent.height;
        framebufferInfo.layers = 1;
        VK_CHECK(vkCreateFramebuffer(this->device, &framebufferInfo, nullptr, &swapchainFramebuffers[i]));
    }
}

void VulkanRenderer::CreateDescriptorSetLayout()
{
    // See vulkanrenderer.h for the full binding layout rationale.
    VkDescriptorSetLayoutBinding bindings[8] = {};
    bindings[0] = { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr };
    bindings[1] = { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
    bindings[2] = { 10, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
    bindings[3] = { 11, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
    bindings[4] = { 13, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
    bindings[5] = { 14, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
    bindings[6] = { 15, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
    bindings[7] = { 20, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };

    VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    layoutInfo.bindingCount = 8;
    layoutInfo.pBindings = bindings;
    VK_CHECK(vkCreateDescriptorSetLayout(this->device, &layoutInfo, nullptr, &this->descriptorSetLayout));
}

void VulkanRenderer::CreateCommandPool()
{
    VkCommandPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = static_cast<ui32>(this->graphicsFamilyQueueIndex);
    VK_CHECK(vkCreateCommandPool(this->device, &poolInfo, nullptr, &this->commandPool));
}

void VulkanRenderer::CreateCommandBuffers()
{
    commandBuffers.resize(FRAME_LAG);
    VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocInfo.commandPool = this->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = FRAME_LAG;
    VK_CHECK(vkAllocateCommandBuffers(this->device, &allocInfo, commandBuffers.data()));
}

void VulkanRenderer::CreateSyncObjects()
{
    imageAvailableSemaphores.resize(FRAME_LAG);
    renderFinishedSemaphores.resize(FRAME_LAG);
    inFlightFences.resize(FRAME_LAG);

    VkSemaphoreCreateInfo semaphoreInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (ui32 i = 0; i < FRAME_LAG; i++)
    {
        VK_CHECK(vkCreateSemaphore(this->device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]));
        VK_CHECK(vkCreateSemaphore(this->device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]));
        VK_CHECK(vkCreateFence(this->device, &fenceInfo, nullptr, &inFlightFences[i]));
    }
}

void VulkanRenderer::CreateDescriptorPools()
{
    descriptorPools.resize(FRAME_LAG);
    VkDescriptorPoolSize poolSizes[3] = {};
    poolSizes[0] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_DRAWS_PER_FRAME * 2 };
    poolSizes[1] = { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MAX_DRAWS_PER_FRAME * 5 };
    poolSizes[2] = { VK_DESCRIPTOR_TYPE_SAMPLER, MAX_DRAWS_PER_FRAME };

    VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.poolSizeCount = 3;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = MAX_DRAWS_PER_FRAME;

    for (ui32 i = 0; i < FRAME_LAG; i++)
    {
        VK_CHECK(vkCreateDescriptorPool(this->device, &poolInfo, nullptr, &descriptorPools[i]));
    }
}

void VulkanRenderer::CreateDefaultSampler()
{
    // Matches D3D11Renderer's defaultSamplerDesc (Init()): wrap addressing, anisotropic filter.
    VkSamplerCreateInfo samplerInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(this->physicalDevice, &props);
    samplerInfo.maxAnisotropy = props.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    VK_CHECK(vkCreateSampler(this->device, &samplerInfo, nullptr, &this->defaultSampler));
}

void VulkanRenderer::CreateModelConstantBuffers()
{
    modelConstantBuffers.resize(FRAME_LAG * MAX_DRAWS_PER_FRAME);
    for (auto& mb : modelConstantBuffers)
    {
        VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferInfo.size = sizeof(modelConstant);
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo info = {};
        VK_CHECK(vmaCreateBuffer(this->allocator, &bufferInfo, &allocInfo, &mb.buffer, &mb.allocation, &info));
        mb.mapped = info.pMappedData;
    }
}

void VulkanRenderer::CreateWorldConstantBuffers()
{
    worldConstantBuffers.resize(FRAME_LAG);
    for (auto& wb : worldConstantBuffers)
    {
        VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferInfo.size = sizeof(worldConstant);
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo info = {};
        VK_CHECK(vmaCreateBuffer(this->allocator, &bufferInfo, &allocInfo, &wb.buffer, &wb.allocation, &info));
        wb.mapped = info.pMappedData;
    }
}

void VulkanRenderer::CreateDefaultTexture()
{
    ui32 white = 0xFFFFFFFFu;
    defaultTextureHandle = CreateTexture(1, 1, 1, TextureFormat::RGBA32, &white);
    defaultTextureView = CreateTextureSRV(defaultTextureHandle, TextureFormat::RGBA32);
}

void VulkanRenderer::CleanupSwapchain()
{
    for (auto fb : swapchainFramebuffers)
    {
        vkDestroyFramebuffer(this->device, fb, nullptr);
    }
    swapchainFramebuffers.clear();

    if (depthImageView)
    {
        vkDestroyImageView(this->device, depthImageView, nullptr);
        depthImageView = nullptr;
    }
    if (depthImage)
    {
        vmaDestroyImage(this->allocator, depthImage, depthImageAllocation);
        depthImage = nullptr;
        depthImageAllocation = nullptr;
    }

    for (auto view : swapchainImageViews)
    {
        vkDestroyImageView(this->device, view, nullptr);
    }
    swapchainImageViews.clear();
    swapchainImages.clear();

    if (swapchain)
    {
        vkDestroySwapchainKHR(this->device, swapchain, nullptr);
        swapchain = nullptr;
    }
}

void VulkanRenderer::BeginScene()
{
    VK_CHECK(vkWaitForFences(this->device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX));

    VkResult acquireResult = vkAcquireNextImageKHR(this->device, this->swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &currentImageIndex);
    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        // Window is presumably about to Resize() us too (see Window::PollEvents); skip this
        // frame's drawing/present rather than fail.
        frameActive = false;
        return;
    }
    ASSERT_MSG(acquireResult == VK_SUCCESS || acquireResult == VK_SUBOPTIMAL_KHR, "vkAcquireNextImageKHR failed");

    VK_CHECK(vkResetFences(this->device, 1, &inFlightFences[currentFrame]));
    VK_CHECK(vkResetDescriptorPool(this->device, descriptorPools[currentFrame], 0));
    drawIndexThisFrame = 0;

    VkCommandBuffer cmd = commandBuffers[currentFrame];
    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));

    VkClearValue clearValues[2];
    // Matches D3D11Renderer::clearColor.
    clearValues[0].color = { { 0.529411f, 0.807843f, 0.921686f, 1.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    renderPassInfo.renderPass = this->renderPass;
    renderPassInfo.framebuffer = swapchainFramebuffers[currentImageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = this->swapchainExtent;
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->graphicsPipeline);

    // Negative height (Vulkan 1.1+ core, targeted via VK_API_VERSION_1_2 above) flips Vulkan's
    // rasterization to match the OpenGL/D3D clip-space convention GLM's glm::perspective (used
    // by camerasystem.h, shared across all three backends) assumes - Vulkan's own NDC Y axis
    // points the opposite way by default, otherwise, and confirmed directly: without this, every
    // Vulkan frame rendered upside down while the identical shared camera math rendered right-
    // side up on D3D11/D3D12. Fixed here, not in shared code, since D3D11/D3D12 need no such
    // adjustment - this is purely a Vulkan-vs-D3D viewport convention difference.
    VkViewport viewport = { 0.0f, static_cast<float>(swapchainExtent.height), static_cast<float>(swapchainExtent.width), -static_cast<float>(swapchainExtent.height), 0.0f, 1.0f };
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    VkRect2D scissor = { { 0, 0 }, swapchainExtent };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // Once per frame (matching D3D11Renderer::BeginScene's worldBuffer map/update) - camera +
    // lights, read by every draw this frame via worldConstantBuffers[currentFrame].
    worldLocalBuffer.lightCount = static_cast<ui32>(lights.size());
    for (ui32 i = 0; i < lights.size() && i < MAX_LIGHTS; i++)
    {
        worldLocalBuffer.lights[i] = lights[i];
    }
    memcpy(worldConstantBuffers[currentFrame].mapped, &worldLocalBuffer, sizeof(worldConstant));

    frameActive = true;
}

void VulkanRenderer::Render(Mat4x4 transformMat, BufferHandle vertexBuffer, BufferHandle indexBuffer, ui32 indexCount)
{
    if (!frameActive)
        return;

    ASSERT_MSG(drawIndexThisFrame < MAX_DRAWS_PER_FRAME, "exceeded MAX_DRAWS_PER_FRAME - raise the constant in vulkanrenderer.h");

    MappedBuffer& modelBuf = modelConstantBuffers[currentFrame * MAX_DRAWS_PER_FRAME + drawIndexThisFrame];
    modelConstant modelCB;
    modelCB.world = ToShaderLayout(transformMat);
    memcpy(modelBuf.mapped, &modelCB, sizeof(modelConstant));

    VkDescriptorSetAllocateInfo setAllocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    setAllocInfo.descriptorPool = descriptorPools[currentFrame];
    setAllocInfo.descriptorSetCount = 1;
    setAllocInfo.pSetLayouts = &descriptorSetLayout;

    VkDescriptorSet descriptorSet = nullptr;
    VK_CHECK(vkAllocateDescriptorSets(this->device, &setAllocInfo, &descriptorSet));

    VkDescriptorBufferInfo modelBufferInfo = { modelBuf.buffer, 0, sizeof(modelConstant) };
    VkDescriptorBufferInfo worldBufferInfo = { worldConstantBuffers[currentFrame].buffer, 0, sizeof(worldConstant) };

    // Engine texture slot -> shader register(tN)/binding, per meshsystem.h/bsdfPixel.hlsl:
    // 0=albedo(->10), 1=metallic(->11), 3=roughness(->13), 4=normal(->14), 5=emission(->15).
    // Slot 2 (occlusion) is intentionally absent - the shader never samples it (see
    // res/ps/bsdfPixel.hlsl), so DXC strips it from the compiled SPIR-V entirely.
    static const ui32 engineSlots[5] = { 0, 1, 3, 4, 5 };
    static const ui32 imageBindings[5] = { 10, 11, 13, 14, 15 };
    VkDescriptorImageInfo imageInfos[5];
    for (ui32 i = 0; i < 5; i++)
    {
        ui32 slot = engineSlots[i];
        ShaderResourceViewHandle view = (slot < pendingTextureViews.size()) ? pendingTextureViews[slot] : ShaderResourceViewHandle{};
        VkImageView imageView = view.IsValid() ? srvPool.Get(view).view : srvPool.Get(defaultTextureView).view;
        imageInfos[i] = { VK_NULL_HANDLE, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    }
    VkDescriptorImageInfo samplerInfo = { defaultSampler, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED };

    VkWriteDescriptorSet writes[8] = {};
    writes[0] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    writes[0].dstSet = descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].pBufferInfo = &modelBufferInfo;

    writes[1] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    writes[1].dstSet = descriptorSet;
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[1].pBufferInfo = &worldBufferInfo;

    for (ui32 i = 0; i < 5; i++)
    {
        writes[2 + i] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        writes[2 + i].dstSet = descriptorSet;
        writes[2 + i].dstBinding = imageBindings[i];
        writes[2 + i].descriptorCount = 1;
        writes[2 + i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writes[2 + i].pImageInfo = &imageInfos[i];
    }

    writes[7] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    writes[7].dstSet = descriptorSet;
    writes[7].dstBinding = 20;
    writes[7].descriptorCount = 1;
    writes[7].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    writes[7].pImageInfo = &samplerInfo;

    vkUpdateDescriptorSets(this->device, 8, writes, 0, nullptr);

    VkCommandBuffer cmd = commandBuffers[currentFrame];
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

    VkBuffer vb = bufferPool.Get(vertexBuffer).buffer;
    VkDeviceSize vbOffset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &vb, &vbOffset);
    vkCmdBindIndexBuffer(cmd, bufferPool.Get(indexBuffer).buffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(cmd, indexCount, 1, 0, 0, 0);

    drawIndexThisFrame++;
    pendingTextureViews.clear();
}

void VulkanRenderer::EndScene()
{
    if (!frameActive)
    {
        currentFrame = (currentFrame + 1) % FRAME_LAG;
        return;
    }

    VkCommandBuffer cmd = commandBuffers[currentFrame];
    vkCmdEndRenderPass(cmd);
    VK_CHECK(vkEndCommandBuffer(cmd));

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailableSemaphores[currentFrame];
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderFinishedSemaphores[currentFrame];

    VK_CHECK(vkQueueSubmit(this->graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]));

    VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphores[currentFrame];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &this->swapchain;
    presentInfo.pImageIndices = &currentImageIndex;

    VkResult presentResult = vkQueuePresentKHR(this->presentQueue, &presentInfo);
    ASSERT_MSG(presentResult == VK_SUCCESS || presentResult == VK_SUBOPTIMAL_KHR || presentResult == VK_ERROR_OUT_OF_DATE_KHR, "vkQueuePresentKHR failed");

    currentFrame = (currentFrame + 1) % FRAME_LAG;
}

bool VulkanRenderer::Resize(ui32 newWidth, ui32 newHeight)
{
    if (newWidth == 0 || newHeight == 0)
        return false; // minimized

    vkDeviceWaitIdle(this->device);

    this->width = newWidth;
    this->height = newHeight;

    CleanupSwapchain();

    CreateSwapchain();
    CreateSwapchainImagesAndViews();
    CreateDepthResources();
    CreateFramebuffers();

    return true;
}

bool VulkanRenderer::CheckForFullscreen()
{
    // No exclusive-fullscreen concept on this (SDL2-owned window) path, unlike D3D11/DXGI -
    // nothing currently calls this (see engine.window/window.cpp PollEvents()).
    return false;
}

void VulkanRenderer::Shutdown()
{
    if (this->device)
    {
        vkDeviceWaitIdle(this->device);
    }

    if (imguiInitialized)
    {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        vkDestroyDescriptorPool(this->device, imguiDescriptorPool, nullptr);
        imguiInitialized = false;
    }

    ReleaseTexture(defaultTextureHandle);
    ReleaseTextureSRV(defaultTextureView);

    // Anything still allocated through CreateTexture/CreateTextureSRV/CreateBuffer (mesh
    // vertex/index buffers, material textures, ...) that its owner never explicitly released -
    // unlike D3D11's COM refcounting, VMA asserts on destruction if any allocation from a
    // memory block is still outstanding, so these must be drained explicitly.
    srvPool.ForEachAlive([this](const VulkanImageView& view) {
        vkDestroyImageView(this->device, view.view, nullptr);
    });
    bufferPool.ForEachAlive([this](const VulkanBuffer& buf) {
        vmaDestroyBuffer(this->allocator, buf.buffer, buf.allocation);
    });
    // Image destruction itself is NOT done here - it's owned by each VulkanTexture/
    // VulkanImageView's `lifetime`/`textureLifetime` shared_ptr (see VulkanTexture's
    // declaration), which calls vmaDestroyImage exactly once regardless of how many copies
    // (texturePool's own, plus any surviving VulkanImageView's) still reference it. Resetting
    // both pools drops every such copy right now, while `allocator` below is still valid - that
    // deleter captures it by value, so this can't be left to whenever texturePool/srvPool's own
    // destructors happen to run (VulkanRenderer's implicit destructor, well after Shutdown()
    // returns and vmaDestroyAllocator() has already invalidated it).
    texturePool = HandlePool<VulkanTexture, TextureHandle>{};
    srvPool = HandlePool<VulkanImageView, ShaderResourceViewHandle>{};

    for (auto& wb : worldConstantBuffers)
    {
        vmaDestroyBuffer(this->allocator, wb.buffer, wb.allocation);
    }
    for (auto& mb : modelConstantBuffers)
    {
        vmaDestroyBuffer(this->allocator, mb.buffer, mb.allocation);
    }

    if (defaultSampler)
    {
        vkDestroySampler(this->device, defaultSampler, nullptr);
    }

    for (auto pool : descriptorPools)
    {
        vkDestroyDescriptorPool(this->device, pool, nullptr);
    }

    for (ui32 i = 0; i < imageAvailableSemaphores.size(); i++)
    {
        vkDestroySemaphore(this->device, imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(this->device, renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(this->device, inFlightFences[i], nullptr);
    }

    if (commandPool)
    {
        // Also frees commandBuffers (they were allocated from this pool).
        vkDestroyCommandPool(this->device, commandPool, nullptr);
    }

    if (descriptorSetLayout)
    {
        vkDestroyDescriptorSetLayout(this->device, descriptorSetLayout, nullptr);
    }
    if (graphicsPipeline)
    {
        vkDestroyPipeline(this->device, graphicsPipeline, nullptr);
    }
    if (pipelineLayout)
    {
        vkDestroyPipelineLayout(this->device, pipelineLayout, nullptr);
    }

    CleanupSwapchain();
    if (renderPass)
    {
        vkDestroyRenderPass(this->device, renderPass, nullptr);
    }

    if (allocator)
    {
        vmaDestroyAllocator(this->allocator);
    }

    if (device)
    {
        vkDestroyDevice(this->device, nullptr);
    }

    if (debugMessenger)
    {
        PFN_vkDestroyDebugUtilsMessengerEXT destroyFunc = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(this->instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (destroyFunc)
        {
            destroyFunc(this->instance, debugMessenger, nullptr);
        }
    }
    if (surface)
    {
        vkDestroySurfaceKHR(this->instance, surface, nullptr);
    }
    if (instance)
    {
        vkDestroyInstance(this->instance, nullptr);
    }
}

static void ImGuiVulkanCheckResult(VkResult err)
{
    ASSERT_MSG(err == VK_SUCCESS, "Dear ImGui Vulkan backend error");
}

bool VulkanRenderer::InitImGui(SDL_Window* window)
{
    // Dear ImGui's Vulkan backend only ever binds one combined-image-sampler descriptor (its
    // font atlas - this old backend doesn't support arbitrary user textures, see
    // imgui_impl_vulkan.h's "Missing features" note), so this pool only ever needs to satisfy
    // that one allocation. Separate from descriptorPools above, which use this renderer's own
    // (incompatible) split sampled-image/sampler layout.
    VkDescriptorPoolSize poolSize = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 };
    VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    VK_CHECK(vkCreateDescriptorPool(this->device, &poolInfo, nullptr, &imguiDescriptorPool));

    if (!ImGui_ImplSDL2_InitForVulkan(window))
        return false;

    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = this->instance;
    initInfo.PhysicalDevice = this->physicalDevice;
    initInfo.Device = this->device;
    initInfo.QueueFamily = static_cast<ui32>(this->graphicsFamilyQueueIndex);
    initInfo.Queue = this->graphicsQueue;
    initInfo.PipelineCache = VK_NULL_HANDLE;
    initInfo.DescriptorPool = imguiDescriptorPool;
    initInfo.MinImageCount = static_cast<ui32>(swapchainImages.size());
    initInfo.ImageCount = static_cast<ui32>(swapchainImages.size());
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.Allocator = nullptr;
    initInfo.CheckVkResultFn = ImGuiVulkanCheckResult;
    if (!ImGui_ImplVulkan_Init(&initInfo, this->renderPass))
        return false;

    VkCommandBuffer cmd = BeginSingleTimeCommands();
    ImGui_ImplVulkan_CreateFontsTexture(cmd);
    EndSingleTimeCommands(cmd);
    ImGui_ImplVulkan_DestroyFontUploadObjects();

    imguiInitialized = true;
    return true;
}

void VulkanRenderer::ImGuiNewFrame(SDL_Window* window)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);
    ImGui::NewFrame();
}

void VulkanRenderer::ImGuiRenderDrawData()
{
    // ImGui::Render() always has to run to balance the ImGui::NewFrame() in ImGuiNewFrame() -
    // skipping it would corrupt ImGui's internal frame-state assertions on the next frame.
    // The actual GPU draw-command recording is skipped when the frame itself was skipped (see
    // BeginScene()'s VK_ERROR_OUT_OF_DATE_KHR path) - commandBuffers[currentFrame] was never
    // put into the recording state in that case.
    ImGui::Render();
    if (!frameActive)
        return;
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffers[currentFrame]);
}
