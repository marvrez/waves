#include "vk/texture.h"

#include "vk/device.h"
#include "vk/common.h"
#include "vk/descs_conversions.h"

#include <map>

struct GlobalSamplerState {
    uint32_t count = 0u;
    VkSampler sampler = VK_NULL_HANDLE;
};
static std::map<std::size_t, GlobalSamplerState> gSamplerStateCache;

static VkImageAspectFlags GetAspectMask(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
}

template <class T>
inline void HashCombine(std::size_t& seed, const T& v)
{
    constexpr std::size_t kMul = 0x9ddfea08eb382d69ULL;
    std::hash<T> hasher;
    std::size_t a = (hasher(v) ^ seed) * kMul;
    a ^= (a >> 47);
    std::size_t b = (seed ^ a) * kMul;
    b ^= (b >> 47);
    seed = b * kMul;
}

static std::size_t CalculateSamplerHash(SamplerDesc desc)
{
    std::size_t hash = 0;
    HashCombine(hash, desc.filter);
    HashCombine(hash, desc.wrapMode);
    return hash;
}

static Texture::SamplerState CreateOrGetSamplerState(VkDevice device, SamplerDesc desc)
{
    const auto hash = CalculateSamplerHash(desc);

    if (auto samplerState = gSamplerStateCache.find(hash); samplerState != gSamplerStateCache.end()) {
        ++samplerState->second.count;
        return { .hash = hash, .sampler = samplerState->second.sampler };
    }

    const VkSamplerCreateInfo samplerCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = GetVkFilter(desc.filter),
        .minFilter = GetVkFilter(desc.filter),
        .mipmapMode = GetVkSamplerMipMapMode(desc.filter),
        .addressModeU = GetVkSamplerAddressMode(desc.wrapMode),
        .addressModeV = GetVkSamplerAddressMode(desc.wrapMode),
        .addressModeW = GetVkSamplerAddressMode(desc.wrapMode),
        .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
        .maxLod = 16.0f,
        .maxAnisotropy = 8.0f,
        .anisotropyEnable = VK_FALSE,
    };
    VkSampler sampler;
    VK_CHECK(vkCreateSampler(device, &samplerCreateInfo, nullptr, &sampler));
    gSamplerStateCache[hash] = { .count = 1u, .sampler = sampler };
    return { .hash = hash, .sampler = sampler };
}

static VkImageView CreateImageView(
    VkDevice device,
    VkImage image,
    VkFormat format,
    uint32_t baseMipLevel,
    uint32_t levelCount
)
{
    const VkImageViewCreateInfo imageViewCreateInfo = { 
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange = {
            .aspectMask = GetAspectMask(format),
            .baseMipLevel = baseMipLevel,
            .levelCount = levelCount,
            .baseArrayLayer = 0u,
            .layerCount = 1u,
        }
    };
    VkImageView imageView;
    VK_CHECK(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageView));
    return imageView;
}

Texture::Texture(const Device& device, TextureDesc desc)
    : mDevice(device), mDimensions(desc.dimensions), mMipIndex(0), mMipCount(desc.mipCount),
      mFormat(desc.format), mImage((VkImage)desc.resource), mFromExistingResource(desc.resource != nullptr)
{
    assert(mMipCount > 0u);
    assert(mDimensions.x != 0u && mDimensions.y != 0u && mDimensions.z != 0u);
    assert(mFormat != Format::NONE);

    if (!mFromExistingResource) {
        assert(desc.usage != TextureUsageBits::NONE);

        const VkImageCreateInfo imageCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = GetVkFormat(desc.format),
            .extent = {
                .width = desc.dimensions.x,
                .height = desc.dimensions.y,
                .depth = desc.dimensions.z,
            },
            .mipLevels = desc.mipCount,
            .arrayLayers = 1u,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = GetVkImageUsageFlags(desc.usage),
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        const VmaAllocationCreateInfo allocationCreateInfo = {
            .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        };

        VK_CHECK(
            vmaCreateImage(device.Allocator(), &imageCreateInfo,
                &allocationCreateInfo, &mImage, &mAllocation, nullptr
            )
        );
    }
    mSamplerState = CreateOrGetSamplerState(device, desc.sampler);
    mImageView = CreateImageView(device, mImage, GetVkFormat(desc.format), 0u, desc.mipCount);
}

Texture::~Texture()
{
    vkDestroyImageView(mDevice, mImageView, nullptr);

    // Destroy sampler
    GlobalSamplerState& samplerState = gSamplerStateCache[mSamplerState.hash];
    assert(samplerState.sampler == mSamplerState.sampler);
    assert(samplerState.count > 0u);
    if (samplerState.count-- == 1u) {
        vkDestroySampler(mDevice, samplerState.sampler, nullptr);
    }

    if (!mFromExistingResource) {
        vmaDestroyImage(mDevice.Allocator(), mImage, mAllocation);
    }
}

void Texture::RecordBarrier(
    VkCommandBuffer cmdBuf,
    VkImageLayout oldLayout, VkImageLayout newLayout,
    VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask
) const
{
    const VkImageMemoryBarrier imageMemoryBarrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .srcAccessMask = srcAccessMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .image = mImage,
        .subresourceRange = {
            .aspectMask = GetAspectMask(GetVkFormat(mFormat)),
            .baseMipLevel = mMipIndex,
            .levelCount = mMipCount,
            .baseArrayLayer = 0u,
            .layerCount = 1u,
        },
    };
    vkCmdPipelineBarrier(
        cmdBuf,
        srcStageMask,
        dstStageMask,
        0u,
        0u, nullptr,
        0u, nullptr,
        1u, &imageMemoryBarrier
    );

}