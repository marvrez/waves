#pragma once

#include "utils.h"
#include "vk/descs.h"

struct BufferDesc {
	uint64_t byteSize = 0ull;                       // Buffer size in bytes.
	MemoryAccess access = MemoryAccess::DEVICE;     // Buffer memory access.
	BufferUsageBits usage = BufferUsageBits::NONE;  // Buffer usage flags.
	const void* data = nullptr;                           // [Optional] Initial buffer contents.
};

class Device;
class Buffer {
public:
    Buffer(const Device& device, BufferDesc desc);
    ~Buffer();

    VkBuffer GetVkBuffer() const { return mBuffer; }
	operator VkBuffer() const { return mBuffer; }

    VkDeviceSize GetSizeInBytes() const { return mByteSize; }
    void* GetMappedData() const { return mMappedData; }

private:
    const Device& mDevice;
	VkDeviceSize mByteSize = 0ull;
	VkBuffer mBuffer = VK_NULL_HANDLE;
	VmaAllocation mAllocation = VK_NULL_HANDLE;
	void* mMappedData = nullptr;

    // ResourceStateBits mResourceMask = ResourceStateBits::COMMON;
    VkAccessFlags mAccessMask = VK_ACCESS_NONE;
    VkPipelineStageFlags mStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
};