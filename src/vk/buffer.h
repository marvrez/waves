#pragma once

#include "utils.h"

enum class MemoryAccess : uint8_t {
	HOST,
	DEVICE,
};

struct BufferDesc {
	uint64_t byteSize = 0ull;                       // Buffer size in bytes.
	MemoryAccess access = MemoryAccess::DEVICE;     // Buffer memory access.
	VkBufferUsageFlags usage = 0;                   // Buffer usage flags.
	void* data = nullptr;                           // [Optional] Initial buffer contents.
};

class Device;
class Buffer {
public:
    Buffer(const Device& device, BufferDesc desc);
    ~Buffer();

    VkBuffer GetVkBuffer() const { return mBuffer; }
    VkDeviceSize GetSizeInBytes() const { return mByteSize; }
    void* GetMappedData() const { return mMappedData; }

    void RecordBarrier(
        VkCommandBuffer cmdBuf,
        VkAccessFlags srcAccessMask,
        VkAccessFlags dstAccessMask,
        VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
    ) const;

private:
    const Device& mDevice;
	VkDeviceSize mByteSize = 0ull;
	VkBuffer mBuffer = VK_NULL_HANDLE;
	VmaAllocation mAllocation = VK_NULL_HANDLE;
	void* mMappedData = nullptr;
};