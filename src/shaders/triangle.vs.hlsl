struct VSConstants {
    float4x4 worldToClip;
};

struct VSInput {
    float3 pos : POSITION0;
    float3 color : COLOR0;
};

struct VSOutput {
    float3 color : COLOR0;
    float4 pos : SV_POSITION;
};

[[vk::push_constant]] VSConstants gConsts;

VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput)0;
    output.color = input.color;
    output.pos = mul(gConsts.worldToClip, float4(input.pos.xyz, 1.0));
    return output;
}