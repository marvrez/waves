#include "tools.h"

[[vk::binding(0, 0)]] StructuredBuffer<uint> gTileAddresses;
[[vk::binding(1, 0)]] StructuredBuffer<uint> gTiles;
[[vk::binding(2, 0)]] Texture3D<float4> gTilesIndirectionTexture;
[[vk::binding(3, 0)]] Texture3D<float4> gAdvectionQuantityTexture; // Quantity to advect, e.g. density
[[vk::binding(4, 0)]] Texture3D<float4> gVelocityTexture;
[[vk::binding(5, 0)]] RWTexture3D<float4> gOutAdvectedTilesTexture;

struct Parameters { float4 advectionFactor; };
[[vk::push_constant]] Parameters gParams;

#define LoadAdvectionQuantity(idx) LoadTile(gTilesIndirectionTexture, gAdvectionQuantityTexture, idx)
#define LoadVelocity(idx) LoadTile(gTilesIndirectionTexture, gVelocityTexture, idx)

#define VELOCITY_SCALE_FACTOR 0.1

static inline float4 SampleAdvectionQuantity(float3 coord)
{
    const int3 icoord = int3(floor(coord));
    const float4 t000 = LoadAdvectionQuantity(icoord);
    const float4 t010 = LoadAdvectionQuantity(icoord + int3(0, 1, 0));
    const float4 t100 = LoadAdvectionQuantity(icoord + int3(1, 0, 0));
    const float4 t110 = LoadAdvectionQuantity(icoord + int3(1, 1, 0));

    const float4 t001 = LoadAdvectionQuantity(icoord + int3(0, 0, 1));
    const float4 t011 = LoadAdvectionQuantity(icoord + int3(0, 1, 1));
    const float4 t101 = LoadAdvectionQuantity(icoord + int3(1, 0, 1));
    const float4 t111 = LoadAdvectionQuantity(icoord + int3(1, 1, 1));

    const float3 t = frac(coord);

    return lerp(lerp(lerp(t000, t100, t.x), lerp(t010, t110, t.x), t.y),
                lerp(lerp(t001, t101, t.x), lerp(t011, t111, t.x), t.y), t.z);
}

[numthreads(8, 8, 8)]
void main(uint3 id : SV_DispatchThreadID, uint3 groupId : SV_GroupID)
{
    const uint tile = gTiles[groupId.y / 2];
    const uint tileAddress = gTileAddresses[groupId.y / 2];

    const int3 localOffset = int3(id.x & 0xF, id.y & 0xF, id.z & 0xF);
    const int3 outIdx = GetGridIndex(tile, localOffset);
    const int3 gridIndex = GetGridIndex(tileAddress, localOffset);

    const float4 velocity = LoadVelocity(gridIndex) * VELOCITY_SCALE_FACTOR;
    const float3 advectedUV = float3(gridIndex) - velocity.xyz;

    gOutAdvectedTilesTexture[outIdx] = gParams.advectionFactor * SampleAdvectionQuantity(advectedUV);
}