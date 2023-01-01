#pragma once

struct RasterizationDesc {
    VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;         // Rasterization cull mode.
    VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;  // Front face orientation for culling.
};

struct DepthStencilDesc {
    bool shouldEnableDepthTesting = false;               // Enable depth testing.
    bool shouldEnableDepthWrite   = false;               // Enable writing to depth buffer.
    VkCompareOp depthCompareOp = VK_COMPARE_OP_GREATER;  // Comparison operation for depth testing.
};

struct ColorAttachmentDesc {
    VkFormat format = VK_FORMAT_UNDEFINED; // Color render target format.
    bool shouldEnableBlend = false;        // Enable color blending.
};

struct AttachmentLayout {
    std::initializer_list<ColorAttachmentDesc> colorAttachments;
    VkFormat depthStencilFormat = VK_FORMAT_UNDEFINED;
};

enum class PipelineType { Compute, Graphics };

class Shader;
struct PipelineDesc {
    PipelineType type;
    std::vector<Shader*> shaders;          // Graphics pipeline shaders.
    // These parameters are only used by compute pipelines
    AttachmentLayout attachmentLayout = {};  // Render target layout.
    RasterizationDesc rasterization = {};    // Rasterization descriptor.
    DepthStencilDesc depthStencil = {};      // Depth stencil descriptor.
};

class Buffer;
class Texture;
union Binding {
    explicit Binding(const Buffer& buffer);
    explicit Binding(const Texture& texture, VkImageLayout layout);

    VkDescriptorImageInfo imageInfo;
    VkDescriptorBufferInfo bufferInfo;
};


class Device;
class Pipeline {
public:
    Pipeline(const Device& device, const PipelineDesc& desc);
    ~Pipeline();
private:
    const Device& mDevice;
    VkPipelineBindPoint mBindPoint = VK_PIPELINE_BIND_POINT_MAX_ENUM;
    VkDescriptorSetLayout mSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
    VkDescriptorUpdateTemplate mUpdateTemplate = VK_NULL_HANDLE;
    VkPipeline mPipeline = VK_NULL_HANDLE;
    VkPushConstantRange mPushConstants = {};
};