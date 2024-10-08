#include "vk/swapchain.h"

#include "vk/device.h"
#include "vk/common.h"
#include "vk/frame_pacing.h"
#include "vk/command_list.h"
#include "vk/texture.h"

#include "logger.h"

static VkSurfaceFormatKHR SelectSwapchainSurfaceFormat(const Device& device)
{
    const auto& availableSurfaceFormats = GetVector<VkSurfaceFormatKHR>(vkGetPhysicalDeviceSurfaceFormatsKHR, device.GetPhysicalDevice(), device.GetSurface());
    for (auto& surfaceFormat : availableSurfaceFormats) {
        if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM) {
            return surfaceFormat;
        }
    }
    // return availableSurfaceFormats[0];
    throw std::runtime_error("ERROR: we only support BGRA8_UNORM for swapchains.");
}

static VkPresentModeKHR SelectSwapchainPresentMode(const Device& device, bool shouldEnableVsync)
{
    if (!shouldEnableVsync) {
        const auto& availablePresentModes = GetVector<VkPresentModeKHR>(vkGetPhysicalDeviceSurfacePresentModesKHR, device.GetPhysicalDevice(), device.GetSurface());
        for (auto& presentMode : availablePresentModes) {
            if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                return presentMode;
            }
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D SelectSwapchainExtent(const Device& device, uint32_t framebufferWidth, uint32_t framebufferHeight)
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.GetPhysicalDevice(), device.GetSurface(), &surfaceCapabilities));
    if (surfaceCapabilities.currentExtent.width != UINT32_MAX && surfaceCapabilities.currentExtent.height != UINT32_MAX) {
        return surfaceCapabilities.currentExtent;
    }

    assert(framebufferWidth > 0 && framebufferHeight > 0);
    const VkExtent2D minImageExtent = surfaceCapabilities.minImageExtent;
    const VkExtent2D maxImageExtent = surfaceCapabilities.maxImageExtent;
    return {
        .width  = std::clamp(framebufferWidth, minImageExtent.width, maxImageExtent.width),
        .height = std::clamp(framebufferHeight, minImageExtent.height, maxImageExtent.height)
    };
}

static VkSwapchainKHR CreateSwapchain(
    const Device& device,
    VkSurfaceFormatKHR surfaceFormat,
    VkPresentModeKHR presentMode,
    VkExtent2D extent,
    VkSwapchainKHR oldSwapchain,
    uint32_t preferredSwapchainImageCount)
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.GetPhysicalDevice(), device.GetSurface(), &surfaceCapabilities));

    const uint32_t minImageCount = std::clamp(preferredSwapchainImageCount, surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount);
    const VkSwapchainCreateInfoKHR swapchainCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = device.GetSurface(),
        .minImageCount = minImageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = oldSwapchain,
    };
    VkSwapchainKHR swapchain;
    VK_CHECK(vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain));
    return swapchain;
}

Swapchain::Swapchain(const Device& device, SwapchainDesc desc)
    : mDevice(device), mFormat(Format::BGRA8_UNORM)
{
    const VkSurfaceFormatKHR surfaceFormat = SelectSwapchainSurfaceFormat(device);
    const VkPresentModeKHR presentMode = SelectSwapchainPresentMode(device, desc.shouldEnableVsync);
    mExtent = SelectSwapchainExtent(device, desc.framebufferWidth, desc.framebufferHeight);

    mSwapchain = CreateSwapchain(
        device,
        surfaceFormat,
        presentMode,
        mExtent,
        desc.oldSwapchain,
        desc.preferredSwapchainImageCount
    );

    auto swapchainImages = GetVector<VkImage>(vkGetSwapchainImagesKHR, (VkDevice)mDevice, mSwapchain);
    mTextures.reserve(swapchainImages.size());
    Handle<CommandList> cmdList = device.CreateCommandList();
    cmdList->Open();
    for (auto swapchainImage : swapchainImages) {
        mTextures.emplace_back(device, TextureDesc{
            .dimensions = { mExtent.width, mExtent.height, 1u },
            .format = mFormat,
            .resource = swapchainImage
        });
        cmdList->SetResourceState(mTextures.back(), ResourceStateBits::PRESENT);
    }
    cmdList->Close();
    device.ExecuteCommandList(cmdList);
}

Swapchain::~Swapchain()
{
    LOG_INFO("Deleting swapchain...");
    mTextures.clear();
    vkDestroySwapchainKHR(mDevice, mSwapchain, nullptr);
}

void Swapchain::SubmitAndPresent(Handle<CommandList> cmdList, uint32_t imageIndex, FrameState frameState)
{
    const VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    const VkCommandBuffer cmdBuf = cmdList->GetCommandBuffer();
    const VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1u,
        .pWaitSemaphores = &frameState.imageAvailableSemaphore,
        .pWaitDstStageMask = &waitStage,
        .commandBufferCount = 1u,
        .pCommandBuffers = &cmdBuf,
        .signalSemaphoreCount = 1u,
        .pSignalSemaphores = &frameState.renderFinishedSemaphore,
    };
    VK_CHECK(vkQueueSubmit(mDevice.GetSelectedQueue(), 1, &submitInfo, frameState.inFlightFence));

    const VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1u,
        .pWaitSemaphores = &frameState.renderFinishedSemaphore,
        .swapchainCount = 1u,
        .pSwapchains = &mSwapchain,
        .pImageIndices = &imageIndex,
    };
    VK_CHECK(vkQueuePresentKHR(mDevice.GetSelectedQueue(), &presentInfo));
}

uint32_t Swapchain::AcquireNextImage(uint64_t timeout, FrameState frameState)
{
    uint32_t nextImageIndex;
    VK_CHECK(vkAcquireNextImageKHR(mDevice, mSwapchain, timeout, frameState.imageAvailableSemaphore, VK_NULL_HANDLE, &nextImageIndex));
    return nextImageIndex;
}

Texture* Swapchain::GetTexture(uint32_t imageIndex)
{
    assert(imageIndex < mTextures.size());
    return &mTextures[imageIndex];
}