struct VSOutput {
    float2 uv : TEXCOORD0;
    float4 pos : SV_POSITION;
};

VSOutput main(uint vertexID : SV_VertexID)
{
    VSOutput output = (VSOutput)0;
    const uint u = vertexID & 1;
	const uint v = (vertexID >> 1) & 1;
    output.pos = float4(float(u) * 2 - 1, 1 - float(v) * 2, 0, 1);
    output.uv = float2(u, v);
    return output;
}