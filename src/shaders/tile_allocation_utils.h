#include "tools.h"

[[vk::binding(0, 0)]] RWStructuredBuffer<uint> gOutTileAddress;
[[vk::binding(1, 0)]] RWTexture3D<float4> gOutTiles;
[[vk::binding(2, 0)]] StructuredBuffer<uint> gTileData;
[[vk::binding(3, 0)]] RWStructuredBuffer<TilesCounter> gOutCounter;
[[vk::binding(4, 0)]] RWTexture3D<float> gOutTileTags;

uint AllocateTile(int3 tileIndex, float tileTag)
{
    uint currentTileCount;
    InterlockedAdd(gOutCounter[0].numTiles, 1, currentTileCount);

    const uint tile = gTileData[currentTileCount];
    gOutTiles[tileIndex] = GetPackedTile(tile);
    gOutTileTags[tileIndex] = GetPackedTileTag(tileTag);
    gOutTileAddress[currentTileCount] = GetTileAddress(tileIndex);

    return tile;
}