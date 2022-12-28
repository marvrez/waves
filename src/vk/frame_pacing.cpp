#include "vk/frame_pacing.h"

#include "vk/device.h"
#include "vk/common.h"

static inline VkSemaphore CreateSemaphore(VkDevice device)
{
    const VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkSemaphore semaphore;
    VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphore));
    return semaphore;
}

static inline VkFence CreateFence(VkDevice device)
{
    const VkFenceCreateInfo fenceCreateInfo = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT, };
    VkFence fence;
    VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));
    return fence;
}

FramePacingState::FramePacingState(const Device& device)
    : mDevice(device)
{
    for (auto& state : mFrameStates) {
        state.imageAvailableSemaphore = CreateSemaphore(device);
        state.renderFinishedSemaphore = CreateSemaphore(device);
        state.inFlightFence = CreateFence(device);
    }
}

FramePacingState::~FramePacingState()
{
    for (auto& state : mFrameStates) {
        vkDestroySemaphore(mDevice, state.renderFinishedSemaphore, nullptr);
        vkDestroySemaphore(mDevice, state.imageAvailableSemaphore, nullptr);
        vkDestroyFence(mDevice, state.inFlightFence, nullptr);
    }
}

FrameState FramePacingState::GetFrameState(uint32_t frameIndex) const
{
    assert(frameIndex < mFrameStates.size());
    return mFrameStates[frameIndex];
}