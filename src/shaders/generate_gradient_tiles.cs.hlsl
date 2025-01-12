#include "tools.h"

[[vk::binding(0, 0)]] StructuredBuffer<uint> gTileAddresses;
[[vk::binding(1, 0)]] StructuredBuffer<uint> gTileData;
[[vk::binding(2, 0)]] Texture3D<float4> gUtexture;
[[vk::binding(3, 0)]] Texture3D<float4> gAdvectedVelocityTexture;
[[vk::binding(4, 0)]] Texture3D<float4> gTilesIndirectionTexture;
[[vk::binding(5, 0)]] RWTexture3D<float4> gOutGradientTexture;

struct Parameters { float scale; };
[[vk::push_constant]] Parameters gParams;

#define LoadU(idx) LoadTile(gTilesIndirectionTexture, gUtexture, idx)
#define LoadAdvectedVelocity(idx) LoadTile(gTilesIndirectionTexture, gAdvectedVelocityTexture, idx)
#define DX int3(1, 0, 0)
#define DY int3(0, 1, 0)
#define DZ int3(0, 0, 1)

[numthreads(8, 8, 8)]
void main(uint3 id : SV_DispatchThreadID, uint3 groupId : SV_GroupID)
{
    const int3 gridIdx = GetGridIndex(gTileAddresses[groupId.y / 2], id);
    const int3 outIdx = GetGridIndex(gTileData[groupId.y / 2], id);

    const float pL = LoadU(gridIdx - DX).x;
    const float pR = LoadU(gridIdx + DX).x;
    const float pB = LoadU(gridIdx - DY).x;
    const float pT = LoadU(gridIdx + DY).x;
    const float pBk = LoadU(gridIdx - DZ).x;
    const float pF = LoadU(gridIdx + DZ).x;

    const float3 gradient = gParams.scale * float3(pR - pL, pT - pB, pF - pBk);
    const float3 advectedVelocity = LoadAdvectedVelocity(gridIdx).xyz;
    gOutGradientTexture[outIdx] = float4(advectedVelocity - gradient, 0.0f);
}
