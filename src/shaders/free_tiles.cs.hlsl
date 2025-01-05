#include "tools.h"

[[vk::binding(0, 0)]] StructuredBuffer<uint> gTileAddresses;
[[vk::binding(1, 0)]] StructuredBuffer<uint> gTiles;
[[vk::binding(2, 0)]] Texture3D<float4> gAdvectionQuantityTexture; // The advected quantity, e.g. density
[[vk::binding(3, 0)]] RWTexture3D<float4> gOutTileTagsTexture;
[[vk::binding(4, 0)]] RWStructuredBuffer<TilesCounter> gOutCounter;
[[vk::binding(5, 0)]] RWStructuredBuffer<uint> gOutActiveTiles;
[[vk::binding(6, 0)]] RWStructuredBuffer<uint> gOutFreedTiles;
[[vk::binding(7, 0)]] RWStructuredBuffer<uint> gOutActiveTileAddresses;

// Any quantity above this threshold will mark the tile as active
#define QUANTITY_THRESHOLD 0.01

static inline void AddTileToActiveList(uint tile, uint tileAddress)
{
    uint numActiveTiles;
    InterlockedAdd(gOutCounter[0].numActiveTiles, 1, numActiveTiles);
    gOutActiveTiles[numActiveTiles] = tile;
    gOutActiveTileAddresses[numActiveTiles] = tileAddress;
}

static inline void MarkTileAsActive(uint tile, uint tileAddress)
{
    AddTileToActiveList(tile, tileAddress);
    const int3 tileIndex = GetTileIndex(tileAddress).xyz;
    gOutTileTagsTexture[tileIndex] = float4(GetPackedTileTag(ACTIVE_TILE_TAG), 0.0, 0.0, 0.0);
}

static inline void FreeTile(uint tile, uint tileAddress)
{
    uint numFreedTiles;
    InterlockedAdd(gOutCounter[0].numFreedTiles, 1, numFreedTiles);
    gOutFreedTiles[numFreedTiles] = tile;
    const int3 tileIndex = GetTileIndex(tileAddress).xyz;
    gOutTileTagsTexture[tileIndex] = float4(EMPTY_TILE_TAG, 0.0, 0.0, 0.0);
}

[numthreads(1, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    const uint tile = gTiles[id.y];
    const uint tileAddress = gTileAddresses[id.y];
    const int3 tileIndex = GetTileIndex(tileAddress).xyz;
    const float currentTag = GetUnpackedTileTag(gOutTileTagsTexture[tileIndex].x);

    // In a 16x16x16 neighborhood around the tile; if any of the quantities are
    // above the given threshold, mark the tile as active
    for (int z = 0; z < 16; z++) {
        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 16; x++) {
                const int3 gridIndex = GetGridIndex(tile, int3(x, y, z));
                const float4 qty = gAdvectionQuantityTexture[gridIndex];
                if (qty.x > QUANTITY_THRESHOLD) {
                    MarkTileAsActive(tile, tileAddress);
                    return;
                }
            }
        }
    }

    // Keep dilated/coarse tiles
    if (currentTag == COARSE_TILE_TAG) {
        AddTileToActiveList(tile, tileAddress);
        return;
    }

    // Free tiles that are not active
    FreeTile(tile, tileAddress);
}