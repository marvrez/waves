#include "tools.h"

[[vk::binding(0, 0)]] RWStructuredBuffer<TilesCounter> gOutCounter;
[[vk::binding(1, 0)]] RWStructuredBuffer<uint> gOutTileData;

struct Parameters { uint numTiles; };
[[vk::push_constant]] Parameters gParams;


[numthreads(1, 1, 1)]
void main()
{
    gOutCounter[0] = (TilesCounter)0;
    for (uint i = 0; i < gParams.numTiles - 1; ++i) {
        gOutTileData[i] = i + 1;
    }
}
