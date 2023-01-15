#include "vk/buffer.h"
#include "vk/device.h"
#include "vk/common.h"
#include "vk/descs_conversions.h"

Buffer::Buffer(const Device& device, BufferDesc desc)
    : mDevice(device), mByteSize(desc.byteSize)
{
    assert(desc.byteSize > 0u);

    const VkBufferCreateInfo bufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = desc.byteSize,
        .usage = GetVkBufferUsageFlags(desc.usage),
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    const VmaAllocationCreateInfo allocationCreateInfo = {
        .usage = VMA_MEMORY_USAGE_AUTO,
        // This enables only sequential writes into this memory,
        // so if we end up needing random access, use VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT.
        .flags = desc.access == MemoryAccess::HOST ? VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT : 0u,
    };
    VK_CHECK(vmaCreateBuffer(device.Allocator(), &bufferCreateInfo, &allocationCreateInfo, &mBuffer, &mAllocation, nullptr));

    // Persistently mapped memory
    if (desc.access == MemoryAccess::HOST) {
        vmaMapMemory(device.Allocator(), mAllocation, &mMappedData);
    }

    if (desc.data) {
        if (desc.access == MemoryAccess::HOST) {
            memcpy(mMappedData, desc.data, mByteSize);
        }
        else {
            const Buffer stagingBuffer = Buffer(device, {
                .byteSize = desc.byteSize,
                .access = MemoryAccess::HOST,
                .data = desc.data
            });
            device.Submit([&](VkCommandBuffer cmdBuf) {
                const VkBufferCopy copyRegion = { .size = desc.byteSize };
                vkCmdCopyBuffer(cmdBuf, stagingBuffer.GetVkBuffer(), mBuffer, 1, &copyRegion);
            });
        }
    }
}

Buffer::~Buffer()
{
    if (mMappedData) {
        vmaUnmapMemory(mDevice.Allocator(), mAllocation);
    }
    vmaDestroyBuffer(mDevice.Allocator(), mBuffer, mAllocation);
}

void Buffer::SetLayout(
    VkCommandBuffer cmdBuf, VkAccessFlags dstAccessMask, VkPipelineStageFlags dstStageMask
)
{
    const VkBufferMemoryBarrier memoryBarrier = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .buffer = mBuffer,
        .size = mByteSize,
        .srcAccessMask = mAccessMask,
        .dstAccessMask = dstAccessMask,
        .srcQueueFamilyIndex = mDevice.GetSelectedQueueIndex(),
        .dstQueueFamilyIndex = mDevice.GetSelectedQueueIndex(),
    };
    vkCmdPipelineBarrier(
        cmdBuf,
        mStageMask,
        dstStageMask,
        0u,
        0u, nullptr,
        1u, &memoryBarrier,
        0u, nullptr
    );

    // Update the current resource state
    mAccessMask = dstAccessMask;
    mStageMask = dstStageMask;
}