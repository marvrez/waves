#include "fluid/structs.h"

class Device;
class Buffer;
class Pipeline;
class CommandList;
class Texture;
class GUI;
struct Camera;

constexpr unsigned numBitsNeeded(unsigned n) { return n <= 1 ? 0 : 1 + numBitsNeeded((n + 1) / 2); }
constexpr int kTileSize = 16;
constexpr int kMaxNumLevels = numBitsNeeded(16); // log2(kTileSize) level of tiles
constexpr int kTextureSize = 256;
constexpr int kThreadGroupSize = 8;
constexpr int kTotalNumTiles = (kTextureSize / kTileSize) * (kTextureSize / kTileSize) * (kTextureSize / kTileSize);
struct JacobiParams {
    Handle<Texture> u;                    // The current solution texture (input/output for Jacobi iterations)
    Handle<Texture> rhs;                  // The right-hand side texture (constant source term for the problem)
    Handle<Texture> tilesIndirections;    // The tiles texture (used to map spatial domains or regions)
    Handle<Buffer> tileData;              // The buffer holding data for individual tiles
    Handle<Buffer> tileAddresses;         // The buffer storing addresses or indices for accessing tiles
    Handle<Buffer> dispatchIndirectArgs;  // The buffer used for indirect dispatch arguments
    float hSquare;                        // The square of the grid spacing (spatial step size squared)
    int numIterations;                    // The total number of Jacobi iterations to perform
};

class FluidRenderer {
public:
    FluidRenderer(const Device& device, const Camera& camera, GUI& gui);
    void Render(Handle<CommandList> cmdList);

    Handle<Texture> GetDebugTilesTexture() const;
    Handle<Texture> GetTileTagsTexture() const;
    Handle<Texture> GetTilesTexture() const;
    Handle<Texture> GetDensityTilesTexture() const { return mDensityTilesTexture; }
    

private:
    void RenderFluid(Handle<CommandList> cmdList, const FluidSimParams& params);
    void ClearTexture(Handle<Texture> texture);
    void ClearTiles(Handle<CommandList> cmdList, Handle<Texture> tilesTextureToClear, Handle<Buffer> tilesBuffer, Handle<Buffer> indirectDispatchArgs);
    void Jacobi(Handle<CommandList> cmdList, const JacobiParams& params);
    void VCycle(Handle<CommandList> cmdList, Handle<Texture> rhs, int level, int maxLevels);

    const Device& mDevice;
    const Camera& mCamera;
    GUI& mGui;

    // Textures
    Handle<Texture> mTilesTexture[kMaxNumLevels];
    Handle<Texture> mTileTagsTexture[kMaxNumLevels];
    Handle<Texture> mJacobiTilesTexture[kMaxNumLevels];
    Handle<Texture> mResidualTilesTexture[kMaxNumLevels];

    Handle<Texture> mGradientTilesTexture;
    Handle<Texture> mDivergenceTilesTexture;
    Handle<Texture> mTempTilesTexture;
    Handle<Texture> mDensityTilesTexture;
    Handle<Texture> mVelocityTilesTexture;
    Handle<Texture> mDensityAdvectedTilesTexture;
    Handle<Texture> mVelocityAdvectedTilesTexture;

    // Buffers
    Handle<Buffer> mDispatchIndirectArgsBuffer[kMaxNumLevels];
    Handle<Buffer> mCounterBuffer[kMaxNumLevels];
    Handle<Buffer> mTileDataBuffer[kMaxNumLevels];
    Handle<Buffer> mTileAddressesBuffer[kMaxNumLevels];

    Handle<Buffer> mActiveTilesBuffer;
    Handle<Buffer> mFreedTilesBuffer;
    Handle<Buffer> mActiveTileAddressesBuffer;

    // Pipelines
    Handle<Pipeline> mClearTexturePipeline;
    Handle<Pipeline> mInitTilesPipeline;
    Handle<Pipeline> mAllocateTilesPipeline;
    Handle<Pipeline> mGenerateIndirectDispatchArgsPipeline;
    Handle<Pipeline> mGenerateDensityTilesPipeline;
    Handle<Pipeline> mGenerateVelocityTilesPipeline;
    Handle<Pipeline> mAdvectTilesPipeline;
    Handle<Pipeline> mFreeTilesPipeline;
    Handle<Pipeline> mCommitTilesPipeline;
    Handle<Pipeline> mDilateTilesPipeline;
    Handle<Pipeline> mGenerateDivergenceTilesPipeline;
    Handle<Pipeline> mClearTilesPipeline;
    Handle<Pipeline> mGenerateJacobiTilesPipeline;
};