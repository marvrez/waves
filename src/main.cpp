#include "window.h"

#include "gui.h"

#include "vk/command_list.h"
#include "vk/device.h"
#include "vk/common.h"
#include "vk/texture.h"
#include "vk/frame_pacing.h"
#include "vk/swapchain.h"
#include "vk/shader.h"
#include "vk/pipeline.h"

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;

int main()
{
    Window window = Window(kWindowWidth, kWindowHeight, "sdf-edit", false);
    Device device = Device(window, true);
    FramePacingState framePacingState = FramePacingState(device);

    const auto [framebufferWidth, framebufferHeight] = window.GetFramebufferSize();
    SwapchainDesc swapchainDesc = {
        .framebufferWidth = framebufferWidth,
        .framebufferHeight = framebufferHeight
    };
    Swapchain swapchain = Swapchain(device, swapchainDesc);
    GUI gui = GUI(device, swapchain, window);

    Shader triangleVS = Shader(device, "triangle.vs.spv");
    Shader trianglePS = Shader(device, "triangle.ps.spv");

    auto trianglePipeline = CreateHandle<Pipeline>(
        device, PipelineDesc{
        .type = PipelineType::GRAPHICS,
        .shaders = { &triangleVS, &trianglePS },
        .attachmentLayout = {
            .colorAttachments = {{
                .format = swapchain.GetFormat(),
                .shouldEnableBlend = true
            }},
        },
        .depthStencil = { .shouldEnableDepthTesting = true }
    });

    uint32_t frameIndex = 0;
    while (!window.ShouldClose()) {
        window.PollEvents();
        gui.NewFrame();

        framePacingState.WaitForFrameInFlight(frameIndex);
        auto frameState = framePacingState.GetFrameState(frameIndex);

        auto cmdList = frameState.commandList;
        cmdList->Open();

        const uint32_t swapchainImageIndex = swapchain.AcquireNextImage(UINT64_MAX, frameState);
        Texture& swapchainTexture = *swapchain.GetTexture(swapchainImageIndex);

        cmdList->SetResourceState(swapchainTexture, ResourceStateBits::RENDER_TARGET);

        // Draw triangle
        cmdList->SetGraphicsState({
            .pipeline = trianglePipeline,
            .viewport = swapchain.GetViewport(),
            .colorAttachments = {{
                .texture = &swapchainTexture,
                .loadOp = LoadOp::CLEAR,
                .clearColor = glm::vec4(0.0f)
            }},
        });
        cmdList->Draw({ .vertexCount = 3 });

        gui.DrawFrame(cmdList, swapchainTexture, frameIndex);
        cmdList->SetResourceState(swapchainTexture, ResourceStateBits::PRESENT);

        cmdList->Close();
        swapchain.SubmitAndPresent(cmdList, swapchainImageIndex, frameState);

        frameIndex = (frameIndex + 1) % kMaxFramesInFlightCount;
    }
    device.WaitIdle();

    return 0;
}