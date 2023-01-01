#include "window.h"

#include "vk/device.h"
#include "vk/common.h"
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
    Device device = Device(window, true);
    FramePacingState framePacingState = FramePacingState(device);

    const auto [framebufferWidth, framebufferHeight] = window.GetFramebufferSize();
    SwapchainDesc swapchainDesc = {
        .framebufferWidth = framebufferWidth,
        .framebufferHeight = framebufferHeight
    };
    Swapchain swapchain = Swapchain(device, swapchainDesc);

    Shader triangleVS = Shader(device, "triangle.vs.spv");
    Shader trianglePS = Shader(device, "triangle.ps.spv");

    const Pipeline trianglePipeline = Pipeline(device, {
        .type = PipelineType::Graphics,
        .shaders = { &triangleVS, &trianglePS },
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

    auto drawTriangle = [&](VkCommandBuffer cmdBuf, uint32_t currentSwapchainImageIndex) {
        const auto& extent = swapchain.GetExtent();
        const auto& swapchainTexture = swapchain.GetTexture(currentSwapchainImageIndex);
        trianglePipeline.Draw(cmdBuf, {
            .drawArguments = { .vertexCount = 3 },
            .viewport = { .offset = { 0.0f, 0.0f }, .extent = { extent.width, extent.height } },
            .scissor = { .offset = { 0, 0 }, .extent = { extent.width, extent.height } },
            .colorAttachments = {{
                .texture = &swapchainTexture,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .clear = {{ 0.0f, 0 }}
            }},
        });
    };

    uint32_t frameIndex = 0;
    while (!window.ShouldClose()) {
        window.PollEvents();

        framePacingState.WaitForFrameInFlight(frameIndex);
        auto frameState = framePacingState.GetFrameState(frameIndex);

        auto cmdBuf = frameState.commandBuffer;
        vkResetCommandBuffer(cmdBuf, 0u);
        const VkCommandBufferBeginInfo commandBufferBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        VK_CHECK(vkBeginCommandBuffer(cmdBuf, &commandBufferBeginInfo));
        const uint32_t swapchainImageIndex = swapchain.AcquireNextImage(
            UINT64_MAX, frameState.imageAvailableSemaphore
        );
        drawTriangle(cmdBuf, swapchainImageIndex);

        VK_CHECK(vkEndCommandBuffer(cmdBuf));
        swapchain.SubmitAndPresent(cmdBuf, swapchainImageIndex, frameState);

        frameIndex = (frameIndex + 1) % kMaxFramesInFlightCount;
    }
    device.WaitIdle();

    return 0;
}