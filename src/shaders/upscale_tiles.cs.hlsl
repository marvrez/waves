#include "tools.h"

[[vk::binding(0, 0)]] StructuredBuffer<uint> gTileAddresses;
[[vk::binding(1, 0)]] StructuredBuffer<uint> gTileData;
[[vk::binding(2, 0)]] Texture3D<float4> gTilesIndirectionTexture;
[[vk::binding(3, 0)]] Texture3D<float4> gUtexture;
[[vk::binding(4, 0)]] RWTexture3D<float4> gOutTexture;

#define LoadU(idx) LoadTile(gTilesIndirectionTexture, gUtexture, idx)
#define DX int3(1, 0, 0)
#define DY int3(0, 1, 0)
#define DZ int3(0, 0, 1)

[numthreads(8, 8, 8)]
void main(uint3 id : SV_DispatchThreadID, uint3 groupId : SV_GroupID)
{
    const int3 outIdx = GetGridIndex(gTileData[groupId.y / 2], id);
    const int3 gridIdx = GetGridIndex(gTileAddresses[groupId.y / 2], id);
    
    const int3 sampleGridIdx = gridIdx / 2;
    const float4 t000 = LoadU(sampleGridIdx);
    const float4 t100 = LoadU(sampleGridIdx + DX);
    const float4 t010 = LoadU(sampleGridIdx + DY);
    const float4 t110 = LoadU(sampleGridIdx + DX + DY);
    const float4 t001 = LoadU(sampleGridIdx + DZ);
    const float4 t101 = LoadU(sampleGridIdx + DX + DZ);
    const float4 t011 = LoadU(sampleGridIdx + DY + DZ);
    const float4 t111 = LoadU(sampleGridIdx + DX + DY + DZ);

    const float3 lerpFactor = 0.5f * float3(gridIdx.x & 1, gridIdx.y & 1, gridIdx.z & 1);
    float4 v = lerp(lerp(lerp(t000, t100, lerpFactor.x),
                         lerp(t010, t110, lerpFactor.x),
                         lerpFactor.y),
                    lerp(lerp(t001, t101, lerpFactor.x),
                         lerp(t011, t111, lerpFactor.x),
                         lerpFactor.y),
                    lerpFactor.z);
    const float value = gOutTexture[outIdx].x + v.x;

    gOutTexture[outIdx] = float4(value, 0.0f, 0.0f, 0.0f);
}