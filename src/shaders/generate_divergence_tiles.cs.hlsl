#include "tools.h"

[[vk::binding(0, 0)]] StructuredBuffer<uint> gTileAddresses;
[[vk::binding(1, 0)]] StructuredBuffer<uint> gTiles;
[[vk::binding(2, 0)]] Texture3D<float4> gTilesIndirectionTexture;
[[vk::binding(3, 0)]] Texture3D<float4> gAdvectedVelocityTexture;
[[vk::binding(4, 0)]] RWTexture3D<float4> gOutDivergenceTexture;

#define LoadAdvectedVelocity(idx) LoadTile(gTilesIndirectionTexture, gAdvectedVelocityTexture, idx)
#define DX int3(1, 0, 0)
#define DY int3(0, 1, 0)
#define DZ int3(0, 0, 1)

[numthreads(8, 8, 8)]
void main(uint3 id : SV_DispatchThreadID, uint3 groupId : SV_GroupID)
{
    const int3 gridIdx = GetGridIndex(gTileAddresses[groupId.y / 2], id);
    const int3 outIdx = GetGridIndex(gTiles[groupId.y / 2], id);
    
    const float wL  = LoadAdvectedVelocity(gridIdx - DX).x;
    const float wR  = LoadAdvectedVelocity(gridIdx + DX).x;
    const float wB  = LoadAdvectedVelocity(gridIdx - DY).y;
    const float wT  = LoadAdvectedVelocity(gridIdx + DY).y;
    const float wBk = LoadAdvectedVelocity(gridIdx - DZ).z;
    const float wF  = LoadAdvectedVelocity(gridIdx + DZ).z;

    const float divergence = 0.5 * (wR - wL + wT - wB + wF - wBk); 
    gOutDivergenceTexture[outIdx] = float4(divergence, 0, 0, 1);
}