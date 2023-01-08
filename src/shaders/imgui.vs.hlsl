struct VSInput {
    float2 position : POSITION0;
    float2 uv       : TEXCOORD0;
    float4 color    : COLOR0;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float4 color    : COLOR0;
    float2 uv       : TEXCOORD0;
};

struct ImGuiConstants {
    float2 scale;
    float2 translate;
};

[[vk::push_constant]] ImGuiConstants gConsts;

VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput)0;
    output.position = float4(input.position * gConsts.scale + gConsts.translate, 0.0, 1.0);
    output.uv = input.uv;
    output.color = input.color;
    return output;
}