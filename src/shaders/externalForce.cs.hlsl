[[vk::binding(0, 0)]] RWTexture2D<float4> gOut;

struct Parameters {
    float2 mousePosition;
    float2 mouseMove;
};
[[vk::push_constant]] Parameters gParams;

static const float MAX_MOUSE_DIST = 120.0f;
static const float MIN_MOUSE_MOVE = 20.0f;

[numthreads(32, 32, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    const float2 pos = float2(id.xy);
    const float dist = length(pos - gParams.mousePosition);
    if (dist < MAX_MOUSE_DIST && length(gParams.mouseMove) > MIN_MOUSE_MOVE) {
        const float2 force = gParams.mouseMove * 0.01f;
        gOut[id.xy] = float4(force, 0.0f, 1.0f);
    }
}
