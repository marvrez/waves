struct VSOutput {
    float2 uv : TEXCOORD0;
    float4 pos : SV_POSITION;
};

VSOutput main(uint vertexID : SV_VertexID)
{
    VSOutput output = (VSOutput)0;
    output.uv = float2(vertexID & 1, (vertexID >> 1) & 1);
    output.pos = float4(output.uv * 2 - 1, 0, 1);
    return output;
}