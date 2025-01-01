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
constexpr int kTotalNumTiles = (kTextureSize / kTileSize) * (kTextureSize / kTileSize) * (kTextureSize / kTileSize);

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
};