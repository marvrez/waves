#pragma once

constexpr uint32_t kMaxFramesInFlightCount = 2;

class CommandList;
struct FrameState {
    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence     inFlightFence           = VK_NULL_HANDLE;
    Handle<CommandList> commandList     = nullptr;
};

class Device;
class FramePacingState {
public:
    FramePacingState(const Device& device);
    ~FramePacingState();

    FrameState GetFrameState(uint32_t frameIndex) const;
    void WaitForFrameInFlight(uint32_t frameIndex) const;
private:
    const Device& mDevice;
    std::array<FrameState, kMaxFramesInFlightCount> mFrameStates;
};