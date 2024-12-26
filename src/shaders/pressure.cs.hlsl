[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] SamplerState gPressureSampler;
[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] Texture2D<float4> gPressureTexture;
[[vk::combinedImageSampler]] [[vk::binding(1, 0)]] SamplerState gDivergenceSampler;
[[vk::combinedImageSampler]] [[vk::binding(1, 0)]] Texture2D<float4> gDivergenceTexture;
[[vk::binding(2, 0)]] RWTexture2D<float4> gOut;

struct Params {
    float2 size;
};
[[vk::push_constant]] Params gParams;

static inline float2 toUV(int2 value) { return (float2(value) + 0.5f) / gParams.size; }


[numthreads(32, 32, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    const int2 pos = int2(id.xy);
    const float2 uv = toUV(pos);

    // Sample neighboring pressure values
    const float presX0 = gPressureTexture.SampleLevel(gPressureSampler, toUV(pos - int2(1, 0)), 0).x;  // (left)
    const float presX1 = gPressureTexture.SampleLevel(gPressureSampler, toUV(pos + int2(1, 0)), 0).x;  // (right)
    const float presY0 = gPressureTexture.SampleLevel(gPressureSampler, toUV(pos - int2(0, 1)), 0).x;  // (up)
    const float presY1 = gPressureTexture.SampleLevel(gPressureSampler, toUV(pos + int2(0, 1)), 0).x;  // (down)

    const float divergence = gDivergenceTexture.SampleLevel(gDivergenceSampler, uv, 0).x;
    const float relaxed = (presX0 + presX1 + presY0 + presY1 - divergence) / 4.0f;
    gOut[pos] = float4(relaxed, 0.0f, 0.0f, 0.0f);
}
