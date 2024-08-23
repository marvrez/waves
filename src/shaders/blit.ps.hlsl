[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] SamplerState gSampler;
[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] Texture2D gTexture;

float4 main(float2 uv : TEXCOORD0): SV_TARGET
{
    return gTexture.Sample(gSampler, uv);
}