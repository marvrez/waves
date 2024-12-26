#include <cstddef>
#include <random>

#include "window.h"

#include "gui.h"
#include "camera.h"
#include "timer.h"

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
constexpr int kWorkGroupDim = 32;
constexpr int kNumIterations = 16;

struct GenericPushConstants {
    glm::vec2 size;
};

struct ExternalForcePushConstants {
    glm::vec2 mousePosition;
    glm::vec2 mouseMovement;
};

int main()
{
    Window window = Window(kWindowWidth, kWindowHeight, "fluid", false);
    Device device = Device(window, true);
    FramePacingState framePacingState = FramePacingState(device);

    const auto [framebufferWidth, framebufferHeight] = window.GetFramebufferSize();
    SwapchainDesc swapchainDesc = {
        .framebufferWidth = framebufferWidth,
        .framebufferHeight = framebufferHeight
    };
    Swapchain swapchain = Swapchain(device, swapchainDesc);
    GUI gui = GUI(device, swapchain, window);

    Camera camera = Camera(glm::vec3(0.f, 10.f, 0.f), 0.1f, 10000.f, 1000.f);

    // Create shaders & corresponding pipelines
    Shader advectCS = Shader(device, "advect.cs.spv");
    auto advectPipeline = CreateHandle<Pipeline>(
        device, PipelineDesc{
        .type = PipelineType::COMPUTE,
        .shaders = { &advectCS }
    }); 

    Shader divergenceCS = Shader(device, "divergence.cs.spv");
    auto divergencePipeline = CreateHandle<Pipeline>(
        device, PipelineDesc{
        .type = PipelineType::COMPUTE,
        .shaders = { &divergenceCS }
    });

    Shader externalForceCS = Shader(device, "externalForce.cs.spv");
    auto externalForcePipeline = CreateHandle<Pipeline>(
        device, PipelineDesc{
        .type = PipelineType::COMPUTE,
        .shaders = { &externalForceCS }
    });

    Shader pressureCS = Shader(device, "pressure.cs.spv");
    auto pressurePipeline = CreateHandle<Pipeline>(
        device, PipelineDesc{
        .type = PipelineType::COMPUTE,
        .shaders = { &pressureCS }
    });

    Shader renderCS = Shader(device, "render.cs.spv");
    auto renderPipeline = CreateHandle<Pipeline>(
        device, PipelineDesc{
        .type = PipelineType::COMPUTE,
        .shaders = { &renderCS }
    });

    Shader blitVS = Shader(device, "blit.vs.spv");
    Shader blitPS = Shader(device, "blit.ps.spv");
    auto blitPipeline = CreateHandle<Pipeline>(
        device , PipelineDesc{
        .type = PipelineType::GRAPHICS,
        .shaders = { &blitVS, &blitPS },
        .attachmentLayout = { .colorAttachments = {{ .format = swapchain.GetFormat(), .shouldEnableBlend = true }} },
        .rasterization = { .primitiveType = PrimitiveType::TRIANGLE_STRIP },
        .depthStencil = { .shouldEnableDepthTesting = true }
    });

    // Create textures
    auto velocityTexture0 = CreateHandle<Texture>(device, 
        TextureDesc{
            .dimensions = { kWindowWidth, kWindowHeight, 1u },
            .sampler = { .filter = Filter::BILINEAR, .wrapMode = WrapMode::CLAMP_TO_EDGE },
            .format = Format::RGBA32_FLOAT,
            .usage = TextureUsageBits::SAMPLED | TextureUsageBits::STORAGE
        }
    );

    auto velocityTexture1 = CreateHandle<Texture>(device, 
        TextureDesc{
            .dimensions = { kWindowWidth, kWindowHeight, 1u },
            .sampler = { .filter = Filter::BILINEAR, .wrapMode = WrapMode::CLAMP_TO_EDGE },
            .format = Format::RGBA32_FLOAT,
            .usage = TextureUsageBits::SAMPLED | TextureUsageBits::STORAGE
        }
    );

    auto divergenceTexture = CreateHandle<Texture>(device, 
        TextureDesc{
            .dimensions = { kWindowWidth, kWindowHeight, 1u },
            .sampler = { .filter = Filter::BILINEAR, .wrapMode = WrapMode::CLAMP_TO_EDGE },
            .format = Format::RGBA32_FLOAT,
            .usage = TextureUsageBits::SAMPLED | TextureUsageBits::STORAGE
        }
    );

    auto pressureTexture0 = CreateHandle<Texture>(device, 
        TextureDesc{
            .dimensions = { kWindowWidth, kWindowHeight, 1u },
            .sampler = { .filter = Filter::BILINEAR, .wrapMode = WrapMode::CLAMP_TO_EDGE },
            .format = Format::RGBA32_FLOAT,
            .usage = TextureUsageBits::SAMPLED | TextureUsageBits::STORAGE
        }
    );

    auto pressureTexture1 = CreateHandle<Texture>(device, 
        TextureDesc{
            .dimensions = { kWindowWidth, kWindowHeight, 1u },
            .sampler = { .filter = Filter::BILINEAR, .wrapMode = WrapMode::CLAMP_TO_EDGE },
            .format = Format::RGBA32_FLOAT,
            .usage = TextureUsageBits::SAMPLED | TextureUsageBits::STORAGE
        }
    );

    auto renderTexture = CreateHandle<Texture>(device, 
        TextureDesc{
            .dimensions = { kWindowWidth, kWindowHeight, 1u },
            .sampler = { .filter = Filter::BILINEAR, .wrapMode = WrapMode::CLAMP_TO_EDGE },
            .format = Format::RGBA32_FLOAT,
            .usage = TextureUsageBits::SAMPLED | TextureUsageBits::STORAGE
        }
    );

    auto [width, height] = window.GetWindowSize();
    const float aspectRatio = float(width) / float(height);
    const glm::mat4 worldToClip = camera.GetViewProjectionMatrix(aspectRatio);

    ExternalForcePushConstants externalForcePushConstants = {
        .mousePosition = glm::vec2(0.f),
        .mouseMovement = glm::vec2(0.f)
    };

    GenericPushConstants genericPushConstants = { .size = glm::vec2(kWindowWidth, kWindowHeight) };

    Timer timer;
    float dt = 0.0f;;
    uint32_t frameIndex = 0;
    while (!window.ShouldClose()) {
        window.PollEvents();
        camera.ProcessKeyboard(window, dt);
        gui.NewFrame();

        // Apply external force(s)
        {
            auto cmdList = device.CreateCommandList();
            cmdList->Open();
            auto [x, y] = window.GetCursorPosition();
            externalForcePushConstants.mouseMovement = glm::vec2(
                float(x) - externalForcePushConstants.mousePosition.x,
                float(y) - externalForcePushConstants.mousePosition.y
            );
            externalForcePushConstants.mousePosition = glm::vec2(float(x), float(y));

            cmdList->SetResourceState(*velocityTexture0, ResourceStateBits::UNORDERED_ACCESS);
            cmdList->SetComputeState({
                .pipeline = externalForcePipeline,
                .bindings = { Binding(*velocityTexture0) },
                .pushConstants = { .byteSize = sizeof(ExternalForcePushConstants), .data = &externalForcePushConstants }
            });
            cmdList->Dispatch(kWindowWidth / kWorkGroupDim, kWindowHeight / kWorkGroupDim);
            cmdList->Close();
            device.ExecuteCommandList(cmdList);
        }

        // Advect
        {
            auto cmdList = device.CreateCommandList();
            cmdList->Open();
            cmdList->SetResourceState(*velocityTexture0, ResourceStateBits::SHADER_RESOURCE);
            cmdList->SetResourceState(*velocityTexture1, ResourceStateBits::UNORDERED_ACCESS);
            cmdList->SetComputeState({
                .pipeline = advectPipeline,
                .bindings = { Binding(*velocityTexture0), Binding(*velocityTexture1) },
                .pushConstants = { .byteSize = sizeof(GenericPushConstants), .data = &genericPushConstants }
            });
            cmdList->Dispatch(kWindowWidth / kWorkGroupDim, kWindowHeight / kWorkGroupDim);
            cmdList->Close();
            device.ExecuteCommandList(cmdList);
        }

        // Divergence
        {
            auto cmdList = device.CreateCommandList();
            cmdList->Open();
            cmdList->SetResourceState(*velocityTexture1, ResourceStateBits::SHADER_RESOURCE);
            cmdList->SetResourceState(*divergenceTexture, ResourceStateBits::UNORDERED_ACCESS);
            cmdList->SetComputeState({
                .pipeline = divergencePipeline,
                .bindings = { Binding(*velocityTexture1), Binding(*divergenceTexture) },
                .pushConstants = { .byteSize = sizeof(GenericPushConstants), .data = &genericPushConstants }
            });
            cmdList->Dispatch(kWindowWidth / kWorkGroupDim, kWindowHeight / kWorkGroupDim);
            cmdList->Close();
            device.ExecuteCommandList(cmdList);
        }

        bool isPingPhase = true;
        for (int i = 0; i < kNumIterations; ++i) {
            auto cmdList = device.CreateCommandList();
            cmdList->Open();
            auto& pressureTexture      = isPingPhase ? pressureTexture0 : pressureTexture1;
            auto& outPressureTexture   = isPingPhase ? pressureTexture1 : pressureTexture0;

            cmdList->SetResourceState(*pressureTexture, ResourceStateBits::SHADER_RESOURCE);
            cmdList->SetResourceState(*outPressureTexture, ResourceStateBits::UNORDERED_ACCESS);
            cmdList->SetComputeState({
                .pipeline = pressurePipeline,
                .bindings = { Binding(*pressureTexture), Binding(*divergenceTexture), Binding(*outPressureTexture) },
                .pushConstants = { .byteSize = sizeof(GenericPushConstants), .data = &genericPushConstants }
            });
            cmdList->Dispatch(kWindowWidth / kWorkGroupDim, kWindowHeight / kWorkGroupDim);
            cmdList->Close();
            device.ExecuteCommandList(cmdList);
        }

        // Render
        {
            auto cmdList = device.CreateCommandList();
            cmdList->Open();
            cmdList->SetResourceState(*velocityTexture0, ResourceStateBits::UNORDERED_ACCESS);
            cmdList->SetResourceState(*velocityTexture1, ResourceStateBits::UNORDERED_ACCESS);
            auto& pressureTexture = isPingPhase ? pressureTexture1 : pressureTexture0;
            cmdList->SetResourceState(*pressureTexture, ResourceStateBits::UNORDERED_ACCESS);
            cmdList->SetResourceState(*renderTexture, ResourceStateBits::UNORDERED_ACCESS);

            cmdList->SetComputeState({
                .pipeline = renderPipeline,
                .bindings = { Binding(*velocityTexture1), Binding(*pressureTexture), Binding(*velocityTexture0), Binding(*renderTexture) },
            });
            cmdList->Dispatch(kWindowWidth / kWorkGroupDim, kWindowHeight / kWorkGroupDim);
            cmdList->Close();
            device.ExecuteCommandList(cmdList);
        }

        framePacingState.WaitForFrameInFlight(frameIndex);
        auto frameState = framePacingState.GetFrameState(frameIndex);

        auto cmdList = frameState.commandList;
        cmdList->Open();

        const uint32_t swapchainImageIndex = swapchain.AcquireNextImage(UINT64_MAX, frameState);
        Texture& swapchainTexture = *swapchain.GetTexture(swapchainImageIndex);

        // Blit texture
        cmdList->SetResourceState(*renderTexture, ResourceStateBits::SHADER_RESOURCE);
        cmdList->SetGraphicsState({
            .pipeline = blitPipeline,
            .viewport = swapchain.GetViewport(),
            .colorAttachments = {{
                .texture = &swapchainTexture,
                .loadOp = LoadOp::CLEAR,
                .clearColor = glm::vec4(0.674f, 0.966f, 0.988f, 1.f)
            }},
            .bindings = { Binding(*renderTexture) },
        });
        cmdList->Draw({ .vertexCount = 4 });

        cmdList->SetResourceState(swapchainTexture, ResourceStateBits::PRESENT);
        gui.DrawFrame(cmdList, swapchainTexture, frameIndex);

        cmdList->Close();
        swapchain.SubmitAndPresent(cmdList, swapchainImageIndex, frameState);

        frameIndex = (frameIndex + 1) % kMaxFramesInFlightCount;
    }
    device.WaitIdle();

    return 0;
}
