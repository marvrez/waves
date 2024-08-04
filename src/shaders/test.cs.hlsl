[[vk::binding(0, 0)]] StructuredBuffer<int> InputBuffer;
[[vk::binding(1, 0)]] RWStructuredBuffer<int> OutputBuffer;

[numthreads(1, 1, 1)]
void main(uint3 gIdx : SV_DispatchThreadID)
{
    OutputBuffer[gIdx.x] = InputBuffer[gIdx.x] + 1;
}