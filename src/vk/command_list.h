#pragma once

#include "descs.h"

class Device;
class Buffer;
class Texture;
class Pipeline;

struct DrawArguments {
    uint32_t vertexCount = 0;
    uint32_t instanceCount = 1;
    uint32_t startIndexLocation = 0;
    uint32_t startVertexLocation = 0;
    uint32_t startInstanceLocation = 0;
};

struct ViewportState {
    Viewport viewport;
    Rect scissorRect;

    ViewportState() = default;
    ViewportState(const Viewport& viewport, const Rect& rect) : viewport(viewport), scissorRect(rect) {};
    ViewportState(const Viewport& viewport) : viewport(viewport), scissorRect(viewport) {};
};

union Binding {
    explicit Binding(const Buffer& buffer);
    explicit Binding(const Texture& texture);
    Binding() = delete;

    VkDescriptorImageInfo imageInfo;
    VkDescriptorBufferInfo bufferInfo;
};
using BindingList = std::initializer_list<Binding>;

struct PushConstants {
    uint32_t byteSize = 0u;
    void* data = nullptr;
};

struct Attachment {
    const Texture* texture = nullptr;
    LoadOp loadOp = LoadOp::DONT_CARE;
    glm::vec4 clearColor = {};
};
using AttachmentList = std::initializer_list<Attachment>;

struct IndexBuffer {
    Handle<Buffer> buffer = nullptr;
    Format format = Format::R16_UINT;
    uint32_t offset = 0;
};

struct GraphicsState {
    Handle<Pipeline> pipeline = nullptr;

    ViewportState viewport = {};
    AttachmentList colorAttachments = {};
    Attachment depthStencilAttachment = {};

    BindingList bindings = {};
    PushConstants pushConstants = {};

    IndexBuffer indexBuffer = {};
    Handle<Buffer> vertexBuffer = nullptr;
    Handle<Buffer> indirectParams = nullptr;
};

struct ComputeState {
    Handle<Pipeline> pipeline = nullptr;

    BindingList bindings = {};
    PushConstants pushConstants = {};
};

class CommandList {
public:
    CommandList(const Device& device);
    ~CommandList();

    void Open();
    void Close();
    void ClearState();

    void Draw(const DrawArguments& args);
    void DrawIndexed(const DrawArguments& args);

    void Dispatch(uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1);

    void CopyBuffer(Buffer* dest, uint64_t destOffsetBytes, const Buffer& src, uint64_t srcOffsetBytes, uint64_t dataSizeBytes);

    void WriteTexture(Texture* dest, const Buffer& src);

    void SetGraphicsState(const GraphicsState& state);
    void SetComputeState(const ComputeState& state);
    void SetResourceState(Texture& texture, ResourceStateBits dstResourceMask);

    operator VkCommandBuffer() const { return mCmdBuf; }
    VkCommandBuffer GetCommandBuffer() const { return mCmdBuf; };

private:
    void EndRendering();
    VkCommandBuffer CreateCommandBuffer() const;

    const Device& mDevice;
    VkCommandBuffer mCmdBuf = VK_NULL_HANDLE;
    GraphicsState mCurrentGraphicsState = {};
    bool mIsRendering = false;
};