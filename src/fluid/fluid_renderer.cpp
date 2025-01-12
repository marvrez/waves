#include "fluid_renderer.h"

#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp> 

#include "gui.h"
#include "utils.h"

#include "vk/device.h"
#include "vk/swapchain.h"
#include "vk/shader.h"
#include "vk/command_list.h"
#include "vk/pipeline.h"
#include "vk/texture.h"
#include "vk/buffer.h"

constexpr glm::vec3 kDensityCenter = { 0.5f, 0.1f, 0.5f };
constexpr glm::vec3 kDensitySize = { 0.05f, 0.05f, 0.05f };

FluidRenderer::FluidRenderer(const Device& device, const Camera& camera, GUI& gui)
    : mDevice(device)
    , mCamera(camera)
    , mGui(gui)
{
    // Set up pipelines
    const auto MakeComputePipeline = [&](const char* filename) {
        Shader shader = Shader(device, filename);
        return CreateHandle<Pipeline>(
            device, PipelineDesc{
            .type = PipelineType::COMPUTE,
            .shaders = { &shader }
        });
    };
    mClearTexturePipeline = MakeComputePipeline("clear_3d_texture.cs.spv");
    mInitTilesPipeline = MakeComputePipeline("init_tiles.cs.spv");
    mAllocateTilesPipeline = MakeComputePipeline("allocate_tiles.cs.spv");
    mGenerateIndirectDispatchArgsPipeline = MakeComputePipeline("generate_indirect_dispatch_args.cs.spv");
    mGenerateDensityTilesPipeline = MakeComputePipeline("generate_density_tiles.cs.spv");
    mGenerateVelocityTilesPipeline = MakeComputePipeline("generate_velocity_tiles.cs.spv");
    mAdvectTilesPipeline = MakeComputePipeline("advect_tiles.cs.spv");
    mFreeTilesPipeline = MakeComputePipeline("free_tiles.cs.spv");
    mCommitTilesPipeline = MakeComputePipeline("commit_tiles.cs.spv");
    mDilateTilesPipeline = MakeComputePipeline("dilate_tiles.cs.spv");
    mGenerateDivergenceTilesPipeline = MakeComputePipeline("generate_divergence_tiles.cs.spv");
    mClearTilesPipeline = MakeComputePipeline("clear_tiles.cs.spv");
    mGenerateJacobiTilesPipeline = MakeComputePipeline("generate_jacobi_tiles.cs.spv");
    
    // Set up buffers
    const auto& CreateBuffer = [&](uint32_t byteSize, BufferUsageBits usage) {
        return CreateHandle<Buffer>(device, BufferDesc{
            .byteSize = byteSize, .usage = usage, .access = MemoryAccess::DEVICE,
        });
    };
    constexpr int tileSizeBytes = kTotalNumTiles * sizeof(uint32_t);
    mActiveTilesBuffer = CreateBuffer(tileSizeBytes, BufferUsageBits::STORAGE);
    mFreedTilesBuffer = CreateBuffer(tileSizeBytes, BufferUsageBits::STORAGE);
    mActiveTileAddressesBuffer = CreateBuffer(tileSizeBytes, BufferUsageBits::STORAGE);

    // Set up textures
    const auto& CreateTexture = [&](Format format, uint32_t texSize) {
        return CreateHandle<Texture>(device, TextureDesc{
            .dimensions = { texSize, texSize, texSize },
            .type = TextureType::TEXTURE_3D,
            .sampler = { .filter = Filter::POINT, .wrapMode = WrapMode::CLAMP_TO_EDGE },
            .format = format,
            .usage = TextureUsageBits::SAMPLED | TextureUsageBits::STORAGE,
        });
    };

    mDensityTilesTexture = CreateTexture(Format::RGBA32_FLOAT, kTextureSize);
    mVelocityTilesTexture = CreateTexture(Format::RGBA32_FLOAT, kTextureSize);
    mDivergenceTilesTexture = CreateTexture(Format::R32_FLOAT, kTextureSize);
    mGradientTilesTexture = CreateTexture(Format::RGBA32_FLOAT, kTextureSize);
    mTempTilesTexture = CreateTexture(Format::RGBA32_FLOAT, kTextureSize);
    mDensityAdvectedTilesTexture = CreateTexture(Format::RGBA32_FLOAT, kTextureSize);
    mVelocityAdvectedTilesTexture = CreateTexture(Format::RGBA32_FLOAT, kTextureSize);

    for (int i = 0; i < kMaxNumLevels; i++) {
        mDispatchIndirectArgsBuffer[i] = CreateBuffer(sizeof(IndirectDispatchArgs), BufferUsageBits::ARGUMENT | BufferUsageBits::STORAGE);
        
        mCounterBuffer[i] = CreateBuffer(sizeof(TilesCounter), BufferUsageBits::STORAGE);
        mTileDataBuffer[i] = CreateBuffer(tileSizeBytes, BufferUsageBits::STORAGE);
        mTileAddressesBuffer[i] = CreateBuffer(tileSizeBytes, BufferUsageBits::STORAGE);

        mJacobiTilesTexture[i] = CreateTexture(Format::R32_FLOAT, kTextureSize);
        mResidualTilesTexture[i] = CreateTexture(Format::R32_FLOAT, kTextureSize);

        const int textureSizeAtLevel = (kTextureSize / kTileSize) >> i;
        mTilesTexture[i] = CreateTexture(Format::RGBA8_UNORM, textureSizeAtLevel);
        mTileTagsTexture[i] = CreateTexture(Format::R8_UNORM, textureSizeAtLevel);
    }
}

void FluidRenderer::Render(Handle<CommandList> cmdList)
{
    FluidSimParams params = mGui.GetParams();
    if (params.currentFrame >= params.targetFrame) return;
    params.currentFrame++;
    RenderFluid(cmdList, params);
    mGui.SetParams(params);
}

void FluidRenderer::ClearTexture(Handle<Texture> texture)
{
    constexpr int kWorkGroupDim = 8;
    constexpr glm::ivec3 groupCount = glm::ivec3(kTextureSize) / kWorkGroupDim;
    Handle<CommandList> cmdList = mDevice.CreateCommandList();
    cmdList->Open();

    cmdList->SetResourceState(*texture, ResourceStateBits::UNORDERED_ACCESS);
    cmdList->SetComputeState({ .pipeline = mClearTexturePipeline, .bindings = { Binding(*texture) } });
    cmdList->Dispatch(groupCount.x, groupCount.y, groupCount.z);

    cmdList->Close();
    mDevice.ExecuteCommandList(cmdList);
    mDevice.WaitIdle();
}

void FluidRenderer::ClearTiles(Handle<CommandList> cmdList, Handle<Texture> tilesTextureToClear, Handle<Buffer> tilesBuffer, Handle<Buffer> indirectDispatchArgs)
{
    cmdList->SetResourceState(*tilesTextureToClear, ResourceStateBits::UNORDERED_ACCESS);
    cmdList->SetComputeState({
        .pipeline = mClearTilesPipeline,
        .bindings = { Binding(*tilesBuffer), Binding(*tilesTextureToClear) }
    });
    cmdList->DispatchIndirect(*indirectDispatchArgs);
}

void FluidRenderer::RenderFluid(Handle<CommandList> _, const FluidSimParams& params)
{
    // Calculate the aligned bounds in world space
    constexpr glm::vec3 worldMin = kDensityCenter - kDensitySize;
    constexpr glm::vec3 worldMax = kDensityCenter + kDensitySize;

    // Align the minimum and maximum bounds to the nearest tile grid
    constexpr int alignedMinX = GetAlignedSizeDown(static_cast<int>(worldMin.x * kTextureSize), kTileSize);
    constexpr int alignedMinY = GetAlignedSizeDown(static_cast<int>(worldMin.y * kTextureSize), kTileSize);
    constexpr int alignedMinZ = GetAlignedSizeDown(static_cast<int>(worldMin.z * kTextureSize), kTileSize);

    constexpr int alignedMaxX = GetAlignedSize(static_cast<int>(worldMax.x * kTextureSize), kTileSize);
    constexpr int alignedMaxY = GetAlignedSize(static_cast<int>(worldMax.y * kTextureSize), kTileSize);
    constexpr int alignedMaxZ = GetAlignedSize(static_cast<int>(worldMax.z * kTextureSize), kTileSize);

    // Calculate the number of invocations in each dimension
    constexpr int numInvocationsX = (alignedMaxX - alignedMinX) / kTileSize;
    constexpr int numInvocationsY = (alignedMaxY - alignedMinY) / kTileSize;
    constexpr int numInvocationsZ = (alignedMaxZ - alignedMinZ) / kTileSize;

    static bool isInitialized = false;
    if (!isInitialized) {
        // Clear all textures
        isInitialized = true;
        for (int i = 0; i < kMaxNumLevels; ++i) {
            ClearTexture(mTilesTexture[i]);
            ClearTexture(mTileTagsTexture[i]);
        }
        ClearTexture(mDensityTilesTexture);
        ClearTexture(mVelocityTilesTexture);

        {
            auto cmdList = mDevice.CreateCommandList();
            cmdList->Open();

            // Initialize the tiles
            const uint32_t numTiles = kTotalNumTiles;
            cmdList->SetComputeState({ 
                .pipeline = mInitTilesPipeline,
                .bindings = { Binding(*mCounterBuffer[0]), Binding(*mTileDataBuffer[0]) },
                .pushConstants = { .byteSize = sizeof(uint32_t), .data = (void*)&numTiles }
            });
            cmdList->Dispatch(1, 1, 1);

            // Allocate the tiles
            // Offset the group position by the aligned min position where the density is
            const glm::ivec3 minGroupPosition = glm::ivec3(alignedMinX, alignedMinY, alignedMinZ) / kTileSize;
            cmdList->SetComputeState({
                .pipeline = mAllocateTilesPipeline,
                .bindings = {
                    Binding(*mTileAddressesBuffer[0]),
                    Binding(*mTilesTexture[0]),
                    Binding(*mTileDataBuffer[0]),
                    Binding(*mCounterBuffer[0]),
                    Binding(*mTileTagsTexture[0]),
                },
                .pushConstants = { .byteSize = sizeof(glm::ivec3), .data = (void*)&minGroupPosition }
            });
            cmdList->Dispatch(numInvocationsX, numInvocationsY, numInvocationsZ);

            cmdList->Close();
            mDevice.ExecuteCommandList(cmdList);
        }
    }
    
    {
        auto cmdList = mDevice.CreateCommandList();
        cmdList->Open();

        // Generate the indirect dispatch arguments
        cmdList->SetComputeState({ 
            .pipeline = mGenerateIndirectDispatchArgsPipeline,
            .bindings = { Binding(*mCounterBuffer[0]), Binding(*mDispatchIndirectArgsBuffer[0]) }
        });
        cmdList->Dispatch(1, 1, 1);

        // Generate density tiles
        {
            const GenerateDensityTilesPushConstants pushConstants = {
                .densityCenter = kDensityCenter,
                .densityRadius = glm::compMax(kDensitySize)
            };
            // Generate the indirect dispatch arguments
            cmdList->SetComputeState({ 
                .pipeline = mGenerateDensityTilesPipeline,
                .bindings = {
                    Binding(*mTileAddressesBuffer[0]),
                    Binding(*mTileDataBuffer[0]),
                    Binding(*mDensityTilesTexture),
                },
                .pushConstants = { .byteSize = sizeof(GenerateDensityTilesPushConstants), .data = (void*)&pushConstants }
            });
            cmdList->DispatchIndirect(*mDispatchIndirectArgsBuffer[0]);
        }

        // Generate velocity tiles
        {
            const GenerateVelocityTilesPushConstants pushConstants = {
                .densityCenter = kDensityCenter,
                .densityRadius = glm::compMax(kDensitySize),
                .velocityAdvectionFactor = glm::vec4(0.0, 30.3, 0.0, 0.0)
            };
            // Generate the indirect dispatch arguments
            cmdList->SetComputeState({ 
                .pipeline = mGenerateVelocityTilesPipeline,
                .bindings = {
                    Binding(*mTileAddressesBuffer[0]),
                    Binding(*mTileDataBuffer[0]),
                    Binding(*mVelocityTilesTexture),
                },
                .pushConstants = { .byteSize = sizeof(GenerateVelocityTilesPushConstants), .data = (void*)&pushConstants }
            });
            cmdList->DispatchIndirect(*mDispatchIndirectArgsBuffer[0]);
        }

        // Advect density
        {
            const AdvectTilesPushConstants pushConstants = { .advectionFactor = glm::vec4(1.0f) };
            cmdList->SetResourceState(*mDensityAdvectedTilesTexture, ResourceStateBits::UNORDERED_ACCESS);
            cmdList->SetComputeState({
                .pipeline = mAdvectTilesPipeline,
                .bindings = {
                    Binding(*mTileAddressesBuffer[0]),
                    Binding(*mTileDataBuffer[0]),
                    Binding(*mTilesTexture[0]),
                    Binding(*mDensityTilesTexture),
                    Binding(*mVelocityTilesTexture),
                    Binding(*mDensityAdvectedTilesTexture),
                },
                .pushConstants = { .byteSize = sizeof(AdvectTilesPushConstants), .data = (void*)&pushConstants }
            });
            cmdList->DispatchIndirect(*mDispatchIndirectArgsBuffer[0]);
        }

        // Advect velocity
        {
            const AdvectTilesPushConstants pushConstants = { .advectionFactor = glm::vec4(1.0f) };
            cmdList->SetResourceState(*mVelocityAdvectedTilesTexture, ResourceStateBits::UNORDERED_ACCESS);
            cmdList->SetComputeState({ 
                .pipeline = mAdvectTilesPipeline,
                .bindings = {
                    Binding(*mTileAddressesBuffer[0]),
                    Binding(*mTileDataBuffer[0]),
                    Binding(*mTilesTexture[0]),
                    Binding(*mVelocityTilesTexture),
                    Binding(*mVelocityTilesTexture),
                    Binding(*mVelocityAdvectedTilesTexture),
                },
                .pushConstants = { .byteSize = sizeof(AdvectTilesPushConstants), .data = (void*)&pushConstants }
            });
            cmdList->DispatchIndirect(*mDispatchIndirectArgsBuffer[0]);
        }

        // Free tiles with little to no advected density
        cmdList->SetResourceState(*mTileTagsTexture[0], ResourceStateBits::UNORDERED_ACCESS);
        cmdList->SetComputeState({ 
            .pipeline = mFreeTilesPipeline,
            .bindings = {
                Binding(*mTileAddressesBuffer[0]),
                Binding(*mTileDataBuffer[0]),
                Binding(*mDensityAdvectedTilesTexture),
                Binding(*mTileTagsTexture[0]),
                Binding(*mCounterBuffer[0]),
                Binding(*mActiveTilesBuffer),
                Binding(*mFreedTilesBuffer),
                Binding(*mActiveTileAddressesBuffer),
            }
        });
        cmdList->DispatchIndirect(*mDispatchIndirectArgsBuffer[0], offsetof(IndirectDispatchArgs, flatTileListGroupCount));

        // Commit tiles
        cmdList->SetComputeState({ 
            .pipeline = mCommitTilesPipeline,
            .bindings = {
                Binding(*mActiveTilesBuffer),
                Binding(*mActiveTileAddressesBuffer),
                Binding(*mFreedTilesBuffer),
                Binding(*mTileDataBuffer[0]),
                Binding(*mCounterBuffer[0]),
                Binding(*mTileAddressesBuffer[0]),
            }
        });
        cmdList->Dispatch(1, 1, 1);

        // Dilate tiles
        cmdList->SetResourceState(*mTilesTexture[0], ResourceStateBits::UNORDERED_ACCESS);
        cmdList->SetResourceState(*mTileTagsTexture[0], ResourceStateBits::UNORDERED_ACCESS);
        cmdList->SetResourceState(*mVelocityAdvectedTilesTexture, ResourceStateBits::UNORDERED_ACCESS);
        cmdList->SetComputeState({ 
            .pipeline = mDilateTilesPipeline,
            .bindings = {
                Binding(*mTileAddressesBuffer[0]),
                Binding(*mTilesTexture[0]),
                Binding(*mTileDataBuffer[0]),
                Binding(*mCounterBuffer[0]),
                Binding(*mTileTagsTexture[0]),
                Binding(*mVelocityAdvectedTilesTexture),
            }
        });
        constexpr glm::uvec3 kDilateTilesGroupCount = glm::uvec3(kTileSize / kThreadGroupSize);
        cmdList->Dispatch(kDilateTilesGroupCount.x, kDilateTilesGroupCount.y, kDilateTilesGroupCount.z);

        // With the tiles freed and dilated, generate the indirect dispatch arguments
        cmdList->SetComputeState({
            .pipeline = mGenerateIndirectDispatchArgsPipeline,
            .bindings = { Binding(*mCounterBuffer[0]), Binding(*mDispatchIndirectArgsBuffer[0]) }
        });
        cmdList->Dispatch(1, 1, 1);

        // Generate divergence tiles
        {
            cmdList->SetResourceState(*mDivergenceTilesTexture, ResourceStateBits::UNORDERED_ACCESS);
            cmdList->SetComputeState({
                .pipeline = mGenerateDivergenceTilesPipeline,
                .bindings = {
                    Binding(*mTileAddressesBuffer[0]),
                    Binding(*mTileDataBuffer[0]),
                    Binding(*mTilesTexture[0]),
                    Binding(*mVelocityAdvectedTilesTexture),
                    Binding(*mDivergenceTilesTexture),
                }
            });
            cmdList->DispatchIndirect(*mDispatchIndirectArgsBuffer[0]);
        }

        // Perform a Multigrid V-Cycle to iteratively solve the Poisson equation
        VCycle(cmdList, mDivergenceTilesTexture, 0, kMaxNumLevels - 1);

        cmdList->Close();
        mDevice.ExecuteCommandList(cmdList);
    }
}

void FluidRenderer::Jacobi(Handle<CommandList> cmdList, const JacobiParams& params)
{
    const JacobiPushConstants pushConstants = { .alpha = params.hSquare, .omega = 6.0f / 7.0f };
    for (int i = 0; i < params.numIterations; ++i) {
        Handle<Texture> srcJacobi = i % 2 == 0       ? params.u : mTempTilesTexture;
        Handle<Texture> dstJacobi = (i + 1) % 2 == 0 ? params.u : mTempTilesTexture;
        cmdList->SetResourceState(*srcJacobi, ResourceStateBits::UNORDERED_ACCESS);
        cmdList->SetResourceState(*dstJacobi, ResourceStateBits::UNORDERED_ACCESS);
        cmdList->SetComputeState({
            .pipeline = mGenerateJacobiTilesPipeline,
            .bindings = {
                Binding(*params.tileAddresses),
                Binding(*params.tileData),
                Binding(*params.tilesIndirections),
                Binding(*params.rhs),
                Binding(*srcJacobi),
                Binding(*dstJacobi),
            },
            .pushConstants = { .byteSize = sizeof(JacobiPushConstants), .data = (void*)&pushConstants }
        });
        cmdList->DispatchIndirect(*params.dispatchIndirectArgs);
    }
}

// See https://people.eecs.berkeley.edu/~demmel/cs267/lecture25/lecture25.html
void FluidRenderer::VCycle(Handle<CommandList> cmdList, Handle<Texture> rhs, int level, int maxLevel)
{
    const JacobiParams jacobiParams = {
        .u = mJacobiTilesTexture[level],
        .rhs = rhs,
        .tilesIndirections = mTilesTexture[level],
        .tileData = mTileDataBuffer[level],
        .tileAddresses = mTileAddressesBuffer[level],
        .dispatchIndirectArgs = mDispatchIndirectArgsBuffer[level],
        .hSquare = float(level + 1),
        .numIterations = level == maxLevel ? 80 : 4, // More iterations at the coarsest level
    };
    ClearTiles(cmdList, jacobiParams.u, jacobiParams.tileData, jacobiParams.dispatchIndirectArgs);
    Jacobi(cmdList, jacobiParams);

    if (level == maxLevel) return;
}

Handle<Texture> FluidRenderer::GetDebugTilesTexture() const
{
    const auto& params = mGui.GetParams();
    const DisplayMode displayMode = params.displayMode;
    const int debugLevel = params.currentLevel;
    switch (displayMode) {
        case DisplayMode::DENSITY:      return mDensityTilesTexture;
        case DisplayMode::VELOCITY:     return mVelocityTilesTexture;
        case DisplayMode::TILE_TAG:     return mTileTagsTexture[debugLevel];
        case DisplayMode::DIVERGENCE:   return mDivergenceTilesTexture;
        case DisplayMode::JACOBI:       return mJacobiTilesTexture[debugLevel];
        case DisplayMode::RESIDUAL:     return mResidualTilesTexture[debugLevel];
        case DisplayMode::GRADIENT:     return mGradientTilesTexture;
    }
    return nullptr;
}

Handle<Texture> FluidRenderer::GetTileTagsTexture() const
{
    const auto& params = mGui.GetParams();
    return mTileTagsTexture[params.currentLevel];
}

Handle<Texture> FluidRenderer::GetTilesTexture() const
{
    const auto& params = mGui.GetParams();
    return mTilesTexture[params.currentLevel];
}