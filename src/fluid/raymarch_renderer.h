#pragma once

#include "structs.h"

class Device;
class Pipeline;
class CommandList;
class Texture;

struct RenderArgs {
    Handle<Texture> tileTags;;
    Handle<Texture> densityTiles;
    Handle<Texture> tilesToRender;
    Handle<Texture> indirectionTiles;
    FluidSimParams params;
};

class RayMarchRenderer {
public:
    RayMarchRenderer(const Device& device);

    void Render(Handle<CommandList> cmdList, const RenderArgs& args);
    Handle<Texture> GetRenderTarget() const { return mRenderTarget; }

private:
    const Device& mDevice;
    Handle<Texture> mRenderTarget;
    Handle<Pipeline> mPipeline;
};