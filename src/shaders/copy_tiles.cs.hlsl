#include "tools.h"

[[vk::binding(0, 0)]] StructuredBuffer<uint> gTileAddresses;
[[vk::binding(1, 0)]] StructuredBuffer<uint> gTileData;
[[vk::binding(2, 0)]] Texture3D<float4> gTilesIndirectionTexture;
[[vk::binding(3, 0)]] Texture3D<float4> gTilesTexture;
[[vk::binding(4, 0)]] RWTexture3D<float4> gOutTexture;

#define LoadValue(idx) LoadTile(gTilesIndirectionTexture, gTilesTexture, idx)

[numthreads(8, 8, 8)]
void main(uint3 id : SV_DispatchThreadID, uint3 groupId : SV_GroupID)
{
    const int3 gridIdx = GetGridIndex(gTileAddresses[groupId.y / 2], id);
    const int3 outIdx = GetGridIndex(gTileData[groupId.y / 2], id);
    gOutTexture[outIdx] = LoadValue(gridIdx);
}