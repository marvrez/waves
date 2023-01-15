#pragma once

#include "descs.h"

struct SwapchainDesc {
    uint32_t framebufferWidth = 0u;                // The frame buffer width
    uint32_t framebufferHeight = 0u;               // The frame buffer height
    bool shouldEnableVsync = true;                 // Enable vertical sync.
    uint32_t preferredSwapchainImageCount = 2;     // Preferred number of swapchain images.
    VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE;  // [Optional] Old swapchain which can be used for faster creation.
};


class Device;
class FrameState;
class Texture;
class Swapchain {
public:
    Swapchain(const Device& device, SwapchainDesc desc);
    ~Swapchain();
    void SubmitAndPresent(VkCommandBuffer cmdBuf, uint32_t imageIndex, FrameState frameState);

    uint32_t AcquireNextImage(uint64_t timeout, VkSemaphore semaphore, VkFence fence = VK_NULL_HANDLE);
    const Texture& GetTexture(uint32_t imageIndex) const;

    Format GetFormat() const { return mFormat; }
    VkExtent2D GetExtent() const { return mExtent; }
private:
    const Device& mDevice;
    VkSwapchainKHR mSwapchain = VK_NULL_HANDLE;
    VkExtent2D mExtent = {};
    Format mFormat = Format::NONE;
    std::vector<Texture> mTextures = {};
};