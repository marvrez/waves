#include "tools.h"

[[vk::binding(0, 0)]] StructuredBuffer<uint> gTileAddresses;
[[vk::binding(1, 0)]] StructuredBuffer<uint> gTiles;
[[vk::binding(2, 0)]] Texture3D<float4> gTilesIndirectionTexture;
[[vk::binding(3, 0)]] Texture3D<float4> gRHStexture;
[[vk::binding(4, 0)]] Texture3D<float4> gUtexture;
[[vk::binding(5, 0)]] RWTexture3D<float4> gOutTexture;

struct Parameters { float alpha; float omega; };
[[vk::push_constant]] Parameters gParams;

#define LoadU(idx) LoadTile(gTilesIndirectionTexture, gUtexture, idx)
#define LoadRHS(idx) LoadTile(gTilesIndirectionTexture, gRHStexture, idx)
#define DX int3(1, 0, 0)
#define DY int3(0, 1, 0)
#define DZ int3(0, 0, 1)

[numthreads(8, 8, 8)]
void main(uint3 id : SV_DispatchThreadID, uint3 groupId : SV_GroupID)
{
    const int3 outIdx = GetGridIndex(gTiles[groupId.y / 2], id);
    const int3 gridIdx = GetGridIndex(gTileAddresses[groupId.y / 2], id);
    const float4 u   = LoadU(gridIdx);
    const float4 rhs = LoadRHS(gridIdx);
    gOutTexture[outIdx] = u + gParams.omega * (1.0f / 6.0f) * (
        -gParams.alpha * rhs +
        LoadU(gridIdx - DX) + LoadU(gridIdx + DX) +
        LoadU(gridIdx - DY) + LoadU(gridIdx + DY) +
        LoadU(gridIdx - DZ) + LoadU(gridIdx + DZ) -
        6.0f * u
    );
}
