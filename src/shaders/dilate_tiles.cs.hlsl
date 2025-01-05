#include "tile_allocation_utils.h"

[[vk::binding(5, 0)]] RWTexture3D<float4> gOutAdvectedTilesTexture;

static inline void ClearAllocatedTile(uint tile)
{
    for (int z = 0; z < 16; z++) {
        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 16; x++) {
                const int3 index = GetGridIndex(tile, int3(x, y, z));
                gOutAdvectedTilesTexture[index] = float4(0, 0, 0, 0);
            }
        }
    }
}

[numthreads(8, 8, 8)]
void main(uint3 id : SV_DispatchThreadID)
{
    const float currentTag = GetUnpackedTileTag(gOutTileTags[id]);
    if (currentTag != EMPTY_TILE_TAG) return; // Skip if the tile is not empty

    for (int z = -1; z <= 1; z++) {
        for (int y = -1; y <= 1; y++) {
            for (int x = -1; x <= 1; x++) {
                const float tag = GetUnpackedTileTag(gOutTileTags[id + int3(x, y, z)]);
                if (tag == ACTIVE_TILE_TAG) {
                    const uint tile = AllocateTile(id, COARSE_TILE_TAG);
                    ClearAllocatedTile(tile);
                    return;
                }
            }
        }
    }
    gOutTiles[id] = float4(0.0, 0.0, 0.0, 0.0); // Clear tile if no tile is allocated
}
