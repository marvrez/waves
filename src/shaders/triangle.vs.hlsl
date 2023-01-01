struct VSOutput {
    float3 color : COLOR0;
    float4 pos : SV_POSITION;
};

static const float2 gPositions[] = {
	float2(-0.5, -0.5),
	float2( 0.0,  0.5),
	float2( 0.5, -0.5),
};

static const float3 gColors[] = {
	float3(1, 0, 0),
	float3(0, 1, 0),
	float3(0, 0, 1),
};

VSOutput main(uint vertexID : SV_VertexID)
{
    VSOutput output = (VSOutput)0;
    output.color = gColors[vertexID];
    output.pos = float4(gPositions[vertexID], 0.0, 1.0);
    return output;
}