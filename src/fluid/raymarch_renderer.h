#pragma once

#include "structs.h"

class Device;
class Pipeline;
class CommandList;
class Texture;
class Camera;
class Buffer;

struct RenderArgs {
    Handle<Texture> tileTags;;
    Handle<Texture> densityTiles;
    Handle<Texture> tilesToRender;
    Handle<Texture> indirectionTiles;
    FluidSimParams params;
};

struct RenderVolumeArgs {
    const Texture& renderTarget;
    const Camera& camera;
    FluidSimParams params;
};

class RayMarchRenderer {
public:
    RayMarchRenderer(const Device& device);

    void Render(Handle<CommandList> cmdList, const RenderArgs& args);
    void RenderVolume(Handle<CommandList> cmdList, const RenderVolumeArgs& args);

    Handle<Texture> GetDebugRenderTarget() const { return mDebugRenderTarget; }

private:
    const Device& mDevice;
    Handle<Texture> mDebugRenderTarget;
    Handle<Pipeline> mPipeline;

    Handle<Buffer> mCubeVertexBuffer;
    Handle<Buffer> mCubeIndexBuffer;
    Handle<Pipeline> mRender3dPipeline;
};