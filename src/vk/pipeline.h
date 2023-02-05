#pragma once

#include "descs.h"

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

class Shader;
class Device;
class Buffer;
class Texture;
class CommandList;
struct PipelineDesc {
    PipelineType type;
    std::vector<Shader*> shaders;                                    // Pipeline shaders.
    // NOTE: The parameters below are only used by *graphics* pipelines
    AttachmentLayout attachmentLayout = {};                          // Render target layout.
    RasterizationDesc rasterization = {};                            // Rasterization descriptor.
    DepthStencilDesc depthStencil = {};                              // Depth stencil descriptor.
    std::initializer_list<VertexAttributeDesc> attributeDescs = {};  // (Input) Attribute descriptions
};

class Pipeline {
public:
    Pipeline(const Device& device, const PipelineDesc& desc);
    ~Pipeline();

    PipelineType GetPipelineType() const { return mDesc.type; }

    void Bind(CommandList* cmdList) const;
    void PushConstants(CommandList* cmdList, uint32_t byteSize, void* data) const;
    void PushDescriptorSet(CommandList* cmdList, uint32_t set, void* bindingData) const;
private:
    const Device& mDevice;
    PipelineDesc mDesc;
    VkPipelineBindPoint mBindPoint = VK_PIPELINE_BIND_POINT_MAX_ENUM;
    VkDescriptorSetLayout mSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
    VkDescriptorUpdateTemplate mUpdateTemplate = VK_NULL_HANDLE;
    VkPipeline mPipeline = VK_NULL_HANDLE;
    VkPushConstantRange mPushConstants = {};
};