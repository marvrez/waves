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

#include "ocean/grid.h"
#include "ocean/ocean.h"

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;
constexpr int kGridSize = 1024;
constexpr int kTextureSize = 512;
constexpr int kWorkGroupDim = 32;

static glm::vec3 GetSunDirection(const GUIParams& params)
{
    const float sunElevationRad = glm::radians((float)params.sunElevation);
    const float sunAzimuthRad = glm::radians((float)params.sunAzimuth);
    return glm::vec3(
        -glm::cos(sunElevationRad) * glm::cos(sunAzimuthRad),
        -glm::sin(sunElevationRad),
        -glm::cos(sunElevationRad) * glm::sin(sunAzimuthRad)
    );
}

static glm::vec2 GetWindDirection(const GUIParams& params)
{
    const float windAngleRad = glm::radians(params.windAngle);
    return params.windMagnitude * glm::vec2(glm::cos(windAngleRad), glm::sin(windAngleRad));
}

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

    Camera camera = Camera(glm::vec3(0.f, 10.f, 0.f), 0.1f, 10000.f, 1000.f);

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

    // Set up ocean rendering pipeline
    const Grid& grid = MakeGrid(kGridSize);
    const GridMesh& gridMesh = MakeGridMesh(device, grid);

    Shader oceanVS = Shader(device, "ocean.vs.spv");
    Shader oceanPS = Shader(device, "ocean.ps.spv");
    auto oceanPipeline = CreateHandle<Pipeline>(
        device , PipelineDesc{
        .type = PipelineType::GRAPHICS,
        .shaders = { &oceanVS, &oceanPS },
        .attachmentLayout = {
            .colorAttachments = {{
                .format = swapchain.GetFormat(),
                .shouldEnableBlend = true
            }},
        },
        .rasterization = { .cullMode = CullMode::NONE, .fillMode = RasterFillMode::SOLID},
        .attributeDescs = {
            { .name = "POSITION0", .format = Format::RGB32_FLOAT, .offset = offsetof(GridVertex, pos), .stride = sizeof(GridVertex) },
            { .name = "TEXCOORD0", .format = Format::RG32_FLOAT, .offset = offsetof(GridVertex, uv), .stride = sizeof(GridVertex) },
        },
        .depthStencil = { .shouldEnableDepthTesting = true, .depthCompareOp = CompareOp::LESS_OR_EQUAL  },
    });
    auto [width, height] = window.GetWindowSize();
    const float aspectRatio = float(width) / float(height);
    const glm::mat4 worldToClip = camera.GetViewProjectionMatrix(aspectRatio);
    OceanPushConstantData oceanPushConstantData = {
        .worldToClip = worldToClip,
        .cameraPosition = camera.GetPosition(),
        .sunDirection = GetSunDirection(gui.GetParams()),
        .displacementScaleFactor = (float)kTextureSize / kGridSize,
    };

    // Set up normal map pipeline
    Shader normalMapCS = Shader(device, "normal_map.cs.spv");
    auto normalMapPipeline = CreateHandle<Pipeline>( device, PipelineDesc{ .type = PipelineType::COMPUTE, .shaders = { &normalMapCS } });
    auto normalMapTexture = CreateHandle<Texture>(device, TextureDesc{
        .dimensions = { kTextureSize, kTextureSize, 1u },
        .sampler = { .filter = Filter::BILINEAR, .wrapMode = WrapMode::WRAP },
        .format = Format::RGBA32_FLOAT,
        .usage = TextureUsageBits::SAMPLED | TextureUsageBits::STORAGE,
    });
    NormalMapPushConstantData normalMapPushConstantData = { .texSize = kTextureSize, .oceanSize = kGridSize };

    // Set up initial spectrum pipeline
    Shader initialSpectrumCS = Shader(device, "initial_spectrum.cs.spv");
    auto initialSpectrumPipeline = CreateHandle<Pipeline>(
        device, PipelineDesc{
        .type = PipelineType::COMPUTE,
        .shaders = { &initialSpectrumCS }
    });
    auto initialSpectrumTexture = CreateHandle<Texture>(device, TextureDesc{
        .dimensions = { kTextureSize, kTextureSize, 1u },
        .format = Format::R32_FLOAT,
        .usage = TextureUsageBits::STORAGE | TextureUsageBits::SAMPLED,
    });
    InitialSpectrumPushConstantData initialSpectrumPushConstantData = {
        .texSize = kTextureSize,
        .oceanSize = kGridSize,
        .windDirection = GetWindDirection(gui.GetParams())
    };

    // Set up phase pipeline
    Shader phaseCS = Shader(device, "phase.cs.spv");
    auto phasePipeline = CreateHandle<Pipeline>(
        device, PipelineDesc{ .type = PipelineType::COMPUTE, .shaders = { &phaseCS }
    });
    PhasePushConstantData phasePushConstantData = {
        .dt = 0.0f,
        .texSize = kTextureSize,
        .oceanSize = kGridSize
    };
    // Store phases separately to ensure continuity of waves during parameter editing
    auto pingPhaseTexture = CreateHandle<Texture>(device, TextureDesc{
        .dimensions = { kTextureSize, kTextureSize, 1u },
        .format = Format::R32_FLOAT,
        .sampler = { .filter = Filter::TRILINEAR, .wrapMode = WrapMode::CLAMP_TO_BORDER },
        .usage = TextureUsageBits::STORAGE | TextureUsageBits::SAMPLED,
    });
    auto pongPhaseTexture = CreateHandle<Texture>(device, TextureDesc{
        .dimensions = { kTextureSize, kTextureSize, 1u },
        .format = Format::R32_FLOAT,
        .usage = TextureUsageBits::STORAGE | TextureUsageBits::SAMPLED,
    });

    std::vector<float> pingPhaseArray(kTextureSize * kTextureSize);
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    for (int i = 0; i < pingPhaseArray.size(); ++i) pingPhaseArray[i] = 2.0f * M_PI * dist(rng);

    Buffer pingPhaseArrayStagingBuffer = Buffer(
        device, { 
        .byteSize = pingPhaseArray.size() * sizeof(float),
        .access = MemoryAccess::HOST,
        .data = pingPhaseArray.data() 
    });
    {
        auto cmdList = device.CreateCommandList();
        cmdList->Open();
        cmdList->SetResourceState(*pingPhaseTexture, ResourceStateBits::COPY_DEST);
        cmdList->WriteTexture(pingPhaseTexture.get(), pingPhaseArrayStagingBuffer);
        cmdList->SetResourceState(*pingPhaseTexture, ResourceStateBits::UNORDERED_ACCESS);
        cmdList->Close();
        device.ExecuteCommandList(cmdList);
    }


    // Set up spectrum pipeline
    Shader spectrumCS = Shader(device, "spectrum.cs.spv");
    auto spectrumPipeline = CreateHandle<Pipeline>(
        device, PipelineDesc{ .type = PipelineType::COMPUTE, .shaders = { &spectrumCS }
    });
    SpectrumPushConstantData spectrumPushConstantData = {
        .choppiness = gui.GetParams().choppiness,
        .oceanSize = kGridSize,
        .texSize = kTextureSize
    };
    auto spectrumTexture = CreateHandle<Texture>(device, TextureDesc{
        .dimensions = { kTextureSize, kTextureSize, 1u },
        .format = Format::RGBA32_FLOAT,
        .usage = TextureUsageBits::STORAGE | TextureUsageBits::SAMPLED,
    });

    // Set up FFT pipelines
    Shader fftHorizontalCS = Shader(device, "fft_horizontal.cs.spv");
    Shader fftVerticalCS = Shader(device, "fft_vertical.cs.spv");
    auto fftHorizontalPipeline = CreateHandle<Pipeline>(
        device, PipelineDesc{ .type = PipelineType::COMPUTE, .shaders = { &fftHorizontalCS }
    });
    auto fftVerticalPipeline = CreateHandle<Pipeline>(
        device, PipelineDesc{ .type = PipelineType::COMPUTE, .shaders = { &fftVerticalCS }
    });
    auto tempTexture = CreateHandle<Texture>(device, TextureDesc{
        .dimensions = { kTextureSize, kTextureSize, 1u },
        .format = Format::RGBA32_FLOAT,
        .usage = TextureUsageBits::STORAGE | TextureUsageBits::SAMPLED,
    });
    FFTPushConstantData fftPushConstantData = { .totalCount = kTextureSize };

    // Flags
    bool shouldUpdateInitialSpectrum = true;
    bool isPingPhase = true;

    Timer timer;
    float dt = 0.0f;;
    uint32_t frameIndex = 0;
    while (!window.ShouldClose()) {
        window.PollEvents();
        camera.ProcessKeyboard(window, dt);
        gui.NewFrame();

        // Generate initial spectrum
        if (shouldUpdateInitialSpectrum) {
            auto cmdList = device.CreateCommandList();
            cmdList->Open();
            cmdList->SetResourceState(*initialSpectrumTexture, ResourceStateBits::UNORDERED_ACCESS);
            cmdList->SetComputeState({ 
                .pipeline = initialSpectrumPipeline,
                .bindings = { Binding(*initialSpectrumTexture) },
                .pushConstants = { .byteSize = sizeof(InitialSpectrumPushConstantData), .data = (void*)&initialSpectrumPushConstantData }
            });
            cmdList->Dispatch(kTextureSize / kWorkGroupDim, kTextureSize / kWorkGroupDim);
            cmdList->Close();
            device.ExecuteCommandList(cmdList);

            shouldUpdateInitialSpectrum = false;
        }

        auto& phaseTexture      = isPingPhase ? pingPhaseTexture : pongPhaseTexture;
        auto& outPhaseTexture   = isPingPhase ? pongPhaseTexture : pingPhaseTexture;
        // Generate phase
        {
            auto cmdList = device.CreateCommandList();
            cmdList->Open();

            cmdList->SetResourceState(*phaseTexture, ResourceStateBits::SHADER_RESOURCE);
            cmdList->SetResourceState(*initialSpectrumTexture, ResourceStateBits::SHADER_RESOURCE);
            cmdList->SetResourceState(*outPhaseTexture, ResourceStateBits::UNORDERED_ACCESS);

            phasePushConstantData.dt = dt;
            cmdList->SetComputeState({ 
                .pipeline = phasePipeline,
                .bindings = { Binding(*phaseTexture), Binding(*outPhaseTexture) },
                .pushConstants = { .byteSize = sizeof(PhasePushConstantData), .data = (void*)&phasePushConstantData }
            });
            cmdList->Dispatch(kTextureSize / kWorkGroupDim, kTextureSize / kWorkGroupDim);
            cmdList->Close();
            device.ExecuteCommandList(cmdList);
        }

        // Generate spectrum
        {
            spectrumPushConstantData.choppiness = gui.GetParams().choppiness;

            auto cmdList = device.CreateCommandList();
            cmdList->Open();
            cmdList->SetResourceState(*outPhaseTexture, ResourceStateBits::SHADER_RESOURCE);
            cmdList->SetResourceState(*spectrumTexture, ResourceStateBits::UNORDERED_ACCESS);
            cmdList->SetComputeState({ 
                .pipeline = spectrumPipeline,
                .bindings = { 
                    Binding(*outPhaseTexture),
                    Binding(*initialSpectrumTexture),
                    Binding(*spectrumTexture)
                },
                .pushConstants = { .byteSize = sizeof(SpectrumPushConstantData), .data = (void*)&spectrumPushConstantData }
            });
            cmdList->Dispatch(kTextureSize / kWorkGroupDim, kTextureSize / kWorkGroupDim);
            cmdList->Close();
            device.ExecuteCommandList(cmdList);
        }

        // FFT Horizontal step
        bool shouldUseTempTextureAsInput = false;
        {
            for (int p = 1; p < kTextureSize; p <<= 1) {
                fftPushConstantData.subseqCount = p;

                auto cmdList = device.CreateCommandList();
                cmdList->Open();

                auto& input = shouldUseTempTextureAsInput ? tempTexture : spectrumTexture;
                auto& output = shouldUseTempTextureAsInput ? spectrumTexture : tempTexture;

                cmdList->SetResourceState(*input, ResourceStateBits::SHADER_RESOURCE);
                cmdList->SetResourceState(*output, ResourceStateBits::UNORDERED_ACCESS);

                cmdList->SetComputeState({ 
                    .pipeline = fftHorizontalPipeline,
                    .bindings = { Binding(*input), Binding(*output) },
                    .pushConstants = { .byteSize = sizeof(FFTPushConstantData), .data = (void*)&fftPushConstantData }
                });

                cmdList->Dispatch(kTextureSize);
                cmdList->Close();
                device.ExecuteCommandList(cmdList);
            
                shouldUseTempTextureAsInput = !shouldUseTempTextureAsInput;
            }
        }

        // FFT Vertical step
        {
            for (int p = 1; p < kTextureSize; p <<= 1) {
                fftPushConstantData.subseqCount = p;

                auto cmdList = device.CreateCommandList();
                cmdList->Open();

                auto& input = shouldUseTempTextureAsInput ? tempTexture : spectrumTexture;
                auto& output = shouldUseTempTextureAsInput ? spectrumTexture : tempTexture;

                cmdList->SetResourceState(*input, ResourceStateBits::SHADER_RESOURCE);
                cmdList->SetResourceState(*output, ResourceStateBits::UNORDERED_ACCESS);

                cmdList->SetComputeState({ 
                    .pipeline = fftVerticalPipeline,
                    .bindings = { Binding(*input), Binding(*output) },
                    .pushConstants = { .byteSize = sizeof(FFTPushConstantData), .data = (void*)&fftPushConstantData }
                });

                cmdList->Dispatch(kTextureSize);
                cmdList->Close();
                device.ExecuteCommandList(cmdList);
            
                shouldUseTempTextureAsInput = !shouldUseTempTextureAsInput;
            }
        }

        // Generate normal map
        {
            auto cmdList = device.CreateCommandList();
            cmdList->Open();
            cmdList->SetResourceState(*spectrumTexture, ResourceStateBits::SHADER_RESOURCE);
            cmdList->SetResourceState(*normalMapTexture, ResourceStateBits::UNORDERED_ACCESS);
            cmdList->SetComputeState({ 
                .pipeline = normalMapPipeline,
                .bindings = { Binding(*spectrumTexture), Binding(*normalMapTexture) },
                .pushConstants = { .byteSize = sizeof(NormalMapPushConstantData), .data = (void*)&normalMapPushConstantData }
            });
            cmdList->Dispatch(kTextureSize / kWorkGroupDim, kTextureSize / kWorkGroupDim);
            cmdList->Close();
            device.ExecuteCommandList(cmdList);
        }


        framePacingState.WaitForFrameInFlight(frameIndex);
        auto frameState = framePacingState.GetFrameState(frameIndex);

        auto cmdList = frameState.commandList;
        cmdList->Open();

        const uint32_t swapchainImageIndex = swapchain.AcquireNextImage(UINT64_MAX, frameState);
        Texture& swapchainTexture = *swapchain.GetTexture(swapchainImageIndex);

        cmdList->SetResourceState(swapchainTexture, ResourceStateBits::RENDER_TARGET);

        // Ocean shading
        oceanPushConstantData.cameraPosition = camera.GetPosition();
        oceanPushConstantData.worldToClip = camera.GetViewProjectionMatrix(aspectRatio);
        oceanPushConstantData.sunDirection = GetSunDirection(gui.GetParams());
        //oceanPushConstantData.displacementScaleFactor = gui.GetParams().displacementScaleFactor;
        auto& displacementMap = shouldUseTempTextureAsInput ? tempTexture : spectrumTexture;
        cmdList->SetResourceState(*displacementMap, ResourceStateBits::SHADER_RESOURCE);
        cmdList->SetResourceState(*normalMapTexture, ResourceStateBits::SHADER_RESOURCE);
        cmdList->SetGraphicsState({
            .pipeline = oceanPipeline,
            .viewport = swapchain.GetViewport(),
            .colorAttachments = {{
                .texture = &swapchainTexture,
                .loadOp = LoadOp::CLEAR,
                .clearColor = glm::vec4(0.674f, 0.966f, 0.988f, 1.f)
            }},
            .bindings = { Binding(*displacementMap), Binding(*normalMapTexture) },
            .vertexBuffer = gridMesh.vertexBuffer,
            .indexBuffer = {.buffer = gridMesh.indexBuffer, .format = Format::R32_UINT },
            .pushConstants = { .byteSize = sizeof(OceanPushConstantData), .data = (void*)&oceanPushConstantData },
        });
        cmdList->DrawIndexed({ .vertexCount = (uint32_t)grid.indices.size() });

        cmdList->SetResourceState(swapchainTexture, ResourceStateBits::PRESENT);
        gui.DrawFrame(cmdList, swapchainTexture, frameIndex);

        cmdList->Close();
        swapchain.SubmitAndPresent(cmdList, swapchainImageIndex, frameState);

        frameIndex = (frameIndex + 1) % kMaxFramesInFlightCount;

        dt = timer.Elapsed() / 1000.0f;
        timer.Reset();

        isPingPhase = !isPingPhase;
    }
    device.WaitIdle();

    return 0;
}