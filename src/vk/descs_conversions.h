#pragma once

#include "descs.h"

#include <assert.h>

constexpr VkFormat GetVkFormat(Format format)
{
    switch (format) {
        case Format::NONE:  return VK_FORMAT_UNDEFINED;

        case Format::R8_UNORM: return VK_FORMAT_R8_UNORM;
        case Format::R8_UINT:  return VK_FORMAT_R8_UINT;
        case Format::R8_SRGB:  return VK_FORMAT_R8_SRGB;

        case Format::RG8_UNORM: return VK_FORMAT_R8G8_UNORM;
        case Format::RG8_UINT:  return VK_FORMAT_R8G8_UINT;
        case Format::RG8_SRGB:  return VK_FORMAT_R8G8_SRGB;

        case Format::BGRA8_UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
        case Format::BGRA8_UINT:  return VK_FORMAT_B8G8R8A8_UINT;
        case Format::BGRA8_SRGB:  return VK_FORMAT_B8G8R8A8_SRGB;

        case Format::RGBA8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
        case Format::RGBA8_UINT:  return VK_FORMAT_R8G8B8A8_UINT;
        case Format::RGBA8_SRGB:  return VK_FORMAT_R8G8B8A8_UNORM;

        case Format::R16_UNORM:  return VK_FORMAT_R16_UNORM;
        case Format::R16_UINT:   return VK_FORMAT_R16_UINT;
        case Format::R16_FLOAT:  return VK_FORMAT_R16_SFLOAT;

        case Format::RG16_UNORM:  return VK_FORMAT_R16G16_UNORM;
        case Format::RG16_UINT:   return VK_FORMAT_R16G16_UINT;
        case Format::RG16_FLOAT:  return VK_FORMAT_R16G16_SFLOAT;

        case Format::RGBA16_UNORM:  return VK_FORMAT_R16G16B16A16_UNORM;
        case Format::RGBA16_UINT:   return VK_FORMAT_R16G16B16A16_UINT;
        case Format::RGBA16_FLOAT:  return VK_FORMAT_R16G16B16A16_SFLOAT;

        case Format::R32_UINT:      return VK_FORMAT_R32_UINT;
        case Format::R32_FLOAT:     return VK_FORMAT_R32_SFLOAT;

        case Format::RG32_UINT:     return VK_FORMAT_R32G32_UINT;
        case Format::RG32_FLOAT:    return VK_FORMAT_R32G32_SFLOAT;

        case Format::RGB32_UINT:    return VK_FORMAT_R32G32B32_UINT;
        case Format::RGB32_FLOAT:   return VK_FORMAT_R32G32B32_SFLOAT;

        case Format::RGBA32_UINT:   return VK_FORMAT_R32G32B32A32_UINT;
        case Format::RGBA32_FLOAT:  return VK_FORMAT_R32G32B32A32_SFLOAT;

        case Format::RGB10A2_UNORM:  return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
        case Format::RGB10A2_UINT:   return VK_FORMAT_A2B10G10R10_UINT_PACK32;
        case Format::RG11B10_UFLOAT: return VK_FORMAT_B10G11R11_UFLOAT_PACK32;

        // Block-compression format
        case Format::BC7_4x4_UNORM:  return VK_FORMAT_BC7_UNORM_BLOCK;
        case Format::BC7_4x4_SRGB:   return VK_FORMAT_BC7_SRGB_BLOCK;

        // Depth-specific
        case Format::D16_UNORM:         return VK_FORMAT_D16_UNORM;
        case Format::D24_UNORM_S8_UINT: return VK_FORMAT_D24_UNORM_S8_UINT;
        case Format::D32_FLOAT:         return VK_FORMAT_D32_SFLOAT;

        case Format::COUNT:             return VK_FORMAT_UNDEFINED;
    }
    return VK_FORMAT_UNDEFINED; // Shouldn't get here
}

constexpr VkFilter GetVkFilter(Filter filter)
{
    switch (filter) {
        case Filter::POINT:     return VK_FILTER_NEAREST;
        case Filter::BILINEAR:  return VK_FILTER_LINEAR;
        case Filter::TRILINEAR: return VK_FILTER_LINEAR;
        case Filter::COUNT:     return VK_FILTER_MAX_ENUM;
    }
    return (VkFilter) 0; // Shouldn't get here
}

constexpr VkSamplerMipmapMode GetVkSamplerMipMapMode(Filter filter)
{
    switch (filter) {
        case Filter::POINT:     return VK_SAMPLER_MIPMAP_MODE_NEAREST;
        case Filter::BILINEAR:  return VK_SAMPLER_MIPMAP_MODE_NEAREST;
        case Filter::TRILINEAR: return VK_SAMPLER_MIPMAP_MODE_LINEAR;
        case Filter::COUNT:     return VK_SAMPLER_MIPMAP_MODE_MAX_ENUM;
    }
    return (VkSamplerMipmapMode) 0; // Shouldn't get here
}

constexpr VkSamplerAddressMode GetVkSamplerAddressMode(WrapMode wrapMode)
{
    switch (wrapMode) {
        case WrapMode::WRAP:             return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case WrapMode::CLAMP_TO_EDGE:    return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case WrapMode::CLAMP_TO_BORDER:  return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        case WrapMode::COUNT:  return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
    }
    return (VkSamplerAddressMode) 0; // Shouldn't get here
}

constexpr VkCullModeFlags GetVkCullModeFlags(CullMode cullMode)
{
    switch (cullMode) {
        case CullMode::NONE: return VK_CULL_MODE_NONE;
        case CullMode::CCW: case CullMode::CW:
            return VK_CULL_MODE_BACK_BIT;
        case CullMode::COUNT:  return VK_CULL_MODE_FLAG_BITS_MAX_ENUM;
    }
    return (VkCullModeFlags) 0; // Shouldn't get here
}

constexpr VkFrontFace GetVkFrontFace(CullMode cullMode)
{
    switch (cullMode) {
        case CullMode::NONE:   return VK_FRONT_FACE_COUNTER_CLOCKWISE; // Don't care about value for NONE
        case CullMode::CCW:    return VK_FRONT_FACE_COUNTER_CLOCKWISE;
        case CullMode::CW:     return VK_FRONT_FACE_CLOCKWISE;
        case CullMode::COUNT:  return VK_FRONT_FACE_MAX_ENUM;
    }
    return (VkFrontFace) 0; // Shouldn't get here
}

constexpr VkImageUsageFlags GetVkImageUsageFlags(TextureUsageBits usageMask)
{
    VkImageUsageFlags flags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if ((uint16_t)usageMask & (uint16_t)TextureUsageBits::SAMPLED)       flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if ((uint16_t)usageMask & (uint16_t)TextureUsageBits::STORAGE)       flags |= VK_IMAGE_USAGE_STORAGE_BIT;
    if ((uint16_t)usageMask & (uint16_t)TextureUsageBits::RENDER_TARGET) flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if ((uint16_t)usageMask & (uint16_t)TextureUsageBits::DEPTH_STENCIL) flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    return flags;
}

constexpr VkBufferUsageFlags GetVkBufferUsageFlags(BufferUsageBits usageMask)
{
    VkBufferUsageFlags flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if ((usageMask & BufferUsageBits::VERTEX)   != 0) flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if ((usageMask & BufferUsageBits::INDEX)    != 0) flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if ((usageMask & BufferUsageBits::CONSTANT) != 0) flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if ((usageMask & BufferUsageBits::ARGUMENT) != 0) flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    if ((usageMask & BufferUsageBits::STORAGE)  != 0) flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    return flags;
}

struct ResourceStateMapping {
    ResourceStateBits resourceStateMask;
    VkPipelineStageFlags stageFlags;
    VkAccessFlags accessMask;
    VkImageLayout imageLayout;
};

constexpr ResourceStateMapping gResourceStateMap[] =
{
    { ResourceStateBits::COMMON,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_ACCESS_NONE,
        VK_IMAGE_LAYOUT_UNDEFINED },
    { ResourceStateBits::CONSTANT_BUFFER,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_ACCESS_UNIFORM_READ_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED },
    { ResourceStateBits::VERTEX_BUFFER,
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
        VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED },
    { ResourceStateBits::INDEX_BUFFER,
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
        VK_ACCESS_INDEX_READ_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED },
    { ResourceStateBits::INDIRECT_ARGUMENT,
        VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
        VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED },
    { ResourceStateBits::SHADER_RESOURCE,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_ACCESS_SHADER_READ_BIT,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
    { ResourceStateBits::UNORDERED_ACCESS,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
        VK_IMAGE_LAYOUT_GENERAL },
    { ResourceStateBits::RENDER_TARGET,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
    { ResourceStateBits::DEPTH_WRITE,
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL },
    { ResourceStateBits::DEPTH_READ,
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL },
    { ResourceStateBits::COPY_DEST,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
    { ResourceStateBits::COPY_SOURCE,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_ACCESS_TRANSFER_READ_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL },
    { ResourceStateBits::PRESENT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_ACCESS_MEMORY_READ_BIT,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR },
};

constexpr ResourceStateMapping ConvertResourceState(ResourceStateBits state)
{
    constexpr uint16_t kNumStateBits = sizeof(gResourceStateMap) / sizeof(gResourceStateMap[0]);

    ResourceStateMapping result = {};
    uint16_t stateBits = uint16_t(state);
    uint16_t bitIndex = 0;
    while (stateBits != 0 && bitIndex < kNumStateBits) {
        const uint16_t bit = (1 << bitIndex);
        if (stateBits & bit) {
            const auto& mapping = gResourceStateMap[bitIndex];
            assert(result.imageLayout == VK_IMAGE_LAYOUT_UNDEFINED || mapping.imageLayout == VK_IMAGE_LAYOUT_UNDEFINED || result.imageLayout == mapping.imageLayout);

            result.resourceStateMask = ResourceStateBits(result.resourceStateMask | mapping.resourceStateMask);
            result.accessMask |= mapping.accessMask;
            result.stageFlags |= mapping.stageFlags;
            if (mapping.imageLayout != VK_IMAGE_LAYOUT_UNDEFINED) result.imageLayout = mapping.imageLayout;

            stateBits &= ~bit;
        }
        bitIndex++;
    }
    assert(result.resourceStateMask == state);

    return result;
}


constexpr VkCompareOp GetVkCompareOp(CompareOp op)
{
    switch (op) {
        case CompareOp::NEVER:              return VK_COMPARE_OP_NEVER;
        case CompareOp::LESS:               return VK_COMPARE_OP_LESS;
        case CompareOp::EQUAL:              return VK_COMPARE_OP_EQUAL;
        case CompareOp::LESS_OR_EQUAL:      return VK_COMPARE_OP_LESS_OR_EQUAL;
        case CompareOp::GREATER:            return VK_COMPARE_OP_GREATER;
        case CompareOp::NOT_EQUAL:          return VK_COMPARE_OP_NOT_EQUAL;
        case CompareOp::GREATER_OR_EQUAL:   return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case CompareOp::ALWAYS:             return VK_COMPARE_OP_ALWAYS;
        case CompareOp::COUNT:              return VK_COMPARE_OP_MAX_ENUM;
    }
    return VK_COMPARE_OP_MAX_ENUM; // Shouldn't get here
}

constexpr VkAttachmentLoadOp GetVkAttachmentLoadOp(LoadOp op)
{
    switch (op) {
        case LoadOp::LOAD:      return VK_ATTACHMENT_LOAD_OP_LOAD;
        case LoadOp::CLEAR:     return VK_ATTACHMENT_LOAD_OP_CLEAR;
        case LoadOp::DONT_CARE: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        case LoadOp::COUNT:     return VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
    }
    return VK_ATTACHMENT_LOAD_OP_MAX_ENUM; // Shouldn't get here
}