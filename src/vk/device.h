#pragma once

class Window;

class Device {
public:
    Device(const Window& window, bool enableValidationLayer=true);
    ~Device();
    VkCommandBuffer CreateCommandBuffer();
    void Submit();

private:
    VkDebugUtilsMessengerEXT mDebugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR mSurface = VK_NULL_HANDLE;

    VkInstance mInstance = VK_NULL_HANDLE;
    VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
    VkDevice mDevice = VK_NULL_HANDLE;

    VkQueue mQueue;
    uint32_t mQueueIndex = ~0;

    VmaAllocator mAllocator = VK_NULL_HANDLE;
    VkCommandPool mCommandPool = VK_NULL_HANDLE;
};
