#include "tools.h"

[[vk::binding(0, 0)]] StructuredBuffer<TilesCounter> gCounter;
[[vk::binding(1, 0)]] RWStructuredBuffer<IndirectDispatchArgs> gOutDispatchArgs;

[numthreads(1, 1, 1)]
void main()
{
    gOutDispatchArgs[0].flatTileListGroupCount = uint4(1, gCounter[0].numTiles, 1, 0);
    gOutDispatchArgs[0].gridGroupCount = uint4(2, 2 * gCounter[0].numTiles, 2, 0);
}