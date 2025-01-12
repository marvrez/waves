#include "tools.h"

[[vk::binding(0, 0)]] StructuredBuffer<uint> gCoarseTileAddresses;
[[vk::binding(1, 0)]] StructuredBuffer<uint> gCoarseTiles;
[[vk::binding(2, 0)]] Texture3D<float4> gTilesIndirectionTexture;
[[vk::binding(3, 0)]] Texture3D<float4> gInputTexture;
[[vk::binding(4, 0)]] RWTexture3D<float4> gOutTexture;

#define LoadResidual(idx) LoadTile(gTilesIndirectionTexture, gInputTexture, idx)
#define DX int3(1, 0, 0)
#define DY int3(0, 1, 0)
#define DZ int3(0, 0, 1)

[numthreads(8, 8, 8)]
void main(uint3 id : SV_DispatchThreadID, uint3 groupId : SV_GroupID)
{
    const int3 gridIdx = 2 * GetGridIndex(gCoarseTileAddresses[groupId.y / 2], id);
    const int3 outIdx = GetGridIndex(gCoarseTiles[groupId.y / 2], id);
    gOutTexture[outIdx] = 0.25f * LoadResidual(gridIdx) + // Center
        0.125f * ( // 6 Face neighbors
            LoadResidual(gridIdx - DX) +
            LoadResidual(gridIdx + DX) +
            LoadResidual(gridIdx - DY) +
            LoadResidual(gridIdx + DY) +
            LoadResidual(gridIdx - DZ) +
            LoadResidual(gridIdx + DZ)
        ) +
        0.0625f * ( // 8 Corner neighbors
            LoadResidual(gridIdx - DX - DY - DZ) +
            LoadResidual(gridIdx + DX - DY - DZ) +
            LoadResidual(gridIdx - DX + DY - DZ) +
            LoadResidual(gridIdx + DX + DY - DZ) +
            LoadResidual(gridIdx - DX - DY + DZ) +
            LoadResidual(gridIdx + DX - DY + DZ) +
            LoadResidual(gridIdx - DX + DY + DZ) +
            LoadResidual(gridIdx + DX + DY + DZ)
        );
}
