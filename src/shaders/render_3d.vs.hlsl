struct VSOutput {
    float3 uvw : TEXCOORD0;
    float4 pos : SV_POSITION;
};

struct Parameters { float4x4 worldToClip; };
[[vk::push_constant]] Parameters gParams;

VSOutput main(float3 position: POSITION0) {
    VSOutput output;
    output.uvw = position * 0.5 + 0.5; 
    output.pos = mul(gParams.worldToClip, float4(position, 1.0));
    return output;
}