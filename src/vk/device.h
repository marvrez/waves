#pragma once

class Window;
class CommandList;

class Device {
public:
    Device(const Window& window, bool enableValidationLayer=true);
    ~Device();

    Handle<CommandList> CreateCommandList() const;
    void ExecuteCommandList(Handle<CommandList> cmdList) const;

    void WaitIdle() const;

    operator VkDevice() const { return mDevice; }
    VmaAllocator Allocator() const { return mAllocator; }
    uint32_t GetSelectedQueueIndex() const { return mQueueIndex; }
    VkQueue GetSelectedQueue() const { return mQueue; }
    VkSurfaceKHR GetSurface() const { return mSurface; }
    VkPhysicalDevice GetPhysicalDevice() const { return mPhysicalDevice; }
    VkCommandPool GetCommandPool() const { return mCommandPool; }

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
