#include "curved_grid_renderer.h"

#include "vk/device.h"
#include "vk/buffer.h"
#include "vk/shader.h"
#include "vk/pipeline.h"
#include "vk/swapchain.h"
#include "vk/command_list.h"

#include "camera.h"

#include <glm/glm.hpp>

#include <vector>
#include <cmath>
#include <cstddef>

struct PushConstantData {
    glm::mat4 VP;
};

struct GridVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct CurvedGrid {
    std::vector<GridVertex> vertices;
    std::vector<uint16_t> indices;
};

constexpr float kGridWidth = 20.f;
constexpr float kGridHeight = 20.f;
constexpr float kOffsetZ = 2.f;
constexpr float kQuarterCircleLength = 0.5f * M_PI; // Arc length of a quarter circle
constexpr uint32_t kTesselationSegments = 16; // Number of segments in the curved part
constexpr uint32_t kIndexCount = kTesselationSegments * 3 * 2; // 2 triangles per segment
constexpr uint32_t kVertexCount = kTesselationSegments * 2 + 2; // 2 vertices per segment + 2 for the ground plane at the start
constexpr int kPlaneIndices[] = { 0, 2, 1, 1, 2, 3 };

CurvedGrid GenerateCurvedGrid()
{
    CurvedGrid out = {};
    out.indices.reserve(kIndexCount);
    out.vertices.reserve(kVertexCount);
    
    // Curved part
    for (int i = 0; i < kTesselationSegments; ++i) {
        out.indices.push_back(i * 2 + 0);
        out.indices.push_back(i * 2 + 2);
        out.indices.push_back(i * 2 + 1);
        
        out.indices.push_back(i * 2 + 1);
        out.indices.push_back(i * 2 + 2);
        out.indices.push_back(i * 2 + 3);
    }
    // For each curved segment, generate two vertices
    for (int i = 0; i < kVertexCount / 2; ++i) {
        const float t = float(i) / float(kTesselationSegments);
        const float normalAngle = 0.5f * M_PI * t + 1.5f * M_PI; // Angles along the 3rd quadrant
        const float c = cosf(normalAngle), s = sinf(normalAngle);
        const glm::vec3 normal = glm::vec3(0.0f, -s, -c);
        out.vertices.push_back({ .position = { -kGridWidth, s + 1.f, c + kOffsetZ }, .normal = normal, .uv = {-kGridWidth, t * kQuarterCircleLength} });
        out.vertices.push_back({ .position = {  kGridWidth, s + 1.f, c + kOffsetZ }, .normal = normal, .uv = { kGridWidth, t * kQuarterCircleLength } });
    }

    // Ground plane
    for (int index : kPlaneIndices) out.indices.push_back(uint16_t(index + out.vertices.size()));
    constexpr glm::vec3 groundPlaneNormal = { 0.f, 1.f, 0.f };
    out.vertices.push_back({ .position = { -kGridWidth, 0.f, 0.0f         + kOffsetZ, }, .normal = groundPlaneNormal, .uv = { -kGridWidth, 0.0f } });
    out.vertices.push_back({ .position = {  kGridWidth, 0.f, 0.0f         + kOffsetZ, }, .normal = groundPlaneNormal, .uv = { kGridWidth,  0.0f } });
    out.vertices.push_back({ .position = { -kGridWidth, 0.f, -kGridHeight + kOffsetZ },  .normal = groundPlaneNormal, .uv = { -kGridWidth, -kGridHeight } });
    out.vertices.push_back({ .position = {  kGridWidth, 0.f, -kGridHeight + kOffsetZ },  .normal = groundPlaneNormal, .uv = { kGridWidth, -kGridHeight } });

    // Top plane
    for (int index : kPlaneIndices) out.indices.push_back(uint16_t(index + out.vertices.size()));
    constexpr glm::vec3 topPlaneNormal = { 0.f, 0.f, -1.f };
    out.vertices.push_back({ .position = { -kGridWidth, 1.f              , 1.0f + kOffsetZ }, .normal = topPlaneNormal, .uv = { -kGridWidth, kQuarterCircleLength } });
    out.vertices.push_back({ .position = {  kGridWidth, 1.f              , 1.0f + kOffsetZ }, .normal = topPlaneNormal, .uv = {  kGridWidth, kQuarterCircleLength } });
    out.vertices.push_back({ .position = { -kGridWidth, 1.f + kGridHeight, 1.0f + kOffsetZ }, .normal = topPlaneNormal, .uv = { -kGridWidth, kQuarterCircleLength + kGridHeight } });
    out.vertices.push_back({ .position = {  kGridWidth, 1.f + kGridHeight, 1.0f + kOffsetZ }, .normal = topPlaneNormal, .uv = {  kGridWidth, kQuarterCircleLength + kGridHeight } });

    return out;
}

CurvedGridRenderer::CurvedGridRenderer(const Device& device, const Swapchain& swapchain, const Camera& camera) 
    : mDevice(device), mSwapchain(swapchain), mCamera(camera)
{
    const auto& grid = GenerateCurvedGrid();

    mGrid = { 
        .vertexBuffer = CreateHandle<Buffer>(device, BufferDesc{
            .byteSize = grid.vertices.size() * sizeof(GridVertex),
            .access = MemoryAccess::HOST, // TODO: Should this be DEVICE?
            .usage = BufferUsageBits::VERTEX,
            .data = grid.vertices.data()
        }), 
        .indexBuffer = CreateHandle<Buffer>(device, BufferDesc{
            .byteSize = grid.indices.size() * sizeof(uint16_t),
            .access = MemoryAccess::HOST, // TODO: Should this be DEVICE?
            .usage = BufferUsageBits::INDEX,
            .data = grid.indices.data()
        })
    };

    Shader gridVS = Shader(device, "grid.vs.spv");
    Shader gridPS = Shader(device, "grid.ps.spv");
    const PipelineDesc pipelineDesc = {
        .type = PipelineType::GRAPHICS,
        .shaders = { &gridVS, &gridPS },
        .attachmentLayout = { .colorAttachments = {{ .format = swapchain.GetFormat(), .shouldEnableBlend = true }} },
        .rasterization = { .cullMode = CullMode::NONE },
        .depthStencil = { .depthCompareOp = CompareOp::LESS_OR_EQUAL, .shouldEnableDepthTesting = true },
        .attributeDescs = {
            { .name = "POSITION0",  .format = Format::RGB32_FLOAT, .offset = offsetof(GridVertex, position), .stride = sizeof(GridVertex) },
            { .name = "NORMAL0",    .format = Format::RGB32_FLOAT, .offset = offsetof(GridVertex, normal),   .stride = sizeof(GridVertex) },
            { .name = "TEXCOORD0",  .format = Format::RG32_FLOAT,  .offset = offsetof(GridVertex, uv),       .stride = sizeof(GridVertex) },
        },
    };
    mPipeline = CreateHandle<Pipeline>(device, pipelineDesc);
}

void CurvedGridRenderer::Render(Handle<CommandList> cmdList, const Texture& renderTarget) {
    const glm::mat4 worldToClip = mCamera.GetViewProjectionMatrix(1280.f / 720.f);
    cmdList->SetGraphicsState({
        .pipeline = mPipeline,
        .viewport = mSwapchain.GetViewport(),
        .colorAttachments = {{ .texture = &renderTarget, .loadOp = LoadOp::CLEAR, .clearColor = { 0.2f, 0.2f, 0.3f, 1.f } }},
        .vertexBuffer = mGrid.vertexBuffer,
        .indexBuffer = mGrid.indexBuffer,
        .pushConstants = { .byteSize = sizeof(glm::mat4), .data = (void*)&(worldToClip[0].x) },
    });
    const uint32_t numElements = mGrid.indexBuffer->GetSizeInBytes() / sizeof(uint16_t);
    cmdList->DrawIndexed({ .vertexCount = numElements });
}