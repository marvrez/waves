#include "raymarch_renderer.h"

#include "structs.h"

#include "vk/texture.h"
#include "vk/device.h"
#include "vk/command_list.h"
#include "vk/shader.h"
#include "vk/pipeline.h"

struct Render2dPushConstantData {
    int shouldRenderGrid;
    DisplayMode displayMode;
    float tileLevelScaleFactor;
    float slice;
};

RayMarchRenderer::RayMarchRenderer(const Device& device)
    : mDevice(device)
{
    mRenderTarget = CreateHandle<Texture>(device, TextureDesc{
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
        .attachmentLayout = { .colorAttachments = {{ .format = mRenderTarget->GetFormat() }} },
        .rasterization = { .primitiveType = PrimitiveType::TRIANGLE_STRIP },
        .depthStencil = { .shouldEnableDepthTesting = true }
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
    cmdList->SetResourceState(*mRenderTarget, ResourceStateBits::RENDER_TARGET);
    cmdList->SetResourceState(*args.tileTags, ResourceStateBits::SHADER_RESOURCE);
    cmdList->SetGraphicsState({
        .pipeline = mPipeline,
        .viewport = Viewport(mRenderTarget->GetWidth(), mRenderTarget->GetHeight()),
        .colorAttachments = {{ .texture = mRenderTarget.get(), .loadOp = LoadOp::CLEAR, .clearColor = glm::vec4(0.f, 0.f, 0.f, 1.f) }},
        .bindings = { Binding(*args.tileTags), Binding(*args.tilesToRender), Binding(*args.indirectionTiles) },
        .pushConstants = { .byteSize = sizeof(Render2dPushConstantData), .data = (void*)&pushConstantsData },
    });
    cmdList->Draw({ .vertexCount = 4 });
    cmdList->SetResourceState(*mRenderTarget, ResourceStateBits::SHADER_RESOURCE);
}