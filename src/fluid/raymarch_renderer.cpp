#include "raymarch_renderer.h"

#include "structs.h"
#include "camera.h"

#include "vk/texture.h"
#include "vk/device.h"
#include "vk/command_list.h"
#include "vk/shader.h"
#include "vk/pipeline.h"
#include "vk/buffer.h"

#include <array>

struct Render2dPushConstantData {
    int shouldRenderGrid;
    DisplayMode displayMode;
    float tileLevelScaleFactor;
    float slice;
};

struct Render3dPushConstantData {
    glm::mat4 worldToClip;
};

constexpr std::array<glm::vec3, 8> kCubeVertices = {
    glm::vec3(0.0f,  1.0f,  1.0f),
    glm::vec3(1.0f,  1.0f,  1.0f),
    glm::vec3(0.0f,  0.0f,  1.0f),
    glm::vec3(1.0f,  0.0f,  1.0f),
    glm::vec3(0.0f,  1.0f,  0.0f),
    glm::vec3(1.0f,  1.0f,  0.0f),
    glm::vec3(0.0f,  0.0f,  0.0f),
    glm::vec3(1.0f,  0.0f,  0.0f),
};

constexpr std::array<uint16_t, 36> kCubeIndices = {
    0, 1, 2, // 0
    1, 3, 2,
    4, 6, 5, // 2
    5, 6, 7,
    0, 2, 4, // 4
    4, 2, 6,
    1, 5, 3, // 6
    5, 7, 3,
    0, 4, 1, // 8
    4, 5, 1,
    2, 3, 6, // 10
    6, 3, 7,
};

RayMarchRenderer::RayMarchRenderer(const Device& device)
    : mDevice(device)
{
    mDebugRenderTarget = CreateHandle<Texture>(device, TextureDesc{
        .dimensions = { 256, 256, 1 },
        .type = TextureType::TEXTURE_2D,
        .format = Format::RGBA8_UNORM,
        .sampler = { .filter = Filter::BILINEAR, .wrapMode = WrapMode::CLAMP_TO_EDGE },
        .usage = TextureUsageBits::SAMPLED | TextureUsageBits::RENDER_TARGET,
    });

    Shader fullScreenQuadVS = Shader(device, "fullscreen_quad.vs.spv");
    Shader debugRenderPS = Shader(device, "render_2d.ps.spv");

    mPipeline = CreateHandle<Pipeline>(
        device , PipelineDesc{
        .type = PipelineType::GRAPHICS,
        .shaders = { &fullScreenQuadVS, &debugRenderPS },
        .attachmentLayout = { .colorAttachments = {{ .format = mDebugRenderTarget->GetFormat() }} },
        .rasterization = { .primitiveType = PrimitiveType::TRIANGLE_STRIP },
        .depthStencil = { .shouldEnableDepthTesting = true }
    });

    mCubeVertexBuffer = CreateHandle<Buffer>(device, BufferDesc{
        .byteSize = kCubeVertices.size() * sizeof(glm::vec3),
        .access = MemoryAccess::DEVICE,
        .usage = BufferUsageBits::VERTEX,
        .data = kCubeVertices.data()
    });
    mCubeIndexBuffer = CreateHandle<Buffer>(device, BufferDesc{
        .byteSize = kCubeIndices.size() * sizeof(uint16_t),
        .access = MemoryAccess::DEVICE,
        .usage = BufferUsageBits::INDEX,
        .data = kCubeIndices.data()
    });

    Shader render3dVS = Shader(device, "render_3d.vs.spv");
    Shader render3dPS = Shader(device, "render_3d.ps.spv");
    mRender3dPipeline = CreateHandle<Pipeline>(
        device, PipelineDesc{
        .type = PipelineType::GRAPHICS,
        .shaders = { &render3dVS, &render3dPS },
        .attachmentLayout = { .colorAttachments = {{ .format = Format::BGRA8_UNORM, .shouldEnableBlend = true }} },
        .rasterization = { .primitiveType = PrimitiveType::TRIANGLE_LIST },
        .depthStencil = { .depthCompareOp = CompareOp::LESS_OR_EQUAL },
        .attributeDescs = {{ .name = "POSITION0",  .format = Format::RGB32_FLOAT, .stride = sizeof(glm::vec3) }},
    });
}

void RayMarchRenderer::Render(Handle<CommandList> cmdList, const RenderArgs& args)
{
    const Render2dPushConstantData pushConstantsData = {
        .shouldRenderGrid = args.params.shouldShowGrid,
        .displayMode = args.params.displayMode,
        .tileLevelScaleFactor = 1.0f / powf(2.0f, args.params.currentLevel),
        .slice = args.params.slice,
    };
    cmdList->SetResourceState(*mDebugRenderTarget, ResourceStateBits::RENDER_TARGET);
    cmdList->SetResourceState(*args.tileTags, ResourceStateBits::SHADER_RESOURCE);
    cmdList->SetGraphicsState({
        .pipeline = mPipeline,
        .viewport = Viewport(mDebugRenderTarget->GetWidth(), mDebugRenderTarget->GetHeight()),
        .colorAttachments = {{ .texture = mDebugRenderTarget.get(), .loadOp = LoadOp::LOAD, .clearColor = glm::vec4(0.f, 0.f, 0.f, 1.f) }},
        .bindings = { Binding(*args.tileTags), Binding(*args.tilesToRender), Binding(*args.indirectionTiles) },
        .pushConstants = { .byteSize = sizeof(Render2dPushConstantData), .data = (void*)&pushConstantsData },
    });
    cmdList->Draw({ .vertexCount = 4 });
    cmdList->SetResourceState(*mDebugRenderTarget, ResourceStateBits::SHADER_RESOURCE);
}

void RayMarchRenderer::RenderVolume(Handle<CommandList> cmdList, const RenderVolumeArgs& args)
{
    const Render3dPushConstantData pushConstantsData = {
        .worldToClip = args.camera.GetViewProjectionMatrix(1280.f / 720.f),
    };
    cmdList->SetGraphicsState({
        .pipeline = mRender3dPipeline,
        .viewport = Viewport(args.renderTarget.GetWidth(), args.renderTarget.GetHeight()),
        .colorAttachments = {{ .texture = &args.renderTarget, .loadOp = LoadOp::LOAD }},
        .vertexBuffer = mCubeVertexBuffer,
        .indexBuffer = mCubeIndexBuffer,
        .pushConstants = { .byteSize = sizeof(Render3dPushConstantData), .data = (void*)&pushConstantsData },
    });
    cmdList->DrawIndexed({ .vertexCount = kCubeIndices.size() });
}