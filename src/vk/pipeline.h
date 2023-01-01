#pragma once

class Device;
class Buffer;
class Texture;

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

union Binding {
    explicit Binding(const Buffer& buffer);
    explicit Binding(const Texture& texture, VkImageLayout layout);

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
    VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
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

    void Draw(VkCommandBuffer cmdBuf, const DrawDesc& desc) const;
private:
    const Device& mDevice;
    VkPipelineBindPoint mBindPoint = VK_PIPELINE_BIND_POINT_MAX_ENUM;
    VkDescriptorSetLayout mSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
    VkDescriptorUpdateTemplate mUpdateTemplate = VK_NULL_HANDLE;
    VkPipeline mPipeline = VK_NULL_HANDLE;
    VkPushConstantRange mPushConstants = {};
};