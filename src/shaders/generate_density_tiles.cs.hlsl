#include "tools.h"

[[vk::binding(0, 0)]] StructuredBuffer<uint> gTileAddress;
[[vk::binding(1, 0)]] StructuredBuffer<uint> gTileData;
[[vk::binding(2, 0)]] RWTexture3D<float4> gOutDensity;

struct Parameters { float3 densityCenter; float densityRadius; };
[[vk::push_constant]] Parameters gParams;

[numthreads(8, 8, 8)]
void main(uint3 id : SV_DispatchThreadID, uint3 groupId : SV_GroupID)
{
    const uint tile = gTileData[groupId.y / 2];
    const uint tileAddress = gTileAddress[groupId.y / 2];
    const TilePositionData data = GetTilePositionData(tile, tileAddress, id);

    const float distance = length(gParams.densityCenter - data.cellPosition);
    const float boundsCheckResult = step(0.0f, gParams.densityRadius - distance);
    const float density = max(gOutDensity[data.index], boundsCheckResult.xxxx);
    gOutDensity[data.index] = density;
}