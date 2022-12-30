#include "vk/device.h"
#include "vk/common.h"

#include "window.h"
#include "utils.h"

const std::vector<const char*> kRequiredExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
    VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
#ifdef __APPLE__
    "VK_KHR_portability_subset",
#endif // __APPLE__
};
    
const std::vector<const char*> kValidationLayers = { "VK_LAYER_KHRONOS_validation" };

const std::vector<const char*> kInstanceExtensions = {
    VK_KHR_SURFACE_EXTENSION_NAME,

#ifdef _WIN32
    "VK_KHR_win32_surface",
#elif __linux__
    "VK_KHR_xcb_surface",
#elif __APPLE__
    "VK_EXT_metal_surface",
    VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
#endif // _WIN32 __linux__ __APPLE__

#ifdef _DEBUG
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif // _DEBUG

    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
};

static VkInstance CreateInstance(bool enableValidationLayers)
{

    const VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "sdf-edit",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "sdf-edit",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_3,
    };

    VkInstanceCreateInfo instanceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#ifdef __APPLE__
        .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
#endif // __APPLE__
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = uint32_t(kInstanceExtensions.size()),
        .ppEnabledExtensionNames = kInstanceExtensions.data(),
    };
    LOG_INFO("Enabled extensions: {}", fmt::join(kInstanceExtensions, ", "));

    if (enableValidationLayers) {
        instanceCreateInfo.enabledLayerCount = kValidationLayers.size();
        instanceCreateInfo.ppEnabledLayerNames = kValidationLayers.data();
        LOG_INFO("Enabled validation layers: {}", fmt::join(kValidationLayers, ", "));
    }

#ifdef _DEBUG
    std::vector<VkValidationFeatureEnableEXT> enabledValidationFeatures = { VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT };
    const VkValidationFeaturesEXT validationFeatures = {
        .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
        .enabledValidationFeatureCount = (uint32_t)enabledValidationFeatures.size(),
        .pEnabledValidationFeatures = enabledValidationFeatures.data(),
    };
    instanceCreateInfo.pNext = &validationFeatures;
#endif // _DEBUG

    VkInstance instance;
    VK_CHECK(vkCreateInstance(&instanceCreateInfo, nullptr, &instance));
    volkLoadInstance(instance);
    return instance;
}


static VkDebugUtilsMessengerEXT CreateDebugMessenger(VkInstance instance, bool enableValidationLayer)
{
    if (!enableValidationLayer) return VK_NULL_HANDLE;
    const VkDebugUtilsMessengerCreateInfoEXT createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = [](
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
            void* userData
        ) {
            LOG_INFO("DebugUtilsMessenger: {}\n", std::string_view(callbackData->pMessage));
            assert(messageSeverity != VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT);
            return VK_FALSE;
        },
        .pUserData = nullptr
    };
    VkDebugUtilsMessengerEXT debugMessenger;
    VK_CHECK(vkCreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger));
    return debugMessenger;
}

static uint32_t GetGraphicsQueueFamilyIndex(VkPhysicalDevice physicalDevice)
{
    auto queueFamilies = GetVectorNoError<VkQueueFamilyProperties>(vkGetPhysicalDeviceQueueFamilyProperties, physicalDevice);
    for (auto [idx, queueFamily] : enumerate(queueFamilies) ) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            return idx;
        }
    }
    return ~0u;
}

static bool IsPhysicalDeviceSupported(VkPhysicalDevice physicalDevice)
{
    const auto& availableExtensions = GetVector<VkExtensionProperties>(vkEnumerateDeviceExtensionProperties, physicalDevice, nullptr);
    const auto IsDeviceExtensionAvailable = [&](const char* extensionName) {
        for (const auto& availableExtension : availableExtensions) {
            if (strcmp(availableExtension.extensionName, extensionName) == 0) {
                return true;
            }
        }
        return false;
    };
    for (auto deviceExtension : kRequiredExtensions) {
        if (!IsDeviceExtensionAvailable(deviceExtension)) {
            return false;
        }
    }
    return true;
}

static std::pair<VkPhysicalDevice, uint32_t> SelectPhysicalDevice(VkInstance instance, VkSurfaceKHR surface)
{
    const auto& physicalDeviceCandidates = GetVector<VkPhysicalDevice>(vkEnumeratePhysicalDevices, instance);
    if (physicalDeviceCandidates.empty()) {
        LOG_ERROR("No physical device was found");
        assert(0);
    }

    VkPhysicalDevice selectedPhysicalDevice = VK_NULL_HANDLE;
    uint32_t selectedQueueIndex = ~0;
    for (auto physicalDevice : physicalDeviceCandidates)
    {
        // TODO: currently, we only get the first available graphics queue. Should we extend the logic?
        uint32_t queueIndex = GetGraphicsQueueFamilyIndex(physicalDevice);
        if (queueIndex == ~0u) continue;

        VkBool32 isSurfaceSupported;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueIndex, surface, &isSurfaceSupported);
        if (isSurfaceSupported == VK_FALSE) continue;
        if (!IsPhysicalDeviceSupported(physicalDevice)) continue;

        selectedPhysicalDevice = physicalDevice;
        selectedQueueIndex = queueIndex;
        break;
    }
    LOG_INFO("Selected queue with index {}", selectedQueueIndex);

    return std::make_pair(selectedPhysicalDevice, selectedQueueIndex);
}

static VkSurfaceKHR CreateSurface(const Window& window, VkInstance instance)
{
    VkSurfaceKHR surface;
    VK_CHECK(glfwCreateWindowSurface(instance, window.GetHandle(), nullptr, &surface));
    return surface;
}

static VkDevice CreateDevice(VkPhysicalDevice physicalDevice, uint32_t queueIndex)
{
    std::vector<const char*> deviceExtensions(kRequiredExtensions);

    float queuePriority = 1.0f;
    const VkDeviceQueueCreateInfo queueCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queueIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
    };

    const VkPhysicalDeviceFeatures2 deviceFeatures2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .features = { .shaderInt16 = VK_TRUE }
    };

    const VkPhysicalDeviceVulkan11Features deviceFeatures11 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
        .storageBuffer16BitAccess = VK_TRUE,
        .shaderDrawParameters = VK_TRUE,
        .pNext = (void*)&deviceFeatures2,
    };

    const VkPhysicalDeviceVulkan12Features deviceFeatures12 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .storageBuffer8BitAccess = VK_TRUE,
        .uniformAndStorageBuffer8BitAccess = VK_TRUE,
        .storagePushConstant8 = VK_TRUE,
        .shaderFloat16 = VK_TRUE,
        .shaderInt8 = VK_TRUE,
        .pNext = (void*)&deviceFeatures11,
    };

    const VkPhysicalDeviceVulkan13Features deviceFeatures13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .dynamicRendering = VK_TRUE,
        // .synchronization2 = VK_TRUE,
        .pNext = (void*)&deviceFeatures12,
    };

    const VkDeviceCreateInfo deviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledExtensionCount = uint32_t(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
        .pNext = &deviceFeatures13,
    };

    VkDevice device;
    VK_CHECK(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device));
    return device;
}

static VmaAllocator CreateAllocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device)
{
    const VmaVulkanFunctions volkFunctions = {
        vkGetInstanceProcAddr,
        vkGetDeviceProcAddr,
        vkGetPhysicalDeviceProperties,
        vkGetPhysicalDeviceMemoryProperties,
        vkAllocateMemory,
        vkFreeMemory,
        vkMapMemory,
        vkUnmapMemory,
        vkFlushMappedMemoryRanges,
        vkInvalidateMappedMemoryRanges,
        vkBindBufferMemory,
        vkBindImageMemory,
        vkGetBufferMemoryRequirements,
        vkGetImageMemoryRequirements,
        vkCreateBuffer,
        vkDestroyBuffer,
        vkCreateImage,
        vkDestroyImage,
        vkCmdCopyBuffer,
        vkGetBufferMemoryRequirements2KHR,
        vkGetImageMemoryRequirements2KHR,
        vkBindBufferMemory2KHR,
        vkBindImageMemory2KHR,
        vkGetPhysicalDeviceMemoryProperties2KHR,
        vkGetDeviceBufferMemoryRequirements,
        vkGetDeviceImageMemoryRequirements,
    };
    const VmaAllocatorCreateInfo allocatorCreateInfo = {
        .vulkanApiVersion = volkGetInstanceVersion(),
        .instance = instance,
        .physicalDevice = physicalDevice,
        .device = device,
        .pVulkanFunctions = &volkFunctions,
    };
    VmaAllocator allocator;
    VK_CHECK(vmaCreateAllocator(&allocatorCreateInfo, &allocator));
    return allocator;
}

static VkCommandPool CreateCommandPool(VkDevice device, uint32_t queueIndex)
{
    const VkCommandPoolCreateInfo commandPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = queueIndex,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    };
    VkCommandPool commandPool;
    VK_CHECK(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool));
    return commandPool;
}

Device::Device(const Window& window, bool enableValidationLayer)
{
    VK_CHECK(volkInitialize());

    mInstance = CreateInstance(enableValidationLayer);
#ifdef _DEBUG
    mDebugMessenger = CreateDebugMessenger(mInstance, enableValidationLayer);
#endif
    mSurface = CreateSurface(window, mInstance);
    
    const auto [physicalDevice, queueIndex] = SelectPhysicalDevice(mInstance, mSurface);
    assert(physicalDevice != VK_NULL_HANDLE && queueIndex != ~0u);
    mPhysicalDevice = physicalDevice;
    mQueueIndex = queueIndex;

    mDevice = CreateDevice(mPhysicalDevice, mQueueIndex);
    vkGetDeviceQueue(mDevice, mQueueIndex, 0, &mQueue);

    mAllocator = CreateAllocator(mInstance, mPhysicalDevice, mDevice);

    mCommandPool = CreateCommandPool(mDevice, mQueueIndex);
}

Device::~Device()
{
    LOG_INFO("Destroying Vulkan device");
    vkDestroyCommandPool(mDevice, mCommandPool, nullptr);

    vmaDestroyAllocator(mAllocator);

    vkDestroyDevice(mDevice, nullptr);

    if (mDebugMessenger != VK_NULL_HANDLE) {
        vkDestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
    vkDestroyInstance(mInstance, nullptr);
}

VkCommandBuffer Device::CreateCommandBuffer() const
{
    VkCommandBuffer commandBuffer;
    VkCommandBufferAllocateInfo allocInfo = { 
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = mCommandPool,
        .commandBufferCount = 1
    };
    VK_CHECK(vkAllocateCommandBuffers(mDevice, &allocInfo, &commandBuffer));
    return commandBuffer;
}

void Device::Submit(std::function<void(VkCommandBuffer)> recordCmdBuffer) const
{
    VkCommandBuffer commandBuffer = this->CreateCommandBuffer();

    // Record commands
    const VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));
    recordCmdBuffer(commandBuffer);
    VK_CHECK(vkEndCommandBuffer(commandBuffer));

    const VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
    };
    VK_CHECK(vkQueueSubmit(mQueue, 1, &submitInfo, VK_NULL_HANDLE));
    VK_CHECK(vkQueueWaitIdle(mQueue));

    vkFreeCommandBuffers(mDevice, mCommandPool, 1, &commandBuffer);
}