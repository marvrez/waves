[[vk::combinedImageSampler]] [[vk::binding(1, 0)]] SamplerState gSampler;
[[vk::combinedImageSampler]] [[vk::binding(1, 0)]] Texture2D gTexture;

float4 main(float4 color : COLOR0, float2 uv : TEXCOORD0) : SV_TARGET
{
    return color * gTexture.Sample(gSampler, uv);
}