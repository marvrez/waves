#pragma once

enum class DisplayMode : int {
    DENSITY = 0,
    VELOCITY = 1,
    TILE_TAG = 2,
    DIVERGENCE = 3,
    JACOBI = 4,
    RESIDUAL = 5,
    GRADIENT = 6,
};

struct FluidSimParams {
    float slice = 0.5f;
    int currentFrame = 0;
    int targetFrame = 1;
    DisplayMode displayMode = DisplayMode::DENSITY;
    int currentLevel = 0; // Current level of the tile hierarchy which we want to visualize.
    bool shouldShowGrid = false;
};

struct TilesCounter {
    uint32_t numTiles;
    uint32_t numActiveTiles;
    uint32_t numFreedTiles;
};
struct IndirectDispatchArgs {
    // (2, 2 * numActiveTiles, 2) thread groups
    // We multiply by 2 since all work groups are dispatched with 8x8x8 threads;
    // ideally, we want 16x16x16, but due to the 1024 workg group invocations
    // limit, we can't do that). As such, we need to dispatch twice as many work groups.
    glm::uvec3 gridGroupCount;         
    // (1, numActiveTiles, 1) thread groups.
    // Mainly meant for processing all tiles in a flat list.
    glm::uvec3 flatTileListGroupCount;
};
struct GenerateDensityTilesPushConstants {
    glm::vec3 densityCenter;
    float densityRadius;
};

struct GenerateVelocityTilesPushConstants {
    glm::vec3 densityCenter;
    float densityRadius;
    glm::vec4 velocityAdvectionFactor;
};

struct AdvectTilesPushConstants {
    glm::vec4 advectionFactor;
};