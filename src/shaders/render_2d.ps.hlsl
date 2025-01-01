#include "tools.h"

[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] SamplerState gTileTagsSampler;
[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] Texture3D<float> gTileTagsTexture;

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

static inline float4 GetTagColor(float tag)
{
    // If we're rendering the tile tags, we always render the whole grid
    if (gParams.displayMode == 2) return float4(1.0, 1.0, 1.0, 1.0);

    if (tag == COARSE_TILE_TAG) return float4(0.4, 0.2, 0.04, 1.0); // Brown-ish
    else if (tag != EMPTY_TILE_TAG) return float4(0.5, 0.5, 0.5, 5.0); // White/grey-ish
    return float4(0.0, 0.0, 0.0, 1.0);
}

static inline float4 GetTileColor(float tag)
{
    if (gParams.displayMode == 2) return float4(tag / 2, 0.0, 0.0, 1.0);

    if (tag.x == EMPTY_TILE_TAG) return float4(0.0, 0.0, 0.0, 1.0);
    return float4(0.0, 0.0, 0.0, 1.0);
}

float4 main(float2 uv : TEXCOORD0) : SV_Target
{
    const float3 uvw = float3(uv, gParams.slice) * gParams.tileLevelScaleFactor;
    const float tag = GetUnpackedTileTag(gTileTagsTexture.SampleLevel(gTileTagsSampler, uvw, 0));

    const float4 tagColor = GetTagColor(tag);
    const float4 tileColor = GetTileColor(tag);

    const float gridFactor = float(gParams.shouldRenderGrid) * IsGridBoundary(fmod(uvw.xy, 1.0 / 16.0));
    return lerp(tileColor, tagColor, gridFactor);
}
