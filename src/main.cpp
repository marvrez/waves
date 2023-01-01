#include "window.h"

#include "vk/device.h"
#include "vk/common.h"
#include "vk/buffer.h"
#include "vk/texture.h"
#include "vk/frame_pacing.h"
#include "vk/swapchain.h"
#include "vk/shader.h"
#include "vk/pipeline.h"

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;

int main()
{
    Window window = Window(WINDOW_WIDTH, WINDOW_HEIGHT, "sdf-edit", false);

    const char* data = "AHHHH";
    Device device = Device(window, true);
    const BufferDesc desc = {
        .byteSize = 1024,
        .access = MemoryAccess::DEVICE,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        .data = (void*)data
    };
    Buffer deviceBuffer = Buffer(device, desc);

    TextureDesc texDesc = {
        .width = 100,
        .height = 100,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
    };
    Texture tex = Texture(device, texDesc);

    FramePacingState framePacingState = FramePacingState(device);

    const auto [framebufferWidth, framebufferHeight] = window.GetFramebufferSize();
    SwapchainDesc swapchainDesc = {
        .framebufferWidth = framebufferWidth,
        .framebufferHeight = framebufferHeight
    };
    Swapchain swapchain = Swapchain(device, swapchainDesc);

    Shader shaderVS = Shader(device, "triangle.vs.spv");
    Shader shaderPS = Shader(device, "triangle.ps.spv");

    const Pipeline trianglePipeline = Pipeline(device, {
        .type = PipelineType::Graphics,
        .shaders = { &shaderVS, &shaderPS },
        .attachmentLayout = {
            .colorAttachments = {{
                .format = swapchain.GetFormat(),
                .shouldEnableBlend = true
            }},
        },
        .rasterization = {
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE
        },
        .depthStencil = { .shouldEnableDepthTesting = true }
    });

    uint32_t frameIndex = 0;
    while (!window.ShouldClose()) {
        window.PollEvents();

        auto frameState = framePacingState.GetFrameState(frameIndex);
        /*
        VK_CHECK(vkWaitForFences(device, 1, &frameState.inFlightFence, VK_TRUE, UINT64_MAX));
        VK_CHECK(vkResetFences(device, 1, &frameState.inFlightFence));
        const uint32_t swapchainImageIndex = swapchain.AcquireNextImage(
            UINT64_MAX, frameState.imageAvailableSemaphore
        );
        */

        frameIndex = (frameIndex + 1) % kMaxFramesInFlightCount;
    }

    return 0;
}