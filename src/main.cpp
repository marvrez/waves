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

#include "fluid/curved_grid_renderer.h"
#include "fluid/fluid_renderer.h"
#include "fluid/raymarch_renderer.h"

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;
constexpr int kWorkGroupDim = 32;

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

    Camera camera = Camera(glm::vec3(0.5f, 0.5f, -1.f), 0.1f, 1000.f, 100.f);

    auto gridRenderer = CurvedGridRenderer(device, swapchain, camera);
    auto fluidRenderer = FluidRenderer(device, camera, gui);
    auto rayMarchRenderer = RayMarchRenderer(device);

    Shader blitVS = Shader(device, "fullscreen_quad.vs.spv");
    Shader blitPS = Shader(device, "blit.ps.spv");
    auto blitPipeline = CreateHandle<Pipeline>(
        device , PipelineDesc{
        .type = PipelineType::GRAPHICS,
        .shaders = { &blitVS, &blitPS },
        .attachmentLayout = { .colorAttachments = {{ .format = swapchain.GetFormat(), .shouldEnableBlend = true }} },
        .rasterization = { .primitiveType = PrimitiveType::TRIANGLE_STRIP },
        .depthStencil = { .shouldEnableDepthTesting = true }
    });

    gui.SetDebugImage(rayMarchRenderer.GetDebugRenderTarget());

    Timer timer;
    float dt = 0.0f;;
    uint32_t frameIndex = 0;
    while (!window.ShouldClose()) {
        window.PollEvents();
        camera.ProcessKeyboard(window, dt);
        gui.NewFrame();

        framePacingState.WaitForFrameInFlight(frameIndex);
        auto frameState = framePacingState.GetFrameState(frameIndex);

        auto cmdList = frameState.commandList;
        cmdList->Open();

        const uint32_t swapchainImageIndex = swapchain.AcquireNextImage(UINT64_MAX, frameState);
        Texture& swapchainTexture = *swapchain.GetTexture(swapchainImageIndex);

        gridRenderer.Render(cmdList, swapchainTexture);
        fluidRenderer.Render(cmdList);
        rayMarchRenderer.RenderVolume(cmdList, RenderVolumeArgs{
            .renderTarget = swapchainTexture,
            .params = gui.GetParams(),
            .camera = camera,
            .indirectionTiles = fluidRenderer.GetTilesTexture(),
            .densityTiles = fluidRenderer.GetDensityTilesTexture(),
        });
        rayMarchRenderer.Render(cmdList, RenderArgs{
            .tileTags = fluidRenderer.GetTileTagsTexture(),
            .tilesToRender = fluidRenderer.GetDebugTilesTexture(),
            .indirectionTiles = fluidRenderer.GetTilesTexture(),
            .params = gui.GetParams()
        });

        cmdList->SetResourceState(swapchainTexture, ResourceStateBits::PRESENT);
        gui.DrawFrame(cmdList, swapchainTexture, frameIndex);

        cmdList->Close();
        swapchain.SubmitAndPresent(cmdList, swapchainImageIndex, frameState);

        frameIndex = (frameIndex + 1) % kMaxFramesInFlightCount;

        dt = timer.Elapsed() / 1000.0f;
        timer.Reset();
    }
    device.WaitIdle();

    return 0;
}
