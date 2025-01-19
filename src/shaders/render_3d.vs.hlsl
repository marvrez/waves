struct VSOutput {
    float3 worldPos : POSITION0;
    float4 pos : SV_POSITION;
};

struct Parameters { float4x4 worldToClip; float3 cameraPosition; };
[[vk::push_constant]] Parameters gParams;

VSOutput main(float3 position: POSITION0) {
    VSOutput output = (VSOutput)0;
    output.worldPos = position;
    output.pos = mul(gParams.worldToClip, float4(position, 1.0));
    return output;
}