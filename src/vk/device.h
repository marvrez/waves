#pragma once

class Window;

class Device {
public:
    Device(const Window& window, bool enableValidationLayer=true);
    ~Device();
    VkCommandBuffer CreateCommandBuffer() const;
    void Submit(std::function<void(VkCommandBuffer)> recordCmdBuffer) const;

    operator VkDevice() const { return mDevice; }
    VmaAllocator Allocator() const { return mAllocator; }
    uint32_t GetSelectedQueueIndex() const { return mQueueIndex; }
    VkQueue GetSelectedQueue() const { return mQueue; }
    VkSurfaceKHR GetSurface() const { return mSurface; }
    VkPhysicalDevice GetPhysicalDevice() const { return mPhysicalDevice; }

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
