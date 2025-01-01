#include "tile_allocation_utils.h"

struct Parameters { int3 tileOffset; };
[[vk::push_constant]] Parameters gParams;

[numthreads(1, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    const int3 tileIndex = gParams.tileOffset + int3(id);
    const float tileTag = gOutTileTags[tileIndex];
    if (tileTag == EMPTY_TILE_TAG) AllocateTile(tileIndex, ACTIVE_TILE_TAG);
}