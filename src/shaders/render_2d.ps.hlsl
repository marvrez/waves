#include "tools.h"

[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] SamplerState gTileTagsSampler;
[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] Texture3D<float> gTileTagsTexture;

[[vk::combinedImageSampler]] [[vk::binding(1, 0)]] SamplerState gVolumeSampler;
[[vk::combinedImageSampler]] [[vk::binding(1, 0)]] Texture3D<float4> gVolumeTexture;

[[vk::combinedImageSampler]] [[vk::binding(2, 0)]] SamplerState gTilesIndirectionSampler;
[[vk::combinedImageSampler]] [[vk::binding(2, 0)]] Texture3D<float4> gTilesIndirectionTexture;

struct Parameters {
    bool shouldRenderGrid;
    int displayMode;
    float tileLevelScaleFactor;
    float slice;
};
[[vk::push_constant]] Parameters gParams;

static inline float IsGridBoundary(float2 st)
{
    return max(
        max(step(st.x, 1.0 / 255.0), step(st.y, 1.0 / 255.0)), // Beginning of a grid
        max(1.0 - step(st.x, 15.0 / 255.0), 1.0 - step(st.y, 15.0 / 255.0)) // End of a grid
    );
}

static inline float4 SampleVolume(float3 uvw, float scale)
{
    const float4 tile = gTilesIndirectionTexture.SampleLevel(gTilesIndirectionSampler, uvw, 0);
    const float3 tileWorldPos = (tile.xyz * 255.0) / 16.0; // In range [0, 15/16]
    const float3 localCoord = fmod(uvw / scale, 1.0 / 16.0); // In range [0, 1/16]
    return gVolumeTexture.SampleLevel(gVolumeSampler, tileWorldPos + localCoord, 0);
}

static inline float4 GetTagColor(float tag)
{
    // If we're rendering the tile tags, we always render the whole grid
    if (gParams.displayMode == 2) return float4(1.0, 1.0, 1.0, 1.0);

    if (tag == COARSE_TILE_TAG) return float4(0.4, 0.2, 0.04, 1.0); // Brown-ish
    else if (tag != EMPTY_TILE_TAG) return float4(0.5, 0.5, 0.5, 5.0); // White/grey-ish
    return float4(0.0, 0.0, 0.0, 1.0);
}

static inline float4 GetTileColor(float3 uvw, float tag)
{
    // Tile tag
    if (gParams.displayMode == 2) return float4(tag / 2, 0.0, 0.0, 1.0);

    if (tag.x == EMPTY_TILE_TAG) return float4(0.0, 0.0, 0.0, 1.0);

    // Density
    if (gParams.displayMode == 0) return float4(SampleVolume(uvw, 1.0).xyz, 1.0);
    // Velocity
    if (gParams.displayMode == 1) {
        const float4 velocity = SampleVolume(uvw, 1.0);
        const float4 color = velocity * 0.5 + 0.5;
        return float4(color.xy, 0.0, 1.0); // Velocity is only advected in the XY plane
    }
    // Divergence, jacobi or residual
    if (gParams.displayMode == 3 || gParams.displayMode == 4 || gParams.displayMode == 5) {
        const float value = SampleVolume(uvw, 1.0).x;
        const float3 color = Heatmap(value);
        return float4(color, 1.0);
    }
    // Gradient
    if (gParams.displayMode == 6) {
        const float2 gradient = SampleVolume(uvw, 1.0).xy + 0.5;
        return float4(gradient, 0.0, 1.0);
    }

    return float4(0.0, 0.0, 0.0, 1.0);
}

float4 main(float2 uv : TEXCOORD0) : SV_Target
{
    const float3 uvw = float3(uv, gParams.slice) * gParams.tileLevelScaleFactor;
    const float tag = GetUnpackedTileTag(gTileTagsTexture.SampleLevel(gTileTagsSampler, uvw, 0));

    const float4 tagColor = GetTagColor(tag);
    const float4 tileColor = GetTileColor(uvw, tag);

    const float gridFactor = float(gParams.shouldRenderGrid) * IsGridBoundary(fmod(uvw.xy, 1.0 / 16.0));
    return lerp(tileColor, tagColor, gridFactor);
}
