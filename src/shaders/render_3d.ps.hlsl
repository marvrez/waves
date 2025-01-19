#include "tools.h"

[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] Texture3D<float4> gDensityTexture;
[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] SamplerState gDensitySampler;
[[vk::combinedImageSampler]] [[vk::binding(1, 0)]] Texture3D<float4> gTilesIndirectionTexture;
[[vk::combinedImageSampler]] [[vk::binding(1, 0)]] SamplerState gTilesIndirectionSampler;

struct Parameters { float4x4 worldToClip; float3 cameraPosition; };
[[vk::push_constant]] Parameters gParams;

static const int kNumSteps = 100;
static const float kJitterScale = 0.25;
static const float3 kFireColor = float3(1.0, 0.5, 0.1);
static const float kSmokeCutoff = 0.2; // Higher cutoff ==> more smoke
static const float kTransmittanceScale = 3;
static const float kDensityCutoff = 5;

static inline float4 SampleDensity(float3 uvw)
{
    const float4 tile = gTilesIndirectionTexture.Sample(gTilesIndirectionSampler, uvw);
    const float3 tileWorldPos = (tile.xyz * 255.0) / 16.0; // In range [0, 15/16]
    const float3 localCoord = fmod(uvw, 1.0 / 16.0); // In range [0, 1/16]
    return gDensityTexture.Sample(gDensitySampler, tileWorldPos + localCoord);
}

static inline float2 RayMarchDensity(float3 rayOrigin, float3 rayDirection)
{
    const float2 boxIntersection = GetUnitBoxIntersection(rayOrigin, rayDirection);
    if (boxIntersection.x > boxIntersection.y) return float2(0.0, 0.0); // No intersection

    float t = 0.0;
    const float jitterScale = GetRandom(float4(rayOrigin.xy + float2(kNumSteps, kNumSteps), rayOrigin.yx * float(kNumSteps))) * kJitterScale;
    const float stepSize = (boxIntersection.y - boxIntersection.x) / float(kNumSteps);
    for (int i = 0; i < kNumSteps; i++) {
        const float3 samplePos = rayOrigin + rayDirection * (boxIntersection.x + stepSize * (float(i) + 0.5 + jitterScale));
        const float density = SampleDensity(samplePos).x;
        t += density;
        if (t > kDensityCutoff) return float2(1.0, density);
    }
    return float2(t, 0.0);
}

float4 main(float3 position: POSITION0) : SV_TARGET
{
    const float3 rayOrigin = position.xyz;
    const float3 rayDirection = normalize(rayOrigin - gParams.cameraPosition);

    const float2 transmittanceEnergy = RayMarchDensity(rayOrigin, rayDirection);
    return float4(max(transmittanceEnergy.y - kSmokeCutoff, 0.0) * kFireColor * kTransmittanceScale, transmittanceEnergy.x);
}