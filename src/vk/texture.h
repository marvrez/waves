#pragma once

#include "descs.h"

struct SamplerDesc {
    Filter filter = Filter::TRILINEAR;
    WrapMode wrapMode = WrapMode::CLAMP_TO_EDGE;
};

struct TextureDesc {
    glm::uvec3 dimensions = glm::uvec3(0u);            // Texture dimensions
    uint32_t mipCount = 1u;                            // Number of mipmaps.
    Format format = Format::NONE;                      // Texture pixel format.
    TextureUsageBits usage = TextureUsageBits::NONE;   // Texture usage flags.
    SamplerDesc sampler = {};                          // Sampler descriptor.
    void* resource = nullptr;                          // [Optional] Usually used for swapchain images.
};

class Device;
class CommandList;
class Texture {
friend class CommandList;
public:

    struct SamplerState {
        std::size_t hash;
        VkSampler sampler;
    };

    Texture(const Device& device, TextureDesc desc);
    ~Texture();

    VkImageView GetView() const { return mImageView; }
    VkImage GetImage() const { return mImage; }
    VkSampler GetSampler() const { return mSamplerState.sampler; }
    Format GetFormat() const { return mFormat; }
    VkImageLayout GetLayout() const;

    glm::uvec3 GetSize() const { return mDimensions; }
    uint32_t GetWidth() const { return mDimensions.x; }
    uint32_t GetHeight() const { return mDimensions.y; }
    uint32_t GetDepth() const { return mDimensions.z; }

private:
    const Device& mDevice;

    glm::uvec3 mDimensions;
    uint32_t mMipIndex = 0u;
    uint32_t mMipCount = 1u;
    Format mFormat = Format::NONE;
    Texture::SamplerState mSamplerState = {};
    VkImageView mImageView = VK_NULL_HANDLE;
    VkImage mImage = VK_NULL_HANDLE;
    VmaAllocation mAllocation = VK_NULL_HANDLE;
    bool mFromExistingResource = false;

    ResourceStateBits mResourceMask = ResourceStateBits::COMMON;
};