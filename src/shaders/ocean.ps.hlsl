struct Constants {
    float4x4 worldToClip;

    float3 cameraPosition;
    float displacementScaleFactor;

    float3 sunDirection;
    float tipScaleFactor;

    float exposure;
};

[[vk::push_constant]] Constants gConsts;

[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] SamplerState gDisplacementMapSampler;
[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] Texture2D gDisplacementMapTexture;
[[vk::combinedImageSampler]] [[vk::binding(1, 0)]] SamplerState gNormalMapSampler;
[[vk::combinedImageSampler]] [[vk::binding(1, 0)]] Texture2D gNormalMapTexture;

// HDR function
float3 HDR(float3 color, float exposure)
{
    return 1.0f - exp(-color * exposure);
}

// Sky and ocean color constants
static const float3 kSkyColor = float3(3.2f, 9.6f, 12.8f);
static const float3 kOceanColor = float3(0.004f, 0.016f, 0.047f);

float4 main(float3 worldPos : TEXCOORD0, float2 uv : TEXCOORD1) : SV_TARGET
{
    // Sample the normal from the normal map
    const float3 normal = gNormalMapTexture.Sample(gNormalMapSampler, uv).xyz;

    const float3 lightDir = -normalize(gConsts.sunDirection);
    const float3 viewDir = normalize(worldPos - gConsts.cameraPosition);
    const float3 halfwayDir = normalize(lightDir + viewDir);

    const float NdotL = saturate(dot(normal, lightDir)); // Lambertian/diffuse factor
    const float NdotV = saturate(dot(normal, viewDir)); // View factor
    const float NdotH = saturate(dot(normal, halfwayDir)); // Specular factor

    // Compute view direction
    float fresnel = 0.02f + 0.98f * pow(1.0f - NdotV, 5.0f);

    // Calculate sky and water color contributions
    float3 sky = fresnel * kSkyColor;
    float3 water = (1.0f - fresnel) * kOceanColor * kSkyColor * NdotL;

    // Ghetto foam approximation based on wave height
    const float3 tipColor = (1.0f - fresnel) * max(0.003 * kSkyColor * pow(-worldPos.y, gConsts.tipScaleFactor), float3(0, 0, 0));

    // Final color computation
    const float3 color = sky + water + tipColor;

    // Apply HDR and set the output color
    return float4(HDR(color, gConsts.exposure), 1.0f);
}