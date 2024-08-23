struct VSInput {
    float3 worldPos : POSITION0;
    float2 uv       : TEXCOORD0;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float2 uv       : TEXCOORD1;
};

struct Constants {
    float4x4 worldToClip;

    float3 cameraPosition;
    float displacementScaleFactor;

    float3 sunDirection;
};

[[vk::push_constant]] Constants gConsts;

// Displacement map
[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] SamplerState gDisplacementMapSampler;
[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] Texture2D gDisplacementMapTexture;
[[vk::combinedImageSampler]] [[vk::binding(1, 0)]] SamplerState gNormalMapSampler;
[[vk::combinedImageSampler]] [[vk::binding(1, 0)]] Texture2D gNormalMapTexture;

VSOutput main(VSInput input)
{
    VSOutput output;

    // Calculate the displaced position
    const float3 displacement = gDisplacementMapTexture.SampleLevel(gDisplacementMapSampler, input.uv, 0).rgb;
    const float3 displacedPosition = input.worldPos + gConsts.displacementScaleFactor * displacement;

    // Output the final position and transformed position
    output.position = mul(gConsts.worldToClip, float4(displacedPosition, 1.0f));
    output.worldPos = displacedPosition;
    output.uv = input.uv;

    return output;
}