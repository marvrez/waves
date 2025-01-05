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
    int3 index;          // The index of the tile in the 3D grid.
    float3 cellPosition; // The position of the center of the cell in world space.
};

static inline int3 GetGridIndex(uint tileAddress)
{
    return int3(tileAddress & 0xFF, (tileAddress >> 8) & 0xFF, (tileAddress >> 16) & 0xFF);
}

static inline uint GetTileAddress(int3 tileIndex)
{
    return tileIndex.x + (tileIndex.y << 8) + (tileIndex.z << 16);
}

// Converts the 1D tile index (max value 4096) to a 3D tile index.
// This is done by wrapping the 1D index around a 16x16x16 grid.
static inline uint4 GetPackedTile(uint tile)
{
    return uint4(tile & 0xF, (tile >> 4) & 0xF, (tile >> 8) & 0xF, 0);
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
    const int3 gridIdx = GetGridIndex(tileAddress); // The base / start of the tile in the 3D grid.

    output.index = 16 * int3(GetPackedTile(tile).xyz) + localOffset;
    output.cellPosition = float3(16.0 * gridIdx + localOffset) / 256.0;

    return output;
}