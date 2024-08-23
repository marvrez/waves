[[vk::binding(0, 0)]] RWTexture2D<float> gOutInitialSpectrum;

struct Params {
    float2 windDirection;
    int texSize;
    int oceanSize;
};
[[vk::push_constant]] Params gParams;

static const float PI = 3.14159265359;
static const float g = 9.81;
static const float KM = 370.0;
static const float CM = 0.23;

static inline float omega(float k)
{
    return sqrt(g * k * (1.0f + ((k * k) / (KM * KM))));
}

static inline float square(float x)
{
    return x * x;
}

[numthreads(32, 32, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    const float n = (id.x < 0.5f * gParams.texSize) ? id.x : id.x - gParams.texSize;
    const float m = (id.y < 0.5f * gParams.texSize) ? id.y : id.y - gParams.texSize;

    float2 waveVector = (2.0 * PI * float2(n, m)) / gParams.oceanSize;
    float k = length(waveVector);

    float U10 = length(gParams.windDirection);

    float Omega = 0.84f;
    float kp = g * square(Omega / U10);

    float c = omega(k) / k;
    float cp = omega(kp) / kp;

    float Lpm = exp(-1.25 * square(kp / k));
    float gamma = 1.7;
    float sigma = 0.08 * (1.0 + 4.0 * pow(Omega, -3.0));
    float Gamma = exp(-square(sqrt(k / kp) - 1.0) / (2.0 * square(sigma)));
    float Jp = pow(gamma, Gamma);
    float Fp = Lpm * Jp * exp(-Omega / sqrt(10.0) * (sqrt(k / kp) - 1.0));
    float alphap = 0.006 * sqrt(Omega);
    float Bl = 0.5 * alphap * cp / c * Fp;

    float z0 = 0.000037 * square(U10) / g * pow(U10 / cp, 0.9);
    float uStar = 0.41 * U10 / log(10.0 / z0);
    float alpham = 0.01 * ((uStar < CM) ? (1.0 + log(uStar / CM)) : (1.0 + 3.0 * log(uStar / CM)));
    float Fm = exp(-0.25 * square(k / KM - 1.0));
    float Bh = 0.5 * alpham * CM / c * Fm * Lpm;

    float a0 = log(2.0) / 4.0;
    float am = 0.13 * uStar / CM;
    float Delta = tanh(a0 + 4.0 * pow(c / cp, 2.5) + am * pow(CM / c, 2.5));

    float cosPhi = dot(normalize(gParams.windDirection), normalize(waveVector));

    float S = (1.0 / (2.0 * PI)) * pow(k, -4.0) * (Bl + Bh) * (1.0 + Delta * (2.0 * cosPhi * cosPhi - 1.0));

    float dk = 2.0 * PI / gParams.oceanSize;
    float h = sqrt(S / 2.0) * dk;

    if (waveVector.x == 0.0 && waveVector.y == 0.0) h = 0.0f;

    gOutInitialSpectrum[id.xy] = h;
}