#pragma once

#include "descs.h"

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