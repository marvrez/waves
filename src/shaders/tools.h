#pragma once

struct TilesCounter {
    uint numTiles;
    uint numActiveTiles;
    uint numFreedTiles;
};

#define EMPTY_TILE_TAG  0.0
#define COARSE_TILE_TAG 1.0
#define ACTIVE_TILE_TAG 2.0

static inline int3 GetTileIndex(uint tileAddress)
{
    return int3(tileAddress & 1023, (tileAddress >> 10) & 1023, (tileAddress >> 20) & 1023);
}

static inline uint GetTileAddress(int3 tileIndex)
{
    return tileIndex.x + (tileIndex.y << 10) + (tileIndex.z << 20);
}

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