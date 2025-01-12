#include "tools.h"

[[vk::binding(0, 0)]] StructuredBuffer<uint> gTileData;
[[vk::binding(1, 0)]] Texture3D<float> gTileTags;
[[vk::binding(2, 0)]] RWStructuredBuffer<uint> gOutTileAddresses;
[[vk::binding(3, 0)]] RWTexture3D<float4> gOutCoarserTiles;
[[vk::binding(4, 0)]] RWStructuredBuffer<TilesCounter> gOutCounter;
[[vk::binding(5, 0)]] RWTexture3D<float> gOutCoarserTileTags;

#define DX int3(1, 0, 0)
#define DY int3(0, 1, 0)

void AllocateCoarseTile(int3 tileIndex)
{
    uint currentTileCount;
    InterlockedAdd(gOutCounter[0].numTiles, 1, currentTileCount);

    const uint tile = gTileData[currentTileCount];
    gOutTileAddresses[currentTileCount] = GetPackedTileIndex(tileIndex);
    gOutCoarserTileTags[tileIndex] = GetPackedTileTag(COARSE_TILE_TAG);
    gOutCoarserTiles[tileIndex] = GetTileIndex(tile) / 255.0;
}

[numthreads(8, 8, 8)]
void main(uint3 id : SV_DispatchThreadID)
{
    const int3 tileIdx = 2 * id;
    const float tag00 = gTileTags[tileIdx];
    const float tag10 = gTileTags[tileIdx + DX];
    const float tag01 = gTileTags[tileIdx + DY];
    const float tag11 = gTileTags[tileIdx + DX + DY];
    if (tag00 > 0.0 || tag10 > 0.0 || tag01 > 0.0 || tag11 > 0.0) {
        AllocateCoarseTile(id);
        return;
    }
    // No active neigbors – clear the current tile
    gOutCoarserTiles[id] = float4(0.0, 0.0, 0.0, 0.0);
    gOutCoarserTileTags[id] = EMPTY_TILE_TAG;
}
