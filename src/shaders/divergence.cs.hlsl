[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] SamplerState gVelocitySampler;
[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] Texture2D<float4> gVelocityTexture;
[[vk::binding(1, 0)]] RWTexture2D<float4> gOut;

struct Parameters {
    float2 size;
};
[[vk::push_constant]] Parameters gParams;

static inline float2 toUV(float2 value) { return (value + 0.5f) / gParams.size; }

[numthreads(32, 32, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    const float velX0 = gVelocityTexture.SampleLevel(gVelocitySampler, toUV(float2(id.xy) - float2(1, 0)), 0).x;
    const float velX1 = gVelocityTexture.SampleLevel(gVelocitySampler, toUV(float2(id.xy) + float2(1, 0)), 0).x;
    const float velY0 = gVelocityTexture.SampleLevel(gVelocitySampler, toUV(float2(id.xy) - float2(0, 1)), 0).y;
    const float velY1 = gVelocityTexture.SampleLevel(gVelocitySampler, toUV(float2(id.xy) + float2(0, 1)), 0).y;

    const float dx = velX1 - velX0;
    const float dy = velY1 - velY0;
    const float divergence = (dx + dy) * 0.5f;
    gOut[id.xy] = float4(divergence, 0.0f, 0.0f, 0.0f);
}