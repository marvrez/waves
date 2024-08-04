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
#include "vk/buffer.h"

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
    Shader testCS = Shader(device, "test.cs.spv");

    std::vector<int> data = std::vector<int>(1024, 1337);
    auto inputBuffer = CreateHandle<Buffer>(device, BufferDesc{
        .byteSize = 1024,
        .access = MemoryAccess::HOST,
        .usage = BufferUsageBits::STORAGE,
        .data = data.data(),
    });
    auto outputBuffer = CreateHandle<Buffer>(device, BufferDesc{
        .byteSize = 1024,
        .access = MemoryAccess::HOST,
        .usage = BufferUsageBits::STORAGE,
    });

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

    auto computePipeline = CreateHandle<Pipeline>(
        device, PipelineDesc{
        .type = PipelineType::COMPUTE,
        .shaders = { &testCS }
    });

    uint32_t frameIndex = 0;
    while (!window.ShouldClose()) {
        window.PollEvents();
        gui.NewFrame();

        auto computeCmdList = device.CreateCommandList();
        computeCmdList->Open();
        computeCmdList->SetComputeState({ 
            .pipeline = computePipeline,
            .bindings = { Binding(*inputBuffer), Binding(*outputBuffer) }
        });
        computeCmdList->Dispatch(1024);
        computeCmdList->Close();
        device.ExecuteCommandList(computeCmdList);
        LOG_INFO("HMM: {0}", ((int*)outputBuffer->GetMappedData())[1]);


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

        cmdList->SetResourceState(swapchainTexture, ResourceStateBits::PRESENT);
        gui.DrawFrame(cmdList, swapchainTexture, frameIndex);

        cmdList->Close();
        swapchain.SubmitAndPresent(cmdList, swapchainImageIndex, frameState);

        frameIndex = (frameIndex + 1) % kMaxFramesInFlightCount;
    }
    device.WaitIdle();

    return 0;
}