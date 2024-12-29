struct VSInput {
    float3 position : POSITION0;
    float3 normal   : NORMAL0;
    float2 uv       : TEXCOORD0;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float3 normal   : NORMAL0;
    float2 uv       : TEXCOORD0;
};

struct Parameters { float4x4 worldToClip; };
[[vk::push_constant]] Parameters gParams;

VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput)0;
    output.uv = input.uv;
    output.normal = input.normal;
    output.position = mul(gParams.worldToClip, float4(input.position, 1.0));
    return output;
}