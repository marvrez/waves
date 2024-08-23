[[vk::binding(0, 0)]] Texture2D<float> gPhase;
[[vk::binding(1, 0)]] Texture2D<float> gInitialSpectrum;
[[vk::binding(2, 0)]] RWTexture2D<float4> gOutSpectrum;

struct Params {
    int texSize;
    int oceanSize;
    float choppiness;
};
[[vk::push_constant]] Params gParams;

static const float PI = 3.14159265359f;
static const float g = 9.81f;
static const float KM = 370.0f;

static inline float2 multiplyComplex(float2 a, float2 b)
{
    return float2(a.x * b.x - a.y * b.y, a.y * b.x + a.x * b.y);
}

static inline float2 multiplyByI(float2 z)
{
    return float2(-z.y, z.x);
}

static inline float omega(float k)
{
    return sqrt(g * k * (1.0f + k * k / (KM * KM)));
}

[numthreads(32, 32, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    float n = (id.x < 0.5f * gParams.texSize) ? id.x : id.x - gParams.texSize;
    float m = (id.y < 0.5f * gParams.texSize) ? id.y : id.y - gParams.texSize;
    float2 waveVector = (2.0f * PI * float2(n, m)) / gParams.oceanSize;

    float phase = gPhase.Load(int3(id.xy, 0));
    float2 phaseVector = float2(cos(phase), sin(phase));

    float2 h0 = float2(gInitialSpectrum.Load(int3(id.xy, 0)), 0.0f);
    int2 h0StarIdx = int2(gParams.texSize.xx - id.xy) % int2(gParams.texSize.xx - 1);
    float2 h0Star = float2(gInitialSpectrum.Load(int3(h0StarIdx, 0)), 0.0f);
    h0Star.y *= -1.0f;

    float2 h = multiplyComplex(h0, phaseVector) + multiplyComplex(h0Star, float2(phaseVector.x, -phaseVector.y));

    float2 hX = -multiplyByI(h * (waveVector.x / length(waveVector))) * gParams.choppiness;
    float2 hZ = -multiplyByI(h * (waveVector.y / length(waveVector))) * gParams.choppiness;

    // No DC term
    if (waveVector.x == 0.0f && waveVector.y == 0.0f) {
        h = float2(0.0f, 0.0f);
        hX = float2(0.0f, 0.0f);
        hZ = float2(0.0f, 0.0f);
    }

    gOutSpectrum[id.xy] = float4(hX + multiplyByI(h), hZ);
}