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