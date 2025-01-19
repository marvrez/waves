float4 main(float3 uvw: TEXCOORD0, float4 pos: SV_POSITION) : SV_TARGET
{
    return float4(normalize(pos.xyz), 1.0);
}