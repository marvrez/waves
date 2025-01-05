#include "tools.h"

[[vk::binding(0, 0)]] StructuredBuffer<uint> gActiveTiles;
[[vk::binding(1, 0)]] StructuredBuffer<uint> gActiveTileAddresses;
[[vk::binding(2, 0)]] StructuredBuffer<uint> gFreedTiles;

[[vk::binding(3, 0)]] RWStructuredBuffer<uint> gOutTiles;
[[vk::binding(4, 0)]] RWStructuredBuffer<TilesCounter> gOutCounter;
[[vk::binding(5, 0)]] RWStructuredBuffer<uint> gOutTileAddresses;

[numthreads(1, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    const uint numActiveTiles = gOutCounter[0].numActiveTiles;
    gOutCounter[0].numTiles = numActiveTiles;

    // First, commit active tiles
    for (int i = 0; i < numActiveTiles; i++) {
        gOutTiles[i] = gActiveTiles[i];
        gOutTileAddresses[i] = gActiveTileAddresses[i];
    }

    // Then, commit freed tiles
    const uint numFreedTiles = gOutCounter[0].numFreedTiles;
    for (int i = 0; i < numFreedTiles; i++) {
        gOutTiles[numActiveTiles + i] = gFreedTiles[i];
    }

    // Finally, reset the counters for freed and active tiles
    gOutCounter[0].numActiveTiles = 0;
    gOutCounter[0].numFreedTiles = 0;
}