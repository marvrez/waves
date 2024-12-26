[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] SamplerState gVelocitySampler;
[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] Texture2D<float4> gVelocityTexture;
[[vk::binding(1, 0)]] RWTexture2D<float4> gOut;

struct Parameters {
    float2 size;
};
[[vk::push_constant]] Parameters gParams;

static const float DELTA_TIME_MS = 20.0f;

static inline float2 toUV(float2 value) { return (value + 0.5f) / gParams.size; }

[numthreads(32, 32, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    const float2 uv = toUV(float2(id.xy));
    const float2 velocity = gVelocityTexture.SampleLevel(gVelocitySampler, uv, 0).xy;

    const float2 prevUV = toUV(float2(id.xy) - velocity * DELTA_TIME_MS);
    const float2 prevVelocity = gVelocityTexture.SampleLevel(gVelocitySampler, prevUV, 0).xy;

    gOut[id.xy] = float4(prevVelocity, 0.0f, 1.0f);
}