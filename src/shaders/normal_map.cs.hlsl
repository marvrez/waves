[[vk::binding(0, 0)]] Texture2D<float4> gDisplacementMap;
[[vk::binding(1, 0)]] RWTexture2D<float4> gOutNormalMap;

struct Params {
    int texSize;
    int oceanSize;
};
[[vk::push_constant]] Params gParams;

[numthreads(32, 32, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    const float texelSize = gParams.oceanSize / gParams.texSize;

    float3 center = gDisplacementMap.Load(int3(id.xy, 0)).xyz;
    float3 left = float3(-texelSize, 0.0f, 0.0f) + gDisplacementMap.Load(int3(clamp(id.x - 1, 0, gParams.texSize - 1), id.y, 0)).xyz - center;
    float3 right = float3(texelSize, 0.0f, 0.0f) + gDisplacementMap.Load(int3(clamp(id.x + 1, 0, gParams.texSize - 1), id.y, 0)).xyz - center;
    float3 top = float3(0.0f, 0.0f, -texelSize) + gDisplacementMap.Load(int3(id.x, clamp(id.y - 1, 0, gParams.texSize - 1), 0)).xyz - center;
    float3 bottom = float3(0.0f, 0.0f, texelSize) + gDisplacementMap.Load(int3(id.x, clamp(id.y + 1, 0, gParams.texSize - 1), 0)).xyz - center;

    float3 topRight = cross(right, top);
    float3 topLeft = cross(top, left);
    float3 bottomLeft = cross(left, bottom);
    float3 bottomRight = cross(bottom, right);

    float3 normal = normalize(topRight + topLeft + bottomRight + bottomLeft);
    gOutNormalMap[id.xy] = float4(normal, 1.0f);
}