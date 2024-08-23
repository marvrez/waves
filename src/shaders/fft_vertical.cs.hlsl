[[vk::binding(0, 0)]] Texture2D<float4> gInput;
[[vk::binding(1, 0)]] RWTexture2D<float4> gOutput;

// Uniform variables
struct Params {
    int totalCount;
    int subseqCount;
};
[[vk::push_constant]] Params gParams;

#define PI 3.14159265358979323846

static inline float2 MultiplyComplex(float2 a, float2 b)
{
    return float2(a.x * b.x - a.y * b.y, a.y * b.x + a.x * b.y);
}

static inline float4 ButterflyOperation(float2 a, float2 b, float2 twiddle)
{
    const float2 twiddleB = MultiplyComplex(twiddle, b);
    return float4(a + twiddleB, a - twiddleB);
}

[numthreads(256, 1, 1)]
void main(uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID)
{
    const uint2 pixelCoord = uint2(groupId.x, groupThreadId.x);

    const int threadCount = int(gParams.totalCount * 0.5f);
    const int threadIdx = int(pixelCoord.y);

    const int inIdx = threadIdx & (gParams.subseqCount - 1);        
    const int outIdx = ((threadIdx - inIdx) << 1) + inIdx;

    const float angle = -PI * (float(inIdx) / float(gParams.subseqCount));
    const float2 twiddle = float2(cos(angle), sin(angle));

    const float4 a = gInput.Load(int3(pixelCoord, 0));
    const float4 b = gInput.Load(int3(pixelCoord.x, pixelCoord.y + threadCount, 0));

    // Transforming two complex sequences independently and simultaneously
    const float4 result0 = ButterflyOperation(a.xy, b.xy, twiddle);
    const float4 result1 = ButterflyOperation(a.zw, b.zw, twiddle);

    gOutput[uint2(pixelCoord.x, outIdx)] = float4(result0.xy, result1.xy);
    gOutput[uint2(pixelCoord.x, outIdx + gParams.subseqCount)] = float4(result0.zw, result1.zw);
}