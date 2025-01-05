#pragma once

#define EMPTY_TILE_TAG  0.0
#define COARSE_TILE_TAG 1.0
#define ACTIVE_TILE_TAG 2.0
struct TilesCounter {
    uint numTiles;
    uint numActiveTiles;
    uint numFreedTiles;
};
struct IndirectDispatchArgs {
    // (2, 2 * numActiveTiles, 2) thread groups
    // We multiply by 2 since all work groups are dispatched with 8x8x8 threads;
    // ideally, we want 16x16x16, but due to the 1024 work group invocations
    // limit, we can't do that). As such, we need to dispatch twice as many work groups.
    uint3 gridGroupCount;         
    // (1, numActiveTiles, 1) thread groups.
    // Mainly meant for processing all tiles in a flat list.
    uint3 flatTileListGroupCount;
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

// Each output dimension is in range [0, 256)
static inline int3 GetGridIndex(uint tile, int3 offset)
{
    return 16 * int3(GetTileIndex(tile).xyz) + int3(offset.x & 0xF, offset.y & 0xF, offset.z & 0xF);
}

static inline uint GetPackedTileIndex(int3 tileIndex)
{
    return tileIndex.x + (tileIndex.y << 4) + (tileIndex.z << 8);
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