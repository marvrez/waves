#include "vk/command_list.h"

#include "vk/common.h"
#include "vk/descs_conversions.h"

#include "vk/buffer.h"
#include "vk/texture.h"
#include "vk/pipeline.h"
#include "vk/device.h"

#include "logger.h"

Binding::Binding(const Buffer& buffer)
{
    bufferInfo = { .buffer = buffer.GetVkBuffer(), .offset = 0u, .range = buffer.GetSizeInBytes() };
}

Binding::Binding(const Texture& texture)
{
    imageInfo = { .sampler = texture.GetSampler(), .imageView = texture.GetView(), .imageLayout = texture.GetLayout() };
}

CommandList::CommandList(const Device& device) : mDevice(device)
{
}

VkCommandBuffer CommandList::CreateCommandBuffer() const
{
    VkCommandBuffer commandBuffer;
    VkCommandBufferAllocateInfo allocInfo = { 
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = mDevice.GetCommandPool(),
        .commandBufferCount = 1
    };
    VK_CHECK(vkAllocateCommandBuffers(mDevice, &allocInfo, &commandBuffer));
    return commandBuffer;
}

void CommandList::Open()
{
    if (mCmdBuf == VK_NULL_HANDLE) {
        mCmdBuf = this->CreateCommandBuffer();
    }
    else {
        VK_CHECK(vkResetCommandBuffer(mCmdBuf, 0u));
    }
    // Record commands
    const VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    VK_CHECK(vkBeginCommandBuffer(mCmdBuf, &beginInfo));
}

void CommandList::Close()
{
    assert(mCmdBuf != VK_NULL_HANDLE);

    this->EndRendering();
    VK_CHECK(vkEndCommandBuffer(mCmdBuf));
}

CommandList::~CommandList()
{
    vkFreeCommandBuffers(mDevice, mDevice.GetCommandPool(), 1, &mCmdBuf);
}

void CommandList::CopyBuffer(Buffer* dest, uint64_t destOffsetBytes, const Buffer& src, uint64_t srcOffsetBytes, uint64_t dataSizeBytes)
{
    const VkBufferCopy copyRegion = { .size = dataSizeBytes, .srcOffset = srcOffsetBytes, .dstOffset = destOffsetBytes };
    vkCmdCopyBuffer(mCmdBuf, src, *dest, 1, &copyRegion);
}

void CommandList::WriteTexture(Texture* dest, const Buffer& src)
{
    assert(dest->GetImage() != VK_NULL_HANDLE && src.GetVkBuffer() != VK_NULL_HANDLE);

    const auto& texState = ConvertResourceState(dest->mResourceMask);
    const glm::uvec3 texSize = dest->GetSize();
    const VkBufferImageCopy bufferCopyRegion = {
        .imageSubresource = { .aspectMask = GetAspectMask(dest->GetFormat()), .layerCount = 1 },
        .imageExtent = { .width = texSize.x, .height = texSize.y, .depth = texSize.z }
    };
    vkCmdCopyBufferToImage(
        mCmdBuf,
        src,
        dest->GetImage(),
        dest->GetLayout(),
        1, &bufferCopyRegion
    );
}

void CommandList::Draw(const DrawArguments& args)
{
    vkCmdDraw(mCmdBuf, args.vertexCount, args.instanceCount, args.startVertexLocation, args.startInstanceLocation);
}

void CommandList::DrawIndexed(const DrawArguments& args)
{
    vkCmdDrawIndexed(mCmdBuf, args.vertexCount, args.instanceCount, args.startIndexLocation, args.startVertexLocation, args.startInstanceLocation);
}

void CommandList::SetResourceState(Texture& texture, ResourceStateBits dstResourceMask)
{
    this->EndRendering(); // We cannot commit barriers while we're rendering

    const auto& srcState = ConvertResourceState(texture.mResourceMask);
    const auto& dstState = ConvertResourceState(dstResourceMask);
    const VkImageMemoryBarrier imageMemoryBarrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .srcAccessMask = srcState.accessMask,
        .dstAccessMask = dstState.accessMask,
        .oldLayout = srcState.imageLayout,
        .newLayout = dstState.imageLayout,
        .image = texture.mImage,
        .subresourceRange = {
            .aspectMask = GetAspectMask(texture.mFormat),
            .baseMipLevel = texture.mMipIndex,
            .levelCount = texture.mMipCount,
            .baseArrayLayer = 0u,
            .layerCount = 1u,
        },
    };
    vkCmdPipelineBarrier(
        mCmdBuf,
        srcState.stageFlags,
        dstState.stageFlags,
        0u,
        0u, nullptr,
        0u, nullptr,
        1u, &imageMemoryBarrier
    );
    texture.mResourceMask = dstResourceMask;
}

void CommandList::EndRendering()
{
    if (mIsRendering) {
        vkCmdEndRenderingKHR(mCmdBuf);
        mIsRendering = false;
    }
}

// TODO(optimization): only run the vk*-commands when the state has changed
void CommandList::SetGraphicsState(const GraphicsState& state)
{
    assert(state.pipeline != nullptr);
    assert(state.pipeline->GetPipelineType() == PipelineType::GRAPHICS);
    mCurrentGraphicsState = state;

    uint32_t width = 0, height = 0;
    std::vector<VkRenderingAttachmentInfo> colorAttachments;
    colorAttachments.reserve(state.colorAttachments.size());
    for (const auto& colorAttachment : state.colorAttachments) {
        assert(colorAttachment.texture->GetImage() != VK_NULL_HANDLE);

        if (width == 0)  width = colorAttachment.texture->GetWidth();
        if (height == 0) height = colorAttachment.texture->GetHeight();

        colorAttachments.push_back({
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = colorAttachment.texture->GetView(),
            .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .loadOp = GetVkAttachmentLoadOp(colorAttachment.loadOp),
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = { .color = {
                colorAttachment.clearColor.r,
                colorAttachment.clearColor.g,
                colorAttachment.clearColor.b,
                colorAttachment.clearColor.a 
            }},
        });
    }

    if (state.vertexBuffer != nullptr) {
        const VkBuffer vertexBuffers[] = { *state.vertexBuffer };
        const VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(mCmdBuf, 0, 1, vertexBuffers, offsets);
    }
    if (state.indexBuffer.buffer != nullptr) {
        vkCmdBindIndexBuffer(mCmdBuf,
            *state.indexBuffer.buffer,
            state.indexBuffer.offset,
            state.indexBuffer.format == Format::R16_UINT ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32
        );
    }
 
    VkRenderingInfo renderingInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .layerCount = 1,
        .renderArea = { .offset = { .x = 0, .y = 0 }, .extent = { .width = width, .height = height, } },
        .colorAttachmentCount = uint32_t(colorAttachments.size()),
        .pColorAttachments = colorAttachments.data(),
    };

    VkRenderingAttachmentInfo depthAttachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
    if (state.depthStencilAttachment.texture != nullptr && state.depthStencilAttachment.texture->GetImage() != VK_NULL_HANDLE) {
        depthAttachment.imageView = state.depthStencilAttachment.texture->GetView();
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = GetVkAttachmentLoadOp(state.depthStencilAttachment.loadOp);
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.clearValue.depthStencil = {0};

        renderingInfo.pDepthAttachment = &depthAttachment;
    }

    vkCmdBeginRenderingKHR(mCmdBuf, &renderingInfo);
    mIsRendering = true;

    if (state.viewport.viewport.width() != 0.0f && state.viewport.viewport.height() != 0.0f) {
        const VkViewport viewport = {
            .x = state.viewport.viewport.minX, .y = state.viewport.viewport.minY,
            .width = std::abs(state.viewport.viewport.width()), .height = std::abs(state.viewport.viewport.height()),
            .minDepth = state.viewport.viewport.minZ, .maxDepth = state.viewport.viewport.maxZ
        };
        vkCmdSetViewport(mCmdBuf, 0u, 1u, &viewport);
    }

    if (state.viewport.scissorRect.width() != 0 && state.viewport.scissorRect.height() != 0) {
        const VkRect2D scissorRect = {
            .offset = { .x = state.viewport.scissorRect.minX, .y = state.viewport.scissorRect.minY },
            .extent = { .width = (uint32_t)std::abs(state.viewport.scissorRect.width()), .height = (uint32_t)std::abs(state.viewport.scissorRect.height()) }
        };
        vkCmdSetScissor(mCmdBuf, 0u, 1u, &scissorRect);
    }

    const auto& pushConstants = state.pushConstants;
    if (pushConstants.byteSize != 0u && pushConstants.data != nullptr) {
        state.pipeline->PushConstants(this, pushConstants.byteSize, pushConstants.data);
    }
    if(state.bindings.size() > 0) {
        state.pipeline->PushDescriptorSet(this, 0, (void*)state.bindings.begin());
    }
    state.pipeline->Bind(this);
}


void CommandList::SetComputeState(const ComputeState& state)
{
    assert(state.pipeline != nullptr);
    assert(state.pipeline->GetPipelineType() == PipelineType::COMPUTE);

    const auto& pushConstants = state.pushConstants;
    if (pushConstants.byteSize != 0u && pushConstants.data != nullptr) {
        state.pipeline->PushConstants(this, pushConstants.byteSize, pushConstants.data);
    }
    if(state.bindings.size() > 0) {
        state.pipeline->PushDescriptorSet(this, 0, (void*)state.bindings.begin());
    }
    state.pipeline->Bind(this);
}

void CommandList::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    vkCmdDispatch(mCmdBuf, groupCountX, groupCountY, groupCountZ);
    vkCmdPipelineBarrier(mCmdBuf, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);
}