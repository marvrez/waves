
#include "tools.h"

[[vk::binding(0, 0)]] StructuredBuffer<uint> gTiles;
[[vk::binding(1, 0)]] RWTexture3D<float4> gOut;

[numthreads(8, 8, 8)]
void main(uint3 id : SV_DispatchThreadID, uint3 groupID : SV_GroupID)
{
    const uint tile = gTiles[groupID.y / 2];
    const int3 outIdx = GetGridIndex(tile, id);
    gOut[outIdx] = float4(0.0, 0.0, 0.0, 0.0);
}