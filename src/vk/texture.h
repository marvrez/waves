#pragma once

#include "descs.h"

struct SamplerDesc {
    VkFilter filterMode = VK_FILTER_LINEAR;                                             // Filter mode.
    VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;           // Address mode.
    VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;                    // Mipmap mode.
};

struct TextureDesc {
    uint32_t width = 0u;                               // Texture width in pixels.
    uint32_t height = 0u;                              // Texture height in pixels.
    uint32_t mipCount = 1u;                            // Number of mipmaps.
    Format format = Format::NONE;                      // Texture pixel format.
    VkImageUsageFlags usage = 0u;                      // Texture usage flags.
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;  // [Optional] Initial texture layout.
    VkAccessFlags access = VK_ACCESS_NONE;             // [Optional] Initial texture access.
    SamplerDesc samplerDesc = {};                      // Sampler descriptor.
    VkImage swapchainImage = VK_NULL_HANDLE;           // [Optional] Usually used for swapchain images.
};

class Device;
class Texture {
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

    void RecordBarrier(
        VkCommandBuffer cmdBuf,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        VkAccessFlags srcAccessMask,
        VkAccessFlags dstAccessMask,
        VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
    ) const;

private:
    const Device& mDevice;

    uint32_t mWidth = 0u;
    uint32_t mHeight = 0u;
    uint32_t mMipIndex = 0u;
    uint32_t mMipCount = 1u;
    Format mFormat = Format::NONE;
    Texture::SamplerState mSamplerState = {};
    VkImageView mImageView = VK_NULL_HANDLE;
    VkImage mImage = VK_NULL_HANDLE;
    VmaAllocation mAllocation = VK_NULL_HANDLE;
    bool mFromSwapchain = false;
};