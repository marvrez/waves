#pragma once

#include <stdint.h>

#define ENUM_CLASS_OPERATORS(T) \
    constexpr inline T operator& (T a, T b) { return static_cast<T>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b)); } \
    constexpr inline T operator| (T a, T b) { return static_cast<T>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b)); } \
    constexpr inline T operator~ (T a) { return static_cast<T>(~static_cast<uint32_t>(a)); } \
    constexpr inline T& operator|= (T& a, T b) { a = a | b; return a; }; \
    constexpr inline T& operator&= (T& a, T b) { a = a & b; return a; }; \
    constexpr inline bool operator!(T a) { return uint32_t(a) == 0; } \
    constexpr inline bool operator ==(T a, uint32_t b) { return static_cast<uint32_t>(a) == b; } \
    constexpr inline bool operator !=(T a, uint32_t b) { return static_cast<uint32_t>(a) != b; } \
    constexpr inline bool IsSet(T val, T flag) { return (val & flag) != static_cast<T>(0); } \
    constexpr inline void FlipBit(T& val, T flag) { val = IsSet(val, flag) ? (val & (~flag)) : (val | flag); }

constexpr uint32_t SetBit(uint32_t index) { return 1 << index; }

enum class Format : uint16_t {
    NONE,

    R8_UNORM, R8_UINT, R8_SRGB,

    RG8_UNORM, RG8_UINT, RG8_SRGB,

    RGBA8_UNORM, RGBA8_UINT, RGBA8_SRGB,
    BGRA8_UNORM, BGRA8_UINT, BGRA8_SRGB,

    R16_UNORM, R16_UINT, R16_FLOAT,

    RG16_UNORM, RG16_UINT, RG16_FLOAT,

    RGBA16_UNORM, RGBA16_UINT, RGBA16_FLOAT,

    R32_UINT, R32_FLOAT,

    RG32_UINT, RG32_FLOAT,

    RGB32_UINT, RGB32_FLOAT,

    RGBA32_UINT, RGBA32_FLOAT,

    RGB10A2_UNORM, RGB10A2_UINT, RG11B10_UFLOAT,

    BC7_4x4_UNORM, BC7_4x4_SRGB,

    D16_UNORM, D24_UNORM_S8_UINT, D32_FLOAT,

    COUNT
};

enum class Filter : uint8_t { POINT, BILINEAR, TRILINEAR, COUNT};
enum class WrapMode : uint16_t { WRAP, CLAMP_TO_EDGE, CLAMP_TO_BORDER, COUNT };
enum class CullMode : uint16_t { NONE, CCW, CW, COUNT };
enum class LoadOp : uint16_t { LOAD, CLEAR, DONT_CARE, COUNT };
enum class CompareOp : uint16_t {
    NEVER,
    LESS,
    EQUAL,
    LESS_OR_EQUAL,
    GREATER,
    NOT_EQUAL,
    GREATER_OR_EQUAL,
    ALWAYS,
    COUNT
};

enum class TextureUsageBits : uint16_t {
    NONE          = 0,
    SAMPLED       = SetBit(0),
    STORAGE       = SetBit(1),
    RENDER_TARGET = SetBit(2),
    DEPTH_STENCIL = SetBit(3)
};
ENUM_CLASS_OPERATORS(TextureUsageBits);

enum class BufferUsageBits : uint16_t {
    NONE      = 0,
    VERTEX    = SetBit(0), // Buffer will be bound as a vertex buffer
    INDEX     = SetBit(1), // Buffer will be bound as an index buffer
    STORAGE   = SetBit(2), // Buffer will be bound as an unordered access view (i.e., read-write)
    CONSTANT  = SetBit(3), // Buffer will be bound as a constant/uniform buffer
    ARGUMENT  = SetBit(4), // Buffer will be bound as an indirect argument buffer
};
ENUM_CLASS_OPERATORS(BufferUsageBits);


enum class ResourceStateBits : uint16_t {
    NONE                   = 0,
    COMMON                 = SetBit(0),
    CONSTANT_BUFFER        = SetBit(1),
    VERTEX_BUFFER          = SetBit(2),
    INDEX_BUFFER           = SetBit(3),
    INDIRECT_ARGUMENT      = SetBit(4),
    SHADER_RESOURCE        = SetBit(5),
    UNORDERED_ACCESS       = SetBit(6),
    RENDER_TARGET          = SetBit(7),
    DEPTH_WRITE            = SetBit(8),
    DEPTH_READ             = SetBit(9),
    COPY_DEST              = SetBit(10),
    COPY_SOURCE            = SetBit(11),
    PRESENT                = SetBit(12),
};
ENUM_CLASS_OPERATORS(ResourceStateBits)