// Headless Vulkan bring-up smoke test: confirms the Vulkan loader/driver stack that
// driver.vulkan depends on is actually present and functional in this environment.
//
// This intentionally doesn't touch VulkanRenderer itself - Init() requires a live native
// window handle (for the surface), which isn't available in a headless test until the
// SDL2 windowing work lands. Instead it exercises the same instance/physical-device
// sequence VulkanRenderer::Init() starts with, using the Vulkan API directly.

#include <vulkan/vulkan.h>
#include <cstdio>
#include <vector>

int main()
{
	VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.apiVersion = VK_API_VERSION_1_2;
	appInfo.pApplicationName = "DuplexEngine tests";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "DuplexEngine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

	VkInstanceCreateInfo instanceInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	instanceInfo.pApplicationInfo = &appInfo;

	VkInstance instance = VK_NULL_HANDLE;
	VkResult result = vkCreateInstance(&instanceInfo, nullptr, &instance);
	if (result != VK_SUCCESS)
	{
		std::fprintf(stderr, "test_vulkan_bringup: vkCreateInstance failed (VkResult=%d)\n", static_cast<int>(result));
		return 1;
	}

	uint32_t deviceCount = 0;
	result = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if (result != VK_SUCCESS)
	{
		std::fprintf(stderr, "test_vulkan_bringup: vkEnumeratePhysicalDevices failed (VkResult=%d)\n", static_cast<int>(result));
		vkDestroyInstance(instance, nullptr);
		return 1;
	}

	if (deviceCount == 0)
	{
		std::fprintf(stderr, "test_vulkan_bringup: no Vulkan-capable physical devices found\n");
		vkDestroyInstance(instance, nullptr);
		return 1;
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	std::printf("test_vulkan_bringup: found %u physical device(s):\n", deviceCount);
	for (VkPhysicalDevice device : devices)
	{
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(device, &props);
		std::printf("  - %s\n", props.deviceName);
	}

	vkDestroyInstance(instance, nullptr);
	std::printf("test_vulkan_bringup: all checks passed\n");
	return 0;
}
