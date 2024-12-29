#define COARSE_GRID_FREQUENCY 8.0
#define COARSE_GRID_SIZE 0.05
#define COARSE_GRID_BASE_COLOR 0.6
#define FINE_GRID_FREQUENCY 40.0
#define FINE_GRID_SIZE 0.1
#define FINE_GRID_BASE_COLOR 0.8
#define GRID_COLOR_SCALE 0.4

static inline float Grid(float2 uv, float gridSize)
{
    const float2 gridPos = frac((uv + 0.5) * gridSize);
    return step(gridSize, gridPos.x) * step(gridSize, gridPos.y);
}

float4 main(float3 normal : NORMAL0, float2 uv : TEXCOORD0) : SV_Target
{
    const float3 L = normalize(float3(0.2, -0.6, 0.5));
    const float NdotL = dot(normalize(normal), -L) * 0.5 + 0.8;

    // Draw low frequency grid
    float g = Grid(uv * COARSE_GRID_FREQUENCY, COARSE_GRID_SIZE) * GRID_COLOR_SCALE + COARSE_GRID_BASE_COLOR;
    // Draw high frequency grid on top
    g = min(g, Grid(uv * FINE_GRID_FREQUENCY, FINE_GRID_SIZE) * GRID_COLOR_SCALE + FINE_GRID_BASE_COLOR);
    // Apply lighting (lambertian + ambient)
    g = NdotL * (g * 0.3 - 0.05);

    return float4(g.xxx, 1.0);
}
