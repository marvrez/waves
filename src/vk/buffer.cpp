#include "vk/buffer.h"
#include "vk/device.h"
#include "vk/command_list.h"
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
            Handle<CommandList> cmdList = device.CreateCommandList();
            cmdList->Open();
            cmdList->CopyBuffer(this, 0, stagingBuffer, 0, desc.byteSize);
            cmdList->Close();
            device.ExecuteCommandList(cmdList);
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