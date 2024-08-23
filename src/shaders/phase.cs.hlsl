[[vk::binding(0, 0)]] Texture2D<float> gPhase;
[[vk::binding(1, 0)]] RWTexture2D<float> gOutDeltaPhase;

struct Params {
    float dt;
    int texSize;
    int oceanSize;
};
[[vk::push_constant]] Params gParams;

static const float PI = 3.14159265359;
static const float g = 9.81;
static const float KM = 370.0;

static inline float Omega(float k)
{
    return sqrt(g * k * (1.0f + k * k / (KM * KM)));
}

[numthreads(32, 32, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    const float dt = 1.0 / 60.0;
    float n = (id.x < 0.5f * gParams.texSize) ? id.x : id.x - gParams.texSize;
	float m = (id.y < 0.5f * gParams.texSize) ? id.y : id.y - gParams.texSize;

    float2 waveVector = (2.0f * PI * float2(n, m)) / gParams.oceanSize;
    float deltaPhase = Omega(length(waveVector)) * dt;
    float phase = gPhase.Load(int3(id.xy, 0));

    gOutDeltaPhase[id.xy] = fmod(phase + deltaPhase, 2.0f * PI);
}