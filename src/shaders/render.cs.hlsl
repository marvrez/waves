[[vk::binding(0, 0)]] RWTexture2D<float4> gVelocity;
[[vk::binding(1, 0)]] RWTexture2D<float4> gPressure;
[[vk::binding(2, 0)]] RWTexture2D<float4> gOutVelocity;
[[vk::binding(3, 0)]] RWTexture2D<float4> gOut;

[numthreads(32, 32, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    const int2 pos = int2(id.xy);

    const float presX0 = gPressure[pos - int2(1, 0)].x;
    const float presX1 = gPressure[pos + int2(1, 0)].x;
    const float presY0 = gPressure[pos - int2(0, 1)].y;
    const float presY1 = gPressure[pos + int2(0, 1)].y;
    const float2 gradient = float2(presX1 - presX0, presY1 - presY0) / 2;

    const float2 velocity = (gVelocity[pos].xy - gradient) * 0.995f;
    gOutVelocity[pos] = float4(velocity, 0.0f, 1.0f);

    const float3 color = float3(abs(velocity.x) * 3.0f, 0.0f, abs(velocity.y) * 3.0f);
    gOut[pos] = float4(color, 1.0f);
}
