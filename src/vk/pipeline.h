#pragma once

#include "descs.h"

class Device;
class Buffer;
class Texture;

struct RasterizationDesc {
    CullMode cullMode = CullMode::CCW;
};

struct DepthStencilDesc {
    bool shouldEnableDepthTesting = false;               // Enable depth testing.
    bool shouldEnableDepthWrite   = false;               // Enable writing to depth buffer.
    CompareOp depthCompareOp = CompareOp::GREATER;  // Comparison operation for depth testing.
};

struct ColorAttachmentDesc {
    Format format = Format::NONE;    // Color render target format.
    bool shouldEnableBlend = false;  // Enable color blending.
};

struct AttachmentLayout {
    std::initializer_list<ColorAttachmentDesc> colorAttachments;
    Format depthStencilFormat = Format::NONE;
};

struct VertexAttributeDesc {
    std::string name;
    Format format = Format::NONE;
    uint32_t binding = 0;
    uint32_t offset = 0;
    uint32_t stride = 0; // note: all strides for a given binding must be identical
    bool isInstanced = false;
};

enum class PipelineType { Compute, Graphics };

class Shader;
struct PipelineDesc {
    PipelineType type;
    std::vector<Shader*> shaders;                                    // Pipeline shaders.
    // NOTE: The parameters below are only used by *graphics* pipelines
    AttachmentLayout attachmentLayout = {};                          // Render target layout.
    RasterizationDesc rasterization = {};                            // Rasterization descriptor.
    DepthStencilDesc depthStencil = {};                              // Depth stencil descriptor.
    std::initializer_list<VertexAttributeDesc> attributeDescs = {};  // (Input) Attribute descriptions
};

union Binding {
    explicit Binding(const Buffer& buffer);
    explicit Binding(const Texture& texture);
    Binding() = delete;

    VkDescriptorImageInfo imageInfo;
    VkDescriptorBufferInfo bufferInfo;
};

struct PushConstants {
    uint32_t byteSize = 0u;
    void* data = nullptr;
};

struct Viewport {
    glm::vec2 offset = {};
    glm::vec2 extent = {};
};

struct Scissor {
    glm::ivec2 offset = {};
    glm::uvec2 extent = {};
};

struct Attachment {
    const Texture* texture = nullptr;
    LoadOp loadOp = LoadOp::DONT_CARE;
    VkClearValue clear = {};
};

struct DrawArguments {
    uint32_t vertexCount = 0;
    uint32_t instanceCount = 1;
    uint32_t startIndexLocation = 0;
    uint32_t startVertexLocation = 0;
    uint32_t startInstanceLocation = 0;
};

struct DrawDesc {
    DrawArguments drawArguments = {};
    Viewport viewport{};                                // [Graphics] Screen viewport.
    Scissor scissor{};                                  // [Graphics] Screen scissor.
    std::initializer_list<Attachment> colorAttachments; // [Graphics] Color render targets for graphics pipeline.
    Attachment depthStencilAttachment{};                // [Graphics] Depth buffer for graphics pipeline.
    std::initializer_list<Binding> bindings;            // [Optional] Resource bindings.
    PushConstants pushConstants;                        // [Optional] Uniform data.
};

class Pipeline {
public:
    Pipeline(const Device& device, const PipelineDesc& desc);
    ~Pipeline();

    void Draw(VkCommandBuffer cmdBuf, const DrawDesc& desc, std::function<void()> recordingCallback = [](){}) const;
private:
    const Device& mDevice;
    VkPipelineBindPoint mBindPoint = VK_PIPELINE_BIND_POINT_MAX_ENUM;
    VkDescriptorSetLayout mSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
    VkDescriptorUpdateTemplate mUpdateTemplate = VK_NULL_HANDLE;
    VkPipeline mPipeline = VK_NULL_HANDLE;
    VkPushConstantRange mPushConstants = {};
};