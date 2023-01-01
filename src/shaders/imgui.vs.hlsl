#define UINT_TO_FLOAT4(val) float4((val >> 0) & 0xFF, (val >> 8) & 0xFF, (val >> 16) & 0xFF, (val >> 24) & 0xFF)

struct Vertex {
    float2 position;
    float2 uv;
    uint color;
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
[[vk::binding(0, 0)]] StructuredBuffer<Vertex> gVertices;

VSOutput main(uint vertexID : SV_VertexID)
{
    VSOutput output = (VSOutput)0;
    const Vertex vertex = gVertices[vertexID];
    const float2 position = float2(vertex.position.x, vertex.position.y);
    output.position = float4(position * gConsts.scale + gConsts.translate, 0.0, 1.0);
    output.uv = float2(vertex.uv.x, vertex.uv.y);
    output.color = UINT_TO_FLOAT4(vertex.color) * (1.0 / 255.0);
    return output;
}