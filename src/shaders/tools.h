#pragma once

#define EMPTY_TILE_TAG  0.0
#define COARSE_TILE_TAG 1.0
#define ACTIVE_TILE_TAG 2.0
struct TilesCounter {
    uint numTiles;
    uint numActiveTiles;
    uint numFreedTiles;
};

// NOTE: W-component is unused. Need the additional dimension since the memory is 16-byte aligned.
struct IndirectDispatchArgs {
    // (2, 2 * numActiveTiles, 2) thread groups
    // We multiply by 2 since all work groups are dispatched with 8x8x8 threads;
    // ideally, we want 16x16x16, but due to the 1024 work group invocations
    // limit, we can't do that). As such, we need to dispatch twice as many work groups.
    uint4 gridGroupCount;         
    // (1, numActiveTiles, 1) thread groups.
    // Mainly meant for processing all tiles in a flat list.
    uint4 flatTileListGroupCount;
};

struct TilePositionData {
    int3 index;          // The grid index for the tile.
    float3 tilePosition; // The position of the tile in world space.
};

// Converts the 1D tile index to a 3D tile index.
// This is done by wrapping the 1D index around a 16x16x16 grid.
static inline uint4 GetTileIndex(uint tile)
{
    return uint4(tile & 0xF, (tile >> 4) & 0xF, (tile >> 8) & 0xF, 0);
}

static inline uint GetPackedTileIndex(int3 tileIndex)
{
    return tileIndex.x + (tileIndex.y << 4) + (tileIndex.z << 8);
}

// Each output dimension is in range [0, 256)
static inline int3 GetGridIndex(uint tile, int3 offset)
{
    return 16 * int3(GetTileIndex(tile).xyz) + int3(offset.x & 0xF, offset.y & 0xF, offset.z & 0xF);
}

static inline float GetPackedTileTag(float tileTag)
{
    return tileTag / 255.0;
}

static inline float GetUnpackedTileTag(float packedTileTag)
{
    return packedTileTag * 255.0;
}

static inline TilePositionData GetTilePositionData(uint tile, uint tileAddress, int3 threadId)
{
    TilePositionData output = (TilePositionData)0;
    const int3 localOffset = int3(threadId.x & 0xF, threadId.y & 0xF, threadId.z & 0xF);
    output.index = GetGridIndex(tile, localOffset);
    output.tilePosition = float3(GetGridIndex(tileAddress, localOffset)) / 256.0;
    return output;
}

static inline float4 LoadTile(Texture3D<float4> tileTexture, Texture3D<float4> targetTexture, int3 gridIndex)
{
    const float4 tile = tileTexture.Load(int4(gridIndex.xyz / 16, 0));
    const int3 offset = int3(gridIndex.x & 0xF, gridIndex.y & 0xF, gridIndex.z & 0xF);
    const int3 baseTileIndex = int3(tile.xyz * 255.0);
    return targetTexture.Load(int4(16 * baseTileIndex + offset, 0));
}

float3 Heatmap(float t)
{
    const float3 c[10] = {
        { 0.0f / 255.0f,   2.0f / 255.0f,    91.0f / 255.0f },
        { 0.0f / 255.0f, 108.0f / 255.0f,   251.0f / 255.0f },
        { 0.0f / 255.0f, 221.0f / 255.0f,   221.0f / 255.0f },
        { 51.0f / 255.0f, 221.0f / 255.0f,    0.0f / 255.0f },
        { 255.0f / 255.0f, 252.0f / 255.0f,   0.0f / 255.0f },
        { 255.0f / 255.0f, 180.0f / 255.0f,   0.0f / 255.0f },
        { 255.0f / 255.0f, 104.0f / 255.0f,   0.0f / 255.0f },
        { 226.0f / 255.0f,  22.0f / 255.0f,   0.0f / 255.0f },
        { 191.0f / 255.0f,   0.0f / 255.0f,  83.0f / 255.0f },
        { 145.0f / 255.0f,   0.0f / 255.0f,  65.0f / 255.0f }
    };

    const float s = t * 10.0f;
    const int cur = int(s) <= 9 ? int(s) : 9;
    const int prv = cur >= 1 ? cur - 1 : 0, nxt = cur < 9 ? cur + 1 : 9;

    const float blur = 0.8f;
    const float wc = smoothstep(float(cur) - blur, float(cur) + blur, s) * (1.0f - smoothstep(float(cur + 1) - blur, float(cur + 1) + blur, s));
    const float wp = 1.0f - smoothstep(float(cur) - blur, float(cur) + blur, s), wn = smoothstep(float(cur + 1) - blur, float(cur + 1) + blur, s);

    const float3 r = wc * c[cur] + wp * c[prv] + wn * c[nxt];
    return saturate(float3(r.x, r.y, r.z));
}

static inline float2 GetUnitBoxIntersection(float3 origin, float3 direction)
{
    static const float3 kMinExtent = float3(0.0, 0.0, 0.0);
    static const float3 kMaxExtent = float3(1.0, 1.0, 1.0);

    const float3 tMin = (kMinExtent - origin) / direction;
    const float3 tMax = (kMaxExtent - origin) / direction;
    const float3 t1 = min(tMin, tMax);
    const float3 t2 = max(tMin, tMax);
    const float tNear = max(max(t1.x, t1.y), t1.z);
    const float tFar = min(min(t2.x, t2.y), t2.z);
    return float2(tNear, tFar);
}

static inline uint4 Pcg4dHash(uint4 v)
{
    v = v * 1664525u + 1013904223u;

    v.x += v.y * v.w;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v.w += v.y * v.z;

    v = v ^ (v >> 16u);

    v.x += v.y * v.w;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v.w += v.y * v.z;

    return v;
}

static inline float UintTo01Float(uint v)
{
    static const uint MANTISSA_MASK = 0x007FFFFFu;
    static const uint ONE = 0x3F800000u;
    return asfloat((v & MANTISSA_MASK) | ONE) - 1.0;
}

static inline float GetRandom(float4 p)
{
    const uint4 seed = asuint(p);
    return UintTo01Float(Pcg4dHash(seed).x);
}