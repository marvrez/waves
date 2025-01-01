[[vk::binding(0, 0)]] RWTexture3D<float4> gOut;

[numthreads(8, 8, 8)]
void main(uint3 id : SV_DispatchThreadID)
{
    gOut[id] = float4(0.0, 0.0, 0.0, 0.0);
}
